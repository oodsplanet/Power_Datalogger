
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
  r+="<meta http-equiv=\"refresh\" content=\"2\">";
  r+="<link rel=\"icon\" href=\"data:,\">";
  // CSS to style the on/off buttons 
  // Feel free to change the background-color and font-size attributes to fit your preferences
  r+="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
  r+=".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;";
  r+="text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}";
  r+=".button2 {background-color: #555555;}</style></head>";
  
  // Web Page Heading
  r+="<body><h1>ESP32 Web Server for Voltcraft SEM6000</h1>";
  if (sem6000_device.isConnected()) r+=("<p>Deivce connected : YES </p>");
  else r+=("<p>Deivce connected : NO </p>");

  // measuremant data  
  r+=("<p>U=");  r+=(sem6000_device._lastMeasureData._voltage); r+=("V");
  r+=(",  P=");  r+=(sem6000_device._lastMeasureData._power);   r+=("kW");
  r+=(",  I=");  r+=(sem6000_device._lastMeasureData._current);  r+=("A");
  r+=("Comsumption= "); r+=(sem6000_device._lastMeasureData._consumption); r+=("kWh");
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

  //Display current run state, and ON/OFF buttons
  if (mainstate==MAINSTATES::idle)
  {
    r+=("<p><a href=\"/switch?plug=on\"><button class=\"button button2\">ON</button></a></p>");
    r+=("<p><a href=\"/switch?plug=off\"><button class=\"button\">OFF</button></a></p>");
  }
  else if (mainstate==MAINSTATES::measure_run)
  {
    r+=("<p><a href=\"/switch?plug=on\"><button class=\"button\">ON</button></a></p>");
    r+=("<p><a href=\"/switch?plug=off\"><button class=\"button button2\">OFF</button></a></p>");
  }
  else
  {
    r+=("<p><a href=\"/switch?plug=disabled\"><button class=\"button\">ON</button></a></p>");
    r+=("<p><a href=\"/switch?plug=disabled\"><button class=\"button\">OFF</button></a></p>");
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
    String inputMessage ="on";
    String inputParam ="plug";
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(inputParam)) {
      inputMessage = request->getParam(inputParam)->value();
    }

    if (inputMessage=="on")     
    {
      if (mainstate== idle) mainstate = measure_start;
    }
    else if (inputMessage="off")
    {
      if (mainstate==measure_run) mainstate = measure_stop;
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

  sdlogger.readConfigFile(sconfigfile, measureintervall_ms);

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
          //sem6000_device.send_OvrPwr1024();
          //sem6000_device.send_DisplayStrom();
          break;
      case MAINSTATES::idle    :          
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