#ifdef INCLUDE_RFIDLOCK

#include <ArduinoJson.h>
#include <buzzer.h>
#include <SPI.h>//https://www.arduino.cc/en/reference/SPI
#include <Wire.h>
#include <MFRC522.h>//https://github.com/miguelbalboa/rfid
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <vector>
#include <relayChannelConfig.h>

class RfidLockConfig
{
    public:

        const uint8_t EPROM_VERSION = 3;

        RfidLockConfig()
        {
        }
        
        RfidLockConfig & operator = (const RfidLockConfig & config) 
        {
            rfid = config.rfid;
            lock = config.lock;
            buzzer = config.buzzer;

            return *this;
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const;

        bool operator == (const RfidLockConfig & config) const
        {
            return rfid == config.rfid && lock == config.lock && buzzer == config.buzzer;
        }

        String as_string() const
        {
            return String("{rfid=") + rfid.as_string() + ", lock=" + lock.as_string() + ", buzzer=" + buzzer.as_string() + "}";
        }

        // data

        struct Rfid
        {
            enum Protocol
            {
                pSPI  = 0,
                pI2C  = 1,
            };

            enum HW
            {
                hwRC522  = 0,
                hwPN532  = 1,
            };

            static const char * protocol_2_str(Protocol protocol) 
            {
                switch(protocol)
                {
                    case pSPI:  return "SPI";
                    case pI2C:  return "I2C";
                }

                return "<unknown>";
            }

            static Protocol str_2_protocol(const char * str) 
            {
                if (!strcmp(str, protocol_2_str(pSPI)))
                {
                    return pSPI;
                }

                if (!strcmp(str, protocol_2_str(pI2C)))
                {
                    return pI2C;
                }

                return Protocol(-1);
            }

            static const char * hw_2_str(HW hw) 
            {
                switch(hw)
                {
                    case hwRC522:  return "RC522";
                    case hwPN532:  return "PN532";
                }

                return "<unknown>";
            }

            static HW str_2_hw(const char * str) 
            {
                if (!strcmp(str, hw_2_str(hwRC522)))
                {
                    return hwRC522;
                }

                if (!strcmp(str, hw_2_str(hwPN532)))
                {
                    return hwPN532;
                }

                return HW(-1);
            }

            Rfid()
            {
                protocol = pSPI;
                hw = hwRC522;
                resetPowerDownPin = (gpio_num_t) UNUSED_PIN;
                chipSelectPin = (gpio_num_t) 5; 
                sclPin = (gpio_num_t) SCL; 
                sdaPin = (gpio_num_t) SDA; 
                i2cAddress = MFRC522_I2C_DEFAULT_ADDR;  
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                if (protocol == pSPI)
                {
                    if (chipSelectPin == (gpio_num_t)-1)
                    {
                        return false;
                    }

                    if (hw != hwRC522)
                    {
                        return false;
                    }
                }
                else
                if (protocol == pI2C)
                {
                    if (sclPin == (gpio_num_t)-1 || sdaPin == (gpio_num_t)-1 || i2cAddress == (uint8_t)-1)
                    {
                        return false;
                    }

                    if (hw != hwPN532)
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }

                if (!(hw == hwRC522 || hw == hwPN532))
                {
                    return false;
                }

                return true;
            }

            bool operator == (const Rfid & rfid) const
            {
                return protocol == rfid.protocol && hw == rfid.hw && resetPowerDownPin == rfid.resetPowerDownPin && 
                    chipSelectPin == rfid.chipSelectPin && 
                       sclPin == rfid.sclPin && sdaPin == rfid.sdaPin && i2cAddress == rfid.i2cAddress;
            }

            String as_string() const
            {
                return String("{protocol=") + protocol_2_str(protocol) + ", hw=" + hw_2_str(hw) + 
                              ", resetPowerDownPin=" + resetPowerDownPin + 
                              ", chipSelectPin=" + chipSelectPin + ", sclPin=" + sclPin + ", sdaPin=" + sdaPin + 
                              ", i2cAddress=" + i2cAddress + "}";
            }
            
            Protocol protocol;
            HW hw;
            gpio_num_t resetPowerDownPin;
            gpio_num_t chipSelectPin;
            gpio_num_t sclPin;           
            gpio_num_t sdaPin; 
            uint8_t i2cAddress;          
        };
        
        Rfid rfid;
    
        struct Lock
        {
            Lock()
            {
                linger = 3; // seconds
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                for (auto it = channels.begin(); it != channels.end(); ++it)
                {
                    if (it->is_valid() == false)
                    {
                        return false;
                    }
                }
                return true;    
            }

            bool operator == (const Lock & lock) const
            {
                if (lock.linger != linger)
                {
                    return false;
                }

                if (lock.channels.size() != channels.size())
                {
                    return false;
                }

                auto other_it = lock.channels.begin();
                
                for (auto it = channels.begin(); it != channels.end(); ++it, ++other_it)
                {
                    if (!(*it == *other_it))
                    {
                        return false;
                    }
                }

                return true;
            }

            String as_string() const
            {
                String r = "{channels=["; 
                
                for (auto it = channels.begin(); it != channels.end(); ++it)
                {
                    r += it->as_string();

                    if (it+1 != channels.end())
                    {
                        r += ",";
                    }
                }
                 
                r += "], linger=" + String(linger) + "}";
                return r;
            }

            
            std::vector<RelayChannelConfig> channels;            
            uint16_t linger;
        };
        
        Lock lock;

        BuzzerConfig buzzer;
};


struct RfidLockStatus
{
    RfidLockStatus()
    {
        is_ready = true;
    }

    RfidLockStatus(const RfidLockStatus & other)
    {
        if (this == & other)
        {
            return;
        }        

        is_ready = other.is_ready;
    }

    RfidLockStatus & operator = (const RfidLockStatus & other)
    {
        if (this == & other)
        {
            return *this;
        }        

        is_ready = other.is_ready;

        return *this;
    }

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("rfid-lock");
        JsonVariant jsonVariant = json["rfid-lock"];

        jsonVariant["is_ready"] = is_ready;
    }

    bool is_ready;
};

void start_rfid_lock_task(const RfidLockConfig &);
void stop_rfid_lock_task();
RfidLockStatus get_rfid_lock_status();

String rfid_lock_program(const String & code_str);

void reconfigure_rfid_lock(const RfidLockConfig &);

#endif // INCLUDE_RFIDLOCK
