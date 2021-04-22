#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <onboardLed.h>

#include <rest.h>
#include <trace.h>

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
{ "pm":
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
}


void on_wifiinfo() 
{
  String r = restWifiInfo();  
  webServer.send(200, "application/json", r.c_str());
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
  led_configured = true;
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
  led_configured = true;
}


void on_cleanup() 
{
  String r = restCleanup();  
  webServer.send(200, "application/json", r.c_str());
  led_configured = false;
}


void on_cleanup_pm() 
{
  String r = restCleanupPm();  
  webServer.send(200, "application/json", r.c_str());
  led_configured = false;
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
}


void on_get_pm() 
{
  String resetStamp;

  if (webServer.hasArg("reset_stamp") == true) 
  {
    resetStamp = webServer.arg("reset_stamp");
  }

  String r = restGetPm(resetStamp);  
  webServer.send(200, "application/json", r.c_str());
  led_blink_once = true;
}


void wwwSetupRouting() 
{
  webServer.on("/restart", HTTP_POST, on_restart);
  webServer.on("/ping", HTTP_GET, on_ping);
  webServer.on("/wifiinfo", HTTP_GET, on_wifiinfo);
  webServer.on("/setup", HTTP_POST, on_setup);
  webServer.on("/setup/pm", HTTP_POST, on_setup_pm);
  webServer.on("/cleanup", HTTP_POST, on_cleanup);
  webServer.on("/cleanup/pm", HTTP_POST, on_cleanup_pm);
  webServer.on("/reset", HTTP_POST, on_reset);
  webServer.on("/reset/pm", HTTP_POST, on_reset_pm);
  webServer.on("/get", HTTP_GET, on_get);
  webServer.on("/get/pm", HTTP_GET, on_get_pm);
 }


void wwwBegin() 
{
  webServer.begin();
}


void wwwHandleClient() 
{
    webServer.handleClient();
}