#ifndef __SEM6000_BLE__
#define __SEM6000_BLE__

#ifdef DEBUG
#define DEBUG_RESPONSE 
// #define DEV_HARDCODED_TEST
#endif

#undef SNIFFER

#include <Arduino.h>
#include <memory>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SEM6000_Response.h>

// Voltcraft 
// Advertised Device: Name:     Voltcraft
//                    Address:  a3:00:00:00:4c:fa
//                    manufacturer data: 0000fa4c000000a3
//                    serviceUUID:  0000fff0-0000-1000-8000-00805f9b34fb
//                    rssi:         -76

#define _SEM6000_SERVER_NAME "Voltcraft"

// try check with hardcoded Adress instead of scan
#ifdef DEV_HARDCODED_TEST
#define _SEM600_ADRESS "a3:00:00:00:4c:fa"
#endif

#define _SEM6000_UUID_SERVICE  "0000fff0-0000-1000-8000-00805f9b34fb"
#define _SEM6000_UUID_NAME     "0000fff1-0000-1000-8000-00805f9b34fb"
#define _SEM6000_UUID_CONTROL  "0000fff3-0000-1000-8000-00805f9b34fb"
#define _SEM6000_UUID_RESPONSE "0000fff4-0000-1000-8000-00805f9b34fb"

// comamnds written to control characteristic
// turn socket power on / off
const uint8_t SEM6000_cmdSWITCH0[] = {0x0f, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x04, 0xff, 0xff};
const uint8_t SEM6000_cmdSWITCH1[] = {0x0f, 0x06, 0x03, 0x00, 0x01, 0x00, 0x00, 0x05, 0xff, 0xff};

// turn LED ring on / off (inverse to night mode)
const uint8_t SEM6000_cmdLED1[]    = {0x0f, 0x09, 0x0f, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x16, 0xff, 0xff};
const uint8_t SEM6000_cmdLED0[]    = {0x0f, 0x09, 0x0f, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0xff, 0xff};

// request actual power and consumption
const uint8_t SEM6000_cmdREQPOW[]  = {0x0f, 0x05, 0x04, 0x00, 0x00, 0x00, 0x05, 0xff, 0xff};

// unknown commands, just for testing
// const uint8_t SEM6000_cmdOnDisplayStrom[]  = {0x0f, 0x05, 0x0C, 0x00, 0x00, 0x00, 0x0D, 0xff, 0xff}; //??
// const uint8_t SEM6000_cmdOvrPwr1024[] = {0x0f, 0x07, 0x05, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0a, 0xff, 0xff}; // 11
// const uint8_t SEM6000_cmdREQUSET[] = {0x0f, 0x05, 0x10, 0x00, 0x00, 0x00, 0x11, 0xff, 0xff};

// authentification?
const uint8_t SEM6000_cmdAuto[] = { 0x0f, 0x0c, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xff, 0xff };
// turns LED ring into green (dont know whta it really does)
const uint8_t SEM6000_cmdLEDGreen[] = { 0x0f, 0x0c, 0x01, 0x00, 0x1c, 0x24, 0x14, 0x1e, 0x0a, 0x07, 0xe7 ,0x00 ,0x00, 0x6c, 0xff, 0xff }; 

class State_timer
{
  private:
    unsigned long _start_time;
    unsigned long _countdown_ms;
    bool _running;

  public:
    State_timer();
    void start(unsigned long coundown_time);
    void stop();
    bool isRunning();
};

class SEM6000_AdvertisedDeviceCB: public BLEAdvertisedDeviceCallbacks 
{
    public:
        SEM6000_AdvertisedDeviceCB();
        BLEAddress* _pServerAddress;
        //Flags stating if should begin connecting and if the connection is up
        bool _doConnect;

    private:
        virtual void onResult(BLEAdvertisedDevice advertisedDevice);      
};

class SEM6000_ControlCharacteristicCallback : public BLECharacteristicCallbacks 
{
    private:
      void onWrite(BLECharacteristic* pCharacteristic);
};

#ifdef SNIFFER
class SEM6000_ServerCallbacks: public BLEServerCallbacks 
{
  virtual void onConnect(BLEServer* pServer);
  virtual void onDisconnect(BLEServer* pServer);
};
#endif

class SEM6000_BLE
{
    public:

    enum STATES { off=0 , scanning, select_device, pre_connected,authentify,connected,wait_response };
    static std::unique_ptr<SEM6000_Response> _SEMresponse; 
    MeasurementData _lastMeasureData;

    private: 

    // // BLE Characteristic-UUID
    BLEUUID SEM6000_UUID_Name;      // "0000fff1-0000-1000-8000-00805f9b34fb";
    BLEUUID SEM6000_UUID_Control;   // "0000fff3-0000-1000-8000-00805f9b34fb";
    BLEUUID SEM6000_UUID_Response;  // "0000fff4-0000-1000-8000-00805f9b34fb";
    BLEUUID SEM6000_UUID_Service; 

    BLERemoteCharacteristic* _responseCharacteristic = nullptr;
    BLERemoteCharacteristic* _controlCharacteristic = nullptr;
    BLERemoteCharacteristic* _nameCharacteristic = nullptr;

    // state machine 
    STATES _state;
    State_timer _statetimer;

    #ifdef SNIFFER
    // Create the BLE Server
    BLEServer* pServer;
    BLEService* pbmeService;
    BLECharacteristic* pservResponseCharacteristic;
    BLECharacteristic* pservControlCharacteristic;
    BLECharacteristic* pservNameCharacteristic;
    BLEDescriptor* pResponseDescriptor;
    #endif 

    // device scanning 
    // response to last command-static becuas of set in callback
    int _scanTime = 5; //In seconds
    BLEScan* _pBLEScan = nullptr;
    SEM6000_AdvertisedDeviceCB* _serviceCB = nullptr;

    bool connectToServer();

  private:

    static void ResponseNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

    void DumpCharacteristicProperties(BLERemoteCharacteristic* blechar); 

    void startBLEScan();    
    void stopBLEScan();    

    /// @brief  is device waitng for comamnds
    /// @return true / false
    bool readyForCommand() const;

  public:
    SEM6000_BLE();    
    /// @brief client state-machine step , has to be included in main loop()
    void Task();

    /// @brief actual machine-state 
    /// @return state
    STATES state() const;
    void tryConnect();
    bool isConnected() const;

    /// @brief turn LED ring on (night mode off)
    void send_LEDOn();
    /// @brief turn LED ring off (night mode on)
    void send_LEDOff();
    /// @brief turn LED ring on or off     
    void send_LEDOnOff(bool on);

    // unimplemented - we dont know what tehse commands really do
    // void send_DisplayStrom();
    // void send_OvrPwr1024();

    /// @brief turn LED-ring to green light
    void send_LEDringGreen();

    void send_requestActualMeasurement();
    void send_Authentify();

    /// @brief turns socket power on or off
    /// @param on 
    void send_turnOnOff(bool on);
    void send_turnOff();
    void send_turnOn();

    #ifdef SNIFFER
    // start as a server
    void startServer(); 
    #endif

};

#endif // __SEM6000_BLE__