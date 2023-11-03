#ifndef __SDLOGGER_H__
#define __SDLOGGER_H__

#include <SD.h>

#define SDLOGGER_MININTERVALL 1000
#define SDLOGGER_DEFPRAEFIX "log"
#define SDLOGGER_DEFEXT ".txt"

/// @brief wrapper class for logging system o SD-card
/// @details filesystem is DOS ( 8.3 max. 8 char name; 3 char extension )
class SDLogger : public SDFS
{
    private:
    File   _logfile;             // output file 
    String _logfilename;         // name output file 
    String _logfilepraefix;      // logfile praefix 

    private:
    /// @brief create logger filename (<name000.ext>)
    /// @param prefix text the name is starting, max 7 chars (at least one digit for number)
    /// @param fname returned filename
    /// @return true on succes, false on cannot create a new name 
    bool createLogfileName(const String& prefix, String& fname);

    /// @brief read configuration file from SD-card
    /// @param filename name of configuration file (eg. config.txt)
    /// @param logfileprefix prefix of logfilename to use 
    /// @param intervallms loggingintervall in ms
    void readConfigFileIntern(const String& filename,String& logfileprefix, unsigned long& intervallms);


    public:
    /// @brief construct SD-filesystem 
    /// @param sd use global SD object from SD.h library 
    SDLogger(SDFS& sd);    

    File& logfile() { return _logfile; }
    String& logfilepraefix() { return _logfilepraefix; };      // logfile praefix 
    
    /// @brief read configuration file from SD-card
    /// @param filename name of configuration file (eg. config.txt)
    /// @param intervallms loggingintervall in ms
    void readConfigFile(const String& filename, unsigned long& intervallms);

    /// @brief create and opens logger filename (<name000.ext>)
    /// @return true on succes, false on cannot create a new name 
    bool createLogfile();

    /// @brief flush and close the logfile
    void closeLogfile();

};

#endif // __SDLOGGER_H__