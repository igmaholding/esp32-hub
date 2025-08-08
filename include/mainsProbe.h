#ifdef INCLUDE_MAINSPROBE

#include <ArduinoJson.h>
#include <digitalInputChannelConfig.h>
#include <digitalOutputChannelConfig.h>
#include <analogInputChannelConfig.h>
#include <genericChannelConfig.h>
#include <mapTable.h>
#include <trace.h>
#include <ina3221.h>

#include <map>
#include <vector>
#include <algorithm>


class MainsProbeConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;

        enum InputWhat
        {
            iwV     = 0,
            iwAHigh = 1,
            iwALow  = 2
        };

        MainsProbeConfig()
        {
            clear();
        }
        
        MainsProbeConfig & operator = (const MainsProbeConfig & config) 
        {
            i2c = config.i2c;
            input_v = config.input_v;
            input_a_high = config.input_a_high;
            input_a_low_channels = config.input_a_low_channels;
            applets = config.applets;

            return *this;
        }

        void clear()
        {
            i2c.clear();
            input_v.clear();
            input_a_high.clear();
            input_a_low_channels.clear();
            applets.clear();
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);
        
        bool is_valid() const;
        
        bool operator == (const MainsProbeConfig & other) const
        {
            if (!(i2c == other.i2c))
            {
                return false;
            }

            if (input_v.size() != other.input_v.size())
            {
                return false;
            }

            if (input_a_high.size() != other.input_a_high.size())
            {
                return false;
            }

            if (input_a_low_channels.size() != other.input_a_low_channels.size())
            {
                return false;
            }

            if (applets.size() != other.applets.size())
            {
                return false;
            }

            // the order of Ina3221Config instances is not important at comparison; this may not work well
            // in case of duplicate instances but then it should be captured by is_valid() in the first place

            for (auto it = input_v.begin(); it != input_v.end(); ++it)
            {
                bool found = false;

                for (auto dit = other.input_v.begin(); dit != other.input_v.end(); ++dit)
                {
                    if (*it == *dit)
                    {
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    return false;
                }
            }

            for (auto it = input_a_high.begin(); it != input_a_high.end(); ++it)
            {
                bool found = false;

                for (auto dit = other.input_a_high.begin(); dit != other.input_a_high.end(); ++dit)
                {
                    if (*it == *dit)
                    {
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    return false;
                }
            }

            // the order of these channels is important at comparison since they are addressed by their index

            { auto dit = other.input_a_low_channels.begin();

            for (auto it = input_a_low_channels.begin(); it != input_a_low_channels.end(); ++it, ++dit)
            {
                if (!(*it == *dit))
                {
                    return false;
                }
            }}

            for (auto it = applets.begin(); it != applets.end(); ++it)
            {
                auto dit = other.applets.find(it->first);

                if (dit == other.applets.end())
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
            String r = "{i2c=" + i2c.as_string() + ", input_v=[";
            
            size_t i = 0;

            for (auto it = input_v.begin(); it != input_v.end(); ++it, ++i)
            {
                r += String(i) + ":" + input_v[i].as_string();
                
                if (it+1 != input_v.end())
                {
                    r += ", ";
                }
            }

            r += "], input_a_high=[";
            
            i = 0;

            for (auto it = input_a_high.begin(); it != input_a_high.end(); ++it, ++i)
            {
                r += String(i) + ":" + input_a_high[i].as_string();
                
                if (it+1 != input_a_high.end())
                {
                    r += ", ";
                }
            }

            r += "], input_a_low_channels=[";
            
            i = 0;

            for (auto it = input_a_low_channels.begin(); it != input_a_low_channels.end(); ++it, ++i)
            {
                r += String(i) + ":" + input_a_low_channels[i].as_string();
                
                if (it+1 != input_a_low_channels.end())
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
                    default_poly_beta = channel.default_poly_beta;
                }

                return *this;
            }

            void clear()
            {
                AnalogInputChannelConfig::clear();
                default_poly_beta.clear();
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
                return AnalogInputChannelConfig::operator == (channel) && default_poly_beta == channel.default_poly_beta;
            }

            String as_string() const
            {
                String r = String("{channel=") + AnalogInputChannelConfig::as_string() + ",default_poly_beta=[";
                              
                for (size_t i=0;i<default_poly_beta.size();++i)
                {
                    r += String(default_poly_beta[i]);

                    if (i+1<default_poly_beta.size())
                    {
                        r += ",";
                    }
                }
                r += "]}";
                return r;
            }
            
            // poly_beta applied by default to calculate out value if no usable calibration is present
            std::vector<float> default_poly_beta;  
        };
        
        struct InputChannelIna3221 
        {
            const int MAX_CHANNEL = 2;

            InputChannelIna3221()
            {
                clear();
            }

            InputChannelIna3221(const InputChannelIna3221 & _channel)
            {
                channel = _channel.channel;
                default_poly_beta = _channel.default_poly_beta;
            }

            const InputChannelIna3221 & operator = (const InputChannelIna3221 & _channel)
            {
                if (this != & _channel)
                {
                    channel = _channel.channel;
                    default_poly_beta = _channel.default_poly_beta;
                }

                return *this;
            }

            void clear()
            {
                channel = -1;
                default_poly_beta.clear();
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return channel >= 0 && channel <= MAX_CHANNEL;
            }

            bool operator == (const InputChannelIna3221 & _channel) const
            {
                return  channel == _channel.channel && default_poly_beta == _channel.default_poly_beta;
            }

            String as_string() const
            {
                String r = String("{channel=") + String(channel) +  + ",default_poly_beta=[";
                              
                for (size_t i=0;i<default_poly_beta.size();++i)
                {
                    r += String(default_poly_beta[i]);

                    if (i+1<default_poly_beta.size())
                    {
                        r += ",";
                    }
                }
                r += "]}";
                return r;
            }
            
            int channel;

            // poly_beta applied by default to calculate out value if no usable calibration is present
            std::vector<float> default_poly_beta;  
        };

        struct Ina3221Config 
        {
            Ina3221Config()
            {
                clear();
            }

            const Ina3221Config & operator = (const Ina3221Config & other)
            {
                if (this != & other)
                {
                    addr = other.addr;
                    channels = other.channels;
                }

                return *this;
            }

            void clear()
            {
                addr = Ina3221::ADDR_MIN;
                channels.clear();
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                if (addr < Ina3221::ADDR_MIN || addr > Ina3221::ADDR_MAX)
                {
                    return false;
                }

                std::vector<int> used_channel_index;

                for (auto it=channels.begin(); it!=channels.end(); ++it)
                {
                    if (it->is_valid() == false)
                    {
                        return false;
                    }

                    if (std::find(used_channel_index.begin(), used_channel_index.end(), it->channel) != used_channel_index.end())
                    {
                        return false;
                    }

                    used_channel_index.push_back(it->channel);
                }

                return true;
            }

            bool operator == (const Ina3221Config & other) const
            {
                if (addr != other.addr)
                {
                    return false;
                } 
                
                if (channels.size() != other.channels.size())
                {
                    return false;
                } 

                // the order of channels is not important at comparison; this may not work well
                // in case of duplicate channels but then it should be captured by is_valid() in the first place

                for (auto it = channels.begin(); it != channels.end(); ++it)
                {
                    bool found = false;

                    for (auto dit = other.channels.begin(); dit != other.channels.end(); ++dit)
                    {
                        if (*it == *dit)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found == false)
                    {
                        return false;
                    }
                }
    
                return true;
            }

            String as_string() const
            {
                String r = String("{addr=0x") + String((int) addr, 16) + ",channels=[";

                for (auto it=channels.begin(); it!=channels.end(); ++it)
                {
                    r += it->as_string();

                    if (it+1 != channels.end())
                    {
                        r += ",";
                    }
                }

                r += "]}";
                return r;
            }
            
            uint8_t addr;
            std::vector<InputChannelIna3221> channels;
        };

        std::vector<Ina3221Config> input_v;
        std::vector<Ina3221Config> input_a_high;
        std::vector<InputChannel> input_a_low_channels;
    
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

        I2c i2c;
};


inline void _set_not_calibrated(float & value)
{
    value = -1;
}

inline bool _is_calibrated(float value)
{
    return value > 0;
}


struct MainsProbeStatus
{
    MainsProbeStatus()
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

        r += "], input_a_high_channels=[";

        for (auto it=input_a_high_channels.begin(); it!= input_a_high_channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != input_a_high_channels.end())
            {
                r += ",";
            }
        }

        r += "], input_a_low_channels=[";

        for (auto it=input_a_low_channels.begin(); it!= input_a_low_channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != input_a_low_channels.end())
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
        json.createNestedObject("mains-probe");
        JsonVariant jsonVariant = json["mains-probe"];

        jsonVariant["status"] = status;

        // restructure input_v_channels and input_a_high_channels so that to add an extra level for addr:
        //
        // { "input_v_channels" : { "0x40" : { "channel[0]": {}} }} 
        //
        // instead of having the address as in the channel definition; this will facilitate machine
        // finding data using key hierarchy : ["input_v_channels", "0x40", "channel[0]"]

        jsonVariant.createNestedObject("input_v_channels");

        { std::vector<uint8_t> addr;

        for (auto it = input_v_channels.begin(); it != input_v_channels.end(); ++it)
        {
            if (std::find(addr.begin(), addr.end(), it->addr) == addr.end())
            {
                addr.push_back(it->addr);
            }
        }

        JsonVariant inputJson = jsonVariant["input_v_channels"];

        for (auto jt = addr.begin(); jt != addr.end(); ++jt)
        {
            size_t i = 0;

            char buf[16];
            sprintf(buf, "0x%02x", (int) *jt);

            inputJson.createNestedObject(buf);
            JsonVariant _inputJson = inputJson[buf];

            for (auto it = input_v_channels.begin(); it != input_v_channels.end(); ++it)
            {
                if (it->addr == *jt)
                {
                    it->to_json(_inputJson, i);
                    ++i;
                }
            }
        }}

        jsonVariant.createNestedObject("input_a_high_channels");

        { std::vector<uint8_t> addr;

            for (auto it = input_a_high_channels.begin(); it != input_a_high_channels.end(); ++it)
            {
                if (std::find(addr.begin(), addr.end(), it->addr) == addr.end())
                {
                    addr.push_back(it->addr);
                }
            }
    
            JsonVariant inputJson = jsonVariant["input_a_high_channels"];
    
            for (auto jt = addr.begin(); jt != addr.end(); ++jt)
            {
                size_t i = 0;
    
                char buf[16];
                sprintf(buf, "0x%02x", (int) *jt);
    
                inputJson.createNestedObject(buf);
                JsonVariant _inputJson = inputJson[buf];
    
                for (auto it = input_a_high_channels.begin(); it != input_a_high_channels.end(); ++it)
                {
                    if (it->addr == *jt)
                    {
                        it->to_json(_inputJson, i);
                        ++i;
                    }
                }
            }}
    

        jsonVariant.createNestedObject("input_a_low_channels");
        JsonVariant inputJson = jsonVariant["input_a_low_channels"];

        size_t i=0;

        for (auto it = input_a_low_channels.begin(); it != input_a_low_channels.end(); ++it, ++i)
        {
            it->to_json(inputJson, i);
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
        Channel()
        {
            clear();
        }

        void clear()
        {
            addr = 0;
            index = -1;

            value = 0;
            status.clear();
        }

        String as_string() const
        {
            String r = String("{") + "addr=0x" + String(addr, 16) + 
                       ",index=" + String(index) + 
                       ", value=" + String(value) +
                       ", status=" + status +
                       "}";
            return r;           
        }

        void to_json(JsonVariant & json, size_t index)
        {
            String name = "channel[" + String((int) index) + "]";
            json.createNestedObject(name);
            JsonVariant jsonVariant = json[name];

            // jsonVariant["addr"] = String("0x") + String(addr, 16);
            // jsonVariant["index"] = index;

            jsonVariant["value"] = value;
            jsonVariant["status"] = status;
        }

        uint8_t addr;  // not used for input_a_low
        int index;    // index for input_a_low is induced by the order in the array, for others it is specified channel number

        float value;
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
    std::vector<Channel> input_a_high_channels;
    std::vector<Channel> input_a_low_channels;
    std::map<String, Applet> applets;
    String status;
};

void start_mains_probe_task(const MainsProbeConfig &);
void stop_mains_probe_task();
MainsProbeStatus get_mains_probe_status();

void reconfigure_mains_probe(const MainsProbeConfig &);

String mains_probe_calibrate_v(const String & addr_str, const String & channel_str, const String & value_str);
String mains_probe_calibrate_a_high(const String & addr_str, const String & channel_str, const String & value_str);
String mains_probe_calibrate_a_low(const String & channel_str, const String & value_str);
String mains_probe_input_v(const String & addr_str, const String & channel_str, String & value_str);
String mains_probe_input_a_high(const String & addr_str, const String & channel_str, String & value_str);
String mains_probe_input_a_low(const String & channel_str, String & value_str);
void mains_probe_get_calibration_data(JsonVariant &);
String mains_probe_import_calibration_data(const JsonVariant & json);


#endif // INCLUDE_MAINSPROBE
