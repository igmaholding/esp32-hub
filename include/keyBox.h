#include <ArduinoJson.h>

#define KEYBOX_NUM_CHANNELS 16  // max 32, limited by KeyBoxStatus; if change also update the Actuator.addr


class KeyBoxConfig
{
    public:

        const uint8_t EPROM_VERSION = 2;

        KeyBoxConfig()
        {
        }
        
        KeyBoxConfig & operator = (const KeyBoxConfig & config) 
        {
            buzzer = config.buzzer;
            keypad = config.keypad;
            actuator = config.actuator;

            codes = config.codes;

            return *this;
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const;

        bool operator == (const KeyBoxConfig & config) const
        {
            return buzzer == config.buzzer && keypad == config.keypad && actuator == config.actuator && codes == config.codes;
        }

        String as_string() const
        {
            return String("{buzzer=") + buzzer.as_string() + ", keypad=" + keypad.as_string() + ", actuator=" + actuator.as_string() + 
                   ", codes=" + codes.as_string() + "}";
        }

        // data

        struct Buzzer
        {
            Buzzer()
            {
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return channel.is_valid();
            }

            bool operator == (const Buzzer & buzzer) const
            {
                return channel == buzzer.channel;
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
                    return gpio == channel.gpio && inverted == channel.inverted;
                }

                String as_string() const
                {
                    return String("{gpio=") + String((int)(gpio)) + ", inverted=" + String(inverted ? "true" : "false") + "}";
                }

                gpio_num_t gpio;
                bool inverted;
                
            };
            
            Channel channel;            

        };
        
        Buzzer buzzer;
    
        struct Keypad
        {
            Keypad()
            {
                debounce = 0;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return c[0].is_valid() && c[1].is_valid() && c[2].is_valid() && c[3].is_valid() &&
                       l[0].is_valid() && l[1].is_valid() && l[2].is_valid() && l[3].is_valid();
            }

            bool operator == (const Keypad & keypad) const
            {
                return c[0] == keypad.c[0] && c[1] == keypad.c[1] && c[2] == keypad.c[2] && c[3] == keypad.c[3] && 
                       l[0] == keypad.l[0] && l[1] == keypad.l[1] && l[2] == keypad.l[2] && l[3] == keypad.l[3] &&
                       debounce == keypad.debounce; 
            }

            String as_string() const
            {
                return String("{c=[") + c[0].as_string() + ", " + c[1].as_string() + ", " + c[2].as_string() + ", " + c[3].as_string() + "], " +
                              "l=[" + l[0].as_string() + ", " + l[1].as_string() + ", " + l[2].as_string() + ", " + l[3].as_string() + "]" +
                              ", debounce=" + String(debounce) + "}";
            }

            struct Channel
            {
                Channel()
                {
                    gpio = gpio_num_t(-1);
                    inverted = false;
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
                    return gpio == channel.gpio && inverted == channel.inverted;
                }

                String as_string() const
                {
                    return String("{gpio=") + String((int)(gpio)) + ", inverted=" + String(inverted ? "true" : "false") + "}";
                }

                gpio_num_t gpio;
                bool inverted;
                
            };
            
            Channel c[4];            
            Channel l[4];            
            uint16_t debounce;

        };
        
        Keypad keypad;


        struct Actuator
        {
            static const size_t NUM_CHANNELS = KEYBOX_NUM_CHANNELS;

            Actuator()
            {
            }

            Actuator(const Actuator & actuator)
            {
                *this = actuator;
            }

            Actuator & operator = (const Actuator & actuator) 
            {
                if (this != & actuator)
                {
                    addr[0] = actuator.addr[0];
                    addr[1] = actuator.addr[1];
                    addr[2] = actuator.addr[2];
                    addr[3] = actuator.addr[3];
                    
                    coil = actuator.coil;
                    status = actuator.status;
                }

                return *this;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return addr[0].is_valid() && addr[1].is_valid() && addr[2].is_valid() && addr[3].is_valid() &&
                       coil.is_valid() && status.is_valid();
            }

            bool operator == (const Actuator & actuator) const
            {
                return addr[0] == actuator.addr[0] && addr[1] == actuator.addr[1] && addr[2] == actuator.addr[2] && addr[3] == actuator.addr[3] && 
                       coil == actuator.coil && status == actuator.status; 
            }

            String as_string() const
            {
                return String("{addr=[") + addr[0].as_string() + ", " + addr[1].as_string() + ", " + addr[2].as_string() + ", " + addr[3].as_string() + "], " +
                              "coil=" + coil.as_string() + ", status=" + status.as_string() + "}";
            }

            struct Channel
            {
                Channel()
                {
                    gpio = gpio_num_t(-1);
                    inverted = false;
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
                    return gpio == channel.gpio && inverted == channel.inverted;
                }

                String as_string() const
                {
                    return String("{gpio=") + String((int)(gpio)) + ", inverted=" + String(inverted ? "true" : "false") + "}";
                }

                gpio_num_t gpio;
                bool inverted;
                
            };
            
            Channel addr[4];            
            Channel coil;            
            Channel status;            

        };
        
        Actuator actuator;

        struct Codes
        {
            static const size_t NUM_CHANNELS = KEYBOX_NUM_CHANNELS;

            Codes()
            {
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                for (size_t i=0; i<sizeof(code)/sizeof(code[0]); ++i)
                {
                    if (code[i].is_valid() == false)
                        return false;
                }

                return true;
            }

            bool operator == (const Codes & codes) const
            {
                for (size_t i=0; i<sizeof(code)/sizeof(code[0]); ++i)
                {
                    if (!(code[i] == codes.code[i]))
                        return false;
                }
                return true; 
            }

            String as_string() const
            {
                String r = "{code=[";

                for (size_t i=0; i<sizeof(code)/sizeof(code[0]); ++i)
                {
                    r += String(i) + ":" + code[i].as_string();
                    
                    if (i < sizeof(code)/sizeof(code[0])-1)
                    {
                        r += ", ";
                    }
                }
                r += "]}";
            }

            struct Code
            {
                Code()
                {
                }
            
                void from_json(const JsonVariant & json);

                void to_eprom(std::ostream & os) const;
                bool from_eprom(std::istream & is);

                bool is_valid() const 
                {
                    if (value.isEmpty())
                    {
                        return true;
                    }

                    for (size_t i=0; i<value.length(); ++i)
                    {
                        if (isdigit(value[i]) == false)
                        {
                            return false;
                        } 
                    }
                    return true;
                }

                bool operator == (const Code & code) const
                {
                    return value == code.value;
                }

                String as_string() const
                {
                    return value;
                }

                String value;
                
            };

            Code code[NUM_CHANNELS];
        };

        Codes codes;

};


struct KeyBoxStatus
{
    static const size_t NUM_CHANNELS = KEYBOX_NUM_CHANNELS;

    KeyBoxStatus()
    {
        clear();
    }

    void clear() 
    {
        value = 0;
    }
    
    bool get_status(size_t channel_number) const
    {
        return (value & (1 << channel_number)) ? true : false;
    }

    void set_status(size_t channel_number, bool status) 
    {
        if (status)
        {
            value |= 1 << channel_number;
        }
        else
        {
            value &= ~(1 << channel_number);
        }
    }

    uint32_t value;
};

void start_keybox_task(const KeyBoxConfig &);
void stop_keybox_task();
KeyBoxStatus get_keybox_status();

void reconfigure_keybox(const KeyBoxConfig &);
