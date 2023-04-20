#ifdef INCLUDE_RFIDLOCK

#include <Arduino.h>
#include <ArduinoJson.h>
#include <rfidLock.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>

struct MIFARE_Classic_1K_Sector
{
    struct Block
    {
        uint8_t bytes[16];
        
        void dump_debug()
        {
            TRACE("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ",
                  (int) bytes[0], (int) bytes[1], (int) bytes[2], (int) bytes[3], 
                  (int) bytes[4], (int) bytes[5], (int) bytes[6], (int) bytes[7], 
                  (int) bytes[8], (int) bytes[9], (int) bytes[10], (int) bytes[11], 
                  (int) bytes[12], (int) bytes[13], (int) bytes[14], (int) bytes[15])
        }
    };

    Block blocks[4];

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }

    void dump_debug()
    {
        blocks[0].dump_debug();
        blocks[1].dump_debug();
        blocks[2].dump_debug();
        blocks[3].dump_debug();
    }
};

const size_t MIFARE_CLASSIC_1K_NUM_SECTORS = 16;

String uid_2_str(const MFRC522::Uid & uid)
{
    String r = "{\"uid\":{\"size\":";
    r += String((int) uid.size);

    r += ", \"bytes\":[";

    for (size_t i=0; i<sizeof(uid.uidByte)/sizeof(uid.uidByte[0]); ++i)
    {
        r += "0x" + String(uid.uidByte[i], 16);

        if (i+1 < sizeof(uid.uidByte)/sizeof(uid.uidByte[0]))
        {
            r += ", ";
        }
    }
    
    r += "], \"sak\":";
    r += (int) uid.sak;
    r += "}}";

    return r;
}

String key_2_str(const MFRC522::MIFARE_Key & key)
{
    char buf[64];
    
    sprintf(buf, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", (int) key.keyByte[0], (int) key.keyByte[1], (int) key.keyByte[2],
            (int) key.keyByte[3], (int) key.keyByte[4], (int) key.keyByte[5]);
    
    return String(buf);         
}

void i2c_scan(TwoWire & _wire)
{
  byte error, address;
  int nDevices;
  TRACE("Scanning I2C...")
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    _wire.beginTransmission(address);
    error = _wire.endTransmission();
    if (error == 0)
    {
      TRACE("I2C device found at address 0x%x", (int) address);
      nDevices++;
    }
    else if (error==4)
    {
      ERROR("Unknown error at address 0x%x", (int) address);
    }    
  }
  if (nDevices == 0)
    TRACE("No I2C devices found")
  else
    TRACE("done")
}

extern GpioHandler gpioHandler;

static void _err_dup(const char *name, int value)
{
    ERROR("%s %d is duplicated / reused", name, value)
}

static void _err_cap(const char *name, int value)
{
    ERROR("%s %d, gpio doesn't have required capabilities", name, value)
}

static void _err_val(const char *name, int value)
{
    ERROR("%s %d incorrect", name, value)
}

bool RfidLockConfig::is_valid() const
{
    bool r = rfid.is_valid() && lock.is_valid() && buzzer.is_valid();

    if (r == false)
    {
        return false;
    }

    GpioCheckpad checkpad;


    const char *object_name = "rfid.resetPowerDownPin";

    if (rfid.resetPowerDownPin != UNUSED_PIN)
    {
        if (checkpad.get_usage(rfid.resetPowerDownPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rfid.resetPowerDownPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.resetPowerDownPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name, (int)rfid.resetPowerDownPin);
            return false;
        }
    }

    if (rfid.protocol == Rfid::pSPI)
    {
        object_name = "rfid.chipSelectPin";

        if (checkpad.get_usage(rfid.chipSelectPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rfid.chipSelectPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.chipSelectPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name, (int)rfid.chipSelectPin);
            return false;
        }
    }

    if (rfid.protocol == Rfid::pI2C)
    {
        object_name = "rfid.sclPin";

        if (checkpad.get_usage(rfid.sclPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rfid.sclPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.sclPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name, (int)rfid.sclPin);
            return false;
        }

        object_name = "rfid.sdaPin";

        if (checkpad.get_usage(rfid.sdaPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rfid.sdaPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.sdaPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name, (int)rfid.sdaPin);
            return false;
        }
    }

    size_t i = 0;

    for (auto it = lock.channels.begin(); it != lock.channels.end(); ++it, ++i)
    {    
        String _object_name = String("lock.channel[") + String(i) + "].gpio";

        if (checkpad.get_usage(it->gpio) != GpioCheckpad::uNone)
        {
            _err_dup(_object_name.c_str(), (int)it->gpio);
            return false;
        }

        if (!checkpad.set_usage(it->gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(_object_name.c_str(), (int)it->gpio);
            return false;
        }
    }
    
    object_name = "buzzer.channel.gpio";

    if (checkpad.get_usage(buzzer.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int)buzzer.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(buzzer.channel.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name, (int)buzzer.channel.gpio);
        return false;
    }

    return true;
}

void RfidLockConfig::from_json(const JsonVariant &json)
{
    if (json.containsKey("rfid"))
    {
        const JsonVariant &_json = json["rfid"];
        rfid.from_json(_json);
    }

    if (json.containsKey("lock"))
    {
        const JsonVariant &_json = json["lock"];
        lock.from_json(_json);
    }

    if (json.containsKey("buzzer"))
    {
        const JsonVariant &_json = json["buzzer"];
        buzzer.from_json(_json);
    }
}

void RfidLockConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    rfid.to_eprom(os);
    lock.to_eprom(os);
    buzzer.to_eprom(os);
}

bool RfidLockConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        rfid.from_eprom(is);
        lock.from_eprom(is);
        buzzer.from_eprom(is);
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read RfidLockConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void RfidLockConfig::Rfid::from_json(const JsonVariant &json)
{
    if (json.containsKey("protocol"))
    {
        protocol = str_2_protocol(json["protocol"]);
    }

    if (json.containsKey("hw"))
    {
        hw = str_2_hw(json["hw"]);
    }

    if (json.containsKey("resetPowerDownPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["resetPowerDownPin"]);
        resetPowerDownPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("chipSelectPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["chipSelectPin"]);
        chipSelectPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("sclPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["sclPin"]);
        sclPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("sdaPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["sdaPin"]);
        sdaPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("i2cAddress"))
    {
        i2cAddress = (uint8_t) (int) json["i2cAddress"];
    }
}

void RfidLockConfig::Rfid::to_eprom(std::ostream &os) const
{
    os.write((const char *)&protocol, sizeof(protocol));
    os.write((const char *)&hw, sizeof(hw));

    uint8_t gpio_uint8 = (uint8_t)resetPowerDownPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));
    
    gpio_uint8 = (uint8_t)chipSelectPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    gpio_uint8 = (uint8_t)sclPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    gpio_uint8 = (uint8_t)sdaPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    os.write((const char *)&i2cAddress, sizeof(i2cAddress));
}

bool RfidLockConfig::Rfid::from_eprom(std::istream &is)
{
    is.read((char *)&protocol, sizeof(protocol));
    is.read((char *)&hw, sizeof(hw));

    int8_t gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    resetPowerDownPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    chipSelectPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    sclPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    sdaPin = (gpio_num_t)gpio_int8;

    is.read((char *)&i2cAddress, sizeof(i2cAddress));

    return is_valid() && !is.bad();
}

void RfidLockConfig::Lock::from_json(const JsonVariant &json)
{
    channels.resize(0);

     if (json.containsKey("channels"))
    {
        const JsonVariant &_json = json["channels"];

        if (_json.is<JsonArray>())
        {
            size_t i=0;
            
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;
                RelayChannelConfig channel;
                channel.from_json(__json);
                channels.push_back(channel);

                ++iterator;
            }
        }
    }

    if (json.containsKey("linger"))
    {
        linger = (unsigned)((int)json["linger"]);
    }
}

void RfidLockConfig::Lock::to_eprom(std::ostream &os) const
{
    uint8_t count = channels.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it = channels.begin(); it != channels.end(); ++it)
    {
        it->to_eprom(os);
    }

    os.write((const char *)&linger, sizeof(linger));
}

bool RfidLockConfig::Lock::from_eprom(std::istream &is)
{
    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        RelayChannelConfig channel;
        channel.from_eprom(is);
        channels.push_back(channel);
    }

    is.read((char *)&linger, sizeof(linger));

    return is_valid() && !is.bad();
}


class RfidLockHandler
{
public:

    RfidLockHandler()
    {
        _is_active = false;
        _is_finished = true;
        rc522 = NULL;
        pn532_i2c = NULL;
        nfc_adapter = NULL;
    }

    ~RfidLockHandler()
    {
        clear_rfid();
    }

    bool is_active() const { return _is_active; }

    void start(const RfidLockConfig &config);
    void stop();
    void reconfigure(const RfidLockConfig &config);

    RfidLockStatus get_status()
    {
        RfidLockStatus _status;

        {
            Lock lock(semaphore);
            _status = status;
        }

        return _status;
    }

protected:
    
    void clear_rfid()
    {
        if (rc522)
        {
            delete rc522;
            rc522 = NULL;
        }
        if (pn532_i2c)
        {
            delete pn532_i2c;
            pn532_i2c = NULL;
        }
        if (nfc_adapter)
        {
            delete nfc_adapter;
            nfc_adapter = NULL;
        }
    }

    void configure_hw();
    void configure_hw_rfid(const RfidLockConfig::Rfid &rfid);
    static void configure_hw_lock(const RfidLockConfig::Lock &lock);
    static void configure_hw_buzzer(const BuzzerConfig &);

    static void write_lock(const RfidLockConfig::Lock &lock, bool value);

    static void task(void *parameter);

    bool rc522_read_mifare_sector(MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                  MFRC522::Uid & uid, MIFARE_Classic_1K_Sector & sector_data);

    bool tag_submitted(const MIFARE_Classic_1K_Sector &);

    BinarySemaphore semaphore;
    RfidLockConfig config;
    RfidLockStatus status;
    bool _is_active;
    bool _is_finished;

    MFRC522 * rc522;
    PN532_I2C * pn532_i2c;
    NfcAdapter * nfc_adapter;
};

static RfidLockHandler handler;

void RfidLockHandler::start(const RfidLockConfig &_config)
{
    if (_is_active)
    {
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    config = _config;
    configure_hw();

    _is_active = true;
    _is_finished = false;

    xTaskCreate(
        task,                // Function that should be called
        "rfid_lock_task",    // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    ); 
}

void RfidLockHandler::stop()
{
    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }

    stop_buzzer_task();
    clear_rfid();
}

void RfidLockHandler::reconfigure(const RfidLockConfig &_config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("rfid_lock_task: config changed")

        if (!(config.rfid == _config.rfid))
        {
            configure_hw_rfid(_config.rfid);
        }

        if (!(config.lock == _config.lock))
        {
            configure_hw_lock(_config.lock);
        }

        config = _config;
    }
}

/**
 		Helper routine to dump a byte array as dec values to Serial.
*/
static void printDec(byte *buffer, byte bufferSize) {
 	for (byte i = 0; i < bufferSize; i++) {
 			Serial.print(buffer[i] < 0x10 ? " 0" : " ");
 			Serial.print(buffer[i], DEC);
 	}
}


void readNFC(NfcAdapter & _nfc) 
{
    //return;
 if (_nfc.tagPresent())
 {
   NfcTag tag = _nfc.read();
   TRACE("NFC tag: %s", tag.getUidString().c_str())
 }
}


void RfidLockHandler::task(void *parameter)
{
    RfidLockHandler *_this = (RfidLockHandler *)parameter;

    TRACE("rfid_lock_task: started")
    buzzer_one_long = true;

    RfidLockStatus status_copy;

    MIFARE_Classic_1K_Sector sector_data;
    sector_data.clear();

    MFRC522::MIFARE_Key key = {0,0,0,0,0,0};

    MFRC522::Uid uid;
    memset(& uid, 0, sizeof(uid));

    size_t sector_index = 1;

    while (_this->_is_active)
    {
        if (_this->rc522)
        {   
            MFRC522::MIFARE_Key _key = key;

            bool result = _this->rc522_read_mifare_sector(_key, sector_index, uid, sector_data);

            if (uid.size > 0)  // new tag is submitted
            {
                if (result == true)
                {
                    TRACE("Successfully read sector %d on tag %s", (int) sector_index, uid_2_str(uid).c_str())
                    _this->tag_submitted(sector_data);
                    sector_data.dump_debug();
                    sector_data.clear();
                }
                else
                {
                    TRACE("Failed to read sector %d on tag %s", (int) sector_index, uid_2_str(uid).c_str())
                    buzzer_one_long = true;
                }
            }
            /*
         	if (_this->rc522->PICC_IsNewCardPresent())
            {
 	            if (_this->rc522->PICC_ReadCardSerial())
                {
	                _this->rc522->PICC_DumpToSerial(&(_this->rc522->uid));
                }
            }
            */
        }

        if (_this->nfc_adapter)
        {
            readNFC(*(_this->nfc_adapter));
        }

        delay(10);
    }

    _this->_is_finished = true;

    TRACE("rfid_lock_task: terminated")
    vTaskDelete(NULL);
}


bool RfidLockHandler::rc522_read_mifare_sector(MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                               MFRC522::Uid & uid, MIFARE_Classic_1K_Sector & sector_data)
{
    // if uid is set upon return but the return value is false then the tag was submitted 
    // but something went wrong (most likely, wrong key)

    // note: the key may change upon return, the function could have tried default key if the provided
    // one didn't work

    static const MFRC522::MIFARE_Key  default_key = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    
    bool r = false;
    memset(& uid, 0, sizeof(uid));

    if (rc522)
    {
        if (rc522->PICC_IsNewCardPresent())
        {
            TRACE("RC522 new card present")

            if (rc522->PICC_Select(& uid) == MFRC522::STATUS_OK)
            {
                TRACE("RC522 PICC select succeded, uid %s", uid_2_str(uid).c_str())

                size_t block_count = sizeof(MIFARE_Classic_1K_Sector::blocks)/sizeof(MIFARE_Classic_1K_Sector::blocks[0]);
                size_t block_index = sector_index * block_count;

			    MFRC522::StatusCode status = 
                rc522->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_index, & key, & uid);

                // if the key provided is not working - maybe the card is factory default, check the default key

                if (status != MFRC522::STATUS_OK) 
                {
                    TRACE("RC522 PCD authenticate with provided key %s failed", key_2_str(key).c_str())

                    // Wake the card up again
                    byte atqa_answer[2];
                    byte atqa_size = 2;
                    
                    if (rc522->PICC_WakeupA(atqa_answer, &atqa_size) ==  MFRC522::STATUS_OK &&
                        rc522->PICC_Select(& uid) ==  MFRC522::STATUS_OK)
                    {
                        status = 
                        rc522->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_index, (MFRC522::MIFARE_Key*)& default_key, & uid);
                    }

                    if (status == MFRC522::STATUS_OK) 
                    {
                        TRACE("RC522 PCD authenticate with default key %s succeded", key_2_str(default_key).c_str())
                        key = default_key;
                    }
                    else
                    {
                        TRACE("RC522 PCD authenticate with default key %s failed too", key_2_str(default_key).c_str())
                    }
                }
                else
                {
                    TRACE("RC522 PCD authenticate with provided key %s succeded", key_2_str(key).c_str())
                }

                if (status == MFRC522::STATUS_OK) 
                {
                    r = true;

                    for (size_t i=0; i<block_count; ++i)
                    {
                        uint8_t buffer[32];
                        uint8_t c = sizeof(buffer);
                        
		                status = rc522->MIFARE_Read(block_index+i, buffer, & c);

		                if (status != MFRC522::STATUS_OK) 
                        {
                            TRACE("RC522 MIFARE read failed with status %d (%s)", (int) status, (const char*)rc522->GetStatusCodeName(status))
                            r = false;
                            break;
                        }

                        memset(sector_data.blocks[i].bytes, 0, sizeof(sector_data.blocks[i].bytes));
                        c = c > sizeof(sector_data.blocks[i].bytes) ? sizeof(sector_data.blocks[i].bytes) : c;
                        memcpy(sector_data.blocks[i].bytes, buffer, c);                                                
                    }

                    rc522->PICC_HaltA();
                 	rc522->PCD_StopCrypto1();
                }
            }
		}
    }

    return r;
}

bool RfidLockHandler::tag_submitted(const MIFARE_Classic_1K_Sector & sector_data)
{
    buzzer_series_short = true; 
    return true; 
}

void RfidLockHandler::configure_hw()
{
    configure_hw_rfid(config.rfid);
    configure_hw_lock(config.lock);
    configure_hw_buzzer(config.buzzer);
}

void RfidLockHandler::configure_hw_buzzer(const BuzzerConfig & config)
{
    start_buzzer_task(config);
}

void RfidLockHandler::configure_hw_rfid(const RfidLockConfig::Rfid &_config)
{
    clear_rfid();

    if (_config.hw == RfidLockConfig::Rfid::hwRC522)
    {
        if (_config.protocol == RfidLockConfig::Rfid::pSPI)
        {
            rc522 = new MFRC522(_config.chipSelectPin, _config.resetPowerDownPin);
            SPI.begin();

            rc522->PCD_Init();
            uint8_t ver = rc522->PCD_ReadRegister(MFRC522::VersionReg);
            TRACE("MFRC522 reader returned version code 0x%x", (int) ver)

            if (_config.resetPowerDownPin != UNUSED_PIN)
            {
                bool self_test_result = rc522->PCD_PerformSelfTest();
                TRACE("MFRC522 reader self-test result is %s OK", self_test_result ? "" : "NOT")
                rc522->PCD_Reset();
                rc522->PCD_Init();
            }
        }
        else
        {
            ERROR("no support for RC522/%s", RfidLockConfig::Rfid::protocol_2_str(_config.protocol))            
        }
    }
    else
    if (_config.hw == RfidLockConfig::Rfid::hwPN532)
    {
        if (_config.protocol == RfidLockConfig::Rfid::pI2C)
        {
            if (_config.resetPowerDownPin != UNUSED_PIN)
            {
                pinMode(_config.resetPowerDownPin, OUTPUT);
                digitalWrite(_config.resetPowerDownPin, 0);
                delay(100);
                digitalWrite(_config.resetPowerDownPin, 1);
            }

            Wire.end();
            Wire.begin(_config.sdaPin, _config.sclPin);
            //i2c_scan(Wire);

            pn532_i2c = new PN532_I2C(Wire);
            nfc_adapter = new NfcAdapter(*pn532_i2c);
            nfc_adapter->begin();
        }
        else
        {
            ERROR("no support for PN532/%s", RfidLockConfig::Rfid::protocol_2_str(_config.protocol))            
        }
    }
    else
    {
        ERROR("no support for %s/%s", RfidLockConfig::Rfid::hw_2_str(_config.hw), RfidLockConfig::Rfid::protocol_2_str(_config.protocol))            
    }
}

void RfidLockHandler::configure_hw_lock(const RfidLockConfig::Lock &config)
{
}

void start_rfid_lock_task(const RfidLockConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start rfid_lock_task while it is running, redirecting to reconfigure")
        reconfigure_rfid_lock(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_rfid_lock_task()
{
    handler.stop();
}

RfidLockStatus get_rfid_lock_status()
{
    return handler.get_status();
}

void reconfigure_rfid_lock(const RfidLockConfig &_config)
{
    handler.reconfigure(_config);
}

String rfid_lock_program(const String & code_str)
{
    return String();
    // return "Parameter error";
}

#endif // INCLUDE_RFIDLOCK
