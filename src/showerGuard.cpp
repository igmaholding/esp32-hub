#include <ArduinoJson.h>
#include <showerGuard.h>
#include <gpio.h>

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

