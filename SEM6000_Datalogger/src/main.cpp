
// #define DEBUG

// run as server to analyse app commands
// #define SNIFFER 

// debg SD-logging 
// #define DEBUG_LOG        

#include <Arduino.h>
#include <SDlogger.h>     
#include <ESPAsyncWebServer.h>
#include <SEM6000_BLE.h>
#include <softrtc.h>
// #include <WebServ.h>      // Async Webserver not implemented yet

// turn off other debugging 
#ifndef DEBUG
#undef DEBUG_LOG
#undef SNIFFER
#endif

#define USE_WIFI_AP                         // work as accesspoint

#pragma region global_variables
// SD card logger
#define pin_cs  22                          // datalogger SD select
const String sconfigfile = "/config.txt";   // hardcoded name config-file on SD card
const uint32_t spiFrequency = 10000000;     // SD 10 MHz

// Wifi
AsyncWebServer server(80);
//WebServ server;                           // not mplemented yet
#ifdef USE_WIFI_AP
  const char* ssid     = "SEM6000-Datalogger";
#else
  const char* ssid     = "NETGEAR51";
#endif

// the SEM6000 device handling class
SEM6000_BLE sem6000_device;

// main statemachine
enum MAINSTATES { initsem=0, idle, measure_start, measure_run, measure_stop };
int mainstate = MAINSTATES::initsem;
 
unsigned long measureintervall_ms = 5000;  // each 5000 sec ask for meaurement
unsigned long last_meaurement_ms  = 0;

SoftRTC  rtc;
SDLogger sdlogger(SD);

#pragma endregion

#pragma region Webserver
void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Web Page Not found");
}

String buildRootResponse()
{
  String r="";  
  r+="<!DOCTYPE html><html>";
  r+="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  // refresh when initialising or measurement is running
  if (mainstate != MAINSTATES::idle)
    r+="<meta http-equiv=\"refresh\" content=\"2\">";
  r+="<link rel=\"icon\" href=\"data:,\">";
  // CSS to style the on/off buttons 
  // Feel free to change the background-color and font-size attributes to fit your preferences
  //r+="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; font-size: 16px; text-align: center;}";
      r += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; font-size: 20px; font-weight: bold; } h1 { font-size: 24px; }";
    r += "        .time-container { align-items: center; text-align: center; }\n";
    r += "        .time-label { margin-right: 5px; text-align: center; }\n";
    r += "        .time-input { width: 3em; text-align: right; }\n";
    r += "        .file-input { width: 6em; text-align: left; }\n";
    r += "        .start-button { background-color: green; color: white; padding: 10px 20px; ";
    r += "                        border: 6px groove #888; border-radius: 5px; box-shadow: 0px 3px 0px #555; ";
    r += "                        font-weight: bold; text-transform: uppercase; }\n";
    r += "        .stop-button { background-color: red; color: white; padding: 10px 20px; ";
    r += "                        border: 6px groove #888; border-radius: 5px; box-shadow: 0px 3px 0px #555; ";
    r += "                        font-weight: bold; text-transform: uppercase; }\n";
  r+="text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}";
  r+=".button2 {background-color: #555555;}</style></head>";
  
  // Web Page Heading
  r+="<body><h1>ESP32 Web Server for Voltcraft SEM6000</h1>";
  if (sem6000_device.isConnected()) r+=("<p>Deivce connected : YES </p>");
  else r+=("<p>Deivce connected : NO </p>");

  // measuremant data  
  r+=("<p>U=");  r+=(sem6000_device._lastMeasureData._voltage); r+=("V");
  r+=(",  P=");  r+=(sem6000_device._lastMeasureData._power);   r+=("W");
  r+=(",  I=");  r+=(sem6000_device._lastMeasureData._current);  r+=("A");
  r+=(" Con="); r+=(sem6000_device._lastMeasureData._consumption); r+=("kWh");
  r+=("</p>");

  r+="<p>Meaurement status: ";
  switch (mainstate) 
  {
    case MAINSTATES::initsem     : r+="initializing SEM6000</p>"; break;
    case MAINSTATES::idle        : r+="idle, ready </p>"; break;
    case MAINSTATES::measure_run : r+="meaurement running</p>"; break;    
    case MAINSTATES::measure_start : r+="starting measurement</p>"; break;    
    case MAINSTATES::measure_stop : r+="ending measurement</p>"; break;    
    default: r+="ERROR: unknown state</p>"; break;    
  }
  r+=("</p>");
  uint8_t sec = (measureintervall_ms/1000)%60;
  uint8_t min = measureintervall_ms/60000;
  r+=("</p>");

    if (mainstate==MAINSTATES::idle)
    {
      // if measurement idle show entry-fields for timeintervall 
      r += "<form action='/times' method='get' id='times-form'>\n";
      r += "    <div class='time-container'>\n";
      r += "        <label class='time-label' for='minutes'>Interval Time:</label>\n";
      r += "        <input type='number' id='minutes' name='minutes' min='0' max='59' value='" +String(min)+"' class='time-input' size='2' inputmode='numeric'>\n";
      r += "        <input type='number' id='seconds' name='seconds' min='1' max='59' value='" +String(sec)+"' class='time-input' size='2' inputmode='numeric'>\n";
      r += "    </div>\n";      
      r += "</br>";
      r += "</form>\n";

      r += "<form action='/switch' method='get' id='switch-form'>\n";
      r += "    <input type='hidden' id='intervall-value' name='intervall' value=''>\n";
      r += "    <input type='hidden' id='button-value' name='button' value=''>\n";
      r += "        <label class='time-label' for='filename'>filename (1-7 characters):</label>\n";
      r += "        <input type='text' id='filename' name='filename' pattern='[a-z]{1,7}'";
      r += " value='"+ sdlogger.logfilepraefix() + "' title='Lowercase letters (a-z) only, 1..7 characters.' class='file-input' maxlength='7' inputmode='verbatim'>\n";
      r += "</br></br>";
      r += "    <button type='button' class='start-button' onclick='setButtonAndTime(\"START\")'>Start</button>\n";
      r += "</form>\n";

      r += "<script>\n";
      r += "    function setButtonAndTime(buttonValue) {\n";
      r += "        const minutesValue = document.getElementById('minutes').value;\n";
      r += "        const secondsValue = document.getElementById('seconds').value;\n";
      r += "        const intervall =  (parseInt(minutesValue) * 60) + parseInt(secondsValue);\n";
      r += "        document.getElementById('button-value').value = buttonValue;\n";
      r += "        document.getElementById('intervall-value').value = intervall;\n";
      r += "        const filenameValue = document.getElementById('filename').value;\n";
      r += "        if (filenameValue.length > 0) {\n";
      r += "            document.getElementById('filename').value = filenameValue.toLowerCase();\n";
      r += "        }\n";
      r += "        const form = document.getElementById('switch-form');\n";
      r += "        form.submit();\n";
      r += "    }\n";
      r += "</script>\n";
    }
    else
    {
      r += "Mesurment intervall: "+String(min)+":"+ String(sec)+" s";
      r += "    <form action=\"/switch\" method=\"get\" id=\"switch-form\">\n";
      r += "        <button type=\"button\" class=\"stop-button\" onclick=\"setButtonAndTime('STOP')\">Stop</button>\n";
      r += "    </form>\n";

      r += "    <script>\n";
      r += "        function setButtonAndTime(buttonValue) {\n";
      r += "            const form = document.getElementById('switch-form');\n";
      r += "            const customURL = `/switch?button=${buttonValue}`;\n";
      r += "            window.location.href = customURL;\n";
      r += "        }\n";
      r += "    </script>\n";      

    }


  r+=("</body></html>");
  return r;
}

void SendRootResponse(AsyncWebServerRequest *request)
{
    request->send(200, "text/html", buildRootResponse());
}
void SendSwitchResponse(AsyncWebServerRequest *request)
{
  String buttonParam = "button";
  if (request->hasParam(buttonParam)) {
    String buttonValue = request->getParam(buttonParam)->value();

    if (mainstate == idle) {
      String intervallParam = "intervall";
      if (request->hasParam(intervallParam)) {
        measureintervall_ms = request->getParam(intervallParam)->value().toInt() * 1000;
      }

      String filenameParam = "filename";
      if (request->hasParam(filenameParam)) {
        sdlogger.logfilepraefix() = request->getParam(filenameParam)->value();
      }

      if (buttonValue == "START") mainstate = measure_start;
    } else if (mainstate == measure_run && buttonValue == "STOP") {
      mainstate = measure_stop;
    }
  }

  request->send(200, "text/html", buildRootResponse());
}
#pragma endregion


void setup() {
  Serial.begin(9600);

  SPI.begin();
  SPI.setFrequency(spiFrequency); // Set the SPI frequency
  // begin without options causes problem with wifi - dont know why     
  if (sdlogger.begin(pin_cs,SPI,4000000U,"/sd",2))
    Serial.println("SD Started");
  else Serial.println("SD ERROR");

  #ifdef DEBUG_LOG
    Serial.println("read config file");
  #endif

//  sdlogger.readConfigFile(sconfigfile, measureintervall_ms);

  #ifdef DEBUG_LOG
   Serial.println("read config file done");
  #endif

  #ifdef USE_WIFI_AP
    // setup as AP
    Serial.print("Setting AP (Access Point)…"); 
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.persistent(false);
    WiFi.softAP(ssid, NULL,1);
    WiFi.mode(WIFI_MODE_AP);
    IPAddress IP = WiFi.softAPIP();  Serial.print("AP IP address: "); Serial.println(IP);

    #ifdef DEBUG
    Serial.println("WiFi connected");
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
  #endif

  #else
    // setup as STATION via WiFirouter 
    // Connect to Wi-Fi network with SSID and password
    #ifdef DEBUG
    Serial.print("Setting up in STA mode");
    #endif
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid,"router-WiFi-password");
    while (!WiFi.isConnected()) { delay(1000); }

    #ifdef DEBUG
    Serial.println("WiFi connected");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    #endif
  #endif

  server.on("/",          HTTP_GET,  [](AsyncWebServerRequest *request) { SendRootResponse(request);    } );
  server.on("/switch",    HTTP_GET,  [](AsyncWebServerRequest *request) { SendSwitchResponse(request);  } );
  server.onNotFound(notFound);
  server.begin();

  #ifdef SNIFFER
  sem6000_device.startServer();
  #else
  sem6000_device.tryConnect(); // start the scan
  #endif

}

void loop() {

  // usaually run as a client 
  #ifndef SNIFFER
  
  sem6000_device.Task();

  if (sem6000_device.state()== SEM6000_BLE::STATES::pre_connected)
  {
    Serial.println("send authentify");
    sem6000_device.send_Authentify();
  } // if pre-connected 
  
 if (sem6000_device.state() == SEM6000_BLE::STATES::connected)
 {    
    switch (mainstate) 
    {
      case MAINSTATES::initsem : 
          mainstate = MAINSTATES::idle;
            sem6000_device.send_LEDOnOff(true);
            sem6000_device.send_LEDringGreen();    
          break;
      case MAINSTATES::idle    :          
          //sem6000_device.send_LEDOnOff(millis() & 0x1);
          break;
      case MAINSTATES::measure_start :
          Serial.println("measure_start");
          // create and open logfile 
          if (!sdlogger.createLogfile())
          {
              Serial.println("E : file creation error");
          }
          else
          {
            sem6000_device.send_LEDOnOff(true);      
            last_meaurement_ms = millis()-measureintervall_ms;
            mainstate=MAINSTATES::measure_run;
          }
          break;
      case MAINSTATES::measure_stop:
          Serial.println("measure_stop");
          sem6000_device.send_LEDOnOff(false);
          mainstate = MAINSTATES::idle;
          sdlogger.closeLogfile();
          break;
      case MAINSTATES::measure_run:
        if ((millis() - last_meaurement_ms)>measureintervall_ms)
        {
          sem6000_device._lastMeasureData.Dump();
      
          String msg = String(rtc.getTimeSeconds()) + " " +
             String(sem6000_device._lastMeasureData._voltage,0) + " " +
             String(sem6000_device._lastMeasureData._power,0);
          #ifdef DEBUG_LOG
          Serial.println(msg);
          #endif
          sdlogger.logfile().println(msg);
          sdlogger.logfile().flush();

          Serial.println("send measure");
            last_meaurement_ms = millis();
            sem6000_device.send_requestActualMeasurement();
        }
    } // switch 
 } // if SEM6000_BLE::STATES::connected

 #endif // run es client
  
  
} // loop 