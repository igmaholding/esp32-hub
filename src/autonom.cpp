#include <Arduino.h>
#include <ArduinoJson.h>
#include <eeprom.h>
#include <map>
#include <string>
#include <sstream>

#include <autonom.h>
#include <showerGuard.h>
#include <trace.h>
#include <epromImage.h>


class AutonomTaskManager
{
    public:

        AutonomTaskManager()
        {
            showerGuardActive = false;
        }

        void startShowerGuard(const ShowerGuardConfig &);
        void stopShowerGuard();
        void reconfigureShowerGuard(const ShowerGuardConfig &);

        ShowerGuardStatus getShowerGuardStatus() const;

        bool isShowerGuardActive() const { return showerGuardActive; }

        void stopAll();


    protected:

        bool showerGuardActive;

};


void AutonomTaskManager::startShowerGuard(const ShowerGuardConfig & config)
{
    TRACE("Starting autonom task showerGuard")
    start_shower_guard_task(config);
    showerGuardActive = true;
}


void AutonomTaskManager::stopShowerGuard()
{
    TRACE("stopping autonom task showerGuard")
    stop_shower_guard_task();
    showerGuardActive = false;
}


void AutonomTaskManager::reconfigureShowerGuard(const ShowerGuardConfig & config)
{
    TRACE("reconfiguring autonom task showerGuard")
    reconfigure_shower_guard(config);
}


ShowerGuardStatus AutonomTaskManager::getShowerGuardStatus() const
{
    TRACE("getShowerGuardStatuss")
    return get_shower_guard_status();
}


void AutonomTaskManager::stopAll()
{
    TRACE("stopping all autonom tasks")

    if (showerGuardActive)
    {
        stopShowerGuard();
    }
}


AutonomTaskManager autonomTaskManager;


const char * function_type_2_str(FunctionType ft)
{
    switch(ft)
    {
        case ftShowerGuard:
            return "shower-guard";
        default:
            return "<unknown>";    
    }
}


String setupAutonom(const JsonVariant & json) 
{
    TRACE("setupAutonom")

    EpromImage epromImage;
    char buf[256];

    ShowerGuardConfig showerGuardConfig;

    if (json.is<JsonArray>())
    {
        DEBUG("payload is array")
        const JsonArray & jsonArray = json.as<JsonArray>();

        auto iterator = jsonArray.begin();

        while(iterator != jsonArray.end())
        {
            DEBUG("analysing payload item")
            const JsonVariant & _json = *iterator;

            if (_json.containsKey("function"))
            {
                String function = _json["function"];
                DEBUG("contains function %s", function.c_str())

                if (function == "shower-guard")
                {
                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        showerGuardConfig.from_json(config_json);

                        TRACE("function %s: showerGuardConfig.is_valid=%s", function.c_str(), (showerGuardConfig.is_valid() ? "true" : "false"))
                        TRACE(showerGuardConfig.as_string().c_str())

                        if (showerGuardConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            showerGuardConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            epromImage.blocks.insert({(uint8_t) ftShowerGuard, buffer});
                        }
                        else
                        {
                            sprintf(buf, "function %s: config invalid", function.c_str());
                            ERROR(buf)
                            return String(buf);
                        }
                    }
                    else
                    {
                        sprintf(buf, "payload item with function %s should contain config", function.c_str());
                        ERROR(buf)
                        return String(buf);
                    }
                }
                else
                {
                    sprintf(buf, "function %s is unknown", function.c_str());
                    ERROR(buf)
                    return String(buf);
                }
            }
            else
            {
                const char * str = "payload item does NOT contain function";
                ERROR(str)
                return String(str);
            }
            ++iterator;
        }

        EpromImage currentEpromImage;
        currentEpromImage.read();

        std::vector<uint8_t> added, removed, changed;

        if (epromImage.diff(currentEpromImage, & added, & removed, & changed) == true)
        {
            TRACE("New autonom configuration is different from one stored in EEPROM: updating EEPROM image")
            epromImage.write();

            for (auto it=added.begin(); it!=added.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    autonomTaskManager.startShowerGuard(showerGuardConfig);
                }
            }
            for (auto it=removed.begin(); it!=removed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    autonomTaskManager.stopShowerGuard();
                }
            }
            for (auto it=changed.begin(); it!=changed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    autonomTaskManager.reconfigureShowerGuard(showerGuardConfig);
                }
            }
        }
        else
        {
            TRACE("New autonom configuration is the same as the one stored in EEPROM: skip updating EEPROM image")

        }
    }
    else
    {
        const char * str = "payload is NOT array";
        ERROR(str)
        return String(str);
    }

    return String();
}


void cleanupAutonom() 
{
    TRACE("cleanupAutonom")

    // remove all functions from EEPROM 

    EpromImage epromImage, currentEpromImage; 
    currentEpromImage.read();

    std::vector<uint8_t> added, removed, changed;

    if (epromImage.diff(currentEpromImage, & added, & removed, & changed) == true)
    {
        TRACE("New autonom configuration is different from one stored in EEPROM: updating EEPROM image")
        epromImage.write();
    }

    // stop all running tasks
        
    autonomTaskManager.stopAll();
}


void getAutonom(JsonVariant & json)
{
  TRACE("getAutonom")

  if (autonomTaskManager.isShowerGuardActive())
  {
        ShowerGuardStatus status = autonomTaskManager.getShowerGuardStatus();
        json.createNestedObject("shower-guard");
        JsonVariant jsonVariant = json["shower-guard"];

        jsonVariant["temp"] = status.temp;
        jsonVariant["rh"] = status.rh;
        jsonVariant["motion"] = status.motion;
        jsonVariant["light"] = status.light;
        jsonVariant["fan"] = status.fan;
  }
}


void restoreAutonom() 
{
  TRACE("restoreAutonom")

  EpromImage epromImage;
  
  if (epromImage.read() == true)
  {
      for (auto it = epromImage.blocks.begin(); it != epromImage.blocks.end(); ++it)
      {
          const char * function_type_str = function_type_2_str((FunctionType) it->first);
          TRACE("Restoring from EEPROM config for function %s", function_type_str)

          std::istringstream is(it->second);

          switch(it->first)
          {
              case ftShowerGuard:
                
                {ShowerGuardConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startShowerGuard(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                break;

              default:
                TRACE("Unhandled config for function %s", function_type_str)
                break;
          }
      }
  }
  else
  {
      ERROR("Cannot read EEPROM image")
  }
}
