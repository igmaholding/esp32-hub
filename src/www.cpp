#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <onboardLed.h>

#include <rest.h>
#include <trace.h>
#include <www.h>

WebServer webServer(80);

#define MAX_BODY_SIZE 16000

#define ASSERT_BODY_SIZE(_body_) if (_body_.length() > MAX_BODY_SIZE) { ERROR("JSON body exceeds %d bytes", (int) MAX_BODY_SIZE); _body_.clear(); }

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
        "sound":{"volume":100, "volume_low":30, 
                 "gain_low_pass":0, "gain_band_pass":-7, "gain_high_pass":3,
                 "schedule":[0,0,0,0,0,0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2]}                
    }
}]

[{
    "function":"rfid-lock", 
    "config":{
                "rfid":{"protocol":"SPI", "hw":"RC522", "resetPowerDownPin":21, "chipSelectPin":5, "mosiPin":23, "misoPin":19, "clkPin":18},

                "keypad":{"c":[{"channel":{"gpio":25, "inverted":false}},  // keypad can be 4x4 or 3x4 
                            {"channel":{"gpio":26, "inverted":false}}, 
                            {"channel":{"gpio":27, "inverted":false}}],
                        "l":[{"channel":{"gpio":34, "inverted":true}}, 
                            {"channel":{"gpio":35, "inverted":true}}, 
                            {"channel":{"gpio":36, "inverted":true}}, 
                            {"channel":{"gpio":39, "inverted":true}}],
                        "debounce":250
                },

                "lock":{"channels":{"main":{"gpio":17, "inverted":0, "coilon_active":1}, 
                                    "sb1":{"gpio":16, "inverted":0, "coilon_active":1},
                                    "sb2":{"gpio":13, "inverted":0, "coilon_active":1},
                                    "sb3":{"gpio":4, "inverted":0, "coilon_active":1}
                                    }, "linger":3},

                "buzzer":{"channel":{"gpio":12, "inverted":false}},

                "green_led":{"gpio":15, "inverted":false},

                "red_led":{"gpio":14, "inverted":false}
    }
}]
                KEYPAD 4x4
                "keypad":{"c":[{"channel":{"gpio":25, "inverted":false}},  // keypad can be 4x4 or 3x4 
                            {"channel":{"gpio":26, "inverted":false}}, 
                            {"channel":{"gpio":27, "inverted":false}}, 
                            {"channel":{"gpio":32, "inverted":false}}],
                        "l":[{"channel":{"gpio":34, "inverted":true}}, 
                            {"channel":{"gpio":35, "inverted":true}}, 
                            {"channel":{"gpio":36, "inverted":true}}, 
                            {"channel":{"gpio":39, "inverted":true}}],
                        "debounce":250
                },

                for I2C (not supported by the breakout)
                "rfid":{"protocol":"I2C", "hw":"RC522", "sclPin":23<same as MOSI>, "sdaPin":5<same as chipSelect>, "i2cAddress":40},

                PROTOTYPE1
                "rfid":{"protocol":"SPI", "hw":"RC522", "resetPowerDownPin":13, "chipSelectPin":5, "sclPin":22, "sdaPin":21, "i2cAddress":40},
                "lock":{"channels":{"main":{"gpio":32, "inverted":0, "coilon_active":1}}, "linger":3},
                "buzzer":{"channel":{"gpio":27, "inverted":false}},
                "green_led":{"gpio":14, "inverted":false},
                "red_led":{"gpio":12, "inverted":false}




ESP32-S2, OBS! gpio34 at startup == high makes target go back to programming mode, e.g. not starting! not use!

[{
    "function":"proportional", 
    "config":{

            "channels":[
                        {"one_a":{"gpio":5, "inverted":false},
                            "one_b":{"gpio":3, "inverted":false},
                            "open":{"gpio":9, "inverted":false, "debounce":250},
                            "closed":{"gpio":7, "inverted":false, "debounce":250},
                            "load_detect":{"pin":{"gpio":1,"atten":0}, "resistance":1.0, "current_threshold":0.05},
                            "valve_profile":"xs05"
                            },
                        {"one_a":{"gpio":4, "inverted":false},
                            "one_b":{"gpio":2, "inverted":false},
                            "open":{"gpio":8, "inverted":false, "debounce":250},
                            "closed":{"gpio":6, "inverted":false, "debounce":250},
                            "load_detect":{"pin":{"gpio":13,"atten":0}, "resistance":1.0, "current_threshold":0.05},
                            "valve_profile":"xs05"
                            },
                        {"one_a":{"gpio":21, "inverted":false},
                            "one_b":{"gpio":36, "inverted":false},
                            "open":{"gpio":33, "inverted":false, "debounce":250},
                            "closed":{"gpio":35, "inverted":false, "debounce":250},
                            "load_detect":{"pin":{"gpio":16,"atten":0}, "resistance":1.0, "current_threshold":0.05},
                            "valve_profile":"xs05"
                            }
        

                        ],
            

            "valve_profiles":[
                        {"name":"xs05", "open_time":6.3, "max_actuate_add_ups":1,
                            "time_2_flow_rate":[[17,6],[18,12],[19,20],[20,27],[21,33],[22,38],[23,44],[24,49],[25,53],[26,57],[27,60],[28,63],
                                                                         [29,66],[30,70],[31,72],[32,75],[33,77],[34,78],[35,81],[36,82],[37,83],[38,85],[39,86],[40,87],[45,91],
                                                                         [50,93],[55,95],[60,96],[70,97], [100,100]]
                        
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
    "function":"multi", 
    "config":{

        "uart":{"uart_num":2, "tx":{"channel":{"gpio":17}}, "rx":{"channel":{"gpio":18}}},

        "bt" : {"name":"apt1", "pin":"4321", "hw":"FSC-BT1036","reset":{"gpio":19, "inverted":true}},
        
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
                   
                   "fm_freq":[{"name":"nrj", "value":105.5}, 
                              {"name":"star", "value":107.1},
                              {"name":"lugna", "value":104.7},
                              {"name":"rix", "value":106.7},
                              {"name":"p3", "value":99.3}
                             ],
                             
                   "fm_freq_select": 0
                   
                   },
        
        "sound":{"hw":"TDA8425", "addr":"0x41", "mute":{"gpio":9, "inverted":false}, "volume":100, "volume_low":30, 
                 "gain_low_pass":7, "gain_band_pass":0, "gain_high_pass":10,
                 "schedule":[1,1,1,1,0,0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]},

        "tm1638":{"dio":{"channel":{"gpio":37}}, "clk":{"channel":{"gpio":38}}},

        "ui":{"name":"apt1", "stb":{"channel":{"gpio":39}}, "audio_enabled":true, "thermostat_enabled":true}
    }
}]   
   

        # volume is 0..100, it is a max volume applied according to schedule
        #
        # tone gain is in dB; note that band_pass might not be supported; in this case if the value is not 0
        # it will be used to offset low and high pass values

[{
    "function":"mains-probe", 
    "config":{

            "i2c":{"scl":{"channel":{"gpio":35}}, "sda":{"channel":{"gpio":33}}},

            "input_v":[
                                { 
                                    "addr":"0x40",

                                    "channels": [   
                                                    {"channel":0, "default_poly_beta":[]},
                                                    {"channel":1, "default_poly_beta":[]},
                                                    {"channel":2, "default_poly_beta":[]}
                                                ]
                                }
                        ],

            "input_a_high":[
                                { 
                                    "addr":"0x41",

                                    "channels": [   
                                                    {"channel":0, "default_poly_beta":[]},
                                                    {"channel":1, "default_poly_beta":[]},
                                                    {"channel":2, "default_poly_beta":[]}
                                                ]
                                }
                        ],

            "input_a_low_channels":[
                            {"gpio":8,"atten":3, "default_poly_beta":[]},
                            {"gpio":6,"atten":3, "default_poly_beta":[]},
                            {"gpio":12,"atten":3, "default_poly_beta":[]}
                        ],

            "applets":[
                    ]
                },
    
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
URL: <base>/action/autonom/rfid-lock/add_code?name=igma&code=47HuF9CemtholcVCz9A6&type=RFID&lock=main&lock=left
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/delete_code?name=igma
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/delete_all_codes
BODY: none
RESPONSE: 
{
}

REST POST action
URL: <base>/action/autonom/rfid-lock/unlock?lock_channel=*
BODY: none
RESPONSE: 
{
}

REST GET get codes
URL: <base>/get/autonom/rfid-lock/codes
BODY: none
RESPONSE: 
{
 "codes": [
  {"name":"igma", "index":0, "type":"RFID", "code":"sfdgijfdigjiw54ijig5445", locks:["main", "sb1"]}, ...
    ]
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

// this is a multi-point calibration; the mapping table will fill in until the maximum points is reached (currently 5)
// and then if a more point is added - it overrides an existing point with the nearest x-value
//
// recommended calibration points for 230v: 8, 30, 90, 200, 300
// recommended calibration points for 230v: 20, 110, 200, 250, 300

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

REST GET get calibration data
URL: <base>/get/autonom/mains-probe/calibration_data
BODY: none
RESPONSE: 
{
 "input_v":[{"addr":<addr>, "index":<channel>, x_2_y_map[<floats: x1,y1,x2,y2...>], "poly_beta":[<floats: c0,c1,c2...>]}, ...],
 "input_a_high":[{"addr":<addr>, "index":<channel>, x_2_y_map[<floats: x1,y1,x2,y2...>], "poly_beta":[<floats: c0,c1,c2...>]}, ...],
 "input_a_low":[{"addr":"", "index":<channel>, x_2_y_map[<floats: x1,y1,x2,y2...>], "poly_beta":[<floats: c0,c1,c2...>]}, ...]
}

REST POST action 
URL: <base>/action/autonom/mains-probe/import_calibration_data
BODY:  
{ "mainsProbeCalibrationData":{
 "input_v":[{"addr":<addr>, "index":<channel>, x_2_y_map[<floats: x1,y1,x2,y2...>]}, ...],
 "input_a_high":[{"addr":<addr>, "index":<channel>, x_2_y_map[<floats: x1,y1,x2,y2...>]}, ...],
 "input_a_low":[{"addr":"", "index":<channel>, x_2_y_map[<floats: x1,y1,x2,y2...>]}, ...]
}}
RESPONSE: 
{
}

// NOTE!!!!! Due to limited buffer size for json parsing the above operation may need be taken by parts (e.g. one group at a time)


REST POST action
URL: <base>/action/autonom/multi/uart_command?command=XX
BODY: none
RESPONSE: 
{
}

// all params are optional!
REST POST action  
URL: <base>/action/autonom/multi/audio_control?source=<bt, www, fm or none>&channel=<channel index, for webradio or fm>&volume=<0..100> 
BODY: none
RESPONSE: 
{
}

// all old items are discarded, so sending "temps":[] will clear thermostat display
REST POST action  
URL: <base>/action/autonom/multi/set_volatile
BODY: 
[ 
    {"ui_name" : "apt1", 
     "temps":[{"item":"apt1", "temp":21.4},{"item":"out", "temp":-8.7}],
     "temp_corr" : 0.5}
]
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
    {
        "shower-guard":{"temp":22.1,"rh":19.8,"motion":false,"light":false,"fan":false,"light_decision":"","fan_decision":"rh-low 44.7/45.0 at 2022-12-16 11:04:53"},   
    }

    {
        "keybox": {'status': '0:0 1:1 2:1 3:1 4:0 5:1 6:1 7:1 8:0 9:0 10:1 11:0 12:1 13:1 14:0 (1==unlocked)'}, 
    }

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
    
    { 
        "proportional":{"channel[0]":{"state":"idle", "error":"", "value":22, "config_open_time":6.2, "calib_open_time":6.0}, ...}},   
    }

    {
    "multi": {
        "bt": {
            "latest_indications": {
                "A2DPMUTED": "0",
                "A2DPSTAT": "2",
                "AVRCPCFG": "15",
                "AVRCPSTAT": "1",
                "BTEN": "1",
                "DEVSTAT": "0",
                "HFPSTAT": "1",
                "I2SCFG": "1",
                "NAME": "APT1",
                "PIN": "4321",
                "PROFILE": "176",
                "PWRSTAT": "0",
                "SPKVOL": "0,14",
                "SSP": "0",
                "VER": "BT1036,V2.8.5,20250108"
            }
        },
        "www": {
            "is_streaming": true,
            "url_index": 1,
            "url_name": "ffh-de",
            "bitrate": 128000
        },
        "fm": {
            "is_streaming": false,
            "index": -1,
            "name": "",
            "freq": 0
        },
        "ui": {
            "name": "apt1",
            "temp_corr": 0   // this will appear if the temp_corr is set by terminal or via set_volatile action
        },
        "audio_control_data": {
            "source": "www",
            "channel": 1,
            "volume": 40
        },
        "commited_volume": 40,
        "title": "Pat Benatar - Love Is A Battlefield",
        "status": ""
        }
    }


    {
        "mains-probe":{"status":"",
                       "input_v_channels":{
                                            "0x40":{
                                                    "channel[0]":{"value":242.8183136,"status":""},
                                                    "channel[1]":{"value":242.1123962,"status":""},
                                                    "channel[2]":{"value":247.1510315,"status":""}
                                            }
                        },

                        "input_a_high_channels":{  same format as input_v_channels },
                        "input_a_low_channels":{ same format as input_v_channels but without the addr level },
                        "applet":{}}
    }


    {
        "zero2ten":{"status":"","
                    input":{"channel[0]":{"type":"input","value":0,"calibration_coefficient":1.024590492,"duty":0,"status":""},
                            "channel[1]":{"type":"input","value":0,"calibration_coefficient":1.049023747,"duty":0,"status":""},
                            "channel[2]":{"type":"input","value":0.006377551,"calibration_coefficient":1.066083789,"duty":0,"status":""},
                            "channel[3]":{"type":"input","value":0.004806152,"calibration_coefficient":1.071207523,"duty":0,"status":""}},
                    "output":{"channel[0]":{"type":"output","value":1.804999948,"calibration_coefficient":0.881999969,"duty":0.159189403,"status":""},
                              "channel[1]":{"type":"output","value":4.859999657,"calibration_coefficient":0.868000031,"duty":0.421839714,"status":""},
                              "channel[2]":{"type":"output","value":10,"calibration_coefficient":0.884000003,"duty":0.888359904,"status":""},
                              "channel[3]":{"type":"output","value":10,"calibration_coefficient":0.881999969,"duty":0.88640666,"status":""}},
                    "applet":{"utk":{"function":"temp2out","time":"2025-07-03 00:54.52","temp_addr":"28-031997797ae2","temp":11.89999962,"output_value":1.804999948,"status":""}}}
    }

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
        ASSERT_BODY_SIZE(body)
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
        ASSERT_BODY_SIZE(body)
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
        ASSERT_BODY_SIZE(body)
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

void on_action_autonom_rfid_lock_add_code()
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
        r = restActionAutonomRfidLockAddCode(name_str, code_str, locks, type_str);
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

void on_action_autonom_rfid_lock_delete_code()
{
    String name_str;

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

    if (argument_ok == true)
    {
        r = restActionAutonomRfidLockDeleteCode(name_str);
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

void on_action_autonom_rfid_lock_delete_all_codes()
{
    DEBUG("on_action_autonom_rfid_lock_delete_all_codes")
    String r;    

    r = restActionAutonomRfidLockDeleteAllCodes();

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

void on_action_autonom_rfid_lock_unlock()
{
    String lock_channel_str;

    bool argument_ok = true;
    String r;    

    if (webServer.hasArg("lock_channel") == true)
    {
        lock_channel_str = webServer.arg("lock_channel");        
    }
    else
    {
        argument_ok = false;
    }

    if (argument_ok == true)
    {
        r = restActionAutonomRfidLockUnlock(lock_channel_str);
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

void on_get_autonom_rfid_lock_codes()
{
    DEBUG("on_get_autonom_rfid_lock_codes")
    String r = restGetAutonomRfidLockCodes();
    DEBUG("%s", r.c_str())
    webServer.send(200, "application/json", r.c_str());
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

void on_get_autonom_mains_probe_calibration_data()
{
    DEBUG("on_get_autonom_mains_probe_calibration_data")
    String r = restGetAutonomMainsProbeCalibrationData();
    DEBUG("%s", r.c_str())
    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;
}

void on_action_autonom_mains_probe_import_calibration_data()
{
    DEBUG("on_action_autonom_mains_probe_import_calibration_data")

    String body;

    if (webServer.hasArg("plain") == false)
    {
        ERROR("mains-probe import calibration data POST request without a payload")
    }
    else
    {
        body = webServer.arg("plain");
        ASSERT_BODY_SIZE(body)
    }

    String r = restActionAutonomMainsProbeImportCalibrationData(body);

    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;

    DEBUG("on_action_autonom_mains_probe_import_calibration_data done")
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

void on_action_autonom_multi_set_volatile()
{
    DEBUG("on_action_autonom_multi_set_volatile")

    String body;

    if (webServer.hasArg("plain") == false)
    {
        ERROR("multi set volatile POST request without a payload")
    }
    else
    {
        body = webServer.arg("plain");
        ASSERT_BODY_SIZE(body)
    }

    String r = restActionAutonomMultiSetVolatile(body);

    webServer.send(200, "application/json", r.c_str());
    onboard_led_blink_once = true;
    onboard_led_paired = true;

    DEBUG("on_action_autonom_multi_set_volatile done")
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
    DEBUG("%s", r.c_str())
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
    DEBUG("%s", r.c_str())
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
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/rfid-lock/add_code", HTTP_POST, on_action_autonom_rfid_lock_add_code);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/rfid-lock/delete_code", HTTP_POST, on_action_autonom_rfid_lock_delete_code);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/rfid-lock/delete_all_codes", HTTP_POST, on_action_autonom_rfid_lock_delete_all_codes);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/rfid-lock/unlock", HTTP_POST, on_action_autonom_rfid_lock_unlock);
    webServer.on("/" HARVESTER_API_KEY "/get/autonom/rfid-lock/codes", HTTP_GET, on_get_autonom_rfid_lock_codes);
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
    webServer.on("/" HARVESTER_API_KEY "/get/autonom/mains-probe/calibration_data", HTTP_GET, on_get_autonom_mains_probe_calibration_data);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/mains-probe/import_calibration_data", HTTP_POST, on_action_autonom_mains_probe_import_calibration_data);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/multi/uart_command", HTTP_POST, on_action_autonom_multi_uart_command);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/multi/audio_control", HTTP_POST, on_action_autonom_multi_audio_control);
    webServer.on("/" HARVESTER_API_KEY "/action/autonom/multi/set_volatile", HTTP_POST, on_action_autonom_multi_set_volatile);
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