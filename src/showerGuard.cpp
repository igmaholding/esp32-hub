#include <ArduinoJson.h>
#include <showerGuard.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <OneWire.h>
#include <DallasTemperature.h>


extern GpioHandler gpioHandler;

void _err_dup(const char * name, int value)
{
    ERROR("%s %d is duplicated / reused", name, value)
}


void _err_cap(const char * name, int value)
{
    ERROR("%s %d, gpio doesn't have required capabilities", name, value)
}


void _err_val(const char * name, int value)
{
    ERROR("%s %d incorrect", name, value)
}


bool ShowerGuardConfig::is_valid() const
{
    bool r = motion.is_valid() && rh.is_valid() && temp.is_valid() && light.is_valid() && fan.is_valid();

    if (r == false)
    {
        return false;
    }

    GpioCheckpad checkpad;

    const char * object_name = "motion.channel.gpio";

    if (checkpad.get_usage(motion.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int) motion.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(motion.channel.gpio, GpioCheckpad::uDigitalInput))
    {
        _err_cap(object_name, (int) motion.channel.gpio);
        return false;
    }
        
    object_name = "rh.vad";

    if (checkpad.get_usage(rh.vad.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int) rh.vad.gpio);
        return false;
    }

    if (!checkpad.set_usage(rh.vad.gpio, GpioCheckpad::uAnalogInput))
    {
        _err_cap(object_name, (int) rh.vad.gpio);
        return false;
    }

    if (checkpad.check_attenuation(rh.vad.atten) == adc_attenuation_t(-1))
    {
        _err_val(object_name, (int) rh.vad.atten);
        return false;
    }

    object_name = "rh.vdd";

    if (checkpad.get_usage(rh.vdd.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int) rh.vdd.gpio);
        return false;
    }

    if (!checkpad.set_usage(rh.vdd.gpio, GpioCheckpad::uAnalogInput))
    {
        _err_cap(object_name, (int) rh.vdd.gpio);
        return false;
    }

    if (checkpad.check_attenuation(rh.vdd.atten) == adc_attenuation_t(-1))
    {
        _err_val(object_name, (int) rh.vdd.atten);
        return false;
    }

    object_name = "temp.channel.gpio";

    if (checkpad.get_usage(temp.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int) temp.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(temp.channel.gpio, GpioCheckpad::uDigitalAll))
    {
        _err_cap(object_name, (int) temp.channel.gpio);
        return false;
    }

    object_name = "light.channel.gpio";

    if (checkpad.get_usage(light.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int) light.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(light.channel.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name, (int) light.channel.gpio);
        return false;
    }

    object_name = "fan.channel.gpio";

    if (checkpad.get_usage(fan.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int) fan.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(fan.channel.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name, (int) fan.channel.gpio);
        return false;
    }

    return true;    
}


void ShowerGuardConfig::from_json(const JsonVariant & json)
{
    if (json.containsKey("motion"))
    {
        const JsonVariant & _json = json["motion"];
        motion.from_json(_json);
    }

    if (json.containsKey("rh"))
    {
        const JsonVariant & _json = json["rh"];
        rh.from_json(_json);
    }

    if (json.containsKey("temp"))
    {
        const JsonVariant & _json = json["temp"];
        temp.from_json(_json);
    }

    if (json.containsKey("light"))
    {
        const JsonVariant & _json = json["light"];
        light.from_json(_json);
    }

    if (json.containsKey("fan"))
    {
        const JsonVariant & _json = json["fan"];
        fan.from_json(_json);
    }
}


void ShowerGuardConfig::to_eprom(std::ostream & os) const
{
    os.write((const char*) & EPROM_VERSION, sizeof(EPROM_VERSION));
    motion.to_eprom(os);
    rh.to_eprom(os);
    temp.to_eprom(os);
    light.to_eprom(os);
    fan.to_eprom(os);
}


bool ShowerGuardConfig::from_eprom(std::istream & is) 
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *) & eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        motion.from_eprom(is);
        rh.from_eprom(is);
        temp.from_eprom(is);
        light.from_eprom(is);
        fan.from_eprom(is);
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read ShowerGuardConfig from EPROM: version mismatch, expected %d, found %d", (int) EPROM_VERSION, (int) eprom_version)
        return false;
    }
}


void ShowerGuardConfig::Motion::from_json(const JsonVariant & json)
{
    if (json.containsKey("channel"))
    {
        const JsonVariant & _json = json["channel"];
        channel.from_json(_json);
    }
}


void ShowerGuardConfig::Motion::to_eprom(std::ostream & os) const
{
    channel.to_eprom(os);
}


bool ShowerGuardConfig::Motion::from_eprom(std::istream & is) 
{
    channel.from_eprom(is);
    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Motion::Channel::from_json(const JsonVariant & json)
{
    if (json.containsKey("gpio"))
    {
        unsigned gpio_unvalidated = (unsigned)((int) json["gpio"]);
        gpio = GpioChannel::validateGpioNum(gpio_unvalidated);
    } 
    if (json.containsKey("inverted"))
    {
        inverted = json["inverted"];
    }
    if (json.containsKey("debounce"))
    {
        debounce = (unsigned)((int) json["debounce"]);
    }
}


void ShowerGuardConfig::Motion::Channel::to_eprom(std::ostream & os) const
{
    uint8_t gpio_uint8 = (uint8_t) gpio;
    os.write((const char*) & gpio_uint8, sizeof(gpio_uint8));

    uint8_t inverted_uint8 = (uint8_t) inverted;
    os.write((const char*) & inverted_uint8, sizeof(inverted_uint8));

    os.write((const char*) & debounce, sizeof(debounce));
}


bool ShowerGuardConfig::Motion::Channel::from_eprom(std::istream & is) 
{
    int8_t gpio_int8 = (int8_t) -1;
    is.read((char*) & gpio_int8, sizeof(gpio_int8));
    gpio = (gpio_num_t) gpio_int8;

    uint8_t inverted_uint8 = (uint8_t) false;
    is.read((char*) & inverted_uint8, sizeof(inverted_uint8));
    inverted = (bool) inverted_uint8;

    is.read((char*) & debounce, sizeof(debounce));
    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Rh::from_json(const JsonVariant & json)
{
    if (json.containsKey("vad"))
    {
        const JsonVariant & _json = json["vad"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant & __json = _json["channel"];
            vad.from_json(__json);
        }
    }
    if (json.containsKey("vdd"))
    {
        const JsonVariant & _json = json["vdd"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant & __json = _json["channel"];
            vdd.from_json(__json);
        }
    }
    if (json.containsKey("corr"))
    {
        corr = json["corr"];
    }
}


void ShowerGuardConfig::Rh::to_eprom(std::ostream & os) const
{
    vad.to_eprom(os);
    vdd.to_eprom(os);
    os.write((const char*) & corr, sizeof(corr));
}


bool ShowerGuardConfig::Rh::from_eprom(std::istream & is) 
{
    vad.from_eprom(is);
    vdd.from_eprom(is);
    is.read((char*) & corr, sizeof(corr));

    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Rh::Channel::from_json(const JsonVariant & json)
{
    if (json.containsKey("gpio"))
    {
        unsigned gpio_unvalidated = (unsigned)((int) json["gpio"]);
        gpio = GpioChannel::validateGpioNum(gpio_unvalidated);
    }
    if (json.containsKey("atten"))
    {
        atten = (unsigned)((int) json["atten"]);
    }
}


void ShowerGuardConfig::Rh::Channel::to_eprom(std::ostream & os) const
{
    uint8_t gpio_uint8 = (uint8_t) gpio;
    os.write((const char*) & gpio_uint8, sizeof(gpio_uint8));

    os.write((const char*) & atten, sizeof(atten));
}


bool ShowerGuardConfig::Rh::Channel::from_eprom(std::istream & is) 
{
    int8_t gpio_int8 = (int8_t) -1;
    is.read((char*) & gpio_int8, sizeof(gpio_int8));
    gpio = (gpio_num_t) gpio_int8;

    is.read((char*) & atten, sizeof(atten));
    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Temp::from_json(const JsonVariant & json)
{
    if (json.containsKey("channel"))
    {
        const JsonVariant & _json = json["channel"];
        channel.from_json(_json);
    }

    if (json.containsKey("addr"))
    {
        addr = (const char*) json["addr"];
    }

    if (json.containsKey("corr"))
    {
        corr = json["corr"];
    }
}


void ShowerGuardConfig::Temp::to_eprom(std::ostream & os) const
{
    channel.to_eprom(os);

    uint8_t len = addr.length();
    os.write((const char*) & len, sizeof(len));

    if (len)
    {
        os.write(addr.c_str(), len);
    }

    os.write((const char*) & corr, sizeof(corr));
}


bool ShowerGuardConfig::Temp::from_eprom(std::istream & is) 
{
    channel.from_eprom(is);
    uint8_t len = 0;

    is.read((char*) & len, sizeof(len));

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

    is.read((char*) & corr, sizeof(corr));

    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Temp::Channel::from_json(const JsonVariant & json)
{
    if (json.containsKey("gpio"))
    {
        unsigned gpio_unvalidated = (unsigned)((int) json["gpio"]);
        gpio = GpioChannel::validateGpioNum(gpio_unvalidated);
    } 
}


void ShowerGuardConfig::Temp::Channel::to_eprom(std::ostream & os) const
{
    uint8_t gpio_uint8 = (uint8_t) gpio;
    os.write((const char*) & gpio_uint8, sizeof(gpio_uint8));
}


bool ShowerGuardConfig::Temp::Channel::from_eprom(std::istream & is) 
{
    int8_t gpio_int8 = (int8_t) -1;
    is.read((char*) & gpio_int8, sizeof(gpio_int8));
    gpio = (gpio_num_t) gpio_int8;
    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Light::from_json(const JsonVariant & json)
{
    if (json.containsKey("channel"))
    {
        const JsonVariant & _json = json["channel"];
        channel.from_json(_json);
    }

    if (json.containsKey("mode"))
    {
        const char * mode_str = (const char*) json["mode"];
        mode = str_2_mode(mode_str);
    }
    
    if (json.containsKey("linger"))
    {
        linger = (unsigned)((int) json["linger"]);
    }

}


void ShowerGuardConfig::Light::to_eprom(std::ostream & os) const
{
    channel.to_eprom(os);

    uint8_t mode_uint8_t = (uint8_t) mode;
    os.write((const char*) & mode_uint8_t, sizeof(mode_uint8_t));
    os.write((const char*) & linger, sizeof(linger));
}


bool ShowerGuardConfig::Light::from_eprom(std::istream & is) 
{
    channel.from_eprom(is);

    uint8_t mode_uint8 = mAuto;
    is.read((char*) & mode_uint8, sizeof(mode_uint8));
    mode = Mode(mode_uint8);
    is.read((char*) & linger, sizeof(linger));

    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Light::Channel::from_json(const JsonVariant & json)
{
    if (json.containsKey("gpio"))
    {
        unsigned gpio_unvalidated = (unsigned)((int) json["gpio"]);
        gpio = GpioChannel::validateGpioNum(gpio_unvalidated);
    } 
    if (json.containsKey("inverted"))
    {
        inverted = json["inverted"];
    }
    if (json.containsKey("coilon_active"))
    {
        coilon_active = json["coilon_active"];
    }
}


void ShowerGuardConfig::Light::Channel::to_eprom(std::ostream & os) const
{
    uint8_t gpio_uint8 = (uint8_t) gpio;
    os.write((const char*) & gpio_uint8, sizeof(gpio_uint8));

    uint8_t inverted_uint8 = (uint8_t) inverted;
    os.write((const char*) & inverted_uint8, sizeof(inverted_uint8));

    uint8_t coilon_active_uint8 = (uint8_t) coilon_active;
    os.write((const char*) & coilon_active_uint8, sizeof(coilon_active_uint8));
}


bool ShowerGuardConfig::Light::Channel::from_eprom(std::istream & is) 
{
    int8_t gpio_int8 = (int8_t) -1;
    is.read((char*) & gpio_int8, sizeof(gpio_int8));
    gpio = (gpio_num_t) gpio_int8;

    uint8_t inverted_uint8 = (uint8_t) false;
    is.read((char*) & inverted_uint8, sizeof(inverted_uint8));
    inverted = (bool) inverted_uint8;

    uint8_t coilon_active_uint8 = (uint8_t) true;
    is.read((char*) & coilon_active_uint8, sizeof(coilon_active_uint8));
    coilon_active = (bool) coilon_active_uint8;

    return is_valid() && !is.bad();
}


void ShowerGuardConfig::Fan::from_json(const JsonVariant & json)
{
    Light::from_json(json);

    if (json.containsKey("rh_on"))
    {
        rh_on = (uint8_t) (unsigned) json["rh_on"];
    }
    if (json.containsKey("rh_off"))
    {
        rh_off = (uint8_t) (unsigned) json["rh_off"];
    }
}


void ShowerGuardConfig::Fan::to_eprom(std::ostream & os) const
{
    Light::to_eprom(os);
    os.write((const char*) & rh_on, sizeof(rh_on));
    os.write((const char*) & rh_off, sizeof(rh_off));
}


bool ShowerGuardConfig::Fan::from_eprom(std::istream & is) 
{
    Light::from_eprom(is);

    is.read((char*) & rh_on, sizeof(rh_on));
    is.read((char*) & rh_off, sizeof(rh_off));

    return is_valid() && !is.bad();
}



class ShowerGuardAlgo 
{
  public:

        ShowerGuardAlgo()
        {
            init();
        }

        void start(const ShowerGuardConfig & config);
        void stop() {}
        void reconfigure(const ShowerGuardConfig & config);

        void loop_once(float rh, float temp, bool motion);

        bool get_light() const { return light; }
        bool get_fan() const { return fan; }

        const char * get_last_light_decision() const { return last_light_decision; }
        const char * get_last_fan_decision() const { return last_fan_decision; }

  protected:

        void init();

        void reset_rh_window();
        void update_rh_window(float rh); 

        bool is_soft_rh_toggle_down_condition() const;

        bool light;
        bool fan;

        uint32_t last_motion_millis;

        float rh_sliding_window[10];
        size_t rh_sliding_window_pos;

        bool rh_toggle;

        unsigned light_linger;
        unsigned fan_linger;
        unsigned rh_off, rh_on;
        ShowerGuardConfig::Light::Mode light_mode;
        ShowerGuardConfig::Light::Mode fan_mode;

        const char * last_light_decision;
        const char * last_fan_decision;
};


class ShowerGuardHandler 
{
  public:

        static const unsigned TEMP_READ_SLOT = 60;
        static const unsigned RH_READ_SLOT = 10;
        static const unsigned MOTION_HYS = 10;
        static const unsigned LOGGING_SLOT = 60;


        ShowerGuardHandler()
        {
            _is_active = false;
            ds18b20.setOneWire(& ow);
        }

        bool is_active() const { return _is_active; }

        void start(const ShowerGuardConfig & config);
        void stop();
        void reconfigure(const ShowerGuardConfig & config);

        ShowerGuardStatus get_status() const { return status; }

        static unsigned analog_read(uint8_t gpio);


  protected:

        void configure_hw();
        static void configure_hw_rh(const ShowerGuardConfig::Rh & rh);
        void configure_hw_temp();
        static void configure_hw_motion(const ShowerGuardConfig::Motion & motion);
        static void configure_hw_light(const ShowerGuardConfig::Light & light);
        static void configure_hw_fan(const ShowerGuardConfig::Fan & fan);

        static float read_rh(const ShowerGuardConfig::Rh & rh, float temp);
        bool read_temp(float & temp);
        static bool read_motion(const ShowerGuardConfig::Motion & motion);

        static void write_light(const ShowerGuardConfig::Light & light, bool value);
        static void write_fan(const ShowerGuardConfig::Fan & fan, bool value);

        static void task(void * parameter);

        BinarySemaphore semaphore;
        ShowerGuardConfig config;
        ShowerGuardStatus status;
        bool _is_active;

        OneWire ow;
        DallasTemperature ds18b20;

        ShowerGuardAlgo algo;
};


ShowerGuardHandler handler;


void ShowerGuardAlgo::start(const ShowerGuardConfig & config)
{
    init();
    reconfigure(config);
}


void ShowerGuardAlgo::reconfigure(const ShowerGuardConfig & config)
{
    light_linger = config.light.linger;
    fan_linger = config.fan.linger;

    rh_off = config.fan.rh_off;
    rh_on = config.fan.rh_on;
    light_mode = config.light.mode;
    fan_mode = config.fan.mode;

    reset_rh_window();
}


void ShowerGuardAlgo::loop_once(float rh, float temp, bool motion)
{
    // run algo even if there are overrides to fan and light (mode not auto)

    uint32_t now = millis();

    update_rh_window(rh);

    bool motion_light = false;
    bool motion_fan = false;

    if (motion)
    {
        motion_light = true;
        motion_fan = true;
        last_motion_millis = now;
    }
    else
    {
        if (((now - last_motion_millis)/1000) < light_linger)
        {
            motion_light = true;
        }

        if (((now - last_motion_millis)/1000) < fan_linger)
        {
            motion_fan = true;
        }
    }

    light = motion_light;
    last_light_decision = "motion";

    bool last_rh_toggle = rh_toggle;

    if (rh >= rh_on)
    {
        rh_toggle = true;
        last_fan_decision = "rh-high";
    }
    else if (rh <= rh_off)
    {
        rh_toggle = false;
        last_fan_decision = "rh-low";
    }
    else
    {
        if (rh_toggle == true)
        {
            if (is_soft_rh_toggle_down_condition())
            {
                TRACE("Soft rh_toggle condition")
                rh_toggle = false;
               last_fan_decision = "rh-soft-down";
            }

        }
    }

    if (last_rh_toggle != rh_toggle)
    {
        // DEBUG("rh_toggle=%d", (int) rh_toggle)
    }

    if (motion_fan)
    {
        fan = true;
        last_fan_decision = "motion";
    }
    else
    {
        fan = rh_toggle;
    }

    if (light_mode != ShowerGuardConfig::Light::mAuto)
    {
        light = light_mode == ShowerGuardConfig::Light::mOn ? true : false;
        last_light_decision = "nonauto-mode";
    }

    if (fan_mode != ShowerGuardConfig::Light::mAuto)
    {
        fan = fan_mode == ShowerGuardConfig::Fan::mOn ? true : false;
        last_fan_decision = "nonauto-mode";
    }
}


void ShowerGuardAlgo::init()
{
    light = false;
    fan = false;
    last_motion_millis = 0;
    rh_toggle = false;
    reset_rh_window();

    light_linger = 0;
    fan_linger = 0;
    rh_off = 0;
    rh_on = 0;
    light_mode = ShowerGuardConfig::Light::mAuto;
    fan_mode = ShowerGuardConfig::Light::mAuto;
    last_light_decision = "";
    last_fan_decision = "";
}


void ShowerGuardAlgo::reset_rh_window()
{
    //for (int i=0;i<sizeof(rh_avg_window)/sizeof(rh_avg_window[0]);++i)
    //    rh_avg_window[i] = 0;

    // rh_avg_window_pos = 0;

    //for (int i=0;i<sizeof(rh_sliding_window)/sizeof(rh_sliding_window[0]);++i)
    //    rh_sliding_window[i] = 0;

    rh_sliding_window_pos = 0;
}


void ShowerGuardAlgo::update_rh_window(float rh)
{
    // add average to sliding window. if sliding window is full - shift left and add to the end

    if (rh_sliding_window_pos < sizeof(rh_sliding_window)/sizeof(rh_sliding_window[0]))
    {
        rh_sliding_window[rh_sliding_window_pos] = rh;
        rh_sliding_window_pos++;
    }
    else
    {
        for (int i=0;i<sizeof(rh_sliding_window)/sizeof(rh_sliding_window[0])-1;++i)
            rh_sliding_window[i] = rh_sliding_window[i+1];

        rh_sliding_window[sizeof(rh_sliding_window)/sizeof(rh_sliding_window[0])-1] = rh;    
    }
    
    /*
    DEBUG("rh_sliding_window updated")
    String dbg_str;

    for (int i=0;i<sizeof(rh_sliding_window)/sizeof(rh_sliding_window[0]);++i)
    {    
        if (i > 0)
        {
            dbg_str += ",";
        }
        dbg_str += rh_sliding_window[i];
    }
    DEBUG("[%s]", dbg_str.c_str())
    */
}


bool ShowerGuardAlgo::is_soft_rh_toggle_down_condition() const
{
    // current condition for soft toggling down of rh_switch is that the sliding window is fully filled and
    // all its values are under the lowest 10 %of the span rh_off -> rh_on 

    if (rh_sliding_window_pos == sizeof(rh_sliding_window)/sizeof(rh_sliding_window[0])) // sliding window is fully filled
    {
        float upper = rh_off + float(rh_on - rh_off)/10.0;

        for (int i=0;i<sizeof(rh_sliding_window)/sizeof(rh_sliding_window[0]);++i)
        {
            if (rh_sliding_window[i] > upper)
                return false;
        }

        return true;
    }

    return false;
}


void ShowerGuardHandler::start(const ShowerGuardConfig & _config)
{
    if (_is_active)
    {
        return; // already running
    }

    config = _config;
    configure_hw();
    algo.start(config);

    _is_active = true;

    xTaskCreate(
        task,                  // Function that should be called
        "shower_guard_task",   // Name of the task (for debugging)
        2048,                  // Stack size (bytes)
        this,                  // Parameter to pass
        1,                     // Task priority
        NULL                   // Task handle
    );
}


void ShowerGuardHandler::stop()
{
    if (_is_active)
    {
        algo.stop();
    }

    _is_active = false;
}


void ShowerGuardHandler::reconfigure(const ShowerGuardConfig & _config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("shower_guard_task: config changed")
        config = _config;
        configure_hw();
        algo.reconfigure(config);

    }
}


void ShowerGuardHandler::task(void * parameter)
{
    ShowerGuardHandler * _this = (ShowerGuardHandler*) parameter;

    TRACE("shower_guard_task: started")
    
    unsigned temp_read_slot_count = 0;
    unsigned rh_read_slot_count = 0;
    unsigned logging_slot_count = 0;

    float rh = 60.0;

    // we average several samples and feed in algo once per average ready; this is because a considerable fluctuation of readings

    float rh_avg_window[5];
    size_t rh_avg_window_pos = 0;

    float temp = 20.0;

    // here is a short hysteresis loop for motion since the sensor would reset to 0 after its internal timeout even if the 
    // motion continues

    bool fan = false;
    bool light = false;

    bool motion_hys = false;
    bool motion = false;
    unsigned motion_hys_count = 0;

    while(_this->_is_active)
    {
        bool do_algo_loop = false;

        { Lock lock(_this->semaphore);

            if (temp_read_slot_count == 0)
            {
                float last_temp = temp;

                if (_this->read_temp(temp))
                {
                    temp_read_slot_count = TEMP_READ_SLOT-1;
                    //DEBUG("read temp=%f", temp)

                    if (abs(last_temp - temp) >= 0.1)
                    {
                        logging_slot_count = 0;
                    }
                }
                // else try again next slot
            }
            else
            {
                temp_read_slot_count--;
            }

            if (rh_read_slot_count == 0)
            {
                float i_rh = _this->read_rh(_this->config.rh, temp);
                rh_read_slot_count = RH_READ_SLOT-1;
                //DEBUG("read rh=%f", rh)

                rh_avg_window[rh_avg_window_pos] = i_rh;
                rh_avg_window_pos++;

                if (rh_avg_window_pos == sizeof(rh_avg_window)/sizeof(rh_avg_window[0]))
                {
                    rh_avg_window_pos = 0;

                    float new_rh = 0;

                    for (int i=0;i<sizeof(rh_avg_window)/sizeof(rh_avg_window[0]);++i)
                        new_rh += rh_avg_window[i];

                    new_rh /=  sizeof(rh_avg_window)/sizeof(rh_avg_window[0]);
                
                    new_rh = round(new_rh * 10.0) / 10.0;


                    if (abs(new_rh - rh) >= 1.0)
                    {
                        logging_slot_count = 0;
                    }

                    rh = new_rh;
                    do_algo_loop = true;
                }
            }
            else
            {
                rh_read_slot_count--;
            }

            bool last_motion_hys = motion_hys;
            bool last_motion = motion;

            motion = read_motion(_this->config.motion);

            if (motion == 0)
            {
                if (motion_hys_count == 0)
                {
                    motion_hys = false;
                }
                else
                {
                    motion_hys_count--;
                }
            }
            else
            {
                motion_hys = true;
                motion_hys_count = MOTION_HYS-1;
            }

            if (last_motion_hys != motion_hys || last_motion != motion)
            {
                //DEBUG("motion=%d, motion_hys=%d", (int) motion, (int) motion_hys)

                if (last_motion_hys != motion_hys)
                {
                    logging_slot_count = 0;
                    do_algo_loop = true;                    
                }
            }

            // because of rh averaging algo loop will be called regularly; but it can also be called when motion changes
            // this will undermine the idea with sliding window in the algo somewhat but hopefully not affect it too much

            if (do_algo_loop == true)  
            {
                _this->algo.loop_once(rh, temp, motion_hys);
            
                bool last_light = light;
                bool last_fan = fan;

                light = _this->algo.get_light();
                fan = _this->algo.get_fan();

                if (light != last_light)
                {
                    write_light(_this->config.light, light);
                }

                if (fan != last_fan)
                {
                    write_fan(_this->config.fan, fan);
                }

                if (last_light != light || last_fan != fan)
                {
                    //DEBUG("light=%d, fan=%d", (int) light, (int) fan)
                    logging_slot_count = 0;
                }
            }
        }

        _this->status.temp = temp;
        _this->status.rh = rh;
        _this->status.motion = motion_hys;
        _this->status.light = light;
        _this->status.fan = fan;

        if (logging_slot_count == 0)
        {
            logging_slot_count = LOGGING_SLOT;
            TRACE("* {\"temp\":%.1f, \"rh\":%.1f, \"motion\":%d, \"light\":%d (%s), \"fan\":%d (%s)}", temp, rh, (int)motion_hys, 
                  (int)light, _this->algo.get_last_light_decision(), (int)fan, _this->algo.get_last_fan_decision())
        }
        else
        {
            logging_slot_count--;
        }

        delay(1000);
    }

    TRACE("shower_guard_task: terminated")
    vTaskDelete(NULL);
}


void ShowerGuardHandler::configure_hw()
{
    configure_hw_rh(config.rh);
    configure_hw_temp();
    configure_hw_motion(config.motion);
    configure_hw_light(config.light);
    configure_hw_fan(config.fan);
}


void ShowerGuardHandler::configure_hw_rh(const ShowerGuardConfig::Rh & rh)
{
    // configure rh  

    TRACE("configure rh.vad: gpio=%d, atten=%d", (int) rh.vad.gpio, (int) rh.vad.atten)
    analogSetPinAttenuation(rh.vad.gpio, (adc_attenuation_t)rh.vad.atten);
    TRACE("configure rh.vdd: gpio=%d, atten=%d", (int) rh.vdd.gpio, (int) rh.vdd.atten)
    analogSetPinAttenuation(rh.vdd.gpio, (adc_attenuation_t)rh.vdd.atten);
}


void ShowerGuardHandler::configure_hw_temp()
{
    // configure temp  

    TRACE("configure temp: gpio=%d", (int) config.temp.channel.gpio)

    ow.begin(config.temp.channel.gpio);
    ds18b20.setOneWire(& ow);
    ds18b20.setWaitForConversion(true);
    ds18b20.setCheckForConversion(true);
}


void ShowerGuardHandler::configure_hw_motion(const ShowerGuardConfig::Motion & motion)
{
    // configure motion  
    // ignore debounce because we are using polling with low frequency (to skip adding synchronisation in case of interrupts)

    TRACE("configure motion: gpio=%d, inverted=%d", (int) motion.channel.gpio, (int) motion.channel.inverted)
    gpioHandler.setupChannel(motion.channel.gpio, INPUT_PULLUP, motion.channel.inverted, NULL);
}


void ShowerGuardHandler::configure_hw_light(const ShowerGuardConfig::Light & light)
{
    // configure light  

    TRACE("configure light: gpio=%d, inverted=%d", (int) light.channel.gpio, (int) light.channel.inverted)
    gpioHandler.setupChannel(light.channel.gpio, OUTPUT, light.channel.inverted, NULL);
}


void ShowerGuardHandler::configure_hw_fan(const ShowerGuardConfig::Fan & fan)
{
    // configure fan  

    TRACE("configure fan: gpio=%d, inverted=%d", (int) fan.channel.gpio, (int) fan.channel.inverted)
    gpioHandler.setupChannel(fan.channel.gpio, OUTPUT, fan.channel.inverted, NULL);
}


unsigned ShowerGuardHandler::analog_read(uint8_t gpio)
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


float ShowerGuardHandler::read_rh(const ShowerGuardConfig::Rh & rh, float temp)
{
    unsigned vad = analog_read(rh.vad.gpio);
    unsigned vdd = analog_read(rh.vdd.gpio);

    // uncomment this trace to calibrate voltage divider
    //DEBUG("vad %d, vdd %d", (int) vad, (int) vdd)

    if (vdd != 0)
    {

        // the formula with temperatore compensation 
        // (((Vout/VS)-0.1515)/0.00636) / (1.0546-0.00216*T) = (RH )
        
        float r_rh = (((float(vad) / float(vdd)) - 0.1515) / 0.00636)  / (1.0546-0.00216 * temp);

        if (rh.corr != 0)
        {
            //DEBUG("applying non-zero rh correction %f", rh.corr)
            r_rh += rh.corr;
        }

        if (r_rh < 0)
        {
            r_rh = 0;
        }
        else if (r_rh > 100)
        {
            r_rh = 100;
        }

        return r_rh;
    }

    return 0;
}


bool ShowerGuardHandler::read_temp(float & temp)
{
    //ow.reset();
    //ow.reset_search();

    size_t device_count = 0;

    unsigned attempts = 10;

    while(device_count == 0)
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

    TRACE("DS18b20 device count %d", (int) device_count)

    float r_temp = 0;
    bool r = false;

    for (size_t i=0;i<device_count;++i)
    {
        uint8_t addr[8];
        
        if (ds18b20.getAddress(addr, i))
        {
            char addr_str[32];

            sprintf(addr_str, "%02x-%02x%02x%02x%02x%02x%02x", (int) addr[0], (int) addr[6],(int) addr[5],(int) addr[4],(int) addr[3],(int) addr[2],(int) addr[1]);
            float i_temp = ds18b20.getTempC(addr);

            TRACE("addr=[%s], temp=%f", addr_str, i_temp)

            if (!strcmp(addr_str, config.temp.addr.c_str()))
            {
                r_temp = round(i_temp * 10.0) / 10.0;

                if (config.temp.corr != 0)
                {
                    //DEBUG("applying non-zero correction %f", config.temp.corr)
                    r_temp += config.temp.corr;
                }

                TRACE("this is configured device, r_temp=%f", r_temp)

                if ((int) r_temp == -127)
                {
                    ERROR("reading failed with N/A value")
                    r = false;
                }
                else 
                {
                    temp = r_temp;
                    r = true;
                }
            }
        }

    }
    return r;
}


bool ShowerGuardHandler::read_motion(const ShowerGuardConfig::Motion & motion)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    return GpioChannel::read(motion.channel.gpio, motion.channel.inverted);
}


void ShowerGuardHandler::write_light(const ShowerGuardConfig::Light & light, bool value)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    bool value_to_write = light.channel.coilon_active ? value : (value ? false : true);
    GpioChannel::write(light.channel.gpio, light.channel.inverted, value_to_write);
}


void ShowerGuardHandler::write_fan(const ShowerGuardConfig::Fan & fan, bool value)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    bool value_to_write = fan.channel.coilon_active ? value : (value ? false : true);
    GpioChannel::write(fan.channel.gpio, fan.channel.inverted, value_to_write);
}


void start_shower_guard_task(const ShowerGuardConfig & config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start shower_guard_task while it is running, redirecting to reconfigure")
        reconfigure_shower_guard(config);
    }
    else
    {
        handler.start(config);

    }
}


void stop_shower_guard_task()
{
    handler.stop();
}


ShowerGuardStatus get_shower_guard_status()
{
    return handler.get_status();
}



void reconfigure_shower_guard(const ShowerGuardConfig & _config)
{
    handler.reconfigure(_config);
}
