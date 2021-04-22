#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <map>

#include <wifiHandler.h>
#include <gpio.h>
#include <pm.h>
#include <www.h>
#include <onboardLed.h>
#include <trace.h>

WifiHandler wifiHandler;

std::vector<std::pair<String, String>> knownNetworks;


void initKnownNetworks()
{
  knownNetworks.push_back(std::make_pair(String("IGMALodge12"), String("IGMALodge14")));
  //knownNetworks.push_back(std::make_pair(String("IGMALodge34"), String("IGMALodge14")));

  knownNetworks.push_back(std::make_pair(String("NETGEAR67"), String("largeumbrella829")));
  knownNetworks.push_back(std::make_pair(String("igmagarpgarden1"), String("IGMASoder1")));
  knownNetworks.push_back(std::make_pair(String("igmagarpgarden2"), String("IGMASoder1")));
  knownNetworks.push_back(std::make_pair(String("igmagarpgarden3"), String("IGMASoder1")));
  knownNetworks.push_back(std::make_pair(String("igmagarpgarden4"), String("IGMASoder1")));
  knownNetworks.push_back(std::make_pair(String("igmagarpgarden5"), String("IGMASoder1")));
  knownNetworks.push_back(std::make_pair(String("igmagarpgarden6"), String("IGMASoder1")));

  //knownNetworks.push_back(std::make_pair(String("dlink-15A8"), String("uiacw71798")));
  knownNetworks.push_back(std::make_pair(String("dlink-15A8-route"), String("uiacw71798")));
}


void connectWifi()
{
  led_wifi_search = true;
  led_wifi_on = false;

  while(wifiHandler.connectStrongest(knownNetworks) == false)
  {
    TRACE("retrying WIFI connection cycle")
  }

  led_wifi_search = false;
  led_wifi_on = true;
}

void setup() 
{ 
  start_onboard_led_task();

  Serial.begin(9600);
  initKnownNetworks();
  connectWifi();
  wwwSetupRouting();
  wwwBegin();
}


static int wifi_connect_retry_count = 0;
const int MAX_WIFI_CONNECT_RETRIES = 20;

void loop() 
{
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