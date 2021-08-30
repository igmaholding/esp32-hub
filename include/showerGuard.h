#include <ArduinoJson.h>

class ShowerGuardConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;

        ShowerGuardConfig()
        {
        }
        
        ShowerGuardConfig & operator = (const ShowerGuardConfig & config) 
        {
            motion = config.motion;
            rh = config.rh;
            temp = config.temp;
            light = config.light;
            fan = config.fan;

            return *this;
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const;

        bool operator == (const ShowerGuardConfig & config) const
        {
            return motion == config.motion && rh == config.rh && temp == config.temp && light == config.light && fan == config.fan;
        }

        String as_string() const
        {
            return String("{motion=") + motion.as_string() + ", rh=" + rh.as_string() + ", temp=" + temp.as_string() + 
                   ", light=" + light.as_string() + ", fan=" + fan.as_string() + "}";
        }

        // data

        struct Motion
        {
            Motion()
            {
                linger = 60; // seconds
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
                return channel == motion.channel && linger == motion.linger;
            }

            String as_string() const
            {
                return String("{channel=") + channel.as_string() + ", linger=" + String(linger) + "}";
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
            uint16_t linger;

        };
        
        Motion motion;
    
        struct Rh
        {
            Rh()
            {
                corr = 0;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return vad.is_valid() && vdd.is_valid();
            }

            bool operator == (const Rh & rh) const
            {
                return vad == rh.vad && vdd == rh.vdd && corr == rh.corr;
            }

            String as_string() const
            {
                return String("{vad=") + vad.as_string() + ", vdd=" + vdd.as_string() + ", corr=" + corr + "}";
            }

            struct Channel
            {
                Channel()
                {
                    gpio = gpio_num_t(-1);
                    atten = 0;
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
                    return gpio == channel.gpio && atten == channel.atten;
                }

                String as_string() const
                {
                    return String("{gpio=") + String((int)(gpio)) + ", atten=" + String(atten) + "}";
                }

                gpio_num_t gpio;
                uint8_t atten;
                
            };
            
            Channel vad;
            Channel vdd;
            float corr;
            
        };
        
        Rh rh;

        struct Temp
        {
            Temp()
            {
                corr = 0;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return channel.is_valid() && !addr.isEmpty();
                // TODO: check addr OW format
            }

            bool operator == (const Temp & temp) const
            {
                return channel == temp.channel && addr == temp.addr && corr == temp.corr;
            }

            String as_string() const
            {
                return String("{channel=") + channel.as_string() + ", addr=\"" + addr + "\", corr=" + corr + "}";
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
            
            Channel channel;            
            String addr;
            float corr;

        };
        
        Temp temp;

        struct Light
        {
            enum Mode
            {
                mOff  = 0,
                mOn   = 1,
                mAuto = 2,
            };

            static const char * mode_2_str(Mode mode) 
            {
                switch(mode)
                {
                    case mOff:  return "off";
                    case mOn:   return "on";
                    case mAuto: return "auto";
                }

                return "<unknown>";
            }

            static Mode str_2_mode(const char * str) 
            {
                if (!strcmp(str, mode_2_str(mOff)))
                {
                    return mOff;
                }
                else if (!strcmp(str, mode_2_str(mOn)))
                {
                    return mOn;
                }
                else if (!strcmp(str, mode_2_str(mAuto)))
                {
                    return mAuto;
                }

                return Mode(-1);
            }

            Light()
            {
                mode = mAuto; 
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return channel.is_valid() && mode != Mode(-1);
            }

            bool operator == (const Light & light) const
            {
                return channel == light.channel && mode == light.mode;
            }

            String as_string() const
            {
                return String("{channel=") + channel.as_string() + ", mode=" + mode_2_str(mode) + "}";
            }

            struct Channel
            {
                Channel()
                {
                    gpio = gpio_num_t(-1);
                    inverted = false;
                    coilon_active = true;
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
                    return gpio == channel.gpio && inverted == channel.inverted && coilon_active == channel.coilon_active;
                }

                String as_string() const
                {
                    return String("{gpio=") + String((int)(gpio)) + ", inverted=" + String(inverted ? "true" : "false") + 
                           ",coilon_active=" + String(coilon_active ? "true" : "false") + "}";
                }

                gpio_num_t gpio;
                bool inverted;
                bool coilon_active;
                
            };
            
            Channel channel;            
            Mode mode;

        };
        
        Light light;

        struct Fan : public Light
        {
            const uint8_t RH_ON = 55;
            const uint8_t RH_OFF = 50;

            Fan & operator = (const Fan & fan) 
            {
                Light::operator = (fan);
                
                rh_on = fan.rh_on;
                rh_off = fan.rh_off;
                
                return *this;
            }

            Fan()
            {
                rh_on = RH_ON;
                rh_off = RH_OFF;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return Light::is_valid() && rh_on >= 0 && rh_on <= 100 && rh_off >= 0 && rh_off <= 100 && rh_off < rh_on;
            }

            bool operator == (const Fan & fan) const
            {
                return Light::operator == (fan) && rh_on == fan.rh_on && rh_off == fan.rh_off;
            }

            String as_string() const
            {
                String light_str = Light::as_string();

                return light_str.substring(0, light_str.length()-1) + ", rh_on=" + String(rh_on) + ", rh_off=" + String(rh_off) + "}";
            }

            uint8_t rh_on;
            uint8_t rh_off;

        };
        
        Fan fan;
};


struct ShowerGuardStatus
{
    ShowerGuardStatus()
    {
        temp = 0;
        rh = 0;
        motion = false;
        light = false;
        fan = false;
    }

    float temp;
    float rh;
    bool motion;
    bool light;
    bool fan;
};

void start_shower_guard_task(const ShowerGuardConfig &);
void stop_shower_guard_task();
ShowerGuardStatus get_shower_guard_status();

void reconfigure_shower_guard(const ShowerGuardConfig &);
