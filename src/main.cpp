#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <map>
#include <time.h>
#include <eeprom.h>

#include <autonom.h>
#include <wifiHandler.h>
#include <gpio.h>
#include <pm.h>
#include <www.h>
#include <onboardLed.h>
#include <trace.h>

WifiHandler wifiHandler;

std::vector<std::pair<String, String>> knownNetworks;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600+3600;

const size_t EEPROM_SIZE = 512;

void initKnownNetworks()
{
  knownNetworks.push_back(std::make_pair(String("IGMALodge12"), String("IGMALodge14")));
  knownNetworks.push_back(std::make_pair(String("IGMALodge3"), String("IGMALodge14")));
  knownNetworks.push_back(std::make_pair(String("IGMALodge4"), String("IGMALodge14")));

  knownNetworks.push_back(std::make_pair(String("NETGEAR67"), String("largeumbrella829")));
  //knownNetworks.push_back(std::make_pair(String("igmagarpgarden1"), String("IGMASoder1")));
  //knownNetworks.push_back(std::make_pair(String("igmagarpgarden2"), String("IGMASoder1")));
  //knownNetworks.push_back(std::make_pair(String("igmagarpgarden3"), String("IGMASoder1")));
  //knownNetworks.push_back(std::make_pair(String("igmagarpgarden4"), String("IGMASoder1")));
  //knownNetworks.push_back(std::make_pair(String("igmagarpgarden5"), String("IGMASoder1")));
  //knownNetworks.push_back(std::make_pair(String("igmagarpgarden6"), String("IGMASoder1")));

  //knownNetworks.push_back(std::make_pair(String("dlink-15A8"), String("uiacw71798")));
  knownNetworks.push_back(std::make_pair(String("dlink-15A8-route"), String("uiacw71798")));
}


void connectWifi()
{
  led_wifi_search = true;
  led_wifi_on = false;

  uint32_t _millis = millis();

  while(wifiHandler.connectStrongest(knownNetworks) == false)
  {
    uint32_t diff_millis = millis() - _millis;
    const uint32_t WIFI_RETRY_MILLIS = 5000;

    if (diff_millis < WIFI_RETRY_MILLIS)
    {
      delay(WIFI_RETRY_MILLIS - diff_millis);
    }

    TRACE("retrying WIFI connection cycle")
    _millis = millis();
  }

  led_wifi_search = false;
  led_wifi_on = true;
}


void print_ntp_time()
{
  struct tm _tm;

  if(!getLocalTime(&_tm))
  {
    ERROR("Failed to obtain time")
  }
  else
  {
    TRACE("NTP time: %s", tm_2_str(_tm).c_str())
  }
}


void setup() 
{ 
  start_onboard_led_task();

  Serial.begin(9600);

  EEPROM.begin(EEPROM_SIZE);

  restoreAutonom();

  initKnownNetworks();
  connectWifi();
  TRACE("Fetching date and time from NTP ...")
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  print_ntp_time();
  wwwSetupRouting();
  wwwBegin();

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

  ArduinoOTA.begin();
}


static int wifi_connect_retry_count = 0;
const int MAX_WIFI_CONNECT_RETRIES = 20;

void loop() 
{
  ArduinoOTA.handle();

  wwwHandleClient();

  if (wifiHandler.isConnected() == false)
  {
    ERROR("Lost WIFI connection, retrying") 
    wifiHandler.disconnect();
    led_wifi_on = false;
    connectWifi();

    wifi_connect_retry_count++;

    if (wifi_connect_retry_count >= MAX_WIFI_CONNECT_RETRIES)
    {
      ERROR("Max retries for WIFI connection reached, restarting....") 
      ESP.restart();
    }
  }
  else
  {
    led_wifi_on = true;
    wifi_connect_retry_count = 0;
  }
}