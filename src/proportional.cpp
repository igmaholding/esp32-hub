#ifdef INCLUDE_PROPORTIONAL
#include <ArduinoJson.h>
#include <proportional.h>
#include <autonom.h>
#include <gpio.h>
#include <binarySemaphore.h>
#include <Wire.h>
#include <deque>
#include <epromImage.h>
#include <sstream>

extern GpioHandler gpioHandler;


template<class T, size_t window_size> class Averager
{
    public:

        Averager()
        {
            reset();
        }

        void reset()
        {
            window_pos = -1;
        }

        void push(T value)
        {
            if (window_pos == window_size-1)
            {
                memmove((char*) window, (const char*) window + sizeof(T), sizeof(T) * (window_size-1));
                window[window_pos] = value;
            }
            else
            {
                window_pos++;
                window[window_pos] = value;
            }
        }

        bool is_window_full() const
        {
            return window_pos == window_size-1;   
        }

        T get_average(T empty) const
        {
            if (window_pos == -1)
            {
                return empty;
            }

            T average = window[0];
            
            for(size_t i=1; i<=window_pos; ++i)
            {
                average += window[i];
            }

            return average / (window_pos+1);
        }

        T window[window_size];
        int window_pos;
};



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

bool ProportionalConfig::is_valid() const
{
    GpioCheckpad checkpad;

    size_t i = 0;

    for (auto it = channels.begin(); it != channels.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
            ERROR("channel %d is_valid() == false", (int) i)
            return false;
        }

        char object_name[64];
        sprintf(object_name, "channel[%d].one_a", (int) i);

        if (checkpad.get_usage(it->one_a.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->one_a.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->one_a.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name, (int)it->one_a.gpio);
            return false;
        }

        sprintf(object_name, "channel[%d].one_b", (int) i);

        if (checkpad.get_usage(it->one_b.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->one_b.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->one_b.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name, (int)it->one_b.gpio);
            return false;
        }

        sprintf(object_name, "channel[%d].open", (int) i);

        if (checkpad.get_usage(it->open.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->open.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->open.gpio, GpioCheckpad::uDigitalInput))
        {
            _err_cap(object_name, (int)it->open.gpio);
            return false;
        }

        sprintf(object_name, "channel[%d].closed", (int) i);

        if (checkpad.get_usage(it->closed.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->closed.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->closed.gpio, GpioCheckpad::uDigitalInput))
        {
            _err_cap(object_name, (int)it->closed.gpio);
            return false;
        }

        sprintf(object_name, "channel[%d].load_detect", (int) i);

        if (checkpad.get_usage(it->load_detect.pin.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->load_detect.pin.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->load_detect.pin.gpio, GpioCheckpad::uAnalogInput))
        {
            _err_cap(object_name, (int)it->load_detect.pin.gpio);
            return false;
        }
    }
    
    return true;
}


void ProportionalConfig::from_json(const JsonVariant &json)
{
    //DEBUG("proportional config from_json")
    clear();

    if (json.containsKey("channels"))
    {
        const JsonVariant &_json = json["channels"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                Channel channel;
                channel.from_json(__json);
                channels.push_back(channel);
                //DEBUG("Channel from_json, %s", channel.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("valve_profiles"))
    {
        //DEBUG("contains key valve_profiles")
        const JsonVariant &_json = json["valve_profiles"];

        if (_json.is<JsonArray>())
        {
            //DEBUG("valve_profiles is array")
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                //DEBUG("analysing valve_profiles item")
                const JsonVariant & __json = *iterator;

                if (__json.containsKey("name"))
                {
                    String name = __json["name"]; 
                    ValveProfile valve_profile;
                    valve_profile.from_json(__json);
                    valve_profiles.insert(std::make_pair(name,valve_profile));
                    //DEBUG("valve_profile from_json: name=%s, %s", name.c_str(), valve_profile.as_string().c_str())
                }
                ++iterator;
            }
        }
    }
}

void ProportionalConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));

    uint8_t count = (uint8_t)channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = channels.begin(); it != channels.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)valve_profiles.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = valve_profiles.begin(); it != valve_profiles.end(); ++it)
    {
        uint8_t len = it->first.length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->first.c_str(), len);

        it->second.to_eprom(os);
    }
}

bool ProportionalConfig::from_eprom(std::istream &is)
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
            Channel channel;
            channel.from_eprom(is);
            channels.push_back(channel);
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

                ValveProfile valve_profile;
                valve_profile.from_eprom(is);
                valve_profiles[name] = valve_profile;

            }
        }
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read ProportionalConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void ProportionalConfig::Channel::LoadDetect::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("pin"))
    {
        const JsonVariant &_json = json["pin"];
        pin.from_json(_json);
    }

    if (json.containsKey("resistance"))
    {
        resistance = (float) json["resistance"];
    }

    if (json.containsKey("current_threshold"))
    {
        current_threshold = (float) json["current_threshold"];
    }
}

void ProportionalConfig::Channel::LoadDetect::to_eprom(std::ostream &os) const
{
    pin.to_eprom(os);

    os.write((const char *)&resistance, sizeof(resistance));
    os.write((const char *)&current_threshold, sizeof(current_threshold));
}

bool ProportionalConfig::Channel::LoadDetect::from_eprom(std::istream &is)
{
    clear();

    pin.from_eprom(is);

    is.read((char *)&resistance, sizeof(resistance));
    is.read((char *)&current_threshold, sizeof(current_threshold));

    return is_valid() && !is.bad();
}

void ProportionalConfig::Channel::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("one_a"))
    {
        const JsonVariant &_json = json["one_a"];
        one_a.from_json(_json);
    }

    if (json.containsKey("one_b"))
    {
        const JsonVariant &_json = json["one_b"];
        one_b.from_json(_json);
    }

    if (json.containsKey("open"))
    {
        const JsonVariant &_json = json["open"];
        open.from_json(_json);
    }

    if (json.containsKey("closed"))
    {
        const JsonVariant &_json = json["closed"];
        closed.from_json(_json);
    }

    if (json.containsKey("load_detect"))
    {
        const JsonVariant &_json = json["load_detect"];
        load_detect.from_json(_json);
    }

    if (json.containsKey("valve_profile"))
    {
        valve_profile = (const char*) json["valve_profile"];
    }

    if (json.containsKey("default_value"))
    {
        default_value = (uint8_t)(int) json["default_value"];
    }
}

void ProportionalConfig::Channel::to_eprom(std::ostream &os) const
{
    one_a.to_eprom(os);
    one_b.to_eprom(os);
    open.to_eprom(os);
    closed.to_eprom(os);
    load_detect.to_eprom(os);

    uint8_t len = valve_profile.length();
    os.write((const char *)&len, sizeof(len));
    os.write((const char *)valve_profile.c_str(), len);
    os.write((const char *)&default_value, sizeof(default_value));
}

bool ProportionalConfig::Channel::from_eprom(std::istream &is)
{
    clear();

    one_a.from_eprom(is);
    one_b.from_eprom(is);
    open.from_eprom(is);
    closed.from_eprom(is);
    load_detect.from_eprom(is);

    uint8_t len = 0;
    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        valve_profile = buf;
    }

    is.read((char *)&default_value, sizeof(default_value));

    return is_valid() && !is.bad();
}

void ProportionalConfig::ValveProfile::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("open_time"))
    {
        open_time = json["open_time"];
    }

    if (json.containsKey("max_actuate_add_ups"))
    {
        max_actuate_add_ups = json["max_actuate_add_ups"];
    }

    if (json.containsKey("time_2_flow_rate"))
    {
        const JsonVariant &_json = json["time_2_flow_rate"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                if (__json.is<JsonArray>())
                {
                    const JsonArray & _jsonArray = __json.as<JsonArray>();
                    auto _iterator = _jsonArray.begin();

                    uint8_t first = 255;
                    uint8_t second = 255;

                    if (_iterator != _jsonArray.end())
                    {
                        first = *_iterator;       
                        ++_iterator;
                    }

                    if (_iterator != _jsonArray.end())
                    {
                        second = *_iterator;       
                    }
                
                    if (first != 255 && second != 255)
                    {
                        time_2_flow_rate.push_back(std::make_pair(first, second));
                    }
                }

                ++iterator;
            }
        }
    }
}

void ProportionalConfig::ValveProfile::to_eprom(std::ostream &os) const
{
    os.write((const char *)&open_time, sizeof(open_time));
    os.write((const char *)&max_actuate_add_ups, sizeof(max_actuate_add_ups));

    uint8_t len = time_2_flow_rate.size();
    os.write((const char *)&len, sizeof(len));
    
    for (auto it=time_2_flow_rate.begin(); it!=time_2_flow_rate.end();++it)
    {
        os.write((const char *)(& (it->first)), sizeof(it->first));
        os.write((const char *)(& (it->second)), sizeof(it->second));
    }
}

bool ProportionalConfig::ValveProfile::from_eprom(std::istream &is)
{
    clear();

    is.read((char *)&open_time, sizeof(open_time));
    is.read((char *)&max_actuate_add_ups, sizeof(max_actuate_add_ups));

    uint8_t len = 0;
    is.read((char *)&len, sizeof(len));

    if (len)
    {
        for (size_t i=0; i<len; ++i)
        {
            uint8_t first = 0;
            uint8_t second = 0;
            is.read((char *)&first, sizeof(first));
            is.read((char *)&second, sizeof(second));
            time_2_flow_rate.push_back(std::make_pair(first, second));
        }
    }

    return is_valid() && !is.bad();
}


/*
    1. channel created, calibration ordered
    2. value is actuated to where it ended up after calibration (0 or 100, unddefined if calibration failed)
    3. eprom is read; read value is actuated

    After calibration value needs to be set again manually!

*/

class ChannelHandler
{
public:

    const uint8_t MAX_ACTUATE_ADD_UPS = 1;

    ChannelHandler()
    {
        _is_active = false;
        _calib_data_needs_save = false;
        _value_needs_save = false;
        actuate_ref = UINT8_MAX;
        actuate_ms = 0;
        actuate_add_ups = 0;
        next_actuate_ref = UINT8_MAX;
    }

    ~ChannelHandler()
    {
    }

    bool is_active() const { return _is_active; }
    bool is_idle() const { return status.state == ProportionalStatus::Channel::sIdle; }

    void start(const ProportionalConfig::Channel &_config, const ProportionalConfig::ValveProfile &_valve_profile);
    void stop();
    void reconfigure(const ProportionalConfig::Channel &_config);
    void set_valve_profile(const ProportionalConfig::ValveProfile &_valve_profile);
    const ProportionalConfig::Channel & get_config() const { return config; }

    ProportionalStatus::Channel get_status()
    {
        ProportionalStatus::Channel _status;

        {
            Lock lock(semaphore);
            status.config_open_time = get_config_open_time();
            status.max_actuate_add_ups = valve_profile.max_actuate_add_ups; 
            _status = status;
        }

        return _status;
    }

    uint32_t get_config_open_time() const
    {
        return valve_profile.open_time * 1000; // ms
    }

    String calibrate();
    String actuate(uint8_t value, uint8_t ref = UINT8_MAX, bool force = false);

    bool has_calib_data() const
    {
        #ifdef ASYMMETRICAL_OPEN_CLOSE

        return status.calib_open_2_closed_time > 0 && status.calib_closed_2_open_time > 0;

        #else

        return status.calib_open_time > 0;

        #endif // ASYMMETRICAL_OPEN_CLOSE
    }

    uint8_t get_value() const
    {
        return status.value;
    }

    void calib_data_to_eprom(std::ostream & os) const
    {
        #ifdef ASYMMETRICAL_OPEN_CLOSE

        os.write((const char *)&status.calib_open_2_closed_time, sizeof(status.calib_open_2_closed_time));
        os.write((const char *)&status.calib_closed_2_open_time, sizeof(status.calib_closed_2_open_time));

        #else

        os.write((const char *)&status.calib_open_time, sizeof(status.calib_open_time));

        #endif // ASYMMETRICAL_OPEN_CLOSE
    }
    
    bool calib_data_from_eprom(std::istream & is)
    {
        // skip taking lock, integral data and no way to signal to possible ongoing actuation
        #ifdef ASYMMETRICAL_OPEN_CLOSE

        is.read((char *)&status.calib_open_2_closed_time, sizeof(status.calib_open_2_closed_time));
        is.read((char *)&status.calib_closed_2_open_time, sizeof(status.calib_closed_2_open_time));

        #else

        is.read((char *)&status.calib_open_time, sizeof(status.calib_open_time));

        #endif // ASYMMETRICAL_OPEN_CLOSE

        return !is.bad();
    }

    static bool dummy_calib_data_from_eprom(std::istream & is)
    {
        ProportionalStatus::Channel dummy_status;

        #ifdef ASYMMETRICAL_OPEN_CLOSE

        is.read((char *)&dummy_status.calib_open_2_closed_time, sizeof(dummy_status.calib_open_2_closed_time));
        is.read((char *)&dummy_status.calib_closed_2_open_time, sizeof(dummy_status.calib_closed_2_open_time));

        #else

        is.read((char *)&dummy_status.calib_open_time, sizeof(dummy_status.calib_open_time));

        #endif // ASYMMETRICAL_OPEN_CLOSE

        return !is.bad();
    }

    bool does_calib_data_need_save() const
    {
        return _calib_data_needs_save;
    }

    void calib_data_saved() 
    {
        _calib_data_needs_save = false; 
    }

    bool does_value_need_save() const
    {
        return _value_needs_save;
    }

    void value_saved() 
    {
        _value_needs_save = false; 
    }

    protected:

    void configure_hw();

    static void write_one(const ProportionalConfig::Channel & config, bool a_value, bool b_value);

    static void write_one_2_open(const ProportionalConfig::Channel & config);
    static void write_one_2_closed(const ProportionalConfig::Channel & config);
    static void write_one_stop(const ProportionalConfig::Channel & config);

    static bool read(const DigitalInputChannelConfig & config);

    static float read_current(const ProportionalConfig::Channel::LoadDetect & config);

    static unsigned analog_read(uint8_t gpio);

    static void calibration_task(void *parameter);
    static void actuation_task(void *parameter);

    static uint32_t flow_2_time(const ProportionalConfig::ValveProfile & _valve_profile,  
                                int flow_percent, uint32_t open_time, uint8_t from);

    BinarySemaphore semaphore;
    ProportionalConfig::Channel config;
    ProportionalConfig::ValveProfile valve_profile;
    ProportionalStatus::Channel status;
    bool _is_active;

    bool _calib_data_needs_save;
    bool _value_needs_save;

    uint8_t actuate_ref;
    uint32_t actuate_ms;

    // this indicates how many actuations were made from the previous actuation as a reference
    // after some (MAX_ACTUATE_ADD_UPS or value from valve_profile) the motor is reset to a nearest end-state and the actuation
    // is done from there

    uint8_t actuate_add_ups;
    uint8_t next_actuate_ref;
};

class ProportionalHandler
{
public:

    #ifdef ASYMMETRICAL_OPEN_CLOSE
    
    const int DATA_EPROM_VERSION = 2;

    #else

    const int DATA_EPROM_VERSION = 102;

    #endif


    struct ActionAndValue
    {
        enum Action
        {
            aNone = 0,
            aCalibrate = 1,
            aActuate = 2
        };

        ActionAndValue()
        {
            action = aNone;
            value = UINT8_MAX;
            ref = UINT8_MAX;
            force = false;
        }

        ActionAndValue(Action _action, uint8_t _value = UINT8_MAX, uint8_t _ref = UINT8_MAX, bool _force = false) : 
              action(_action), value(_value), ref(_ref), force(_force)
        {
        }

        Action action;
        uint8_t value;
        uint8_t ref;
        bool force;
    };

    ProportionalHandler()
    {
        _is_active = false;
        _is_finished = true;
    }

    ~ProportionalHandler()
    {
        delete_all_channel_handlers();
    }

    bool is_active() const { return _is_active; }

    void start(const ProportionalConfig &config);
    void stop();
    void reconfigure(const ProportionalConfig &config);

    ProportionalStatus get_status()
    {
        ProportionalStatus _status;

        {
            Lock lock(semaphore);

            status.channels.clear();

            for (auto it=channel_handlers.begin(); it!=channel_handlers.end(); ++it)
            {
                status.channels.push_back((*it)->get_status());
            }

            _status = status;
        }

        return _status;
    }

    String calibrate(size_t channel);
    String actuate(size_t channel, uint8_t value, uint8_t ref = UINT8_MAX, bool force = false);

    void calibrate_uncalibrated();
    
    bool does_data_need_save();
    void data_saved();

    void data_to_eprom(std::ostream & os);
    bool data_from_eprom(std::istream & is);

    bool read_data();
    void save_data();

    void configure_channels();
    void update_valve_profiles();

    size_t get_num_channels() const
    {
        return channel_handlers.size();
    }

protected:

    void delete_all_channel_handlers();

    static void task(void *parameter);

    BinarySemaphore semaphore;
    ProportionalConfig config;
    ProportionalStatus status;
    bool _is_active;
    bool _is_finished;

    std::vector<ChannelHandler*> channel_handlers;
    std::deque<std::pair<int, ActionAndValue>> action_queue;
};

static ProportionalHandler handler;

void ChannelHandler::start(const ProportionalConfig::Channel &_config, 
                           const ProportionalConfig::ValveProfile &_valve_profile)
{
    TRACE("ChannelHandler::start, status %s", status.as_string().c_str())
 
    if (_is_active)
    {
        ERROR("attempt to start a channel that is already active, %s", _config.as_string().c_str())
    }
    else
    {
        // wait for calibration or actuation of previous active time to abort (running->stop->start situation)

        while(status.state == ProportionalStatus::Channel::sCalibrating || 
                status.state == ProportionalStatus::Channel::sActuating)
        {
            delay(100);
        }

        TRACE("starting channel %s", _config.as_string().c_str())
        
        config = _config;
        
        status.clear();
        status.state = ProportionalStatus::Channel::sIdle;

        status.value = config.default_value;

        configure_hw();

        set_valve_profile(_valve_profile);

        _is_active = true;
    }

    TRACE("ChannelHandler::start returns, status %s", status.as_string().c_str())
}

void ChannelHandler::stop()
{
    TRACE("ChannelHandler::stop channel %s, status %s", config.as_string().c_str(), status.as_string().c_str())

    _is_active = false; // this should trigger aborting possible calibration or actuation

    while(status.state == ProportionalStatus::Channel::sCalibrating || 
            status.state == ProportionalStatus::Channel::sActuating)
    {
        delay(100);
    }

    write_one(config, false, false);  // just in case

    status.clear();
    config.clear();

    TRACE("ChannelHandler::stop returns, status %s", status.as_string().c_str())
}

void ChannelHandler::reconfigure(const ProportionalConfig::Channel &_config)
{
    TRACE("ChannelHandler::reconfigure, status %s", status.as_string().c_str())
    TRACE("config from %s", config.as_string().c_str())
    TRACE("config to %s", _config.as_string().c_str())

    if (!(config == _config))
    {
        TRACE("channel: config changed")

        _is_active = false;

        while(status.state == ProportionalStatus::Channel::sCalibrating || 
                status.state == ProportionalStatus::Channel::sActuating)
        {
            delay(100);
        }

        if (config.will_need_calibrate(_config) == false)
        {
            status.clear_keep_calib_data();

        }
        else
        {
            status.clear();
        }

        config = _config;
        
        status.state = ProportionalStatus::Channel::sIdle;

        status.value = config.default_value;

        configure_hw();

        _is_active = true;
    }
    TRACE("ChannelHandler::reconfigure returns, status %s", status.as_string().c_str())
}

void ChannelHandler::set_valve_profile(const ProportionalConfig::ValveProfile &_valve_profile)
{
    if (!(valve_profile == _valve_profile))
    {
        valve_profile = _valve_profile;

        if (valve_profile.time_2_flow_rate.empty() == false)
        {
            size_t size = valve_profile.time_2_flow_rate.size();

            if (valve_profile.time_2_flow_rate[size-1].second < 100)
            {
                valve_profile.time_2_flow_rate.push_back(std::make_pair(100,100));    
            }
        }
    }
}

String ChannelHandler::calibrate()
{
    TRACE("ChannelHandler::calibrate, status %s", status.as_string().c_str())
    String r;

    if (_is_active == true)
    {
       // manual calibration should be done on explicitly idle channel

       if (status.state == ProportionalStatus::Channel::sCalibrating)
       {
            return r;  // calibration already ongoing
       }

       if (status.state == ProportionalStatus::Channel::sActuating)
       {
            r = "Channel busy";
            ERROR("Request to calibrate, channel busy")
       }
       else
       {
            status.state = ProportionalStatus::Channel::sCalibrating;

            TRACE("starting calibration task")

            xTaskCreate(
                calibration_task,      // Function that should be called
                "calibration",         // Name of the task (for debugging)
                4096,                  // Stack size (bytes)
                this,                  // Parameter to pass
                1,                     // Task priority
                NULL                   // Task handle
            );
       }
    }
    else
    {
        r = "Channel not active";
        ERROR("Request to calibrate, channel not active")
    }

    TRACE("ChannelHandler::calibrate returns, status %s", status.as_string().c_str())
    return r;
}

String ChannelHandler::actuate(uint8_t value, uint8_t ref, bool force)
{
    TRACE("ChannelHandler::actuate, value %d, ref %d, status %s", (int) value, (int) ref, status.as_string().c_str())
    String r;

    if (_is_active == true)
    {
        // actuation should be possible at any time
        // if calibration is ongoing - then actuation will always be done afterwards
        // if another actuation is ongoing - then the task will check at the end and restart actuation if the
        // new value is different from its former target value

        if (value > 100)
        {
            value = 100;
        }

        if (!(ref == 0 || ref == 100 || ref == UINT8_MAX))
        {
            ref = UINT8_MAX;
        }

        {Lock lock(semaphore);

        if (force || status.value != value)
        {
            status.value = value;
            _value_needs_save = true;
        }
        else
        {
            r = "Value unchanged, actuation skipped";
        }   

        next_actuate_ref = ref; } 

        // theoretically there could be a slight chance that we fall between the chairs and
        // do not start new actuation at exact moment the one which is ongoing finishes
        // TODO?

        if (r.isEmpty())
        {
            if (status.state == ProportionalStatus::Channel::sIdle)
            {
                status.state = ProportionalStatus::Channel::sActuating;

                TRACE("starting actuation task")

                xTaskCreate(
                    actuation_task,        // Function that should be called
                    "actuation",           // Name of the task (for debugging)
                    4096,                  // Stack size (bytes)
                    this,                  // Parameter to pass
                    1,                     // Task priority
                    NULL                   // Task handle
                );
            }
        }
    }
    else
    {
        r = "Channel not active, actuation skipped";
    }

    if (r.isEmpty() == false)
    {
        TRACE("%s", r.c_str())
    }
    
    TRACE("ChannelHandler::actuate returns, status %s", status.as_string().c_str())

    return r;
}

void ChannelHandler::configure_hw()
{
    // ignore debounce, the circuit contains schmidt trigger that should fix the dribble

    TRACE("configure open: gpio=%d, inverted=%d", (int)config.open.gpio, (int)config.open.inverted)
    gpioHandler.setupChannel(config.open.gpio, INPUT_PULLUP, config.open.inverted, NULL);

    TRACE("configure closed: gpio=%d, inverted=%d", (int)config.closed.gpio, (int)config.closed.inverted)
    gpioHandler.setupChannel(config.closed.gpio, INPUT_PULLUP, config.closed.inverted, NULL);

    write_one(config, false, false);

    TRACE("configure one_a: gpio=%d, inverted=%d", (int)config.one_a.gpio, (int)config.one_a.inverted)
    gpioHandler.setupChannel(config.one_a.gpio, OUTPUT, config.one_a.inverted, NULL);

    TRACE("configure one_b: gpio=%d, inverted=%d", (int)config.one_b.gpio, (int)config.one_b.inverted)
    gpioHandler.setupChannel(config.one_b.gpio, OUTPUT, config.one_b.inverted, NULL);

    TRACE("configure load_detect.pin: gpio=%d, atten=%d", (int)config.load_detect.pin.gpio, 
          (int)config.load_detect.pin.atten)
    analogSetPinAttenuation(config.load_detect.pin.gpio, (adc_attenuation_t)config.load_detect.pin.atten);
}

void ChannelHandler::write_one(const ProportionalConfig::Channel & config, bool a_value, bool b_value)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    GpioChannel::write(config.one_a.gpio, config.one_a.inverted, a_value);
    GpioChannel::write(config.one_b.gpio, config.one_b.inverted, b_value);

    DEBUG("write_one (a=%s gpio%d, b=%s gpio%d)", a_value ? "true":"false", (int) config.one_a.gpio, b_value ? "true":"false", (int) config.one_b.gpio)
}

void ChannelHandler::write_one_2_open(const ProportionalConfig::Channel & config)
{
    write_one(config, false, true);
}

void ChannelHandler::write_one_2_closed(const ProportionalConfig::Channel & config)
{
    write_one(config, true, false);
}

void ChannelHandler::write_one_stop(const ProportionalConfig::Channel & config)
{
    write_one(config, false, false);
}

bool ChannelHandler::read(const DigitalInputChannelConfig & config)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    return GpioChannel::read(config.gpio, config.inverted);
}

float ChannelHandler::read_current(const ProportionalConfig::Channel::LoadDetect & _config)
{
    unsigned readout = analog_read(_config.pin.gpio);
    float voltage = (readout * 0.75 ) / 8191;  
    float current = voltage * _config.resistance;
    DEBUG("read_current: readout %d, voltage %f, current %f", (int) readout, voltage, current)
    return current;
}

unsigned ChannelHandler::analog_read(uint8_t gpio)
{
    adcAttachPin(gpio);
    /* adcStart(gpio);
    
    int max_attempts = 10;

    while(adcBusy(gpio) == true)
    {
        delay(10);
        max_attempts--;

        if (max_attempts <= 0)
        {
            ERROR("Timeout reading adc, gpio=%d", (int) gpio)
            return 0;
        }
    } */

    return analogRead(gpio);
}

void ChannelHandler::calibration_task(void *parameter)
{
    ChannelHandler *_this = (ChannelHandler *)parameter;

    TRACE("calibration_task started, status %s", _this->status.as_string().c_str())

    // try to minimize go-time by either going to another end-state if the motor is in one of the end-states
    // or making to end-state + open-close cycle so that we end up closest to the value 

    size_t timeout_seconds = 40;

    if (_this->valve_profile.open_time != 0)
    {
        timeout_seconds = _this->valve_profile.open_time * 4;    
    }

    DEBUG("calibration timeout set to %d seconds", (int) timeout_seconds)

    size_t num_passes = 3;

    uint8_t run_to[3];
    uint32_t milliseconds[3];
    
    bool is_open = read(_this->config.open);
    bool is_closed = read(_this->config.closed);

    #ifdef ASYMMETRICAL_OPEN_CLOSE

    if (is_open == true || is_closed == true)
    {
        num_passes = 2;

        if (is_open == true)
        {
            run_to[0] = 0;
            run_to[1] = 100;
        }
        else
        {
            run_to[0] = 100;
            run_to[1] = 0;
        }
    }
    else
    {
        num_passes = 3;

        if (_this->status.value > 50)
        {
            run_to[0] = 100;
            run_to[1] = 0;
            run_to[2] = 100;
        }
        else
        {
            run_to[0] = 0;
            run_to[1] = 100;
            run_to[2] = 0;
        }
    }

    #else

    if (is_open == true || is_closed == true)
    {
        num_passes = 1;

        if (is_open == true)
        {
            run_to[0] = 0;
        }
        else
        {
            run_to[0] = 100;
        }
    }
    else
    {
        num_passes = 2;

        if (_this->status.value > 50)
        {
            run_to[0] = 0;
            run_to[1] = 100;
        }
        else
        {
            run_to[0] = 100;
            run_to[1] = 0;
        }
    }

    #endif // ASYMMETRICAL_OPEN_CLOSE

    int pass = 0;
    bool is_error = false;
    const char * error_str = "";
    Averager<float, 5> current_averager;

    int _is_open = (int)read(_this->config.open);
    int _is_closed = (int)read(_this->config.closed);

    DEBUG("before calibration, open=%d, closed=%d", _is_open, _is_closed)

    if (_is_open && _is_closed)
    {
        ERROR("calibration cannot be started, both open and closed are 1")
        is_error = true;
        error_str = "calibration error: both open and closed are 1";
    }
    else
    {
        while(pass < num_passes)
        {
            TRACE("calibration pass %d, run_to %d", pass, (int) run_to[pass])

            uint32_t t_begin = millis();
            DigitalInputChannelConfig wait_on;

            if (run_to[pass] == 100)
            {
                wait_on = _this->config.open;
            }
            else
            {
                wait_on = _this->config.closed;
            }

            bool ready = read(wait_on);

            if (ready == false)
            {
                if (run_to[pass] == 100)
                {
                    write_one_2_open(_this->config);
                }
                else
                {
                    write_one_2_closed(_this->config);
                }
            }

            size_t c_count = 0;
            current_averager.reset();

            while(_this->_is_active == true)
            {
                if (ready == true)
                {
                    break;
                }

                delay(1);
                ready = read(wait_on);

                if (c_count % 200 == 0)
                {
                    unsigned long t_probe = millis();
                    unsigned long time_passed = 0;

                    if (t_begin <= t_probe)
                    {
                        time_passed = t_probe - t_begin;
                    }
                    else
                    {
                        time_passed = t_probe + (ULONG_MAX - t_begin);
                    }

                    if (time_passed > timeout_seconds * 1000)
                    {
                        ERROR("calibration timeout detected, t_begin %d, t_probe %d", (int) t_begin, (int) t_probe)
                        is_error = true;
                        error_str = "calibration error: timeout";
                        break;
                    }

                    float current = read_current(_this->config.load_detect);
                    current_averager.push(current);

                    if (current_averager.is_window_full())
                    {
                        float average_current = current_averager.get_average(0);
                        TRACE("average current %f", average_current)

                        if (average_current < _this->config.load_detect.current_threshold)
                        {
                            ERROR("calibration no load detected, measured current %f, configured threshold %f", 
                                current, _this->config.load_detect.current_threshold)
                            is_error = true;
                            error_str = "calibration error: no load";
                            break;
                        }
                    }
                }
                
                c_count ++;
            }

            write_one_stop(_this->config);

            if (_this->_is_active == false)
            {
                break;
            }
            
            if (is_error == true)
            {
                break;
            }

            unsigned long t_end = millis();

            if (t_begin < t_end)
            {
                milliseconds[pass] = t_end - t_begin;
            }
            else
            {
                milliseconds[pass] = t_end + (ULONG_MAX - t_begin);
            }

            _this->status.value = run_to[pass]; 

            TRACE("milliseconds %d", (int) milliseconds[pass])
            pass++;

            delay(1000); // let the motor stop
        }
    }

    {Lock lock(_this->semaphore);

    if (is_error == true || _this->_is_active == false)
    {
        #ifdef ASYMMETRICAL_OPEN_CLOSE

        _this->status.calib_closed_2_open_time = 0;
        _this->status.calib_open_2_closed_time = 0;

        #else

        _this->status.calib_open_time = 0;

        #endif // ASYMMETRICAL_OPEN_CLOSE

        _this->actuate_ref = UINT8_MAX;

        if  (_this->_is_active == false)
        {
            error_str = "calibration error: aborted (is_active==false)";
        }

        _this->status.error = error_str;
        ERROR(error_str)
    }
    else
    {
        _this->status.error.clear();

        #ifdef ASYMMETRICAL_OPEN_CLOSE

        if (num_passes == 3)
        {
            _this->status.calib_open_2_closed_time = run_to[1] == 0 ? milliseconds[1] : milliseconds[2];
            _this->status.calib_closed_2_open_time = run_to[1] == 100 ? milliseconds[1] : milliseconds[2];

            _this->actuate_ref = run_to[2];
        }
        else
        {
            _this->status.calib_open_2_closed_time = run_to[0] == 0 ? milliseconds[0] : milliseconds[1];
            _this->status.calib_closed_2_open_time = run_to[0] == 100 ? milliseconds[0] : milliseconds[1];

            _this->actuate_ref = run_to[1];
        }

        #else

        if (num_passes == 2)
        {
            _this->status.calib_open_time = milliseconds[1];
            _this->actuate_ref = run_to[1];
        }
        else
        {
            _this->status.calib_open_time = milliseconds[0];
            _this->actuate_ref = run_to[0];
        }

        #endif // ASYMMETRICAL_OPEN_CLOSE
    }

    _this->actuate_ms = 0;
    _this->status.state = ProportionalStatus::Channel::sIdle; 

    _this->_calib_data_needs_save = true; }

    TRACE("calibration_task: terminated, status %s", _this->status.as_string().c_str())
    vTaskDelete(NULL);
}

void ChannelHandler::actuation_task(void *parameter)
{
    ChannelHandler *_this = (ChannelHandler *)parameter;

    TRACE("actuation_task started, status %s", _this->status.as_string().c_str())

    while(1)  // in case value has changed while actuation is ongoing
    {
        TRACE("before: actuate_ref %d, actuate_ms %d, actuate_add_ups %d next_actuate_ref %d", (int) _this->actuate_ref,
            (int) _this->actuate_ms, (int) _this->actuate_add_ups, (int) _this->next_actuate_ref)
        
        // store and use this in case actuation value changes during this task; if it happens - restart the while loop 
        uint8_t current_value = _this->status.value;  

        uint32_t open_2_closed_time = 0;
        uint32_t closed_2_open_time = 0;

        // unset actuate_ref means tbat we need to restart to some reference and go from there;
        // one of the use cases including this - reading calibration data from eprom and then directly
        // setting the value

        bool should_restart = !(_this->actuate_ref == 0 || _this->actuate_ref == 100);

        if (should_restart)
        {
            DEBUG("will restart due to unset actuate_ref")
        }

        #ifdef ASYMMETRICAL_OPEN_CLOSE

        open_2_closed_time = _this->status.calib_open_2_closed_time;
        closed_2_open_time = _this->status.calib_closed_2_open_time;

        #else

        open_2_closed_time = _this->status.calib_open_time;
        closed_2_open_time = _this->status.calib_open_time;
        
        #endif

        if (open_2_closed_time == 0)
        {
            open_2_closed_time = _this->get_config_open_time();
            TRACE("will use config_open_time instead of calibrated open_2_closed_time (==0)")
        }

        if (closed_2_open_time == 0)
        {
            closed_2_open_time = _this->get_config_open_time();
            TRACE("will use config_open_time instead of calibrated closed_2_open_time (==0)")
        }

        if (open_2_closed_time > 0 && closed_2_open_time > 0)
        {
            // none of these will be needed if the next_actuate_ref is specified (see should_restart condition
            // below) but we declare them here because they are used for other parts of algo
            
            uint32_t new_actuate_ms_from_closed = flow_2_time(_this->valve_profile, current_value, closed_2_open_time, 0);
            uint32_t new_actuate_ms_from_open = flow_2_time(_this->valve_profile, current_value, open_2_closed_time, 100);  

            uint32_t open_time = closed_2_open_time; 
            uint32_t new_actuate_ms = new_actuate_ms_from_closed;
            int delta = (int) new_actuate_ms;

            if (should_restart)
            {
                // this means unset reference, go the simple case with restarting through reference 0 
                _this->next_actuate_ref = current_value == 100 ? 100 : 0;   
            }
            else
            {
                open_time = _this->actuate_ref == 0 ? closed_2_open_time : open_2_closed_time; 
                new_actuate_ms = _this->actuate_ref == 0 ? new_actuate_ms_from_closed : new_actuate_ms_from_open;
                delta = (int) new_actuate_ms - (int) _this->actuate_ms;

                DEBUG("new_actuate_ms_from_closed %d, new_actuate_ms_from_open %d, new_actuate_ms %d, delta %d",
                    (int) new_actuate_ms_from_closed, (int) new_actuate_ms_from_open, (int)new_actuate_ms, delta)

                if (_this->next_actuate_ref == 0 || _this->next_actuate_ref == 100)
                {
                    DEBUG("will restart from %d due to explicit command", (int) _this->next_actuate_ref)
                    should_restart = true;
                }
                else if (_this->actuate_ms >= open_time || _this->actuate_add_ups >= _this->valve_profile.max_actuate_add_ups)
                {
                    DEBUG("will restart due to invalid actuate_ms or add-up threshold")
                    should_restart = true;
                }
                else if (current_value <= 10 || current_value >= 90)
                {
                    DEBUG("will restart due to value is too close to an end-state")
                    // if value is close enough to an end-state - dont bother with add-up, restart and go from there 
                    should_restart = true;
                }
                else
                {
                    #ifdef ASYMMETRICAL_OPEN_CLOSE

                    // if we have asymmetrical times closed-2-open and open-2-closed then we should 
                    // only continue if delta is > 0

                    if (delta < 0)
                    {
                        should_restart = true;
                        DEBUG("will restart due to negative delta")
                    }                
                    # endif // ASYMMETRICAL_OPEN_CLOSE
                }
            }

            size_t timeout_seconds = 30;

            if (_this->valve_profile.open_time != 0)
            {
                timeout_seconds = _this->valve_profile.open_time * 2;    
            }

            bool is_error = false;
            const char * error_str = "";
            Averager<float, 5> current_averager;

            if (should_restart == true)
            {
                // we need to reset to a nearest end-state and start from there;

                uint8_t go_through = 0;

                if (_this->next_actuate_ref == 0 || _this->next_actuate_ref == 100)
                {
                    go_through = _this->next_actuate_ref;
                    _this->next_actuate_ref = UINT8_MAX;
                }
                else if (_this->actuate_ms >= open_time)
                {
                    go_through = new_actuate_ms_from_closed < new_actuate_ms_from_open ? 0 : 100;
                }
                /*else if (current_value <= 10)
                {
                    go_through = 0;
                }
                else if (current_value >= 90)
                {
                    go_through = 100;
                }*/
                else
                {
                    // select nearest by minimum of go-time sum : go-to-end-state + go-to-new-value

                    uint32_t go_time_to_closed = _this->actuate_ref == 0 ? _this->actuate_ms : (open_2_closed_time -_this->actuate_ms);
                    uint32_t go_time_to_open = _this->actuate_ref == 100 ? _this->actuate_ms : (closed_2_open_time-_this->actuate_ms);

                    uint32_t go_time_through_closed = go_time_to_closed + new_actuate_ms_from_closed;
                    uint32_t go_time_through_open = go_time_to_open + new_actuate_ms_from_open;

                    go_through = go_time_through_closed < go_time_through_open ? 0 : 100;
                }

                new_actuate_ms = go_through == 0 ? new_actuate_ms_from_closed : new_actuate_ms_from_open;                    
                delta = new_actuate_ms;

                TRACE("Will actuate going through state %d, go-time is %d ms", (int) go_through, (int) new_actuate_ms)                    
                TRACE("Resetting to end-state %d", (int) go_through)                    

                uint32_t t_begin = millis();
                DigitalInputChannelConfig wait_on;

                if (go_through == 100)
                {
                    wait_on = _this->config.open;
                }
                else
                {
                    wait_on = _this->config.closed;
                }

                bool ready = read(wait_on);

                if (ready == false)
                {
                    if (go_through == 100)
                    {
                        write_one_2_open(_this->config);
                    }
                    else
                    {
                        write_one_2_closed(_this->config);
                    }
                }

                size_t c_count = 0;

                while(_this->_is_active == true)
                {
                    if (ready == true)
                    {
                        break;
                    }

                    delay(1);
                    ready = read(wait_on);

                    if (c_count % 200 == 0)
                    {
                        unsigned long t_probe = millis();
                        unsigned long time_passed = 0;

                        if (t_begin <= t_probe)
                        {
                            time_passed = t_probe - t_begin;
                        }
                        else
                        {
                            time_passed = t_probe + (ULONG_MAX - t_begin);
                        }

                        if (time_passed > timeout_seconds * 1000)
                        {
                            ERROR("actuation timeout detected, t_begin %d, t_probe %d", (int) t_begin, (int) t_probe)
                            is_error = true;
                            error_str = "actuation error: timeout at resetting to end-state";
                            break;
                        }

                        float current = read_current(_this->config.load_detect);
                        current_averager.push(current);

                        if (current_averager.is_window_full())
                        {
                            float average_current = current_averager.get_average(0);
                            TRACE("average current %f", average_current)

                            if (average_current < _this->config.load_detect.current_threshold)
                            {                        
                                ERROR("actuation no load detected, measured current %f, configured threshold %f", 
                                    current, _this->config.load_detect.current_threshold)
                                is_error = true;
                                error_str = "actuation error: no load";
                                break;
                            }
                        }
                    }
                    
                    c_count ++;
                }

                write_one_stop(_this->config);
                
                if (is_error == true || _this->_is_active == false)
                {
                    if  (_this->_is_active == false)
                    {
                        error_str = "actuation error: aborted (is_active==false)";
                    }
                    
                    _this->status.error = error_str;
                    ERROR(error_str)

                    _this->actuate_ref = UINT8_MAX;
                }
                else
                {
                    TRACE("End-state %d reached", (int) go_through)                    

                    _this->actuate_ref = go_through;    
                    _this->status.error.clear();

                    delay(1000); // let the motor stop
                }

                _this->actuate_ms = 0;
                _this->actuate_add_ups = 0;
            }
            else
            {
                TRACE("Will actuate from current ref, delta %d ms", (int) delta)
            }

            if (_this->_is_active == true && is_error == false)
            {
                TRACE("Adding up delta %d ms", (int) delta)                    

                // continue with add-up

                if (abs(delta)  <= 50)
                {                
                    TRACE("Delta is too short - do nothing")                    

                    // do nothing since we cannot get a predictable motor movement for this time (50ms)
                    // neither update actuate_ms and actuate_add_ups
                }
                else
                {
                    read_current(_this->config.load_detect);

                    uint32_t t_begin = millis();

                    if (_this->actuate_ref == 100)
                    {
                        if (delta > 0)
                        {
                            write_one_2_closed(_this->config);
                        }
                        else
                        {
                            write_one_2_open(_this->config);
                        }
                    }
                    else
                    {
                        if (delta > 0)
                        {
                            write_one_2_open(_this->config);
                        }
                        else
                        {
                            write_one_2_closed(_this->config);
                        }
                    }

                    read_current(_this->config.load_detect);

                    size_t c_count = 0;

                    while(_this->_is_active == true)
                    {
                        delay(1);

                        unsigned long t_probe = millis();
                        unsigned long time_passed = 0;

                        if (t_begin <= t_probe)
                        {
                            time_passed = t_probe - t_begin;
                        }
                        else
                        {
                            time_passed = t_probe + (ULONG_MAX - t_begin);
                        }

                        if (time_passed >= abs(delta))
                        {
                            break;
                        }

                        if (c_count % 200 == 0)
                        {
                            float current = read_current(_this->config.load_detect);
                            current_averager.push(current);

                            if (current_averager.is_window_full())
                            {
                                float average_current = current_averager.get_average(0);
                                TRACE("average current %f", average_current)

                                if (average_current < _this->config.load_detect.current_threshold)
                                {
                                ERROR("actuation no load detected, measured current %f, configured threshold %f", 
                                    current, _this->config.load_detect.current_threshold)
                                is_error = true;
                                error_str = "actuation error: no load";
                                break;
                                }
                            }
                        }

                        c_count++;
                    }

                    read_current(_this->config.load_detect);

                    write_one_stop(_this->config);
                    
                    if (is_error == true || _this->_is_active == false)
                    {
                        if (_this->_is_active == false)
                        {
                            error_str = "actuation error: aborted (is_active==false)";
                        }

                        _this->status.error = error_str;
                        ERROR(error_str)

                        _this->actuate_ms = UINT32_MAX;
                    }
                    else
                    {
                        _this->actuate_ms += delta;
                        _this->actuate_add_ups++;

                        _this->status.error.clear();
                    }
                }
            }
            
            _this->status.actuate_add_ups = _this->actuate_add_ups;

            TRACE("after: actuate_ref %d, actuate_ms %d, actuate_add_ups %d next_actuate_ref %d", (int) _this->actuate_ref,
                (int) _this->actuate_ms, (int) _this->actuate_add_ups, (int) _this->next_actuate_ref)

            if (current_value != _this->status.value)
            {
                TRACE("actuation value has changed while actuation task was ongoing (current value %d, new value %d), "
                    "restarting the task", (int) current_value, (int) _this->status.value)

                continue; // while(1)    
            }
        }
        else
        {
            const char * error_str = "actuation error: invalid target open time (calibrate?)";
            _this->status.error = error_str;
            ERROR(error_str)
        }

        break;

    } // while(1)

    _this->status.state = ProportionalStatus::Channel::sIdle;
    TRACE("actuation_task: terminated, status %s", _this->status.as_string().c_str())
    vTaskDelete(NULL);
}

uint32_t ChannelHandler::flow_2_time(const ProportionalConfig::ValveProfile & _valve_profile,  
                                     int flow_percent, uint32_t open_time, uint8_t from)
{
    if (flow_percent < 0)
    {
        flow_percent = 0;
    }

    if (flow_percent > 100)
    {
        flow_percent = 100;
    }

    if (flow_percent == 0 || flow_percent == 100)
    {
        // do this special handling even if the flow table is present to ensure end states
        // are ancored to the actual end states on the valve

        DEBUG("flow_2_time for flow_percent %d is called for an end state", flow_percent)

        uint32_t r = 0;

        if (from == 0)
        {
            r = flow_percent == 0 ? 0 : open_time;
        }
        else
        {
            r = flow_percent == 0 ? open_time : 0;
        }

        DEBUG("returning end state value %d (open_time %d, from %d)", (int) r, (int) open_time, (int) from)
        return r;
    }
    else
    {
        if (_valve_profile.time_2_flow_rate.empty() == true)
        {
            DEBUG("flow_2_time for flow_percent %d is called with empty time_2_flow_rate_table", flow_percent)

            uint32_t r = 0;

            if (from == 0)
            {
                r = (flow_percent * open_time)/100;
            }
            else
            {
                r = ((100-flow_percent) * open_time)/100;
            }

            DEBUG("returning linear approximation %d (open_time %d, from %d)", (int) r, (int) open_time, (int) from)
            return r;
        }
        else
        {
            DEBUG("flow_2_time for flow_percent %d is called with NON-empty time_2_flow_rate_table", flow_percent)

            // the profile is prepended so that the last item's flow should always be 100

            uint8_t prev_flow = 0;
            uint8_t prev_time = 0;

            for (size_t i=0; i<_valve_profile.time_2_flow_rate.size(); ++i)
            {
                uint8_t this_flow = _valve_profile.time_2_flow_rate[i].second;
                uint8_t this_time = _valve_profile.time_2_flow_rate[i].first;

                // skip intervals where prev_time >= this_time or prev_flow >= this_flow
                //
                // this is a protection in case the table begins like this:
                // [0,0], [15,0], ... which is usually the case for motorized which start to open after
                // some go-time 

                if (this_flow > prev_flow && this_time > prev_time)
                {                        
                    if (flow_percent >= prev_flow && flow_percent <= this_flow)
                    {
                        float flow_span = this_flow - prev_flow;
                        float time_span = this_time - prev_time;

                        float k = float(flow_percent - prev_flow)/flow_span;
                    
                        uint8_t time_percent = uint8_t(prev_time + time_span * k);

                        uint32_t r = 0;

                        if (from == 0)
                        {
                            r = (time_percent * open_time)/100;
                        }
                        else
                        {
                            r = ((100-time_percent) * open_time)/100;
                        }

                        DEBUG("returning span approximation %d percent / %d ms on span %d (open_time %d, from %d)", 
                            (int) time_percent, (int) r, (int) i, (int) open_time, (int) from)

                        return r;
                    }
                }

                prev_flow = this_flow;
                prev_time = this_time;
            }        
        }

        // we shouldn't get here
    }

    return 0;
}

void ProportionalHandler::start(const ProportionalConfig &_config)
{
    TRACE("starting proportional handler")
    //Serial.write("DIRECT: starting proportional handler");

    if (_is_active)
    {
        ERROR("proportional handler already running")
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    config = _config;

    configure_channels();

    if (read_data() == false)
    {
        calibrate_uncalibrated();
    }

    // update_valve_profiles();  // already included in configure_channels

    _is_active = true;
    _is_finished = false;

    TRACE("starting proportional handler task")

    xTaskCreate(
        task,                // Function that should be called
        "proportional_task", // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void ProportionalHandler::stop()
{
    TRACE("stopping proportional handler")

    if (_is_active)
    {
    }

    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }

    action_queue.clear(); // just in case
    delete_all_channel_handlers();
}

void ProportionalHandler::reconfigure(const ProportionalConfig &_config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("proportional_task: config changed")

        bool should_configure_channels = config.channels == _config.channels  ? false : true;
        bool should_update_valve_profiles = config.valve_profiles == _config.valve_profiles  ? false : true;

        // do some fine-tuning to find out what needs to be recalibrated and reactuated; this is 
        // to minimize excess wear of the valves

        std::vector<bool> will_calibrate;
        std::vector<bool> will_actuate;

        for (size_t i=0; i<config.channels.size() && i<_config.channels.size(); ++i)
        {
            bool _will_calibrate = config.channels[i].will_need_calibrate(_config.channels[i]);
            will_calibrate.push_back(_will_calibrate);
            DEBUG("will calibrate channel %d", (int) i)

            bool _will_actuate = false;

            // re-actuate if there is a chance the valve profile has changed

            auto _selected_valve_profile_it = _config.valve_profiles.find(_config.channels[i].valve_profile);

            if (_selected_valve_profile_it != _config.valve_profiles.end())
            {
                auto selected_valve_profile_it = config.valve_profiles.find(_config.channels[i].valve_profile);

                if (selected_valve_profile_it != config.valve_profiles.end())
                {
                    _will_actuate = !(*_selected_valve_profile_it  == *selected_valve_profile_it);
                }
                else
                {
                    _will_actuate = true;
                }
            }
            else
            {
                _will_actuate = true;
            }

            _will_actuate = _will_actuate || config.channels[i].will_need_actuate(_config.channels[i]);
            will_actuate.push_back(_will_actuate);
            DEBUG("will actuate channel %d", (int) i)
        }

        config = _config;

        if (should_configure_channels)
        {
            configure_channels();
        }
        else  // else because updating valve profiles is part of configure_channels as well
        {
            if (should_update_valve_profiles)
            {
                update_valve_profiles();
            }
        }

        for (size_t i=0; i<config.channels.size(); ++i)
        {
            if (will_calibrate[i])
            {
                calibrate(i);
            }

            if (will_actuate[i])
            {
                actuate(i, channel_handlers[i]->get_value(), UINT8_MAX, true);
            }
        }
    }
}

void ProportionalHandler::task(void *parameter)
{
    ProportionalHandler *_this = (ProportionalHandler *)parameter;

    TRACE("proportional_task: started")

    const size_t SAVE_DATA_INTERVAL = 10; // seconds
    unsigned long last_save_data_millis = millis();

    while (_this->_is_active)
    {
        #ifdef USE_ACTION_QUEUE

        { Lock lock(_this->semaphore);

        if (_this->action_queue.empty() == false)
        {
            bool all_are_idle = true;

            for (auto it=_this->channel_handlers.begin(); it!=_this->channel_handlers.end(); ++it)
            {
                if ((*it)->is_idle() == false)
                {
                    all_are_idle = false;
                    break;
                }
            }

            if (all_are_idle == true)
            {
                auto action = _this->action_queue.front();
                _this->action_queue.pop_front();

                DEBUG("proportional task pop action from queue: channel %d, action %d, value %d, ref %d",
                      (int) action.first, (int) action.second.action, (int) action.second.value, 
                      (int) action.second.ref)

                if (action.first >= 0 && action.first < _this->channel_handlers.size())
                {
                    if (action.second.action == ActionAndValue::aCalibrate)
                    {
                        _this->channel_handlers[action.first]->calibrate();
                    }
                    else if (action.second.action == ActionAndValue::aActuate)
                    {
                        _this->channel_handlers[action.first]->actuate(action.second.value, action.second.ref, action.second.force);
                    }
                }
            }
        }}

        #endif // USE_ACTION_QUEUE

        unsigned long now_millis = millis();

        if (now_millis < last_save_data_millis || 
            (now_millis-last_save_data_millis)/1000 >= SAVE_DATA_INTERVAL)
        {
            last_save_data_millis = now_millis;

            if (_this->does_data_need_save())
            {
                TRACE("saving proportional data to EPROM")
                _this->save_data();
                _this->data_saved();
            }
        }

        delay(1000);
    }

    _this->action_queue.clear();
    _this->_is_finished = true;

    TRACE("proportional_task: terminated")
    vTaskDelete(NULL);
}

void ProportionalHandler::delete_all_channel_handlers()
{
    for (auto it=channel_handlers.begin(); it!=channel_handlers.end(); ++it)
    {
        (*it)->stop();
        delete *it;
    }

    channel_handlers.clear();
}

void ProportionalHandler::configure_channels()
{
    TRACE("configure channels")

    // Lock lock(semaphore);
    
    action_queue.clear();

    // for channels - reuse existing channel handlers, do not kill and re-create

    size_t cc = config.channels.size() < channel_handlers.size() ? config.channels.size() : channel_handlers.size();

    auto it=channel_handlers.begin();  

    ProportionalConfig::ValveProfile default_valve_profile;
    
    // configure matching count of channels / channel configs

    if (cc > 0)    
    {
        TRACE("will reuse %d channels",(int) cc)

        size_t i = 0;
        auto dit = config.channels.begin();
        
        for (; i<cc; ++it, ++dit, ++i)
        {
            if (!((*it)->get_config() == *dit))
            {
                (*it)->reconfigure(*dit);

                // done in data_from_eprom
                /*
                #ifdef USE_ACTION_QUEUE

                DEBUG("push action to queue: channel %d, action aCalibrate", (int) i)
                action_queue.push_back(std::make_pair(i, ActionAndValue(ActionAndValue::aCalibrate)));

                #else

                (*it)->calibrate();

                #endif // USE_ACTION_QUEUE
                */
            }

            //DEBUG("Checking valve profile")

            if (!dit->valve_profile.isEmpty())
            {
                //DEBUG("valve profile not empty, %s",  dit->valve_profile.c_str())

                auto vp = config.valve_profiles.find(dit->valve_profile);

                if (vp != config.valve_profiles.end())
                {
                    TRACE("setting valve profile %s on channel %i", (*vp).first.c_str(), (int) i)
                    (*it)->set_valve_profile((*vp).second);
                    continue;
                }
            }

            TRACE("setting default valve profile on channel %i", (int) i)
            (*it)->set_valve_profile(default_valve_profile);
        }
    }

    // note: the it will be used later in removing, if necessary

    // add more channels if necessary

    size_t mc = config.channels.size() > channel_handlers.size() ? config.channels.size()-channel_handlers.size() : 0;

    if (mc > 0)
    {
        TRACE("will add  %d channels",(int) mc)

        for (size_t i=0; i<mc; ++i)
        {
            ProportionalConfig::ValveProfile valve_profile;

            if (!config.channels[i+cc].valve_profile.isEmpty())
            {
                auto vp = config.valve_profiles.find(config.channels[i+cc].valve_profile);

                if (vp != config.valve_profiles.end())
                {
                    TRACE("setting valve profile %s on channel %i", (*vp).first.c_str(), (int) i)
                    valve_profile = (*vp).second;
                }
            }

            // if no match for valve_profile, the one in a newly created channel is the default

            ChannelHandler * channel_handler = new ChannelHandler();
            channel_handler->start(config.channels[i+cc], valve_profile); 
            channel_handlers.push_back(channel_handler);

            // done in data_from_eprom
            /*
            #ifdef USE_ACTION_QUEUE

            DEBUG("push action to queue: channel %d, action aCalibrate", (int) i)
            action_queue.push_back(std::make_pair(i, ActionAndValue(ActionAndValue::aCalibrate)));

            #else

            (channel_handler)->calibrate();

            #endif // USE_ACTION_QUEUE
            */        }
    }

    // delete unused channels 

    size_t lc = config.channels.size() < channel_handlers.size() ? channel_handlers.size()-config.channels.size() : 0;

    if (lc > 0)
    {
        TRACE("will delete  %d channels",(int) lc)

        for (size_t i=0; i<lc; ++i)
        {
            ChannelHandler * channel_handler = channel_handlers[cc+i];

            channel_handler->stop();
            delete channel_handler;
        }
        channel_handlers.erase(it, it+lc);
    }
}

void ProportionalHandler::update_valve_profiles()
{    
    TRACE("update_valve_profiles")

    size_t i=0;
    ProportionalConfig::ValveProfile default_valve_profile;

    for (auto it=channel_handlers.begin(); it!=channel_handlers.end(); ++it, ++i)        
    {
        auto channel_config = (*it)->get_config();

        if (!channel_config.valve_profile.isEmpty())
        {
            auto vp = config.valve_profiles.find(channel_config.valve_profile);

            if (vp != config.valve_profiles.end())
            {
                TRACE("setting valve profile %s on channel %i", (*vp).first.c_str(), (int) i)
                (*it)->set_valve_profile((*vp).second);
                continue;
            }
        }

        TRACE("setting default valve profile on channel %i", (int) i)
        (*it)->set_valve_profile(default_valve_profile);
    }
}

void ProportionalHandler::calibrate_uncalibrated() 
{
    TRACE("ProportionalHandler calibrate_uncalibrated")

    Lock lock(semaphore);

    for (size_t i=0; i<channel_handlers.size(); ++i)
    {
        if (channel_handlers[i]->has_calib_data() == false)
        {
            calibrate(i);
        }
    }     
}

String ProportionalHandler::calibrate(size_t channel)
{
    TRACE("calibrate channel %d", (int) channel)
    String r;

    Lock lock(semaphore);

    if (channel >= 0 && channel < channel_handlers.size())
    {
        #ifdef USE_ACTION_QUEUE

        DEBUG("push action to queue: channel %d, action aCalibrate", (int) channel)
        action_queue.push_back(std::make_pair(channel, ActionAndValue(ActionAndValue::aCalibrate)));

        #else

        return channel_handlers[channel]->calibrate();

        #endif // USE_ACTION_QUEUE
    }
    else
    {
        r = "channel out of range";
    }

    return r;
}

String ProportionalHandler::actuate(size_t channel, uint8_t value, uint8_t ref, bool force)
{
    TRACE("actuate channel %d value %d ref %d", (int) channel, (int) value, (int) ref)
    String r;

    Lock lock(semaphore);

    if (channel >= 0 && channel < channel_handlers.size())
    {
        #ifdef USE_ACTION_QUEUE

        DEBUG("push action to queue: channel %d, action aActuate, value %d, ref %d, force %d", (int) channel, (int) value, (int) ref, (int) force)
        action_queue.push_back(std::make_pair(channel, ActionAndValue(ActionAndValue::aActuate, value, ref, force)));

        #else

        return channel_handlers[channel]->actuate(value, ref, force);

        #endif // USE_ACTION_QUEUE
    }
    else
    {
        r = "channel out of range";
    }

    return r;
}

bool ProportionalHandler::does_data_need_save() 
{
    Lock lock(semaphore);

    for (auto it=channel_handlers.begin(); it!=channel_handlers.end(); ++it)
    {
        if ((*it)->does_calib_data_need_save() || (*it)->does_value_need_save())
        {
            return true;
        }
    }

    return false;
}

void ProportionalHandler::data_saved() 
{
    Lock lock(semaphore);

    for (auto it=channel_handlers.begin(); it!=channel_handlers.end(); ++it)
    {
        (*it)->calib_data_saved();
        (*it)->value_saved();
    }
}

void ProportionalHandler::data_to_eprom(std::ostream &os) 
{
    Lock lock(semaphore);

    DEBUG("ProportionalHandler data_to_eprom")

    uint8_t eprom_version = (uint8_t)DATA_EPROM_VERSION;
    os.write((const char *)&eprom_version, sizeof(eprom_version));

    size_t count = channel_handlers.size();

    os.write((const char *)&count, sizeof(count));

    DEBUG("count %d", (int) count)

   for (auto it = channel_handlers.begin(); it != channel_handlers.end(); ++it)
    {
        (*it)->calib_data_to_eprom(os);
        uint8_t value = (*it)->get_value();
        os.write((const char *)&value, sizeof(value));
        DEBUG("value %d", (int) value)
    }
}

bool ProportionalHandler::data_from_eprom(std::istream &is)
{
    uint8_t eprom_version = DATA_EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    DEBUG("ProportionalHandler data_from_eprom")

    std::vector<uint8_t> values;

    if (eprom_version == DATA_EPROM_VERSION)
    {
        DEBUG("Version match")
        size_t count = 0;
        is.read((char *)&count, sizeof(count));

        DEBUG("count %d", (int) count)
        
        for (size_t i=0; i<count; ++i)
        {
            if (i < channel_handlers.size())
            {
                channel_handlers[i]->calib_data_from_eprom(is);
            }
            else
            {
                ChannelHandler::dummy_calib_data_from_eprom(is);
            }

            uint8_t value = 100;
            is.read((char *)&value, sizeof(value));
            values.push_back(value);
            DEBUG("value %d", (int) value)
        }
    }
    else
    {
        ERROR("Failed to read proportional data from EPROM: version mismatch, expected %d, found %d", (int)DATA_EPROM_VERSION, (int)eprom_version)
        TRACE("Will continue with calibration anyway")
    }


    Lock lock(semaphore);

    DEBUG("calibrating uncalibrated")

    for (size_t i=0; i<channel_handlers.size(); ++i)
    {
        if (channel_handlers[i]->has_calib_data() == false)
        {
            calibrate(i);
        }
    } 

    DEBUG("actuating to values read")

    for (size_t i=0; i<values.size() && i<channel_handlers.size(); ++i)
    {
        actuate(i, values[i], UINT8_MAX, true);
    } 

    return !is.bad();
}


bool ProportionalHandler::read_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);

    TRACE("ProportionalHandler reading data from EEPROM")

    if (dataVolume.read() == true)
    {
        for (auto it = dataVolume.blocks.begin(); it != dataVolume.blocks.end(); ++it)
        {
            if(it->first == ftProportional)
            {
                const char * function_type_str = function_type_2_str((FunctionType) it->first);
                TRACE("Found block type for function %s", function_type_str)

                std::istringstream is(it->second);

                if (data_from_eprom(is) == true)
                {
                    TRACE("Proportional data read success")
                    return true;
                }
                else
                {
                    TRACE("Proportional data read failure")
                }
            }
        }
    }
    else
    {
        // this is not necessarily a fault, maybe no data yet
    
        ERROR("Cannot read EEPROM image (data), no data yet?")
    }
    
    return false;
}

void ProportionalHandler::save_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);
    dataVolume.read();

    std::ostringstream os;

    TRACE("Saving proportional data to EEPROM")
    data_to_eprom(os);

    std::string buffer = os.str();
    TRACE("block size %d", (int) os.tellp())
    
    if (dataVolume.blocks.find((uint8_t) ftProportional) == dataVolume.blocks.end())
    {
        dataVolume.blocks.insert({(uint8_t) ftProportional, buffer});
    }
    else
    {
        if (dataVolume.blocks[(uint8_t) ftProportional] == buffer)
        {
            TRACE("Data identical, skip saving")
            return;
        }
        else
        {
            dataVolume.blocks[(uint8_t) ftProportional] = buffer;
        }
    }
    
    if (dataVolume.write())
    {
        TRACE("Proportional data save success")
    }
    else
    {
        TRACE("Proportional data save failure")
    }
}

void start_proportional_task(const ProportionalConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start proportional_task while it is running, redirecting to reconfigure")
        reconfigure_proportional(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_proportional_task()
{
    handler.stop();
}

ProportionalStatus get_proportional_status()
{
    return handler.get_status();
}

void reconfigure_proportional(const ProportionalConfig &_config)
{
    handler.reconfigure(_config);
}

String proportional_calibrate(const String & channel_str)
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

            if (channel >= 0 && channel < handler.get_num_channels())
            {
                return handler.calibrate(channel);
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

bool __is_number_or_empty(const String & value)
{
    for (size_t i=0; i<value.length(); ++i)
    {
        if (isdigit(value[i]) == false)
        {
            return false;
        } 
    }
    return true;
}

String proportional_actuate(const String & channel_str, const String & value_str, const String & ref_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty() && !value_str.isEmpty())
    {
        param_ok = __is_number_or_empty(channel_str) && __is_number_or_empty(value_str) && 
                   __is_number_or_empty(ref_str);

        size_t channel = (size_t)  channel_str.toInt();
        uint8_t value = (uint8_t)  value_str.toInt();
        
        uint8_t ref = UINT8_MAX;

        if (!ref_str.isEmpty())
        {
            ref = (uint8_t)  ref_str.toInt();
        }        

        DEBUG("validating channel number, get_num_channels %d", (int)handler.get_num_channels())
        if (channel >= 0 && channel < handler.get_num_channels())
        {
            if (value >= 0 && value <= 100)
            {
                if (!(ref == 0 || ref == 100 || ref == UINT8_MAX))
                {
                    return "Optional ref could only be 0 or 100"; 
                }
                else
                {
                    return handler.actuate(channel, value, ref);
                }
            }
            else
            {
                return "Value out of range"; 
            }
        }
        else
        {
            return "Channel out of range"; 
        }
    }
    else
    {
        param_ok = false;
    }
    
    return "Parameter error";
}

#endif // INCLUDE_PROPORTIONAL
