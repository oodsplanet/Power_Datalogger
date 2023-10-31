#ifndef __SEM6000_RESPONSE__
#define __SEM6000_RESPONSE__
#include <Arduino.h>
#include <memory>
#include <SEM6000_cmds.h>

class MeasurementData
{
    public:
        bool  _power_on;
        float _power;       // Watt
        float _voltage;     // volt
        float _current;      // Amps        
        float _frequ;       // Hz   
        float _consumption; // kwH

    public:                
        void Dump() const;
};

class SEM6000_Response
{
    private:
        size_t  _length;    // response data length
        uint8_t _cmd;       // response command

    private:
       virtual void Init(); 
    protected: 
       float stream2Float(const uint8_t* p, uint8_t bytes) const;

    public:
    SEM6000_Response();
    SEM6000_Response(const uint8_t* const pData);
    bool isCmd(uint8_t cmd) const;
    uint8_t getCmd() const;
};

// Notification handle = 0x002e value: 0f 06 17 00 00 00 00 18 ff ff
//                                     |  |  |     |  |     |  + static end sequence of message, 0xffff
//                                     |  |  |     |  |     + checksum byte starting with length-byte, ending w/ byte before
//                                     |  |  |     |  + always 0x0000
//                                     |  |  |     + 0 = success, 1 = unsuccess
//                                     |  |  + Login command 0x1700
//                                     |  + Length of payload starting w/ next byte incl. checksum
//                                     + static start sequence for message, 0x0f
class SEM6000_Response_Authentify : public SEM6000_Response
{
        bool _no_sucess;
    public:
        SEM6000_Response_Authentify(const uint8_t* const pData);
        bool success() const;
};
//                                     0F 0F 04 00 01,00,00,00,E5,00,00,32,01,00,00,00,01 44 
// Notification handle = 0x002e value: 0f 11 04 00 01 00 00 00 eb 00 0c 32 00 00 00 00 00 00 2f
//                                     |  |  |     |  |        |  |     |        |           |  
//                                     |  |  |     |  |        |  |     |        |           + checksum byte starting with length-byte
//                                     |  |  |     |  |        |  |     |        + total consumption, 4 bytes
//                                     |  |  |     |  |        |  |     + frequency (Hz)
//                                     |  |  |     |  |        |  + Ampere/1000 (A), 2 bytes
//                                     |  |  |     |  |        + Voltage (V)
//                                     |  |  |     |  |
//                                     |  |  |     |  + Watt/1000, 3 bytes
//                                     |  |  |     + Power, 0 = off, 1 = on
//                                     |  |  + Capture measurement response 0x0400
//                                     |  + Length of payload starting w/ next byte incl. checksum
//                                     + static start sequence for message, 0x0f

class SEM6000_Response_Measurement : public SEM6000_Response
{
    public:
        MeasurementData  data;
    public:
        SEM6000_Response_Measurement(const uint8_t* const pData);
};

class SEM6000_ResponseFactory
{
    public: 
        static std::unique_ptr<SEM6000_Response> FromBuffer(const uint8_t* const pData, size_t lenData, bool isNotify);
};
#endif // __SEM6000_BLE__