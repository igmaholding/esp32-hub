#ifdef INCLUDE_MULTI

#include <Audio.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <multi.h>
#include <gpio.h>
#include <trace.h>
#include <esp_log.h>
#include <time.h>

extern GpioHandler gpioHandler;

void audio_info(const char* msg)
{
   TRACE("AUDIO %s", msg)
}

static void _err_dup(const char *name, int value)
{
    ERROR("%s %d is duplicated / reused", name, value)
}

static void _err_cap(const char *name, int value)
{
    ERROR("%s %d, gpio doesn't have required capabilities", name, value)
}

bool MultiConfig::is_valid() const
{
    bool r = uart.is_valid() && bt.is_valid() && fm.is_valid() && i2s.is_valid() && i2c.is_valid() && service.is_valid() && 
             sound.is_valid() && tm1638.is_valid() && ui.is_valid();

    if (r == false)
    {
        return false;
    }

    GpioCheckpad checkpad;

    String object_name;

    object_name = "i2s.dout.gpio";

    if (checkpad.get_usage(i2s.dout.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)i2s.dout.gpio);
        return false;
    }

    if (!checkpad.set_usage(i2s.dout.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)i2s.dout.gpio);
        return false;
    }

    object_name = "i2s.bclk.gpio";

    if (checkpad.get_usage(i2s.bclk.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)i2s.bclk.gpio);
        return false;
    }

    if (!checkpad.set_usage(i2s.bclk.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)i2s.bclk.gpio);
        return false;
    }

    object_name = "i2s.lrc.gpio";

    if (checkpad.get_usage(i2s.lrc.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)i2s.lrc.gpio);
        return false;
    }

    if (!checkpad.set_usage(i2s.lrc.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)i2s.lrc.gpio);
        return false;
    }

    object_name = "i2c.scl.gpio";

    if (checkpad.get_usage(i2c.scl.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)i2c.scl.gpio);
        return false;
    }

    if (!checkpad.set_usage(i2c.scl.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)i2c.scl.gpio);
        return false;
    }

    object_name = "i2c.sda.gpio";

    if (checkpad.get_usage(i2c.sda.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)i2c.sda.gpio);
        return false;
    }

    if (!checkpad.set_usage(i2c.sda.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)i2c.sda.gpio);
        return false;
    }

    object_name = "uart.tx.gpio";

    if (checkpad.get_usage(uart.tx.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)uart.tx.gpio);
        return false;
    }

    if (!checkpad.set_usage(uart.tx.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)uart.tx.gpio);
        return false;
    }

    object_name = "uart.rx.gpio";

    if (checkpad.get_usage(uart.rx.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)uart.rx.gpio);
        return false;
    }

    if (!checkpad.set_usage(uart.rx.gpio, GpioCheckpad::uDigitalInput))
    {
        _err_cap(object_name.c_str(), (int)uart.rx.gpio);
        return false;
    }

    object_name = "sound.mute.gpio";

    if (sound.mute.is_valid())
    {
        if (checkpad.get_usage(sound.mute.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)sound.mute.gpio);
            return false;
        }

        if (!checkpad.set_usage(sound.mute.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)sound.mute.gpio);
            return false;
        }
    }

    object_name = "bt.reset.gpio";

    if (bt.reset.is_valid())
    {
        if (checkpad.get_usage(bt.reset.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)bt.reset.gpio);
            return false;
        }

        if (!checkpad.set_usage(bt.reset.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)bt.reset.gpio);
            return false;
        }
    }

    object_name = "tm1638.dio.gpio";

    if (checkpad.get_usage(tm1638.dio.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)tm1638.dio.gpio);
        return false;
    }

    if (!checkpad.set_usage(tm1638.dio.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)tm1638.dio.gpio);
        return false;
    }

    object_name = "tm1638.clk.gpio";

    if (checkpad.get_usage(tm1638.clk.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)tm1638.clk.gpio);
        return false;
    }

    if (!checkpad.set_usage(tm1638.clk.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)tm1638.clk.gpio);
        return false;
    }

    if (tm1638.dir.is_valid())
    {
        object_name = "tm1638.dir.gpio";

        if (checkpad.get_usage(tm1638.dir.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)tm1638.dir.gpio);
            return false;
        }

        if (!checkpad.set_usage(tm1638.dir.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)tm1638.dir.gpio);
            return false;
        }
    }

    object_name = "ui.stb.gpio";

    if (checkpad.get_usage(ui.stb.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)ui.stb.gpio);
        return false;
    }

    if (!checkpad.set_usage(ui.stb.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)ui.stb.gpio);
        return false;
    }

    return true;
}

void MultiConfig::from_json(const JsonVariant &json)
{
    if (json.containsKey("uart"))
    {
        const JsonVariant &_json = json["uart"];
        uart.from_json(_json);
    }

    if (json.containsKey("bt"))
    {
        const JsonVariant &_json = json["bt"];
        bt.from_json(_json);
    }

    if (json.containsKey("fm"))
    {
        const JsonVariant &_json = json["fm"];
        fm.from_json(_json);
    }

    if (json.containsKey("i2s"))
    {
        const JsonVariant &_json = json["i2s"];
        i2s.from_json(_json);
    }

    if (json.containsKey("i2c"))
    {
        const JsonVariant &_json = json["i2c"];
        i2c.from_json(_json);
    }

    if (json.containsKey("service"))
    {
        const JsonVariant &_json = json["service"];
        service.from_json(_json);
    }

    if (json.containsKey("sound"))
    {
        const JsonVariant &_json = json["sound"];
        sound.from_json(_json);
    }

    if (json.containsKey("tm1638"))
    {
        const JsonVariant &_json = json["tm1638"];
        tm1638.from_json(_json);
    }

    if (json.containsKey("ui"))
    {
        const JsonVariant &_json = json["ui"];
        ui.from_json(_json);
    }
}

void MultiConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    uart.to_eprom(os);
    bt.to_eprom(os);
    fm.to_eprom(os);
    i2s.to_eprom(os);
    i2c.to_eprom(os);
    service.to_eprom(os);
    sound.to_eprom(os);
    tm1638.to_eprom(os);
    ui.to_eprom(os);
}

bool MultiConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        uart.from_eprom(is);
        bt.from_eprom(is);
        fm.from_eprom(is);
        i2s.from_eprom(is);
        i2c.from_eprom(is);
        service.from_eprom(is);
        sound.from_eprom(is);
        tm1638.from_eprom(is);
        ui.from_eprom(is);

        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read MultiConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void MultiConfig::I2s::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("dout"))
    {
        //DEBUG("MultiConfig::I2s::from_json contains dout")
        const JsonVariant &_json = json["dout"];

        if (_json.containsKey("channel"))
        {
            //DEBUG("MultiConfig::I2s::from_json dout contains channel")
            const JsonVariant &__json = _json["channel"];
            dout.from_json(__json);
            //DEBUG("dout after from_json %s", dout.as_string().c_str())
        }
    }
    if (json.containsKey("bclk"))
    {
        const JsonVariant &_json = json["bclk"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            bclk.from_json(__json);
        }
    }
    if (json.containsKey("lrc"))
    {
        const JsonVariant &_json = json["lrc"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            lrc.from_json(__json);
        }
    }
}

void MultiConfig::I2s::to_eprom(std::ostream &os) const
{
    dout.to_eprom(os);
    bclk.to_eprom(os);
    lrc.to_eprom(os);
}

bool MultiConfig::I2s::from_eprom(std::istream &is)
{
    dout.from_eprom(is);
    bclk.from_eprom(is);
    lrc.from_eprom(is);

    return is_valid() && !is.bad();
}

void MultiConfig::I2c::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("scl"))
    {
        //DEBUG("MultiConfig::I2c::from_json contains scl")
        const JsonVariant &_json = json["scl"];

        if (_json.containsKey("channel"))
        {
            //DEBUG("MultiConfig::I2c::from_json scl contains channel")
            const JsonVariant &__json = _json["channel"];
            scl.from_json(__json);
            //DEBUG("scl after from_json %s", scl.as_string().c_str())
        }
    }
    if (json.containsKey("sda"))
    {
        //DEBUG("MultiConfig::I2c::from_json contains sda")
        const JsonVariant &_json = json["sda"];

        if (_json.containsKey("channel"))
        {
            //DEBUG("MultiConfig::I2c::from_json sda contains channel")
            const JsonVariant &__json = _json["channel"];
            sda.from_json(__json);
            //DEBUG("sda after from_json %s", sda.as_string().c_str())
        }
    }
}

void MultiConfig::I2c::to_eprom(std::ostream &os) const
{
    scl.to_eprom(os);
    sda.to_eprom(os);
}

bool MultiConfig::I2c::from_eprom(std::istream &is)
{
    scl.from_eprom(is);
    sda.from_eprom(is);

    return is_valid() && !is.bad();
}

void MultiConfig::UART::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("uart_num"))
    {
        uart_num = (uint8_t)(int) json["uart_num"];
    }

    if (json.containsKey("tx"))
    {
        const JsonVariant &_json = json["tx"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            tx.from_json(__json);
        }
    }

    if (json.containsKey("rx"))
    {
        const JsonVariant &_json = json["rx"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            rx.from_json(__json);
        }
    }
}

void MultiConfig::UART::to_eprom(std::ostream &os) const
{
    os.write((const char *)&uart_num, sizeof(uart_num));

    tx.to_eprom(os);
    rx.to_eprom(os);
}

bool MultiConfig::UART::from_eprom(std::istream &is)
{
    is.read((char *)&uart_num, sizeof(uart_num));

    tx.from_eprom(is);
    rx.from_eprom(is);

    return is_valid() && !is.bad();
}

void MultiConfig::Bt::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("name"))
    {
        name = (const char*) json["name"];
    }
    if (json.containsKey("pin"))
    {
        pin = (const char*) json["pin"];
    }
    if (json.containsKey("hw"))
    {
        hw = json["hw"];
    }
    if (json.containsKey("reset"))
    {
        reset.from_json(json["reset"]);
    }
}

void MultiConfig::Bt::to_eprom(std::ostream &os) const
{
    uint8_t len = name.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(name.c_str(), len);
    }

    len = pin.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(pin.c_str(), len);
    }

    os.write((const char *)&hw, sizeof(hw));
    reset.to_eprom(os);
}

bool MultiConfig::Bt::from_eprom(std::istream &is)
{
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        name = buf;
    }
    else
    {
        name = "";
    }
    
    len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        pin = buf;
    }
    else
    {
        pin = "";
    }

    is.read((char *)&hw, sizeof(hw));
    reset.from_eprom(is);

    return is_valid() && !is.bad();
}

void MultiConfig::Service::from_json(const JsonVariant &json)
{
    if (json.containsKey("url"))
    {
        const JsonVariant &_json = json["url"];

        if (_json.is<JsonArray>())
        {
            size_t i=0;

            for (; i<sizeof(url)/sizeof(url[0]); ++i)
            {
                url[i].clear();
            }
            
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();
            i=0;

            while(iterator != jsonArray.end() && i<sizeof(url)/sizeof(url[0]))
            {
                const JsonVariant & __json = *iterator;
                url[i].from_json(__json);

                ++iterator;
                ++i;
            }
        }
    }

    if (json.containsKey("url_select"))
    {
        url_select = (int8_t) (int) json["url_select"];
    }

    if (json.containsKey("fm_freq"))
    {
        const JsonVariant &_json = json["fm_freq"];

        if (_json.is<JsonArray>())
        {
            size_t i=0;

            for (; i<sizeof(fm_freq)/sizeof(fm_freq[0]); ++i)
            {
                fm_freq[i].clear();
            }
            
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();
            i=0;

            while(iterator != jsonArray.end() && i<sizeof(fm_freq)/sizeof(fm_freq[0]))
            {
                const JsonVariant & __json = *iterator;
                fm_freq[i].from_json(__json);

                ++iterator;
                ++i;
            }
        }
    }

    if (json.containsKey("fm_freq_select"))
    {
        fm_freq_select = (int8_t) (int) json["fm_freq_select"];
    }
}

void MultiConfig::Service::to_eprom(std::ostream &os) const
{
    for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
    {
        url[i].to_eprom(os);
    }

    os.write((const char *)&url_select, sizeof(url_select));

    for (size_t i=0; i<sizeof(fm_freq)/sizeof(fm_freq[0]); ++i)
    {
        fm_freq[i].to_eprom(os);
    }

    os.write((const char *)&fm_freq_select, sizeof(fm_freq_select));
}

bool MultiConfig::Service::from_eprom(std::istream &is)
{
    for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
    {
        url[i].from_eprom(is);
    }

    is.read((char *)&url_select, sizeof(url_select));

    for (size_t i=0; i<sizeof(fm_freq)/sizeof(fm_freq[0]); ++i)
    {
        fm_freq[i].from_eprom(is);
    }

    is.read((char *)&fm_freq_select, sizeof(fm_freq_select));

    return is_valid() && !is.bad();
}

void MultiConfig::Service::Url::from_json(const JsonVariant &json)
{
    if (json.containsKey("name"))
    {
        name = (const char *)json["name"];
    }

    if (json.containsKey("value"))
    {
        value = (const char *)json["value"];
    }
}

void MultiConfig::Service::Url::to_eprom(std::ostream &os) const
{
    uint8_t len = name.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(name.c_str(), len);
    }
    
    len = value.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(value.c_str(), len);
    }
}

bool MultiConfig::Service::Url::from_eprom(std::istream &is)
{
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        name = buf;
    }
    else
    {
        name = "";
    }
    
    len = 0;

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

void MultiConfig::Service::FmFreq::from_json(const JsonVariant &json)
{
    if (json.containsKey("name"))
    {
        name = (const char *)json["name"];
    }

    if (json.containsKey("value"))
    {
        value = (float)json["value"];
    }
}

void MultiConfig::Service::FmFreq::to_eprom(std::ostream &os) const
{
    uint8_t len = name.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(name.c_str(), len);
    }
    
    os.write((const char *)&value, sizeof(value));
}

bool MultiConfig::Service::FmFreq::from_eprom(std::istream &is)
{
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        name = buf;
    }
    else
    {
        name = "";
    }
    
    is.read((char *)&value, sizeof(value));

    return is_valid() && !is.bad();
}

void MultiConfig::Sound::from_json(const JsonVariant &json)
{
    if (json.containsKey("hw"))
    {
        hw = (const char*) json["hw"];
    }

    if (json.containsKey("addr"))
    {
        addr = (const char*) json["addr"];
    }

    if (json.containsKey("mute"))
    {
        mute.from_json(json["mute"]);
    }

    if (json.containsKey("volume"))
    {
        volume = (uint8_t) (int) json["volume"];
    }

    if (json.containsKey("volume_low"))
    {
        volume_low = (uint8_t) (int) json["volume_low"];
    }

    if (json.containsKey("gain_low_pass"))
    {
        gain_low_pass = (int8_t) (int) json["gain_low_pass"];
    }

    if (json.containsKey("gain_band_pass"))
    {
        gain_band_pass = (int8_t) (int) json["gain_band_pass"];
    }

    if (json.containsKey("gain_high_pass"))
    {
        gain_high_pass = (int8_t) (int) json["gain_high_pass"];
    }

    if (json.containsKey("schedule"))
    {
        const JsonVariant &_json = json["schedule"];

        if (_json.is<JsonArray>())
        {
            size_t i=0;

            for (; i<sizeof(schedule)/sizeof(schedule[0]); ++i)
            {
                schedule[i] = smOn;
            }
            
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();
            i=0;

            while(iterator != jsonArray.end() && i<sizeof(schedule)/sizeof(schedule[0]))
            {
                const JsonVariant & __json = *iterator;
                schedule[i] = (uint8_t) (int) __json;

                ++iterator;
                ++i;
            }
        }
    }
}

void MultiConfig::Sound::to_eprom(std::ostream &os) const
{
    uint8_t len = (uint8_t) hw.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(hw.c_str(), len);
    }

    len = (uint8_t) addr.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(addr.c_str(), len);
    }

    mute.to_eprom(os);

    os.write((const char *)&volume, sizeof(volume));
    os.write((const char *)&volume_low, sizeof(volume_low));

    os.write((const char *)&gain_low_pass, sizeof(gain_low_pass));
    os.write((const char *)&gain_band_pass, sizeof(gain_band_pass));
    os.write((const char *)&gain_high_pass, sizeof(gain_high_pass));

    os.write((const char *)schedule, sizeof(schedule));
}

bool MultiConfig::Sound::from_eprom(std::istream &is)
{
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        hw = buf;
    }
    else
    {
        hw = "";
    }
 
    len = 0;

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

    mute.from_eprom(is);

    is.read((char *)&volume, sizeof(volume));
    is.read((char *)&volume_low, sizeof(volume_low));

    is.read((char *)&gain_low_pass, sizeof(gain_low_pass));
    is.read((char *)&gain_band_pass, sizeof(gain_band_pass));
    is.read((char *)&gain_high_pass, sizeof(gain_high_pass));

    is.read((char *)schedule, sizeof(schedule));

    return is_valid() && !is.bad();
}

void MultiConfig::Fm::from_json(const JsonVariant &json)
{
    if (json.containsKey("hw"))
    {
        hw = (const char*) json["hw"];
    }

    if (json.containsKey("addr"))
    {
        addr = (const char*) json["addr"];
    }
}

void MultiConfig::Fm::to_eprom(std::ostream &os) const
{
    uint8_t len = (uint8_t) hw.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(hw.c_str(), len);
    }

    len = (uint8_t) addr.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(addr.c_str(), len);
    }
}

bool MultiConfig::Fm::from_eprom(std::istream &is)
{
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        hw = buf;
    }
    else
    {
        hw = "";
    }
 
    len = 0;

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

    return is_valid() && !is.bad();
}

void MultiConfig::Tm1638::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("dio"))
    {
        //DEBUG("MultiConfig::Tm1638::from_json contains dio")
        const JsonVariant &_json = json["dio"];

        if (_json.containsKey("channel"))
        {
            //DEBUG("MultiConfig::Tm1638::from_json dio contains channel")
            const JsonVariant &__json = _json["channel"];
            dio.from_json(__json);
            //DEBUG("dio after from_json %s", dio.as_string().c_str())
        }
    }
    if (json.containsKey("clk"))
    {
        const JsonVariant &_json = json["clk"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            clk.from_json(__json);
        }
    }
    if (json.containsKey("dir"))
    {
        const JsonVariant &_json = json["dir"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            dir.from_json(__json);
        }
    }
}

void MultiConfig::Tm1638::to_eprom(std::ostream &os) const
{
    dio.to_eprom(os);
    clk.to_eprom(os);
    dir.to_eprom(os);
}

bool MultiConfig::Tm1638::from_eprom(std::istream &is)
{
    dio.from_eprom(is);
    clk.from_eprom(is);
    dir.from_eprom(is);

    return is_valid() && !is.bad();
}

void MultiConfig::UI::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("name"))
    {
        name = (const char*) json["name"];
    }

    if (json.containsKey("stb"))
    {
        const JsonVariant &_json = json["stb"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            stb.from_json(__json);
        }
    }
}

void MultiConfig::UI::to_eprom(std::ostream &os) const
{
    uint8_t len = (uint8_t) name.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(name.c_str(), len);
    }

    stb.to_eprom(os);
}

bool MultiConfig::UI::from_eprom(std::istream &is)
{
    uint8_t len = 0;

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        char buf[256];
        is.read(buf, len);
        buf[len] = 0;
        name = buf;
    }
    else
    {
        name = "";
    }

    stb.from_eprom(is);

    return is_valid() && !is.bad();
}




#endif // INCLUDE_MULTI