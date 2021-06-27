#include <Arduino.h>
#include <ArduinoJson.h>

#include <autonom.h>
#include <showerGuard.h>
#include <trace.h>


void setupAutonom(const JsonVariant & json) 
{
    TRACE("setupAutonom")

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

