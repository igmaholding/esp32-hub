#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <onboardLed.h>

#include <rest.h>
#include <trace.h>
#include <www.h>

WebServer webServer(80);

/*

REST POST restart
URL: <base>/restart
BODY: none
RESPONSE: 
{
}

REST GET ping
URL: <base>/ping<?info=true>
BODY: none
RESPONSE: 
{
    "version": "1.0",
    
    "pm":
      {   
        "reset_stamp": "GHJKLM",
        "is_configured": true
      },
      
  # if ?info parameter is given include:
  # wifi info

}

REST GET wifiinfo
URL: <base>/wifiinfo
BODY: none
RESPONSE: 
{
    "wifiinfo": <available and selected network, multiline string>
}

REST POST setup
URL: <base>/setup?reset_stamp=ABCDEF
BODY: 
{ "pm": <see setup pm>,
  "autonom": <see setup autonom>
}

RESPONSE: 
{
}

REST POST setup pm
URL: <base>/setup/pm?reset_stamp=ABCDEF
BODY: 
{ 
  "channels":
  [
      {
          "gpio":23, "inverted":true, "debounce":250
      },
      {
          "gpio":25, "inverted":true, "debounce":50
      }
  ]
}

RESPONSE: 
{
}

REST POST setup autonom
URL: <base>/setup/autonom
BODY: 
[{
    "function":"shower-guard", 
    "config":{
    "motion":{"channel":{"gpio":23, "inverted":true, "debounce":250}, "linger":300},
    "rh":{"vad":{"channel":{"gpio":34, "atten":3}},"vdd":{"channel":{"gpio":35, "atten":3}}}, 
    "temp":{"channel":{"gpio":4}, "addr":"28-000009079138"}, 
    "light":{"channel":{"gpio":13, "inverted":true, "coilon_active":false}, "mode":"auto"}, 
    "fan":{"channel":{"gpio":14, "inverted":true, "coilon_active":false}, "rh_off":50, "rh_on":55, "mode":"auto"} 
    }
}]

RESPONSE: 
{
}

REST POST cleanup
URL: <base>/cleanup
BODY: none 
RESPONSE: 
{
}

REST POST cleanup pm
URL: <base>/cleanup/pm
BODY: none 
RESPONSE: 
{
}

REST POST cleanup autonom
URL: <base>/cleanup/autonom
BODY: none 
RESPONSE: 
{
}

REST POST reset 
URL: <base>/reset?reset_stamp=GHJKLM
BODY: none 
RESPONSE: 
{
}

REST POST reset pm
URL: <base>/reset/pm?reset_stamp=GHJKLM
BODY: none 
RESPONSE: 
{
}

REST GET get
URL: <base>/get?reset_stamp=GHJKLM
BODY: none
RESPONSE: 
{
    "pm": {
        "reset_stamp": "GHJKLM",
        "is_configured": true,
        "gpio": [
            23,
            25
        ],
        "counter": [
            0,
            0
        ]
    }
}

REST GET get pm
URL: <base>/get/pm?reset_stamp=GHJKLM
BODY: none
RESPONSE: 
{
    "reset_stamp": "GHJKLM",
    "is_configured": true,
    "gpio": [
        23,
        25
    ],
    "counter": [
        0,
        0
    ]
}

*/

void on_restart()
{
    TRACE("*** RESTART REQUESTED VIA REST ***")

    webServer.send(200, "application/json", "{}");
    led_blink_once = true;

    delay(5000);
    ESP.restart();
}

void on_ping()
{
    bool include_info = false;

    if (webServer.hasArg("info") == true)
    {
        include_info = true;
    }

    String r = restPing(include_info);
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_wifiinfo()
{
    String r = restWifiInfo();
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_setup()
{
    String body;
    String resetStamp;

    if (webServer.hasArg("reset_stamp") == true)
    {
        resetStamp = webServer.arg("reset_stamp");
    }

    if (webServer.hasArg("plain") == false)
    {
        ERROR("Setup POST request without a payload")
    }
    else
    {
        body = webServer.arg("plain");
    }

    String r = restSetup(body, resetStamp);

    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_setup_pm()
{
    String body;
    String resetStamp;

    if (webServer.hasArg("reset_stamp") == true)
    {
        resetStamp = webServer.arg("reset_stamp");
    }

    if (webServer.hasArg("plain") == false)
    {
        ERROR("Setup PM POST request without a payload")
    }
    else
    {
        body = webServer.arg("plain");
    }

    String r = restSetupPm(body, resetStamp);

    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_setup_autonom()
{
    String body;

    if (webServer.hasArg("plain") == false)
    {
        ERROR("Setup AUTONOM POST request without a payload")
    }
    else
    {
        body = webServer.arg("plain");
    }

    String r = restSetupAutonom(body);

    if (r.length() == 0)
    {
        webServer.send(200, "application/json", "{}");
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    led_blink_once = true;
    led_paired = true;
}

void on_cleanup()
{
    String r = restCleanup();
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_cleanup_pm()
{
    String r = restCleanupPm();
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_cleanup_autonom()
{
    String r = restCleanupAutonom();
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_reset()
{
    String resetStamp;

    if (webServer.hasArg("reset_stamp") == true)
    {
        resetStamp = webServer.arg("reset_stamp");
    }

    String r = restReset(resetStamp);
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_reset_pm()
{
    String resetStamp;

    if (webServer.hasArg("reset_stamp") == true)
    {
        resetStamp = webServer.arg("reset_stamp");
    }

    String r = restResetPm(resetStamp);
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_get()
{
    String resetStamp;

    if (webServer.hasArg("reset_stamp") == true)
    {
        resetStamp = webServer.arg("reset_stamp");
    }

    String r = restGet(resetStamp);
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_get_pm()
{
    String resetStamp;
    DEBUG("on_get_pm")

    if (webServer.hasArg("reset_stamp") == true)
    {
        resetStamp = webServer.arg("reset_stamp");
    }

    String r = restGetPm(resetStamp);
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_get_autonom()
{
    DEBUG("on_get_autonom")
    String r = restGetAutonom();
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void on_pop_log()
{
    String r = restPopLog();
    webServer.send(200, "application/json", r.c_str());
    led_blink_once = true;
    led_paired = true;
}

void wwwSetupRouting()
{
    webServer.on("/" HARVESTER_API_KEY "/restart", HTTP_POST, on_restart);
    webServer.on("/" HARVESTER_API_KEY "/ping", HTTP_GET, on_ping);
    webServer.on("/" HARVESTER_API_KEY "/wifiinfo", HTTP_GET, on_wifiinfo);
    webServer.on("/" HARVESTER_API_KEY "/setup", HTTP_POST, on_setup);
    webServer.on("/" HARVESTER_API_KEY "/setup/pm", HTTP_POST, on_setup_pm);
    webServer.on("/" HARVESTER_API_KEY "/setup/autonom", HTTP_POST, on_setup_autonom);
    webServer.on("/" HARVESTER_API_KEY "/cleanup", HTTP_POST, on_cleanup);
    webServer.on("/" HARVESTER_API_KEY "/cleanup/pm", HTTP_POST, on_cleanup_pm);
    webServer.on("/" HARVESTER_API_KEY "/cleanup/autonom", HTTP_POST, on_cleanup_autonom);
    webServer.on("/" HARVESTER_API_KEY "/reset", HTTP_POST, on_reset);
    webServer.on("/" HARVESTER_API_KEY "/reset/pm", HTTP_POST, on_reset_pm);
    webServer.on("/" HARVESTER_API_KEY "/get", HTTP_GET, on_get);
    webServer.on("/" HARVESTER_API_KEY "/get/pm", HTTP_GET, on_get_pm);
    webServer.on("/" HARVESTER_API_KEY "/get/autonom", HTTP_GET, on_get_autonom);
    webServer.on("/" HARVESTER_API_KEY "/poplog", HTTP_GET, on_pop_log);
}

void wwwBegin()
{
    webServer.begin();
}

void wwwHandleClient()
{
    webServer.handleClient();
}