#ifdef INCLUDE_KEYBOX

#include <ArduinoJson.h>

#define KEYBOX_NUM_CHANNELS 15  // max 32, limited by KeyBoxStatus; if change also update the Actuator.addr



struct KeyboxActuatorConfig
{
    static const size_t NUM_CHANNELS = KEYBOX_NUM_CHANNELS;

    KeyboxActuatorConfig()
    {
    }

    KeyboxActuatorConfig(const KeyboxActuatorConfig & actuator)
    {
        *this = actuator;
    }

    KeyboxActuatorConfig & operator = (const KeyboxActuatorConfig & actuator) 
    {
        if (this != & actuator)
        {
            addr[0] = actuator.addr[0];
            addr[1] = actuator.addr[1];
            addr[2] = actuator.addr[2];
            addr[3] = actuator.addr[3];
            
            latch = actuator.latch;
            power = actuator.power;
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
                latch.is_valid() && power.is_valid() && status.is_valid();
    }

    bool operator == (const KeyboxActuatorConfig & actuator) const
    {
        return addr[0] == actuator.addr[0] && addr[1] == actuator.addr[1] && addr[2] == actuator.addr[2] && addr[3] == actuator.addr[3] && 
                latch == actuator.latch && power == actuator.power && status == actuator.status; 
    }

    String as_string() const
    {
        return String("{addr=[") + addr[0].as_string() + ", " + addr[1].as_string() + ", " + addr[2].as_string() + ", " + addr[3].as_string() + "], " +
                        "latch=" + latch.as_string() + ", power=" + power.as_string() + ", status=" + status.as_string() + "}";
    }

    struct Channel
    {
        Channel()
        {
            clear();
        }
    
        void clear()
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
    Channel latch;            
    Channel power;            
    Channel status;            

};


struct KeyboxStatus
{
    static const size_t NUM_CHANNELS = KEYBOX_NUM_CHANNELS;

    KeyboxStatus()
    {
        clear();
    }

    void clear() 
    {
        value = 0;
    }
    
    bool operator == (const KeyboxStatus & status) const
    {
        return status.value == value;
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

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("keybox");
        JsonVariant jsonVariant = json["keybox"];
        jsonVariant["status"] = as_string() + " (1==unlocked)";
    }

    String as_string() const
    {
        char buf[KEYBOX_NUM_CHANNELS*5+1];
        char * ptr = buf;

        for (size_t i=0; i<KEYBOX_NUM_CHANNELS;++i)
        {
            sprintf(ptr, "%d:%d%s", (int) i, (int) ((value & (1 << i)) ? 1 : 0), (i == KEYBOX_NUM_CHANNELS-1 ? "" : " "));
            //sprintf(ptr, "%d:%d ", (int) i, (int) ((value & (1 << i)) ? 1 : 0));

            ptr = buf + strlen(buf);
        }

        return buf;
    }

    uint32_t value;
};


KeyboxStatus get_keybox_status();
void actuate_channel(size_t);

void start_keybox_actuator_task(const KeyboxActuatorConfig &);
void stop_keybox_actuator_task();
void reconfigure_keybox_actuator_task(const KeyboxActuatorConfig &);

#endif // INCLUDE_KEYBOX


