#ifdef INCLUDE_AUDIO

#include <Audio.h>
#include <ArduinoJson.h>
#include <aaudio.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <esp_log.h>
#include <time.h>

#define AUDIO_CONNECTION_TIMEOUT 3000 // ms
#define AUDIO_RECONNECT_TIMEOUT  5000 // ms

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

bool AudioConfig::is_valid() const
{
    bool r = motion.is_valid() && i2s.is_valid() && service.is_valid() && sound.is_valid();

    if (r == false)
    {
        return false;
    }

    GpioCheckpad checkpad;

    String object_name = "motion.channel.gpio";

    if (checkpad.get_usage(motion.channel.gpio) != GpioCheckpad::uNone)
    {
        _err_dup(object_name.c_str(), (int)motion.channel.gpio);
        return false;
    }

    if (!checkpad.set_usage(motion.channel.gpio, GpioCheckpad::uDigitalInput))
    {
        _err_cap(object_name.c_str(), (int)motion.channel.gpio);
        return false;
    }

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

    if (onoff == onoffOff || onoff == onoffOn || onoff == onoffMotion)
    {

    }
    else
    {
        ERROR("Illegal value of onoff %d", (int) onoff)
        return false;
    }

    // the delay is possible to ingest regardless of the values

    return true;
}

void AudioConfig::from_json(const JsonVariant &json)
{
    if (json.containsKey("motion"))
    {
        const JsonVariant &_json = json["motion"];
        motion.from_json(_json);
    }

    if (json.containsKey("i2s"))
    {
        const JsonVariant &_json = json["i2s"];
        i2s.from_json(_json);
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

    if (json.containsKey("onoff"))
    {
        onoff = (OnOff) (int) json["onoff"];
    }

    if (json.containsKey("delay"))
    {
        delay = (uint32_t)((int)json["delay"]);
    }
}

void AudioConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));
    motion.to_eprom(os);
    i2s.to_eprom(os);
    service.to_eprom(os);
    sound.to_eprom(os);

    uint8_t onoff_uint8 = (uint8_t)onoff;
    os.write((const char *)&onoff_uint8, sizeof(onoff_uint8));

    os.write((const char *)&delay, sizeof(delay));
}

bool AudioConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        motion.from_eprom(is);
        i2s.from_eprom(is);
        service.from_eprom(is);
        sound.from_eprom(is);

        uint8_t onoff_uint8 = (uint8_t) onoffOff;
        is.read((char *)&onoff_uint8, sizeof(onoff_uint8));
        onoff = (OnOff)onoff_uint8;

        is.read((char *)&delay, sizeof(delay));

        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read AudioConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void AudioConfig::Motion::from_json(const JsonVariant &json)
{
    if (json.containsKey("channel"))
    {
        const JsonVariant &_json = json["channel"];
        channel.from_json(_json);
    }
}

void AudioConfig::Motion::to_eprom(std::ostream &os) const
{
    channel.to_eprom(os);
}

bool AudioConfig::Motion::from_eprom(std::istream &is)
{
    channel.from_eprom(is);
    return is_valid() && !is.bad();
}

void AudioConfig::Motion::Channel::from_json(const JsonVariant &json)
{
    if (json.containsKey("gpio"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["gpio"]);
        gpio = GpioChannel::validateGpioNum(gpio_unvalidated);
    }
    if (json.containsKey("inverted"))
    {
        inverted = json["inverted"];
    }
    if (json.containsKey("debounce"))
    {
        debounce = (unsigned)((int)json["debounce"]);
    }
}

void AudioConfig::Motion::Channel::to_eprom(std::ostream &os) const
{
    uint8_t gpio_uint8 = (uint8_t)gpio;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));

    uint8_t inverted_uint8 = (uint8_t)inverted;
    os.write((const char *)&inverted_uint8, sizeof(inverted_uint8));

    os.write((const char *)&debounce, sizeof(debounce));
}

bool AudioConfig::Motion::Channel::from_eprom(std::istream &is)
{
    int8_t gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    gpio = (gpio_num_t)gpio_int8;

    uint8_t inverted_uint8 = (uint8_t) false;
    is.read((char *)&inverted_uint8, sizeof(inverted_uint8));
    inverted = (bool)inverted_uint8;

    is.read((char *)&debounce, sizeof(debounce));
    return is_valid() && !is.bad();
}

void AudioConfig::I2s::from_json(const JsonVariant &json)
{
    if (json.containsKey("dout"))
    {
        const JsonVariant &_json = json["dout"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &__json = _json["channel"];
            dout.from_json(__json);
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


void AudioConfig::I2s::to_eprom(std::ostream &os) const
{
    dout.to_eprom(os);
    bclk.to_eprom(os);
    lrc.to_eprom(os);
}

bool AudioConfig::I2s::from_eprom(std::istream &is)
{
    dout.from_eprom(is);
    bclk.from_eprom(is);
    lrc.from_eprom(is);

    return is_valid() && !is.bad();
}

void AudioConfig::I2s::Channel::from_json(const JsonVariant &json)
{
    if (json.containsKey("gpio"))
    {
        unsigned gpio_unvalidated = (unsigned)((int)json["gpio"]);
        gpio = GpioChannel::validateGpioNum(gpio_unvalidated);
    }
}

void AudioConfig::I2s::Channel::to_eprom(std::ostream &os) const
{
    uint8_t gpio_uint8 = (uint8_t)gpio;
    os.write((const char *)&gpio_uint8, sizeof(gpio_uint8));
}

bool AudioConfig::I2s::Channel::from_eprom(std::istream &is)
{
    int8_t gpio_int8 = (int8_t)-1;
    is.read((char *)&gpio_int8, sizeof(gpio_int8));
    gpio = (gpio_num_t)gpio_int8;

    return is_valid() && !is.bad();
}


void AudioConfig::Service::from_json(const JsonVariant &json)
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

}

void AudioConfig::Service::to_eprom(std::ostream &os) const
{
    for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
    {
        url[i].to_eprom(os);
    }

    os.write((const char *)&url_select, sizeof(url_select));
}

bool AudioConfig::Service::from_eprom(std::istream &is)
{
    for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
    {
        url[i].from_eprom(is);
    }

    is.read((char *)&url_select, sizeof(url_select));

    return is_valid() && !is.bad();
}

void AudioConfig::Service::Url::from_json(const JsonVariant &json)
{
    if (json.containsKey("value"))
    {
        value = (const char *)json["value"];
    }
}

void AudioConfig::Service::Url::to_eprom(std::ostream &os) const
{
    uint8_t len = value.length();
    os.write((const char *)&len, sizeof(len));

    if (len)
    {
        os.write(value.c_str(), len);
    }
}

bool AudioConfig::Service::Url::from_eprom(std::istream &is)
{
    uint8_t len = 0;

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


void AudioConfig::Sound::from_json(const JsonVariant &json)
{
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


void AudioConfig::Sound::to_eprom(std::ostream &os) const
{
    os.write((const char *)&volume, sizeof(volume));
    os.write((const char *)&volume_low, sizeof(volume_low));

    os.write((const char *)&gain_low_pass, sizeof(gain_low_pass));
    os.write((const char *)&gain_band_pass, sizeof(gain_band_pass));
    os.write((const char *)&gain_high_pass, sizeof(gain_high_pass));

    os.write((const char *)schedule, sizeof(schedule));
}

bool AudioConfig::Sound::from_eprom(std::istream &is)
{
    is.read((char *)&volume, sizeof(volume));
    is.read((char *)&volume_low, sizeof(volume_low));

    is.read((char *)&gain_low_pass, sizeof(gain_low_pass));
    is.read((char *)&gain_band_pass, sizeof(gain_band_pass));
    is.read((char *)&gain_high_pass, sizeof(gain_high_pass));

    is.read((char *)schedule, sizeof(schedule));

    return is_valid() && !is.bad();
}

class AudioHandler
{
public:

    static const unsigned MOTION_HYS_MILLIS = 7000;

    AudioHandler()
    {
        _is_active = false;
        _is_finished = true;
        _should_reconnect = false;
    }

    bool is_active() const { return _is_active; }

    void start(const AudioConfig &config);
    void stop();
    void reconfigure(const AudioConfig &config);
    void actuate(size_t channel_number);

    AudioStatus get_status()
    {
        AudioStatus _status;

        {
            Lock lock(semaphore);
            _status = status;
        }

        return _status;
    }

protected:
    void configure_audio();
    void configure_sound();
    static void configure_hw_motion(const AudioConfig::Motion &motion);

    static bool read_motion(const AudioConfig::Motion &motion);

    static void task(void *parameter);

    String choose_url();
    uint8_t get_volume(bool trace=false) const;

    BinarySemaphore semaphore;
    AudioConfig config;
    AudioStatus status;
    bool _should_reconnect;
    bool _is_active;
    bool _is_finished;

    Audio engine;

    friend void audio_showstreamtitle(const char *info);
    friend void audio_bitrate(const char *info);

};


static AudioHandler handler;


void audio_showstreamtitle(const char *info)
{
    Lock lock(handler.semaphore);
    handler.status.title = info;
}
void audio_bitrate(const char *info)
{
    uint32_t bitrate = atol(info);
    Lock lock(handler.semaphore);
    handler.status.bitrate = bitrate;
}


void AudioHandler::start(const AudioConfig &_config)
{
    if (_is_active)
    {
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    Lock lock(semaphore);
    config = _config;
    configure_audio();
    configure_hw_motion(config.motion);

    _is_active = true;
    _is_finished = false;

    xTaskCreate(
        task,                // Function that should be called
        "audio_task",        // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void AudioHandler::stop()
{
    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }
}

void AudioHandler::reconfigure(const AudioConfig &_config)
{
    Lock lock(semaphore);

    bool should_configure_audio = false;
    bool should_configure_sound = false;
    bool should_configure_motion = false;

    if (!(config.motion == _config.motion))
    {
        TRACE("audio_task: motion config changed")
        should_configure_motion = true;
    }

    if (!(config.i2s == _config.i2s))
    {
        TRACE("audio_task: i2s config changed")
        should_configure_audio = true;
    }

    if (!(config.sound == _config.sound))
    {
        TRACE("audio_task: sound config changed")
        should_configure_sound = true;
    }

    if (!(config.service == _config.service))
    {
        TRACE("audio_task: service config changed")
        _should_reconnect = true;
    }

    if (!(config == _config))
    {
        TRACE("audio_task: config changed")
        config = _config;
    }

    if (should_configure_audio == true)
    {
        configure_audio();
        _should_reconnect = true;
    }

    if (should_configure_sound == true  && should_configure_audio == false)
    {
        configure_sound();
    }

    if (should_configure_motion == true)
    {
        configure_hw_motion(config.motion);
    }
}


void AudioHandler::task(void *parameter)
{
    AudioHandler *_this = (AudioHandler *)parameter;

    TRACE("audio_task: started")
    
    uint32_t now_millis = millis();
    uint32_t last_motion_millis = now_millis;
    uint32_t wait_retry_audio_millis = 0;
    bool audio_on = _this->config.onoff == AudioConfig::onoffOn ? true : false;
    uint8_t volume = 0;

    // here is a short hysteresis loop for motion since the sensor would reset to 0 after its internal timeout even if the
    // motion continues

    bool motion_hys = false;
    bool last_motion_hys = false;
    bool motion = false;
    bool last_motion = false;
    uint32_t motion_hys_millis = 0;

    while (_this->_is_active)
    {
        now_millis = millis();

        motion = read_motion(_this->config.motion);

        if (last_motion != motion)
        {
            TRACE("motion %d", (int) motion)
            last_motion = motion;
        }
        if (motion == 0)
        {
            if (now_millis < motion_hys_millis || motion_hys_millis + MOTION_HYS_MILLIS < now_millis)
            {
                motion_hys = false;
            }
        }
        else
        {
            motion_hys = true;
            motion_hys_millis = millis();
        }

        if (last_motion_hys != motion_hys)
        {
            TRACE("motion_hys %d", (int) motion_hys)
            last_motion_hys = motion_hys;
        }

        _this->status.motion = motion_hys;

        if (motion_hys == true)
        {
            last_motion_millis = now_millis;
        }

        if (_this->config.onoff == AudioConfig::onoffMotion)
        {
            if (now_millis < last_motion_millis || (now_millis-last_motion_millis)/1000 >= _this->config.delay)
            {
                audio_on = false;
            }    
            else
            {
                audio_on = true;
            }
        }
        else
        {
            audio_on = _this->config.onoff == AudioConfig::onoffOn ? true : false;
        }

        if (_this->_should_reconnect == true)
        {
            TRACE("audio_task: requested reconnect")
            _this->_should_reconnect = false;

            if (_this->engine.isRunning() == true)
            {
                _this->engine.stopSong();
                TRACE("audio_task: stream disconnected (off)")
            }

        }

        uint8_t new_volume = _this->get_volume();

        if (volume != new_volume)
        {
            volume = new_volume;
            _this->engine.setVolume(volume);
            TRACE("audio_task: change volume to %d", (int) volume)
            _this->status.volume = volume;
        }
        
        if (volume == 0)
        {
            audio_on = false;
        }

        _this->status.is_streaming = audio_on;

        if (wait_retry_audio_millis == 0 || wait_retry_audio_millis+AUDIO_RECONNECT_TIMEOUT <= now_millis || now_millis < wait_retry_audio_millis)
        {
            if (audio_on == true)
            {
                if (_this->engine.isRunning() == false)
                {
                    //esp_log_level_set("*", ESP_LOG_ERROR);  
                    String url = _this->choose_url();
                    bool connect_ok = _this->engine.connecttohost(url.c_str());

                    TRACE("audio_task: connection attempt to URL %s, result %d", url.c_str(), (int) connect_ok)

                    if (connect_ok == false)
                    {
                        wait_retry_audio_millis = millis();
                        TRACE("audio_task: will retry connection in %d milliseconds", (int) AUDIO_RECONNECT_TIMEOUT)
                    }
                    else
                    {
                        wait_retry_audio_millis = 0;  
                        TRACE("audio_task: stream connected (on)")
                    }
                }
                else
                {
                    _this->engine.loop();
                    delay(0);
                }
            }
            else
            {
                if (_this->engine.isRunning() == true)
                {
                    _this->engine.stopSong();
                    TRACE("audio_task: stream disconnected (off)")
                }
                delay(100);
            }
        }
        else
        {
            delay(100);
        }    
    }

    if (_this->engine.isRunning() == true)
    {
        _this->engine.stopSong();
    }

    _this->_is_finished = true;
    TRACE("audio_task: finished")
    vTaskDelete(NULL);
}


String AudioHandler::choose_url() 
{
    Lock lock(semaphore);

    int index = 0;

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

    status.url_index = index;
    return config.service.url[index].value;
}


uint8_t AudioHandler::get_volume(bool trace) const
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
            TRACE("get_volume, datetime is valid, _tm.tm_hour %d, hour %d, schedule[hour] %d", (int) _tm.tm_hour, (int) hour, (int)config.sound.schedule[hour])
        }

        switch(config.sound.schedule[hour])
        {
            case AudioConfig::Sound::smOff:
                if (trace)
                {
                    TRACE("get_volume, returning volume 0, smOff")
                }
                return 0;
            case AudioConfig::Sound::smOn:
                if (trace)
                {
                    TRACE("get_volume, returning volume %d, smOn", (int) config.sound.volume)
                }
                return config.sound.volume;
            case AudioConfig::Sound::smOnLowVolume:
                if (trace)
                {
                    TRACE("get_volume, returning volume_low %d, smLowVolume", (int) config.sound.volume_low)
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
            TRACE("get_volume, datetime invalid, _tm.tm_year %d", (int) _tm.tm_year)
        }
    }
    
    if (trace)
    {
        TRACE("get_volume, returning default, not by schedule")
    }
    return config.sound.volume;
}


void AudioHandler::configure_audio()
{
    TRACE("audio_task: configure_audio")
    engine.setConnectionTimeout(AUDIO_CONNECTION_TIMEOUT, AUDIO_CONNECTION_TIMEOUT);
    engine.setPinout(config.i2s.bclk.gpio, config.i2s.lrc.gpio, config.i2s.dout.gpio);
    //engine.forceMono(true);
    configure_sound();
    TRACE("audio_task: configure_audio done")
}


void AudioHandler::configure_sound()
{
    TRACE("audio_task: configure_sound")
    engine.setTone(config.sound.gain_low_pass, config.sound.gain_band_pass, config.sound.gain_high_pass);
    get_volume(true);  // trace selection once
    TRACE("audio_task: configure_sound done")
}


void AudioHandler::configure_hw_motion(const AudioConfig::Motion &motion)
{
    // configure motion
    // ignore debounce because we are using polling with low frequency (to skip adding synchronisation in case of interrupts)

    TRACE("configure motion: gpio=%d, inverted=%d", (int)motion.channel.gpio, (int)motion.channel.inverted)
    gpioHandler.setupChannel(motion.channel.gpio, INPUT_PULLUP, motion.channel.inverted, NULL);
}


bool AudioHandler::read_motion(const AudioConfig::Motion &motion)
{
    // avoid using non-static functions of gpiochannel to skip thread synchronisation

    return GpioChannel::read(motion.channel.gpio, motion.channel.inverted);
}


void start_audio_task(const AudioConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start audio_task while it is running, redirecting to reconfigure")
        reconfigure_audio(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_audio_task()
{
    handler.stop();
}

void reconfigure_audio(const AudioConfig &_config)
{
    handler.reconfigure(_config);
}

AudioStatus get_audio_status()
{
    return handler.get_status();
}

#endif // INCLUDE_AUDIO