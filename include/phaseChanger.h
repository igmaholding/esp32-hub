#ifdef INCLUDE_ZERO2TEN

#include <ArduinoJson.h>
#include <digitalInputChannelConfig.h>
#include <digitalOutputChannelConfig.h>
#include <analogInputChannelConfig.h>
#include <genericChannelConfig.h>
#include <mapTable.h>
#include <trace.h>

#include <map>
#include <vector>
#include <algorithm>


class Zero2tenConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;

        Zero2tenConfig()
        {
        }
        
        Zero2tenConfig & operator = (const Zero2tenConfig & config) 
        {
            input_channels = config.input_channels;
            output_channels = config.output_channels;
            applets = config.applets;

            return *this;
        }

        void clear()
        {
            input_channels.clear();
            output_channels.clear();
            applets.clear();
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);
        
        bool is_valid() const;
        
        bool operator == (const Zero2tenConfig & config) const
        {
            if (input_channels.size() != config.input_channels.size())
            {
                return false;
            }

            if (output_channels.size() != config.output_channels.size())
            {
                return false;
            }

            if (applets.size() != config.applets.size())
            {
                return false;
            }

            // the order of channels is also important at comparison since channels are addressed 
            // by their index

            {auto dit = config.input_channels.begin();

            for (auto it = input_channels.begin(); it != input_channels.end(); ++it, ++dit)
            {
                if (!(*it == *dit))
                {
                    return false;
                }
            }}

            {auto dit = config.output_channels.begin();

            for (auto it = output_channels.begin(); it != output_channels.end(); ++it, ++dit)
            {
                if (!(*it == *dit))
                {
                    return false;
                }
            }}

            for (auto it = applets.begin(); it != applets.end(); ++it)
            {
                auto dit = config.applets.find(it->first);

                if (dit == config.applets.end())
                {
                    return false;
                }

                if (!(*it == *dit))
                {
                    return false;
                }
            }

            return true; 
        }

        String as_string() const
        {
            String r = "{input_channels=[";
            
            size_t i = 0;

            for (auto it = input_channels.begin(); it != input_channels.end(); ++it, ++i)
            {
                r += String(i) + ":" + input_channels[i].as_string();
                
                if (it+1 != input_channels.end())
                {
                    r += ", ";
                }
            }
            r += "], output_channels=[";

            i = 0;

            for (auto it = output_channels.begin(); it != output_channels.end(); ++it, ++i)
            {
                r += String(i) + ":" + output_channels[i].as_string();
                
                if (it+1 != output_channels.end())
                {
                    r += ", ";
                }
            }

            r += "], applets=[";

            for (auto it = applets.begin(); it != applets.end(); ++it)
            {
                r += it->first + ":" + it->second.as_string();
                
                auto test_it = it;
                test_it++;

                if (test_it != applets.end())
                {
                    r += ", ";
                }
            }
            
            r += "]}";
            return r;
        }

        // data

        struct InputChannel : public AnalogInputChannelConfig
        {
            InputChannel()
            {
                clear();
            }

            const InputChannel & operator = (const InputChannel & channel)
            {
                if (this != & channel)
                {
                    AnalogInputChannelConfig::operator =(channel);
                    ratio = channel.ratio;
                }

                return *this;
            }

            void clear()
            {
                AnalogInputChannelConfig::clear();
                ratio = 1;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return AnalogInputChannelConfig::is_valid();
            }

            bool operator == (const InputChannel & channel) const
            {
                return AnalogInputChannelConfig::operator == (channel) && ratio == channel.ratio;
            }

            String as_string() const
            {
                return String("{channel=") + AnalogInputChannelConfig::as_string() + 
                              ",ratio=" + String(ratio) + 
                              "}";
            }
            
            // external voltage to read voltage ratio, consider the atten
            float ratio;  
        };
        
        struct OutputChannel
        {            
            OutputChannel()
            {
                clear();
            }

            const OutputChannel & operator = (const OutputChannel & channel)
            {
                if (this != & channel)
                {
                    output = channel.output;
                    max_voltage = channel.max_voltage;
                    loopback = channel.loopback;
                }

                return *this;
            }

            void clear()
            {
                output.clear();
                max_voltage = 10;
                loopback.clear();
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                if (output.is_valid() == false)
                {
                    ERROR("output.is_valid() == false")
                    return false;
                }

                if (max_voltage < 0)
                {
                    ERROR("max_voltage is invalid")
                    return false;
                }

                if (loopback.is_valid() == false)
                {
                    ERROR("loopback.is_valid() == false")
                    return false;
                }
                return true;
            }

            bool operator == (const OutputChannel & channel) const
            {
                return output == channel.output && loopback == channel.loopback;
            }

            String as_string() const
            {
                return String("{output=") + output.as_string() + 
                              ",max_voltage=" + String(max_voltage) + 
                              ",loopback=" + loopback.as_string() + 
                              "}";
            }
            

            DigitalOutputChannelConfig output;
            float max_voltage;  // the voltage that corresponds to 100% pwm duty (always on)            
            InputChannel loopback;            
        };

        std::vector<InputChannel> input_channels;
        std::vector<OutputChannel> output_channels;
    
        struct Applet
        {
            // the temperature config has DS18B20 device address and it is optional; if there is only
            // one device present and no device address is configured - then the one found is used
            //
            // moreover, the channel also can be empty in which case temperature is not requested this
            // way (e.g. in case of AHT10 himudity sensor which has its own temperature reading)

            struct Temp
            {
                Temp()
                {
                    corr = 0;
                }

                void clear()
                {
                    corr = 0;
                    addr.clear();
                    channel.clear();
                }

                void from_json(const JsonVariant & json);

                void to_eprom(std::ostream & os) const;
                bool from_eprom(std::istream & is);

                bool is_valid() const 
                {
                    return addr.isEmpty() ? true : channel.is_valid();
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
                
                GenericChannelConfig channel;            
                String addr;
                float corr;
            };
            
            enum Function
            {
                fTemp2out = 0
            };

            static const char * function_2_str(Function function) 
            {
                switch(function)
                {
                    case fTemp2out:  return "temp2out";
                }

                return "<unknown>";
            }

            static Function str_2_function(const char * str) 
            {
                if (!strcmp(str, function_2_str(fTemp2out)))
                {
                    return fTemp2out;
                }

                return Function(-1);
            }

            Applet()
            {
                clear();
            }

            const Applet & operator = (const Applet & applet)
            {
                if (this != & applet)
                {
                    function = applet.function;
                    input_channel = applet.input_channel;
                    output_channel = applet.output_channel;
                    temp = applet.temp;
                    map_table = applet.map_table;
                }

                return *this;
            }

            void clear()
            {
                function = Function(-1);
                input_channel = uint8_t(-1);
                output_channel = uint8_t(-1);
                temp.clear();
                map_table.clear();
            }
            
            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                if (function == fTemp2out)
                {
                    if (output_channel == uint8_t(-1))
                    {
                        ERROR("applet.output_channel is not valid")
                        return false;
                    }

                    if (temp.is_valid() == false)
                    {
                        ERROR("applet.temp.is_valid()==false")
                        return false;
                    }
   
                    if (map_table.is_empty() || !map_table.can_x_to_y())
                    {
                        ERROR("applet.map_table.is_valid()==false")
                        return false;
                    }

                    return true;
                }
            
                return false;
            }

            bool operator == (const Applet & applet) const
            {
                if (function != applet.function)
                {
                    return false;
                }

                if (input_channel != applet.input_channel)
                {
                    return false;
                }

                if (output_channel != applet.output_channel)
                {
                    return false;
                }

                if (!(temp == applet.temp))
                {
                    return false;
                }

                if (!(map_table == applet.map_table))
                {
                    return false;
                }

                return true;
            }

            String as_string() const
            {
                String r = String("{") + "function=" + function_2_str(function) + 
                              ", input_channel=" + String((int) input_channel) +
                              ", output_channel=" + String((int) output_channel) +
                              ", temp=" + temp.as_string() +
                              ", map_table=...";

                r += "}";

                return r;
            }
            
            Function function;
            uint8_t input_channel;  
            uint8_t output_channel; 
            Temp temp;
            MapTable map_table;
         };

        std::map<String, Applet> applets;
};


inline void _set_not_calibrated(float & value)
{
    value = -1;
}

inline bool _is_calibrated(float value)
{
    return value > 0;
}


struct Zero2tenStatus
{
    Zero2tenStatus()
    {
    }

    String as_string() const
    {
        String r = "{status=\"" + status + "\"" 
                   ", input_channels=[";

        for (auto it=input_channels.begin(); it!= input_channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != input_channels.end())
            {
                r += ",";
            }
        }

        r += "], output_channels=[";

        for (auto it=output_channels.begin(); it!= output_channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != output_channels.end())
            {
                r += ",";
            }
        }

        r += "], applets=[";

        for (auto it=applets.begin(); it!= applets.end(); ++it)
        {
            r += "\"" + it->first + "\":" + it->second.as_string();

            auto it_tmp = it;
            ++it_tmp;

            if (it_tmp != applets.end())
            {
                r += ",";
            }
        }

        r += "]}";


        return r;
    }

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("zero2ten");
        JsonVariant jsonVariant = json["zero2ten"];

        jsonVariant["status"] = status;

        jsonVariant.createNestedObject("input");
        JsonVariant inputJson = jsonVariant["input"];

        size_t i=0;

        for (auto it = input_channels.begin(); it != input_channels.end(); ++it, ++i)
        {
            if (it->type != Channel::tUninitialized)
            {
                it->to_json(inputJson, i);
            }
        }

        jsonVariant.createNestedObject("output");
        JsonVariant outputJson = jsonVariant["output"];

        i=0;

        for (auto it = output_channels.begin(); it != output_channels.end(); ++it, ++i)
        {
            if (it->type != Channel::tUninitialized)
            {
                it->to_json(outputJson, i);
            }
        }

        jsonVariant.createNestedObject("applet");
        JsonVariant appletJson = jsonVariant["applet"];

        for (auto it = applets.begin(); it != applets.end(); ++it, ++i)
        {
            it->second.to_json(appletJson, it->first);
        }
    }

    struct Channel
    {
        enum Type
        {
            tUninitialized = -1,
            tInput = 0,
            tOutput = 1    
        };

        Channel()
        {
            reset();
        }

        Channel(Type _type)
        {
            reset();
            type = _type;
        }

        static const char * type_2_str(Type _type) 
        {
            switch(_type)
            {
               case tUninitialized: return "uninitialized";
                case tInput:        return "input";
                case tOutput:       return "output";
            }

            return "<unknown>";
        }

        void reset()
        {
            type = tUninitialized;
            value = 0;
            calibration_coefficient = -1;
            duty = 0;
            status.clear();
        }

        String as_string() const
        {
            String r = String("{") + "type=" + type_2_str(type) + 
                       ", value=" + String(value) +
                       ", calibration_coefficient=" + String(calibration_coefficient) +
                       ", duty=" + String(duty) +
                       ", status=" + status +
                       "}";
            return r;           
        }


        void to_json(JsonVariant & json, size_t index)
        {
            String name = "channel[" + String((int) index) + "]";
            json.createNestedObject(name);
            JsonVariant jsonVariant = json[name];

            jsonVariant["type"] = type_2_str(type);
            jsonVariant["value"] = value;
            jsonVariant["calibration_coefficient"] = calibration_coefficient;
            jsonVariant["duty"] = duty;
            jsonVariant["status"] = status;
        }

        float value;
        float calibration_coefficient;
        float duty;
        Type type;
        String status;
    };
    
    struct Applet
    {
        Applet()
        {
            clear();
        }

        void clear()
        {
            output_value = 0;
            temp = 0;
            function.clear();
            time.clear();
            temp_addr.clear();
            status.clear();
        }

        String as_string() const
        {
            String r = String("{") + "function=" + function + 
                       ", time=" + time +
                       ", temp_addr=" + temp_addr +
                       ", temp=" + String(temp) +
                       ", temp=" + String(temp) +
                       ", output_value=" + String(output_value) +
                       ", status=" + status +
                       "}";
            return r;           
        }


        void to_json(JsonVariant & json, const String & name)
        {
            json.createNestedObject(name);
            JsonVariant jsonVariant = json[name];

            jsonVariant["function"] = function;
            jsonVariant["time"] = time;
            jsonVariant["temp_addr"] = temp_addr;
            jsonVariant["temp"] = temp;
            jsonVariant["output_value"] = output_value;
            jsonVariant["status"] = status;
        }

        String function;
        String time;
        String temp_addr;
        float temp;
        float output_value;
        String status;
    };

    std::vector<Channel> input_channels;
    std::vector<Channel> output_channels;
    std::map<String, Applet> applets;
    String status;
};

void start_zero2ten_task(const Zero2tenConfig &);
void stop_zero2ten_task();
Zero2tenStatus get_zero2ten_status();

void reconfigure_zero2ten(const Zero2tenConfig &);

String zero2ten_calibrate_input(const String & channel_str, const String & value_str);
String zero2ten_input(const String & channel_str, String & value_str);
String zero2ten_calibrate_output(const String & channel_str, const String & value_str);
String zero2ten_output(const String & channel_str, const String & value_str);


#endif // INCLUDE_ZERO2TEN
