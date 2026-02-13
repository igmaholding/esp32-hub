#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <map>
#include <time.h>
#include <eeprom.h>
#include <esp_log.h>
#include <esp_phy_init.h>
#include <esp_wifi.h>

#ifdef INCLUDE_ETHHUB
#include <EthernetESP32.h>
#endif

#include <autonom.h>
#include <wifiHandler.h>
#include <wifiNetworks.h>
#include <gpio.h>
#include <pm.h>
#include <www.h>
#include <onboardLed.h>
#include <trace.h>

// note: something strange in handling header files that are included in the Esp32Utils library; the eeprom.h included in 
// epromimage.cpp would not be found unless it is included and used (?) here, in the main project

WifiHandler wifiHandler;

#ifdef INCLUDE_ETHHUB
EMACDriver driver(ETH_PHY_LAN8720, 23, 18, 16);
#endif

const int MAX_WIFI_CONNECT_RETRIES = 20;

std::vector<std::pair<String, String>> knownNetworks;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600+3600;

const size_t EEPROM_SIZE = 4096;

void connectWifi()
{
    int wifi_connect_retry_count = 0;
    onboard_led_wifi_search = true;
    onboard_led_wifi_on = false;

    uint32_t _millis = millis();

    while (wifiHandler.connectStrongest(knownNetworks) == false)
    {
        uint32_t diff_millis = millis() - _millis;
        const uint32_t WIFI_RETRY_MILLIS = 5000;

        if (diff_millis < WIFI_RETRY_MILLIS)
        {
            delay(WIFI_RETRY_MILLIS - diff_millis);
        }

        TRACE("retrying WIFI connection cycle (%d/%d)", wifi_connect_retry_count, MAX_WIFI_CONNECT_RETRIES)
        _millis = millis();

        wifi_connect_retry_count++;

        if (wifi_connect_retry_count >= MAX_WIFI_CONNECT_RETRIES)
        {
            wifi_connect_retry_count = 0;

            #ifndef INCLUDE_PROPORTIONAL

            // we do not restart the target with proportional since it will start calibration / actuation
            // procedures which will eventually destroy the actuator if repeated endlessly 

            ERROR("Max retries for WIFI connection reached, restarting....")
            ESP.restart();
    
            #endif

        }
    }

    onboard_led_wifi_search = false;
    onboard_led_wifi_on = true;
}

void print_ntp_time()
{
    struct tm _tm;

    if (!getLocalTime(&_tm))
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
    #if CONFIG_IDF_TARGET_ESP32S3

    // onboard_led_is_neopixel = true;

    #endif
    
    start_led_task();
    onboard_led_just_started = true;

    Serial.begin(9600);
    //Serial.write("DIRECT: SERIAL BEGIN");

    #ifdef INCLUDE_ETHHUB

    Ethernet.init(driver);
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable not connected / Câble Ethernet non connecté.");
    }
    Serial.println("Ethernet init by DHCP / Initialisation Ethernet par DHCP:");
    if (Ethernet.begin()) {
      Serial.println("IP address / Adresse IP Ethernet assignée par DHCP : " + Ethernet.localIP().toString());
    } else {
      Serial.println("Failed to configure Ethernet using DHCP");
      delay(1);
    }
  
    #endif

    EEPROM.begin(EEPROM_SIZE);

    //delay(2000);
    restoreAutonom();
    initKnownNetworks(knownNetworks);
    //delay(3000);
    //esp_phy_erase_cal_data_in_nvs();
    esp_wifi_set_max_tx_power(40);

    connectWifi();
    TRACE("Fetching date and time from NTP ...")
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    print_ntp_time();
    wwwSetupRouting();
    //TRACE("wwwSetupRouting OK")
    //delay(3000);
    wwwBegin();
    //TRACE("wwwBegin OK")
    //delay(3000);

    // the below call somehow interferes with tag reading activities in the PN532 library and
    // causes core panic crash
    //TRACE("Total heap: %d", (int) ESP.getHeapSize())

    TRACE("Free heap: %d", (int) ESP.getFreeHeap())
    TRACE("Total PSRAM: %d", (int) ESP.getPsramSize())
    TRACE("Free PSRAM: %d", (int) ESP.getFreePsram())
    
    TaskHandle_t myTaskHandle = xTaskGetCurrentTaskHandle();
    auto priority = uxTaskPriorityGet( myTaskHandle );
    TRACE("main task priority is %d", int(priority))
    DEBUG("onboard led is defined at GPIO %d", (int) DEFAULT_ONBOARD_LED_GPIO)

    ArduinoOTA
        .onStart([]()
                 {
                     String type;
                     if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                     else // U_SPIFFS
                         type = "filesystem";

                     // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                     Serial.println("Start updating " + type);
                 })
        .onEnd([]()
               {
                   Serial.println("\nEnd");
                   //ESP.restart(); // should be done automatically?
               })
        .onProgress([](unsigned int progress, unsigned int total)
                    { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
        .onError([](ota_error_t error)
                 {
                     Serial.printf("Error[%u]: ", error);
                     if (error == OTA_AUTH_ERROR)
                         Serial.println("Auth Failed");
                     else if (error == OTA_BEGIN_ERROR)
                         Serial.println("Begin Failed");
                     else if (error == OTA_CONNECT_ERROR)
                         Serial.println("Connect Failed");
                     else if (error == OTA_RECEIVE_ERROR)
                         Serial.println("Receive Failed");
                     else if (error == OTA_END_ERROR)
                         Serial.println("End Failed");
                 });

    ArduinoOTA.begin();
    TRACE("WIFI MAC: %s", WiFi.macAddress().c_str())
}


static unsigned long _last_mac_ip_report_millis = 0;
static const unsigned long MAC_IP_REPORT_MILLIS_INTERVAL = 30000;

void loop()
{
    ArduinoOTA.handle();

    wwwHandleClient();

    if (wifiHandler.isConnected() == false)
    {
        ERROR("Lost WIFI connection, retrying")
        wifiHandler.disconnect();
        onboard_led_wifi_on = false;
        connectWifi();
    }
    else
    {
        onboard_led_wifi_on = true;

        unsigned long _millis = millis();

        if (_last_mac_ip_report_millis == 0 || _millis < _last_mac_ip_report_millis || (_millis-_last_mac_ip_report_millis) >= MAC_IP_REPORT_MILLIS_INTERVAL)
        {
            TRACE("WIFI SSID: %s MAC: %s IP: %s", WiFi.SSID().c_str(), WiFi.macAddress().c_str(), WiFi.localIP().toString().c_str());
            _last_mac_ip_report_millis = _millis;
        }
    }
}

