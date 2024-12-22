#ifdef INCLUDE_ZERO2TEN
#include <ArduinoJson.h>
#include "driver/ledc.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <zero2ten.h>
#include <autonom.h>
#include <gpio.h>
#include <binarySemaphore.h>
#include <mapTable.h>
#include <Wire.h>
#include <deque>
#include <epromImage.h>
#include <sstream>

extern GpioHandler gpioHandler;

const float DEFAULT_OUTPUT_CALIBRATION_COEFFICIENT= 0.88;
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
            if (it->second.temp.is_valid())
            {
                char object_name[64];
                sprintf(object_name, "applet[%s].temp.channel.gpio", it->first.c_str());

                if (checkpad.get_usage(it->second.temp.channel.gpio) != GpioCheckpad::uNone)
                {
                    _err_dup(object_name, (int)it->second.temp.channel.gpio);
                    return false;
                }

                if (!checkpad.set_usage(it->second.temp.channel.gpio, GpioCheckpad::uDigitalAll))
                {
                    _err_cap(object_name, (int)it->second.temp.channel.gpio);
                    return false;
                }
            }

            if (it->second.function == Applet::fTemp2out)
            {
                if (it->second.output_channel < 0 || it->second.output_channel >= output_channels.size())
                {
                    ERROR("applet %s is_valid() == false, output_channel index is invalid", it->first.c_str())
                    return false;
                }
            }
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
                    DEBUG("applet from_json: name=%s, %s", name.c_str(), applet.as_string().c_str())
                    applet.map_table.debug_print();
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

    if (json.containsKey("ratio"))
    {
        ratio = (float) json["ratio"];
    }
}

void Zero2tenConfig::InputChannel::to_eprom(std::ostream &os) const
{
    AnalogInputChannelConfig::to_eprom(os);
    os.write((const char *)&ratio, sizeof(ratio));
}

bool Zero2tenConfig::InputChannel::from_eprom(std::istream &is)
{
    clear();
    AnalogInputChannelConfig::from_eprom(is);
    is.read((char *)&ratio, sizeof(ratio));

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

    if (json.containsKey("max_voltage"))
    {
        max_voltage = (float) json["max_voltage"];
    }

    if (json.containsKey("loopback"))
    {
        const JsonVariant &_json = json["loopback"];
        loopback.from_json(_json);
    }
}

void Zero2tenConfig::OutputChannel::to_eprom(std::ostream &os) const
{
    output.to_eprom(os);
    os.write((const char *)&max_voltage, sizeof(max_voltage));
    loopback.to_eprom(os);
}

bool Zero2tenConfig::OutputChannel::from_eprom(std::istream &is)
{
    clear();

    output.from_eprom(is);
    is.read((char *)&max_voltage, sizeof(max_voltage));
    loopback.from_eprom(is);

    return is_valid() && !is.bad();
}

void Zero2tenConfig::Applet::Temp::from_json(const JsonVariant &json)
{
    if (json.containsKey("channel"))
    {
        const JsonVariant &_json = json["channel"];
        channel.from_json(_json);
    }

    if (json.containsKey("addr"))
    {
        addr = (const char *)json["addr"];
    }

    if (json.containsKey("corr"))
    {
        corr = json["corr"];
    }
}

void Zero2tenConfig::Applet::Temp::to_eprom(std::ostream &os) const
{
    channel.to_eprom(os);

    uint8_t len = addr.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(addr.c_str(), len);
    }

    os.write((const char *)&corr, sizeof(corr));
}

bool Zero2tenConfig::Applet::Temp::from_eprom(std::istream &is)
{
    channel.from_eprom(is);
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        addr = buf;
    }
    else
    {
        addr = "";
    }

    is.read((char *)&corr, sizeof(corr));

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
        input_channel = (uint8_t)(int) json["input_channel"];
    }

    if (json.containsKey("output_channel"))
    {
        output_channel = (uint8_t)(int)json["output_channel"];
    }

    if (json.containsKey("temp"))
    {
        const JsonVariant &_json = json["temp"];
        temp.from_json(_json);
    }

    if (json.containsKey("map_table"))
    {
        const JsonVariant &_json = json["map_table"];
        map_table.from_json(_json);
    }
}

void Zero2tenConfig::Applet::to_eprom(std::ostream &os) const
{
    os.write((const char *)&function, sizeof(function));
    os.write((const char *)&input_channel, sizeof(input_channel));
    os.write((const char *)&output_channel, sizeof(output_channel));
    temp.to_eprom(os);
    map_table.to_eprom(os);
}

bool Zero2tenConfig::Applet::from_eprom(std::istream &is)
{
    clear();

    is.read((char *)&function, sizeof(function));
    is.read((char *)&input_channel, sizeof(input_channel));
    is.read((char *)&output_channel, sizeof(output_channel));
    temp.from_eprom(is);
    map_table.from_eprom(is);

    return is_valid() && !is.bad();
}


class Zero2tenHandler
{
public:

    const int DATA_EPROM_VERSION = 1;

    const ledc_timer_bit_t PWM_RESOLUTION = LEDC_TIMER_14_BIT;
    const uint32_t PWM_FREQ = 4000;
    const ledc_mode_t PWM_SPEED_MODE = LEDC_LOW_SPEED_MODE;
    const uint32_t PWM_HPOINT = 0;

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

    class OutputChannelData
    {
    public:

        OutputChannelData() 
        {
            clear();
        }

        void clear() 
        {
            value = 0;
            _set_not_calibrated(calibration_coefficient);
            loopback.clear();
        }

        void to_eprom(std::ostream &os) const
        {
            os.write((const char *)&value, sizeof(value));
            os.write((const char *)&calibration_coefficient, sizeof(calibration_coefficient));
            loopback.to_eprom(os);
        }

        bool from_eprom(std::istream &is)
        {
            clear();
            is.read((char *)&value, sizeof(value));
            is.read((char *)&calibration_coefficient, sizeof(calibration_coefficient));
            bool r = loopback.from_eprom(is);
            return r == true && !is.bad();
        }

        float value;
        float calibration_coefficient;  // relative to gain 1.0
        InputChannelData loopback;
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

        void init(const Zero2tenConfig::Applet & _config)
        {   
            config = _config;

            if (config.function == Zero2tenConfig::Applet::fTemp2out)
            {
                if (config.temp.is_valid())
                {
                    ow.begin(config.temp.channel.gpio);
                    ds18b20.setOneWire(&ow);
                    ds18b20.setWaitForConversion(true);
                    ds18b20.setCheckForConversion(true);
                }
            }
        }

        bool algo_fTemp2out(Zero2tenStatus::Applet & status);

        OneWire ow;
        DallasTemperature ds18b20;
        Zero2tenConfig::Applet config;
    };

    Zero2tenHandler()
    {
        _data_needs_save = false;

        _is_active = false;
        _is_finished = true;

        // * output gain is what the duty cycle is multiplied with

        const float output_gain_function[] =   // by true rmc multimeter
        {
            1, 1,
            2, 1,
            3, 1,
            4, 1,
            5, 1,
            6, 1,
            7, 1.002,
            8, 1.002,
            9, 1.003,
            10, 1.005
        };

        // * input gain is what the measured value is multiplied with to get the final result

        const float input_gain_function[] =
        {
            1, 1,
            2, 1,
            3, 1,
            4, 1,
            5, 1,
            6, 1,
            7, 1,
            8, 1,
            9, .995,
            10, .995
        };

        /*    1, 0.87,  by oscilloscope?
            2, 0.95,
            3, 0.978,
            4, 0.988,
            5, 1.0,
            6, 1.005,
            7, 1.013,
            8, 1.015,
            9, 1.018,
            10, 1.020
        */

        /* 0.85     0.85  input meas
        1.9      0.95
        2.935    0.978
        3.95     0.988
        5        1
        6.03     1.005
        7.1      1.014
        8.133    1.017
        9.175    1.019
        10.21    1.021
        */


        /*  output meas
            1, 0.81609195,
            2, 0.93103448,
            3, 0.96934866,
            4, 0.97701149,
            5, 1.0,
            6, 1.00574713,
            7, 1.008,
            8, 1.01293103,
            9, 1.01787994,
            10, 1.01954023
        */

        output_gain_map_table.init_table(output_gain_function, (sizeof(output_gain_function)/sizeof(float))/2);
        input_gain_map_table.init_table(input_gain_function, (sizeof(input_gain_function)/sizeof(float))/2);
    }

    ~Zero2tenHandler()
    {
        for (auto it=applet_handlers.begin(); it!=applet_handlers.end(); ++it)
        {
            delete it->second;
        }
        applet_handlers.clear();
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

    String calibrate_input(size_t channel, float value);
    String uncalibrate_input(size_t channel);
    String input(size_t channel, float & value, bool no_trace = false);
    String calibrate_output(size_t channel, float value);
    String uncalibrate_output(size_t channel);
    String output(size_t channel, float value);

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
    Zero2tenConfig config;
    Zero2tenStatus status;

    std::vector<InputChannelData> input_channel_data;
    std::vector<OutputChannelData> output_channel_data;
 
    std::map<String, AppletHandler*> applet_handlers;

    bool _data_needs_save;

    bool _is_active;
    bool _is_finished;

    MapTable input_gain_map_table;
    MapTable output_gain_map_table;
};

static Zero2tenHandler handler;


bool Zero2tenHandler::AppletHandler::algo_fTemp2out(Zero2tenStatus::Applet & status)
{
    status.output_value = 0;

    time_t _time_t;
    time(& _time_t);
    tm _tm = {0};
    _tm = *localtime(&_time_t);

    if (_tm.tm_year+1900 > 2000) // otherwise - NTP failed to fetch time 
    {
        char buf[64];
        sprintf(buf, "%d-%02d-%02d %02d:%02d.%02d", _tm.tm_year+1900,  _tm.tm_mon+1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
        status.time = buf;
    }
    else
    {
        status.time.clear();
    }

    if (config.temp.channel.is_valid())
    {
        size_t device_count = 0;

        unsigned attempts = 10;

        while (device_count == 0)
        {
            ds18b20.begin();
            device_count = ds18b20.getDeviceCount();

            if (attempts)
            {
                attempts--;
            }
            else
            {
                //DEBUG("No one wire devices are found after 10 attempts")
                break;
            }
        }

        ds18b20.requestTemperatures();

        DEBUG("DS18b20 device count %d", (int)device_count)

        float r_temp = 0;

        for (size_t i = 0; i < device_count; ++i)
        {
            uint8_t addr[8];

            if (ds18b20.getAddress(addr, i))
            {
                char addr_str[32];

                sprintf(addr_str, "%02x-%02x%02x%02x%02x%02x%02x", (int)addr[0], (int)addr[6], (int)addr[5], (int)addr[4], (int)addr[3], (int)addr[2], (int)addr[1]);
                float i_temp = ds18b20.getTempC(addr);

                
                DEBUG("addr=[%s], temp=%f", addr_str, i_temp)
                status.temp_addr = addr_str;

                if ((config.temp.addr.isEmpty() && device_count == 1) || !strcmp(addr_str, config.temp.addr.c_str()))
                {
                    r_temp = round(i_temp * 10.0) / 10.0;

                    if (config.temp.corr != 0)
                    {
                        //DEBUG("applying non-zero correction %f", config.temp.corr)
                        r_temp += config.temp.corr;
                    }

                    status.temp = r_temp;
                    /*
                    if (!strcmp(addr_str, config.temp.addr.c_str()))
                    {
                        DEBUG("this is configured device, r_temp=%f", r_temp)
                    }
                    else
                    {
                        DEBUG("this is the only device and no addr in config, r_temp=%f", r_temp)
                    }                    
                    */

                    if ((int)r_temp == -127)
                    {
                        status.status = "reading temp failed with N/A value, will set output_value to 0";
                        ERROR(status.status.c_str())
                    }
                    else
                    {
                        status.output_value = config.map_table.x_to_y(r_temp);

                        if (status.output_value < 0)
                        {
                            status.output_value = 0;
                        }
                        
                        status.status.clear();
                        DEBUG("map_table.x_to_y returns %.2f based on x/temp=%f", status.output_value, r_temp)
                        return true;
                    }
                }
            }
        }

        status.status = "no temp sensor found or reading invalid, will set output_value to 0";
        ERROR(status.status.c_str())
        status.temp_addr.clear();
        status.temp = 0;
    }
    else
    {
        status.status = "temp config invalid, will set output_value to 0";
        ERROR(status.status.c_str())
    }

    return false;
}

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
    configure_applets();

    read_data();


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

void Zero2tenHandler::task(void *parameter)
{
    Zero2tenHandler *_this = (Zero2tenHandler *)parameter;

    TRACE("zero2ten_task: started")

    const size_t SAVE_DATA_INTERVAL = 10; // seconds
    unsigned long last_save_data_millis = millis();

    const size_t TEMP2OUT_ALGO_INTERVAL = 60; // seconds
    unsigned long last_temp2out_algo_millis = 0;  // will run first time

    while (_this->_is_active)
    {
        // automatically refresh input values in status

        { Lock lock(_this->semaphore);

        for (size_t i=0; i < _this->input_channel_data.size(); ++i)
        {
            float dummy = 0;
            _this->input(i, dummy, true);
        }}


        unsigned long now_millis = millis();

        if (now_millis < last_temp2out_algo_millis || 
            (now_millis-last_temp2out_algo_millis)/1000 >= TEMP2OUT_ALGO_INTERVAL)
        {
            last_temp2out_algo_millis = now_millis;

            for (auto it=_this->config.applets.begin(); it!=_this->config.applets.end();++it)
            {
                if (it->second.function == Zero2tenConfig::Applet::fTemp2out)
                {
                    TRACE("Calling algo_fTemp2out on applet %s", it->first.c_str())

                    size_t output_channel_index = it->second.output_channel;
                    Zero2tenStatus::Applet & _applet_status = _this->status.applets[it->first]; 
                    _this->applet_handlers[it->first]->algo_fTemp2out(_applet_status);

                    //TRACE("setting output_value on channel %d to %.2f", output_channel_index, output_value)

                    _this->output(output_channel_index, _applet_status.output_value);
                    _this->_data_needs_save = false; // do not save values set by algo
                }
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

    TRACE("zero2ten_task: terminated")
    vTaskDelete(NULL);
}


static String esp_err_2_string(esp_err_t esp_r)
{
    char buf[128];
    esp_err_to_name_r(esp_r, buf, sizeof(buf));
    return String("ERR=") + String((int) esp_r) + ", " + buf;
}


void Zero2tenHandler::configure_channels()
{
    TRACE("configure channels")
    Lock lock(semaphore);

    //status.input_channels.clear();
    Zero2tenStatus::Channel input_channel_p(Zero2tenStatus::Channel::tInput);
    status.input_channels.resize(config.input_channels.size(), input_channel_p);
    
    input_channel_data.resize(config.input_channels.size());

    size_t i=0;

    for (auto it=config.input_channels.begin(); it!=config.input_channels.end(); ++it, ++i)
    {
        TRACE("configure input_channels[%d] gpio=%d, atten=%d", (int)i, (int)it->gpio, (int)it->atten)
        analogSetPinAttenuation(it->gpio, (adc_attenuation_t)it->atten);
    }

    ledc_timer_config_t _ledc_timer_config;
    memset(& _ledc_timer_config, 0, sizeof(_ledc_timer_config));
    _ledc_timer_config.speed_mode = PWM_SPEED_MODE;
    _ledc_timer_config.duty_resolution = PWM_RESOLUTION;
    _ledc_timer_config.timer_num = LEDC_TIMER_0;
    _ledc_timer_config.freq_hz = PWM_FREQ;
    _ledc_timer_config.clk_cfg = LEDC_AUTO_CLK;

    esp_err_t esp_r = ledc_timer_config(& _ledc_timer_config);

    if (esp_r != ESP_OK)
    {
        status.status = String("failed to configure ledc timer, ") + esp_err_2_string(esp_r); 
        ERROR(status.status.c_str())
        // but continue anyway
    }
    else
    {
        TRACE("configure ledc timer OK")
    }

    //status.output_channels.clear();
    Zero2tenStatus::Channel output_channel_p(Zero2tenStatus::Channel::tOutput);
    status.output_channels.resize(config.output_channels.size(), output_channel_p);

    output_channel_data.resize(config.output_channels.size());

    i=0;

    for (auto it=config.output_channels.begin(); it!=config.output_channels.end(); ++it, ++i)
    {
        TRACE("configure output_channels[%d].output gpio=%d", (int)i, (int)it->output.gpio)

        ledc_channel_config_t _ledc_channel_config;
        memset(& _ledc_channel_config, 0, sizeof(_ledc_channel_config));

        ledc_channel_t _channel = ledc_channel_t(LEDC_CHANNEL_0 + i);

        if (_channel >= LEDC_CHANNEL_MAX)
        {
            ledc_channel_t _last_channel = ledc_channel_t(LEDC_CHANNEL_MAX - 1);
            char buf[128];
            sprintf(buf, "ledc channel number (%d) exceeds the limit (should be < %d), will use last valid channel number (this and the previous channel may not work correctly)", 
                    (int)_channel, (int)LEDC_CHANNEL_MAX, (int) _last_channel); 
            status.output_channels[i].status = buf; 
            ERROR(buf)

            _channel = _last_channel;
        }

        _ledc_channel_config.gpio_num = it->output.gpio;
        _ledc_channel_config.speed_mode = PWM_SPEED_MODE;
        _ledc_channel_config.channel = _channel;
        _ledc_channel_config.intr_type = LEDC_INTR_DISABLE;
        _ledc_channel_config.timer_sel = LEDC_TIMER_0;
        _ledc_channel_config.duty = 0;
        _ledc_channel_config.hpoint = PWM_HPOINT; // phase
        _ledc_channel_config.flags.output_invert = 0;

        esp_err_t esp_r = ledc_channel_config(& _ledc_channel_config);

        if (esp_r != ESP_OK)
        {
            status.output_channels[i].status = String("failed to configure ledc channel ") + String((int) i) + ", " + esp_err_2_string(esp_r); 
            ERROR(status.output_channels[i].status.c_str())
            // but continue anyway
        }

        TRACE("configure output_channels[%d].loopback gpio=%d, atten=%d", (int)i, (int)it->loopback.gpio, (int)it->loopback.atten)
        analogSetPinAttenuation(it->loopback.gpio, (adc_attenuation_t)it->loopback.atten);

        DEBUG("re-outputting value")
        output(i, output_channel_data[i].value);
    }

    _data_needs_save = true;
}

void Zero2tenHandler::configure_applets()
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

        Zero2tenStatus::Applet _applet_status;
        _applet_status.function = it->second.function_2_str(it->second.function);
        status.applets.insert(std::make_pair(it->first, _applet_status));
    }

    
}

unsigned Zero2tenHandler::analog_read(uint8_t gpio)
{
    adcAttachPin(gpio);
    return analogRead(gpio);
}

String Zero2tenHandler::calibrate_input(size_t channel, float value)
{
    TRACE("calibrating input channel %d, value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);

    // calibration ought to be done by applying voltage in the range 0.1 max_voltage ... 1.0 max_voltage and
    // telling with the help of calibration what exact value is it; the max_voltage is implied, it is not configured
    // for an input channel

    if (channel < input_channel_data.size())
    {
        if (_is_calibrated(input_channel_data[channel].calibration_coefficient) == false)
        {
            if (value > 0)
            {
                float _value = 0;
                r = input(channel, _value, true);

                if (r.isEmpty())
                {
                    if (_value > 0)
                    {
                        float calibration_coefficient = value / _value;
                        float gain = 1;
                        
                        try
                        {
                            gain = input_gain_map_table.x_to_y(value); 
                        }
                        catch(const char * str)
                        {
                            ERROR("output gain function is invalid, assuming gain=1")
                        }

                        calibration_coefficient = calibration_coefficient / gain;
                        input_channel_data[channel].calibration_coefficient = calibration_coefficient;

                        status.input_channels[channel].calibration_coefficient = input_channel_data[channel].calibration_coefficient;  // always make sure it is synced 
                        _data_needs_save = true;

                        TRACE("calculated calibration coefficient for input channel %d is %f", (int) channel, calibration_coefficient)

                        status.input_channels[channel].value = value;
                    }
                    else
                    {
                        r = String("read value should be > 0 but it is not");
                    }
                }
            }
            else
            {
                r = String("calibration value should be > 0");
            }
        }
        else
        {
            r = String("calibration cannot be done, channel already calibrated; remove calibration and try again (run calibrate without value)");
        }
    }
    else
    {
        r = String("input channel number ") + String((int) channel) + " is invalid";
    }

    if (r.length())
    {
        ERROR(r.c_str())
    }

    return r;
}

String Zero2tenHandler::uncalibrate_input(size_t channel)
{
    TRACE("uncalibrating input channel %d", (int) channel)
    String r;

    Lock lock(semaphore);

    if (channel < input_channel_data.size())
    {
        if (_is_calibrated(input_channel_data[channel].calibration_coefficient))
        {
            _set_not_calibrated(input_channel_data[channel].calibration_coefficient);
            _data_needs_save = true;

            TRACE("calibration coefficient for input channel %d is reset", (int) channel)

            float dummy = 0;
            input(channel, dummy, true);  
        }
    }
    else
    {
        r = String("input channel number ") + String((int) channel) + " is invalid";
        ERROR(r.c_str())
    }

    return r;
}

String Zero2tenHandler::input(size_t channel, float & value, bool no_trace)
{
    if (no_trace == false)
    {
        TRACE("getting input of channel %d", (int) channel)
    }

    String r;

    Lock lock(semaphore);

    if (channel < input_channel_data.size())
    {
        float ratio = config.input_channels[channel].ratio;
        uint8_t gpio = config.input_channels[channel].gpio;
        uint8_t atten = config.input_channels[channel].atten;

        // we have to take several samples preferably during the half of a pulsation sine (we assume some pulsation)
        // there are two problems with this: we don't know the frequency in and we don't know how long time reading takes
        // thus just read several samples in a row and average

        const uint8_t SAMPLE_COUNT=10;

        unsigned long sample_average = 0;

        for (size_t i=0; i<SAMPLE_COUNT; ++i)
        {
            sample_average += analog_read(gpio);
        }

        sample_average /= SAMPLE_COUNT;

        if (no_trace == false)
        {
            DEBUG("analog read sample average %lu", sample_average)
        }

        float _value = Zero2tenConfig::InputChannel::adc_value_2_mv(sample_average, atten);

        if (no_trace == false)
        {
            DEBUG("analog read mv %f", _value)
        }

        value = _value / (ratio * 1000);

        if (no_trace == false)
        {
            DEBUG("calculated value v %f", value)
        }

        if (_is_calibrated(input_channel_data[channel].calibration_coefficient) == true)
        {
            float relative_calibration_coefficient = 1;

            try
            {
                relative_calibration_coefficient = input_gain_map_table.x_to_y(value) * input_channel_data[channel].calibration_coefficient;
            }
            catch(const char * str)
            {
                ERROR("input gain function is invalid, assuming relative_calibration_coefficient=1")
            }

            value = value * relative_calibration_coefficient;

            if (no_trace == false)
            {
                TRACE("calibration coefficient %f, relative calibration coefficient %f", input_channel_data[channel].calibration_coefficient,
                        relative_calibration_coefficient)

                DEBUG("adjusted by calibration value v %f", value)
            }
        }

        status.input_channels[channel].value = value;
        // DEBUG("adjusted by calibration value v %f", value)
    }
    else
    {
        r = String("input channel number ") + String((int) channel) + " is invalid";
    }

    return r;
}

String Zero2tenHandler::calibrate_output(size_t channel, float value)
{
    TRACE("calibrating output channel %d, value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);

    // calibration ought to be done by gradually increasing the output value until middle of the range is reached; then 
    // calibration command has to be given with the actually measured voltage; it i posible to calibrate in the range
    // 0.1 max_voltage ... 1.0 max_voltage; the system automatically adjusts calibration value with the help of
    // gain function

    // note that hardware can have saturation at the end of the function (e.g. after duty 9.5)

    if (channel < output_channel_data.size())
    {
        float max_voltage = config.output_channels[channel].max_voltage;

        if (_is_calibrated(output_channel_data[channel].calibration_coefficient) == false)
        {
            if (output_channel_data[channel].value > max_voltage * 0.08)
            {
                if (value > 0)
                {
                    // if the channel is not calibrated then gain function is not used; thus we make coefficient
                    // correspond to gain 1.0

                    float calibration_coefficient = output_channel_data[channel].value / value;
                    float gain = 1;
                    
                    try
                    {
                        gain = output_gain_map_table.x_to_y(value); 
                    }
                    catch(const char * str)
                    {
                        ERROR("output gain function is invalid, assuming gain=1")
                    }

                    output_channel_data[channel].calibration_coefficient = calibration_coefficient / gain;

                    status.output_channels[channel].calibration_coefficient = output_channel_data[channel].calibration_coefficient;  // always make sure it is synced 
                    _data_needs_save = true;

                    TRACE("calculated calibration coefficient raw for output channel %d is %f, gain is %f", (int) channel, calibration_coefficient, gain)
                    TRACE("adjusted calibration coefficient (gain 1.0) for output channel %d is set to %f", (int) channel, output_channel_data[channel].calibration_coefficient)

                    output(channel, value);  // this will also set _data_needs_save at success
                }
                else
                {
                    r = String("calibration value should be > 0");
                }
            }
            else
            {
                r = String("calibration cannot be done when set value is under max_voltage * 0.08 (") + String((float) max_voltage/2) + String(")");
            }
        }
        else
        {
            r = String("calibration cannot be done, channel already calibrated; remove calibration and try again (run calibrate without value)");
        }
    }
    else
    {
        r = String("output channel number ") + String((int) channel) + " is invalid";
    }

    if (r.length())
    {
        ERROR(r.c_str())
    }

    return r;
}

String Zero2tenHandler::uncalibrate_output(size_t channel)
{
    TRACE("uncalibrating output channel %d", (int) channel)
    String r;

    Lock lock(semaphore);

    if (channel < output_channel_data.size())
    {
        if (_is_calibrated(output_channel_data[channel].calibration_coefficient))
        {
            _set_not_calibrated(output_channel_data[channel].calibration_coefficient);
            _data_needs_save = true;

            TRACE("calibration coefficient for output channel %d is reset", (int) channel)

            output(channel, output_channel_data[channel].value);  // this will also set _data_needs_save at success
        }
    }
    else
    {
        r = String("output channel number ") + String((int) channel) + " is invalid";
        ERROR(r.c_str())
    }

    return r;
}


String Zero2tenHandler::output(size_t channel, float value)
{
    TRACE("setting output channel %d to value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);

    if (channel < output_channel_data.size())
    {
        float max_voltage = config.output_channels[channel].max_voltage;
        status.output_channels[channel].calibration_coefficient = output_channel_data[channel].calibration_coefficient;  // always make sure it is synced 
        
        if (value >= 0 && value <= max_voltage)
        {
            float relative_calibration_coefficient = 1;

            if (_is_calibrated(output_channel_data[channel].calibration_coefficient))
            {
                try
                {
                    relative_calibration_coefficient = output_gain_map_table.x_to_y(value) * output_channel_data[channel].calibration_coefficient;
                }
                catch(const char * str)
                {
                    ERROR("output gain function is invalid, assuming relative_calibration_coefficient=1")
                }

                TRACE("calibration coefficient %f, relative calibration coefficient %f", output_channel_data[channel].calibration_coefficient,
                      relative_calibration_coefficient)
            }
            
            uint32_t duty = (uint32_t) (((value * relative_calibration_coefficient) / max_voltage) * float((1ul << PWM_RESOLUTION)-1));
            ledc_channel_t _ledc_channel = ledc_channel_t(LEDC_CHANNEL_0 + channel);

            TRACE("calculated duty as %lu / %lu from calibrated value %f / %f", (unsigned long)duty, (unsigned long)((1ul << PWM_RESOLUTION)-1), value, max_voltage)

            //esp_err_t esp_r = ledc_set_duty_and_update(PWM_SPEED_MODE, _ledc_channel, duty, PWM_HPOINT);

            esp_err_t esp_r = ledc_set_duty(PWM_SPEED_MODE, _ledc_channel, duty);
            
            if (esp_r != ESP_OK)
            {
                status.output_channels[channel].status = String("failed to set duty on channel ") + String((int) channel) + ", " + esp_err_2_string(esp_r); 
                ERROR(status.output_channels[channel].status.c_str())
            }
            else
            {
                esp_r = ledc_update_duty(PWM_SPEED_MODE, _ledc_channel);

                if (esp_r != ESP_OK)
                {
                    status.output_channels[channel].status = String("failed to update duty on channel ") + String((int) channel) + ", " + esp_err_2_string(esp_r); 
                    ERROR(status.output_channels[channel].status.c_str())
                }
                else
                {
                    status.output_channels[channel].value = value;
                    output_channel_data[channel].value = value;    
                    status.output_channels[channel].duty = (float) duty /  float((1ul << PWM_RESOLUTION)-1); 
                    _data_needs_save = true;
                    status.output_channels[channel].status.clear();
                }
            }
        }
        else
        {
            r = String("value (voltage) is out of range 0...") + String((float) max_voltage);
        }
    }
    else
    {
        r = String("output channel number ") + String((int) channel) + " is invalid";
    }

    return r;
}

bool Zero2tenHandler::does_data_need_save() 
{
    return _data_needs_save;
}

void Zero2tenHandler::data_saved() 
{
    _data_needs_save = false;
}

void Zero2tenHandler::data_to_eprom(std::ostream &os) 
{
    Lock lock(semaphore);

    DEBUG("Zero2tenHandler data_to_eprom")

    uint8_t eprom_version = (uint8_t)DATA_EPROM_VERSION;
    os.write((const char *)&eprom_version, sizeof(eprom_version));

    uint8_t count = input_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=input_channel_data.begin(); it!=input_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }
    
    count = output_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=output_channel_data.begin(); it!=output_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }
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

        std::vector<InputChannelData> new_input_channel_data;

        uint8_t count = 0;
        is.read((char *)&count, sizeof(count));

        if (is.bad())
        {
            ERROR("error reading input channel data count from eprom. corrupt?")
            return false;
        }

        for (size_t i=0; i<count; ++i)
        {
            InputChannelData input_channel_data_item;
            
            if (input_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading input channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_input_channel_data.push_back(input_channel_data_item);
        }

        for (size_t i=0; i<config.input_channels.size(); ++i)
        {
            if (i < new_input_channel_data.size())
            {
                input_channel_data[i] = new_input_channel_data[i];
            }
            else
            {
                input_channel_data.push_back(InputChannelData());
            }
            
            status.input_channels[i].calibration_coefficient =  input_channel_data[i].calibration_coefficient; 
        }

        std::vector<OutputChannelData> new_output_channel_data;

        count = 0;
        is.read((char *)&count, sizeof(count));

        if (is.bad())
        {
            ERROR("error reading output channel data count from eprom. corrupt?")
            return false;
        }

        for (size_t i=0; i<count; ++i)
        {
            OutputChannelData output_channel_data_item;
            
            if (output_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading output channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_output_channel_data.push_back(output_channel_data_item);
        }

        for (size_t i=0; i<config.output_channels.size(); ++i)
        {
            if (i < new_output_channel_data.size())
            {
                output_channel_data[i] = new_output_channel_data[i];
            }
            else
            {
                output_channel_data.push_back(OutputChannelData());
            }
            
            status.output_channels[i].calibration_coefficient =  output_channel_data[i].calibration_coefficient; 
        }

        DEBUG("outputting values being read")

        size_t i=0;

        for (auto it=output_channel_data.begin(); it!=output_channel_data.end(); ++it, ++i)
        {
            output(i, it->value);
        }

        _data_needs_save = false;  // output(..) sets this to true, but we just read the values from eprom 
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
                const char * function_type_str = function_type_2_str((FunctionType) it->first);
                TRACE("Found block type for function %s", function_type_str)

                std::istringstream is(it->second);

                if (data_from_eprom(is) == true)
                {
                    TRACE("Zero2ten data read success")
                    return true;
                }
                else
                {
                    TRACE("Zero2ten data read failure")
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

String zero2ten_calibrate_input(const String & channel_str, const String & value_str)
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
                    return handler.calibrate_input(channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_input(channel);
            }
        }
    }
    
    return "Parameter error";
}

String zero2ten_input(const String & channel_str, String & value_str)
{
    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = -1;
        
            String r = handler.input(channel, value);

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

String zero2ten_calibrate_output(const String & channel_str, const String & value_str)
{
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
                    return handler.calibrate_output(channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_output(channel);
            }
        }
    }
    
    return "Parameter error";
}

String zero2ten_output(const String & channel_str, const String & value_str)
{
    if (!channel_str.isEmpty() && !value_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str) && __is_number_or_empty(value_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = (float) value_str.toFloat();
        
            return handler.output(channel, value);
        }
    }
    
    return "Parameter error";
}


#endif // INCLUDE_ZERO2TEN
