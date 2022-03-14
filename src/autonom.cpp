#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>
#include <sstream>

#include <autonom.h>
#include <showerGuard.h>
#include <keyBox.h>
#include <trace.h>
#include <epromImage.h>


class AutonomTaskManager
{
    public:

        AutonomTaskManager()
        {
            showerGuardActive = false;
            keyBoxActive = false;
        }

        void startShowerGuard(const ShowerGuardConfig &);
        void stopShowerGuard();
        void reconfigureShowerGuard(const ShowerGuardConfig &);

        ShowerGuardStatus getShowerGuardStatus() const;

        bool isShowerGuardActive() const { return showerGuardActive; }

        void startKeybox(const KeyboxConfig &);
        void stopKeybox();
        void reconfigureKeybox(const KeyboxConfig &);

        String keyboxActuate(const String & channel_str);

        KeyboxStatus getKeyboxStatus() const;

        bool isKeyboxActive() const { return keyBoxActive; }

        void stopAll();


    protected:

        bool showerGuardActive;
        bool keyBoxActive;

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
    TRACE("getShowerGuardStatus")
    return get_shower_guard_status();
}


void AutonomTaskManager::startKeybox(const KeyboxConfig & config)
{
    TRACE("Starting autonom task keyBox")
    start_keybox_task(config);
    keyBoxActive = true;
}


void AutonomTaskManager::stopKeybox()
{
    TRACE("stopping autonom task keyBox")
    stop_keybox_task();
    keyBoxActive = false;
}


void AutonomTaskManager::reconfigureKeybox(const KeyboxConfig & config)
{
    TRACE("reconfiguring autonom task keyBox")
    reconfigure_keybox(config);
}

String AutonomTaskManager::keyboxActuate(const String & channel_str)
{
    return keybox_actuate(channel_str);
}

KeyboxStatus AutonomTaskManager::getKeyboxStatus() const
{
    TRACE("getKeyboxStatus")
    return get_keybox_status();
}


void AutonomTaskManager::stopAll()
{
    TRACE("stopping all autonom tasks")

    if (showerGuardActive)
    {
        stopShowerGuard();
    }

    if (keyBoxActive)
    {
        stopKeybox();
    }
}


AutonomTaskManager autonomTaskManager;


const char * function_type_2_str(FunctionType ft)
{
    switch(ft)
    {
        case ftShowerGuard:
            return "shower-guard";
        case ftKeybox:
            return "keybox";
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
    KeyboxConfig keyBoxConfig;

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
                if (function == "keybox")
                {
                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        keyBoxConfig.from_json(config_json);

                        TRACE("function %s: keyBoxConfig.is_valid=%s", function.c_str(), (keyBoxConfig.is_valid() ? "true" : "false"))
                        TRACE(keyBoxConfig.as_string().c_str())

                        if (keyBoxConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            keyBoxConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            epromImage.blocks.insert({(uint8_t) ftKeybox, buffer});
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
                else
                if (*it == ftKeybox)
                {
                    autonomTaskManager.startKeybox(keyBoxConfig);
                }

            }
            for (auto it=removed.begin(); it!=removed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    autonomTaskManager.stopShowerGuard();
                }
                else
                if (*it == ftKeybox)
                {
                    autonomTaskManager.stopKeybox();
                }
            }
            for (auto it=changed.begin(); it!=changed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    autonomTaskManager.reconfigureShowerGuard(showerGuardConfig);
                }
                else
                if (*it == ftKeybox)
                {
                    autonomTaskManager.reconfigureKeybox(keyBoxConfig);
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


String actionAutonomKeyboxActuate(const String & channel_str)
{
    if (autonomTaskManager.isKeyboxActive())
    {
        return autonomTaskManager.keyboxActuate(channel_str);
    }
    else
    {
        return "Keybox not active";
    }
}


void getAutonom(JsonVariant & json)
{
  TRACE("getAutonom")

  if (autonomTaskManager.isShowerGuardActive())
  {
        ShowerGuardStatus status = autonomTaskManager.getShowerGuardStatus();
        status.to_json(json);
  }

  if (autonomTaskManager.isKeyboxActive())
  {
        KeyboxStatus status = autonomTaskManager.getKeyboxStatus();
        status.to_json(json);
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

              case ftKeybox:
                
                {KeyboxConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startKeybox(config);
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
