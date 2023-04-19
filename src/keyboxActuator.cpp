#ifdef INCLUDE_KEYBOX

#include <Arduino.h>
#include <keyboxActuator.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>

void KeyboxActuatorConfig::from_json(const JsonVariant &json)
{
    if (json.containsKey("addr"))
    {
        const JsonVariant &_json = json["addr"];

        if (_json.is<JsonArray>())
        {
            size_t i=0;

            for (; i<sizeof(addr)/sizeof(addr[0]); ++i)
            {
                addr[i].clear();
            }
            
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();
            i=0;

            while(iterator != jsonArray.end() && i<sizeof(addr)/sizeof(addr[0]))
            {
                const JsonVariant & __json = *iterator;

                if (__json.containsKey("channel"))
                {
                    const JsonVariant &_json_channel = __json["channel"];
                    addr[i].from_json(_json_channel);
                }

                ++iterator;
                ++i;;
            }
        }
    }

    if (json.containsKey("latch"))
    {
        const JsonVariant &_json = json["latch"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &_json_channel = _json["channel"];
            latch.from_json(_json_channel);
        }
    }

    if (json.containsKey("power"))
    {
        const JsonVariant &_json = json["power"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &_json_channel = _json["channel"];
            power.from_json(_json_channel);
        }
    }

    if (json.containsKey("status"))
    {
        const JsonVariant &_json = json["status"];

        if (_json.containsKey("channel"))
        {
            const JsonVariant &_json_channel = _json["channel"];
            status.from_json(_json_channel);
        }
    }
}

void KeyboxActuatorConfig::to_eprom(std::ostream &os) const
{
    for (size_t i=0; i<sizeof(addr)/sizeof(addr[0]); ++i)
    {
        addr[i].to_eprom(os);
    }

    latch.to_eprom(os);
    power.to_eprom(os);
    status.to_eprom(os);
}

bool KeyboxActuatorConfig::from_eprom(std::istream &is)
{
    for (size_t i=0; i<sizeof(addr)/sizeof(addr[0]); ++i)
    {
        addr[i].from_eprom(is);
    }

    latch.from_eprom(is);
    power.from_eprom(is);
    status.from_eprom(is);
    return is_valid() && !is.bad();
}


static KeyboxActuatorConfig _config;
static bool _is_active = false;
static bool _is_finished = true;
static BinarySemaphore _semaphore;
static KeyboxStatus _status;
static size_t channel_to_actuate = size_t(-1);


static void install_config()
{
    // start from latch & power and set them to down at once

    pinMode(_config.power.gpio, OUTPUT);
    digitalWrite(_config.power.gpio, _config.power.inverted ? 1 : 0);

    pinMode(_config.latch.gpio, OUTPUT);
    digitalWrite(_config.latch.gpio, _config.latch.inverted ? 1 : 0);

    pinMode(_config.status.gpio, INPUT);

    for (size_t i=0; i<sizeof(_config.addr)/sizeof(_config.addr[0]); ++i)
    {
       pinMode(_config.addr[i].gpio, OUTPUT);
       digitalWrite(_config.addr[i].gpio, _config.addr[i].inverted ? 1 : 0);
    }
}


static void uninstall_config()
{
    // make sure the coil is left down
    digitalWrite(_config.power.gpio, _config.power.inverted ? 1 : 0);
    digitalWrite(_config.latch.gpio, _config.latch.inverted ? 1 : 0);
}


void keybox_actuator_task(void *parameter)
{
    size_t _millis_status_trace = 0;

    while (_is_active)
    {
        // actuate

        if (channel_to_actuate != size_t(-1))
        {
            if (channel_to_actuate < KEYBOX_NUM_CHANNELS)
            {            
                TRACE("actuating channel %d", (int) channel_to_actuate)

                { Lock lock(_semaphore);

                for (size_t j=0; j<sizeof(_config.addr)/sizeof(_config.addr[0]); ++j)
                {
                    int addr_bit_value = ((1 << j) & (_config.addr[j].inverted ? ~channel_to_actuate: channel_to_actuate)) ? 1 : 0;
                    digitalWrite(_config.addr[j].gpio, addr_bit_value);
                }

                channel_to_actuate = size_t(-1);
                
                digitalWrite(_config.power.gpio, _config.power.inverted ? 0 : 1);
                digitalWrite(_config.latch.gpio, _config.latch.inverted ? 0 : 1);

                delay(2000);

                digitalWrite(_config.latch.gpio, _config.latch.inverted ? 1 : 0);
                digitalWrite(_config.power.gpio, _config.power.inverted ? 1 : 0);

                for (size_t j=0; j<sizeof(_config.addr)/sizeof(_config.addr[0]); ++j)
                {
                    digitalWrite(_config.addr[j].gpio, _config.addr[j].inverted ? 1 : 0);
                }

                TRACE("actuate channel complete")
                } // lock
            }    
        }

        // read status

        { Lock lock(_semaphore); 
        
        KeyboxStatus new_status = _status;

        // just in case put latch / power to down since we are going to go through channels rapidly
        // and an activated coil signal will be a disaster

        digitalWrite(_config.power.gpio, _config.power.inverted ? 1 : 0);
        digitalWrite(_config.latch.gpio, _config.latch.inverted ? 1 : 0);
        
        //int _debug2[KEYBOX_NUM_CHANNELS];

        for (size_t i=0; i<KEYBOX_NUM_CHANNELS; ++i)
        {
            //int _debug1[sizeof(_config.addr)/sizeof(_config.addr[0])];

            for (size_t j=0; j<sizeof(_config.addr)/sizeof(_config.addr[0]); ++j)
            {
               int addr_bit_value = ((1 << j) & (_config.addr[j].inverted ? ~i : i)) ? 1 : 0;
               digitalWrite(_config.addr[j].gpio, addr_bit_value);

              //_debug1[j] = addr_bit_value;
            }
            
            //DEBUG("addr %d %d %d %d", _debug1[3], _debug1[2], _debug1[1], _debug1[0])

            delay(1);
            int status_bit = digitalRead(_config.status.gpio);
            status_bit = _config.status.inverted ? (status_bit == 0 ? 1 : 0) : status_bit;
            new_status.set_status(i, status_bit);

            //_debug2[i] = status_bit;
        }
        
        //DEBUG("status %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", 
        //      _debug2[0], _debug2[1], _debug2[2], _debug2[3], _debug2[4],
        //      _debug2[5], _debug2[6], _debug2[7], _debug2[8], _debug2[9],
        //      _debug2[10], _debug2[11], _debug2[12], _debug2[13], _debug2[14])

        // summarize once a minute status of all channels in log

        size_t new_millis_status_trace = millis();

        if (_millis_status_trace == 0 || _millis_status_trace > new_millis_status_trace || 
            (new_millis_status_trace - _millis_status_trace) >= 60*1000)
        {
            _millis_status_trace = new_millis_status_trace;
            TRACE("status %s", _status.as_string().c_str())
        }

        // keep channel addr to 0 by default

        for (size_t j=0; j<sizeof(_config.addr)/sizeof(_config.addr[0]); ++j)
        {
            digitalWrite(_config.addr[j].gpio, _config.addr[j].inverted ? 1 : 0);
        }

        if (!(new_status == _status))
        {
            for (size_t i=0; i<KEYBOX_NUM_CHANNELS; ++i)
            {
                bool old_channel_status = _status.get_status(i);
                bool new_channel_status = new_status.get_status(i);

                if (old_channel_status != new_channel_status)
                {
                    TRACE("status for channel %d changed to %d", (int) i, (int) new_channel_status)
                }
            }            
        }

        _status = new_status;

        } // lock

        delay(100);
    }

    _is_finished = true;
    vTaskDelete(NULL);
}


KeyboxStatus get_keybox_status()
{
    Lock lock(_semaphore);
    return _status;
}


void actuate_channel(size_t channel)
{
    channel_to_actuate = channel;
}


void start_keybox_actuator_task(const KeyboxActuatorConfig &config)
{
    if (_is_active == true)
    {
        reconfigure_keybox_actuator_task(config);
        return;
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    Lock lock(_semaphore);

    _is_finished = false;
    _is_active = true;
    _config = config;

    install_config();

    TRACE("Starting keybox_actuator task")

    xTaskCreate(
        keybox_actuator_task,        // Function that should be called
        "keybox_actuator_task",      // Name of the task (for debugging)
        4800,               // Stack size (bytes)
        (void *)(&_config), // Parameter to pass
        1,                  // Task priority
        NULL                // Task handle
    );
}


void reconfigure_keybox_actuator_task(const KeyboxActuatorConfig &config)
{
    if (_is_active == false)
    {
        start_keybox_actuator_task(config);
        return;
    }

    Lock lock(_semaphore);

    if (config == _config)
    {
        return;
    }

    uninstall_config();

    _config = config;
   
    install_config();

     TRACE("Reconfigured keybox_actuator task")
}


void stop_keybox_actuator_task()
{
    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }

    uninstall_config();
}

#endif // INCLUDE_KEYBOX