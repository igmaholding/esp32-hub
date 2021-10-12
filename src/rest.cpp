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
}


String restPing(bool include_info) 
{
  TRACE("REST ping")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  jsonDocument["version"] = REST_VERSION;

  jsonDocument.createNestedObject("pm");
  JsonVariant pm = jsonDocument["pm"];
  pingPm(pm);

  if (include_info)
  {
    jsonDocument["wifiinfo"] = wifiHandler.getWifiInfo();
  }

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

  setupPm(jsonDocument.as<JsonVariant>(), resetStamp);
  
  return String("{}");
}


String restSetupAutonom(const String & body) 
{
  TRACE("REST setup AUTONOM")
  DEBUG(body.c_str())

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


String restGet(const String & resetStamp) 
{
  TRACE("REST get")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  jsonDocument.createNestedObject("pm");
  JsonVariant pm = jsonDocument["pm"];
  getPm(pm, resetStamp);
  jsonDocument.createNestedObject("autonom");
  JsonVariant autonom = jsonDocument["autonom"];
  getAutonom(autonom);
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
  JsonVariant system = jsonDocument["system"];
  getSystem(system);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restGetAutonom() 
{
  TRACE("REST get AUTONOM")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  JsonVariant autonom = jsonDocument.as<JsonVariant>();
  getAutonom(autonom);
  JsonVariant system = jsonDocument["system"];
  getSystem(system);

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
  JsonVariant system = jsonDocument["system"];
  getSystem(system);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}
