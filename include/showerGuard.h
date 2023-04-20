#ifdef INCLUDE_SHOWERGUARD

#include <ArduinoJson.h>
#include <digitalInputChannelConfig.h>
#include <analogInputChannelConfig.h>
#include <relayChannelConfig.h>
#include <genericChannelConfig.h>

class ShowerGuardConfig
{
    public:

        const uint8_t EPROM_VERSION = 3;

        ShowerGuardConfig()
        {
        }
        
        ShowerGuardConfig & operator = (const ShowerGuardConfig & config) 
        {
            motion = config.motion;
            rh = config.rh;
            temp = config.temp;
            lumi = config.lumi;
            light = config.light;
            fan = config.fan;

            return *this;
        }

        void from_json(const JsonVariant & json);

        void to_eprom(std::ostream & os) const;
        bool from_eprom(std::istream & is);

        bool is_valid() const;

        bool operator == (const ShowerGuardConfig & config) const
        {
            return motion == config.motion && rh == config.rh && temp == config.temp && lumi == config.lumi && 
                   light == config.light && fan == config.fan;
        }

        String as_string() const
        {
            return String("{motion=") + motion.as_string() + ", rh=" + rh.as_string() + ", temp=" + temp.as_string() + 
                   ", lumi=" + lumi.as_string() + ", light=" + light.as_string() + ", fan=" + fan.as_string() + "}";
        }

        // data

        struct Motion
        {
            Motion()
            {
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
                return channel == motion.channel;
            }

            String as_string() const
            {
                return String("{channel=") + channel.as_string() + "}";
            }
            
            DigitalInputChannelConfig channel;            

        };
        
        Motion motion;
    
        struct Rh
        {
            Rh()
            {
                corr = 0;
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
                return vad == rh.vad && vdd == rh.vdd && corr == rh.corr;
            }

            String as_string() const
            {
                return String("{vad=") + vad.as_string() + ", vdd=" + vdd.as_string() + ", corr=" + corr + "}";
            }
            
            AnalogInputChannelConfig vad;
            AnalogInputChannelConfig vdd;
            float corr;
            
        };
        
        Rh rh;

        struct Temp
        {
            Temp()
            {
                corr = 0;
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
        
        Temp temp;

        struct Lumi
        {
            Lumi()
            {
                clear();
            }

            void clear()
            {
                corr = 0;
                ldr.clear();
                threshold = 0;
            }

            bool is_configured() const
            {
                return ldr.is_valid();
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return true; // reservation for not installed == ldr's channel == -1
            }

            bool operator == (const Lumi & lumi) const
            {
                return ldr == lumi.ldr && corr == lumi.corr && threshold == lumi.threshold;
            }

            String as_string() const
            {
                return String("{ldr=") + ldr.as_string() + ", corr=" + corr + ", threshold=" + threshold + "}";
            }
            
            AnalogInputChannelConfig ldr;
            float corr;
            float threshold;            
        };
        
        Lumi lumi;

        struct Light
        {
            enum Mode
            {
                mOff  = 0,
                mOn   = 1,
                mAuto = 2,
            };

            static const char * mode_2_str(Mode mode) 
            {
                switch(mode)
                {
                    case mOff:  return "off";
                    case mOn:   return "on";
                    case mAuto: return "auto";
                }

                return "<unknown>";
            }

            static Mode str_2_mode(const char * str) 
            {
                if (!strcmp(str, mode_2_str(mOff)))
                {
                    return mOff;
                }
                else if (!strcmp(str, mode_2_str(mOn)))
                {
                    return mOn;
                }
                else if (!strcmp(str, mode_2_str(mAuto)))
                {
                    return mAuto;
                }

                return Mode(-1);
            }

            Light()
            {
                mode = mAuto; 
                linger = 60; // seconds

            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return channel.is_valid() && mode != Mode(-1);
            }

            bool operator == (const Light & light) const
            {
                return channel == light.channel && mode == light.mode  && linger == light.linger;
            }

            String as_string() const
            {
                return String("{channel=") + channel.as_string() + ", mode=" + mode_2_str(mode)  + ", linger=" + String(linger) + "}";
            }
            
            RelayChannelConfig channel;            
            Mode mode;
            uint16_t linger;
        };
        
        Light light;

        struct Fan : public Light
        {
            const uint8_t RH_ON = 55;
            const uint8_t RH_OFF = 50;

            Fan & operator = (const Fan & fan) 
            {
                Light::operator = (fan);
                
                rh_on = fan.rh_on;
                rh_off = fan.rh_off;
                
                return *this;
            }

            Fan()
            {
                rh_on = RH_ON;
                rh_off = RH_OFF;
            }

            void from_json(const JsonVariant & json);

            void to_eprom(std::ostream & os) const;
            bool from_eprom(std::istream & is);

            bool is_valid() const 
            {
                return Light::is_valid() && rh_on >= 0 && rh_on <= 100 && rh_off >= 0 && rh_off <= 100 && rh_off < rh_on;
            }

            bool operator == (const Fan & fan) const
            {
                return Light::operator == (fan) && rh_on == fan.rh_on && rh_off == fan.rh_off;
            }

            String as_string() const
            {
                String light_str = Light::as_string();

                return light_str.substring(0, light_str.length()-1) + ", rh_on=" + String(rh_on) + ", rh_off=" + String(rh_off) + "}";
            }

            uint8_t rh_on;
            uint8_t rh_off;

        };
        
        Fan fan;
};


struct ShowerGuardStatus
{
    ShowerGuardStatus()
    {
        temp = 0;
        rh = 0;
        luminance_percent = 0;
        light_luminance_mask = true;
        motion = false;
        light = false;
        fan = false;
        light_decision = "";
        fan_decision = "";
    }

    ShowerGuardStatus(const ShowerGuardStatus & other)
    {
        if (this == & other)
        {
            return;
        }        

        temp = other.temp;
        rh = other.rh;
        motion = other.motion;
        luminance_percent = other.luminance_percent;
        light_luminance_mask = other.light_luminance_mask;
        light = other.light;
        fan = other.fan;
        light_decision = other.light_decision;
        fan_decision = other.fan_decision;
    }

    ShowerGuardStatus & operator = (const ShowerGuardStatus & other)
    {
        if (this == & other)
        {
            return *this;
        }        

        temp = other.temp;
        rh = other.rh;
        motion = other.motion;
        luminance_percent = other.luminance_percent;
        light_luminance_mask = other.light_luminance_mask;
        light = other.light;
        fan = other.fan;
        light_decision = other.light_decision;
        fan_decision = other.fan_decision;

        return *this;
    }

    void to_json(JsonVariant & json)
    {
        json.createNestedObject("shower-guard");
        JsonVariant jsonVariant = json["shower-guard"];

        jsonVariant["temp"] = temp;
        jsonVariant["rh"] = rh;
        jsonVariant["motion"] = motion;
        jsonVariant["luminance_percent"] = luminance_percent;
        jsonVariant["light_luminance_mask"] = light_luminance_mask;
        jsonVariant["light"] = light;
        jsonVariant["fan"] = fan;
        jsonVariant["light_decision"] = light_decision;
        jsonVariant["fan_decision"] = fan_decision;
    }

    float temp;
    float rh;
    bool motion;
    float luminance_percent;
    bool light_luminance_mask;
    bool light;
    bool fan;
    String light_decision;
    String fan_decision;
};

void start_shower_guard_task(const ShowerGuardConfig &);
void stop_shower_guard_task();
ShowerGuardStatus get_shower_guard_status();

void reconfigure_shower_guard(const ShowerGuardConfig &);

#endif // INCLUDE_SHOWERGUARD
