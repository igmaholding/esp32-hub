#include <ArduinoJson.h>
#include <showerGuard.h>
#include <gpio.h>
#include <trace.h>

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
}


void ShowerGuardConfig::to_eprom(std::ostream & os) const
{
    os.write((const char*) & EPROM_VERSION, sizeof(EPROM_VERSION));
    motion.to_eprom(os);
    rh.to_eprom(os);
    temp.to_eprom(os);
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

    if (json.containsKey("linger"))
    {
        linger = (unsigned)((int) json["linger"]);
    }
}


void ShowerGuardConfig::Motion::to_eprom(std::ostream & os) const
{
    channel.to_eprom(os);
    os.write((const char*) & linger, sizeof(linger));
}


bool ShowerGuardConfig::Motion::from_eprom(std::istream & is) 
{
    channel.from_eprom(is);
    is.read((char*) & linger, sizeof(linger));
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
    uint8_t gpio_uint8 = (uint8_t) -1;
    is.read((char*) & gpio_uint8, sizeof(gpio_uint8));
    gpio = (gpio_num_t) gpio_uint8;

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
}


void ShowerGuardConfig::Rh::to_eprom(std::ostream & os) const
{
    vad.to_eprom(os);
    vdd.to_eprom(os);
}


bool ShowerGuardConfig::Rh::from_eprom(std::istream & is) 
{
    vad.from_eprom(is);
    vdd.from_eprom(is);
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
    uint8_t gpio_uint8 = (uint8_t) -1;
    is.read((char*) & gpio_uint8, sizeof(gpio_uint8));
    gpio = (gpio_num_t) gpio_uint8;

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
    uint8_t gpio_uint8 = (uint8_t) -1;
    is.read((char*) & gpio_uint8, sizeof(gpio_uint8));
    gpio = (gpio_num_t) gpio_uint8;
    return is_valid() && !is.bad();
}



