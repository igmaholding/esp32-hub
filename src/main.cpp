#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <map>
#include <time.h>
#include <eeprom.h>
#include <esp_log.h>

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
    start_led_task();
    onboard_led_just_started = true;

    Serial.begin(9600);
    //Serial.write("DIRECT: SERIAL BEGIN");

    EEPROM.begin(EEPROM_SIZE);

    //delay(2000);
    restoreAutonom();

    initKnownNetworks(knownNetworks);
    //delay(3000);
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
    }
}

