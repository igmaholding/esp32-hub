#include <ArduinoJson.h>
#include <keybox.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>

extern GpioHandler gpioHandler;

static void _err_dup(const char *name, int value)
{
    ERROR("%s %d is duplicated / reused", name, value)
}

static void _err_cap(const char *name, int value)
{
    ERROR("%s %d, gpio doesn't have required capabilities", name, value)
}

bool KeyboxConfig::is_valid() const
{
    bool r = buzzer.is_valid() && keypad.is_valid() && actuator.is_valid() && codes.is_valid();

    if (r == false)
    {
        return false;
    }

    GpioCheckpad checkpad;

    String object_name = "buzzer.channel.gpio";

    if (checkpad.get_usage(buzzer.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)buzzer.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(buzzer.channel.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)buzzer.channel.gpio);
        return false;
    }

    for (size_t i=0; i<sizeof(keypad.c)/sizeof(keypad.c[0]); ++i)
    {
        object_name = "keypad.c[" + String(i) + "]";

        if (checkpad.get_usage(keypad.c[i].gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)keypad.c[i].gpio);
            return false;
        }

        if (!checkpad.set_usage(keypad.c[i].gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)keypad.c[i].gpio);
            return false;
        }
    }

    for (size_t i=0; i<sizeof(keypad.l)/sizeof(keypad.l[0]); ++i)
    {
        object_name = "keypad.l[" + String(i) + "]";

        if (checkpad.get_usage(keypad.l[i].gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)keypad.l[i].gpio);
            return false;
        }

        if (!checkpad.set_usage(keypad.l[i].gpio, GpioCheckpad::uDigitalInput))
        {
            _err_cap(object_name.c_str(), (int)keypad.l[i].gpio);
            return false;
        }
    }

    for (size_t i=0; i<sizeof(actuator.addr)/sizeof(actuator.addr[0]); ++i)
    {
        object_name = "actuator.addr[" + String(i) + "]";

        if (checkpad.get_usage(actuator.addr[i].gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)actuator.addr[i].gpio);
            return false;
        }

        if (!checkpad.set_usage(actuator.addr[i].gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)actuator.addr[i].gpio);
            return false;
        }
    }

    object_name = "actuator.latch";

    if (checkpad.get_usage(actuator.latch.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)actuator.latch.gpio);
        return false;
    }

    if (!checkpad.set_usage(actuator.latch.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)actuator.latch.gpio);
        return false;
    }

    object_name = "actuator.power";

    if (checkpad.get_usage(actuator.power.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)actuator.power.gpio);
        return false;
    }

    if (!checkpad.set_usage(actuator.power.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)actuator.power.gpio);
        return false;
    }

    object_name = "actuator.status";

    if (checkpad.get_usage(actuator.status.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)actuator.status.gpio);
        return false;
    }

    if (!checkpad.set_usage(actuator.status.gpio, GpioCheckpad::uDigitalInput))
    {
        _err_cap(object_name.c_str(), (int)actuator.status.gpio);
        return false;
    }

    return true;
}

void KeyboxConfig::from_json(const JsonVariant &json)
{
    if (json.containsKey("buzzer"))
    {
        const JsonVariant &_json = json["buzzer"];
        buzzer.from_json(_json);
    }

    if (json.containsKey("keypad"))
    {
        const JsonVariant &_json = json["keypad"];
        keypad.from_json(_json);
    }

    if (json.containsKey("actuator"))
    {
        const JsonVariant &_json = json["actuator"];
        actuator.from_json(_json);
    }

    if (json.containsKey("codes"))
    {
        const JsonVariant &_json = json["codes"];
        codes.from_json(_json);
    }
}

void KeyboxConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    buzzer.to_eprom(os);
    keypad.to_eprom(os);
    actuator.to_eprom(os);
    codes.to_eprom(os);
}

bool KeyboxConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        buzzer.from_eprom(is);
        keypad.from_eprom(is);
        actuator.from_eprom(is);
        codes.from_eprom(is);
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read KeyboxConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}


void KeyboxConfig::Codes::from_json(const JsonVariant &json)
{
    if (json.containsKey("code"))
    {
        const JsonVariant &_json = json["code"];

        if (_json.is<JsonArray>())
        {
            size_t i=0;

            for (; i<sizeof(code)/sizeof(code[0]); ++i)
            {
                code[i].clear();
            }
            
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();
            i=0;

            while(iterator != jsonArray.end() && i<sizeof(code)/sizeof(code[0]))
            {
                const JsonVariant & __json = *iterator;
                code[i].from_json(__json);

                ++iterator;
                ++i;
            }
        }
    }
}

void KeyboxConfig::Codes::to_eprom(std::ostream &os) const
{
    for (size_t i=0; i<sizeof(code)/sizeof(code[0]); ++i)
    {
        code[i].to_eprom(os);
    }
}

bool KeyboxConfig::Codes::from_eprom(std::istream &is)
{
    for (size_t i=0; i<sizeof(code)/sizeof(code[0]); ++i)
    {
        code[i].from_eprom(is);
    }
    return is_valid() && !is.bad();
}

void KeyboxConfig::Codes::Code::from_json(const JsonVariant &json)
{
    if (json.containsKey("value"))
    {
        value = (const char *)json["value"];
    }
}

void KeyboxConfig::Codes::Code::to_eprom(std::ostream &os) const
{
    uint8_t len = value.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(value.c_str(), len);
    }
}

bool KeyboxConfig::Codes::Code::from_eprom(std::istream &is)
{
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        value = buf;
    }
    else
    {
        value = "";
    }
    return is_valid() && !is.bad();
}



class KeyboxHandler
{
public:

    KeyboxHandler()
    {
        _is_active = false;
        _is_finished = true;
    }

    bool is_active() const { return _is_active; }

    void start(const KeyboxConfig &config);
    void stop();
    void reconfigure(const KeyboxConfig &config);
    void actuate(size_t channel_number);

protected:
    void configure_hw();
    static void configure_hw_buzzer(const BuzzerConfig &);
    static void configure_hw_keypad(const KeypadConfig &);
    static void configure_hw_actuator(const KeyboxActuatorConfig &);

    void handle_keypad_input(const String &);

    static void task(void *parameter);

    BinarySemaphore semaphore;
    KeyboxConfig config;
    bool _is_active;
    bool _is_finished;

};

static KeyboxHandler handler;

static void press_reflex()
{
    buzzer_one_short = true;
}

void KeyboxHandler::start(const KeyboxConfig &_config)
{
    if (_is_active)
    {
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    Lock lock(semaphore);
    config = _config;
    configure_hw();

    _is_active = true;
    _is_finished = false;

    xTaskCreate(
        task,                // Function that should be called
        "keybox_task", // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void KeyboxHandler::stop()
{
    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }

    stop_buzzer_task();
    stop_keypad_task();
    stop_keybox_actuator_task();
}

void KeyboxHandler::reconfigure(const KeyboxConfig &_config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("keybox_task: config changed")
        config = _config;
        configure_hw();
    }
}

void KeyboxHandler::handle_keypad_input(const String & keypad_input)
{
    TRACE("keypad input %s", keypad_input.c_str())

    String keypad_input_value = keypad_input.substring(1,keypad_input.length()-1);

    Lock lock(semaphore);
    size_t channel = size_t(-1);

    if (!keypad_input_value.isEmpty())
    {
        for (size_t i=0; i<KEYBOX_NUM_CHANNELS; ++i)
        {
            if (keypad_input_value == config.codes.code[i].value)
            {
                channel = i;
                break;
            }
        }
    }

    if (channel != size_t(-1))
    {
        TRACE("code valid for channel %d", (int) channel)
        actuate(channel);
    }
    else
    {
        TRACE("code invalid")
        buzzer_one_long = true;
    }
}   

void KeyboxHandler::actuate(size_t channel)
{
    ::actuate_channel(channel);
    delay(100);
    buzzer_series_short = true;
}

void KeyboxHandler::task(void *parameter)
{
    KeyboxHandler *_this = (KeyboxHandler *)parameter;

    TRACE("keybox_task: started")
    
    buzzer_one_long = true;
    
    String keypad_input;

    while (_this->_is_active)
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
                    // the input should start and end with a '*'
                    // do nothing and warn with a signal

                    buzzer_one_long = true;
                }
                else
                {
                    keypad_input += keypad_queue[i];
                }
            }
        }

        delay(100);
    }

    _this->_is_finished = true;
    TRACE("keybox_task: finished")
    vTaskDelete(NULL);
}

void KeyboxHandler::configure_hw()
{
    TRACE("keybox_task: configure_hw")
    configure_hw_buzzer(config.buzzer);
    configure_hw_keypad(config.keypad);
    configure_hw_actuator(config.actuator);
    TRACE("keybox_task: configure_hw done")
}

void KeyboxHandler::configure_hw_buzzer(const BuzzerConfig & config)
{
    start_buzzer_task(config);
}


void KeyboxHandler::configure_hw_keypad(const KeypadConfig & config)
{
    start_keypad_task(config, press_reflex);
}


void KeyboxHandler::configure_hw_actuator(const KeyboxActuatorConfig & config)
{
    start_keybox_actuator_task(config);
}


void start_keybox_task(const KeyboxConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start keybox_task while it is running, redirecting to reconfigure")
        reconfigure_keybox(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_keybox_task()
{
    handler.stop();
}

void reconfigure_keybox(const KeyboxConfig &_config)
{
    handler.reconfigure(_config);
}


String keybox_actuate(const String & channel_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        for (size_t i=0; i<channel_str.length(); ++i)
        {
            if (isdigit(channel_str[i]) == false)
            {
                param_ok = false;
                break;
            } 
        }

        if (param_ok)
        {
            size_t channel = (size_t)  channel_str.toInt();

            if (channel >= 0 && channel < KEYBOX_NUM_CHANNELS)
            {
                handler.actuate(channel);
                return String();
            }
            else
            {
                return "Channel out of range";
            }
        }
    }
    else
    {
        param_ok = false;
    }
    
    return "Parameter error";
}
