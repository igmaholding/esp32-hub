#ifdef INCLUDE_AUDIO

#include <ArduinoJson.h>
#include <buzzer.h>
#include <keypad.h>
#include <keyboxActuator.h>


class AudioConfig
{
    public:

        const uint8_t EPROM_VERSION = 4;

        enum OnOff
        {
            onoffOff = 0,
            onoffOn  = 1,
            onoffMotion = 2
        };

        AudioConfig()
        {
            onoff = onoffMotion;
            delay = 300; // seconds
        }
        
        AudioConfig & operator = (const AudioConfig & config) 
        {
            onoff = config.onoff;

            motion = config.motion;
            delay = config.delay;

            i2s = config.i2s;

            service = config.service;
            sound = config.sound;

            return *this;
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const;

        bool operator == (const AudioConfig & config) const
        {
            return onoff == config.onoff && motion == config.motion && delay == config.delay && i2s == config.i2s &&
                   service == config.service && sound == config.sound;
        }

        String as_string() const
        {
            return String("{onoff=") + onoff_2_str(onoff) + ", motion=" + motion.as_string() + ", delay=" + String((int) delay) + 
                          ", i2s=" + i2s.as_string() + ", service=" + service.as_string() + ", sound=" + sound.as_string() + "}";
        }

        const char * onoff_2_str(OnOff value) const
        {
            switch (value)
            {
                case onoffOff: return "off";
                case onoffOn: return "off";
                case onoffMotion: return "motion";
                default: return "<unknown>";
            }
        }

        struct Motion
        {
            Motion()
            {
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return channel.is_valid();
            }

            bool operator == (const Motion & motion) const
            {
                return channel == motion.channel;
            }

            String as_string() const
            {
                return String("{channel=") + channel.as_string() + "}";
            }

            struct Channel
            {
                Channel()
                {
                    gpio = gpio_num_t(-1);
                    inverted = false;
                    debounce = 0;
                }
            
                void from_json(const JsonVariant & json);

                void to_eprom(std::ostream & os) const;
                bool from_eprom(std::istream & is);

                bool is_valid() const 
                {
                    return gpio != gpio_num_t(-1);
                }

                bool operator == (const Channel & channel) const
                {
                    return gpio == channel.gpio && inverted == channel.inverted && debounce == channel.debounce;
                }

                String as_string() const
                {
                    return String("{gpio=") + String((int)(gpio)) + ", inverted=" + String(inverted ? "true" : "false") + ", debounce=" + String(debounce) + "}";
                }

                gpio_num_t gpio;
                bool inverted;
                uint16_t debounce;
                
            };
            
            Channel channel;            

        };
        
        struct I2s
        {
            I2s()
            {
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return dout.is_valid() && bclk.is_valid() && lrc.is_valid();
            }

            bool operator == (const I2s & i2s) const
            {
                return dout == i2s.dout && bclk == i2s.bclk && lrc == i2s.lrc;
            }

            String as_string() const
            {
                return String("{dout=") + dout.as_string() + ", bclk=" + bclk.as_string() + ", lrc=" + lrc.as_string() + "}";
            }

            struct Channel
            {
                Channel()
                {
                    gpio = gpio_num_t(-1);
                }
            
                void from_json(const JsonVariant & json);

                void to_eprom(std::ostream & os) const;
                bool from_eprom(std::istream & is);

                bool is_valid() const 
                {
                    return gpio != gpio_num_t(-1);
                }

                bool operator == (const Channel & channel) const
                {
                    return gpio == channel.gpio;
                }

                String as_string() const
                {
                    return String("{gpio=") + String((int)(gpio))+ "}";
                }

                gpio_num_t gpio;
                
            };
            
            Channel dout;            
            Channel bclk;            
            Channel lrc;            

        };

        struct Service
        {
            static const size_t NUM_URLS = 5;

            enum UrlSelect
            {
                usPriority = -1,
                usRandom   = -2
            };

            Service()
            {
                url_select = usPriority;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
                {
                    if (url[i].is_valid() == false)
                        return false;
                }

                return true;
            }

            bool operator == (const Service & service) const
            {
                for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
                {
                    if (!(url[i] == service.url[i]))
                        return false;
                }
                return url_select == service.url_select; 
            }

            String as_string() const
            {
                String r = "{url=[";

                for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
                {
                    r += String(i) + ":" + url[i].as_string();
                    
                    if (i < sizeof(url)/sizeof(url[0])-1)
                    {
                        r += ", ";
                    }
                }
                r = r + "], url_select=" + url_select_2_str(url_select) + "}";
                return r;
            }

            String url_select_2_str(int8_t value) const
            {
                if (value >= 0)
                {
                    return String((int) value);
                }
                else
                {
                    switch ((UrlSelect)value)
                    {
                        case usPriority: return "priority";
                        case usRandom: return "random";
                        default: return "<unknown>";
                    }
                }
            }

            struct Url
            {
                Url()
                {
                }
            
                void clear()
                {
                    value.clear();
                }

                void from_json(const JsonVariant & json);

                void to_eprom(std::ostream & os) const;
                bool from_eprom(std::istream & is);

                bool is_valid() const 
                {
                    return true;
                }

                bool operator == (const Url & url) const
                {
                    return value == url.value;
                }

                String as_string() const
                {
                    return value;
                }

                String value;
                
            };

            Url url[NUM_URLS];
            int8_t url_select;
        };

        struct Sound
        {
            enum ScheduleMask
            {
                smOff = 0,
                smOn = 1,
                smOnLowVolume = 2
            };

            Sound()
            {
                volume = 0;
                volume_low = 0;
                gain_low_pass = 0;
                gain_band_pass = 0;
                gain_high_pass = 0;
                
                for (size_t i=0; i<sizeof(schedule)/sizeof(schedule[0]); ++i)
                {
                    schedule[i] = smOn;
                }
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return true;
            }

            bool operator == (const Sound & sound) const
            {
                for (size_t i=0; i<sizeof(schedule)/sizeof(schedule[0]); ++i)
                {
                    if (schedule[i] != sound.schedule[i])
                    {
                        return false;
                    }
                }

                return volume ==  sound.volume && volume_low == sound.volume_low && 
                       gain_low_pass == sound.gain_low_pass &&
                       gain_band_pass == sound.gain_band_pass && 
                       gain_high_pass == sound.gain_high_pass; 
            }

            String as_string() const
            {
                char buf[sizeof(schedule)/sizeof(schedule[0])*2 + 1];

                for (size_t i=0; i<sizeof(schedule)/sizeof(schedule[0]); ++i)
                {
                    buf[i*2] = (char)('0' + (int) schedule[i]);

                    if (i == sizeof(schedule)/sizeof(schedule[0]) - 1)
                    {
                        buf[i*2+1] = 0;
                    }
                    else
                    {
                        buf[i*2+1] = ',';

                    }                    
                }

                return String("{volume=") + String((int) volume) + ", volume_low=" + String((int) volume_low) + 
                       ", gain_low_pass=" + String((int) gain_low_pass) +
                       ", gain_band_pass=" + String((int) gain_band_pass) +
                       ", gain_high_pass=" + String((int) gain_high_pass) + 
                       ", schedule=" + buf + "}";
            }
            
            uint8_t volume;        // 0..21
            uint8_t volume_low;    // 0..21
            int8_t gain_low_pass;  // -40..3 
            int8_t gain_band_pass; // -40..3 
            int8_t gain_high_pass; // -40..3 
            uint8_t schedule[24];  // per hour: a value of ScheduleMask; 0 - disabled, 1 - enabled normal volume, 2 - enabled low volume
        };

        OnOff onoff;

        Motion motion;
        uint32_t delay;
        
        I2s i2s;
            
        Service service;
        
        Sound sound;
};

struct AudioStatus
{
    AudioStatus()
    {
        motion = false;
        is_streaming = false;
        volume = 0;
        url_index = -1;
        bitrate = 0;
    }

    AudioStatus(const AudioStatus & other)
    {
        if (this == & other)
        {
            return;
        }        

        motion = other.motion;
        is_streaming = other.is_streaming;
        volume = other.volume;
        url_index = other.url_index;
        bitrate = other.bitrate;
        title = other.title;
    }

    AudioStatus & operator = (const AudioStatus & other)
    {
        if (this == & other)
        {
            return *this;
        }        

        motion = other.motion;
        is_streaming = other.is_streaming;
        volume = other.volume;
        url_index = other.url_index;
        bitrate = other.bitrate;
        title = other.title;

        return *this;
    }

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("audio");
        JsonVariant jsonVariant = json["audio"];

        jsonVariant["motion"] = motion ? "true" : "false";
        jsonVariant["is_streaming"] = is_streaming ? "true" : "false";
        jsonVariant["volume"] = volume;
        jsonVariant["url_index"] = url_index;

        jsonVariant["bitrate"] = bitrate;
        jsonVariant["title"] = title;
    }

    bool motion;
    bool is_streaming;
    uint8_t volume;
    int url_index;
    uint32_t bitrate;
    String title;
};


void start_audio_task(const AudioConfig &);
void stop_audio_task();

void reconfigure_audio(const AudioConfig &);

AudioStatus get_audio_status();


#endif // INCLUDE_AUDIO