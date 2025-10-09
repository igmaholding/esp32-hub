#ifdef INCLUDE_RFIDLOCK

// #define INCLUDE_PN532 1

#include <Arduino.h>
#include <ArduinoJson.h>

#include <rfidLock.h>
#include <gpio.h>
#include <trace.h>

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

bool RfidLockConfig::is_valid() const
{
    bool r = rfid.is_valid() && lock.is_valid() && buzzer.is_valid() && green_led.is_valid() && red_led.is_valid();

    if (r == false)
    {
        return false;
    }

    GpioCheckpad checkpad;


    String object_name = "rfid.resetPowerDownPin";

    if (rfid.resetPowerDownPin != UNUSED_PIN)
    {
        if (checkpad.get_usage(rfid.resetPowerDownPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)rfid.resetPowerDownPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.resetPowerDownPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)rfid.resetPowerDownPin);
            return false;
        }
    }

    if (rfid.protocol == Rfid::pSPI)
    {
        object_name = "rfid.chipSelectPin";

        if (checkpad.get_usage(rfid.chipSelectPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)rfid.chipSelectPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.chipSelectPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)rfid.chipSelectPin);
            return false;
        }

        object_name = "rfid.mosiPin";

        if (checkpad.get_usage(rfid.mosiPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)rfid.mosiPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.mosiPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)rfid.mosiPin);
            return false;
        }

        object_name = "rfid.misoPin";

        if (checkpad.get_usage(rfid.misoPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)rfid.misoPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.misoPin, GpioCheckpad::uDigitalInput))
        {
            _err_cap(object_name.c_str(), (int)rfid.misoPin);
            return false;
        }

        object_name = "rfid.sckPin";

        if (checkpad.get_usage(rfid.sckPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)rfid.sckPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.sckPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)rfid.sckPin);
            return false;
        }
    }

    if (rfid.protocol == Rfid::pI2C)
    {
        object_name = "rfid.sclPin";

        if (checkpad.get_usage(rfid.sclPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)rfid.sclPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.sclPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)rfid.sclPin);
            return false;
        }

        object_name = "rfid.sdaPin";

        if (checkpad.get_usage(rfid.sdaPin) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)rfid.sdaPin);
            return false;
        }

        if (!checkpad.set_usage(rfid.sdaPin, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)rfid.sdaPin);
            return false;
        }
    }

    size_t i = 0;

    for (auto it = lock.channels.begin(); it != lock.channels.end(); ++it, ++i)
    {    
        String _object_name = String("lock.channel[") + it->first + "].gpio";

        if (checkpad.get_usage(it->second.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(_object_name.c_str(), (int)it->second.gpio);
            return false;
        }

        if (!checkpad.set_usage(it->second.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(_object_name.c_str(), (int)it->second.gpio);
            return false;
        }
    }
    
    object_name = "buzzer.channel.gpio";

    if (checkpad.get_usage(buzzer.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)buzzer.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(buzzer.channel.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)buzzer.channel.gpio);
        return false;
    }

    for (size_t i=0; i<sizeof(keypad.c)/sizeof(keypad.c[0]); ++i)
    {
        object_name = "keypad.c[" + String(i) + "]";

        if (keypad.c[i].is_valid())
        {
            if (checkpad.get_usage(keypad.c[i].gpio) != GpioCheckpad::uNone)
            {
                _err_dup(object_name.c_str(), (int)keypad.c[i].gpio);
                return false;
            }

            if (!checkpad.set_usage(keypad.c[i].gpio, GpioCheckpad::uDigitalOutput))
            {
                _err_cap(object_name.c_str(), (int)keypad.c[i].gpio);
                return false;
            }
        }
    }

    for (size_t i=0; i<sizeof(keypad.l)/sizeof(keypad.l[0]); ++i)
    {
        object_name = "keypad.l[" + String(i) + "]";

        if (checkpad.get_usage(keypad.l[i].gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)keypad.l[i].gpio);
            return false;
        }

        if (!checkpad.set_usage(keypad.l[i].gpio, GpioCheckpad::uDigitalInput))
        {
            _err_cap(object_name.c_str(), (int)keypad.l[i].gpio);
            return false;
        }
    }

    object_name = "green_led.gpio";

    if (checkpad.get_usage(green_led.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)green_led.gpio);
        return false;
    }

    if (!checkpad.set_usage(green_led.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)green_led.gpio);
        return false;
    }

    object_name = "red_led.gpio";

    if (checkpad.get_usage(red_led.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)red_led.gpio);
        return false;
    }

    if (!checkpad.set_usage(red_led.gpio, GpioCheckpad::uDigitalOutput))
    {
        _err_cap(object_name.c_str(), (int)red_led.gpio);
        return false;
    }

    return true;
}

void RfidLockConfig::from_json(const JsonVariant &json)
{
    if (json.containsKey("rfid"))
    {
        const JsonVariant &_json = json["rfid"];
        rfid.from_json(_json);
    }

    if (json.containsKey("lock"))
    {
        const JsonVariant &_json = json["lock"];
        lock.from_json(_json);
    }

    if (json.containsKey("buzzer"))
    {
        const JsonVariant &_json = json["buzzer"];
        buzzer.from_json(_json);
    }

    if (json.containsKey("keypad"))
    {
        const JsonVariant &_json = json["keypad"];
        keypad.from_json(_json);
    }

    if (json.containsKey("green_led"))
    {
        const JsonVariant &_json = json["green_led"];
        green_led.from_json(_json);
    }

    if (json.containsKey("red_led"))
    {
        const JsonVariant &_json = json["red_led"];
        red_led.from_json(_json);
    }
}

void RfidLockConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    rfid.to_eprom(os);
    lock.to_eprom(os);
    buzzer.to_eprom(os);
    keypad.to_eprom(os);
    green_led.to_eprom(os);
    red_led.to_eprom(os);
}

bool RfidLockConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        rfid.from_eprom(is);
        lock.from_eprom(is);
        buzzer.from_eprom(is);
        keypad.from_eprom(is);
        green_led.from_eprom(is);
        red_led.from_eprom(is);
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read RfidLockConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void RfidLockConfig::Rfid::from_json(const JsonVariant &json)
{
    if (json.containsKey("protocol"))
    {
        protocol = str_2_protocol(json["protocol"]);
    }

    if (json.containsKey("hw"))
    {
        hw = str_2_hw(json["hw"]);
    }

    if (json.containsKey("resetPowerDownPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["resetPowerDownPin"]);
        resetPowerDownPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("chipSelectPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["chipSelectPin"]);
        chipSelectPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("mosiPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["mosiPin"]);
        mosiPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("misoPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["misoPin"]);
        misoPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("sckPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["sckPin"]);
        sckPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("sclPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["sclPin"]);
        sclPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("sdaPin"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["sdaPin"]);
        sdaPin = GpioChannel::validateGpioNum(gpio_unvalidated);
    }

    if (json.containsKey("i2cAddress"))
    {
        i2cAddress = (uint8_t) (int) json["i2cAddress"];
    }
}

void RfidLockConfig::Rfid::to_eprom(std::ostream &os) const
{
    os.write((const char *)&protocol, sizeof(protocol));
    os.write((const char *)&hw, sizeof(hw));

    uint8_t gpio_uint8 = (uint8_t)resetPowerDownPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));
    
    gpio_uint8 = (uint8_t)chipSelectPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    gpio_uint8 = (uint8_t)mosiPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    gpio_uint8 = (uint8_t)misoPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    gpio_uint8 = (uint8_t)sckPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    gpio_uint8 = (uint8_t)sclPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    gpio_uint8 = (uint8_t)sdaPin;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    os.write((const char *)&i2cAddress, sizeof(i2cAddress));
}

bool RfidLockConfig::Rfid::from_eprom(std::istream &is)
{
    is.read((char *)&protocol, sizeof(protocol));
    is.read((char *)&hw, sizeof(hw));

    int8_t gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    resetPowerDownPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    chipSelectPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    misoPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    mosiPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    sckPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    sclPin = (gpio_num_t)gpio_int8;

    gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    sdaPin = (gpio_num_t)gpio_int8;

    is.read((char *)&i2cAddress, sizeof(i2cAddress));

    return is_valid() && !is.bad();
}

void RfidLockConfig::Lock::from_json(const JsonVariant &json)
{
    channels.clear();

     if (json.containsKey("channels"))
    {
        const JsonVariant &_json = json["channels"];
        const JsonObject & _json_object = _json.as<JsonObject>();
        
        for(auto iterator = _json_object.begin(); iterator != _json_object.end(); ++iterator) 
        {
            const JsonVariant & __json = iterator->value().as<JsonVariant>();
            RelayChannelConfig channel;
            channel.from_json(__json);

            channels.push_back(std::make_pair(String(iterator->key().c_str()), channel));
        }
    }

    if (json.containsKey("linger"))
    {
        linger = (unsigned)((int)json["linger"]);
    }
}

void RfidLockConfig::Lock::to_eprom(std::ostream &os) const
{
    uint8_t count = channels.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it = channels.begin(); it != channels.end(); ++it)
    {
        uint8_t len = (uint8_t) it->first.length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->first.c_str(), len);

        it->second.to_eprom(os);
    }

    os.write((const char *)&linger, sizeof(linger));
}

bool RfidLockConfig::Lock::from_eprom(std::istream &is)
{
    channels.clear();

    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        char buf[256];
        uint8_t len = 0;
        is.read((char *)&len, sizeof(len));
        is.read((char *)buf, len);
        buf[len]=0;

        RelayChannelConfig channel;
        channel.from_eprom(is);

        channels.push_back(std::make_pair(String(buf), channel));
    }

    is.read((char *)&linger, sizeof(linger));

    return is_valid() && !is.bad();
}

void RfidLockConfig::Codes::from_json(const JsonVariant &json)
{ 
    // codes are managed via separate URLs
}

void RfidLockConfig::Codes::to_json(JsonVariant & json)
{
    json.createNestedArray("codes");
    JsonArray json_array = json["codes"]; 

    int i = 0;
    
    for (auto it=codes.begin(); it!=codes.end(); ++it, ++i)
    {
        DynamicJsonDocument doc(1024);
        JsonVariant item = doc.to<JsonVariant>();
        
        it->second.to_json(item);

        item["name"] = it->first;
        item["index"] = i;

        json_array.add(item);
    }
}

void RfidLockConfig::Codes::to_eprom(std::ostream &os) const
{ 
    uint8_t count = codes.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it = codes.begin(); it != codes.end(); ++it)
    {
        uint8_t len = (uint8_t) it->first.length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->first.c_str(), len);

        it->second.to_eprom(os);
    }
}
    
bool RfidLockConfig::Codes::from_eprom(std::istream &is)
{ 
    codes.clear();

    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        char buf[256];
        uint8_t len = 0;
        is.read((char *)&len, sizeof(len));
        is.read((char *)buf, len);
        buf[len]=0;

        Code code;
        code.from_eprom(is);

        codes.insert(std::make_pair(String(buf), code));
    }

    return is_valid() && !is.bad();
}

void RfidLockConfig::Codes::Code::from_json(const JsonVariant &json)
{
    // codes are managed via separate URLs
}

void RfidLockConfig::Codes::Code::to_json(JsonVariant & json)
{
    json["type"] = type_2_str(type);
    json["value"] = value;

    json.createNestedArray("locks");
    JsonArray json_array_locks = json["locks"]; 

    for (auto it=locks.begin(); it!=locks.end(); ++it)
    {
        json_array_locks.add(*it);
    }
}

void RfidLockConfig::Codes::Code::to_eprom(std::ostream &os) const
{
    uint8_t len = value.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(value.c_str(), len);
    }

    uint8_t count = locks.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it = locks.begin(); it != locks.end(); ++it)
    {
        uint8_t len = (uint8_t) it->length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->c_str(), len);
    }

    uint8_t type_uint8_t = (uint8_t) type;
    os.write((const char *)&type_uint8_t, sizeof(type_uint8_t));
}

bool RfidLockConfig::Codes::Code::from_eprom(std::istream &is)
{
    uint8_t len = 0;
    char buf[256];

    is.read((char *)&len, sizeof(len));

    if (len)
    {
        is.read(buf, len);
        buf[len] = 0;
        value = buf;
    }
    else
    {
        value = "";
    }

    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        is.read((char *)&len, sizeof(len));
        is.read((char *)buf, len);
        buf[len]=0;

        locks.push_back(buf);
    }

    uint8_t type_uint8_t = 0;
    is.read((char *)&type_uint8_t, sizeof(type_uint8_t));
    type = (Type) type_uint8_t;

    return is_valid() && !is.bad();
}



#endif // INCLUDE_RFIDLOCK
