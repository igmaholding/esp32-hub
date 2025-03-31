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

// NOTE:
//
// 1.
// the temperature config has DS18B20 device address and it is optional (but has to be an empty string then); 
// if there is only one device present and no device address is configured - then the one found is used
//
// moreover, the channel also can be empty (-1) in which case temperature is not requested this
// way (e.g. in case of AHT10 himudity sensor which has its own temperature reading)

// ESP32 + HIH5030

[{
    "function":"shower-guard", 
    "config":{
                "motion":{"channel":{"gpio":23, "inverted":0, "debounce":250}},
                "rh":{"hw":"HIH5030", "vad":{"channel":{"gpio":34, "atten":3}},"vdd":{"channel":{"gpio":35, "atten":3}}, "corr":0.0}, 
                "temp":{"channel":{"gpio":4}, "addr":"28-01201d2496c8", "corr":0}, 
                "lumi":{"ldr":{"channel":{"gpio":32, "atten":3}}, "corr":0.0, "threshold":50}, 
                "light":{"channel":{"gpio":12, "inverted":0, "coilon_active":1}, "mode":"auto", "linger":60}, 
                "fan":{"channel":{"gpio":13, "inverted":0, "coilon_active":1}, "rh_off":45, "rh_on":57, "mode":"auto", "linger":600} 

    }
}]

// ESP32-s2 + HIH5030 --- OLD

[{
    "function":"shower-guard", 
    "config":{
                "motion":{"channel":{"gpio":38, "inverted":0, "debounce":250}},
                "rh":{"hw":"HIH5030", "vad":{"channel":{"gpio":14, "atten":3}},"vdd":{"channel":{"gpio":15, "atten":3}}, "corr":0.0}, 
                "temp":{"channel":{"gpio":4}, "addr":"28-0300a2794313", "corr":0}, 
                "lumi":{"ldr":{"channel":{"gpio":12, "atten":3}}, "corr":0.0, "threshold":50}, 
                "light":{"channel":{"gpio":36, "inverted":0, "coilon_active":1}, "mode":"auto", "linger":120}, 
                "fan":{"channel":{"gpio":34, "inverted":0, "coilon_active":1}, "rh_off":45, "rh_on":57, "mode":"auto", "linger":600} 

    }
}]

// ESP32-s2 + HIH5030 --- PCB

[{
    "function":"shower-guard", 
    "config":{
                "motion":{"channel":{"gpio":38, "inverted":0, "debounce":250}},
                "rh":{"hw":"HIH5030", "vad":{"channel":{"gpio":14, "atten":3}},"vdd":{"channel":{"gpio":16, "atten":3}}, "corr":0.0}, 
                "temp":{"channel":{"gpio":4}, "addr":"28-0300a2794313", "corr":0}, 
                "lumi":{"ldr":{"channel":{"gpio":12, "atten":3}}, "corr":0.0, "threshold":50}, 
                "light":{"channel":{"gpio":36, "inverted":0, "coilon_active":1}, "mode":"auto", "linger":120}, 
                "fan":{"channel":{"gpio":34, "inverted":0, "coilon_active":1}, "rh_off":45, "rh_on":57, "mode":"auto", "linger":600} 

    }
}]

// esp32-s2 + AHT10

[{
    "function":"shower-guard", 
    "config":{
                "motion":{"channel":{"gpio":38, "inverted":0, "debounce":250}},
                "rh":{"hw":"AHT10", "sda":{"channel":{"gpio":33}},"scl":{"channel":{"gpio":35}}, "addr":"0x38", "corr":0.0}, 
                "temp":{"channel":{"gpio":4}, "addr":"28-0300a2794313", "corr":0}, 
                "lumi":{"ldr":{"channel":{"gpio":12, "atten":3}}, "corr":0.0, "threshold":50}, 
                "light":{"channel":{"gpio":36, "inverted":0, "coilon_active":1}, "mode":"auto", "linger":120}, 
                "fan":{"channel":{"gpio":34, "inverted":0, "coilon_active":1}, "rh_off":45, "rh_on":57, "mode":"auto", "linger":600} 

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

ESP32-S2, OBS! gpio34 at startup == high makes target go back to programming mode, e.g. not starting! not use!

[{
    "function":"proportional", 
    "config":{

            "channels":[
                        {"one_a":{"gpio":3, "inverted":false},
                            "one_b":{"gpio":5, "inverted":false},
                            "open":{"gpio":7, "inverted":false, "debounce":250},
                            "closed":{"gpio":9, "inverted":false, "debounce":250},
                            "load_detect":{"pin":{"gpio":1,"atten":0}, "resistance":1.0, "current_threshold":0.05},
                            "valve_profile":"xs05"
                            },
                        {"one_a":{"gpio":2, "inverted":false},
                            "one_b":{"gpio":4, "inverted":false},
                            "open":{"gpio":6, "inverted":false, "debounce":250},
                            "closed":{"gpio":8, "inverted":false, "debounce":250},
                            "load_detect":{"pin":{"gpio":13,"atten":0}, "resistance":1.0, "current_threshold":0.05},
                            "valve_profile":"xs05"
                            },
                        {"one_a":{"gpio":36, "inverted":false},
                            "one_b":{"gpio":21, "inverted":false},
                            "open":{"gpio":35, "inverted":false, "debounce":250},
                            "closed":{"gpio":33, "inverted":false, "debounce":250},
                            "load_detect":{"pin":{"gpio":16,"atten":0}, "resistance":1.0, "current_threshold":0.05},
                            "valve_profile":"xs05"
                            }
        

                        ],
            

            "valve_profiles":[
                        {"name":"xs05", "open_time":6.3, "max_actuate_add_ups":1,
                            "time_2_flow_rate":[[25,2],[30,20],[60,75], [80,80], [100,100]]
                        
                        }
                        
                        ]
                },
    
}]

[{
    "function":"zero2ten", 
    "config":{

            "input_channels":[
                            {"gpio":6,"atten":3, "ratio":0.20408},
                            {"gpio":8,"atten":3, "ratio":0.20408},
                            {"gpio":12,"atten":3, "ratio":0.20408},
                            {"gpio":16,"atten":3, "ratio":0.20408}
                        ],

            "output_channels":[
                        {"output":{"gpio":17, "inverted":false}, "max_voltage":10.0, "loopback":{"gpio":1,"atten":0, "ratio":0.20408}},
                        {"output":{"gpio":13, "inverted":false}, "max_voltage":10.0, "loopback":{"gpio":3,"atten":0, "ratio":0.20408}},
                        {"output":{"gpio":14, "inverted":false}, "max_voltage":10.0, "loopback":{"gpio":5,"atten":0, "ratio":0.20408}},
                        {"output":{"gpio":18, "inverted":false}, "max_voltage":10.0, "loopback":{"gpio":7,"atten":0, "ratio":0.20408}}
                        ],
            

            "applets":[
                        {"name":"utk", "function":"temp2out", "input_channel":-1, "output_channel":0,"temp":{"channel":{"gpio":4}, "corr":0},
                         "map_table":[    // RAF-1, 20..70C == 0..10V
                                        [-35, 9.4],
                                        [-30, 9.2],
                                        [-25, 8.9],
                                        [-20, 8.4],
                                        [-15, 7.6],
                                        [-10, 6.6],
                                        [-5, 5.2],
                                        [0, 4.2],
                                        [5, 3.2],
                                        [10, 2.2],
                                        [15, 1.4],
                                        [20, 0]
                                        ]
                        }
                    ]
                },
    
}]

[{
    "function":"mains-probe", 
    "config":{

            "i2c":{"scl":{"channel":{"gpio":35}}, "sda":{"channel":{"gpio":33}}},

            "input_v_channels":[
                                { 
                                    "addr":"0x40",

                                    "channels": [   
                                                    {"channel":0, "ratio":0.20408},
                                                    {"channel":1, "ratio":0.20408},
                                                    {"channel":2, "ratio":0.20408}
                                                ]
                                }
                        ],

            "input_a_high_channels":[
                                { 
                                    "addr":"0x41",

                                    "channels": [   
                                                    {"channel":0, "ratio":0.20408},
                                                    {"channel":1, "ratio":0.20408},
                                                    {"channel":2, "ratio":0.20408}
                                                ]
                                }
                        ],

            "input_a_low_channels":[
                            {"gpio":8,"atten":3, "ratio":0.20408},
                            {"gpio":6,"atten":3, "ratio":0.20408},
                            {"gpio":12,"atten":3, "ratio":0.20408}
                        ],

            "applets":[
                    ]
                },
    
}]

[{
    "function":"multi", 
    "config":{

        "uart":{"uart_num":2, "tx":{"channel":{"gpio":17}}, "rx":{"channel":{"gpio":18}}},

        "bt" : {"hw":"FSC-BT955","reset":{"gpio":19, "inverted":true}},
        
        "fm":{"hw":"RDA5807", "addr":"0x60"},

        "i2s":{"dout":{"channel":{"gpio":6}}, "bclk":{"channel":{"gpio":4}},"lrc":{"channel":{"gpio":5}}},
        
        "i2c":{"scl":{"channel":{"gpio":8}}, "sda":{"channel":{"gpio":7}}},

        "service":{"url":[{"name":"lugna", "value":"http://fm03-ice.stream.khz.se/fm03_aac"}, 
                          {"name":"ffh-de", "value":"http://mp3.ffh.de/radioffh/hqlivestream.aac"},
                          {"name":"heart00s", "value":"http://vis.media-ice.musicradio.com/Heart00s"},
                          {"name":"power", "value":"http://fm03-ice.stream.khz.se/fm04_aac"},
                          {"name":"rix", "value":"http://fm03-ice.stream.khz.se/fm01_aac"}
                         ],
                   
                   "url_select": 1,  
                   
                   "fm_freq":[{"name":"rix", "value":101.7}, 
                              {"name":"star", "value":90.4},
                              {"name":"p1", "value":91.2},
                              {"name":"p2", "value":94.6},
                              {"name":"p3", "value":96.5}
                             ],
                             
                   "fm_freq_select": 0
                   
                   },
        
        "sound":{"hw":"TDA8425", "addr":"0x41", "mute":{"gpio":9, "inverted":false}, "volume":100, "volume_low":30, 
                 "gain_low_pass":7, "gain_band_pass":0, "gain_high_pass":10,
                 "schedule":[1,1,1,1,0,0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]}                
    }
}]   

        # volume is 0..100, it is a max volume applied according to schedule
        #
        # tone gain is in dB; note that band_pass might not be supported; in this case if the value is not 0
        # it will be used to offset low and high pass values




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
URL: <base>/action/autonom/rfid-lock/add?name=igma&code=47HuF9CemtholcVCz9A6&type=RFID&lock=main&lock=left
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

REST POST action
URL: <base>/action/autonom/proportional/calibrate?channel=XX
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/proportional/actuate?channel=XX&value=YY&ref=ZZ    ref is optional, 0 or 100 
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/zero2ten/calibrate_input?6channel=XX&value=YY // without the value -> uncalibrate
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/zero2ten/input?channel=XX
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/zero2ten/calibrate_output?channel=XX&value=YY  // without the value -> uncalibrate
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/zero2ten/output?channel=XX&value=YY
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/mains-probe/calibrate_v?addr=ZZ&channel=XX&value=YY // without the value -> uncalibrate
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/mains-probe/calibrate_a_high?addr=ZZ&channel=XX&value=YY // without the value -> uncalibrate
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/mains-probe/calibrate_a_low?channel=XX&value=YY // without the value -> uncalibrate
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/mains-probe/input_v?addr=ZZ&channel=XX
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/mains-probe/input_a_high?addr=ZZ&channel=XX
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/mains-probe/input_a_low?channel=XX
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/multi/uart_command?command=XX
BODY: none
RESPONSE: 
{
}

// all params are optional!
REST POST action  
URL: <base>/action/autonom/multi/audio_control?source=<bt, www, fm or none>,channel=<channel index, for webradio or fm>,volume=<0..100> 
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
 {"proportional":{"channel[0]":{"state":"idle", "error":"", "value":22, "config_open_time":6.2, "calib_open_time":6.0}, ...}},   
 }

 'system': {'uptime': '46d 22h 02m 10s'}

}

NOTE for proportional: "calib_open_time" can be variated with "calib_open_2_closed_time" and "calib_closed_2_open_time",  
subject to #ifdef

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

void on_action_autonom_rfid_lock_add()
{
    String name_str;
    String code_str;
    String type_str;
    std::vector<String> locks;

    bool argument_ok = true;
    String r;    

    if (webServer.hasArg("name") == true)
    {
        name_str = webServer.arg("name");        
    }
    else
    {
        argument_ok = false;
    }

    if (webServer.hasArg("code") == true)
    {
        code_str = webServer.arg("code");        
    }
    else
    {
        argument_ok = false;
    }

    if (webServer.hasArg("type") == true)
    {
        type_str = webServer.arg("type");        
    }
    else
    {
        argument_ok = false;
    }

    for (size_t i=0; i<webServer.args(); ++i)
    {
        if (webServer.argName(i) == "lock")
        {
            locks.push_back(webServer.arg(i));
        }
    }

    if (argument_ok == true)
    {
        r = restActionAutonomRfidLockAdd(name_str, code_str, locks, type_str);
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

void on_action_autonom_proportional_calibrate()
{
    String channel_str;
    String r;    

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        
        r = restActionAutonomProportionalCalibrate(channel_str);
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

void on_action_autonom_proportional_actuate()
{
    String channel_str;
    String value_str;
    String ref_str;
    String r;    

    if (webServer.hasArg("channel") == true && webServer.hasArg("value") == true)
    {
        channel_str = webServer.arg("channel");        
        value_str = webServer.arg("value");        
        
        if (webServer.hasArg("ref") == true)
        {
            ref_str = webServer.arg("ref"); 
        }
        
        r = restActionAutonomProportionalActuate(channel_str, value_str, ref_str);
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

void on_action_autonom_zero2ten_calibrate_input()
{
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        

        if (webServer.hasArg("value") == true)
        {
            value_str = webServer.arg("value");        
        }
        // otherwise - uncalibrate
        
        r = restActionAutonomZero2tenCalibrateInput(channel_str, value_str);
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

void on_action_autonom_zero2ten_input()
{
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        
        
        r = restActionAutonomZero2tenInput(channel_str, value_str);
    }
    else
    {
        r = "Wrong or missing arguments";
    }

    if (r.isEmpty())
    {
        webServer.send(200, "application/json", String("{\"value\":\"" + value_str + "\"}"));
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_zero2ten_calibrate_output()
{
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        

        if (webServer.hasArg("value") == true)
        {
            value_str = webServer.arg("value");        
        }
        // otherwise - uncalibrate

        r = restActionAutonomZero2tenCalibrateOutput(channel_str, value_str);
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

void on_action_autonom_zero2ten_output()
{
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("channel") == true && webServer.hasArg("value") == true)
    {
        channel_str = webServer.arg("channel");        
        value_str = webServer.arg("value");        
        
        r = restActionAutonomZero2tenOutput(channel_str, value_str);
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

void on_action_autonom_mains_probe_calibrate_v()
{
    String addr_str;
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("addr") == true)
    {
        addr_str = webServer.arg("addr");
    }

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        

        if (webServer.hasArg("value") == true)
        {
            value_str = webServer.arg("value");        
        }
        // otherwise - uncalibrate
        
        r = restActionAutonomMainsProbeCalibrateV(addr_str, channel_str, value_str);
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

void on_action_autonom_mains_probe_calibrate_a_high()
{
    String addr_str;
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("addr") == true)
    {
        addr_str = webServer.arg("addr");
    }

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        

        if (webServer.hasArg("value") == true)
        {
            value_str = webServer.arg("value");        
        }
        // otherwise - uncalibrate
        
        r = restActionAutonomMainsProbeCalibrateAHigh(addr_str, channel_str, value_str);
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

void on_action_autonom_mains_probe_calibrate_a_low()
{
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        

        if (webServer.hasArg("value") == true)
        {
            value_str = webServer.arg("value");        
        }
        // otherwise - uncalibrate
        
        r = restActionAutonomMainsProbeCalibrateALow(channel_str, value_str);
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

void on_action_autonom_mains_probe_input_v()
{
    String addr_str;
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("addr") == true)
    {
        addr_str = webServer.arg("addr");
    }

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        
        
        r = restActionAutonomMainsProbeInputV(addr_str, channel_str, value_str);
    }
    else
    {
        r = "Wrong or missing arguments";
    }

    if (r.isEmpty())
    {
        webServer.send(200, "application/json", String("{\"value\":\"" + value_str + "\"}"));
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_mains_probe_input_a_high()
{
    String addr_str;
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("addr") == true)
    {
        addr_str = webServer.arg("addr");
    }

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        
        
        r = restActionAutonomMainsProbeInputAHigh(addr_str, channel_str, value_str);
    }
    else
    {
        r = "Wrong or missing arguments";
    }

    if (r.isEmpty())
    {
        webServer.send(200, "application/json", String("{\"value\":\"" + value_str + "\"}"));
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_mains_probe_input_a_low()
{
    String channel_str;
    String value_str;
    String r;    

    if (webServer.hasArg("channel") == true)
    {
        channel_str = webServer.arg("channel");        
        
        r = restActionAutonomMainsProbeInputALow(channel_str, value_str);
    }
    else
    {
        r = "Wrong or missing arguments";
    }

    if (r.isEmpty())
    {
        webServer.send(200, "application/json", String("{\"value\":\"" + value_str + "\"}"));
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_multi_uart_command()
{
    String command;
    String response;
    String r;    

    if (webServer.hasArg("command") == true)
    {
        command = webServer.arg("command");        
        
        r = restActionAutonomMultiUartCommand(command, response);
    }
    else
    {
        r = "Wrong or missing arguments";
    }

    if (r.isEmpty())
    {
        webServer.send(200, "application/json", String("{\"response\":\"" + response + "\"}"));
    }
    else
    {
        webServer.send(500, "application/json", String("{\"error\":\"" + r + "\"}"));
    }

    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_multi_audio_control()
{
    String source;
    String channel;
    String volume;
    String response;
    String r;    

    if (webServer.hasArg("source") == true)
    {
        source = webServer.arg("source");        
    }

    if (webServer.hasArg("channel") == true)
    {
        channel = webServer.arg("channel");        
    }

    if (webServer.hasArg("volume") == true)
    {
        volume = webServer.arg("volume");        
    }

    r = restActionAutonomMultiAudioControl(source, channel, volume, response);
    
    if (r.isEmpty())
    {
        webServer.send(200, "application/json", String("{\"response\":\"" + response + "\"}"));
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
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/rfid-lock/add", HTTP_POST, on_action_autonom_rfid_lock_add);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/proportional/calibrate", HTTP_POST, on_action_autonom_proportional_calibrate);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/proportional/actuate", HTTP_POST, on_action_autonom_proportional_actuate);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/zero2ten/calibrate_input", HTTP_POST, on_action_autonom_zero2ten_calibrate_input);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/zero2ten/input", HTTP_POST, on_action_autonom_zero2ten_input);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/zero2ten/calibrate_output", HTTP_POST, on_action_autonom_zero2ten_calibrate_output);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/zero2ten/output", HTTP_POST, on_action_autonom_zero2ten_output);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/mains-probe/calibrate_v", HTTP_POST, on_action_autonom_mains_probe_calibrate_v);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/mains-probe/calibrate_a_high", HTTP_POST, on_action_autonom_mains_probe_calibrate_a_high);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/mains-probe/calibrate_a_low", HTTP_POST, on_action_autonom_mains_probe_calibrate_a_low);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/mains-probe/input_v", HTTP_POST, on_action_autonom_mains_probe_input_v);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/mains-probe/input_a_high", HTTP_POST, on_action_autonom_mains_probe_input_a_high);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/mains-probe/input_a_low", HTTP_POST, on_action_autonom_mains_probe_input_a_low);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/multi/uart_command", HTTP_POST, on_action_autonom_multi_uart_command);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/multi/audio_control", HTTP_POST, on_action_autonom_multi_audio_control);
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