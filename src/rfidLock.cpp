#ifdef INCLUDE_RFIDLOCK

// #define INCLUDE_PN532 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>//https://www.arduino.cc/en/reference/SPI
#include <Wire.h>

#ifdef INCLUDE_PN532

#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

#endif

#include <rfidLock.h>
#include <autonom.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <epromImage.h>
#include <onboardLed.h>
#include <i2c_utils.h>

#include <sstream>

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


class RfidLockHandler
{
public:

    static const int DATA_EPROM_VERSION = 1;

    RfidLockHandler()
    {
        _is_active = false;
        _is_finished = true;

        _data_needs_save = false;

        rc522 = NULL;

        #ifdef INCLUDE_PN532
        pn532_i2c = NULL;
        nfc_adapter = NULL;
        #endif
        
        red_led_state = ledOff;
        green_led_state = ledOff;
        _is_unlocking = false;
        _is_programming = false;

        // the lock_preselect mechanism allows to select which of the allowed locks needs to be open after successful
        // scan or entering the code; the user does that by clicking the number of the lock (index) before authentication

        lock_preselect = -1;


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

    void get_codes(RfidLockConfig::Codes & _codes)
    {
        Lock lock(semaphore);
        _codes = codes;
    }

    void add_code(const String & name, const RfidLockConfig::Codes::Code & code)
    {
        Lock lock(semaphore);
        codes.codes[name] = code;
        _data_needs_save = true;        
    }

    bool delete_code(const String & name)
    {
        Lock lock(semaphore);
        
        if (codes.codes.find(name) == codes.codes.end())
        {
            return false;
        }
        
        codes.codes.erase(name);
        _data_needs_save = true;        
        return true;        
    }

    void delete_all_codes()
    {
        Lock lock(semaphore);
        codes.clear();
        _data_needs_save = true;        
    }

    bool unlock(int lock_channel)
    {
        Lock lock(semaphore);
        
        if (lock_channel < 0 || lock_channel >= config.lock.channels.size())
        {
            return false;
        }
        
        if (_is_programming == true || _is_unlocking == true)
        {
            return false;
        }

        std::vector<String> locks;
        locks.push_back(config.lock.channels[lock_channel].first);

        lock_preselect = -1;
        buzzer_series_short = true; 
        commence_unlock(locks);

        return true;        
    }

    bool does_data_need_save();
    void data_saved();

    void data_to_eprom(std::ostream & os);
    bool data_from_eprom(std::istream & is);

    bool read_data();
    void save_data();

protected:
    
    void clear_rfid()
    {
        if (rc522)
        {
            delete rc522;
            rc522 = NULL;
        }

        #ifdef INCLUDE_PN532

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

        #endif
    }

    void configure_hw();
    void configure_hw_rfid(const RfidLockConfig::Rfid &rfid);
    static void configure_hw_keypad(const KeypadConfig &);
    static void configure_hw_lock(const RfidLockConfig::Lock &lock);
    static void configure_hw_buzzer(const BuzzerConfig &);
    void configure_hw_leds(const DigitalOutputChannelConfig &green_led, const DigitalOutputChannelConfig &red_led);
    static void unconfigure_hw_leds(const DigitalOutputChannelConfig &green_led, const DigitalOutputChannelConfig &red_led);

    static void write_lock(const RfidLockConfig::Lock &lock, bool value);

    bool handle_keypad_input(const String &);

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

    RfidLockConfig::Codes codes;

    bool _is_active;
    bool _is_finished;

    bool _data_needs_save;

    MFRC522 * rc522;
    
    #ifdef INCLUDE_PN532

    PN532_I2C * pn532_i2c;
    NfcAdapter * nfc_adapter;

    #endif

    LedState green_led_state;
    LedState red_led_state;

    bool _is_unlocking;
    int lock_preselect;

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
    //codes = _config.codes;

    read_data();

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
    stop_keypad_task();
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


static void press_reflex()
{
    buzzer_one_short = true;
}


#ifdef INCLUDE_PN532

void readNFC(NfcAdapter & _nfc) 
{
    //return;
 if (_nfc.tagPresent())
 {
   NfcTag tag = _nfc.read();
   TRACE("NFC tag: %s", tag.getUidString().c_str())
 }
}

#endif


void RfidLockHandler::task(void *parameter)
{
    RfidLockHandler *_this = (RfidLockHandler *)parameter;

    TRACE("rfid_lock_task: started")
    buzzer_one_long = true;

    String keypad_input;

    const size_t LOCK_PRESELECT_TIMEOUT = 10; // seconds
    unsigned long last_lock_presellect_millis = millis();

    const size_t SAVE_DATA_INTERVAL = 10; // seconds
    unsigned long last_save_data_millis = millis();

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
        unsigned long now_millis = millis();

        if (now_millis < last_save_data_millis || 
            (now_millis-last_save_data_millis)/1000 >= SAVE_DATA_INTERVAL)
        {
            last_save_data_millis = now_millis;

            if (_this->does_data_need_save())
            {
                _this->save_data();
                _this->data_saved();
            }
        }
        
        if (_this->lock_preselect != -1)
        {
            if (now_millis < last_lock_presellect_millis || 
                (now_millis-last_lock_presellect_millis)/1000 >= LOCK_PRESELECT_TIMEOUT)
            {
               _this->lock_preselect = -1;
            }
        }

        if (_this->_is_unlocking == false && _this->_is_programming == false)
        {
            String keypad_queue = pop_keypad_queue();

            for (size_t i=0; i<keypad_queue.length(); ++i)
            {
                if (keypad_queue[i] == '*')
                {
                    keypad_input += keypad_queue[i];

                    if (keypad_input.length() >= 2)  // complete string
                    {
                        _this->handle_keypad_input(keypad_input);
                        keypad_input.clear();
                    }
                }
                else
                {
                    if (keypad_input.isEmpty())
                    {
                        if (strchr("0123456789", keypad_queue[i]) != NULL)
                        {
                            _this->lock_preselect = (int) (keypad_queue[i]-'0');
                            last_lock_presellect_millis = millis();
                        }                        
                        else
                        {
                            // the input should start and end with a '*'
                            // do nothing and warn with a signal

                            buzzer_one_long = true;
                        }
                    }
                    else
                    {
                        keypad_input += keypad_queue[i];
                    }
                }
            }

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

            #ifdef INCLUDE_PN532

            if (_this->nfc_adapter)
            {
                readNFC(*(_this->nfc_adapter));
            }

            #endif
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
    std::vector<std::pair<uint8_t, uint8_t>> commit_gpios;
    
    { Lock lock(_this->semaphore);

    linger = _this->config.lock.linger;

    if (_this->lock_preselect != -1)
    {
        TRACE("lock_preselect is set to %d", _this->lock_preselect)

        if (_this->lock_preselect >= 0 && _this->lock_preselect < _this->config.lock.channels.size())
        {
            auto preselect_channel = _this->config.lock.channels[_this->lock_preselect];

            if (locks_param->empty() == true || std::find(locks_param->begin(), locks_param->end(),preselect_channel.first) != locks_param->end())
            {
                TRACE("Will unlock channel %s", preselect_channel.first.c_str())
                commit_gpios.push_back(std::make_pair((uint8_t) preselect_channel.second.gpio, (uint8_t) preselect_channel.second.inverted));
            }
            else
            {
                ERROR("lock_preselect channel %d (%s) is valid but not in the unlock list for the given code", _this->lock_preselect, preselect_channel.first.c_str())
            }
        }
        else
        {
                ERROR("lock_preselect channel index %d is invalid", _this->lock_preselect)
        }

        _this->lock_preselect = -1;
    }
    else
    {
        for (auto it=_this->config.lock.channels.begin(); it!=_this->config.lock.channels.end(); ++it)
        {
            if (locks_param->empty() == true || 
                std::find(locks_param->begin(), locks_param->end(), it->first) != locks_param->end())
            {            
                TRACE("Will unlock channel %s", it->first.c_str())
                commit_gpios.push_back(std::make_pair((uint8_t) it->second.gpio, (uint8_t) it->second.inverted));
            }
        }
    }}

    // actuate locks one at a time to avoid dips in power consumption

    for (auto it=commit_gpios.begin(); it!=commit_gpios.end(); ++it)
    {
        digitalWrite(it->first, it->second ? 0 : 1);
        delay(linger * 1000);
        digitalWrite(it->first, it->second ? 1 : 0);
    }

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

bool RfidLockHandler::handle_keypad_input(const String & keypad_input)
{
    TRACE("keypad input %s", keypad_input.c_str())

    String code = keypad_input.substring(1,keypad_input.length()-1);

    bool found = false;
    String code_name;
    std::vector<String> locks;

    DEBUG("code %s", code.c_str())

    if (!code.isEmpty())
    {
        { Lock lock(semaphore);

            for (auto it=codes.codes.begin(); it!=codes.codes.end();++it)
            {
                if (it->second.type == RfidLockConfig::Codes::Code::tKeypad && it->second.value == code)
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
        }
    }

    return false;
}   

bool RfidLockHandler::does_data_need_save() 
{
    return _data_needs_save;
}

void RfidLockHandler::data_saved() 
{
    _data_needs_save = false;
}

void RfidLockHandler::data_to_eprom(std::ostream &os) 
{
    Lock lock(semaphore);

    DEBUG("RfidLockHandler data_to_eprom")

    uint8_t eprom_version = (uint8_t)DATA_EPROM_VERSION;
    os.write((const char *)&eprom_version, sizeof(eprom_version));

    codes.to_eprom(os);
}

bool RfidLockHandler::data_from_eprom(std::istream &is)
{
    uint8_t eprom_version = DATA_EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    DEBUG("RfidLockHandler data_from_eprom")

    if (eprom_version == DATA_EPROM_VERSION)
    {
        DEBUG("Version match")

        Lock lock(semaphore);
        codes.from_eprom(is);
    }

    if (is.bad())
    {
        ERROR("error reading codes")
        return false;
    }

    return true;
}


bool RfidLockHandler::read_data() 
{
    Lock lock(semaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);

    TRACE("RfidLockHandler reading data from EEPROM")

    if (dataVolume.read() == true)
    {
        for (auto it = dataVolume.blocks.begin(); it != dataVolume.blocks.end(); ++it)
        {
            if(it->first == ftRfidLock)
            {
                const char * function_type_str = function_type_2_str((FunctionType) it->first);
                TRACE("Found block type for function %s", function_type_str)

                std::istringstream is(it->second);

                if (data_from_eprom(is) == true)
                {
                    TRACE("RfidLockHandler data read success")
                    return true;
                }
                else
                {
                    TRACE("RfidLockHandler data read failure")
                }
            }
        }
    }
    else
    {
        ERROR("Cannot read eprom image (data)")
    }

    return false;
}

void RfidLockHandler::save_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);
    dataVolume.read();

    std::ostringstream os;

    TRACE("Saving RfidLock data to EEPROM")
    data_to_eprom(os);

    std::string buffer = os.str();
    TRACE("block size %d", (int) os.tellp())
    
    if (dataVolume.blocks.find((uint8_t) ftRfidLock) == dataVolume.blocks.end())
    {
        dataVolume.blocks.insert({(uint8_t) ftRfidLock, buffer});
    }
    else
    {
        if (dataVolume.blocks[(uint8_t) ftRfidLock] == buffer)
        {
            TRACE("Data identical, skip saving")
            return;
        }
        else
        {
            dataVolume.blocks[(uint8_t) ftRfidLock] = buffer;
        }
    }
    
    if (dataVolume.write())
    {
        TRACE("RfidLock data save success")
    }
    else
    {
        TRACE("RfidLock data save failure")
    }
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

        for (auto it=codes.codes.begin(); it!=codes.codes.end();++it)
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
                for (auto jt=config.lock.channels.begin(); jt!=config.lock.channels.end(); ++jt)
                {
                    if (jt->first == *it)
                    {
                        locks_param.push_back(*it);                    
                        break;
                    }
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
    configure_hw_keypad(config.keypad);
    configure_hw_leds(config.green_led, config.red_led);
}

void RfidLockHandler::configure_hw_buzzer(const BuzzerConfig & config)
{
    start_buzzer_task(config);
}

void RfidLockHandler::configure_hw_keypad(const KeypadConfig & config)
{
    start_keypad_task(config, press_reflex);
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
        #ifdef INCLUDE_PN532

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
        #else
            
        ERROR("no support for PN532/%s", RfidLockConfig::Rfid::protocol_2_str(_config.protocol))            

        #endif

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

bool __is_number(const String & value)
{
    size_t c = 0;

    for (size_t i=0; i<value.length(); ++i)
    {
        if (!(isdigit(value[i]) || value[i] == '.'))
        {
            return false;
        } 
        else
        {
            c++;
        }
    }
    
    if (c == 0)
    {
        return false;
    }
    
    return true;
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

void rfid_lock_get_codes(JsonVariant & json_variant)
{
    RfidLockConfig::Codes codes;
    handler.get_codes(codes);
    codes.to_json(json_variant);
}

String rfid_lock_add_code(const String & name, const RfidLockConfig::Codes::Code & code)
{
    handler.add_code(name, code);
    return String();
}

String rfid_lock_delete_code(const String & name)
{
    if (handler.delete_code(name) == true)
    {
        return String();
    }
    else
    {
        return String("code name invalid");
    }
}

String rfid_lock_delete_all_codes()
{
    handler.delete_all_codes();
    return String();
}

String rfid_lock_unlock(const String & lock_channel_str)
{
    if (__is_number(lock_channel_str))
    {
        int lock_channel = (int) lock_channel_str.toInt();

        if (handler.unlock(lock_channel))
        {
            return String();
        }
        else
        {
            return String("lock_channel does not exist or busy");    
        }
    }
    else
    {
        return String("lock_channel must be integer");
    }
}

#endif // INCLUDE_RFIDLOCK
