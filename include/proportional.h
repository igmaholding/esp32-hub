#ifdef INCLUDE_PROPORTIONAL

#include <ArduinoJson.h>
#include <digitalInputChannelConfig.h>
#include <digitalOutputChannelConfig.h>

#include <map>
#include <vector>


#define PROPORTIONAL_NUM_CHANNELS 6 


class ProportionalConfig
{
    public:

        const uint8_t EPROM_VERSION = 1;
        static const size_t NUM_CHANNELS = PROPORTIONAL_NUM_CHANNELS;

        ProportionalConfig()
        {
        }
        
        ProportionalConfig & operator = (const ProportionalConfig & config) 
        {
            for (size_t i=0; i<sizeof(channels)/sizeof(channels[0]); ++i)
            {
                channels[i] = config.channels[i];
            }

            return *this;
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const
        {
                for (size_t i=0; i<sizeof(channels)/sizeof(channels[0]); ++i)
                {
                    if (channels[i].is_valid() == false)
                        return false;
                }

                return true;
        }

        bool operator == (const ProportionalConfig & config) const
        {
            for (size_t i=0; i<sizeof(channels)/sizeof(channels[0]); ++i)
            {
                if (!(channels[i] == config.channels[i]))
                    return false;
            }
            return true; 
        }

        String as_string() const
        {
            String r = "{channels=[";

            for (size_t i=0; i<sizeof(channels)/sizeof(channels[0]); ++i)
            {
                r += String(i) + ":" + channels[i].as_string();
                
                if (i < sizeof(channels)/sizeof(channels[0])-1)
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
                }

                return *this;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return one_a.is_valid() && one_b.is_valid() && 
                       closed.is_valid() && open.is_valid();
            }

            bool operator == (const Channel & channel) const
            {
                return one_a == channel.one_a && one_b == channel.one_b && 
                       open == channel.open && closed == channel.closed &&
                       valve_profile == channel.valve_profile;
            }

            String as_string() const
            {
                return String("{one_a=") + one_a.as_string() + 
                              ",one_b=" + one_b.as_string() + 
                              ",open=" + open.as_string() + 
                              ",closed=" + closed.as_string() + 
                              ",valve_profile=" + valve_profile + 
                              "}";
            }
            
            DigitalInputChannelConfig open;            
            DigitalInputChannelConfig closed;            

            DigitalOutputChannelConfig one_a;            
            DigitalOutputChannelConfig one_b;    

            String valve_profile;
        };
        
        Channel channels[NUM_CHANNELS];
    
        struct ValveProfile
        {
            const uint8_t DEFAULT_OPEN_TIME = 15;

            ValveProfile()
            {
                open_time = DEFAULT_OPEN_TIME;
            }

            const ValveProfile & operator = (const ValveProfile & vp)
            {
                if (this != & vp)
                {
                    open_time = vp.open_time;
                }

                return *this;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return open_time > 0;
            }

            bool operator == (const ValveProfile & vp) const
            {
                return open_time == vp.open_time;
            }

            String as_string() const
            {
                return String("{") +
                              ",open_time=" + String((int) open_time) + 
                              "}";
            }
            
            uint8_t open_time;

            // time_2_flow_rate is a characteristics function that shows how much flow is enabled when
            // the valve has been opening for a certain time. example:
            //
            // (10% of open_time / 5% of flow) = 200%, (30% of open_time / 50% of flow) = 60%
            //
            // the values are in percent; 
            //
            // the handler will approximate valve time according to the table to meet the requested flow;
            // if the table is empty the function is considered linear,  100% at any point
            //
            // because the table contains ratios, the whole table in turn can be adjusted by calibration
            // which will measure the actual full open time

            std::vector<uint16_t> time_2_flow_rate;
        };

        std::map<String, ValveProfile> valve_profiles;
};


struct ProportionalStatus
{
    static const size_t NUM_CHANNELS = PROPORTIONAL_NUM_CHANNELS;

    ProportionalStatus()
    {
    }

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("proportional");
        JsonVariant jsonVariant = json["proportional"];

        for (size_t i=0; i<sizeof(channels)/sizeof(channels[0]); ++i)
        {
            if (channels[i].state != Channel::sUninitialized)
            {
                channels[i].to_json(json);
            }
        }
    }

    struct Channel
    {
        Channel()
        {
            state = sUninitialized;
            value = 0;
            config_open_time = 0;
            calib_open_2_closed_time = 0;
            calib_closed_2_open_time = 0;
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

        void to_json(JsonVariant & json)
        {
            json.createNestedObject("channel");
            JsonVariant jsonVariant = json["channel"];

            jsonVariant["state"] = state_2_str(state);
            jsonVariant["value"] = value;
            jsonVariant["config_open_time"] = config_open_time;
            jsonVariant["calib_open_2_closed_time"] = calib_open_2_closed_time;
            jsonVariant["calib_closed_2_open_time"] = calib_closed_2_open_time;
        }

        uint8_t value;
        State state;

        uint16_t config_open_time;
        uint16_t calib_open_2_closed_time;
        uint16_t calib_closed_2_open_time;
    };
    
    Channel channels[NUM_CHANNELS];
};

void start_proportional_task(const ProportionalConfig &);
void stop_proportional_task();
ProportionalStatus get_proportional_status();

void reconfigure_proportional(const ProportionalConfig &);

#endif // INCLUDE_PROPORTIONAL
