#ifdef INCLUDE_PROPORTIONAL

#include <ArduinoJson.h>
#include <digitalInputChannelConfig.h>
#include <digitalOutputChannelConfig.h>

#include <map>
#include <vector>
#include <algorithm>


// define this to allow for motors which have different open and close time (normally not)

//#define ASYMMETRICAL_OPEN_CLOSE 1

// define this to allow serial execution of calibrations and actuations among channels so that there is always
// at most one motor in motion;
// this is needed to furher enhance calibration effect since engaging several motors at a time will sink
// the power supply (more the more motors are connected) thus rendering calibration inaccurate

#define USE_ACTION_QUEUE 1

class ProportionalConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;

        ProportionalConfig()
        {
        }
        
        ProportionalConfig & operator = (const ProportionalConfig & config) 
        {
            channels = config.channels;
            valve_profiles = config.valve_profiles;

            return *this;
        }

        void clear()
        {
            channels.clear();
            valve_profiles.clear();
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);
        
        bool is_valid() const;
        
        bool operator == (const ProportionalConfig & config) const
        {
            if (channels.size() != config.channels.size())
            {
                return false;
            }

            if (valve_profiles.size() != config.valve_profiles.size())
            {
                return false;
            }

            // the order of channels is also important at comparison since channels are addressed 
            // by their index

            {auto dit = config.channels.begin();

            for (auto it = channels.begin(); it != channels.end(); ++it, ++dit)
            {
                if (!(*it == *dit))
                {
                    return false;
                }
            }}

            for (auto it = valve_profiles.begin(); it != valve_profiles.end(); ++it)
            {
                auto dit = config.valve_profiles.find(it->first);

                if (dit == config.valve_profiles.end())
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
            String r = "{channels=[";
            
            size_t i = 0;

            for (auto it = channels.begin(); it != channels.end(); ++it, ++i)
            {
                r += String(i) + ":" + channels[i].as_string();
                
                if (it+1 != channels.end())
                {
                    r += ", ";
                }
            }
            r += "], valve_profiles=[";

            for (auto it = valve_profiles.begin(); it != valve_profiles.end(); ++it)
            {
                r += it->first + ":" + it->second.as_string();
                
                auto test_it = it;
                test_it++;

                if (test_it != valve_profiles.end())
                {
                    r += ", ";
                }
            }
            
            r += "]}";
            return r;
        }

        // data

        struct Channel
        {
            Channel()
            {
                clear();
            }

            const Channel & operator = (const Channel & channel)
            {
                if (this != & channel)
                {
                    one_a = channel.one_a;
                    one_b = channel.one_b;
                    open = channel.open;
                    closed = channel.closed;
                    valve_profile = channel.valve_profile;
                    default_value = channel.default_value;
                }

                return *this;
            }

            void clear()
            {
                default_value = 100;
                one_a.clear();
                one_b.clear();
                open.clear();
                closed.clear();
                valve_profile.clear();
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return one_a.is_valid() && one_b.is_valid() && 
                       closed.is_valid() && open.is_valid() &&
                       default_value >= 0 && default_value <= 100;
            }

            bool operator == (const Channel & channel) const
            {
                return one_a == channel.one_a && one_b == channel.one_b && 
                       open == channel.open && closed == channel.closed &&
                       valve_profile == channel.valve_profile &&
                       default_value == channel.default_value;
            }

            String as_string() const
            {
                return String("{one_a=") + one_a.as_string() + 
                              ",one_b=" + one_b.as_string() + 
                              ",open=" + open.as_string() + 
                              ",closed=" + closed.as_string() + 
                              ",valve_profile=" + valve_profile + 
                              ",default_value=" + String((int) default_value) + 
                              "}";
            }
            
            DigitalInputChannelConfig open;            
            DigitalInputChannelConfig closed;            

            DigitalOutputChannelConfig one_a;            
            DigitalOutputChannelConfig one_b;    

            String valve_profile;

            uint8_t default_value;
        };
        
        std::vector<Channel> channels;
    
        struct ValveProfile
        {
            const uint8_t DEFAULT_OPEN_TIME = 15;

            ValveProfile()
            {
                clear();
            }

            const ValveProfile & operator = (const ValveProfile & vp)
            {
                if (this != & vp)
                {
                    open_time = vp.open_time;
                    time_2_flow_rate = vp.time_2_flow_rate;
                }

                return *this;
            }

            void clear()
            {
                open_time = DEFAULT_OPEN_TIME;
                time_2_flow_rate.clear();
            }
            
            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return open_time > 0;
                // TODO: add check of time_2_flow_rate?
            }

            bool operator == (const ValveProfile & vp) const
            {
                if (open_time != vp.open_time)
                {
                    return false;
                }

                if (time_2_flow_rate.size() != vp.time_2_flow_rate.size())
                {
                    return false;
                }

                auto dit = vp.time_2_flow_rate.begin();

                for (auto it = time_2_flow_rate.begin(); it != time_2_flow_rate.end(); ++it, ++dit)
                {
                    if (*it != *dit)
                    {
                        return false;
                    }
                }
                return true;
            }

            String as_string() const
            {
                String r = String("{") + "open_time=" + String((int) open_time) +

                              ", time_2_flow_rate=[";

                for (auto it = time_2_flow_rate.begin(); it != time_2_flow_rate.end(); ++it)
                {
                    r += "[" + String(int(it->first)) + "," + String(int(it->second)) + "]";

                    if (it+1 != time_2_flow_rate.end())
                    {
                        r += ",";
                    }
                }

                r += "]}";

                return r;
            }
            
            uint8_t open_time;

            // time_2_flow_rate is a characteristics function that shows how much flow is enabled when
            // the valve has been opening for a certain time. example:
            //
            // [10, 5] 10% of open_time == 5% of flow
            //
            // the values are in percent; 
            //
            // the handler will approximate valve time according to the table to meet the requested flow;
            // if the table is empty the function is considered linear with k=1 at any point
            //
            // because the table contains ratios, the whole table in turn can be adjusted by calibration
            // which will measure the actual full open time
            //
            // the order of values have to be ascending

            std::vector<std::pair<uint8_t, uint8_t>> time_2_flow_rate; 
        };

        std::map<String, ValveProfile> valve_profiles;
};


struct ProportionalStatus
{
    ProportionalStatus()
    {
    }

    String as_string() const
    {
        String r = "channels=[";

        for (auto it=channels.begin(); it!= channels.end(); ++it)
        {
            r += it->as_string();

            if (it+1 != channels.end())
            {
                r += ",";
            }
        }

        r += "]";
        return r;
    }

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("proportional");
        JsonVariant jsonVariant = json["proportional"];

        for (auto it = channels.begin(); it != channels.end(); ++it)
        {
            if (it->state != Channel::sUninitialized)
            {
                it->to_json(jsonVariant);
            }
        }
    }

    struct Channel
    {
        Channel()
        {
            reset();
        }

        enum State
        {
            sUninitialized = -1,
            sIdle = 0,
            sActuating = 1,
            sCalibrating = 2    
        };

        static const char * state_2_str(State _state) 
        {
            switch(_state)
            {
               case sUninitialized: return "uninitialized";
                case sIdle:         return "idle";
                case sActuating:    return "actuating";
                case sCalibrating:  return "calibrating";
            }

            return "<unknown>";
        }

        void reset()
        {
            state = sUninitialized;
            error.clear();
            value = 0;
            config_open_time = 0;
    
            #ifdef ASYMMETRICAL_OPEN_CLOSE

            calib_open_2_closed_time = 0;
            calib_closed_2_open_time = 0;

            #else

            calib_open_time = 0;

            #endif // ASYMMETRICAL_OPEN_CLOSE

        }

        String as_string() const
        {
            String r = String("{") + "state=" + state_2_str(state) + 
                       ", error=" + error + 
                       ", value=" + String((int)value) + 
                       ", config_open_time=" + String((int)config_open_time) + 

                        #ifdef ASYMMETRICAL_OPEN_CLOSE

                       ", calib_open_2_closed_time=" + String((int)calib_open_2_closed_time) + 
                       ", calib_closed_2_open_time=" + String((int)calib_closed_2_open_time) + 

                        #else

                       ", calib_open_time=" + String((int)calib_open_time) + 

                        #endif // ASYMMETRICAL_OPEN_CLOSE

                       "}";
            return r;           
        }


        void to_json(JsonVariant & json)
        {
            json.createNestedObject("channel");
            JsonVariant jsonVariant = json["channel"];

            jsonVariant["state"] = state_2_str(state);
            jsonVariant["error"] = error;
            jsonVariant["value"] = value;
            jsonVariant["config_open_time"] = config_open_time;
    
            #ifdef ASYMMETRICAL_OPEN_CLOSE

            jsonVariant["calib_open_2_closed_time"] = calib_open_2_closed_time;
            jsonVariant["calib_closed_2_open_time"] = calib_closed_2_open_time;

            #else

            jsonVariant["calib_open_time"] = calib_open_time;

            #endif // ASYMMETRICAL_OPEN_CLOSE
        }

        uint8_t value;
        State state;
        String error;

        uint32_t config_open_time;

        #ifdef ASYMMETRICAL_OPEN_CLOSE

        uint32_t calib_open_2_closed_time;
        uint32_t calib_closed_2_open_time;

        #else

        uint32_t calib_open_time;

        #endif // ASYMMETRICAL_OPEN_CLOSE
    };
    
    std::vector<Channel> channels;
};

void start_proportional_task(const ProportionalConfig &);
void stop_proportional_task();
ProportionalStatus get_proportional_status();

void reconfigure_proportional(const ProportionalConfig &);

String proportional_calibrate(const String & channel_str);
String proportional_actuate(const String & channel_str, const String & value_str);


#endif // INCLUDE_PROPORTIONAL
