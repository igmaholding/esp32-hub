#ifdef INCLUDE_PHASECHANGER
#include <ArduinoJson.h>
#include "driver/ledc.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <phaseChanger.h>
#include <autonom.h>
#include <gpio.h>
#include <binarySemaphore.h>
#include <mapTable.h>
#include <Wire.h>
#include <deque>
#include <epromImage.h>
#include <sstream>

extern GpioHandler gpioHandler;

const float DEFAULT_INPUT_CALIBRATION_COEFFICIENT= 1.04;


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

bool PhaseChangerConfig::is_valid() const
{
    GpioCheckpad checkpad;

    size_t i = 0;

    for (auto it = input_i_low_channels.begin(); it != input_i_low_channels.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
            ERROR("input_i_low_channel %d is_valid() == false", (int) i)
            return false;
        }

        char object_name[64];
        sprintf(object_name, "input_i_low_channel[%d]", (int) i);

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

    for (auto it = applets.begin(); it != applets.end(); ++it, ++i)
    {
        if (it->second.is_valid() == false)
        {
            ERROR("applet %s is_valid() == false", it->first.c_str())
            return false;
        }
        else
        {
        }
    }

    return true;
}


void PhaseChangerConfig::from_json(const JsonVariant &json)
{
    //DEBUG("zero2ten config from_json")
    clear();

    if (json.containsKey("input_v_channels"))
    {
        const JsonVariant &_json = json["input_v_channels"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                InputChannelIna3221 input_channel;
                input_channel.from_json(__json);
                input_v_channels.push_back(input_channel);
                //DEBUG("Channel from_json, %s", input_channel.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("input_i_high_channels"))
    {
        const JsonVariant &_json = json["input_i_high_channels"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                InputChannelIna3221 input_channel;
                input_channel.from_json(__json);
                input_i_high_channels.push_back(input_channel);
                //DEBUG("Channel from_json, %s", input_channel.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("input_i_low_channels"))
    {
        const JsonVariant &_json = json["input_i_low_channels"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                InputChannel input_channel;
                input_channel.from_json(__json);
                input_i_low_channels.push_back(input_channel);
                //DEBUG("Channel from_json, %s", input_channel.as_string().c_str())

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
                    DEBUG("applet from_json: name=%s, %s", name.c_str(), applet.as_string().c_str())
                }
                ++iterator;
            }
        }
    }
}

void PhaseChangerConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));

    uint8_t count = (uint8_t)input_v_channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = input_v_channels.begin(); it != input_v_channels.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)input_i_high_channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = input_i_high_channels.begin(); it != input_i_high_channels.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)input_i_low_channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = input_i_low_channels.begin(); it != input_i_low_channels.end(); ++it)
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

bool PhaseChangerConfig::from_eprom(std::istream &is)
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
            InputChannelIna3221 input_channel;
            input_channel.from_eprom(is);
            input_v_channels.push_back(input_channel);
        }

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            InputChannelIna3221 input_channel;
            input_channel.from_eprom(is);
            input_i_high_channels.push_back(input_channel);
        }

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            InputChannel input_channel;
            input_channel.from_eprom(is);
            input_i_low_channels.push_back(input_channel);
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
        ERROR("Failed to read PhaseChangerConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void PhaseChangerConfig::InputChannelIna3221::from_json(const JsonVariant &json)
{
    clear();    

    if (json.containsKey("addr"))
    {
        addr = (uint8_t)(int) json["addr"];
    }

    if (json.containsKey("channel"))
    {
        addr = (int) json["channel"];
    }

    if (json.containsKey("ratio"))
    {
        ratio = (float) json["ratio"];
    }
}

void PhaseChangerConfig::InputChannelIna3221::to_eprom(std::ostream &os) const
{
    os.write((const char *)&addr, sizeof(addr));
    os.write((const char *)&channel, sizeof(channel));
    os.write((const char *)&ratio, sizeof(ratio));
}

bool PhaseChangerConfig::InputChannelIna3221::from_eprom(std::istream &is)
{
    clear();
    is.read((char *)&addr, sizeof(addr));
    is.read((char *)&channel, sizeof(channel));
    is.read((char *)&ratio, sizeof(ratio));

    return is_valid() && !is.bad();
}

void PhaseChangerConfig::InputChannel::from_json(const JsonVariant &json)
{
    clear();    
    AnalogInputChannelConfig::from_json(json);

    if (json.containsKey("ratio"))
    {
        ratio = (float) json["ratio"];
    }
}

void PhaseChangerConfig::InputChannel::to_eprom(std::ostream &os) const
{
    AnalogInputChannelConfig::to_eprom(os);
    os.write((const char *)&ratio, sizeof(ratio));
}

bool PhaseChangerConfig::InputChannel::from_eprom(std::istream &is)
{
    clear();
    AnalogInputChannelConfig::from_eprom(is);
    is.read((char *)&ratio, sizeof(ratio));

    return is_valid() && !is.bad();
}


void PhaseChangerConfig::Applet::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("function"))
    {
        String function_str = json["function"];
        function = str_2_function(function_str.c_str());
    }
}

void PhaseChangerConfig::Applet::to_eprom(std::ostream &os) const
{
    os.write((const char *)&function, sizeof(function));
}

bool PhaseChangerConfig::Applet::from_eprom(std::istream &is)
{
    clear();

    is.read((char *)&function, sizeof(function));

    return is_valid() && !is.bad();
}


class PhaseChangerHandler
{
public:

    const int DATA_EPROM_VERSION = 1;

    class InputChannelData
    {
    public:

        InputChannelData() 
        {
            clear();
        }

        void clear() 
        {
            _set_not_calibrated(calibration_coefficient); 
        }

        void to_eprom(std::ostream &os) const
        {
            os.write((const char *)&calibration_coefficient, sizeof(calibration_coefficient));
        }

        bool from_eprom(std::istream &is)
        {
            clear();
            is.read((char *)&calibration_coefficient, sizeof(calibration_coefficient));

            return !is.bad();
        }

        float calibration_coefficient;
    };

    class AppletHandler
    {
    public:

        AppletHandler() 
        {
        }

        ~AppletHandler() 
        {
        }

        void init(const PhaseChangerConfig::Applet & _config)
        {   
            config = _config;
        }

        PhaseChangerConfig::Applet config;
    };

    PhaseChangerHandler()
    {
        _data_needs_save = false;

        _is_active = false;
        _is_finished = true;

        // * input gain is what the measured value is multiplied with to get the final result

        const float input_v_gain_function[] =
        {
            1, 1,
            2, 1,
            3, 1
        };

        const float input_i_high_gain_function[] =
        {
            1, 1,
            2, 1,
            3, 1
        };

        const float input_i_low_gain_function[] =
        {
            1, 1,
            2, 1,
            3, 1
        };

        input_v_gain_map_table.init_table(input_v_gain_function, (sizeof(input_v_gain_function)/sizeof(float))/2);
        input_i_high_gain_map_table.init_table(input_i_high_gain_function, (sizeof(input_i_high_gain_function)/sizeof(float))/2);
        input_i_low_gain_map_table.init_table(input_i_low_gain_function, (sizeof(input_i_low_gain_function)/sizeof(float))/2);
    }

    ~PhaseChangerHandler()
    {
        for (auto it=applet_handlers.begin(); it!=applet_handlers.end(); ++it)
        {
            delete it->second;
        }
        applet_handlers.clear();
    }

    bool is_active() const { return _is_active; }

    void start(const PhaseChangerConfig &config);
    void stop();
    void reconfigure(const PhaseChangerConfig &config);

    PhaseChangerStatus get_status()
    {
        PhaseChangerStatus _status;

        Lock lock(semaphore);
        _status = status;

        return _status;
    }

    String calibrate_v(size_t channel, float value);
    String calibrate_i_high(size_t channel, float value);
    String calibrate_i_low(size_t channel, float value);
    String uncalibrate_v(size_t channel);
    String uncalibrate_i_high(size_t channel);
    String uncalibrate_i_low(size_t channel);
    String input_v(size_t channel, float & value, bool no_trace = false);
    String input_i_high(size_t channel, float & value, bool no_trace = false);
    String input_i_low(size_t channel, float & value, bool no_trace = false);

    bool does_data_need_save();
    void data_saved();

    void data_to_eprom(std::ostream & os);
    bool data_from_eprom(std::istream & is);

    bool read_data();
    void save_data();

protected:

    void configure_channels();
    void configure_applets();

    unsigned analog_read(uint8_t gpio);

    static void task(void *parameter);

    BinarySemaphore semaphore;
    PhaseChangerConfig config;
    PhaseChangerStatus status;

    std::vector<InputChannelData> input_v_channel_data;
    std::vector<InputChannelData> input_i_high_channel_data;
    std::vector<InputChannelData> input_i_low_channel_data;
 
    std::map<String, AppletHandler*> applet_handlers;

    bool _data_needs_save;

    bool _is_active;
    bool _is_finished;

    MapTable input_v_gain_map_table;
    MapTable input_i_high_gain_map_table;
    MapTable input_i_low_gain_map_table;
};

static PhaseChangerHandler handler;

void PhaseChangerHandler::start(const PhaseChangerConfig &_config)
{
    TRACE("starting phase-changer handler")
    //Serial.write("DIRECT: starting zero2ten handler");

    if (_is_active)
    {
        ERROR("phase-changer handler already running")
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    config = _config;

    configure_channels();
    configure_applets();

    read_data();


    _is_active = true;
    _is_finished = false;

    TRACE("starting phase-hander handler task")

    xTaskCreate(
        task,                // Function that should be called
        "phase_hander_task", // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void PhaseChangerHandler::stop()
{
    TRACE("stopping phase-hander handler")

    if (_is_active)
    {
    }

    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }
}

void PhaseChangerHandler::reconfigure(const PhaseChangerConfig &_config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("phase-handler task: config changed")

        bool should_configure_channels = (config.input_v_channels == _config.input_v_channels && 
                                          config.input_i_high_channels == _config.input_i_high_channels && 
                                          config.input_i_low_channels == _config.input_i_low_channels) ? false : true;

        bool should_configure_applets = config.applets == _config.applets  ? false : true;

        config = _config;

        if (should_configure_channels)
        {
            configure_channels();
        }
        else
        {
            if (should_configure_applets)
            {
                configure_applets();
            }
        }
    }
}

void PhaseChangerHandler::task(void *parameter)
{
    PhaseChangerHandler *_this = (PhaseChangerHandler *)parameter;

    TRACE("phase-changer task: started")

    const size_t SAVE_DATA_INTERVAL = 10; // seconds
    unsigned long last_save_data_millis = millis();

    const size_t TEMP2OUT_ALGO_INTERVAL = 60; // seconds
    unsigned long last_temp2out_algo_millis = 0;  // will run first time

    while (_this->_is_active)
    {
        // automatically refresh input values in status

        { Lock lock(_this->semaphore);

        for (size_t i=0; i < _this->input_v_channel_data.size(); ++i)
        {
            float dummy = 0;
            _this->input_v(i, dummy, true);
        }

        for (size_t i=0; i < _this->input_i_high_channel_data.size(); ++i)
        {
            float dummy = 0;
            _this->input_i_high(i, dummy, true);
        }

        for (size_t i=0; i < _this->input_i_low_channel_data.size(); ++i)
        {
            float dummy = 0;
            _this->input_i_low(i, dummy, true);
        }}

        unsigned long now_millis = millis();

        if (now_millis < last_temp2out_algo_millis || 
            (now_millis-last_temp2out_algo_millis)/1000 >= TEMP2OUT_ALGO_INTERVAL)
        {
            last_temp2out_algo_millis = now_millis;

            for (auto it=_this->config.applets.begin(); it!=_this->config.applets.end();++it)
            {
            }
        }

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

    TRACE("phase-changer task: terminated")
    vTaskDelete(NULL);
}


static String esp_err_2_string(esp_err_t esp_r)
{
    char buf[128];
    esp_err_to_name_r(esp_r, buf, sizeof(buf));
    return String("ERR=") + String((int) esp_r) + ", " + buf;
}


void PhaseChangerHandler::configure_channels()
{
    TRACE("configure channels")
    Lock lock(semaphore);

    PhaseChangerStatus::Channel input_v_channel_p(PhaseChangerStatus::Channel::tInputV);
    status.input_v_channels.resize(config.input_v_channels.size(), input_v_channel_p);
    PhaseChangerStatus::Channel input_i_high_channel_p(PhaseChangerStatus::Channel::tInputIHigh);
    status.input_i_high_channels.resize(config.input_i_high_channels.size(), input_i_high_channel_p);
    PhaseChangerStatus::Channel input_i_low_channel_p(PhaseChangerStatus::Channel::tInputILow);
    status.input_i_low_channels.resize(config.input_i_low_channels.size(), input_i_low_channel_p);
    
    // TODO: configure INA3221

    input_v_channel_data.resize(config.input_v_channels.size());

    size_t i=0;

    for (auto it=config.input_v_channels.begin(); it!=config.input_v_channels.end(); ++it, ++i)
    {
        TRACE("configure input_v_channels[%d]", (int)i)
        // TODO
    }

    input_i_high_channel_data.resize(config.input_i_high_channels.size());

    i=0;

    for (auto it=config.input_i_high_channels.begin(); it!=config.input_i_high_channels.end(); ++it, ++i)
    {
        TRACE("configure input_i_high_channels[%d]", (int)i)
        // TODO
    }
    
    input_i_low_channel_data.resize(config.input_i_low_channels.size());

    i=0;

    for (auto it=config.input_i_low_channels.begin(); it!=config.input_i_low_channels.end(); ++it, ++i)
    {
        TRACE("configure input_i_low_channels[%d] gpio=%d, atten=%d", (int)i, (int)it->gpio, (int)it->atten)
        analogSetPinAttenuation(it->gpio, (adc_attenuation_t)it->atten);
    }

    _data_needs_save = true;
}

void PhaseChangerHandler::configure_applets()
{    
    TRACE("configure_applets")
    Lock lock(semaphore);
    
    for (auto it=applet_handlers.begin(); it!=applet_handlers.end(); ++it)
    {
        delete it->second;
    }
    applet_handlers.clear();

    status.applets.clear();

    for (auto it=config.applets.begin(); it!=config.applets.end(); ++it)
    {
        TRACE("configure applet %s", it->first.c_str())
        AppletHandler * _applet_handler = new AppletHandler();
        _applet_handler->init(it->second);
        applet_handlers.insert(std::make_pair(it->first, _applet_handler));

        PhaseChangerStatus::Applet _applet_status;
        _applet_status.function = it->second.function_2_str(it->second.function);
        status.applets.insert(std::make_pair(it->first, _applet_status));
    }
}

unsigned PhaseChangerHandler::analog_read(uint8_t gpio)
{
    adcAttachPin(gpio);
    return analogRead(gpio);
}

String PhaseChangerHandler::calibrate_v(size_t channel, float value)
{
    TRACE("calibrating input V channel %d, value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);

    if (channel < input_v_channel_data.size())
    {
    }
    else
    {
        r = String("input V channel number ") + String((int) channel) + " is invalid";
    }

    if (r.length())
    {
        ERROR(r.c_str())
    }

    return r;
}

String PhaseChangerHandler::calibrate_i_high(size_t channel, float value)
{
    TRACE("calibrating input I High channel %d, value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);

    if (channel < input_i_high_channel_data.size())
    {
    }
    else
    {
        r = String("input I High channel number ") + String((int) channel) + " is invalid";
    }

    if (r.length())
    {
        ERROR(r.c_str())
    }

    return r;
}

String PhaseChangerHandler::calibrate_i_low(size_t channel, float value)
{
    TRACE("calibrating input I Low channel %d, value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);

    if (channel < input_i_low_channel_data.size())
    {
    }
    else
    {
        r = String("input I Low channel number ") + String((int) channel) + " is invalid";
    }

    if (r.length())
    {
        ERROR(r.c_str())
    }

    return r;
}

String PhaseChangerHandler::uncalibrate_v(size_t channel)
{
    TRACE("uncalibrating input V channel %d", (int) channel)
    String r;

    Lock lock(semaphore);

    if (channel < input_v_channel_data.size())
    {
        if (_is_calibrated(input_v_channel_data[channel].calibration_coefficient))
        {
        }
    }
    else
    {
        r = String("input V channel number ") + String((int) channel) + " is invalid";
        ERROR(r.c_str())
    }

    return r;
}

String PhaseChangerHandler::uncalibrate_i_high(size_t channel)
{
    TRACE("uncalibrating input I High channel %d", (int) channel)
    String r;

    Lock lock(semaphore);

    if (channel < input_i_high_channel_data.size())
    {
        if (_is_calibrated(input_i_high_channel_data[channel].calibration_coefficient))
        {
        }
    }
    else
    {
        r = String("input I High channel number ") + String((int) channel) + " is invalid";
        ERROR(r.c_str())
    }

    return r;
}

String PhaseChangerHandler::uncalibrate_i_low(size_t channel)
{
    TRACE("uncalibrating input I Low channel %d", (int) channel)
    String r;

    Lock lock(semaphore);

    if (channel < input_i_low_channel_data.size())
    {
        if (_is_calibrated(input_i_low_channel_data[channel].calibration_coefficient))
        {
        }
    }
    else
    {
        r = String("input I Low channel number ") + String((int) channel) + " is invalid";
        ERROR(r.c_str())
    }

    return r;
}


String PhaseChangerHandler::input_v(size_t channel, float & value, bool no_trace)
{
    if (no_trace == false)
    {
        TRACE("getting input V of channel %d", (int) channel)
    }

    String r;

    Lock lock(semaphore);

    if (channel < input_v_channel_data.size())
    {
        value = 0;
        status.input_v_channels[channel].value = value;
        // DEBUG("adjusted by calibration value v %f", value)
    }
    else
    {
        r = String("input V channel number ") + String((int) channel) + " is invalid";
    }

    return r;
}

String PhaseChangerHandler::input_i_high(size_t channel, float & value, bool no_trace)
{
    if (no_trace == false)
    {
        TRACE("getting input I High of channel %d", (int) channel)
    }

    String r;

    Lock lock(semaphore);

    if (channel < input_i_high_channel_data.size())
    {
        value = 0;
        status.input_i_high_channels[channel].value = value;
        // DEBUG("adjusted by calibration value v %f", value)
    }
    else
    {
        r = String("input I High channel number ") + String((int) channel) + " is invalid";
    }

    return r;
}

String PhaseChangerHandler::input_i_low(size_t channel, float & value, bool no_trace)
{
    if (no_trace == false)
    {
        TRACE("getting input I Low of channel %d", (int) channel)
    }

    String r;

    Lock lock(semaphore);

    if (channel < input_i_low_channel_data.size())
    {
        value = 0;
        status.input_i_low_channels[channel].value = value;
        // DEBUG("adjusted by calibration value v %f", value)
    }
    else
    {
        r = String("input I Low channel number ") + String((int) channel) + " is invalid";
    }

    return r;
}

bool PhaseChangerHandler::does_data_need_save() 
{
    return _data_needs_save;
}

void PhaseChangerHandler::data_saved() 
{
    _data_needs_save = false;
}

void PhaseChangerHandler::data_to_eprom(std::ostream &os) 
{
    Lock lock(semaphore);

    DEBUG("PhaseChangerHandler data_to_eprom")

    uint8_t eprom_version = (uint8_t)DATA_EPROM_VERSION;
    os.write((const char *)&eprom_version, sizeof(eprom_version));

    uint8_t count = input_v_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=input_v_channel_data.begin(); it!=input_v_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }
    
    count = input_i_high_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=input_i_high_channel_data.begin(); it!=input_i_high_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = input_i_low_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=input_i_low_channel_data.begin(); it!=input_i_low_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }

}

bool PhaseChangerHandler::data_from_eprom(std::istream &is)
{
    uint8_t eprom_version = DATA_EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    DEBUG("PhaseChangerHandler data_from_eprom")

    if (eprom_version == DATA_EPROM_VERSION)
    {
        DEBUG("Version match")

        Lock lock(semaphore);

        std::vector<InputChannelData> new_input_v_channel_data;

        uint8_t count = 0;
        is.read((char *)&count, sizeof(count));

        if (is.bad())
        {
            ERROR("error reading input V channel data count from eprom. corrupt?")
            return false;
        }

        for (size_t i=0; i<count; ++i)
        {
            InputChannelData input_channel_data_item;
            
            if (input_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading input V channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_input_v_channel_data.push_back(input_channel_data_item);
        }

        for (size_t i=0; i<config.input_v_channels.size(); ++i)
        {
            if (i < new_input_v_channel_data.size())
            {
                input_v_channel_data[i] = new_input_v_channel_data[i];
            }
            else
            {
                input_v_channel_data.push_back(InputChannelData());
            }
            
            status.input_v_channels[i].calibration_coefficient =  input_v_channel_data[i].calibration_coefficient; 
        }

        std::vector<InputChannelData> new_input_i_high_channel_data;

        count = 0;
        is.read((char *)&count, sizeof(count));

        if (is.bad())
        {
            ERROR("error reading input I High channel data count from eprom. corrupt?")
            return false;
        }

        for (size_t i=0; i<count; ++i)
        {
            InputChannelData input_channel_data_item;
            
            if (input_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading input I High channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_input_i_high_channel_data.push_back(input_channel_data_item);
        }

        for (size_t i=0; i<config.input_i_high_channels.size(); ++i)
        {
            if (i < new_input_i_high_channel_data.size())
            {
                input_i_high_channel_data[i] = new_input_i_high_channel_data[i];
            }
            else
            {
                input_i_high_channel_data.push_back(InputChannelData());
            }
            
            status.input_i_high_channels[i].calibration_coefficient =  input_i_high_channel_data[i].calibration_coefficient; 
        }

        std::vector<InputChannelData> new_input_i_low_channel_data;

        count = 0;
        is.read((char *)&count, sizeof(count));

        if (is.bad())
        {
            ERROR("error reading input I Low channel data count from eprom. corrupt?")
            return false;
        }

        for (size_t i=0; i<count; ++i)
        {
            InputChannelData input_channel_data_item;
            
            if (input_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading input I Low channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_input_i_low_channel_data.push_back(input_channel_data_item);
        }

        for (size_t i=0; i<config.input_i_low_channels.size(); ++i)
        {
            if (i < new_input_i_low_channel_data.size())
            {
                input_i_low_channel_data[i] = new_input_i_low_channel_data[i];
            }
            else
            {
                input_i_low_channel_data.push_back(InputChannelData());
            }
            
            status.input_i_low_channels[i].calibration_coefficient =  input_i_low_channel_data[i].calibration_coefficient; 
        }

        _data_needs_save = false;  // output(..) sets this to true, but we just read the values from eprom 
    }
    else
    {
        ERROR("Failed to read phase-changer data from EPROM: version mismatch, expected %d, found %d", (int)DATA_EPROM_VERSION, (int)eprom_version)
        return false;
    }

    return !is.bad();
}


bool PhaseChangerHandler::read_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);

    TRACE("PhaseChangerHandler reading data from EEPROM")

    if (dataVolume.read() == true)
    {
        for (auto it = dataVolume.blocks.begin(); it != dataVolume.blocks.end(); ++it)
        {
            if(it->first == ftPhaseChanger)
            {
                const char * function_type_str = function_type_2_str((FunctionType) it->first);
                TRACE("Found block type for function %s", function_type_str)

                std::istringstream is(it->second);

                if (data_from_eprom(is) == true)
                {
                    TRACE("PhaseChanger data read success")
                    return true;
                }
                else
                {
                    TRACE("PhaseChanger data read failure")
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

void PhaseChangerHandler::save_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);
    dataVolume.read();

    std::ostringstream os;

    TRACE("Saving zero2ten data to EEPROM")
    data_to_eprom(os);

    std::string buffer = os.str();
    TRACE("block size %d", (int) os.tellp())
    
    if (dataVolume.blocks.find((uint8_t) ftPhaseChanger) == dataVolume.blocks.end())
    {
        dataVolume.blocks.insert({(uint8_t) ftPhaseChanger, buffer});
    }
    else
    {
        if (dataVolume.blocks[(uint8_t) ftPhaseChanger] == buffer)
        {
            TRACE("Data identical, skip saving")
            return;
        }
        else
        {
            dataVolume.blocks[(uint8_t) ftPhaseChanger] = buffer;
        }
    }
    
    if (dataVolume.write())
    {
        TRACE("PhaseChanger data save success")
    }
    else
    {
        TRACE("PhaseChanger data save failure")
    }
}

void start_phase_changer_task(const PhaseChangerConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start phase-changer task while it is running, redirecting to reconfigure")
        reconfigure_phase_changer(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_phase_changer_task()
{
    handler.stop();
}

PhaseChangerStatus get_phase_changer_status()
{
    return handler.get_status();
}

void reconfigure_phase_changer(const PhaseChangerConfig &_config)
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

String phase_changer_calibrate_v(const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();

            if (!value_str.isEmpty())
            {
                if (__is_number_or_empty(value_str))
                {
                    float value = (float)  value_str.toFloat();
                    return handler.calibrate_v(channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_v(channel);
            }
        }
    }
    
    return "Parameter error";
}

String phase_changer_calibrate_i_high(const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();

            if (!value_str.isEmpty())
            {
                if (__is_number_or_empty(value_str))
                {
                    float value = (float)  value_str.toFloat();
                    return handler.calibrate_i_high(channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_i_high(channel);
            }
        }
    }
    
    return "Parameter error";
}

String phase_changer_calibrate_i_low(const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();

            if (!value_str.isEmpty())
            {
                if (__is_number_or_empty(value_str))
                {
                    float value = (float)  value_str.toFloat();
                    return handler.calibrate_i_low(channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_i_low(channel);
            }
        }
    }
    
    return "Parameter error";
}

String phase_changer_input_v(const String & channel_str, String & value_str)
{
    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = -1;
        
            String r = handler.input_v(channel, value);

            if (r.isEmpty() == true)
            {
                char buf[64];
                sprintf(buf, "%.2f", value);
                value_str = buf;
            }
            
            return r;
        }
    }
    
    return "Parameter error";
}

String phase_changer_input_i_high(const String & channel_str, String & value_str)
{
    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = -1;
        
            String r = handler.input_i_high(channel, value);

            if (r.isEmpty() == true)
            {
                char buf[64];
                sprintf(buf, "%.2f", value);
                value_str = buf;
            }
            
            return r;
        }
    }
    
    return "Parameter error";
}

String phase_changer_input_i_low(const String & channel_str, String & value_str)
{
    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = -1;
        
            String r = handler.input_i_low(channel, value);

            if (r.isEmpty() == true)
            {
                char buf[64];
                sprintf(buf, "%.2f", value);
                value_str = buf;
            }
            
            return r;
        }
    }
    
    return "Parameter error";
}


#endif // INCLUDE_PHASECHANGER
