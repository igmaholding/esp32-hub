#include <ArduinoJson.h>
#include <keyBox.h>
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

bool KeyBoxConfig::is_valid() const
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

    object_name = "actuator.coil";

    if (checkpad.get_usage(actuator.coil.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)actuator.coil.gpio);
        return false;
    }

    if (!checkpad.set_usage(actuator.coil.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)actuator.coil.gpio);
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

void KeyBoxConfig::from_json(const JsonVariant &json)
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

void KeyBoxConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    buzzer.to_eprom(os);
    keypad.to_eprom(os);
    actuator.to_eprom(os);
    codes.to_eprom(os);
}

bool KeyBoxConfig::from_eprom(std::istream &is)
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
        ERROR("Failed to read KeyBoxConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}


void KeyBoxConfig::Actuator::from_json(const JsonVariant &json)
{
    if (json.containsKey("addr"))
    {
        const JsonVariant &_json = json["addr"];

        if (_json.is<JsonArray>())
        {
            size_t i=0;

            for (; i<sizeof(addr)/sizeof(addr[0]); ++i)
            {
                addr[i].clear();
            }
            
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();
            i=0;

            while(iterator != jsonArray.end() && i<sizeof(addr)/sizeof(addr[0]))
            {
                const JsonVariant & __json = *iterator;

                if (__json.containsKey("channel"))
                {
                    const JsonVariant &_json_channel = __json["channel"];
                    addr[i].from_json(_json_channel);
                }

                ++iterator;
                ++i;;
            }
        }
    }

    if (json.containsKey("coil"))
    {
        const JsonVariant &_json = json["coil"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &_json_channel = _json["channel"];
            coil.from_json(_json_channel);
        }
    }

    if (json.containsKey("status"))
    {
        const JsonVariant &_json = json["status"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &_json_channel = _json["channel"];
            status.from_json(_json_channel);
        }
    }
}

void KeyBoxConfig::Actuator::to_eprom(std::ostream &os) const
{
    for (size_t i=0; i<sizeof(addr)/sizeof(addr[0]); ++i)
    {
        addr[i].to_eprom(os);
    }

    coil.to_eprom(os);
    status.to_eprom(os);
}

bool KeyBoxConfig::Actuator::from_eprom(std::istream &is)
{
    for (size_t i=0; i<sizeof(addr)/sizeof(addr[0]); ++i)
    {
        addr[i].from_eprom(is);
    }

    coil.from_eprom(is);
    status.from_eprom(is);
    return is_valid() && !is.bad();
}

void KeyBoxConfig::Actuator::Channel::from_json(const JsonVariant &json)
{
    if (json.containsKey("gpio"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["gpio"]);
        gpio = GpioChannel::validateGpioNum(gpio_unvalidated);
    }
    if (json.containsKey("inverted"))
    {
        inverted = (bool)((int)json["inverted"]);
    }
}

void KeyBoxConfig::Actuator::Channel::to_eprom(std::ostream &os) const
{
    uint8_t gpio_uint8 = (uint8_t)gpio;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    int _inverted = inverted;
    os.write((const char *)&_inverted, sizeof(_inverted));
}

bool KeyBoxConfig::Actuator::Channel::from_eprom(std::istream &is)
{
    int8_t gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    gpio = (gpio_num_t)gpio_int8;

    int _inverted = 0;
    is.read((char *)&_inverted, sizeof(_inverted));
    inverted = (bool) _inverted;
    return is_valid() && !is.bad();
}

void KeyBoxConfig::Codes::from_json(const JsonVariant &json)
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

void KeyBoxConfig::Codes::to_eprom(std::ostream &os) const
{
    for (size_t i=0; i<sizeof(code)/sizeof(code[0]); ++i)
    {
        code[i].to_eprom(os);
    }
}

bool KeyBoxConfig::Codes::from_eprom(std::istream &is)
{
    for (size_t i=0; i<sizeof(code)/sizeof(code[0]); ++i)
    {
        code[i].from_eprom(is);
    }
    return is_valid() && !is.bad();
}

void KeyBoxConfig::Codes::Code::from_json(const JsonVariant &json)
{
    if (json.containsKey("value"))
    {
        value = (const char *)json["value"];
    }
}

void KeyBoxConfig::Codes::Code::to_eprom(std::ostream &os) const
{
    uint8_t len = value.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(value.c_str(), len);
    }
}

bool KeyBoxConfig::Codes::Code::from_eprom(std::istream &is)
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



class KeyBoxHandler
{
public:

    KeyBoxHandler()
    {
        _is_active = false;
        _is_finished = true;
    }

    bool is_active() const { return _is_active; }

    void start(const KeyBoxConfig &config);
    void stop();
    void reconfigure(const KeyBoxConfig &config);

    KeyBoxStatus get_status()
    {
        KeyBoxStatus _status;

        {
            Lock lock(semaphore);
            _status = status;
        }

        return _status;
    }


protected:
    void configure_hw();
    static void configure_hw_buzzer(const BuzzerConfig &);
    static void configure_hw_keypad(const KeypadConfig &);
    static void configure_hw_actuator(const KeyBoxConfig::Actuator &);

    static void task(void *parameter);

    BinarySemaphore semaphore;
    KeyBoxConfig config;
    KeyBoxStatus status;
    bool _is_active;
    bool _is_finished;

};

static KeyBoxHandler handler;

void KeyBoxHandler::start(const KeyBoxConfig &_config)
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

void KeyBoxHandler::stop()
{
    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }

    stop_buzzer_task();
    stop_keypad_task();
}

void KeyBoxHandler::reconfigure(const KeyBoxConfig &_config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("keybox_task: config changed")
        config = _config;
        configure_hw();
    }
}

void KeyBoxHandler::task(void *parameter)
{
    KeyBoxHandler *_this = (KeyBoxHandler *)parameter;

    TRACE("keybox_task: started")
    
    buzzer_one_long = true;
    
    while (_this->_is_active)
    {
        delay(1000);
    }

    _this->_is_finished = true;
    TRACE("keybox_task: finished")
    vTaskDelete(NULL);
}

void KeyBoxHandler::configure_hw()
{
    configure_hw_buzzer(config.buzzer);
    configure_hw_keypad(config.keypad);
    configure_hw_actuator(config.actuator);
}

void KeyBoxHandler::configure_hw_buzzer(const BuzzerConfig & config)
{
    start_buzzer_task(config);
}


void KeyBoxHandler::configure_hw_keypad(const KeypadConfig & config)
{
    start_keypad_task(config);
}


void KeyBoxHandler::configure_hw_actuator(const KeyBoxConfig::Actuator &)
{
}


void start_keybox_task(const KeyBoxConfig &config)
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

KeyBoxStatus get_keybox_status()
{
    return handler.get_status();
}

void reconfigure_keybox(const KeyBoxConfig &_config)
{
    handler.reconfigure(_config);
}
