#ifdef INCLUDE_ZERO2TEN
#include <ArduinoJson.h>
#include <zero2ten.h>
#include <autonom.h>
#include <gpio.h>
#include <binarySemaphore.h>
#include <Wire.h>
#include <deque>
#include <epromImage.h>
#include <sstream>

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

bool Zero2tenConfig::is_valid() const
{
    GpioCheckpad checkpad;

    size_t i = 0;

    for (auto it = input_channels.begin(); it != input_channels.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
            ERROR("input_channel %d is_valid() == false", (int) i)
            return false;
        }

        char object_name[64];
        sprintf(object_name, "input_channel[%d]", (int) i);

        if (checkpad.get_usage(it->gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->gpio);
            return false;
        }

        if (!checkpad.set_usage(it->gpio, GpioCheckpad::uAnalogInput))
        {
            _err_cap(object_name, (int)it->gpio);
            return false;
        }
    }
    
    i=0;

    for (auto it = output_channels.begin(); it != output_channels.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
            ERROR("output_channels %d is_valid() == false", (int) i)
            return false;
        }

        char object_name[64];
        sprintf(object_name, "output_channels[%d].output", (int) i);

        if (checkpad.get_usage(it->output.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->output.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->output.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name, (int)it->output.gpio);
            return false;
        }

        sprintf(object_name, "output_channels[%d].loopback", (int) i);

        if (checkpad.get_usage(it->loopback.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->loopback.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->loopback.gpio, GpioCheckpad::uAnalogInput))
        {
            _err_cap(object_name, (int)it->loopback.gpio);
            return false;
        }
    }

    return true;
}


void Zero2tenConfig::from_json(const JsonVariant &json)
{
    //DEBUG("zero2ten config from_json")
    clear();

    if (json.containsKey("input_channels"))
    {
        const JsonVariant &_json = json["input_channels"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                InputChannel input_channel;
                input_channel.from_json(__json);
                input_channels.push_back(input_channel);
                //DEBUG("Channel from_json, %s", input_channel.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("output_channels"))
    {
        const JsonVariant &_json = json["output_channels"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                OutputChannel output_channel;
                output_channel.from_json(__json);
                output_channels.push_back(output_channel);
                //DEBUG("Channel from_json, %s", output_channel.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("applets"))
    {
        //DEBUG("contains key applets")
        const JsonVariant &_json = json["applets"];

        if (_json.is<JsonArray>())
        {
            //DEBUG("applets is array")
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                //DEBUG("analysing applets item")
                const JsonVariant & __json = *iterator;

                if (__json.containsKey("name"))
                {
                    String name = __json["name"]; 
                    Applet applet;
                    applet.from_json(__json);
                    applets.insert(std::make_pair(name,applet));
                    //DEBUG("applet from_json: name=%s, %s", name.c_str(), applet.as_string().c_str())
                }
                ++iterator;
            }
        }
    }
}

void Zero2tenConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));

    uint8_t count = (uint8_t)input_channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = input_channels.begin(); it != input_channels.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)output_channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = output_channels.begin(); it != output_channels.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)applets.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = applets.begin(); it != applets.end(); ++it)
    {
        uint8_t len = it->first.length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->first.c_str(), len);

        it->second.to_eprom(os);
    }
}

bool Zero2tenConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        clear();

        uint8_t count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            InputChannel input_channel;
            input_channel.from_eprom(is);
            input_channels.push_back(input_channel);
        }

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            OutputChannel output_channel;
            output_channel.from_eprom(is);
            output_channels.push_back(output_channel);
        }

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            uint8_t len = 0;
            is.read((char *)&len, sizeof(len));

            if (len)
            {
                char buf[256];
                is.read(buf, len);
                buf[len] = 0;
                String name = buf;

                Applet applet;
                applet.from_eprom(is);
                applets[name] = applet;

            }
        }
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read Zero2tenConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void Zero2tenConfig::InputChannel::from_json(const JsonVariant &json)
{
    clear();    
    AnalogInputChannelConfig::from_json(json);
}

void Zero2tenConfig::InputChannel::to_eprom(std::ostream &os) const
{
    AnalogInputChannelConfig::to_eprom(os);
}

bool Zero2tenConfig::InputChannel::from_eprom(std::istream &is)
{
    clear();
    AnalogInputChannelConfig::from_eprom(is);

    return is_valid() && !is.bad();
}

void Zero2tenConfig::OutputChannel::from_json(const JsonVariant &json)
{
    clear();
    
    if (json.containsKey("output"))
    {
        const JsonVariant &_json = json["output"];
        output.from_json(_json);
    }

    if (json.containsKey("loopback"))
    {
        const JsonVariant &_json = json["loopback"];
        output.from_json(_json);
    }
}

void Zero2tenConfig::OutputChannel::to_eprom(std::ostream &os) const
{
    output.to_eprom(os);
    loopback.to_eprom(os);
}

bool Zero2tenConfig::OutputChannel::from_eprom(std::istream &is)
{
    clear();

    output.from_eprom(is);
    loopback.from_eprom(is);

    return is_valid() && !is.bad();
}

void Zero2tenConfig::Applet::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("function"))
    {
        String function_str = json["function"];
        function = str_2_function(function_str.c_str());
    }

    if (json.containsKey("input_channel"))
    {
        input_channel = json["input_channel"];
    }

    if (json.containsKey("output_channel"))
    {
        output_channel = json["output_channel"];
    }
}

void Zero2tenConfig::Applet::to_eprom(std::ostream &os) const
{
    os.write((const char *)&function, sizeof(function));
    os.write((const char *)&input_channel, sizeof(input_channel));
    os.write((const char *)&output_channel, sizeof(output_channel));
}

bool Zero2tenConfig::Applet::from_eprom(std::istream &is)
{
    clear();

    is.read((char *)&function, sizeof(function));
    is.read((char *)&input_channel, sizeof(input_channel));
    is.read((char *)&output_channel, sizeof(output_channel));

    return is_valid() && !is.bad();
}


class Zero2tenHandler
{
public:

    const int DATA_EPROM_VERSION = 1;

    Zero2tenHandler()
    {
        _is_active = false;
        _is_finished = true;
    }

    ~Zero2tenHandler()
    {
    }

    bool is_active() const { return _is_active; }

    void start(const Zero2tenConfig &config);
    void stop();
    void reconfigure(const Zero2tenConfig &config);

    Zero2tenStatus get_status()
    {
        Zero2tenStatus _status;

        Lock lock(semaphore);
        _status = status;

        return _status;
    }

    String set(size_t channel, float value);
    float get(size_t channel);

    bool does_data_need_save();
    void data_saved();

    void data_to_eprom(std::ostream & os);
    bool data_from_eprom(std::istream & is);

    bool read_data();
    void save_data();

    void update_applets();

protected:

    void configure_channels();

    static void task(void *parameter);

    BinarySemaphore semaphore;
    Zero2tenConfig config;
    Zero2tenStatus status;
    bool _is_active;
    bool _is_finished;
};

static Zero2tenHandler handler;


void Zero2tenHandler::start(const Zero2tenConfig &_config)
{
    TRACE("starting zero2ten handler")
    //Serial.write("DIRECT: starting zero2ten handler");

    if (_is_active)
    {
        ERROR("zero2ten handler already running")
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    config = _config;

    configure_channels();
    read_data();

    // update_applets();  // already included in configure_channels

    _is_active = true;
    _is_finished = false;

    TRACE("starting zero2ten handler task")

    xTaskCreate(
        task,                // Function that should be called
        "zero2ten_task", // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void Zero2tenHandler::stop()
{
    TRACE("stopping zero2ten handler")

    if (_is_active)
    {
    }

    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }
}

void Zero2tenHandler::reconfigure(const Zero2tenConfig &_config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("zero2ten_task: config changed")

        bool should_configure_channels = (config.input_channels == _config.input_channels && 
                                          config.output_channels == _config.output_channels) ? false : true;

        bool should_update_applets = config.applets == _config.applets  ? false : true;

        config = _config;

        if (should_configure_channels)
        {
            configure_channels();
        }
        else
        {
            if (should_update_applets)
            {
                update_applets();
            }
        }
    }
}

void Zero2tenHandler::task(void *parameter)
{
    Zero2tenHandler *_this = (Zero2tenHandler *)parameter;

    TRACE("zero2ten_task: started")

    const size_t SAVE_DATA_INTERVAL = 10; // seconds
    unsigned long last_save_data_millis = millis();

    while (_this->_is_active)
    {
        unsigned long now_millis = millis();

        if (now_millis < last_save_data_millis || 
            (now_millis-last_save_data_millis)/1000 >= SAVE_DATA_INTERVAL)
        {
            last_save_data_millis = now_millis;

            if (_this->does_data_need_save())
            {
                TRACE("saving zero2ten data to EPROM")
                _this->save_data();
                _this->data_saved();
            }
        }

        delay(1000);
    }

    _this->_is_finished = true;

    TRACE("zero2ten_task: terminated")
    vTaskDelete(NULL);
}

void Zero2tenHandler::configure_channels()
{
    TRACE("configure channels")
}

void Zero2tenHandler::update_applets()
{    
    TRACE("update_applets")
}

String Zero2tenHandler::set(size_t channel, float value)
{
    TRACE("setting channel %d value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);
    return r;
}

float Zero2tenHandler::get(size_t channel)
{
    TRACE("getting value of channel %d", (int) channel)
    float r = 0;

    Lock lock(semaphore);
    return r;
}

bool Zero2tenHandler::does_data_need_save() 
{
    Lock lock(semaphore);
    return false;
}

void Zero2tenHandler::data_saved() 
{
    Lock lock(semaphore);
}

void Zero2tenHandler::data_to_eprom(std::ostream &os) 
{
    Lock lock(semaphore);

    DEBUG("Zero2tenHandler data_to_eprom")

    uint8_t eprom_version = (uint8_t)DATA_EPROM_VERSION;
    os.write((const char *)&eprom_version, sizeof(eprom_version));
}

bool Zero2tenHandler::data_from_eprom(std::istream &is)
{
    uint8_t eprom_version = DATA_EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    DEBUG("Zero2tenHandler data_from_eprom")

    if (eprom_version == DATA_EPROM_VERSION)
    {
        DEBUG("Version match")

        Lock lock(semaphore);

        DEBUG("actuating to values being read")
    }
    else
    {
        ERROR("Failed to read zero2ten data from EPROM: version mismatch, expected %d, found %d", (int)DATA_EPROM_VERSION, (int)eprom_version)
        return false;
    }

    return !is.bad();
}


bool Zero2tenHandler::read_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);

    TRACE("Zero2tenHandler reading data from EEPROM")

    if (dataVolume.read() == true)
    {
        for (auto it = dataVolume.blocks.begin(); it != dataVolume.blocks.end(); ++it)
        {
            if(it->first == ftZero2ten)
            {
            }
        }
    }
    else
    {
        ERROR("Cannot read EEPROM image (data)")
    }

    return false;
}

void Zero2tenHandler::save_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);
    dataVolume.read();

    std::ostringstream os;

    TRACE("Saving zero2ten data to EEPROM")
    data_to_eprom(os);

    std::string buffer = os.str();
    TRACE("block size %d", (int) os.tellp())
    
    if (dataVolume.blocks.find((uint8_t) ftZero2ten) == dataVolume.blocks.end())
    {
        dataVolume.blocks.insert({(uint8_t) ftZero2ten, buffer});
    }
    else
    {
        if (dataVolume.blocks[(uint8_t) ftZero2ten] == buffer)
        {
            TRACE("Data identical, skip saving")
            return;
        }
        else
        {
            dataVolume.blocks[(uint8_t) ftZero2ten] = buffer;
        }
    }
    
    if (dataVolume.write())
    {
        TRACE("Zero2ten data save success")
    }
    else
    {
        TRACE("Zero2ten data save failure")
    }
}

void start_zero2ten_task(const Zero2tenConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start zero2ten_task while it is running, redirecting to reconfigure")
        reconfigure_zero2ten(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_zero2ten_task()
{
    handler.stop();
}

Zero2tenStatus get_zero2ten_status()
{
    return handler.get_status();
}

void reconfigure_zero2ten(const Zero2tenConfig &_config)
{
    handler.reconfigure(_config);
}


bool __is_number_or_empty(const String & value)
{
    for (size_t i=0; i<value.length(); ++i)
    {
        if (!(isdigit(value[i]) || value[i] == '.'))
        {
            return false;
        } 
    }
    return true;
}

String zero2ten_set(const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty() && !value_str.isEmpty())
    {
        param_ok = __is_number_or_empty(channel_str) && __is_number_or_empty(value_str);

        size_t channel = (size_t)  channel_str.toInt();
        float value = (float)  value_str.toFloat();
        
        DEBUG("validating channel number")
        return handler.set(channel, value);
    }
    else
    {
        param_ok = false;
    }
    
    return "Parameter error";
}

String zero2ten_get(const String & channel_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        param_ok = __is_number_or_empty(channel_str);

        size_t channel = (size_t)  channel_str.toInt();
        
        DEBUG("validating channel number")
        float r = handler.get(channel);
        char buf[64];
        sprintf(buf, "%f", r);
        return buf;
    }
    else
    {
        param_ok = false;
    }
    
    return "Parameter error";
}

#endif // INCLUDE_ZERO2TEN
