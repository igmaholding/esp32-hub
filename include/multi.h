#ifdef INCLUDE_MULTI

#include <ArduinoJson.h>
#include <map>
#include <buzzer.h>
#include <keypad.h>
#include <keyboxActuator.h>
#include <digitalInputChannelConfig.h>
#include <genericChannelConfig.h>
#include <bt.h>
#include <trace.h>

class MultiConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;

        MultiConfig()
        {
        }
        
        MultiConfig & operator = (const MultiConfig & config) 
        {
            uart = config.uart;
            
            bt = config.bt;

            fm = config.fm;

            i2s = config.i2s;

            i2c = config.i2c;

            service = config.service;
            sound = config.sound;

            return *this;
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const;

        bool operator == (const MultiConfig & config) const
        {
            return i2s == config.i2s && i2c == config.i2c && service == config.service && sound == config.sound;
        }

        String as_string() const
        {
            return String("{uart=") + uart.as_string() + ", bt=" + bt.as_string() + ", fm=" + fm.as_string() + ", i2s=" + i2s.as_string() + 
                    ", i2c=" + i2c.as_string() + ", service=" + service.as_string() + ", sound=" + sound.as_string() + "}";
        }
        
        struct I2s
        {
            I2s()
            {
            }

            void clear()
            {
                dout.clear();
                bclk.clear();
                lrc.clear();
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
            
            GenericChannelConfig dout;            
            GenericChannelConfig bclk;            
            GenericChannelConfig lrc;            

        };

        struct I2c
        {
            I2c()
            {
            }

            void clear()
            {
                scl.clear();
                sda.clear();
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return scl.is_valid() && sda.is_valid();
            }

            bool operator == (const I2c & i2c) const
            {
                return scl == i2c.scl && sda == i2c.sda;
            }

            String as_string() const
            {
                return String("{scl=") + scl.as_string() + ", sda=" + sda.as_string() + "}";
            }
            
            GenericChannelConfig scl;            
            GenericChannelConfig sda;            

        };

        struct UART
        {
            UART()
            {
                clear();
            }

            void clear()
            {
                uart_num = -1;
                tx.clear();                
                rx.clear();                
            } 

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return uart_num >= 0 && uart_num <= 2 && tx.is_valid() && rx.is_valid();
            }

            bool operator == (const UART & uart) const
            {
                return uart_num == uart.uart_num && tx == uart.tx && rx == uart.rx;
            }

            String as_string() const
            {
                return String("{uart_num=") + String(uart_num) + ", tx=" + tx.as_string() + ", rx=" + rx.as_string() + "}";
            }
            
            uint8_t uart_num;
            GenericChannelConfig tx;            
            GenericChannelConfig rx;            

        };

        struct Bt
        {
            typedef FscBt::Hw Hw;
            
            Bt()
            {
                clear();
            }

            void clear()
            {
                hw = (Hw) -1;
                reset.clear();
            } 

            static String hw_2_str(Hw hw)
            {
                return FscBt::hw_2_str(hw);
            }
        
            static Hw str_2_hw(const String & string)
            {
                return FscBt::str_2_hw(string);
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return hw != (Hw) -1;
            }

            bool operator == (const Bt & bt) const
            {
                return hw == bt.hw && reset == bt.reset;
            }

            String as_string() const
            {
                return String("{hw=") + hw_2_str(hw) + ", reset=" + reset.as_string() + "}";
            }
            
            Hw hw;
            DigitalOutputChannelConfig reset;
        };

        struct Fm
        {
            Fm()
            {
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return true;
            }

            bool operator == (const Fm & fm) const
            {
                return hw == fm.hw && addr && fm.addr;
            }

            String as_string() const
            {
                return String("{hw=") + hw + ", addr=" + addr +  "}";
            }
            
            String hw;
            String addr;
        };

        struct Service
        {
            static const size_t NUM_URLS = 5;
            static const size_t NUM_FM_FREQS = 5;

            enum UrlSelect
            {
                usPriority = -1,
                usRandom   = -2
            };

            enum FmFreqSelect
            {
                ffsPriority = -1,
                ffsRandom   = -2
            };

            Service()
            {
                url_select = usPriority;
                fm_freq_select = ffsPriority;
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

                for (size_t i=0; i<sizeof(fm_freq)/sizeof(fm_freq[0]); ++i)
                {
                    if (fm_freq[i].is_valid() == false)
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

                if (url_select != service.url_select)
                {
                    return false;
                }

                for (size_t i=0; i<sizeof(fm_freq)/sizeof(fm_freq[0]); ++i)
                {
                    if (!(fm_freq[i] == service.fm_freq[i]))
                        return false;
                }

                if (fm_freq_select != service.fm_freq_select)
                {
                    return false;
                }

                return true;
            }

            String as_string() const
            {
                String r = "{url=[";

                /*for (size_t i=0; i<sizeof(url)/sizeof(url[0]); ++i)
                {
                    r += String(i) + ":" + url[i].as_string();
                    
                    if (i < sizeof(url)/sizeof(url[0])-1)
                    {
                        r += ", ";
                    }
                }*/

                r = r + "...], fm_freq=[...";

                r = r + "], url_select=" + url_select_2_str(url_select) + ", fm_freq_select=" + fm_freq_select_2_str(fm_freq_select) + "}";
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

            String fm_freq_select_2_str(int8_t value) const
            {
                if (value >= 0)
                {
                    return String((int) value);
                }
                else
                {
                    switch ((FmFreqSelect)value)
                    {
                        case ffsPriority: return "priority";
                        case ffsRandom: return "random";
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
                    name.clear();
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
                    return name == url.name && value == url.value;
                }

                String as_string() const
                {
                    return name + ":" + value;
                }

                String name;
                String value;
                
            };

            struct FmFreq
            {
                FmFreq()
                {
                    clear();
                }
            
                void clear()
                {
                    name.clear();
                    value = 0;
                }

                void from_json(const JsonVariant & json);

                void to_eprom(std::ostream & os) const;
                bool from_eprom(std::istream & is);

                bool is_valid() const 
                {
                    return true;
                }

                bool operator == (const FmFreq & fm_freq) const
                {
                    return name == fm_freq.name && value == fm_freq.value;
                }

                String as_string() const
                {
                    return name + ":" + String(value);
                }

                String name;
                float value;
                
            };

            Url url[NUM_URLS];
            int8_t url_select;

            FmFreq fm_freq[NUM_FM_FREQS];
            int8_t fm_freq_select;
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

                return hw == sound.hw && addr && sound.addr && 
                       mute == sound.mute &&
                       volume == sound.volume && volume_low == sound.volume_low && 
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

                return String("{hw=") + hw + ", addr=" + addr + ", mute=" + mute.as_string() + 
                       ", volume=" + String((int) volume) + ", volume_low=" + String((int) volume_low) + 
                       ", gain_low_pass=" + String((int) gain_low_pass) +
                       ", gain_band_pass=" + String((int) gain_band_pass) +
                       ", gain_high_pass=" + String((int) gain_high_pass) + 
                       ", schedule=" + buf + "}";
            }
            
            String hw;
            String addr;
            DigitalOutputChannelConfig mute;
            uint8_t volume;        // 0..100%
            uint8_t volume_low;    // 0..100%
            int8_t gain_low_pass;  // -40..3 
            int8_t gain_band_pass; // -40..3 
            int8_t gain_high_pass; // -40..3 
            uint8_t schedule[24];  // per hour: a value of ScheduleMask; 0 - disabled, 1 - enabled normal volume, 2 - enabled low volume
        };
        
        UART uart;
        
        Bt bt;

        Fm fm;

        I2s i2s;
            
        I2c i2c;

        Service service;
        
        Sound sound;
};

struct MultiStatus
{
    struct Bt
    {
        Bt()
        {
        }

        Bt(const Bt & other)
        {
            if (this == & other)
            {
                return;
            }        

            latest_indications = other.latest_indications;
        }

        Bt & operator = (const Bt & other)
        {
            if (this == & other)
            {
                return *this;
            }        

            latest_indications = other.latest_indications;

            return *this;
        }

        void to_json(JsonVariant & json)
        {
            //DEBUG("MultiStatus::Bt::to_json")
            json.createNestedObject("bt");
            JsonVariant jsonVariant = json["bt"];

            jsonVariant.createNestedObject("latest_indications");
            JsonVariant liJson = jsonVariant["latest_indications"];

            for (auto it = latest_indications.begin(); it != latest_indications.end(); ++it)
            {
                liJson.createNestedObject(it->first);
                liJson[it->first] = it->second;
                //DEBUG("added latest indication for %s:%s", it->first.c_str(), it->second.c_str())
            }
        }

        std::map<String, String> latest_indications;        
    };

    struct Www
    {
        Www()
        {
            is_streaming = false;
            url_index = -1;
            bitrate = 0;
        }

        Www(const Www & other)
        {
            if (this == & other)
            {
                return;
            }        

            is_streaming = other.is_streaming;
            url_index = other.url_index;
            url_name = other.url_name;
            bitrate = other.bitrate;
        }

        Www & operator = (const Www & other)
        {
            if (this == & other)
            {
                return *this;
            }        

            is_streaming = other.is_streaming;
            url_index = other.url_index;
            url_name = other.url_name;
            bitrate = other.bitrate;

            return *this;
        }

        void to_json(JsonVariant & json)
        {
            json.createNestedObject("www");
            JsonVariant jsonVariant = json["www"];

            jsonVariant["is_streaming"] = is_streaming;
            jsonVariant["url_index"] = url_index;
            jsonVariant["url_name"] = url_name;

            jsonVariant["bitrate"] = bitrate;
        }

        bool is_streaming;
        int url_index;
        String url_name;
        uint32_t bitrate;        
    };

    struct Fm
    {
        Fm()
        {
            is_streaming = false;
            index = -1;
            freq = 0;
        }

        Fm(const Fm & other)
        {
            if (this == & other)
            {
                return;
            }        

            is_streaming = other.is_streaming;
            index = other.index;
            name = other.name;
            freq = other.freq;
        }

        Fm & operator = (const Fm & other)
        {
            if (this == & other)
            {
                return *this;
            }        

            is_streaming = other.is_streaming;
            index = other.index;
            name = other.name;
            freq = other.freq;

            return *this;
        }

        void to_json(JsonVariant & json)
        {
            json.createNestedObject("fm");
            JsonVariant jsonVariant = json["fm"];

            jsonVariant["is_streaming"] = is_streaming;
            jsonVariant["index"] = index;
            jsonVariant["name"] = name;
            jsonVariant["freq"] = freq;
        }

        bool is_streaming;
        int index;
        String name;
        float freq;
    };

    MultiStatus()
    {
        volume = 0;
    }

    MultiStatus(const MultiStatus & other)
    {
        if (this == & other)
        {
            return;
        }        

        bt = other.bt;
        www = other.www;
        fm = other.fm;
        volume = other.volume;
        title = other.title;
        status = other.status;
    }

    MultiStatus & operator = (const MultiStatus & other)
    {
        if (this == & other)
        {
            return *this;
        }        

        bt = other.bt;
        www = other.www;
        fm = other.fm;
        volume = other.volume;
        title = other.title;
        status = other.status;

        return *this;
    }

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("multi");
        JsonVariant jsonVariant = json["multi"];

        bt.to_json(jsonVariant);
        www.to_json(jsonVariant);
        fm.to_json(jsonVariant);

        jsonVariant["volume"] = volume;
        jsonVariant["title"] = title;
        jsonVariant["status"] = status;
    }

    Www www;
    Fm fm;
    Bt bt;
    uint8_t volume;
    String title;
    String status;
};


void start_multi_task(const MultiConfig &);
void stop_multi_task();

void reconfigure_multi(const MultiConfig &);

String multi_uart_command(const String & command, String & response);
String multi_audio_control(const String & source, const String & channel, const String & volume, String & response);


MultiStatus get_multi_status();


#endif // INCLUDE_MULTI