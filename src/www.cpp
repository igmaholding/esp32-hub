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
    
    "sw_caps": ["pm", "keybox", "showerGuard"],
      
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
          "gpio":23, "inverted":true, "debounce":50
      },
      {
          "gpio":25, "inverted":true, "debounce":50
      },
      {
          "gpio":26, "inverted":true, "debounce":50
      },
      {
          "gpio":27, "inverted":true, "debounce":50
      },
      {
          "gpio":32, "inverted":true, "debounce":50
      },
      {
          "gpio":33, "inverted":true, "debounce":50
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
    "function":"keybox", 
    "config":{
    "buzzer":{"channel":{"gpio":13, "inverted":false}},
    "keypad":{"c":[{"channel":{"gpio":25, "inverted":false}}, 
                   {"channel":{"gpio":26, "inverted":false}}, 
                   {"channel":{"gpio":27, "inverted":false}}, 
                   {"channel":{"gpio":32, "inverted":false}}],
              "l":[{"channel":{"gpio":34, "inverted":true}}, 
                   {"channel":{"gpio":35, "inverted":true}}, 
                   {"channel":{"gpio":14, "inverted":true}}, 
                   {"channel":{"gpio":15, "inverted":true}}],
              "debounce":250
    },
    "actuator":{"addr":[{"channel":{"gpio":17, "inverted":false}}, 
                   {"channel":{"gpio":18, "inverted":false}}, 
                   {"channel":{"gpio":19, "inverted":false}}, 
                   {"channel":{"gpio":21, "inverted":false}}],
                "latch": {"channel":{"gpio":23, "inverted":true}},  
                "power": {"channel":{"gpio":4, "inverted":false}},  
                "status": {"channel":{"gpio":22, "inverted":false}}  
    }, 
    "codes":{"code":[{"value":"512136"}, 
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""},
                     {"value":""}
                    ]
    }
    }
}]
RESPONSE: 
{
}


[{
    "function":"shower-guard", 
    "config":{
                "motion":{"channel":{"gpio":23, "inverted":0, "debounce":250}},
                "rh":{"vad":{"channel":{"gpio":34, "atten":3}},"vdd":{"channel":{"gpio":35, "atten":3}}, "corr":0.0}, 
                "temp":{"channel":{"gpio":4}, "addr":"28-01201d2496c8", "corr":0}, 
                "lumi":{"ldr":{"channel":{"gpio":32, "atten":3}}, "corr":0.0, "threshold":12.3}, 
                "light":{"channel":{"gpio":12, "inverted":0, "coilon_active":1}, "mode":"auto", "linger":60}, 
                "fan":{"channel":{"gpio":13, "inverted":0, "coilon_active":1}, "rh_off":45, "rh_on":57, "mode":"auto", "linger":600} 

    }
}]

[{
    "function":"audio", 
    "config":{
        "motion":{"channel":{"gpio":23, "inverted":0, "debounce":250}},
        "onoff":2,
        "delay":300,
        "i2s":{"dout":{"channel":{"gpio":25}}, "bclk":{"channel":{"gpio":27}},"lrc":{"channel":{"gpio":26}}},
        "service":{"url":[{"value":"http://fm03-ice.stream.khz.se/fm03_aac"}, 
                          {"value":"http://mp3.ffh.de/radioffh/hqlivestream.aac"},
                          {"value":"http://vis.media-ice.musicradio.com/Heart00s"},
                          {"value":"http://fm03-ice.stream.khz.se/fm04_aac"},
                          {"value":"http://fm03-ice.stream.khz.se/fm01_aac"}
                         ],
                   "url_select": 1},
        "sound":{"volume":10, "volume_low":6, 
                 "gain_low_pass":0, "gain_band_pass":-7, "gain_high_pass":3,
                 "schedule":[0,0,0,0,0,0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2]}                
    }
}]

[{
    "function":"rfid-lock", 
    "config":{
                "rfid":{"protocol":"SPI", "hw":"RC522", "resetPowerDownPin":13, "chipSelectPin":5, "sclPin":22, "sdaPin":21, "i2cAddress":40},
                "lock":{"channels":{"main":{"gpio":32, "inverted":0, "coilon_active":1}}, "linger":3},
                "buzzer":{"channel":{"gpio":27, "inverted":false}},
                "green_led":{"gpio":14, "inverted":false},
                "red_led":{"gpio":12, "inverted":false}
    }
}]


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



REST POST action
URL: <base>/action
BODY: none
RESPONSE: 
{
}


REST POST action
URL: <base>/action/autonom
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/keybox/actuate?channel=XX
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/program?code=47HuF9CemtholcVCz9A6&timeout=10
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/add?name=igma&code=47HuF9CemtholcVCz9A6&locks=*
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/remove?name=igma
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/remove_all
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/unlock?lock_channels=*
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

REST GET get autonom
URL: <base>/get/autonom
BODY: none
RESPONSE: 
{
 "shower-guard":{"temp":22.1,"rh":19.8,"motion":false,"light":false,"fan":false,"light_decision":"","fan_decision":"rh-low 44.7/45.0 at 2022-12-16 11:04:53"},   
 {'keybox': {'status': '0:0 1:1 2:1 3:1 4:0 5:1 6:1 7:1 8:0 9:0 10:1 11:0 12:1 13:1 14:0 (1==unlocked)'}, 

 {
    "audio": {
        "motion": "true",
        "is_streaming": "true",
        "volume": 10,
        "url_index": 1,
        "bitrate": 128000,
        "title": "Du horst HIT RADIO FFH"
    }
 }


 'system': {'uptime': '46d 22h 02m 10s'}

}

*/

void on_restart()
{
    TRACE("*** RESTART REQUESTED VIA REST ***")

    webServer.send(200, "application/json", "{}");
    onboard_led_blink_once = true;

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
    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_wifiinfo()
{
    String r = restWifiInfo();
    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;
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
    onboard_led_blink_once = true;
    onboard_led_paired = true;
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

    if (r.length() == 0)
    {
        webServer.send(200, "application/json", "{}");
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
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

    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_cleanup()
{
    String r = restCleanup();
    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_cleanup_pm()
{
    String r = restCleanupPm();
    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_cleanup_autonom()
{
    String r = restCleanupAutonom();
    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_keybox_actuate()
{
    String channel_str;
    String r;    

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        
        r = restActionAutonomKeyboxActuate(channel_str);
    }
    else
    {
        r = "Wrong or missing arguments";
    }

    if (r.isEmpty())
    {
        webServer.send(200, "application/json", "{}");
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_rfid_lock_program()
{
    String code_str;
    String timeout_str;

    bool argument_ok = true;
    String r;    

    if (webServer.hasArg("code") == true)
    {
        code_str = webServer.arg("code");        
    }
    else
    {
        argument_ok = false;
    }

    if (webServer.hasArg("timeout") == true)
    {
        timeout_str = webServer.arg("timeout");        
    }
    else
    {
        argument_ok = false;
    }

    if (argument_ok == true)
    {
        r = restActionAutonomRfidLockProgram(code_str, (uint16_t) timeout_str.toInt());
    }
    else
    {
        r = "Wrong or missing arguments";
    }

    if (r.isEmpty())
    {
        webServer.send(200, "application/json", "{}");
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
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
    onboard_led_blink_once = true;
    onboard_led_paired = true;
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
    onboard_led_blink_once = true;
    onboard_led_paired = true;
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
    onboard_led_blink_once = true;
    onboard_led_paired = true;
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
    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_get_autonom()
{
    DEBUG("on_get_autonom")
    String r = restGetAutonom();
    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_pop_log()
{
    String r = restPopLog();
    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;
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
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/keybox/actuate", HTTP_POST, on_action_autonom_keybox_actuate);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/rfid-lock/program", HTTP_POST, on_action_autonom_rfid_lock_program);
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