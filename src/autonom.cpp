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


void setupAutonom(const JsonVariant & json) 
{
    TRACE("setupAutonom")

    EpromImage epromImage;

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

                        ShowerGuardConfig config;
                        config.from_json(config_json);

                        TRACE("function %s: config.is_valid=%s", function.c_str(), (config.is_valid() ? "true" : "false"))
                        TRACE(config.as_string().c_str())

                        if (config.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            config.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            epromImage.blocks.insert({(uint8_t) ftShowerGuard, buffer});
                        }
                    }
                    else
                    {
                        ERROR("payload item with function %s should contain config", function.c_str())
                    }
                }
                else
                {
                    ERROR("function %s is unknown", function.c_str())
                }
            }
            else
            {
                ERROR("payload item does NOT contain function")
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

            // TODO: reconfigure running threads
        }
        else
        {
            TRACE("New autonom configuration is the same as the one stored in EEPROM: skip updating EEPROM image")

        }
    }
    else
    {
        ERROR("payload is NOT array")
    }
}


void cleanupAutonom() 
{
  TRACE("cleanupAutonom")
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
                    TRACE("Config read ok: %s", config.as_string().c_str())
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
