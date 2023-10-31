#ifndef __WEBSERV_H__
#define __WEBSERV_H__

class WebServ {

    protected: 
            AsyncWebServer _server;

   public:
        WebServ() :_server(80) {}

        void setup() 
        {

            _server.on("/",      HTTP_GET,  std::bind(&WebServ::onRootResponse,   this, std::placeholders::_1));
            _server.on("/switch", HTTP_GET, std::bind(&WebServ::onSwitchResponse, this, std::placeholders::_1));
            _server.onNotFound(             std::bind(&WebServ::showNotFound,     this, std::placeholders::_1));
        }

        void showNotFound(AsyncWebServerRequest *request) { request->send(404, "text/plain", "Web Page Not found"); }

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


        void onRootResponse(AsyncWebServerRequest *request)
        {
            request->send(200, "text/html", buildRootResponse());
        }

        void onSwitchResponse(AsyncWebServerRequest *request)
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


};

#endif //__WEBSERV_H__