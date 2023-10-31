#include <SEM6000_Response.h>


void MeasurementData::Dump() const
{
        Serial.println("Data On U[V] F[Hz] I[A] P[W] - comsumed ENergy[kWh]" );
        Serial.printf("        %2d %3.0f %2.0f %4.2f % 4.2f - %4.2f", 
            _power_on, _voltage, _frequ, _current, _power, _consumption);
        Serial.println();
}


SEM6000_Response::SEM6000_Response() 
{
    Serial.println("construct SEM6000_Response empty");
    Init();
}

void SEM6000_Response::Init()
{
    _length = 0;
    _cmd= SEMCMD_INVALID;
}
float SEM6000_Response::stream2Float(const uint8_t* p, uint8_t bytes) const
{
    uint32_t val = 0;
    auto pb = p;
    while (bytes >0)
    {
        val = (val *0x100 + *pb);
        pb++;
        bytes--;        
    }
    return (float)val;
}

bool SEM6000_Response::isCmd(uint8_t cmd) const
{
    return _cmd==cmd;
}

uint8_t SEM6000_Response::getCmd() const
{
    return _cmd;
}

SEM6000_Response::SEM6000_Response(const uint8_t* const pData) 
{
    Serial.println("construct SEM6000_Response");
    Init();
    if (pData) { 
        _cmd=pData[2];
        _length=pData[1];
    }
}
SEM6000_Response_Authentify::SEM6000_Response_Authentify(const uint8_t* const pData): 
    SEM6000_Response(pData) 
{
    Serial.println("construct SEM6000_Response_Authentify");
    _no_sucess=true;
    if (pData)
    {
        _no_sucess = (pData[4] == 0x01);
    }
}

bool SEM6000_Response_Authentify::success() const
{
    return !_no_sucess;
}

//                                      F  F 4, 0  1  0  F 1C E6 0 1C  32  1 0   0  0  1 44 AB
//                                      0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18
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
//                                     + static start sequence for message, 0x0f [0]
SEM6000_Response_Measurement::SEM6000_Response_Measurement(const uint8_t* const pData) :
    SEM6000_Response(pData) 
{
    Serial.println("construct SEM6000_Response_Measurement");
    if (pData)
    {
        data._power_on = (pData[4] == 0x01);
        data._power= stream2Float(pData+5,3)/1000;
        data._voltage = (float)pData[8];
        data._current = stream2Float(pData+9,2)/1000;
        data._frequ = pData[11];
        data._consumption = stream2Float(pData+14,4)/1000; 
    }
}

std::unique_ptr<SEM6000_Response> SEM6000_ResponseFactory::FromBuffer(const uint8_t* const pData, size_t lenData, bool isNotify)
{
    uint8_t cmd = 0x00;
    if (pData && lenData>2)
    {
        uint8_t checksum = 1;   // checksum = cmd + following bytes+1;
        auto p = pData;
        Serial.print("check response:");
        Serial.print(*p,HEX); Serial.print(","); Serial.println(*(p+1),HEX);
        // first value must be 0xff
        if (*p==0xf)
        {
            p++;
            uint8_t l = *p;
            if (l>0 && l<lenData)
            {
                p++;
                uint8_t cmd=*p;

                Serial.println("calc checksum >>");
                // test checksum
                // while (l>1)
                // {
                //     checksum+=*p;
                //     Serial.print(checksum,HEX); Serial.print(":");
                //     p++; l--;
                // }
                // Serial.println(*p,HEX);
                //if (checksum==*p) // last byte contains checksum
                {
                    Serial.println("->checksum OK");
                    //p = pData+2;
                    switch (cmd)
                    {
                    case SEMCMD_AUTHENTIFY:
                        return std::make_unique<SEM6000_Response_Authentify>(pData);
                        break;
                    case SEMCMD_ACTUALMEAS:
                        return std::make_unique<SEM6000_Response_Measurement>(pData);
                        break;
                    
                    default:
                        break;
                    }
                } // if checksum OK
            }
        }
    }
    return std::make_unique<SEM6000_Response>();
}
