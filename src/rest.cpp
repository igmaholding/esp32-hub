#include <Arduino.h>
#include <ArduinoJson.h>

#include <rest.h>
#include <trace.h>
#include <pm.h>
#include <autonom.h>
#include <wifiHandler.h>
#include <logBuffer.h>

extern WifiHandler wifiHandler;

#define BIG_JSON_BUFFER_SIZE 8192
#define SMALL_JSON_BUFFER_SIZE 256

static char _buffer[BIG_JSON_BUFFER_SIZE]; // for serialization of bigger things, not thread safe!


void getSystem(JsonVariant & json)
{
    TRACE("getSystem")

    unsigned long total_seconds = millis() / 1000;
    int seconds = total_seconds % 60;
    int minutes = (total_seconds / 60) % 60;
    int hours = (total_seconds / (60*60)) % 24;
    int days = total_seconds / (60*60*24);

    char buf[64];
    sprintf(buf, "%02dd %02dh %02dm %02ds", days, hours, minutes, seconds);
    json["uptime"] = String(buf);

    TRACE("System uptime %s", buf)
}


String restPing(bool include_info) 
{
  TRACE("REST ping")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  jsonDocument["version"] = REST_VERSION;

  jsonDocument.createNestedArray("sw_caps");
  JsonArray sw_caps = jsonDocument["sw_caps"]; 

  #ifdef INCLUDE_PM 

  sw_caps.add("pm");

  jsonDocument.createNestedObject("pm");
  JsonVariant pm = jsonDocument["pm"];
  pingPm(pm);

  #endif

  #ifdef INCLUDE_SHOWERGUARD

  sw_caps.add("showerGuard");

  #endif

  #ifdef INCLUDE_KEYBOX

  sw_caps.add("keybox");

  #endif

  #ifdef INCLUDE_AUDIO

  sw_caps.add("audio");

  #endif

  if (include_info)
  {
    jsonDocument["wifiinfo"] = wifiHandler.getWifiInfo();
  }

  jsonDocument.createNestedObject("system");
  JsonVariant system = jsonDocument["system"];
  getSystem(system);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restWifiInfo() 
{
  TRACE("REST wifiinfo")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  jsonDocument["wifiinfo"] = wifiHandler.getWifiInfo();

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restSetup(const String & body, const String & resetStamp) 
{
  TRACE("REST setup")
  DEBUG("resetStamp %s", resetStamp.c_str())
  DEBUG(body.c_str())

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  deserializeJson(jsonDocument, body);

  if (jsonDocument.containsKey("pm"))
  {
    const JsonVariant & pmJson = jsonDocument["pm"];
    setupPm(pmJson, resetStamp);
  }

  if (jsonDocument.containsKey("autonom"))
  {
    const JsonVariant & pmJson = jsonDocument["autonom"];
    setupAutonom(pmJson);
  }

  return String("{}");
}


String restSetupPm(const String & body, const String & resetStamp) 
{
  TRACE("REST setup PM")
  DEBUG("resetStamp %s", resetStamp.c_str())
  DEBUG(body.c_str())

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  deserializeJson(jsonDocument, body);

  String none_or_error = setupPm(jsonDocument.as<JsonVariant>(), resetStamp);
  
  return none_or_error;
}


String restSetupAutonom(const String & body) 
{
  TRACE("REST setup AUTONOM")
  //DEBUG(body.c_str())

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  deserializeJson(jsonDocument, body);

  String none_or_error = setupAutonom(jsonDocument.as<JsonVariant>());
  
  return none_or_error;
}


String restCleanup() 
{
  TRACE("REST cleanup")

  cleanupPm();
  cleanupAutonom();

  return String("{}");
}


String restCleanupPm() 
{
  TRACE("REST cleanup PM")

  cleanupPm();

  return String("{}");
}


String restCleanupAutonom() 
{
  TRACE("REST cleanup autonom")

  cleanupAutonom();

  return String("{}");
}


String restReset(const String & resetStamp) 
{
  TRACE("REST reset")
  DEBUG(resetStamp.c_str())

  resetPm(resetStamp);

  return String("{}");
}


String restResetPm(const String & resetStamp) 
{
  TRACE("REST reset PM")
  DEBUG("resetStamp %s", resetStamp.c_str())

  resetPm(resetStamp);

  return String("{}");
}

String restActionAutonomKeyboxActuate(const String & channel_str)
{
  TRACE("REST action autonom keybox actuate")
  DEBUG("channel %s", channel_str.c_str())

  return actionAutonomKeyboxActuate(channel_str);
}

String restActionAutonomRfidLockProgram(const String & code_str, uint16_t timeout)
{
  TRACE("REST action autonom rfid-lock program")
  
  DEBUG("code %s, timeout %d", code_str.c_str(), (int) timeout)

  return actionAutonomRfidLockProgram(code_str, timeout);
}

String restActionAutonomRfidLockAdd(const String & name_str, const String & code_str, const std::vector<String> & locks, 
                                    const String & type_str)
{
  TRACE("REST action autonom rfid-lock add")

  String locks_str("[");
  for (auto it=locks.begin(); it!=locks.end(); ++it)
  {
    locks_str += *it;
    
    if (it+1 != locks.end())
    {
      locks_str += ",";
    }
  }
  locks_str += "]";
  
  DEBUG("name %s, code %s, lock(s) %s, type %s", name_str.c_str(), code_str.c_str(), locks_str.c_str(), type_str.c_str())

  return actionAutonomRfidLockAdd(name_str, code_str, locks, type_str);
}

String restActionAutonomProportionalCalibrate(const String & channel_str)
{
  TRACE("REST action autonom proportional calibrate")
  DEBUG("channel %s", channel_str.c_str())

  return actionAutonomProportionalCalibrate(channel_str);
}

String restActionAutonomProportionalActuate(const String & channel_str, const String & value_str,
                                            const String & ref_str)
{
  TRACE("REST action autonom proportional actuate")
  DEBUG("channel %s, value %s ref %s", channel_str.c_str(), value_str.c_str(), ref_str.c_str())

  return actionAutonomProportionalActuate(channel_str, value_str, ref_str);
}

String restActionAutonomZero2tenCalibrateInput(const String & channel_str, const String & value_str)
{
  TRACE("REST action autonom zero2ten calibrate input")
  DEBUG("channel %s", channel_str.c_str())

  return actionAutonomZero2tenCalibrateInput(channel_str, value_str);
}

String restActionAutonomZero2tenInput(const String & channel_str, String & value_str)
{
  TRACE("REST action autonom zero2ten input")
  DEBUG("channel %s", channel_str.c_str())

  return actionAutonomZero2tenInput(channel_str, value_str);
}

String restActionAutonomZero2tenCalibrateOutput(const String & channel_str, const String & value_str)
{
  TRACE("REST action autonom zero2ten calibrate output")
  DEBUG("channel %s", channel_str.c_str())

  return actionAutonomZero2tenCalibrateOutput(channel_str, value_str);
}

String restActionAutonomZero2tenOutput(const String & channel_str, const String & value_str)
{
  TRACE("REST action autonom zero2ten output")
  DEBUG("channel %s", channel_str.c_str())

  return actionAutonomZero2tenOutput(channel_str, value_str);
}

String restGet(const String & resetStamp) 
{
  TRACE("REST get")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  #ifdef INCLUDE_PM 

  jsonDocument.createNestedObject("pm");
  JsonVariant pm = jsonDocument["pm"];
  getPm(pm, resetStamp);

  #endif

  jsonDocument.createNestedObject("autonom");
  JsonVariant autonom = jsonDocument["autonom"];
  getAutonom(autonom);
  jsonDocument.createNestedObject("system");
  JsonVariant system = jsonDocument["system"];
  getSystem(system);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restGetPm(const String & resetStamp) 
{
  TRACE("REST get PM")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  JsonVariant pm = jsonDocument.as<JsonVariant>();
  getPm(pm, resetStamp);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restGetAutonom() 
{
  TRACE("REST get AUTONOM")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  JsonVariant autonom = jsonDocument.as<JsonVariant>();
  getAutonom(autonom);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restPopLog() 
{
  TRACE("REST pop log")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  jsonDocument.createNestedObject("log");
  JsonVariant log = jsonDocument["log"];
  popLog(log);
  jsonDocument.createNestedObject("system");
  JsonVariant system = jsonDocument["system"];
  getSystem(system);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}
