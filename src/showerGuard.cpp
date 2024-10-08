#ifdef INCLUDE_SHOWERGUARD

#include <ArduinoJson.h>
#include <showerGuard.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AHT10.h>
#include <Wire.h>

#define CALIBRATE_RH 0   // HIH5030 only!

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

bool ShowerGuardConfig::is_valid() const
{
    bool r = motion.is_valid() && rh.is_valid() && temp.is_valid() && lumi.is_valid() && light.is_valid() && fan.is_valid();

    if (r == false)
    {
        return false;
    }

    GpioCheckpad checkpad;

    const char *object_name = "motion.channel.gpio";

    if (checkpad.get_usage(motion.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int)motion.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(motion.channel.gpio, GpioCheckpad::uDigitalInput))
    {
        _err_cap(object_name, (int)motion.channel.gpio);
        return false;
    }

    object_name = "rh.vad";

    if (rh.vad.is_valid())
    {
        if (checkpad.get_usage(rh.vad.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rh.vad.gpio);
            return false;
        }

        if (!checkpad.set_usage(rh.vad.gpio, GpioCheckpad::uAnalogInput))
        {
            _err_cap(object_name, (int)rh.vad.gpio);
            return false;
        }

        if (checkpad.check_attenuation(rh.vad.atten) == adc_attenuation_t(-1))
        {
            _err_val(object_name, (int)rh.vad.atten);
            return false;
        }
    }

    object_name = "rh.vdd";

    if (rh.vdd.is_valid())
    {
        if (checkpad.get_usage(rh.vdd.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rh.vdd.gpio);
            return false;
        }

        if (!checkpad.set_usage(rh.vdd.gpio, GpioCheckpad::uAnalogInput))
        {
            _err_cap(object_name, (int)rh.vdd.gpio);
            return false;
        }

        if (checkpad.check_attenuation(rh.vdd.atten) == adc_attenuation_t(-1))
        {
            _err_val(object_name, (int)rh.vdd.atten);
            return false;
        }
    }

    object_name = "rh.sda";

    if (rh.sda.is_valid())
    {
        if (checkpad.get_usage(rh.sda.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rh.sda.gpio);
            return false;
        }

        if (!checkpad.set_usage(rh.sda.gpio, GpioCheckpad::uDigitalAll))
        {
            _err_cap(object_name, (int)rh.sda.gpio);
            return false;
        }
    }

    object_name = "rh.scl";

    if (rh.scl.is_valid())
    {
        if (checkpad.get_usage(rh.scl.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)rh.scl.gpio);
            return false;
        }

        if (!checkpad.set_usage(rh.scl.gpio, GpioCheckpad::uDigitalAll))
        {
            _err_cap(object_name, (int)rh.scl.gpio);
            return false;
        }
    }

    object_name = "lumi.ldr";

    if (lumi.is_configured())
    {
        if (checkpad.get_usage(lumi.ldr.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)lumi.ldr.gpio);
            return false;
        }

        if (!checkpad.set_usage(lumi.ldr.gpio, GpioCheckpad::uAnalogInput))
        {
            _err_cap(object_name, (int)lumi.ldr.gpio);
            return false;
        }

        if (checkpad.check_attenuation(lumi.ldr.atten) == adc_attenuation_t(-1))
        {
            _err_val(object_name, (int)lumi.ldr.atten);
            return false;
        }
    }

    object_name = "temp.channel.gpio";

    if (temp.channel.is_valid())
    {
        if (checkpad.get_usage(temp.channel.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)temp.channel.gpio);
            return false;
        }

        if (!checkpad.set_usage(temp.channel.gpio, GpioCheckpad::uDigitalAll))
        {
            _err_cap(object_name, (int)temp.channel.gpio);
            return false;
        }
    }

    object_name = "light.channel.gpio";

    if (checkpad.get_usage(light.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int)light.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(light.channel.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name, (int)light.channel.gpio);
        return false;
    }

    object_name = "fan.channel.gpio";

    if (checkpad.get_usage(fan.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name, (int)fan.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(fan.channel.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name, (int)fan.channel.gpio);
        return false;
    }

    return true;
}

void ShowerGuardConfig::from_json(const JsonVariant &json)
{
    if (json.containsKey("motion"))
    {
        const JsonVariant &_json = json["motion"];
        motion.from_json(_json);
    }

    if (json.containsKey("rh"))
    {
        const JsonVariant &_json = json["rh"];
        rh.from_json(_json);
    }

    if (json.containsKey("temp"))
    {
        const JsonVariant &_json = json["temp"];
        temp.from_json(_json);
    }

    lumi.clear();

    if (json.containsKey("lumi"))
    {
        const JsonVariant &_json = json["lumi"];
        lumi.from_json(_json);
    }

    if (json.containsKey("light"))
    {
        const JsonVariant &_json = json["light"];
        light.from_json(_json);
    }

    if (json.containsKey("fan"))
    {
        const JsonVariant &_json = json["fan"];
        fan.from_json(_json);
    }
}

void ShowerGuardConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    motion.to_eprom(os);
    rh.to_eprom(os);
    temp.to_eprom(os);
    lumi.to_eprom(os);
    light.to_eprom(os);
    fan.to_eprom(os);
}

bool ShowerGuardConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        motion.from_eprom(is);
        rh.from_eprom(is);
        temp.from_eprom(is);
        lumi.from_eprom(is);
        light.from_eprom(is);
        fan.from_eprom(is);
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read ShowerGuardConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void ShowerGuardConfig::Motion::from_json(const JsonVariant &json)
{
    if (json.containsKey("channel"))
    {
        const JsonVariant &_json = json["channel"];
        channel.from_json(_json);
    }
}

void ShowerGuardConfig::Motion::to_eprom(std::ostream &os) const
{
    channel.to_eprom(os);
}

bool ShowerGuardConfig::Motion::from_eprom(std::istream &is)
{
    channel.from_eprom(is);
    return is_valid() && !is.bad();
}

void ShowerGuardConfig::Rh::from_json(const JsonVariant &json)
{
    if (json.containsKey("hw"))
    {
        const char *hw_str = (const char *)json["hw"];
        hw = str_2_hw(hw_str);
    }
    else
    {
        hw = HW(-1);
    }
    if (json.containsKey("vad"))
    {
        const JsonVariant &_json = json["vad"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            vad.from_json(__json);
        }
    }
    if (json.containsKey("vdd"))
    {
        const JsonVariant &_json = json["vdd"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            vdd.from_json(__json);
        }
    }
    if (json.containsKey("sda"))
    {
        const JsonVariant &_json = json["sda"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            sda.from_json(__json);
        }
    }
    if (json.containsKey("scl"))
    {
        const JsonVariant &_json = json["scl"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            scl.from_json(__json);
        }
    }
    if (json.containsKey("addr"))
    {
        addr = (const char *) json["addr"];
    }
    if (json.containsKey("corr"))
    {
        corr = json["corr"];
    }
}

void ShowerGuardConfig::Rh::to_eprom(std::ostream &os) const
{
    uint8_t hw_uint8_t = (uint8_t)hw;
    os.write((const char *)&hw_uint8_t, sizeof(hw_uint8_t));

    vad.to_eprom(os);
    vdd.to_eprom(os);
    sda.to_eprom(os);
    scl.to_eprom(os);

    uint8_t len = addr.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(addr.c_str(), len);
    }

    os.write((const char *)&corr, sizeof(corr));
}

bool ShowerGuardConfig::Rh::from_eprom(std::istream &is)
{
    uint8_t hw_uint8 = (uint8_t) -1;
    is.read((char *)&hw_uint8, sizeof(hw_uint8));
    hw = HW(hw_uint8);

    vad.from_eprom(is);
    vdd.from_eprom(is);
    sda.from_eprom(is);
    scl.from_eprom(is);

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

void ShowerGuardConfig::Lumi::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("ldr"))
    {
        const JsonVariant &_json = json["ldr"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            ldr.from_json(__json);
        }
    }
    if (json.containsKey("corr"))
    {
        corr = json["corr"];
    }
    if (json.containsKey("threshold"))
    {
        threshold = json["threshold"];
    }
}

void ShowerGuardConfig::Lumi::to_eprom(std::ostream &os) const
{
    ldr.to_eprom(os);
    os.write((const char *)&corr, sizeof(corr));
    os.write((const char *)&threshold, sizeof(threshold));
}

bool ShowerGuardConfig::Lumi::from_eprom(std::istream &is)
{
    ldr.from_eprom(is);
    is.read((char *)&corr, sizeof(corr));
    is.read((char *)&threshold, sizeof(threshold));

    return is_valid() && !is.bad();
}

void ShowerGuardConfig::Temp::from_json(const JsonVariant &json)
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

void ShowerGuardConfig::Temp::to_eprom(std::ostream &os) const
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

bool ShowerGuardConfig::Temp::from_eprom(std::istream &is)
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

void ShowerGuardConfig::Light::from_json(const JsonVariant &json)
{
    if (json.containsKey("channel"))
    {
        const JsonVariant &_json = json["channel"];
        channel.from_json(_json);
    }

    if (json.containsKey("mode"))
    {
        const char *mode_str = (const char *)json["mode"];
        mode = str_2_mode(mode_str);
    }

    if (json.containsKey("linger"))
    {
        linger = (unsigned)((int)json["linger"]);
    }
}

void ShowerGuardConfig::Light::to_eprom(std::ostream &os) const
{
    channel.to_eprom(os);

    uint8_t mode_uint8_t = (uint8_t)mode;
    os.write((const char *)&mode_uint8_t, sizeof(mode_uint8_t));
    os.write((const char *)&linger, sizeof(linger));
}

bool ShowerGuardConfig::Light::from_eprom(std::istream &is)
{
    channel.from_eprom(is);

    uint8_t mode_uint8 = mAuto;
    is.read((char *)&mode_uint8, sizeof(mode_uint8));
    mode = Mode(mode_uint8);
    is.read((char *)&linger, sizeof(linger));

    return is_valid() && !is.bad();
}

void ShowerGuardConfig::Fan::from_json(const JsonVariant &json)
{
    Light::from_json(json);

    if (json.containsKey("rh_on"))
    {
        rh_on = (uint8_t)(unsigned)json["rh_on"];
    }
    if (json.containsKey("rh_off"))
    {
        rh_off = (uint8_t)(unsigned)json["rh_off"];
    }
}

void ShowerGuardConfig::Fan::to_eprom(std::ostream &os) const
{
    Light::to_eprom(os);
    os.write((const char *)&rh_on, sizeof(rh_on));
    os.write((const char *)&rh_off, sizeof(rh_off));
}

bool ShowerGuardConfig::Fan::from_eprom(std::istream &is)
{
    Light::from_eprom(is);

    is.read((char *)&rh_on, sizeof(rh_on));
    is.read((char *)&rh_off, sizeof(rh_off));

    return is_valid() && !is.bad();
}

class ShowerGuardAlgo
{
public:
    ShowerGuardAlgo()
    {
        init();
    }

    void start(const ShowerGuardConfig &config);
    void stop() {}
    void reconfigure(const ShowerGuardConfig &config);

    void loop_once(float rh, float temp, bool motion);

    bool get_light() const { return light; }
    bool get_fan() const { return fan; }

    String get_last_light_decision() const;
    String get_last_fan_decision() const;

    void debug_last_light_decision() const;
    void debug_last_fan_decision() const;

protected:
    void init();

    void reset_rh_window();
    void update_rh_window(float rh);

    bool is_soft_rh_toggle_down_condition() const;

    bool light;
    bool fan;

    bool reset_rh_decision;
    uint32_t last_motion_millis;
    time_t last_motion_time;

    float rh_sliding_window[10];
    size_t rh_sliding_window_pos;

    bool rh_toggle;

    unsigned light_linger;
    unsigned fan_linger;
    unsigned rh_off, rh_on;
    ShowerGuardConfig::Light::Mode light_mode;
    ShowerGuardConfig::Light::Mode fan_mode;

    String last_light_decision[2]; // 2 levels
    String last_fan_decision[3];   // 3 levels
};

class ShowerGuardHandler
{
public:
    static const unsigned TEMP_READ_SLOT = 60;
    static const unsigned RH_READ_SLOT = 10;
    static const unsigned LUMI_READ_SLOT = 5;
    static const unsigned MOTION_HYS = 10;
    static const unsigned LOGGING_SLOT = 60;

    ShowerGuardHandler()
    {
        _is_active = false;
        _is_finished = true;
        aht10 = NULL;
        ds18b20.setOneWire(&ow);
    }

    ~ShowerGuardHandler()
    {
        if (aht10)
        {
            delete aht10;
            aht10 = NULL;
        }
    }

    bool is_active() const { return _is_active; }

    void start(const ShowerGuardConfig &config);
    void stop();
    void reconfigure(const ShowerGuardConfig &config);

    ShowerGuardStatus get_status()
    {
        ShowerGuardStatus _status;

        {
            Lock lock(semaphore);
            _status = status;
        }

        return _status;
    }

    static unsigned analog_read(uint8_t gpio);

protected:
    void configure_hw();
    void configure_hw_rh();
    void configure_hw_temp();
    static void configure_hw_motion(const ShowerGuardConfig::Motion &motion);
    static void configure_hw_lumi(const ShowerGuardConfig::Lumi &lumi);
    static void configure_hw_light(const ShowerGuardConfig::Light &light);
    static void configure_hw_fan(const ShowerGuardConfig::Fan &fan);

    float read_rh(float temp);
    bool read_temp(float &temp);
    static bool read_motion(const ShowerGuardConfig::Motion &motion);
    static float read_lumi(const ShowerGuardConfig::Lumi &lumi);

    static void write_light(const ShowerGuardConfig::Light &light, bool value);
    static void write_fan(const ShowerGuardConfig::Fan &fan, bool value);

    static void task(void *parameter);

    BinarySemaphore semaphore;
    ShowerGuardConfig config;
    ShowerGuardStatus status;
    bool _is_active;
    bool _is_finished;

    OneWire ow;
    DallasTemperature ds18b20;
    AHT10 * aht10;

    ShowerGuardAlgo algo;
};

static ShowerGuardHandler handler;

void ShowerGuardAlgo::start(const ShowerGuardConfig &config)
{
    init();
    reconfigure(config);
}

void ShowerGuardAlgo::reconfigure(const ShowerGuardConfig &config)
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

    char buf[128];

    uint32_t now_millis = millis();

    time_t now_time;
    time(&now_time);

    update_rh_window(rh);

    bool motion_light = false;
    bool motion_fan = false;

    if (reset_rh_decision) // running first time after init
    {
        motion = true; // imitate initial motion to put on everything

        unsigned rh_middle = rh_off + (rh_on - rh_off) / 2;

        if (rh >= rh_middle)
        {
            rh_toggle = true;
        }

        sprintf(buf, "init %.1f/%.1f (middle) at %s", rh, (float)rh_middle, time_t_2_str(now_time).c_str());
        last_fan_decision[0] = buf;

        reset_rh_decision = false;
    }

    if (motion)
    {
        motion_light = true;
        motion_fan = true;
        last_motion_millis = now_millis;
        last_motion_time = now_time;
    }
    else
    {
        if (((now_millis - last_motion_millis) / 1000) < light_linger)
        {
            motion_light = true;
        }

        if (((now_millis - last_motion_millis) / 1000) < fan_linger)
        {
            motion_fan = true;
        }
    }

    if (motion_light == true)
    {
        sprintf(buf, "motion at %s (+%d s)", time_t_2_str(last_motion_time).c_str(), light_linger);
        last_light_decision[0] = buf;
    }
    else 
    {
        last_light_decision[0] = "linger out";
    }

    light = motion_light;

    bool last_rh_toggle = rh_toggle;

    if (rh >= rh_on)
    {
        if (rh_toggle == false)
        {
            rh_toggle = true;
            sprintf(buf, "rh-high %.1f/%.1f at %s", rh, (float)rh_on, time_t_2_str(now_time).c_str());
            last_fan_decision[0] = buf;
        }
    }
    else if (rh <= rh_off)
    {
        if (rh_toggle == true)
        {
            rh_toggle = false;
            sprintf(buf, "rh-low %.1f/%.1f at %s", rh, (float)rh_off, time_t_2_str(now_time).c_str());
            last_fan_decision[0] = buf;
        }
    }
    else
    {
        if (rh_toggle == true)
        {
            if (is_soft_rh_toggle_down_condition())
            {
                TRACE("Soft rh_toggle condition")
                rh_toggle = false;
                sprintf(buf, "rh-soft-down at %s", time_t_2_str(now_time).c_str());
                last_fan_decision[0] = buf;

                DEBUG("rh_sliding_window at rh_toggle condition")
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
            }
        }
    }

    if (last_rh_toggle != rh_toggle)
    {
        // DEBUG("rh_toggle=%d", (int) rh_toggle)
    }

    if (motion_fan == true)
    {
        //if (fan == false)
        //{
            fan = true;
            sprintf(buf, "motion at %s (+%d s)", time_t_2_str(last_motion_time).c_str(), fan_linger);
            last_fan_decision[1] = buf;
        //}
    }
    else
    {
        if (last_fan_decision[1].length() > 0)
        {
            last_fan_decision[1] = "";
        }
        fan = rh_toggle;
    }

    if (light_mode != ShowerGuardConfig::Light::mAuto)
    {
        light = light_mode == ShowerGuardConfig::Light::mOn ? true : false;
        last_light_decision[1] = "nonauto-mode";
    }
    else
    {
        last_light_decision[1] = "";
    }

    if (fan_mode != ShowerGuardConfig::Fan::mAuto)
    {
        fan = fan_mode == ShowerGuardConfig::Fan::mOn ? true : false;
        last_fan_decision[2] = "nonauto-mode";
    }
    else
    {
        last_fan_decision[2] = "";
    }
}

String ShowerGuardAlgo::get_last_light_decision() const
{
    for (int i = sizeof(last_light_decision) / sizeof(last_light_decision[0]) - 1; i >= 0; --i)
    {
        if (!last_light_decision[i].isEmpty())
        {
            return last_light_decision[i];
        }
    }

    return String("");
}

String ShowerGuardAlgo::get_last_fan_decision() const
{
    for (int i = sizeof(last_fan_decision) / sizeof(last_fan_decision[0]) - 1; i >= 0; --i)
    {
        if (!last_fan_decision[i].isEmpty())
        {
            return last_fan_decision[i];
        }
    }

    return String("");
}

void ShowerGuardAlgo::debug_last_light_decision() const
{
    DEBUG("last_light_decision:")

    for (int i = sizeof(last_light_decision) / sizeof(last_light_decision[0]) - 1; i >= 0; --i)
    {
        if (!last_light_decision[i].isEmpty() || i==0)
        {
            DEBUG("[%d]=%s", i, last_light_decision[i].c_str())
        }
    }
}

void ShowerGuardAlgo::debug_last_fan_decision() const
{
    DEBUG("last_fan_decision:")

    for (int i = sizeof(last_fan_decision) / sizeof(last_fan_decision[0]) - 1; i >= 0; --i)
    {
        if (!last_fan_decision[i].isEmpty() || i==0)
        {
            DEBUG("[%d]=%s", i, last_fan_decision[i].c_str())
        }
    }
}

void ShowerGuardAlgo::init()
{
    light = false;
    fan = false;
    reset_rh_decision = true;
    last_motion_millis = 0;
    time(&last_motion_time);
    rh_toggle = false;
    reset_rh_window();

    light_linger = 0;
    fan_linger = 0;
    rh_off = 0;
    rh_on = 0;
    light_mode = ShowerGuardConfig::Light::mAuto;
    fan_mode = ShowerGuardConfig::Light::mAuto;

    for (size_t i = 1; i < sizeof(last_light_decision) / sizeof(last_light_decision[0]); ++i)
    {
        last_light_decision[i] = "";
    }

    for (size_t i = 1; i < sizeof(last_fan_decision) / sizeof(last_fan_decision[0]); ++i)
    {
        last_fan_decision[i] = "";
    }

    last_light_decision[0] = "init";
    last_fan_decision[0] = "init";
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

    if (rh_sliding_window_pos < sizeof(rh_sliding_window) / sizeof(rh_sliding_window[0]))
    {
        rh_sliding_window[rh_sliding_window_pos] = rh;
        rh_sliding_window_pos++;
    }
    else
    {
        for (int i = 0; i < sizeof(rh_sliding_window) / sizeof(rh_sliding_window[0]) - 1; ++i)
            rh_sliding_window[i] = rh_sliding_window[i + 1];

        rh_sliding_window[sizeof(rh_sliding_window) / sizeof(rh_sliding_window[0]) - 1] = rh;
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

    if (rh_sliding_window_pos == sizeof(rh_sliding_window) / sizeof(rh_sliding_window[0])) // sliding window is fully filled
    {
        float upper = rh_off + float(rh_on - rh_off) / 10.0;

        for (int i = 0; i < sizeof(rh_sliding_window) / sizeof(rh_sliding_window[0]); ++i)
        {
            if (rh_sliding_window[i] > upper)
                return false;
        }

        return true;
    }

    return false;
}

void ShowerGuardHandler::start(const ShowerGuardConfig &_config)
{
    if (_is_active)
    {
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    config = _config;
    configure_hw();
    algo.start(config);

    _is_active = true;
    _is_finished = false;

    xTaskCreate(
        task,                // Function that should be called
        "shower_guard_task", // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void ShowerGuardHandler::stop()
{
    if (_is_active)
    {
        algo.stop();
    }

    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }
}

void ShowerGuardHandler::reconfigure(const ShowerGuardConfig &_config)
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

void ShowerGuardHandler::task(void *parameter)
{
    ShowerGuardHandler *_this = (ShowerGuardHandler *)parameter;

    TRACE("shower_guard_task: started")

    unsigned temp_read_slot_count = 0;
    unsigned rh_read_slot_count = 0;
    unsigned lumi_read_slot_count = 0;
    unsigned logging_slot_count = 0;

    float rh = 60.0;

    // we average several samples and feed in algo once per average ready; this is because a considerable fluctuation of readings

    float rh_avg_window[5];
    size_t rh_avg_window_pos = 0;

    float temp = 20.0;

    float luminance_percent = 0;
    bool light_luminance_mask = true;

    // here is a short hysteresis loop for motion since the sensor would reset to 0 after its internal timeout even if the
    // motion continues

    bool fan = false;
    bool light = false;

    bool motion_hys = false;
    bool motion = false;
    unsigned motion_hys_count = 0;
    ShowerGuardStatus status_copy;

    while (_this->_is_active)
    {
        bool do_algo_loop = false;

        {
            Lock lock(_this->semaphore);

            if (temp_read_slot_count == 0)
            {
                float last_temp = temp;

                if (_this->read_temp(temp))
                {
                    temp_read_slot_count = TEMP_READ_SLOT - 1;
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
                float i_rh = _this->read_rh(temp);
                rh_read_slot_count = RH_READ_SLOT - 1;
                //DEBUG("read rh=%f", rh)

                rh_avg_window[rh_avg_window_pos] = i_rh;
                rh_avg_window_pos++;

                if (rh_avg_window_pos == sizeof(rh_avg_window) / sizeof(rh_avg_window[0]))
                {
                    rh_avg_window_pos = 0;

                    float new_rh = 0;

                    for (int i = 0; i < sizeof(rh_avg_window) / sizeof(rh_avg_window[0]); ++i)
                        new_rh += rh_avg_window[i];

                    new_rh /= sizeof(rh_avg_window) / sizeof(rh_avg_window[0]);

                    new_rh = round(new_rh * 10.0) / 10.0;

                    if (abs(new_rh - rh) >= 1.0)
                    {
                        logging_slot_count = 0;
                    }

                    rh = new_rh;
                    do_algo_loop = true;
                    DEBUG("do_algo_loop rh")
                }
            }
            else
            {
                if (CALIBRATE_RH)  // to make trace more often, this is calibration mode only
                {
                    _this->read_rh(temp);
                }
                rh_read_slot_count--;
            }

            if (lumi_read_slot_count == 0)
            {
                luminance_percent = _this->read_lumi(_this->config.lumi);
                //DEBUG("luminance_percent %f", luminance_percent)
                lumi_read_slot_count = LUMI_READ_SLOT - 1;
            }
            else
            {
                lumi_read_slot_count--;
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
                motion_hys_count = MOTION_HYS - 1;
            }

            if (last_motion_hys != motion_hys || last_motion != motion)
            {
                DEBUG("motion=%d, motion_hys=%d", (int) motion, (int) motion_hys)

                if (last_motion_hys != motion_hys)
                {
                    logging_slot_count = 0;
                    do_algo_loop = true;
                    DEBUG("do_algo_loop motion")
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
                    if (light == true)
                    {
                        if (_this->config.lumi.is_configured())
                        {
                            // we need to re-read luminance before we turn on the light; it might have been read
                            // last time while the light still was on (because it is done at certain slots) and 
                            // if the light rapidly changes from on-off-on then luminance can reflect what it was
                            // at last light on 
            
                            luminance_percent = _this->read_lumi(_this->config.lumi);
                            light_luminance_mask = luminance_percent < _this->config.lumi.threshold ? true : false;
                        }
                        else
                        {
                            light_luminance_mask = true;   
                        }
                    }
                    else
                    {
                        light_luminance_mask = true;   
                    }

                    //write_light(_this->config.light, light && light_luminance_mask);
                }

                if (fan != last_fan)
                {
                    //write_fan(_this->config.fan, fan);
                }

                if (last_light != light || last_fan != fan)
                {
                    //DEBUG("light=%d, fan=%d", (int) light, (int) fan)
                    logging_slot_count = 0;
                }
            }

            status_copy.temp = temp;
            status_copy.rh = rh;
            status_copy.motion = motion_hys;
            status_copy.luminance_percent = luminance_percent;
            status_copy.light_luminance_mask = light_luminance_mask;
            status_copy.light = light;
            status_copy.fan = fan;
            status_copy.light_decision = _this->algo.get_last_light_decision();
            status_copy.fan_decision = _this->algo.get_last_fan_decision();

            _this->status = status_copy;
        }

        if (logging_slot_count == 0)
        {
            logging_slot_count = LOGGING_SLOT;

            // moved it here from -- if now... != last... -- to also periodically write
            // down the latest value in case we somehow miss to do it at value change (???)

            write_fan(_this->config.fan, fan);
            write_light(_this->config.light, light && light_luminance_mask);

            //TRACE("light_decision length %d, is empty %d is NULL %d", (int) status_copy.light_decision.length(), (int)(status_copy.light_decision.isEmpty() == NULL ? 1 : 0), (int)(status_copy.light_decision.c_str() == NULL ? 1 : 0) )

            TRACE("* {\"temp\":%.1f, \"rh\":%.1f, \"motion\":%d, \"luminance_percent\":%f, \"light_luminance_mask\":%d, "
                  "\"light\":%d, \"fan\":%d, \"light_decision\":\"%s\", \"fan_decision\":\"%s\"}",
                  temp, rh, (int)motion_hys, luminance_percent, (int) light_luminance_mask, (int)light, (int)fan, 
                  status_copy.light_decision.c_str(), status_copy.fan_decision.c_str())

            _this->algo.debug_last_light_decision();
            _this->algo.debug_last_fan_decision();
        }
        else
        {
            logging_slot_count--;
        }

        delay(1000);
    }

    _this->_is_finished = true;

    TRACE("shower_guard_task: terminated")
    vTaskDelete(NULL);
}

void ShowerGuardHandler::configure_hw()
{
    configure_hw_rh();
    configure_hw_temp();
    configure_hw_motion(config.motion);
    configure_hw_lumi(config.lumi);
    configure_hw_light(config.light);
    configure_hw_fan(config.fan);
}

void ShowerGuardHandler::configure_hw_rh()
{
    // configure rh

    if (aht10)
    {
        delete aht10;
        aht10 = NULL;
    }

    if (config.rh.hw == ShowerGuardConfig::Rh::hwHih5030)
    {
        TRACE("configure rh.vad: gpio=%d, atten=%d", (int)config.rh.vad.gpio, (int)config.rh.vad.atten)
        analogSetPinAttenuation(config.rh.vad.gpio, (adc_attenuation_t)config.rh.vad.atten);
        TRACE("configure rh.vdd: gpio=%d, atten=%d", (int)config.rh.vdd.gpio, (int)config.rh.vdd.atten)
        analogSetPinAttenuation(config.rh.vdd.gpio, (adc_attenuation_t)config.rh.vdd.atten);
    }
    else if (config.rh.hw == ShowerGuardConfig::Rh::hwAht10)
    {
        // this will only work once; if sda / scl are changed the target needs to be restarted!
        TRACE("configure sda: gpio=%d, scl: gpio=%d", (int)config.rh.sda.gpio, (int)config.rh.scl.gpio)
        Wire.begin(config.rh.sda.gpio,config.rh.scl.gpio); 

        aht10 = new AHT10();  // addr is default and fixed, ignore from config?

        if (aht10->begin() != true)
        {
            ERROR("AHT10 not connected or fail to load calibration coefficient")
        }
        else
        {
            TRACE("AHT10 begin OK")

        }
    }
}

void ShowerGuardHandler::configure_hw_temp()
{
    // configure temp

    if (config.temp.channel.is_valid())
    {
        TRACE("configure temp: gpio=%d", (int)config.temp.channel.gpio)

        ow.begin(config.temp.channel.gpio);
        ds18b20.setOneWire(&ow);
        ds18b20.setWaitForConversion(true);
        ds18b20.setCheckForConversion(true);
    }
}

void ShowerGuardHandler::configure_hw_motion(const ShowerGuardConfig::Motion &motion)
{
    // configure motion
    // ignore debounce because we are using polling with low frequency (to skip adding synchronisation in case of interrupts)

    TRACE("configure motion: gpio=%d, inverted=%d", (int)motion.channel.gpio, (int)motion.channel.inverted)
    gpioHandler.setupChannel(motion.channel.gpio, INPUT_PULLUP, motion.channel.inverted, NULL);
}

void ShowerGuardHandler::configure_hw_lumi(const ShowerGuardConfig::Lumi &lumi)
{
    // configure lumi

    if (lumi.is_configured())
    {
        TRACE("configure lumi.ldr: gpio=%d, atten=%d", (int)lumi.ldr.gpio, (int)lumi.ldr.atten)        
        analogSetPinAttenuation(lumi.ldr.gpio, (adc_attenuation_t)lumi.ldr.atten);
    }
    else
    {
        TRACE("skip configuring lumi: not defined / not installed")        
    }
}

void ShowerGuardHandler::configure_hw_light(const ShowerGuardConfig::Light &light)
{
    // configure light

    TRACE("configure light: gpio=%d, inverted=%d", (int)light.channel.gpio, (int)light.channel.inverted)
    gpioHandler.setupChannel(light.channel.gpio, OUTPUT, light.channel.inverted, NULL);
}

void ShowerGuardHandler::configure_hw_fan(const ShowerGuardConfig::Fan &fan)
{
    // configure fan

    TRACE("configure fan: gpio=%d, inverted=%d", (int)fan.channel.gpio, (int)fan.channel.inverted)
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

float ShowerGuardHandler::read_rh(float temp)
{
    if (config.rh.hw == ShowerGuardConfig::Rh::hwHih5030)
    {
        unsigned vad = analog_read(config.rh.vad.gpio);
        unsigned vdd = analog_read(config.rh.vdd.gpio);

        if (CALIBRATE_RH)
        {
            // trace to calibrate voltage divider
            // VDD should be as close to VAD as possible with a loop between VAD input and 3.3V
            DEBUG("vad %d, vdd %d", (int) vad, (int) vdd)
        }

        if (vdd != 0)
        {
            // the formula with temperature compensation
            // (((Vout/VS)-0.1515)/0.00636) / (1.0546-0.00216*T) = (RH )

            float r_rh = (((float(vad) / float(vdd)) - 0.1515) / 0.00636) / (1.0546 - 0.00216 * temp);

            if (config.rh.corr != 0)
            {
                //DEBUG("applying non-zero rh correction %f", rh.corr)
                r_rh += config.rh.corr;
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
    }
    else if (config.rh.hw == ShowerGuardConfig::Rh::hwAht10)
    {
        float r_rh = 0;

        if (aht10)
        {
            r_rh = aht10->readHumidity();

            if (r_rh == AHT10_ERROR)
            {
                ERROR("failed to read humidity from AHT10")
            }
            else
            {
                if (config.rh.corr != 0)
                {
                    //DEBUG("applying non-zero rh correction %f", rh.corr)
                    r_rh += config.rh.corr;
                }

                return r_rh; 
            }
        }
    }
    
    return 0;
}

bool ShowerGuardHandler::read_temp(float &temp)
{
    //ow.reset();
    //ow.reset_search();

    bool r = false;

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

                if ((config.temp.addr.isEmpty() && device_count == 1) || !strcmp(addr_str, config.temp.addr.c_str()))
                {
                    r_temp = round(i_temp * 10.0) / 10.0;

                    if (config.temp.corr != 0)
                    {
                        //DEBUG("applying non-zero correction %f", config.temp.corr)
                        r_temp += config.temp.corr;
                    }
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
    }

    if (r == false)
    {
        if (config.rh.hw == ShowerGuardConfig::Rh::hwAht10)
        {
            float r_temp = 0;

            if (aht10)
            {
                r_temp = aht10->readTemperature();

                if (r_temp == AHT10_ERROR)
                {
                    ERROR("failed to read temperature from AHT10")
                }
                else
                {
                    r = true;
                }
            }

            if (r == true)
            {
                if (config.temp.corr != 0)
                {
                    //DEBUG("applying non-zero correction %f", config.temp.corr)
                    r_temp += config.temp.corr;
                }

                temp = r_temp;
            }
        }
    }
    
    return r;
}

bool ShowerGuardHandler::read_motion(const ShowerGuardConfig::Motion &motion)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    return GpioChannel::read(motion.channel.gpio, motion.channel.inverted);
}

float ShowerGuardHandler::read_lumi(const ShowerGuardConfig::Lumi &lumi)
{

    if (lumi.is_configured())
    {
        unsigned ldr_reading = analog_read(lumi.ldr.gpio);
        // DEBUG("ldr_reading %d", (int) ldr_reading)
        return float(int((float(ldr_reading) / float(4095))*100));
    }

    return 0;
}

void ShowerGuardHandler::write_light(const ShowerGuardConfig::Light &light, bool value)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    bool value_to_write = light.channel.coilon_active ? value : (value ? false : true);
    GpioChannel::write(light.channel.gpio, light.channel.inverted, value_to_write);
    DEBUG("write_light gpio %d value %d", (int) light.channel.gpio, (int) value_to_write)
}

void ShowerGuardHandler::write_fan(const ShowerGuardConfig::Fan &fan, bool value)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    bool value_to_write = fan.channel.coilon_active ? value : (value ? false : true);
    GpioChannel::write(fan.channel.gpio, fan.channel.inverted, value_to_write);
    DEBUG("write_fan gpio %d value %d", (int) fan.channel.gpio, (int) value_to_write)
}

void start_shower_guard_task(const ShowerGuardConfig &config)
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

void reconfigure_shower_guard(const ShowerGuardConfig &_config)
{
    handler.reconfigure(_config);
}

#endif // INCLUDE_SHOWERGUARD
