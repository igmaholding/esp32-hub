#ifdef INCLUDE_PHASECHANGER

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


class PhaseChangerConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;

        PhaseChangerConfig()
        {
        }
        
        PhaseChangerConfig & operator = (const PhaseChangerConfig & config) 
        {
            input_v_channels = config.input_v_channels;
            input_i_high_channels = config.input_i_high_channels;
            input_i_low_channels = config.input_i_low_channels;
            applets = config.applets;

            return *this;
        }

        void clear()
        {
            input_v_channels.clear();
            input_i_high_channels.clear();
            input_i_low_channels.clear();
            applets.clear();
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);
        
        bool is_valid() const;
        
        bool operator == (const PhaseChangerConfig & config) const
        {
            if (input_v_channels.size() != config.input_v_channels.size())
            {
                return false;
            }

            if (input_i_high_channels.size() != config.input_i_high_channels.size())
            {
                return false;
            }

            if (input_i_low_channels.size() != config.input_i_low_channels.size())
            {
                return false;
            }

            if (applets.size() != config.applets.size())
            {
                return false;
            }

            // the order of channels is also important at comparison since channels are addressed 
            // by their index

            {auto dit = config.input_v_channels.begin();

            for (auto it = input_v_channels.begin(); it != input_v_channels.end(); ++it, ++dit)
            {
                if (!(*it == *dit))
                {
                    return false;
                }
            }}

            {auto dit = config.input_i_high_channels.begin();

            for (auto it = input_i_high_channels.begin(); it != input_i_high_channels.end(); ++it, ++dit)
            {
                if (!(*it == *dit))
                {
                    return false;
                }
            }}

            {auto dit = config.input_i_low_channels.begin();

            for (auto it = input_i_low_channels.begin(); it != input_i_low_channels.end(); ++it, ++dit)
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
            String r = "{input_v_channels=[";
            
            size_t i = 0;

            for (auto it = input_v_channels.begin(); it != input_v_channels.end(); ++it, ++i)
            {
                r += String(i) + ":" + input_v_channels[i].as_string();
                
                if (it+1 != input_v_channels.end())
                {
                    r += ", ";
                }
            }

            r += "{input_i_high_channels=[";
            
            i = 0;

            for (auto it = input_i_high_channels.begin(); it != input_i_high_channels.end(); ++it, ++i)
            {
                r += String(i) + ":" + input_i_high_channels[i].as_string();
                
                if (it+1 != input_i_high_channels.end())
                {
                    r += ", ";
                }
            }

            r += "{input_i_high_channels=[";
            
            i = 0;

            for (auto it = input_i_low_channels.begin(); it != input_i_low_channels.end(); ++it, ++i)
            {
                r += String(i) + ":" + input_i_low_channels[i].as_string();
                
                if (it+1 != input_i_low_channels.end())
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
        
        struct InputChannelIna3221 
        {
            const int MAX_CHANNEL = 2;

            InputChannelIna3221()
            {
                clear();
            }

            const InputChannelIna3221 & operator = (const InputChannelIna3221 & _channel)
            {
                if (this != & _channel)
                {
                    addr = _channel.addr; 
                    channel = _channel.channel;
                    ratio = _channel.ratio;
                }

                return *this;
            }

            void clear()
            {
                addr = 0;
                channel = -1;
                ratio = 1;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return addr >= 0x40 && addr <= 0x43 && channel >= 0 && channel <= MAX_CHANNEL;
            }

            bool operator == (const InputChannelIna3221 & _channel) const
            {
                return  addr == _channel.addr && channel == _channel.channel && ratio == _channel.ratio;
            }

            String as_string() const
            {
                return String("{addr=") + String(addr, 16) + 
                              ",channel=" + String(channel) + 
                              ",ratio=" + String(ratio) + 
                              "}";
            }
            
            uint8_t addr;
            int channel;

            // external voltage to read voltage ratio, consider the atten
            float ratio;  
        };

        std::vector<InputChannelIna3221> input_v_channels;
        std::vector<InputChannelIna3221> input_i_high_channels;
        std::vector<InputChannel> input_i_low_channels;
    
        struct Applet
        {
            enum Function
            {
            };

            static const char * function_2_str(Function function) 
            {
                switch(function)
                {
                }

                return "<unknown>";
            }

            static Function str_2_function(const char * str) 
            {
                /*if (!strcmp(str, function_2_str(fTemp2out)))
                {
                    return fTemp2out;
                }*/

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
                }

                return *this;
            }

            void clear()
            {
                function = Function(-1);
            }
            
            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return false;
            }

            bool operator == (const Applet & applet) const
            {
                if (function != applet.function)
                {
                    return false;
                }

                return true;
            }

            String as_string() const
            {
                String r = String("{") + "function=" + function_2_str(function);
                r += "}";

                return r;
            }
            
            Function function;
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


struct PhaseChangerStatus
{
    PhaseChangerStatus()
    {
    }

    String as_string() const
    {
        String r = "{status=\"" + status + "\"" 
                   ", input_v_channels=[";

        for (auto it=input_v_channels.begin(); it!= input_v_channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != input_v_channels.end())
            {
                r += ",";
            }
        }

        r += "], input_i_high_channels=[";

        for (auto it=input_i_high_channels.begin(); it!= input_i_high_channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != input_i_high_channels.end())
            {
                r += ",";
            }
        }

        r += "], input_i_low_channels=[";

        for (auto it=input_i_low_channels.begin(); it!= input_i_low_channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != input_i_low_channels.end())
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
        json.createNestedObject("phase-changer");
        JsonVariant jsonVariant = json["phase-changer"];

        jsonVariant["status"] = status;

        jsonVariant.createNestedObject("input_v");
        JsonVariant inputJson = jsonVariant["input_v"];

        size_t i=0;

        for (auto it = input_v_channels.begin(); it != input_v_channels.end(); ++it, ++i)
        {
            if (it->type != Channel::tUninitialized)
            {
                it->to_json(inputJson, i);
            }
        }

        jsonVariant.createNestedObject("input_i_high");
        inputJson = jsonVariant["input_i_high"];

        i=0;

        for (auto it = input_i_high_channels.begin(); it != input_i_high_channels.end(); ++it, ++i)
        {
            if (it->type != Channel::tUninitialized)
            {
                it->to_json(inputJson, i);
            }
        }

        jsonVariant.createNestedObject("input_i_low");
        inputJson = jsonVariant["input_i_low"];

        i=0;

        for (auto it = input_i_low_channels.begin(); it != input_i_low_channels.end(); ++it, ++i)
        {
            if (it->type != Channel::tUninitialized)
            {
                it->to_json(inputJson, i);
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
            tInputV = 0,
            tInputIHigh = 1,
            tInputILow = 2
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
                case tInputV:        return "input_v";
                case tInputIHigh:    return "input_i_high";
                case tInputILow:     return "input_i_low";
            }

            return "<unknown>";
        }

        void reset()
        {
            type = tUninitialized;
            value = 0;
            calibration_coefficient = -1;
            status.clear();
        }

        String as_string() const
        {
            String r = String("{") + "type=" + type_2_str(type) + 
                       ", value=" + String(value) +
                       ", calibration_coefficient=" + String(calibration_coefficient) +
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
            jsonVariant["status"] = status;
        }

        float value;
        float calibration_coefficient;
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
            status.clear();
        }

        String as_string() const
        {
            String r = String("{") + "function=" + function + 
                       ", time=" + time +
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
            jsonVariant["status"] = status;
        }

        String function;
        String time;
        String status;
    };

    std::vector<Channel> input_v_channels;
    std::vector<Channel> input_i_high_channels;
    std::vector<Channel> input_i_low_channels;
    std::map<String, Applet> applets;
    String status;
};

void start_phase_changer_task(const PhaseChangerConfig &);
void stop_phase_changer_task();
PhaseChangerStatus get_phase_changer_status();

void reconfigure_phase_changer(const PhaseChangerConfig &);

String phase_changer_calibrate_v(const String & channel_str, const String & value_str);
String phase_changer_calibrate_i_high(const String & channel_str, const String & value_str);
String phase_changer_calibrate_i_low(const String & channel_str, const String & value_str);
String phase_changer_input_v(const String & channel_str, String & value_str);
String phase_changer_input_i_high(const String & channel_str, String & value_str);
String phase_changer_input_i_low(const String & channel_str, String & value_str);


#endif // INCLUDE_PHASECHANGER
