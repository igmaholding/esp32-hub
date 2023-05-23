#ifdef INCLUDE_RFIDLOCK

#include <Arduino.h>
#include <ArduinoJson.h>
#include <rfidLock.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <onboardLed.h>

const uint16_t DEFAULT_PROGRAM_TIMEOUT = 10;

static const MFRC522::MIFARE_Key DEFAULT_MIFARE_KEY = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const MFRC522::MIFARE_Key ZERO_MIFARE_KEY = {0, 0, 0, 0, 0, 0};

static const MFRC522::MIFARE_Key ACTIVE_MIFARE_KEY_A = {0x7e, 0xdc, 0x93, 0x65, 0xc0, 0xdd};
static const MFRC522::MIFARE_Key ACTIVE_MIFARE_KEY_B = {0x53, 0x31, 0x2e, 0x21, 0x57, 0x0c};

const uint8_t ACTIVE_MIFARE_SECTOR = 1;

struct MIFARE_Classic_1K_Sector
{
    struct Block
    {
        uint8_t bytes[16];
        
        void dump_debug() const
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

    void dump_debug() const
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

String mifare_key_type_2_str(uint8_t key_type)
{
    const char * str = "";

    switch(key_type)
    {
        case MFRC522::PICC_CMD_MF_AUTH_KEY_A:  str = "key A"; break;
        case MFRC522::PICC_CMD_MF_AUTH_KEY_B:  str = "key B"; break;
        default:  str = "unknown"; break;
    }

    return String(str);
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
    bool r = rfid.is_valid() && lock.is_valid() && buzzer.is_valid() && green_led.is_valid() && red_led.is_valid();

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
        String _object_name = String("lock.channel[") + it->first + "].gpio";

        if (checkpad.get_usage(it->second.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(_object_name.c_str(), (int)it->second.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->second.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(_object_name.c_str(), (int)it->second.gpio);
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

    object_name = "green_led.gpio";

    if (checkpad.get_usage(green_led.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int)green_led.gpio);
        return false;
    }

    if (!checkpad.set_usage(green_led.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name, (int)green_led.gpio);
        return false;
    }

    object_name = "red_led.gpio";

    if (checkpad.get_usage(red_led.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int)red_led.gpio);
        return false;
    }

    if (!checkpad.set_usage(red_led.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name, (int)red_led.gpio);
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

    if (json.containsKey("green_led"))
    {
        const JsonVariant &_json = json["green_led"];
        green_led.from_json(_json);
    }

    if (json.containsKey("red_led"))
    {
        const JsonVariant &_json = json["red_led"];
        red_led.from_json(_json);
    }
}

void RfidLockConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    rfid.to_eprom(os);
    lock.to_eprom(os);
    buzzer.to_eprom(os);
    green_led.to_eprom(os);
    red_led.to_eprom(os);
    codes.to_eprom(os);
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
        green_led.from_eprom(is);
        red_led.from_eprom(is);
        codes.from_eprom(is);
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
    channels.clear();

     if (json.containsKey("channels"))
    {
        const JsonVariant &_json = json["channels"];
        const JsonObject & _json_object = _json.as<JsonObject>();
        
        for(auto iterator = _json_object.begin(); iterator != _json_object.end(); ++iterator) 
        {
            const JsonVariant & __json = iterator->value().as<JsonVariant>();
            RelayChannelConfig channel;
            channel.from_json(__json);

            channels.insert(std::make_pair(String(iterator->key().c_str()), channel));
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
        uint8_t len = (uint8_t) it->first.length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->first.c_str(), len);

        it->second.to_eprom(os);
    }

    os.write((const char *)&linger, sizeof(linger));
}

bool RfidLockConfig::Lock::from_eprom(std::istream &is)
{
    channels.clear();

    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        char buf[256];
        uint8_t len = 0;
        is.read((char *)&len, sizeof(len));
        is.read((char *)buf, len);
        buf[len]=0;

        RelayChannelConfig channel;
        channel.from_eprom(is);

        channels.insert(std::make_pair(String(buf), channel));
    }

    is.read((char *)&linger, sizeof(linger));

    return is_valid() && !is.bad();
}

void RfidLockConfig::Codes::from_json(const JsonVariant &json)
{ 
    // codes are managed via separate URLs
}

void RfidLockConfig::Codes::to_eprom(std::ostream &os) const
{ 
    uint8_t count = codes.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it = codes.begin(); it != codes.end(); ++it)
    {
        uint8_t len = (uint8_t) it->first.length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->first.c_str(), len);

        it->second.to_eprom(os);
    }
}

bool RfidLockConfig::Codes::from_eprom(std::istream &is)
{ 
    codes.clear();

    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        char buf[256];
        uint8_t len = 0;
        is.read((char *)&len, sizeof(len));
        is.read((char *)buf, len);
        buf[len]=0;

        Code code;
        code.from_eprom(is);

        codes.insert(std::make_pair(String(buf), code));
    }

    return is_valid() && !is.bad();
}

void RfidLockConfig::Codes::Code::from_json(const JsonVariant &json)
{
    // codes are managed via separate URLs
}

void RfidLockConfig::Codes::Code::to_eprom(std::ostream &os) const
{
    uint8_t len = value.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(value.c_str(), len);
    }

    uint8_t count = locks.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it = locks.begin(); it != locks.end(); ++it)
    {
        uint8_t len = (uint8_t) it->length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->c_str(), len);
    }

    uint8_t type_uint8_t = (uint8_t) type;
    os.write((const char *)&type_uint8_t, sizeof(type_uint8_t));
}

bool RfidLockConfig::Codes::Code::from_eprom(std::istream &is)
{
    uint8_t len = 0;
    char buf[256];

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        is.read(buf, len);
        buf[len] = 0;
        value = buf;
    }
    else
    {
        value = "";
    }

    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        is.read((char *)&len, sizeof(len));
        is.read((char *)buf, len);
        buf[len]=0;

        locks.push_back(buf);
    }

    uint8_t type_uint8_t = 0;
    is.read((char *)&type_uint8_t, sizeof(type_uint8_t));
    type = (Type) type_uint8_t;

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
        red_led_state = ledOff;
        green_led_state = ledOff;
        _is_unlocking = false;
        _is_programming = false;
        program_timeout = DEFAULT_PROGRAM_TIMEOUT;
    }

    ~RfidLockHandler()
    {
        clear_rfid();
    }

    bool is_active() const { return _is_active; }

    void start(const RfidLockConfig &config);
    void stop();
    void reconfigure(const RfidLockConfig &config);
    String program(const String & code, uint16_t timeout);

    RfidLockStatus get_status()
    {
        RfidLockStatus _status;

        {
            Lock lock(semaphore);
            _status = status;
        }

        return _status;
    }

    void get_codes(RfidLockConfig::Codes & codes)
    {
        Lock lock(semaphore);
        codes = config.codes;
    }

    void update_codes(const RfidLockConfig::Codes & codes)
    {
        Lock lock(semaphore);
        config.codes = codes;        
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
    void configure_hw_leds(const DigitalOutputChannelConfig &green_led, const DigitalOutputChannelConfig &red_led);
    static void unconfigure_hw_leds(const DigitalOutputChannelConfig &green_led, const DigitalOutputChannelConfig &red_led);

    static void write_lock(const RfidLockConfig::Lock &lock, bool value);

    static void task(void *parameter);
    static void unlock_task(void *parameter);
    static void program_task(void *parameter);

    bool rc522_read_mifare_sector(MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                  MFRC522::Uid & uid, MIFARE_Classic_1K_Sector & sector_data, bool wake_up = false);

    bool rc522_check_read_mifare_sector(MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                        MFRC522::Uid & uid, MIFARE_Classic_1K_Sector & sector_data);

    bool rc522_check_write_mifare_sector(const MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                         MFRC522::Uid & uid, const MIFARE_Classic_1K_Sector & sector_data);

    bool tag_submitted(const MIFARE_Classic_1K_Sector &);
    void commence_unlock(const std::vector<String> & locks);

    BinarySemaphore semaphore;
    RfidLockConfig config;
    RfidLockStatus status;

    bool _is_active;
    bool _is_finished;

    MFRC522 * rc522;
    PN532_I2C * pn532_i2c;
    NfcAdapter * nfc_adapter;

    LedState green_led_state;
    LedState red_led_state;

    bool _is_unlocking;
    std::vector<String> unlocking_what;

    bool _is_programming;
    String program_code;
    uint16_t program_timeout;
};

static RfidLockHandler handler;

void RfidLockHandler::start(const RfidLockConfig &_config)
{
    if (_is_active)
    {
        return; // already running
    }

    while(_is_finished == false || _is_programming == true || _is_unlocking == true)
    {
        delay(100);
    }

    config = _config;
    config.codes = _config.codes;

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

    while(_is_finished == false || _is_programming == true || _is_unlocking == true)
    {
        delay(100);
    }

    stop_buzzer_task();
    clear_rfid();
    unconfigure_hw_leds(config.green_led, config.red_led);
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

        if (!(config.buzzer == _config.buzzer))
        {
            configure_hw_buzzer(_config.buzzer);
        }

        if (!(config.green_led == _config.green_led && config.red_led == _config.red_led))
        {
            unconfigure_hw_leds(config.green_led, config.red_led);
            configure_hw_leds(_config.green_led, _config.red_led);
        }

        config = _config;
        // do not override codes
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

    size_t sector_index = ACTIVE_MIFARE_SECTOR;

    _this->red_led_state = ledOn;
    _this->green_led_state = ledOff;

    while (_this->_is_active)
    {
        if (_this->_is_unlocking == false && _this->_is_programming == false)
        {
            if (_this->rc522)
            {   
                MFRC522::MIFARE_Key _key = ACTIVE_MIFARE_KEY_A;

                bool result = _this->rc522_check_read_mifare_sector(_key, sector_index, uid, sector_data);

                if (uid.size > 0)  // new tag is submitted
                {
                    if (result == true)
                    {
                        TRACE("Successfully read sector %d on tag %s", (int) sector_index, uid_2_str(uid).c_str())
                        sector_data.dump_debug();
                        _this->tag_submitted(sector_data);
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
        }

        delay(10);
    }

    _this->_is_finished = true;

    TRACE("rfid_lock_task: terminated")
    vTaskDelete(NULL);
}


void RfidLockHandler::unlock_task(void *parameter)
{
    void ** args = (void**) parameter;

    RfidLockHandler *_this = (RfidLockHandler *)args[0];
    const std::vector<String> * locks_param = (const std::vector<String> *) args[1];

    //DEBUG("args 0x%x this 0x%x locks_param 0x%x", (uint32_t) &args, (uint32_t) _this, (uint32_t) locks_param)

    TRACE("rfid_unlock_task: started")

    _this->red_led_state = ledOff;
    _this->green_led_state = ledOn;

    uint16_t linger = 1;
    
    { Lock lock(_this->semaphore);

    linger = _this->config.lock.linger;
    for (auto it=_this->config.lock.channels.begin(); it!=_this->config.lock.channels.end(); ++it)
    {
        if (locks_param->empty() == true || 
            std::find(locks_param->begin(), locks_param->end(), it->first) != locks_param->end())
        {
            TRACE("Unlocking channel %s", it->first.c_str())
            digitalWrite(it->second.gpio, it->second.inverted ? 0 : 1);
        }
    }}

    delay(linger * 1000);

    { Lock lock(_this->semaphore);

    for (auto it=_this->config.lock.channels.begin(); it!=_this->config.lock.channels.end(); ++it)
    {
        digitalWrite(it->second.gpio, it->second.inverted ? 1 : 0);
    }}

    _this->red_led_state = ledOn;
    _this->green_led_state = ledOff;

    { Lock lock(_this->semaphore);
    _this->_is_unlocking = false; }

    TRACE("rfid_unlock_task: finished")
    vTaskDelete(NULL);
}


void RfidLockHandler::program_task(void *parameter)
{
    RfidLockHandler *_this = (RfidLockHandler *)parameter;

    TRACE("rfid_program_task: started")

    _this->red_led_state = led50Slow;
    _this->green_led_state = led50SlowInverted;

    MIFARE_Classic_1K_Sector sector_data;
    memset(& sector_data, 0, sizeof(sector_data));

    // access bytes: KEY A - read, KEY B - read/write
    static const uint8_t ACCESS_BYTES[4] = {0x08, 0x77, 0x8F, 0x69}; // TRANSPORT: {0xff, 0x07, 0x80, 0x69};

    memcpy(sector_data.blocks[sizeof(sector_data.blocks)/sizeof(sector_data.blocks[0])-1].bytes, ACTIVE_MIFARE_KEY_A.keyByte, sizeof(MFRC522::MIFARE_Key));
    memcpy(sector_data.blocks[sizeof(sector_data.blocks)/sizeof(sector_data.blocks[0])-1].bytes + 6, ACCESS_BYTES, sizeof(ACCESS_BYTES));
    memcpy(sector_data.blocks[sizeof(sector_data.blocks)/sizeof(sector_data.blocks[0])-1].bytes + 10, ACTIVE_MIFARE_KEY_B.keyByte, sizeof(MFRC522::MIFARE_Key));

    uint8_t user_data_count = sizeof(sector_data.blocks[0]) * 3 - 1;

    { Lock lock(_this->semaphore);
    
    if (_this->program_code.length() < user_data_count)
    {
        user_data_count = _this->program_code.length();    
    }

    uint8_t block_index = 0;
    uint8_t pos = 0;

    sector_data.blocks[block_index].bytes[pos] = user_data_count;
    pos++;

    for(size_t i=0; i<user_data_count; ++i)
    {
        sector_data.blocks[block_index].bytes[pos] = _this->program_code[i];    
        pos++;

        if (pos == sizeof(sector_data.blocks[block_index].bytes))
        {
            pos = 0;
            block_index++;
        }
    } }
    
    TRACE("program_task: prepared sector data to write, waiting for card")
    sector_data.dump_debug();

    MFRC522::Uid uid;
    memset(& uid, 0, sizeof(uid));

    uint32_t _millis = millis();

    bool success = false;

    while(true)
    {
        bool result = _this->rc522_check_write_mifare_sector(ACTIVE_MIFARE_KEY_B, ACTIVE_MIFARE_SECTOR, uid, sector_data);

                if (uid.size > 0)  // new tag is submitted
                {
                    if (result == true)
                    {
                        _this->red_led_state = ledOn;
                        _this->green_led_state = ledOn;  

                        TRACE("program_task: successfully written sector %d on tag %s", (int) ACTIVE_MIFARE_SECTOR, uid_2_str(uid).c_str())

                        MFRC522::MIFARE_Key _key = ACTIVE_MIFARE_KEY_A;
                        MIFARE_Classic_1K_Sector read_sector_data;

                        result = _this->rc522_read_mifare_sector(_key, ACTIVE_MIFARE_SECTOR, uid, read_sector_data, true);

                        if (result == true)
                        {
                            TRACE("program_task: read to verify is ok")
                            success = true;

                            for (auto i=0; i<3; ++i)
                            {
                                if (memcmp(sector_data.blocks[i].bytes, read_sector_data.blocks[i].bytes, sizeof(sector_data.blocks[i].bytes)))
                                {
                                    TRACE("program_task: verification failed")
                                    read_sector_data.dump_debug();
                                    success = false;
                                    break;
                                }
                            }

                            if (success == true)
                            {
                                TRACE("program_task: verification succeded, tag ready to use")
                            }
                        }
                        else
                        {
                            TRACE("program_task: failed to read to verify")
                        }
                    }
                    else
                    {
                        TRACE("program_task: failed to write sector %d on tag %s", (int) ACTIVE_MIFARE_SECTOR, uid_2_str(uid).c_str())
                        buzzer_one_long = true;
                    }

                    if (success == true)
                    {
                        _this->red_led_state = ledOff;
                        _this->green_led_state = led50Fast;  
                        buzzer_series_short = true; 
                      
                    }
                    else
                    {
                        _this->red_led_state = led50Fast;
                        _this->green_led_state = ledOff;                        
                        buzzer_one_long = true; 
                    }

                    delay(2000);
                    break;
                }

        delay(10);

        uint32_t new_millis = millis();

        uint32_t diff = 0;

        if (new_millis < _millis)
        {
            diff = (0xfffffff - _millis) + new_millis;
        }
        else
        {
            diff = new_millis - _millis;
        }
        if (diff >= _this->program_timeout * 1000)
        {
            TRACE("program_task: timeout, no card submitted") 
            break;
        }
    }
    
    _this->red_led_state = ledOn;
    _this->green_led_state = ledOff;

    { Lock lock(_this->semaphore);
    _this->_is_programming = false; }

    TRACE("rfid_program_task: finished")
    vTaskDelete(NULL);
}

bool RfidLockHandler::rc522_check_read_mifare_sector(MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                                     MFRC522::Uid & uid, MIFARE_Classic_1K_Sector & sector_data)
{
    // if uid is set upon return but the return value is false then the tag was submitted 
    // but something went wrong (most likely, wrong key)

    // note: the key may change upon return, the function could have tried default key if the provided
    // one didn't work

    bool r = false;
    memset(& uid, 0, sizeof(uid));

    if (rc522)
    {
        if (rc522->PICC_IsNewCardPresent())
        {
            TRACE("RC522 new card present")

            bool wake_up = false; 
            r = rc522_read_mifare_sector(key, sector_index, uid, sector_data, wake_up);
        }
    }

    return r;
}

bool RfidLockHandler::rc522_read_mifare_sector(MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                               MFRC522::Uid & uid, MIFARE_Classic_1K_Sector & sector_data, bool wake_up)
{
    // if uid is set upon return but the return value is false then the tag was submitted 
    // but something went wrong (most likely, wrong key)

    // note: the key may change upon return, the function could have tried default key if the provided
    // one didn't work

    bool r = false;
    memset(& uid, 0, sizeof(uid));

    if (rc522)
    {
        size_t block_count = sizeof(MIFARE_Classic_1K_Sector::blocks)/sizeof(MIFARE_Classic_1K_Sector::blocks[0]);
        size_t block_index = sector_index * block_count;

        std::vector<std::pair<MFRC522::PICC_Command, MFRC522::MIFARE_Key>> keys;
        
        keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_A, key));

        keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_A, DEFAULT_MIFARE_KEY));
        keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_B, DEFAULT_MIFARE_KEY));

        keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_A, ZERO_MIFARE_KEY));
        keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_B, ZERO_MIFARE_KEY));

        MFRC522::StatusCode status = MFRC522::STATUS_ERROR;

        for (auto it=keys.begin(); it!= keys.end(); ++it)
        {
            if (wake_up == true)
            {
                byte atqa_answer[2];
                byte atqa_size = 2;
                rc522->PICC_WakeupA(atqa_answer, &atqa_size); // we don't care if it fails, the important is select
            }
            else
            {
                wake_up = true;
            }

            if (rc522->PICC_Select(& uid) == MFRC522::STATUS_OK)
            {
                TRACE("RC522 PICC select succeded, uid %s", uid_2_str(uid).c_str())

                MFRC522::MIFARE_Key _key = it->second;
                MFRC522::PICC_Command _key_type = it->first;

                status = rc522->PCD_Authenticate(_key_type, block_index, & _key, & uid);

                if (status == MFRC522::STATUS_OK) 
                {
                    TRACE("RC522 PCD authenticate with key %s (as %s) succeded", key_2_str(_key).c_str(), 
                            mifare_key_type_2_str(_key_type).c_str()) 
                    key = _key;
                    break;
                }
                else
                {
                    TRACE("RC522 PCD authenticate with key %s (as %s) failed", key_2_str(_key).c_str(), 
                            mifare_key_type_2_str(_key_type).c_str()) 
                }
            }
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

    return r;
}

bool RfidLockHandler::rc522_check_write_mifare_sector(const MFRC522::MIFARE_Key & key, uint8_t sector_index, 
                                                      MFRC522::Uid & uid, const MIFARE_Classic_1K_Sector & sector_data)
{
    // if uid is set upon return but the return value is false then the tag was submitted 
    // but something went wrong (most likely, wrong key)

    bool r = false;
    memset(& uid, 0, sizeof(uid));

    if (rc522)
    {
        if (rc522->PICC_IsNewCardPresent())
        {
            TRACE("RC522 new card present")

            size_t block_count = sizeof(MIFARE_Classic_1K_Sector::blocks)/sizeof(MIFARE_Classic_1K_Sector::blocks[0]);
            size_t block_index = sector_index * block_count;

            std::vector<std::pair<MFRC522::PICC_Command, MFRC522::MIFARE_Key>> keys;
            
            keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_B, key));

            keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_A, DEFAULT_MIFARE_KEY));
            keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_B, DEFAULT_MIFARE_KEY));

            keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_A, ZERO_MIFARE_KEY));
            keys.push_back(std::make_pair(MFRC522::PICC_CMD_MF_AUTH_KEY_B, ZERO_MIFARE_KEY));

            MFRC522::StatusCode status = MFRC522::STATUS_ERROR;

            bool wake_up = false;

            for (auto it=keys.begin(); it!= keys.end(); ++it)
            {
                if (wake_up == true)
                {
                    byte atqa_answer[2];
                    byte atqa_size = 2;
                    rc522->PICC_WakeupA(atqa_answer, &atqa_size); // we don't care if it fails, the important is select
                }
                else
                {
                    wake_up = true;
                }

                if (rc522->PICC_Select(& uid) == MFRC522::STATUS_OK)
                {
                    TRACE("RC522 PICC select succeded, uid %s", uid_2_str(uid).c_str())

                    MFRC522::MIFARE_Key _key = it->second;
                    MFRC522::PICC_Command _key_type = it->first;

			        status = rc522->PCD_Authenticate(_key_type, block_index, & _key, & uid);

                    if (status == MFRC522::STATUS_OK) 
                    {
                        TRACE("RC522 PCD authenticate with key %s (as %s) succeded", key_2_str(_key).c_str(), 
                              mifare_key_type_2_str(_key_type).c_str()) 
                        break;
                    }
                    else
                    {
                        TRACE("RC522 PCD authenticate with key %s (as %s) failed", key_2_str(_key).c_str(), 
                              mifare_key_type_2_str(_key_type).c_str()) 
                    }
                }
            }

            if (status == MFRC522::STATUS_OK) 
            {
                r = true;

                for (size_t i=0; i<block_count; ++i)
                {
                    uint8_t buffer[32];
                    uint8_t c = sizeof(sector_data.blocks[i].bytes);
                    
                    memcpy(buffer, sector_data.blocks[i].bytes, c);                                                

                    //DEBUG("writing block %d", (int)  block_index+i)
                    //sector_data.blocks[i].dump_debug();

                    status = rc522->MIFARE_Write(block_index+i, buffer, c);

                    if (status != MFRC522::STATUS_OK) 
                    {
                        TRACE("RC522 MIFARE write failed with status %d (%s)", (int) status, (const char*)rc522->GetStatusCodeName(status))
                        r = false;
                        break;
                    }
                }

                rc522->PICC_HaltA();
                rc522->PCD_StopCrypto1();
            }            
		}
    }

    return r;
}

bool RfidLockHandler::tag_submitted(const MIFARE_Classic_1K_Sector & sector_data)
{
    // extract code from sector data

    uint8_t block_index = 0;
    uint8_t pos = 0;

    uint8_t user_data_count = sector_data.blocks[block_index].bytes[pos];
    pos++;

    char buf[256];
    uint8_t buf_pos = 0;

    for(size_t i=0; i<user_data_count; ++i)
    {
        buf[buf_pos] = sector_data.blocks[block_index].bytes[pos];
        buf_pos++;    
        pos++;

        if (pos == sizeof(sector_data.blocks[block_index].bytes))
        {
            pos = 0;
            block_index++;

            if (block_index >= sizeof(sector_data.blocks)/sizeof(sector_data.blocks[0])-1)
            {
                break;
            }
        }
    } 
    
    buf[buf_pos] = 0;
    TRACE("Submitted tag with code %s", buf)

    String code(buf);

    bool found = false;
    String code_name;
    std::vector<String> locks;

    { Lock lock(semaphore);

        for (auto it=config.codes.codes.begin(); it!=config.codes.codes.end();++it)
        {
            if (it->second.type == RfidLockConfig::Codes::Code::tRFID && it->second.value == code)
            {
                found = true;
                code_name = it->first;
                locks = it->second.locks;
                break; 
            }
        }
    }

    if (found == true)
    {
        TRACE("Code found, code_name %s", code_name.c_str())
        buzzer_series_short = true; 
        commence_unlock(locks);
        return true; 
    }
    else
    {
        TRACE("Code not found")
        buzzer_one_long = true; 
        return false; 
    }
}

void RfidLockHandler::commence_unlock(const std::vector<String> & locks)
{
    Lock lock(semaphore);

    if (_is_programming == true)
    {
        TRACE("unlock cancelled due to ongoing programming")
        return;
    }

    static std::vector<String> locks_param;

    if (_is_unlocking == false)
    {
        _is_unlocking = true;

        locks_param.clear();

        // if incoming list of locks is empty == unlock all locks

        if (locks.empty() == false)
        {
            // ... otherwise match against existing channels in config.lock

            for (auto it=locks.begin(); it!=locks.end();++it)
            {
                auto channel_it = config.lock.channels.find(*it);

                if (channel_it != config.lock.channels.end())
                {
                    locks_param.push_back(*it);                    
                }
            }

            if (locks_param.empty() == true)
            {
                TRACE("unlock cancelled, no channels match locks list")
                _is_unlocking = false;
                return;
            }
        }
        else
        {
            if (config.lock.channels.empty() == true)
            {
                TRACE("unlock cancelled, no lock channels configured")
                _is_unlocking = false;
                return;
            }
        }

        static void * args[] = { this, & locks_param };

        //DEBUG("args 0x%x this 0x%x locks_param 0x%x", (uint32_t) args, (uint32_t) this, (uint32_t) &locks_param)

        xTaskCreate(
            unlock_task,           // Function that should be called
            "rfid_unlock_task",    // Name of the task (for debugging)
            4096,                  // Stack size (bytes)
            args,                  // Parameter to pass
            1,                     // Task priority
            NULL                   // Task handle
        ); 
    }
    else
    {
        TRACE("unlock cancelled, already ongoing")
    }
}

String RfidLockHandler::program(const String & code, uint16_t timeout)
{
    String r;
    Lock lock(semaphore);

    if (_is_unlocking == true)
    {
        r = "programming cancelled due to ongoing unlocking";
        TRACE(r.c_str())
        return r;
    }

    if (_is_programming == false)
    {
       _is_programming = true;

        program_code = code;
        program_timeout = timeout == 0 ? DEFAULT_PROGRAM_TIMEOUT : timeout;

        xTaskCreate(
            program_task,           // Function that should be called
            "rfid_program_task",    // Name of the task (for debugging)
            4096,                  // Stack size (bytes)
            this,                  // Parameter to pass
            1,                     // Task priority
            NULL                   // Task handle
        ); 
    }
    else
    {
        r = "programming cancelled, already ongoing";
        TRACE(r.c_str())
    }

    return r;
}

void RfidLockHandler::configure_hw()
{
    configure_hw_rfid(config.rfid);
    configure_hw_lock(config.lock);
    configure_hw_buzzer(config.buzzer);
    configure_hw_leds(config.green_led, config.red_led);
}

void RfidLockHandler::configure_hw_buzzer(const BuzzerConfig & config)
{
    start_buzzer_task(config);
}

void RfidLockHandler::configure_hw_leds(const DigitalOutputChannelConfig &green_led, const DigitalOutputChannelConfig &red_led)
{
    register_led(green_led, & green_led_state);
    register_led(red_led, & red_led_state);
}

void RfidLockHandler::unconfigure_hw_leds(const DigitalOutputChannelConfig &green_led, const DigitalOutputChannelConfig &red_led)
{
    unregister_led(green_led.gpio);
    unregister_led(red_led.gpio);
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
    for (auto it=config.channels.begin(); it!=config.channels.end(); ++it)
    {
        pinMode(it->second.gpio, OUTPUT);
        digitalWrite(it->second.gpio, it->second.inverted ? 1 : 0);
    }
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

String rfid_lock_program(const String & code_str, uint16_t timeout)
{
    return handler.program(code_str, timeout);
}

void rfid_lock_get_codes(RfidLockConfig::Codes & codes)
{
    handler.get_codes(codes);
}

void rfid_lock_update_codes(const RfidLockConfig::Codes & codes)
{
    handler.update_codes(codes);
}

#endif // INCLUDE_RFIDLOCK
