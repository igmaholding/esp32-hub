#ifdef INCLUDE_MULTI

#include <Audio.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <multi.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <esp_log.h>
#include <time.h>
#include <i2c_utils.h>
#include <at.h>
#include <tda8425.h>
#include <rda5807.h>

#define USE_HARDWARE_SERIAL

#ifdef USE_HARDWARE_SERIAL
#include <HardwareSerial.h>
# else
#include <driver/uart.h>
#endif

#define AUDIO_CONNECTION_TIMEOUT 3000 // ms
#define AUDIO_RECONNECT_TIMEOUT  5000 // ms

// NOTE: has to overload new / delete to clean the memory because the Audio class (library)
// DOES NOT INITIALIZES ITS MEMBER VARIABLES! thus relying on that it is created statically
// in clean memory

void* operator new(size_t size)
{
    void * m = calloc(size, 1);
    return m;
}

void operator delete(void* m)
{
    if (m)
    {
        free(m);
    }
}


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
    bool r = uart.is_valid() && bt.is_valid() && fm.is_valid() && i2s.is_valid() && i2c.is_valid() && service.is_valid() && sound.is_valid();

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
    os.write((const char *)&hw, sizeof(hw));
    reset.to_eprom(os);
}

bool MultiConfig::Bt::from_eprom(std::istream &is)
{
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


bool multi_handler_uart_command_func(const String & command, AtResponse & response, String * error);
String multi_uart_command(const String & command, String & response, String * error);



class MultiHandler
{
public:

    static const unsigned LOG_FOR_STATS_INTERVAL_SECONDS = 60;
    static const int UART_BUFFER_SIZE = 1024;
    static const size_t UART_READ_WAIT_MS = 10;
    static const size_t UART_POLL_INTERVAL_MS = 1000;

    class AudioControlData
    {
    public:

        enum Source
        {
            sNone  = 0,
            sBt    = 1,
            sWww   = 2,
            sFm    = 3
        };

        AudioControlData() 
        {
            clear();
        }

        void clear() 
        {
            source = sNone;
            channel = 0;
            volume = 0;
        }

        static String source_2_str(Source _source)
        {
            switch (_source)
            {
                case sNone: return "none";
                case sBt: return "bt";
                case sWww: return "www";
                case sFm: return "fm";
                default: return "<unknown>";
            }
        }

        static Source str_2_source(const String & string)
        {
            if (source_2_str(sNone) == string)
            {
                return sNone;
            }
            
            if (source_2_str(sBt) == string)
            {
                return sBt;
            }

            if (source_2_str(sWww) == string)
            {
                return sWww;
            }

            if (source_2_str(sFm) == string)
            {
                return sFm;
            }

            return (Source) -1;
        }

        String as_string() const
        {
            return String("{source=") + source_2_str(source) + ", channel=" + String((int) channel) + 
                   ", volume=" + String((int) volume) + "}";
        }

        void to_eprom(std::ostream &os) const
        {
            os.write((const char *)&source, sizeof(source));
            os.write((const char *)&channel, sizeof(channel));
            os.write((const char *)&volume, sizeof(volume));
        }

        bool from_eprom(std::istream &is)
        {
            clear();
            is.read((char *)&source, sizeof(source));
            is.read((char *)&channel, sizeof(channel));
            is.read((char *)&volume, sizeof(volume));

            return !is.bad();
        }

        Source source;
        uint8_t channel;
        uint8_t volume;
    };


    MultiHandler()
    {
        _is_active = false;
        _task_finished = true;
        _audio_task_finished = true;
        _should_reconnect_www = false;

        #ifdef USE_HARDWARE_SERIAL
        hardware_serial = NULL;
        #else
        uart_setup_ok = false;
        #endif

        fsc_bt = NULL;

        audio_engine = NULL;

        two_wire = &Wire;
        tda8425 = NULL;
        rda5807 = NULL;

        new_delete_enabled = true;  // TODO:
    }

    ~MultiHandler()
    {
        #ifdef USE_HARDWARE_SERIAL
        delete hardware_serial;
        hardware_serial = NULL;
        #endif

        delete fsc_bt;
        fsc_bt = NULL;

        delete tda8425;
        tda8425 = NULL;

        delete rda5807;
        rda5807 = NULL;

        delete audio_engine;
        audio_engine = NULL;
    }

    bool is_active() const { return _is_active; }

    void start(const MultiConfig &config);
    void stop();
    void reconfigure(const MultiConfig &config);

    MultiStatus get_status()
    {
        MultiStatus _status;

        {
            Lock lock(semaphore);
            _status = status;
        }

        return _status;
    }

    bool audio_control(const String & source, const String & channel, const String & volume, String & response, String * error = NULL);

protected:
    void configure_uart();
    void configure_i2c();
    void configure_bt();
    void configure_fm();
    void configure_audio_engine();
    void configure_sound();

    void set_mute(bool);

    bool uart_command(const String & command, AtResponse & response, String * error = NULL);
    bool uart_poll_indications(AtResponse & response, String * error = NULL);

    static void task(void *parameter);
    static void audio_task(void *parameter);

    String choose_url();
    float choose_fm_freq();
    uint8_t get_max_volume(bool trace=false) const;
    uint8_t get_volume() const;

    void commit_volume();
    void commit_fm_freq();

    BinarySemaphore semaphore;
    BinarySemaphore new_delete_semaphore;
    bool new_delete_enabled;
    BinarySemaphore uart_semaphore;

    MultiConfig config;
    MultiStatus status;
    bool _should_reconnect_www;
    bool _is_active;
    bool _task_finished;
    bool _audio_task_finished;

    Audio * audio_engine;
    
    AudioControlData audio_control_data;

    #ifdef USE_HARDWARE_SERIAL
    HardwareSerial * hardware_serial;
    #else
    bool uart_setup_ok;
    #endif 

    FscBt * fsc_bt;
    AtLatestIndications at_latest_indications;

    TwoWire * two_wire;

    Tda8425 * tda8425;

    Rda5807 * rda5807;

    friend void audio_showstreamtitle(const char *info);
    friend void audio_bitrate(const char *info);
    friend bool multi_handler_uart_command_func(const String & command, AtResponse & response, String * error);
    friend String multi_uart_command(const String & command, String & response);
};


static MultiHandler handler;

static String esp_err_2_string(esp_err_t esp_r)
{
    char buf[128];
    esp_err_to_name_r(esp_r, buf, sizeof(buf));
    return String("ERR=") + String((int) esp_r) + ", " + buf;
}

void audio_showstreamtitle(const char *info)
{
    Lock lock(handler.semaphore);
    handler.status.title = info;
}

void audio_bitrate(const char *info)
{
    uint32_t bitrate = atol(info);
    Lock lock(handler.semaphore);
    handler.status.www.bitrate = bitrate;
}

bool multi_handler_uart_command_func(const String & command, AtResponse & response, String * error = NULL)
{
    return handler.uart_command(command, response, error);
}

void MultiHandler::start(const MultiConfig &_config)
{
    if (_is_active)
    {
        return; // already running
    }

    while(_task_finished == false  || _audio_task_finished == false)
    {
        delay(100);
    }

    Lock lock(semaphore);
    config = _config;
    configure_uart();
    configure_i2c();
    configure_bt();
    configure_fm();
    configure_audio_engine();
    configure_sound();

    _is_active = true;
    _task_finished = false;
    _audio_task_finished = false;

    xTaskCreate(
        task,                // Function that should be called
        "multi_task",        // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
    xTaskCreate(
        audio_task,          // Function that should be called
        "multi_task-audio",  // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void MultiHandler::stop()
{
    _is_active = false;

    while(_task_finished == false || _audio_task_finished == false)
    {
        delay(100);
    }
}

void MultiHandler::reconfigure(const MultiConfig &_config)
{
    TRACE("multi_task: reconfigure enters")

    MultiConfig current_config;

    {Lock lock(semaphore);
    current_config = config;}

    bool should_configure_uart = false;
    bool should_configure_i2c = false;
    bool should_configure_bt = false;
    bool should_configure_fm = false;
    bool should_configure_audio_engine = false;
    bool should_configure_sound = false;
    bool should_commit_fm_freq = false;
    bool will_reconnect_www = false;

    if (!(current_config.uart == _config.uart))
    {
        TRACE("multi_task: uart config changed")
        should_configure_uart = true;
    }

    if (!(current_config.bt == _config.bt))
    {
        TRACE("multi_task: bt config changed")
        should_configure_bt = true;
    }

    if (!(current_config.fm == _config.fm))
    {
        TRACE("multi_task: fm config changed")
        should_configure_fm = true;
    }

    if (!(current_config.i2s == _config.i2s))
    {
        TRACE("multi_task: i2s config changed")
        should_configure_audio_engine = true;
    }

    if (!(current_config.i2c == _config.i2c))
    {
        TRACE("multi_task: i2c config changed")
        should_configure_i2c = true;
    }

    if (!(current_config.sound == _config.sound))
    {
        TRACE("multi_task: sound config changed")
        should_configure_sound = true;
    }

    if (!(current_config.service == _config.service))
    {
        TRACE("multi_task: service config changed")
        will_reconnect_www = true;
        should_commit_fm_freq = true;
    }

    if (!(current_config == _config))
    {
        TRACE("multi_task: config changed")
        Lock lock(semaphore);
        config = _config;
    }

    if (should_configure_uart == true)
    {
        configure_uart();
    }

    if (should_configure_i2c == true)
    {
        configure_i2c();
    }

    if (should_configure_bt == true)
    {
        configure_bt();
    }

    if (should_configure_fm == true)
    {
        configure_fm();
    }

    if (should_configure_audio_engine == true)
    {
        configure_audio_engine();
        will_reconnect_www = true;
    }

    if (should_commit_fm_freq == true)
    {
        commit_fm_freq();
    }

    if (should_configure_sound == true)
    {
        configure_sound();
    }

    if (will_reconnect_www)
    {
        _should_reconnect_www = true;
    }

    TRACE("multi_task: reconfigure leaves")
}

void MultiHandler::task(void *parameter)
{
    MultiHandler *_this = (MultiHandler *)parameter;

    TRACE("multi_task: started")
    
    uint32_t now_millis = millis();
    uint32_t task_start_millis = now_millis;
    uint32_t wait_retry_audio_millis = 0;
    AudioControlData::Source source = AudioControlData::sNone;

    bool should_log_for_stats = true;
    uint32_t last_log_for_stats_millis = 0;

    uint32_t last_i2c_scan_millis = 0;
    const uint32_t I2C_SCAN_INTERVAL_MILLIS = 600000; // 10 minutes

    uint32_t last_check_hour_shift_millis = 0;
    const uint32_t CHECK_HOUR_SHIFT_INTERVAL_MILLIS = 60000;

    uint32_t last_uart_poll_millis = 0;

    const size_t BT_SETUP_DELAY_MS = 8000;
    bool bt_setup_done = false;

    int hour = -1;

    while (_this->_is_active)
    {
        now_millis = millis();

        if (bt_setup_done == false)
        {
            // there is no technical reason for this delay other than to be able to see log during startup

            if ((now_millis > task_start_millis && now_millis - task_start_millis >= BT_SETUP_DELAY_MS) ||
                (now_millis < task_start_millis && now_millis + (0xffffffff - task_start_millis) >= BT_SETUP_DELAY_MS))
            {
                {Lock lock(_this->new_delete_semaphore);

                if (_this->fsc_bt)
                {
                    bool r = _this->fsc_bt->module_setup();

                    if (r == false)
                    {
                        ERROR("BT module setup failed")
                    }
                    else
                    {
                        TRACE("BT module setup OK")
                    }
                }
                else
                {
                    ERROR("BT module setup failed, fsc_bt is not allocated")
                }}
            
                bt_setup_done = true;
            }
        }

        if (last_i2c_scan_millis > now_millis || (now_millis-last_i2c_scan_millis) >= I2C_SCAN_INTERVAL_MILLIS)
        {
            last_i2c_scan_millis = now_millis;
            i2c_scan(*(_this->two_wire));  // TODO: move to a separate task? or audio_engine?
        }

        if (last_check_hour_shift_millis > now_millis || (now_millis-last_check_hour_shift_millis) >= CHECK_HOUR_SHIFT_INTERVAL_MILLIS)
        {
            last_check_hour_shift_millis = now_millis;

            // check if we need to limit max volume at hour shift

            time_t _time_t;
            time(& _time_t);
            tm _tm = {0};
            _tm = *localtime(&_time_t);
        
            if (_tm.tm_year+1900 > 2000) // otherwise - NTP failed to fetch time 
            {
                if (_tm.tm_hour != hour)
                {
                    hour = _tm.tm_hour;
                    _this->commit_volume();
                }
            }
        }

        if (last_uart_poll_millis > now_millis || (now_millis-last_uart_poll_millis) >= _this->UART_POLL_INTERVAL_MS)
        {
            last_uart_poll_millis = now_millis;
            //DEBUG("uart_poll interval")

            AtResponse dummy;
            _this->uart_poll_indications(dummy);  // this will update at_poll_indications

            Lock lock(_this->semaphore);
            _this->status.bt.latest_indications = _this->at_latest_indications.indications; 
            //DEBUG("_this->status.bt.latest_indications.size() %d", (int) _this->status.bt.latest_indications.size())

            // TODO: update BT title, etc.
        }

        should_log_for_stats = false;

        AudioControlData::Source last_source = source;

        source = _this->audio_control_data.source;

        if (last_source != source)
        {
            should_log_for_stats = true;
            //DEBUG("should_log_for_stats: streaming_on")

            _this->status.www.is_streaming = _this->audio_control_data.source == AudioControlData::sWww;
            _this->status.fm.is_streaming = _this->audio_control_data.source == AudioControlData::sFm;
        }

        if (last_log_for_stats_millis > now_millis || ((now_millis-last_log_for_stats_millis)/1000) >= _this->LOG_FOR_STATS_INTERVAL_SECONDS)
        {
            should_log_for_stats = true;
            last_log_for_stats_millis = now_millis;
            //DEBUG("should_log_for_stats: interval")
        }

        // log for stats if something has changed or every LOG_FOR_STATS_INTERVAL_SECONDS seconds

        if (should_log_for_stats == true)
        {
            MultiStatus status_copy;

            { Lock lock(_this->semaphore);
            status_copy = _this->status; }
            
            String www_title;
            
            if  (status_copy.www.is_streaming)
            {
                www_title = status_copy.title;
            }
            
            TRACE("* \"www\":{\"is_streaming\":%d, \"url_index\":\"%d\", \"url_name\":\"%s\", \"bitrate\":\"%d\"}, \"volume\":%d, \"title\":\"%s\"",
                   (int)status_copy.www.is_streaming, status_copy.www.url_index, status_copy.www.url_name.c_str(), status_copy.www.bitrate, 
                   (int)status_copy.volume, www_title.c_str())

            TRACE("* \"fm\":{\"is_streaming\":%d, \"index\":\"%d\", \"name\":\"%s\", \"freq\":%f}, \"volume\":%d, \"title\":\"%s\"",
                    (int)status_copy.fm.is_streaming, status_copy.fm.index, status_copy.fm.name.c_str(), status_copy.fm.freq, 
                    (int)status_copy.volume, "")

            // TODO: BT        
         }

        delay(100);            
    }

    _this->_task_finished = true;
    TRACE("multi_task: finished")
    vTaskDelete(NULL);
}


void MultiHandler::audio_task(void *parameter)
{
    MultiHandler *_this = (MultiHandler *)parameter;

    TRACE("multi_task, audio_task: started")
    
    esp_task_wdt_deinit();
    esp_task_wdt_init(30000, true);

    uint32_t now_millis = millis();
    uint32_t wait_retry_audio_millis = 0;
    bool streaming_on = false;

    while (_this->_is_active)
    {
        now_millis = millis();
        bool last_streaming_on = streaming_on;

        streaming_on = _this->audio_control_data.source == AudioControlData::sWww;

        if (last_streaming_on != streaming_on)
        {
            TRACE("streaming_on %d", (int) streaming_on)

            if (streaming_on == false)
            {
                Lock lock(_this->semaphore);
                _this->status.www.bitrate = 0;
                _this->status.title = "";
            }
        }

        if (_this->_should_reconnect_www == true)
        {
            TRACE("multi_task: requested reconnecting stream")
            Lock lock(_this->new_delete_semaphore);
            _this->_should_reconnect_www = false;

            if (_this->audio_engine && _this->audio_engine->isRunning() == true)
            {
                _this->audio_engine->stopSong();
                TRACE("multi_task: stream disconnected (off)")
            }
        }

        if (wait_retry_audio_millis == 0 || wait_retry_audio_millis+AUDIO_RECONNECT_TIMEOUT <= now_millis || now_millis < wait_retry_audio_millis)
        {
            Lock lock(_this->new_delete_semaphore);

            if (streaming_on == true)
            {
                if (_this->audio_engine)
                {
                    if (_this->audio_engine->isRunning() == false)
                    {
                        //esp_log_level_set("*", ESP_LOG_ERROR);  
                        String url = _this->choose_url();
                        bool connect_ok = _this->audio_engine->connecttohost(url.c_str());
                        _this->audio_engine->setVolume(100);

                        TRACE("multi_task: connection attempt to URL %s, result %d", url.c_str(), (int) connect_ok)

                        if (connect_ok == false)
                        {
                            wait_retry_audio_millis = now_millis;
                            TRACE("multi_task: will retry connection in %d millis (now_millis %d) ", (int) AUDIO_RECONNECT_TIMEOUT, (int) now_millis)
                        }
                        else
                        {
                            wait_retry_audio_millis = 0;  
                            TRACE("multi_task: stream connected (on)")
                        }
                    }
                    else
                    {
                        _this->audio_engine->loop();
                        delay(0);
                    }
                }
            }
            else
            {
                if (_this->audio_engine && _this->audio_engine->isRunning() == true)
                {
                    _this->audio_engine->stopSong();
                    TRACE("multi_task: stream disconnected (off)")
                }
                delay(100);
            }
        }
        else
        {
            delay(100);
        }    
    }

    {Lock lock(_this->new_delete_semaphore);

    if (_this->audio_engine && _this->audio_engine->isRunning() == true)
    {
        _this->audio_engine->stopSong();
    }}

    _this->_audio_task_finished = true;
    TRACE("multi_task, audio_task: finished")
    vTaskDelete(NULL);
}


bool MultiHandler::audio_control(const String & source, const String & channel, const String & volume, String & response, String * error)
{
    TRACE("audio_control enters")

    AudioControlData new_audio_control_data;
    
    { Lock lock(semaphore);
    new_audio_control_data = audio_control_data; }

    DEBUG("audio_control copied data")

    if (!source.isEmpty())
    {
        new_audio_control_data.source = AudioControlData::str_2_source(source);

        if (new_audio_control_data.source == (AudioControlData::Source)-1)
        {
            char buf[80];
            sprintf(buf, "source %s is invalid", source.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
            return false;
        }
    }

    DEBUG("audio_control checked source")

    if (!channel.isEmpty())
    {
        new_audio_control_data.channel = (uint8_t) channel.toInt();
    }

    DEBUG("audio_control processed channel")

    if (!volume.isEmpty())
    {
        new_audio_control_data.volume = (uint8_t) volume.toInt();

        if (new_audio_control_data.volume > 100)
        {
            new_audio_control_data.volume = 100;
        }
    }

    DEBUG("audio_control processed volume")

    if (new_audio_control_data.source != audio_control_data.source)
    {
        TRACE("audio_control: changing source to %s", source)

        if (audio_control_data.source == AudioControlData::sBt) // changing from Bt
        {
            Lock lock(new_delete_semaphore);

            if (fsc_bt != NULL)
            {
                fsc_bt->module_enable_bt(false);
                fsc_bt->module_enable_i2s(false);
            }
        }
        else if (audio_control_data.source == AudioControlData::sWww) // changing from Www
        {
            Lock lock(new_delete_semaphore);

            if (audio_engine)
            {
                delete audio_engine;
                audio_engine = NULL;
            }

            configure_audio_engine(); // make sure to put all GPIOs in tri-state
        }
        else if (audio_control_data.source == AudioControlData::sFm) // changing from FM
        {
            Lock lock(new_delete_semaphore);

            if (rda5807)
            {
                rda5807->set_mute(true);
            }
        }

        if (new_audio_control_data.source == AudioControlData::sBt) // changing to Bt
        {
            Lock lock(new_delete_semaphore);

            if (fsc_bt != NULL)
            {
                fsc_bt->module_enable_bt(true);
                fsc_bt->module_enable_i2s(true);
            }

            if (tda8425)
            {
                tda8425->set_input(0);
            }
        }
        else if (new_audio_control_data.source == AudioControlData::sWww) // changing to Www
        {
            Lock lock(new_delete_semaphore);
            //DEBUG("free heap before audio_engine alloc %ul", (long)ESP.getFreeHeap())

            if (audio_engine)
            {
                delete audio_engine;
                audio_engine = NULL;
            }

            audio_engine = new Audio();
            //DEBUG("free heap after audio_engine alloc %ul", (long)ESP.getFreeHeap())
            configure_audio_engine(); 

            if (tda8425)
            {
                tda8425->set_input(0);
            }
        }
        else if (new_audio_control_data.source == AudioControlData::sFm) // changing to FM
        {
            Lock lock(new_delete_semaphore);

            if (rda5807)
            {
                rda5807->set_mute(false);
            }

            if (tda8425)
            {
                tda8425->set_input(1);
            }
        }
    }

    DEBUG("audio_control processed source (change)")

    bool will_reconnect_www = false;

    if (new_audio_control_data.channel != audio_control_data.channel)
    {
        if (new_audio_control_data.source == AudioControlData::sWww)
        {
            DEBUG("changing www channel to %d", (int) new_audio_control_data.channel)
            will_reconnect_www = true;
        }
    }

    response = new_audio_control_data.as_string();
    
    // update audo_control_data

    {Lock lock(semaphore);
    audio_control_data = new_audio_control_data;

    if (will_reconnect_www == true)
    {
        _should_reconnect_www = true;    
    }}

    // commit what might change from other places

    if (audio_control_data.source == AudioControlData::sFm)
    {
        commit_fm_freq();
    }

    commit_volume();

    // TODO: save to EPROM 

    TRACE("audio_control leaves")

    return true;
}


String MultiHandler::choose_url() 
{
    Lock lock(semaphore);

    int index = 0;

    if (audio_control_data.channel >= 0 && audio_control_data.channel < config.service.NUM_URLS)
    {
        index = audio_control_data.channel;
    }
    else
    {
        if (config.service.url_select >= 0)
        {
            if (config.service.url_select < config.service.NUM_URLS)
            {
                index = config.service.url_select;
            }
        }
        else
        {
            // handle enum
        }
    }

    status.www.url_index = index;
    status.www.url_name = config.service.url[index].name;
    return config.service.url[index].value;
}


float MultiHandler::choose_fm_freq() 
{
    Lock lock(semaphore);

    int index = 0;

    if (audio_control_data.channel >= 0 && audio_control_data.channel < config.service.NUM_FM_FREQS)
    {
        index = audio_control_data.channel;
    }
    else
    {
        if (config.service.fm_freq_select >= 0)
        {
            if (config.service.fm_freq_select < config.service.NUM_FM_FREQS)
            {
                index = config.service.fm_freq_select;
            }
        }
        else
        {
            // handle enum
        }
    }

    status.fm.index = index;
    status.fm.name = config.service.fm_freq[index].name;
    status.fm.freq = config.service.fm_freq[index].value;
    return config.service.fm_freq[index].value;
}


uint8_t MultiHandler::get_max_volume(bool trace) const
{
    time_t _time_t;
    time(& _time_t);
    tm _tm = {0};
    _tm = *localtime(&_time_t);

    if (_tm.tm_year+1900 > 2000) // otherwise - NTP failed to fetch time 
    {
        uint8_t hour = (uint8_t)((size_t) _tm.tm_hour % (sizeof(config.sound.schedule)/sizeof(config.sound.schedule[0])));

        if (trace)
        {
            TRACE("get_max_volume, datetime is valid, _tm.tm_hour %d, hour %d, schedule[hour] %d", (int) _tm.tm_hour, (int) hour, (int)config.sound.schedule[hour])
        }

        switch(config.sound.schedule[hour])
        {
            case MultiConfig::Sound::smOff:
                if (trace)
                {
                    TRACE("get_max_volume, returning volume 0, smOff")
                }
                return 0;
            case MultiConfig::Sound::smOn:
                if (trace)
                {
                    TRACE("get_max_volume, returning volume %d, smOn", (int) config.sound.volume)
                }
                return config.sound.volume;
            case MultiConfig::Sound::smOnLowVolume:
                if (trace)
                {
                    TRACE("get_max_volume, returning volume_low %d, smLowVolume", (int) config.sound.volume_low)
                }
                return config.sound.volume_low;
            default:
                break;    
        }
    }
    else
    {
        if (trace)
        {
            TRACE("get_max_volume, datetime invalid, _tm.tm_year %d", (int) _tm.tm_year)
        }
    }
    
    if (trace)
    {
        TRACE("get_max_volume, returning default, not by schedule")
    }
    return config.sound.volume;
}


uint8_t MultiHandler::get_volume() const
{
    uint8_t volume = audio_control_data.volume;
    uint8_t max_volume = get_max_volume();

    if (volume <= max_volume)
    {
        return volume;
    }

    return max_volume;
}


void MultiHandler::commit_volume()
{
    Lock lock(new_delete_semaphore);
    
    status.volume = get_volume();

    if (tda8425)
    {
        tda8425->set_volume(status.volume);
    }
}


void MultiHandler::commit_fm_freq()
{
    float freq = 0;
    {Lock lock(semaphore);
    freq = choose_fm_freq();}

    Lock lock(new_delete_semaphore);

    if (rda5807)
    {
        rda5807->set_freq(freq);
    }
}


void MultiHandler::configure_audio_engine()
{
    TRACE("multi_task: configure_audio_engine")
    Lock lock(new_delete_semaphore);
    
    if (audio_engine)
    {
        audio_engine->setConnectionTimeout(AUDIO_CONNECTION_TIMEOUT, AUDIO_CONNECTION_TIMEOUT);
        audio_engine->setPinout(config.i2s.bclk.gpio, config.i2s.lrc.gpio, config.i2s.dout.gpio);

        // audio_engine->forceMono(true);
        // configure_sound();
    }
    else
    {
        // put GPIOs to tri-state to allow bt to take over i2s DAC
        pinMode(config.i2s.bclk.gpio, INPUT);
        pinMode(config.i2s.lrc.gpio, INPUT);
        pinMode(config.i2s.dout.gpio, INPUT);
    }

    TRACE("multi_task: configure_audio_engine done")
}

#ifndef USE_HARDWARE_SERIAL

static uart_port_t uart_num_2_port(uint8_t uart_num)
{
    return uart_num == 0 ? UART_NUM_0 : (uart_num == 1 ? UART_NUM_1 : UART_NUM_2);
}

#endif

void MultiHandler::configure_uart()
{
    TRACE("multi_task: configure_uart")
    
    Lock lock(uart_semaphore);

    #ifdef USE_HARDWARE_SERIAL
    
    if (hardware_serial != NULL)
    {
        delete hardware_serial;
        hardware_serial = NULL;
    }
    
    DEBUG("creating hardware_serial for uart_num %d", (int) config.uart.uart_num)
    hardware_serial = new HardwareSerial(config.uart.uart_num);    
    hardware_serial->begin(115200, SERIAL_8N1, config.uart.rx.gpio, config.uart.tx.gpio);
        
    #else

    uart_setup_ok = false;
    const uart_port_t uart_num = uart_num_2_port(config.uart.uart_num);

    uart_driver_delete(uart_num);

    // Install UART driver using an event queue here
    esp_err_t esp_r = uart_driver_install(uart_num, UART_BUFFER_SIZE, 0, 0, NULL,0); 

    if (esp_r != ESP_OK)
    {
        status.status = String("failed to install uart driver, ") + esp_err_2_string(esp_r); 
        ERROR(status.status.c_str())
        // but continue anyway
    }
    else
    {
        TRACE("install uart driver OK")
    }

    uart_config_t uart_config = {
    
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl =  UART_HW_FLOWCTRL_DISABLE, 
    .rx_flow_ctrl_thresh = 120,
    .source_clk = UART_SCLK_APB
    };

    esp_r = uart_param_config(uart_num, &uart_config);

    if (esp_r != ESP_OK)
    {
        status.status = String("failed to configure uart, ") + esp_err_2_string(esp_r); 
        ERROR(status.status.c_str())
    }
    else
    {
        TRACE("configure uart OK")

        esp_r = uart_set_pin(uart_num, config.uart.tx.gpio, config.uart.rx.gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

        if (esp_r != ESP_OK)
        {
            status.status = String("failed to configure uart gpio, ") + esp_err_2_string(esp_r); 
            ERROR(status.status.c_str())
        }
        else
        {
            TRACE("configure uart gpio OK")
            uart_setup_ok = true;
        }
    }
    
    #endif

    TRACE("multi_task: configure_uart done")
}


void MultiHandler::configure_i2c()
{
    TRACE("multi_task: configure_i2c")
    
    if (two_wire)
    {
        two_wire->end();
        two_wire->begin(config.i2c.sda.gpio, config.i2c.scl.gpio);
        i2c_scan(*two_wire);
    }

    TRACE("multi_task: configure_i2c done")
}

void MultiHandler::configure_bt()
{
    TRACE("multi_task: configure_bt")
    
    {Lock lock(new_delete_semaphore);
    
    if (fsc_bt != NULL)
    {
        delete fsc_bt;
        fsc_bt = NULL;
    }

    fsc_bt = new FscBt(config.bt.hw);}

    fsc_bt->install_uart_command_func(multi_handler_uart_command_func);
}


void MultiHandler::configure_fm()
{
    TRACE("multi_task: configure_fm")
    
    Lock lock(new_delete_semaphore);
    
    if (config.fm.hw != "RDA5807")
    {
        if (rda5807)
        {
            delete rda5807;
            rda5807 = NULL;
        }
    }
    else if (config.fm.hw == "RDA5807")
    {
        if (rda5807 == NULL)
        {
            rda5807 = new Rda5807(two_wire);
        }
    }
    
    if (rda5807)
    {
        rda5807->set_addr(config.fm.addr.c_str());
        rda5807->commit_all();
    }
}


void MultiHandler::configure_sound()
{
    TRACE("multi_task: configure_sound")

    Lock lock(new_delete_semaphore);

    if (config.sound.hw != "TDA8425")
    {
        if (tda8425)
        {
            delete tda8425;
            tda8425 = NULL;
        }
    }
    else if (config.sound.hw == "TDA8425")
    {
        if (tda8425 == NULL)
        {
            tda8425 = new Tda8425(two_wire);
        }
    }
    
    if (tda8425)
    {
        tda8425->set_addr(config.sound.addr.c_str());

        int gain_low_pass = config.sound.gain_low_pass - config.sound.gain_band_pass;
        int gain_high_pass = config.sound.gain_high_pass - config.sound.gain_band_pass;

        tda8425->set_bass(gain_low_pass);
        tda8425->set_treble(gain_high_pass);

        tda8425->commit_all();
    }

    pinMode(config.sound.mute.gpio, OUTPUT);

    /*
    if (audio_engine)
    {
        audio_engine->setTone(config.sound.gain_low_pass, config.sound.gain_band_pass, config.sound.gain_high_pass);
    }
    */

    get_max_volume(true);  // trace selection once
    commit_volume();

    TRACE("multi_task: configure_sound done")
}


void MultiHandler::set_mute(bool is_mute)
{
    int gpio_state = config.sound.mute.inverted ? (is_mute ? 0 : 1) : (is_mute ? 1 : 0);
    digitalWrite(config.sound.mute.gpio, gpio_state);
}


bool MultiHandler::uart_command(const String & command, AtResponse & response, String * error)
{
    char crlf[] = {0x0d, 0x0a, 0};
    String command_crlf = command + (const char *) crlf;
    TRACE("¤ %s", command.c_str())

    #ifdef USE_HARDWARE_SERIAL
    
    if (hardware_serial)
    {
        { Lock lock(uart_semaphore);
        hardware_serial->write(command_crlf.c_str(), command_crlf.length());

        response.start_timing();

        while ((hardware_serial->available() || !response.is_ready()) && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf);

            length = hardware_serial->read(buf, length); // hej

            if (length > 0)
            {
                response.feed((const char*) buf, length);
            }

            if (!hardware_serial->available())
            {
                delay(UART_READ_WAIT_MS);
            }
        }}

        if (response.is_timeout())
        {
            char buf[80];
            sprintf(buf, "AT command %s timeout", command.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }
        
        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return response.is_ready();
    }

    #else
    
    if (uart_setup_ok == true)
    {
        const uart_port_t uart_num = uart_num_2_port(config.uart.uart_num);

        { Lock lock(uart_semaphore);

        int bytes_written = 
        uart_write_bytes(uart_num, (const char*)command_crlf.c_str(), command_crlf.length());

        if (bytes_written < 0)
        {
            char buf[80];
            sprintf(buf, "uart_write_bytes parameter error, command %s", command.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }

            return false;
        }

        if (bytes_written < command_crlf.length())
        {
            char buf[80];
            sprintf(buf, "uart_write_bytes written %d bytes, expected %d", bytes_written, (int) command_crlf.length());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }

        response.start_timing();
            
        while (!response.is_ready() && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf)-1;

            //DEBUG("about to uart_read_bytes, length %d", length)

            length = uart_read_bytes(uart_num, buf, length, 100);

            //DEBUG("uart_read_bytes returned length %d", length)

            if (length < 0)
            {
                status.status = String("failed uart_read_bytes"); 
                ERROR(status.status.c_str())

                if (error != NULL)
                {
                    *error = status.status;
                }
            }
            else
            if (length > 0)
            {
                //buf[length] = 0;
                //DEBUG("uart_read_bytes: %s", buf)
                response.feed((const char*) buf, length);
            }

            delay(UART_READ_WAIT_MS);
        }}

        if (response.is_timeout())
        {
            char buf[80];
            sprintf(buf, "AT command %s timeout", command.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }

        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return response.is_ready();        
    }

    #endif

    return false; 
}


bool MultiHandler::uart_poll_indications(AtResponse & response, String * error)
{
    #ifdef USE_HARDWARE_SERIAL
    
    if (hardware_serial)
    {
        { Lock lock(uart_semaphore);

        response.start_timing();

        while ((hardware_serial->available() || response.is_line_pending()) && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf);

            length = hardware_serial->read(buf, length); // hej

            if (length > 0)
            {
                response.feed((const char*) buf, length);
            }

            if (!hardware_serial->available())
            {
                delay(UART_READ_WAIT_MS);
            }
        }}

        if (response.is_timeout())
        {
            const char * buf = "AT poll timeout";
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }
        
        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return true;
    }

    #else
    
    if (uart_setup_ok == true)
    {
        const uart_port_t uart_num = uart_num_2_port(config.uart.uart_num);

        { Lock lock(uart_semaphore);
        int length = 0;

        if (uart_get_buffered_data_len(uart_num, (size_t*)&length) == ESP_FAIL)
        {
            ERROR("Failed to uart_get_buffered_data_len")
        }
        else
        {
            if (length == 0)
            {
                // nothing is waiting so return immediately
                return true;
            }
        }
        
        response.start_timing();
            
        while (response.is_line_pending() && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf)-1;

            //DEBUG("about to uart_read_bytes, length %d", length)

            length = uart_read_bytes(uart_num, buf, length, 100);

            //DEBUG("uart_read_bytes returned length %d", length)

            if (length < 0)
            {
                status.status = String("failed uart_read_bytes"); 
                ERROR(status.status.c_str())

                if (error != NULL)
                {
                    *error = status.status;
                }
            }
            else
            if (length > 0)
            {
                //buf[length] = 0;
                //DEBUG("uart_read_bytes: %s", buf)
                response.feed((const char*) buf, length);
            }

            delay(UART_READ_WAIT_MS);
        }}

        if (response.is_timeout())
        {
            const char * buf = "AT poll timeout";
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }

        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return true;
    }

    #endif

    return false; 
}


void start_multi_task(const MultiConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start multi_task while it is running, redirecting to reconfigure")
        reconfigure_multi(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_multi_task()
{
    handler.stop();
}

void reconfigure_multi(const MultiConfig &_config)
{
    handler.reconfigure(_config);
}

String multi_uart_command(const String & command, String & response)
{
    if (!command.isEmpty())
    {
        AtResponse at_response;
        String r;

        if (handler.uart_command(command, at_response, & r))
        {
            response = at_response.to_string();
            return "";
        }
        else
        {
            return r;
        }        
    }
    
    return "Parameter error";
}


String multi_audio_control(const String & source, const String & channel, const String & volume, String & response)
{
    String error;

    if (handler.audio_control(source, channel, volume, response, & error))
    {
        return "";
    }
    else
    {
        return error;
    }        
}

MultiStatus get_multi_status()
{
    return handler.get_status();
}

#endif // INCLUDE_MULTI