#include <Arduino.h>
#include <ArduinoJson.h>

#include <rest.h>
#include <trace.h>
#include <pm.h>
#include <wifiHandler.h>
#include <logBuffer.h>

extern WifiHandler wifiHandler;

#define BIG_JSON_BUFFER_SIZE 8192
#define SMALL_JSON_BUFFER_SIZE 256

static char _buffer[BIG_JSON_BUFFER_SIZE]; // for serialization of bigger things, not thread safe!

String restPing(bool include_info) 
{
  TRACE("REST ping")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  jsonDocument["version"] = REST_VERSION;

  jsonDocument.createNestedObject("pm");
  JsonVariant pmJsonVariant = jsonDocument["pm"];
  pingPm(pmJsonVariant);

  if (include_info)
  {
    jsonDocument["wifiinfo"] = wifiHandler.getWifiInfo();
  }

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


String restCleanup() 
{
  TRACE("REST cleanup")

  cleanupPm();

  return String("{}");
}


String restCleanupPm() 
{
  TRACE("REST cleanup PM")

  cleanupPm();

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
  JsonVariant pmJsonVariant = jsonDocument["pm"];
  getPm(pmJsonVariant, resetStamp);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restGetPm(const String & resetStamp) 
{
  TRACE("REST get PM")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  JsonVariant pmJsonVariant = jsonDocument.as<JsonVariant>();
  getPm(pmJsonVariant, resetStamp);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}


String restPopLog() 
{
  TRACE("REST pop log")

  DynamicJsonDocument jsonDocument(BIG_JSON_BUFFER_SIZE);

  jsonDocument.createNestedObject("log");
  JsonVariant jsonVariant = jsonDocument["log"];
  popLog(jsonVariant);

  serializeJson(jsonDocument, _buffer); 
  return String(_buffer);
}
