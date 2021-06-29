#include <ArduinoJson.h>

class ShowerGuardConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;

        ShowerGuardConfig()
        {
        }
        
        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const
        {
            return motion.is_valid() && rh.is_valid() && temp.is_valid();
        }

        bool operator == (const ShowerGuardConfig & config) const
        {
            return motion == config.motion && rh == config.rh && temp == config.temp;
        }

        String as_string() const
        {
            return String("{motion=") + motion.as_string() + ", rh=" + rh.as_string() + ", temp=" + temp.as_string() + "}";
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
                return vad == rh.vad && vdd == rh.vdd;
            }

            String as_string() const
            {
                return String("{vad=") + vad.as_string() + ", vdd=" + vdd.as_string() + "}";
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
            
        };
        
        Rh rh;

        struct Temp
        {
            Temp()
            {
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
                return channel == temp.channel && addr == temp.addr;
            }

            String as_string() const
            {
                return String("{channel=") + channel.as_string() + ", addr=\"" + addr + "\"}";
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

        };
        
        Temp temp;


};