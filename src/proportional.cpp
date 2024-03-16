#ifdef INCLUDE_PROPORTIONAL
#include <ArduinoJson.h>
#include <proportional.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <Wire.h>
#include <deque>

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

bool ProportionalConfig::is_valid() const
{
    GpioCheckpad checkpad;

    size_t i = 0;

    for (auto it = channels.begin(); it != channels.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
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


class ChannelHandler
{
public:

    ChannelHandler()
    {
        _is_active = false;

        actuate_ref = 255;
        actuate_ms = 0;
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
            status.config_open_time = valve_profile.open_time * 1000; // ms
            _status = status;
        }

        return _status;
    }

    String calibrate();
    String actuate(uint8_t value);

protected:

    void configure_hw();

    static void write_one(const ProportionalConfig::Channel & config, bool a_value, bool b_value);
    static bool read(const DigitalInputChannelConfig & config);

    static void calibration_task(void *parameter);
    static void actuation_task(void *parameter);

    BinarySemaphore semaphore;
    ProportionalConfig::Channel config;
    ProportionalConfig::ValveProfile valve_profile;
    ProportionalStatus::Channel status;
    bool _is_active;

    uint8_t actuate_ref;
    uint32_t actuate_ms;
};

class ProportionalHandler
{
public:

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
            value = 255;
        }

        ActionAndValue(Action _action, uint8_t _value = 255) : action(_action), value(_value)
        {
        }

        Action action;
        uint8_t value;
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
    String actuate(size_t channel, uint8_t value);

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
        // wait for calibration or actuation of proevious active time to abort (running->stop->start situation)

        while(status.state == ProportionalStatus::Channel::sCalibrating || 
                status.state == ProportionalStatus::Channel::sActuating)
        {
            delay(100);
        }

        TRACE("starting channel %s", _config.as_string().c_str())
        
        config = _config;
        
        status.reset();
        status.state = ProportionalStatus::Channel::sIdle;
        status.value = config.default_value;

        configure_hw();

        valve_profile = _valve_profile;

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

    status.reset();
    config.clear();

    TRACE("ChannelHandler::stop returns, status %s", status.as_string().c_str())
}

void ChannelHandler::reconfigure(const ProportionalConfig::Channel &_config)
{
    TRACE("ChannelHandler::reconfigure, status %s", status.as_string().c_str())
    TRACE("config from %s to %s", config.as_string().c_str(),_config.as_string().c_str())

    if (!(config == _config))
    {
        TRACE("channel: config changed")

        _is_active = false;

        while(status.state == ProportionalStatus::Channel::sCalibrating || 
                status.state == ProportionalStatus::Channel::sActuating)
        {
            delay(100);
        }

        config = _config;
        
        status.reset();
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
                8192,                  // Stack size (bytes)
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

String ChannelHandler::actuate(uint8_t value)
{
    TRACE("ChannelHandler::actuate, value %d, status %s", (int) value, status.as_string().c_str())
    String r;

    if (_is_active == true)
    {
        // actuation should be possible at any time
        // if calibration is ongoing - then actuation will always be done afterwards
        // if another actuation is ongoing - then the task will check at the end and restart actuation if the
        // new value is different from its former target value

        {Lock lock(semaphore);
        status.value = value; } 

        // theoretically there could be a slight chance that we fall between the chairs and
        // do not start new actuation at exact moment the one which is ongoing finishes
        // TODO?

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
    else
    {
        r = "Channel not active";
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
}

void ChannelHandler::write_one(const ProportionalConfig::Channel & config, bool a_value, bool b_value)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    GpioChannel::write(config.one_a.gpio, config.one_a.inverted, a_value);
    GpioChannel::write(config.one_b.gpio, config.one_b.inverted, b_value);

    DEBUG("write_one (a=%s, b=%s)", a_value ? "true":"false", b_value ? "true":"false")
}

bool ChannelHandler::read(const DigitalInputChannelConfig & config)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    return GpioChannel::read(config.gpio, config.inverted);
}

void ChannelHandler::calibration_task(void *parameter)
{
    ChannelHandler *_this = (ChannelHandler *)parameter;

    TRACE("calibration_task started, status %s", _this->status.as_string().c_str())

    // try to minimize go-time by either going to another end-state if the motor is in one of the end-states
    // or making to end-state + open-close cycle so that we end up closest to the value 

    size_t timeout_seconds = 30;

    if (_this->status.config_open_time != 0)
    {
        timeout_seconds = _this->status.config_open_time * 2;    
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

    DEBUG("before calibration, open=%d, closed=%d", (int)read(_this->config.open), (int)read(_this->config.closed))

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
                write_one(_this->config, true, false);
            }
            else
            {
                write_one(_this->config, false, true);
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

            if (c_count % 300 == 0)
            {
                unsigned long t_probe = millis();
                unsigned long time_passed = 0;
                if (t_begin < t_probe)
                {
                    time_passed = t_probe - t_begin;
                }
                else
                {
                    time_passed = t_probe + (ULONG_MAX - t_begin);
                }

                if (time_passed > timeout_seconds * 1000)
                {
                    ERROR("calibration timeout detected")
                    break;
                }
            }
            
            c_count ++;
        }

        write_one(_this->config, false, false);

        if (_this->_is_active == false)
        {
            break;
        }
        
        if (ready == false)
        {
            is_error = true;
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

        TRACE("milliseconds %d", (int) milliseconds[pass])
        pass++;
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

        _this->actuate_ref = 255;

        if (is_error == true)
        {
            _this->status.error = "calibration error";
            ERROR("calibration error")
        }
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
    _this->status.state = ProportionalStatus::Channel::sIdle; }

    TRACE("calibration_task: terminated, status %s", _this->status.as_string().c_str())
    vTaskDelete(NULL);
}

void ChannelHandler::actuation_task(void *parameter)
{
    ChannelHandler *_this = (ChannelHandler *)parameter;

    TRACE("actuation_task started, status %s", _this->status.as_string().c_str())

    delay(1000);

    _this->status.state = ProportionalStatus::Channel::sIdle;

    TRACE("actuation_task: terminated, status %s", _this->status.as_string().c_str())
    vTaskDelete(NULL);
}

void ProportionalHandler::start(const ProportionalConfig &_config)
{
    TRACE("starting proportional handler")

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

        config = _config;

        if (should_configure_channels)
        {
            configure_channels();
        }
        else
        {
            if (should_update_valve_profiles)
            {
                update_valve_profiles();
            }
        }
    }
}

void ProportionalHandler::task(void *parameter)
{
    ProportionalHandler *_this = (ProportionalHandler *)parameter;

    TRACE("proportional_task: started")

    uint32_t tmp=0;

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

                DEBUG("proportional task pop action from queue: channel %d, action %d, value %d",
                      (int) action.first, (int) action.second.action, (int) action.second.value)

                if (action.first >= 0 && action.first < _this->channel_handlers.size())
                {
                    if (action.second.action == ActionAndValue::aCalibrate)
                    {
                        _this->channel_handlers[action.first]->calibrate();
                    }
                    else if (action.second.action == ActionAndValue::aActuate)
                    {
                        _this->channel_handlers[action.first]->actuate(action.second.value);
                    }
                }
            }
        }}

        #endif // USE_ACTION_QUEUE


        if (tmp%60 == 0)
        {
            //TRACE("proportional task peep")
        }

        delay(1000);
        tmp++;
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

                #ifdef USE_ACTION_QUEUE

                DEBUG("push action to queue: channel %d, action aCalibrate", (int) i)
                action_queue.push_back(std::make_pair(i, ActionAndValue(ActionAndValue::aCalibrate)));

                #else

                (*it)->calibrate();

                #endif // USE_ACTION_QUEUE
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
                }
            }
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

            ChannelHandler * channel_handler = new ChannelHandler();
            channel_handler->start(config.channels[i+cc], valve_profile); 
            channel_handlers.push_back(channel_handler);

            #ifdef USE_ACTION_QUEUE

            DEBUG("push action to queue: channel %d, action aCalibrate", (int) i)
            action_queue.push_back(std::make_pair(i, ActionAndValue(ActionAndValue::aCalibrate)));

            #else

            (channel_handler)->calibrate();

            #endif // USE_ACTION_QUEUE
        }
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
            }
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

String ProportionalHandler::actuate(size_t channel, uint8_t value)
{
    TRACE("actuate channel %d to value %d", (int) channel, (int) value)
    String r;

    Lock lock(semaphore);

    if (channel >= 0 && channel < channel_handlers.size())
    {
        #ifdef USE_ACTION_QUEUE

        DEBUG("push action to queue: channel %d, action aActuate, value %d", (int) channel, (int) value)
        action_queue.push_back(std::make_pair(channel, ActionAndValue(ActionAndValue::aActuate, value)));

        #else

        return channel_handlers[channel]->actuate(value);

        #endif // USE_ACTION_QUEUE
    }
    else
    {
        r = "channel out of range";
    }

    return r;
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

String proportional_actuate(const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty() && !value_str.isEmpty())
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
            for (size_t i=0; i<value_str.length(); ++i)
            {
                if (isdigit(value_str[i]) == false)
                {
                    param_ok = false;
                    break;
                } 
            }

            if (param_ok)
            {
                size_t channel = (size_t)  channel_str.toInt();
                size_t value = (size_t)  value_str.toInt();

                DEBUG("validating channel number, get_num_channels %d", (int)handler.get_num_channels())
                if (channel >= 0 && channel < handler.get_num_channels())
                {
                    if (value >= 0 && value <= 100)
                    {
                        return handler.actuate(channel, value);
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
        }
    }
    else
    {
        param_ok = false;
    }
    
    return "Parameter error";
}

#endif // INCLUDE_PROPORTIONAL
