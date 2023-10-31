#include <SDlogger.h>

SDLogger::SDLogger(SDFS& sd) : SDFS(sd) {}

void SDLogger::readConfigFileIntern(const String& filename,  String& logfileprefix, unsigned long& intervallms)
{
    #ifdef DEBUG_SDLOGGER
    Serial.println("read config name:"+filename);    
    #endif

    if (exists(filename))
    {
        File configfile;
        configfile = open(filename, FILE_READ);
        if (configfile)
        {
            #ifdef DEBUG_SDLOGGER            
            Serial.println("config opened => read the config-file");
            #endif
            String line = configfile.readStringUntil('\n');
            while (line.length()>0)
            {
                line.trim();
                #ifdef DEBUG_SDLOGGER
                Serial.print("read line "); Serial.println(line); 
                #endif
                if (line.startsWith("t:"))   // time in t:hh:mm:ss
                {                    
                    if (line.length()>=String("t:00:00:00").length() && line[4]==':' && line[7]==':')
                    {
                        intervallms = line.substring(2,4).toInt();
                        intervallms = intervallms*60 + line.substring(5,7).toInt();
                        intervallms = intervallms*60 + line.substring(8,10).toInt();
                        intervallms *= 1000;
                        #ifdef DEBUG_SDLOGGER
                         Serial.println("intervall ms:"+String(intervallms));
                        #endif
                    }
                }
                if (line.startsWith("f:")) // logfile f:filename
                {
                    logfileprefix = "/"+line.substring(2);
                    #ifdef DEBUG_SDLOGGER
                    Serial.println("config logfile: "+logfileprefix+";");
                    #endif
                }
                line = configfile.readStringUntil('\n');
            }
            configfile.close();
        }
        else
        {
            #ifdef DEBUG_SDLOGGER
            Serial.println("E: failed read config-file");
            #endif
        }
    }
    else {
      #ifdef DEBUG_SDLOGGER
      Serial.println("Error: file "+filename+ " does not exist!");
      #endif
    }
    if (intervallms<SDLOGGER_MININTERVALL) intervallms = SDLOGGER_MININTERVALL;
    if (logfileprefix == "") logfileprefix = SDLOGGER_DEFPRAEFIX;
}
void SDLogger::readConfigFile(const String& filename,   unsigned long& intervallms)
{
    readConfigFileIntern(filename,_logfilepraefix, intervallms);
}

bool SDLogger::createLogfileName(const String& prefix, String& fname)
{
    unsigned long maxnum=10;
    unsigned long i;
    for (i=prefix.length();i<7;i++) maxnum*=10;
    i=1;
    while (i<maxnum)
    {
        fname = prefix+String(i)+SDLOGGER_DEFEXT;
        #ifdef DEBUG_SDLOGGER
        Serial.print("try:"); Serial.println(fname);
        #endif
        if (exists(fname)) i++;
        else return true;
    }
    return false;
}

      // create and open logfile 
bool SDLogger::createLogfile()
{
      if (!createLogfileName(_logfilepraefix, _logfilename))
      {
          #ifdef  DEBUG_SDLOGGER
          Serial.println("E : filename creation error");
          #endif
          return false;
      }
      else 
      {
        _logfile=open(_logfilename,FILE_WRITE);
        if (!_logfile)
        {
            #ifdef  DEBUG_SDLOGGER
            Serial.println("error opening logfile"+logfilename);        
            Serial.flush();
            #endif
            return false;
        }
      }
      return true;
}


void SDLogger::closeLogfile()
{
    if (_logfile)    
    {
        #ifdef  DEBUG_SDLOGGER
        Serial.println("close udn flush logfile");        
        #endif
        _logfile.flush();
        _logfile.close();
    }
    else
    {
        #ifdef  DEBUG_SDLOGGER
        Serial.println("E: closeLogfile(): logfile was notopened!");        
        #endif
    }
}
