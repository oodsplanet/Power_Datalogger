#include <SEM6000_BLE.h>

SEM6000_AdvertisedDeviceCB::SEM6000_AdvertisedDeviceCB()     
{    
    _pServerAddress  = nullptr;
    _doConnect       = false;
}


void SEM6000_AdvertisedDeviceCB::onResult(BLEAdvertisedDevice advertisedDevice)
{      
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    if (advertisedDevice.getName() == "Voltcraft")
    {
        auto addr = advertisedDevice.getAddress();
        Serial.print("BLE dev. Adress: "); Serial.println(addr.toString().c_str());
        advertisedDevice.getScan()->stop();      
        //Scan can be stopped, we found what we are looking for
        _pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
        _doConnect = true; //Set indicator, stating that we are ready to connect
        Serial.println("Device found. Connecting!");        
    }
    else _doConnect = false;
}

State_timer::State_timer() : 
    _running(false),
    _countdown_ms(0),
    _start_time(0)
{
}

void State_timer::start(unsigned long coundown_time)
{
    _start_time = millis();
    _countdown_ms = coundown_time;   
    _running = true;
    //Serial.println("... timer start");
}

bool State_timer::isRunning()
{
    if (_running)
    {
        // Serial.print(".. tstart="); Serial.print(_start_time);
        // Serial.print(" coundwn ="); Serial.print(_countdown_ms);
        // Serial.print(" dif== "); Serial.println((millis()-_start_time));
        _running = ( (millis()-_start_time) < _countdown_ms);
    }
    return _running;
}
void State_timer::stop() 
{
    _running = false;
    //Serial.println("... timer stop");
}

std::unique_ptr<SEM6000_Response> SEM6000_BLE::_SEMresponse = std::make_unique<SEM6000_Response>(); 

SEM6000_BLE::SEM6000_BLE() : 
    SEM6000_UUID_Service(_SEM6000_UUID_SERVICE),
    SEM6000_UUID_Control(_SEM6000_UUID_CONTROL),
    SEM6000_UUID_Response(_SEM6000_UUID_RESPONSE),
    SEM6000_UUID_Name(_SEM6000_UUID_NAME),
    _state(STATES::off)

{
}

SEM6000_BLE::STATES SEM6000_BLE::state() const
{
    return _state;
}

bool SEM6000_BLE::isConnected() const
{
    return _state == STATES::connected;
}

/// @brief  : state-machine step, has to be called from owner in loop()
void SEM6000_BLE::Task()
{
    if (_state == STATES::off)  // nothing to change 
        return;

    // check timer an change states
    switch (_state)
    {
        case STATES::scanning: // connect if scan was sucessfull
            if (_statetimer.isRunning())
            {
                Serial.println("scanned");
                if (connectToServer())
                {
                    Serial.println("pre-connected ");
                    stopBLEScan();
                    _state =STATES::pre_connected;
                    Serial.println("pre-connected set");
                }
                else _state = STATES::off;
            }
            else 
            {
                stopBLEScan();         
                // if (_pBLEScan->getResults().getCount()>0)
                //     _state= STATES::select_device;
                 _state=STATES::off;
            }
        break;  
        case STATES::authentify:
            if (_statetimer.isRunning()) 
            {                
                // check for respone 
                if (_SEMresponse->isCmd(SEMCMD_AUTHENTIFY))
                {
                    Serial.println("check Cmd Authetify is OK");
                    SEM6000_Response_Authentify* resp= static_cast<SEM6000_Response_Authentify*>(_SEMresponse.get());
                    if (resp->success()) 
                    {
                        _state = STATES::connected;
                        _SEMresponse = std::make_unique<SEM6000_Response>();
                    }
                    _statetimer.stop();
                }
            }
            else  // authetification fialed 
            {
                Serial.println("Authetification failed - timer nt unning");
                _state = STATES::off;
            }
        break;
        case STATES::connected:
        {
            if (_statetimer.isRunning())  // waiting for an resopnse 
            {
                switch (_SEMresponse->getCmd())
                {
                case SEMCMD_ACTUALMEAS:
                    //Serial.println("got actual measurement");
                     _statetimer.stop();
                    SEM6000_Response_Measurement* resp= static_cast<SEM6000_Response_Measurement*>(_SEMresponse.get());
                    _lastMeasureData = resp->data;
                    break;
                
                }
            }            
        }
        break;
    }
}

#ifdef DEV_HARDCODED_TEST
void SEM6000_BLE::startBLEScan()
{
  BLEDevice::init("ESP32_SEM6000");
  //_pBLEScan = BLEDevice::getScan(); //create new scan
  // if (!_serviceCB) _serviceCB = new SEM6000_AdvertisedDeviceCB(); // !! #ptr
  _pBLEScan = nullptr;
  if (!_serviceCB) _serviceCB = new SEM6000_AdvertisedDeviceCB(); // !! #ptr

//   _pBLEScan->setAdvertisedDeviceCallbacks(_serviceCB);
//   _pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
//   _pBLEScan->start(_scanTime);

    Serial.printf("Testing Hardcoded  Device: %s \n", "Voltcraft");
    String addr(_SEM600_ADRESS);
    Serial.println("BLE dev. Adress: "+addr); 
    _serviceCB->_pServerAddress = new BLEAddress(addr.c_str()); //Address of advertiser is the one we need
    _serviceCB->_doConnect = true; //Set indicator, stating that we are ready to connect

  _state = STATES::scanning;
  _statetimer.start(_scanTime*1000);   // ms 

}
#else
void SEM6000_BLE::startBLEScan()
{
  BLEDevice::init("ESP32_SEM6000");
  _pBLEScan = BLEDevice::getScan(); //create new scan
  if (!_serviceCB) _serviceCB = new SEM6000_AdvertisedDeviceCB(); // !! #ptr
  _pBLEScan->setAdvertisedDeviceCallbacks(_serviceCB);
  _pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  _pBLEScan->start(_scanTime);
  _state = STATES::scanning;
  _statetimer.start(_scanTime*1000);   // ms 
}
#endif


void SEM6000_BLE::stopBLEScan()
{
    Serial.println(" _pBLEScan->stop(); ");
    if (_pBLEScan!= nullptr)
    {
        //Serial.println(" _pBLEScan->stop(); ");
        _pBLEScan->stop();
        _pBLEScan->setAdvertisedDeviceCallbacks(nullptr);
        _pBLEScan = nullptr; // singletone
    }
    _statetimer.stop();
}

//When the BLE Server sends a new humidity reading with the notify property
void SEM6000_BLE::ResponseNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {

    #ifdef DEBUG_RESPONSE                                   
    Serial.println("ResponseNotify");
    Serial.print("length= "); Serial.print(length);
    Serial.print("  isNotify= "); Serial.print(isNotify);
    Serial.println();
    if (length>2)
    {

        uint8_t* pV = pData;
        uint8_t len = 20; //pV[1]+2;
        while (len)
        {
            uint8_t v = *pV;
            {Serial.print(v,HEX); Serial.print(","); }
            len--; pV++;
        }  
        Serial.println("--- End ---");
    }
    #endif

    #ifndef SNIFFER    
    _SEMresponse = SEM6000_ResponseFactory::FromBuffer(pData,length,isNotify);
    #endif
}

void SEM6000_BLE::DumpCharacteristicProperties(BLERemoteCharacteristic* blechar) 
{
    Serial.print("All char data:"); Serial.println(blechar->toString().c_str());
}

//Connect to the BLE Server that has the name, Service, and Characteristics
bool SEM6000_BLE::connectToServer() 
{
    if (_serviceCB == nullptr) _state = STATES::off;
    if (!_serviceCB->_doConnect) return false;
    if (_serviceCB->_pServerAddress == nullptr) return false;

    BLEClient* pClient = BLEDevice::createClient();
 
    // Connect to the remove BLE Server.
    pClient->connect(*_serviceCB->_pServerAddress);
    #ifdef DEBUG
    Serial.println(" - Connected to server"); 
    #endif
    _state = STATES::pre_connected;
    _serviceCB->_doConnect =false;

    // Obtain a reference to the service we are after in the remote BLE server. (UUID from device-scan)
    BLERemoteService* pRemoteService = pClient->getService(SEM6000_UUID_Service);
    if (pRemoteService == nullptr) {
        #ifdef DEBUG
        Serial.print("Failed to find our service UUID: ");
        Serial.println(SEM6000_UUID_Service.toString().c_str());
        #endif
        _state = STATES::off;
        return false;
    }

    // Obtain a reference to the characteristics in the service of the remote BLE server.
    _controlCharacteristic  = pRemoteService->getCharacteristic(SEM6000_UUID_Control);
    _responseCharacteristic = pRemoteService->getCharacteristic(SEM6000_UUID_Response);
    _nameCharacteristic     = pRemoteService->getCharacteristic(SEM6000_UUID_Name);

    if (_controlCharacteristic == nullptr || _responseCharacteristic== nullptr)
    {
        _state= STATES::off;
        pClient->disconnect();
        return false;   
    }

    #ifdef DEBUG
    // debug characteristiccs:
    Serial.println("Properties: Controlcharacteristic:");
    DumpCharacteristicProperties(_controlCharacteristic);
    Serial.println("Properties: ResponseCharacteristic:");
    DumpCharacteristicProperties(_responseCharacteristic);
    Serial.println("Properties: NameCharacteristic:");
    DumpCharacteristicProperties(_nameCharacteristic);
    #endif

    _responseCharacteristic->registerForNotify(SEM6000_BLE::ResponseNotifyCallback);

    return true;
}


/// @brief start scan+connect sequence
void SEM6000_BLE::tryConnect()
{
    if (_state==STATES::off)
            startBLEScan();
}

bool SEM6000_BLE::readyForCommand() const
{
    return 
    (_state == STATES::connected) && 
    (_controlCharacteristic != nullptr);
}

void SEM6000_BLE::send_LEDOn()
{
    if (readyForCommand())
     _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdLED1, 13, false);    
}
void SEM6000_BLE::send_LEDOff()
{
    if (readyForCommand())
     _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdLED0, 13, false);    
}
void SEM6000_BLE::send_LEDOnOff(bool on)
{
    if (on) send_LEDOn();
    else send_LEDOff();
}

// void SEM6000_BLE::send_DisplayStrom()
// {
//     if (readyForCommand())
//      _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdOnDisplayStrom, 9, false);    
// }

void SEM6000_BLE::send_turnOnOff(bool on)
{
    if (on) send_turnOn();
        else send_turnOff();
}

void SEM6000_BLE::send_turnOff()
{
    if (readyForCommand())
     _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdSWITCH0, 10, false);    
}

void SEM6000_BLE::send_turnOn()
{
    if (readyForCommand())
     _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdSWITCH1, 10, false);
    
}

void SEM6000_BLE::send_requestActualMeasurement()
{
    if (readyForCommand())
    {
        #ifdef DEBUG
        Serial.println("REQ POWER");
        #endif
        _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdREQPOW, 10, false);   
        _statetimer.start(1500);
    }
}
void SEM6000_BLE::send_Authentify()
{
    if ((_state == STATES::pre_connected) && 
        (_controlCharacteristic != nullptr))
    {
        #ifdef DEBUG
        Serial.println("CMD Authentifivcation");
        #endif
        _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdAuto, 16, false);
        _state=STATES::authentify;
        _statetimer.start(2000); // run for 2s
    }
}
void SEM6000_BLE::send_LEDringGreen()
{
    if (readyForCommand())
     _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdLEDGreen, 16, false);
    
}

// void SEM6000_BLE::send_OvrPwr1024()
// {
//     if (readyForCommand())
//      _controlCharacteristic->writeValue((uint8_t*)SEM6000_cmdOvrPwr1024, 11, false);    
// }

// SNIFFER , SERVER ==================================================================

#ifdef SNIFFER
void SEM6000_ControlCharacteristicCallback::onWrite(BLECharacteristic* pCharacteristic) 
{
    std::string value = pCharacteristic->getValue(); // Get the value sent by the client
    Serial.println("Char callback got value:");
    // Dump the value
    for (size_t i = 0; i < value.length(); i++) {
        if (i % 16 == 0) {
            Serial.println();  // Start a new line every 16 bytes
        }
        Serial.print(String(value[i], HEX));
        Serial.print(" ");  // Separate hexadecimal values with a space
    }
    Serial.println();  // Print a newline at the end}
}

// SERVER meethods

void SEM6000_ServerCallbacks::onConnect(BLEServer* pServer)
{
    Serial.println("SERVER:: connected");
}

void SEM6000_ServerCallbacks::onDisconnect(BLEServer* pServer) 
{ 
    Serial.println("SERVER:: dis-connected");
}

/// @brief 
void SEM6000_BLE::startServer()
{
    BLEDevice::init(_SEM6000_SERVER_NAME);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new SEM6000_ServerCallbacks());

    //Create the BLE Service of the device
    pbmeService = pServer->createService(_SEM6000_UUID_SERVICE);

    // pservNameCharacteristic = 
    //         pbmeService->createCharacteristic(_SEM6000_UUID_NAME,BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE_NR);
    pservControlCharacteristic = 
            pbmeService->createCharacteristic(_SEM6000_UUID_CONTROL,BLECharacteristic::PROPERTY_WRITE_NR);
    pservResponseCharacteristic = 
            pbmeService->createCharacteristic(_SEM6000_UUID_RESPONSE,BLECharacteristic::PROPERTY_NOTIFY);

    //pservNameCharacteristic->setValue("\x56\x4f\x4c\x43\x46\x54\x04\x00\x00\x00\x00\x01\x16\x03\x00\x14");

    //Create a CCC descriptor for notifications.
    pResponseDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
    // Set the CCC descriptor value to enable notifications.
    uint8_t cccInitialValue[] = {0x01, 0x00};
    pResponseDescriptor->setValue(cccInitialValue, 2);
    pservResponseCharacteristic->addDescriptor(pResponseDescriptor);

    pservControlCharacteristic->setCallbacks(new SEM6000_ControlCharacteristicCallback());

    pbmeService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(_SEM6000_UUID_SERVICE);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x12);  // functions that help with  connections issue
    pServer->getAdvertising()->start();
    //BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify..."); 

}
#endif 

// end of file SEM6000_BLE.cpp
