#ifdef INCLUDE_KEYBOX

#include <ArduinoJson.h>
#include <buzzer.h>
#include <keypad.h>
#include <keyboxActuator.h>


class KeyboxConfig
{
    public:

        const uint8_t EPROM_VERSION = 3;

        KeyboxConfig()
        {
        }
        
        KeyboxConfig & operator = (const KeyboxConfig & config) 
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

        bool operator == (const KeyboxConfig & config) const
        {
            return buzzer == config.buzzer && keypad == config.keypad && actuator == config.actuator && codes == config.codes;
        }

        String as_string() const
        {
            return String("{buzzer=") + buzzer.as_string() + ", keypad=" + keypad.as_string() + ", actuator=" + actuator.as_string() + 
                   ", codes=" + codes.as_string() + "}";
        }

        
        BuzzerConfig buzzer;
            
        KeypadConfig keypad;
        
        KeyboxActuatorConfig actuator;

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
                return r;
            }

            struct Code
            {
                Code()
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



void start_keybox_task(const KeyboxConfig &);
void stop_keybox_task();

void reconfigure_keybox(const KeyboxConfig &);

String keybox_actuate(const String & channel_str);

#endif // INCLUDE_KEYBOX