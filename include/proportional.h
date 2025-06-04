#ifdef INCLUDE_PROPORTIONAL

#include <ArduinoJson.h>
#include <digitalInputChannelConfig.h>
#include <digitalOutputChannelConfig.h>
#include <analogInputChannelConfig.h>
#include <trace.h>

#include <map>
#include <vector>
#include <algorithm>


// define this to allow for motors which have different open and close time (normally not)

#define ASYMMETRICAL_OPEN_CLOSE 1

// define this to allow serial execution of calibrations and actuations among channels so that there is always
// at most one motor in motion;
// this is needed to furher enhance calibration effect since engaging several motors at a time will sink
// the power supply (more the more motors are connected) thus rendering calibration inaccurate

#define USE_ACTION_QUEUE 1

class ProportionalConfig
{
    public:

        const uint8_t EPROM_VERSION = 2;

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
            struct LoadDetect
            {
                LoadDetect()
                {
                    clear();
                }

                void clear()
                {
                    pin.clear();
                    resistance = 0;
                    current_threshold = 0;
                }

                void from_json(const JsonVariant & json);

                void to_eprom(std::ostream & os) const;
                bool from_eprom(std::istream & is);

                bool is_valid() const 
                {
                    if (pin.is_valid() && resistance > 0 && current_threshold > 0)
                    {
                        return true;
                    }

                    ERROR("LoadDetect is_valid() == false")
                    return false;
                }

                bool operator == (const LoadDetect & load_detect) const
                {
                    return pin == load_detect.pin && 
                           resistance == load_detect.resistance && 
                           current_threshold == load_detect.current_threshold;
                }

                String as_string() const
                {
                    String r = String("{pin=") + pin.as_string() +
                                ",resistance=" + String(resistance) + 
                                ",current_threshold=" + String(current_threshold) +
                                "}"; 
                    return r;
                }
                
                AnalogInputChannelConfig pin;

                float resistance;
                float current_threshold;
            };
            
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
                    load_detect = channel.load_detect;
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
                load_detect.clear();
                valve_profile.clear();
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                if (one_a.is_valid() == false)
                {
                    ERROR("one_a.is_valid() == false")
                    return false;
                }

                if (one_b.is_valid() == false)
                {
                    ERROR("one_b.is_valid() == false")
                    return false;
                }

                if (closed.is_valid() == false)
                {
                    ERROR("closed.is_valid() == false")
                    return false;
                }

                if (open.is_valid() == false)
                {
                    ERROR("open.is_valid() == false")
                    return false;
                }

                if (default_value < 0 || default_value > 100)
                {
                    ERROR("default_value is not valid")
                    return false;
                }

                if (load_detect.is_valid() == false)
                {
                    return false;
                }
                return true;
            }

            bool operator == (const Channel & channel) const
            {
                return one_a == channel.one_a && one_b == channel.one_b && 
                       open == channel.open && closed == channel.closed &&
                       load_detect == channel.load_detect && 
                       valve_profile == channel.valve_profile &&
                       default_value == channel.default_value;
            }

            bool will_need_calibrate(const Channel & new_config) const
            {
                return !(one_a == new_config.one_a) || !(one_b == new_config.one_b);
            }

            bool will_need_actuate(const Channel & new_config) const
            {
                return will_need_calibrate(new_config) || !(valve_profile == new_config.valve_profile);
            }

            String as_string() const
            {
                return String("{one_a=") + one_a.as_string() + 
                              ",one_b=" + one_b.as_string() + 
                              ",open=" + open.as_string() + 
                              ",closed=" + closed.as_string() +
                              ",load_detect=" + load_detect.as_string() +
                              ",valve_profile=" + valve_profile + 
                              ",default_value=" + String((int) default_value) + 
                              "}";
            }
            
            DigitalInputChannelConfig open;            
            DigitalInputChannelConfig closed;            

            DigitalOutputChannelConfig one_a;            
            DigitalOutputChannelConfig one_b;    

            LoadDetect load_detect;

            String valve_profile;

            uint8_t default_value;
        };
        
        std::vector<Channel> channels;
    
        struct ValveProfile
        {
            const uint8_t DEFAULT_OPEN_TIME = 15;
            const uint8_t DEFAULT_MAX_ACTUATE_ADD_UPS = 1;

            ValveProfile()
            {
                clear();
            }

            const ValveProfile & operator = (const ValveProfile & vp)
            {
                if (this != & vp)
                {
                    open_time = vp.open_time;
                    max_actuate_add_ups = vp.max_actuate_add_ups;
                    time_2_flow_rate = vp.time_2_flow_rate;
                }

                return *this;
            }

            void clear()
            {
                open_time = DEFAULT_OPEN_TIME;
                max_actuate_add_ups = DEFAULT_MAX_ACTUATE_ADD_UPS;
                time_2_flow_rate.clear();
            }
            
            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return open_time > 0 && max_actuate_add_ups > 0;
                // TODO: add check of time_2_flow_rate?
            }

            bool operator == (const ValveProfile & vp) const
            {
                if (open_time != vp.open_time)
                {
                    return false;
                }

                if (max_actuate_add_ups != vp.max_actuate_add_ups)
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
                              "max_actuate_add_ups=" + String((int) max_actuate_add_ups) +

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
            
            // this is a "configured" open time which is not used for anything except info/timeouts right now
            // because calibration is a necessary condition 

            uint8_t open_time;  

            // this indicates how many actuations will be made base on the previous actuation as a reference;
            // after max_actuate_add_ups the motor is reset to a nearest end-state and the actuation
            // is done from there

            uint8_t max_actuate_add_ups;

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

        size_t i=0;

        for (auto it = channels.begin(); it != channels.end(); ++it, ++i)
        {
            if (it->state != Channel::sUninitialized)
            {
                it->to_json(jsonVariant, i);
            }
        }
    }

    struct Channel
    {
        Channel()
        {
            clear();
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

        void clear_keep_calib_data()
        {
            state = sUninitialized;
            error.clear();
            value = 0;
            config_open_time = 0;

            actuate_add_ups = 0;
            max_actuate_add_ups = 1;
        }

        void clear()
        {
            clear_keep_calib_data();
    
            #ifdef ASYMMETRICAL_OPEN_CLOSE

            calib_open_2_closed_time = 0;
            calib_closed_2_open_time = 0;

            #else

            calib_open_time = 0;

            #endif // ASYMMETRICAL_OPEN_CLOSE

            actuate_add_ups = 0;
            max_actuate_add_ups = 1;
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

                       ", actuate_add_ups=" + String((int)actuate_add_ups) + 
                       ", max_actuate_add_ups=" + String((int)max_actuate_add_ups) + 

                       "}";
            return r;           
        }


        void to_json(JsonVariant & json, size_t index)
        {
            String name = "channel[" + String((int) index) + "]";
            json.createNestedObject(name);
            JsonVariant jsonVariant = json[name];

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

            jsonVariant["actuate_add_ups"] = actuate_add_ups;
            jsonVariant["max_actuate_add_ups"] = max_actuate_add_ups;
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

        uint8_t actuate_add_ups;
        uint8_t max_actuate_add_ups;
    };
    
    std::vector<Channel> channels;
};

void start_proportional_task(const ProportionalConfig &);
void stop_proportional_task();
ProportionalStatus get_proportional_status();

void reconfigure_proportional(const ProportionalConfig &);

String proportional_calibrate(const String & channel_str);
String proportional_actuate(const String & channel_str, const String & value_str, const String & ref_str);


#endif // INCLUDE_PROPORTIONAL
