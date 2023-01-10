#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>
#include <sstream>

#include <autonom.h>
#include <showerGuard.h>

#ifdef INCLUDE_KEYBOX
#include <keyBox.h>
#endif

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

        #ifdef INCLUDE_SHOWERGUARD

        void startShowerGuard(const ShowerGuardConfig &);
        void stopShowerGuard();
        void reconfigureShowerGuard(const ShowerGuardConfig &);

        ShowerGuardStatus getShowerGuardStatus() const;

        bool isShowerGuardActive() const { return showerGuardActive; }

        #endif

        #ifdef INCLUDE_KEYBOX
        
        void startKeybox(const KeyboxConfig &);
        void stopKeybox();
        void reconfigureKeybox(const KeyboxConfig &);

        String keyboxActuate(const String & channel_str);

        KeyboxStatus getKeyboxStatus() const;

        bool isKeyboxActive() const { return keyBoxActive; }
        
        #endif

        void stopAll();


    protected:

        bool showerGuardActive;
        bool keyBoxActive;

};

#ifdef INCLUDE_SHOWERGUARD

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

#endif // INCLUDE_SHOWERGUARD

#ifdef INCLUDE_KEYBOX

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

#endif // INCLUDE_KEYBOX

void AutonomTaskManager::stopAll()
{
    TRACE("stopping all autonom tasks")
    
    #ifdef INCLUDE_SHOWERGUARD

    if (showerGuardActive)
    {
        stopShowerGuard();
    }

    #endif

    # if INCLUDE_KEYBOX

    if (keyBoxActive)
    {
        stopKeybox();
    }

    #endif
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

    #ifdef INCLUDE_SHOWERGUARD
    ShowerGuardConfig showerGuardConfig;
    #endif

    #ifdef INCLUDE_KEYBOX
    KeyboxConfig keyBoxConfig;
    #endif

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
                    #ifdef INCLUDE_SHOWERGUARD

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

                    #else

                    sprintf(buf, "attempt to configure function %s which is not built in current module", function.c_str());
                    ERROR(buf)
                    return String(buf);

                    #endif // INCLUDE_SHOWERGUARD
                }
                else
                if (function == "keybox")
                {
                    #ifdef INCLUDE_KEYBOX

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

                    #else

                    sprintf(buf, "attempt to configure function %s which is not built in current module", function.c_str());
                    ERROR(buf)
                    return String(buf);

                    #endif // INCLUDE_KEYBOX
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
                    #ifdef INCLUDE_SHOWERGUARD
                    autonomTaskManager.startShowerGuard(showerGuardConfig);
                    #endif
                }
                else
                if (*it == ftKeybox)
                {
                    #ifdef INCLUDE_KEYBOX
                    autonomTaskManager.startKeybox(keyBoxConfig);
                    #endif
                }

            }
            for (auto it=removed.begin(); it!=removed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    #ifdef INCLUDE_SHOWERGUARD
                    autonomTaskManager.stopShowerGuard();
                    #endif
                }
                else
                if (*it == ftKeybox)
                {
                    #ifdef INCLUDE_KEYBOX
                    autonomTaskManager.stopKeybox();
                    #endif
                }
            }
            for (auto it=changed.begin(); it!=changed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    #ifdef INCLUDE_SHOWERGUARD
                    autonomTaskManager.reconfigureShowerGuard(showerGuardConfig);
                    #endif
                }
                else
                if (*it == ftKeybox)
                {
                    #ifdef INCLUDE_KEYBOX
                    autonomTaskManager.reconfigureKeybox(keyBoxConfig);
                    #endif
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
    #ifdef INCLUDE_KEYBOX
    if (autonomTaskManager.isKeyboxActive())
    {
        return autonomTaskManager.keyboxActuate(channel_str);
    }
    else
    {
        return "Keybox not active";
    }
    
    #else

    return "Keybox is not built in currrent module";

    #endif // INCLUDE_KEYBOX
}

void getAutonom(JsonVariant & json)
{
  TRACE("getAutonom")

  #ifdef INCLUDE_SHOWERGUARD
  if (autonomTaskManager.isShowerGuardActive())
  {
        ShowerGuardStatus status = autonomTaskManager.getShowerGuardStatus();
        status.to_json(json);
  }
  #endif

  #ifdef INCLUDE_KEYBOX
  if (autonomTaskManager.isKeyboxActive())
  {
        KeyboxStatus status = autonomTaskManager.getKeyboxStatus();
        status.to_json(json);
  }
  
  #else

  #endif // INCLUDE_KEYBOX
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
                
                #ifdef INCLUDE_SHOWERGUARD
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
                #endif
                break;

              case ftKeybox:
                
                #ifdef INCLUDE_KEYBOX
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
                #endif // INCLUDE_KEYBOX
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
