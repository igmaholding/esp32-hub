#ifdef INCLUDE_MULTI

#include <Audio.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <multi.h>
#include <gpio.h>
#include <trace.h>
#include <binarySemaphore.h>
#include <esp_log.h>
#include <time.h>
#include <i2c_utils.h>
#include <at.h>
#include <tda8425.h>
#include <rda5807.h>
#include <tm1638.h>

#define USE_HARDWARE_SERIAL

#ifdef USE_HARDWARE_SERIAL
#include <HardwareSerial.h>
# else
#include <driver/uart.h>
#endif

#define AUDIO_CONNECTION_TIMEOUT 3000 // ms
#define AUDIO_RECONNECT_TIMEOUT  5000 // ms
#define AUDIO_MAX_RECONNECT_ATTEMPTS  3

// NOTE: has to overload new / delete to clean the memory because the Audio class (library)
// DOES NOT INITIALIZES ITS MEMBER VARIABLES! thus relying on that it is created statically
// in clean memory

void* operator new(size_t size)
{
    void * m = calloc(size, 1);
    return m;
}

void operator delete(void* m)
{
    if (m)
    {
        free(m);
    }
}


int calc_loop_delay(int current_interval, int current_delay, int required_interval)
{
    int interval_delta = required_interval - current_interval;
     
    if (interval_delta < 0)
    {
        if (interval_delta * (-1) < current_delay)
        {
            return current_delay + interval_delta;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return current_delay + interval_delta;
    }
}

class UISM
{
public:

    enum State 
    {
        sNone = 0,
        sIdle = 1,
        sThermostat = 2,
        sAudio = 3,
        sTest  = 100
    };

    enum Button 
    {
        bRed = 0,
        bDown = 1,
        bUp = 2,
        bLeft = 3,
        bRight = 4,
        bTest = 6,
        bBlue = 7,

        bNone = -1
    };

    static const char* button_2_str(Button _button)
    {
        switch (_button)
        {
            case bNone: return "none";
            case bRed: return "red";
            case bDown: return "down";
            case bUp: return "up";
            case bLeft: return "left";
            case bRight: return "right";
            case bTest: return "test";
            case bBlue: return "blue";

            default: return "<unknown>";
        }
    }

    static Button str_2_button(const String & _string)
    {
        if (_string == button_2_str(bNone))
        {
            return bNone;
        }
        
        if (_string == button_2_str(bRed))
        {
            return bRed;
        }

        if (_string == button_2_str(bUp))
        {
            return bUp;
        }

        if (_string == button_2_str(bDown))
        {
            return bDown;
        }

        if (_string == button_2_str(bLeft))
        {
            return bLeft;
        }

        if (_string == button_2_str(bRight))
        {
            return bRight;
        }

        if (_string == button_2_str(bTest))
        {
            return bTest;
        }

        if (_string == button_2_str(bBlue))
        {
            return bBlue;
        }

        return bNone;
    }

    static const std::pair<int, String> BLUE_LED_INFO;

    const int PUSH_TEST_LED_INDEX = 0;
	
    const int THERMOSTAT_TIMEOUT = 4; // seconds

    const int LONGPRESS_PERFORATION = 2; // 1/x of how often a new press will be generated under the longpress condition

    UISM(const MultiConfig::UI & _config, const MultiConfig::Service & _service_config, const MultiConfig::Thermostat & _thermostat_config);
    UISM();

    void init();

    ~UISM();

    void reconfigure(const MultiConfig::UI & _config, const MultiConfig::Service & _service_config, const MultiConfig::Thermostat & _thermostat_config);
    void set_bus(Tm1638Bus * bus);

    const MultiConfig::UI & get_config() const { return config; }
    const MultiConfig::Service & get_service_config() const { return service_config; }
    const MultiConfig::Thermostat & get_thermostat_config() const { return thermostat_config; }

    void enable_audio(bool _audio_enabled = true)
    {
        audio_enabled = _audio_enabled;
    }

    bool is_audio_enabled() const
    {
        return audio_enabled;
    }

    void enable_thermostat(bool _thermostat_enabled = true)
    {
        thermostat_enabled = _thermostat_enabled;
    }

    bool is_thermostat_enabled() const
    {
        return thermostat_enabled;
    }

    void set_audio_control_data_ext(const AudioControlData &);
    
    bool is_audio_control_data_changed() const 
    {
        return audio_control_data_changed;
    }
    
    void audio_control_data_change_commited() 
    {
        audio_control_data_changed = false;
    }

    const AudioControlData & get_audio_control_data() const
    {
        return audio_control_data;
    }
    
    bool set_temps(const JsonVariant & json);
    bool set_temp_corr(float);

    float get_temp_corr() const { return temp_corr; }
    bool is_temp_corr_set() const { return temp_corr_set; }

    void reset();

    void tick();

    bool poll_buttons();

protected:

    void set_audio_control_data(const AudioControlData &);

    SmartButton::EventType pop_event_with_longpress(Button);
    
    void change_state(State to_state);
    void handle_state();
    void handle_transition_from(State to_state);
    void handle_transition_to(State to_state);   

    void handle_state_idle();
    void handle_state_thermostat();
    void handle_state_audio();
    void handle_state_test();

    void handle_transition_to_idle(State from_state);
    void handle_transition_to_thermostat(State from_state);
    void handle_transition_to_audio(State from_state);
    void handle_transition_to_test(State from_state);

    void handle_transition_from_idle(State to_state);
    void handle_transition_from_thermostat(State to_state);
    void handle_transition_from_audio(State to_state);
    void handle_transition_from_test(State to_state);

    void handle_left_right_button_event(SmartButton::EventType et, Button button);
    void handle_up_down_button_event(SmartButton::EventType et, Button button);    
    void handle_blue_button_event(SmartButton::EventType et);
    void handle_test_button_event(SmartButton::EventType et);

    void display_volume();
    void display_temp_corr();

    void commit_audio_input();

    static int reverse_map_volume_to_0_to_10(int volume_0_to_100);

    MultiConfig::UI config;
    MultiConfig::Service service_config;
    MultiConfig::Thermostat thermostat_config;

    bool audio_enabled;

    AudioControlData::Source selected_audio_source;

    int selected_url;
    int selected_fm;

    int selected_volume_0_to_10;

    static const int volume_0_to_10_mapping[11];

    AudioControlData audio_control_data;
    bool audio_control_data_changed;  // by UI operation

    Tm1638Terminal * terminal;
    
    State state;

    std::vector<std::pair<String, float>> temps;

    bool thermostat_enabled;
    float temp_corr;
    bool temp_corr_set;

    unsigned long last_thermostat_click_millis;
    unsigned long last_poll_buttons_millis;

    Button longpress;
    int longpress_check;
};

const int UISM::volume_0_to_10_mapping[11] = 
{
    0,   // 0
    25,  // 1
    40,  // 2
    50,  // 3
    55,  // 4
    60,  // 5
    65,  // 6
    70,  // 7
    74,  // 8
    80,  // 9
    86   // 10
};

int UISM::reverse_map_volume_to_0_to_10(int volume_0_to_100)
{
    if (volume_0_to_100 < 0)
    {
        volume_0_to_100 = 0;
    }
    else if (volume_0_to_100 > 100)
    {
        volume_0_to_100 = 100;
    }

    int index = -1;
    int diff = 0;

    for (size_t i=0; i<sizeof(volume_0_to_10_mapping)/sizeof(volume_0_to_10_mapping[0]); ++i)
    {
        if (index == -1)
        {
            diff = abs((volume_0_to_10_mapping[i] - volume_0_to_100));
            index = i;
        }
        else
        {
            int new_diff = abs((volume_0_to_10_mapping[i] - volume_0_to_100));

            if (new_diff < diff)
            {
                diff = new_diff;
                index = i;
            }
        }
    }

    return index;
}


UISM::UISM(const MultiConfig::UI & _config, const MultiConfig::Service & _service_config, const MultiConfig::Thermostat & _thermostat_config)
{
    init();

    config = _config;
    service_config = _service_config;
    thermostat_config = _thermostat_config;

    enable_audio(config.audio_enabled);
    enable_thermostat(config.thermostat_enabled);

    selected_url = service_config.get_defined_url_count() == 0 ? -1 : 0;
    selected_fm = service_config.get_defined_fm_freq_count() == 0 ? -1 : 0;
}

UISM::UISM()
{
    init();
}

void UISM::init()
{
    terminal = NULL;

    selected_audio_source = AudioControlData::sNone;
    selected_url = -1;
    selected_fm = -1;
    selected_volume_0_to_10 = 3;

    state = sNone;   
    
    audio_enabled = true;
    audio_control_data_changed = false;

    thermostat_enabled = true;
    temp_corr = 0.0;
    temp_corr_set = false;

    last_thermostat_click_millis = (unsigned long)-1;
    last_poll_buttons_millis = (unsigned long)-1; 
    
    longpress = bNone;
    longpress_check = -1;
}

UISM::~UISM()
{
    if (terminal != NULL)
    {
        delete terminal;
    }
}

void UISM::reconfigure(const MultiConfig::UI & _config, const MultiConfig::Service & _service_config, const MultiConfig::Thermostat & _thermostat_config)
{
    config = _config;
    service_config = _service_config;
    thermostat_config = _thermostat_config;

    int url_count = service_config.get_defined_url_count();

    if (selected_url < 0 || selected_url >= url_count)
    {
        selected_url = url_count == 0 ? -1 : 0;
    }
    
    int fm_freq_count = service_config.get_defined_fm_freq_count();

    if (selected_fm < 0 || selected_fm >= fm_freq_count)
    {
        selected_fm = fm_freq_count == 0 ? -1 : 0;
    }

    if (terminal)
    {
        terminal->set_stb(config.stb);   
    }

    enable_audio(config.audio_enabled);
    enable_thermostat(config.thermostat_enabled);
}

void UISM::set_bus(Tm1638Bus * bus)
{
    if (bus == NULL)
    {
        if (terminal)
        {
            delete terminal;
            terminal = NULL;
        }
    }
    else
    {
        if (terminal == NULL)
        {
            terminal = new Tm1638Terminal(*bus, config.stb);
        }
        else
        {
            terminal->set_bus(*bus);
        }

        reset();
    }
}

void UISM::reset()
{
    if (terminal)
    {
        terminal->turn_on();
    }

    change_state(sIdle);
}

void UISM::set_audio_control_data(const AudioControlData & _audio_control_data)
{
    DEBUG("UISM::set_audio_control_data %s", _audio_control_data.as_string().c_str())
    audio_control_data = _audio_control_data;
    audio_control_data_changed = true;
}

void UISM::set_audio_control_data_ext(const AudioControlData & _audio_control_data)
{
    DEBUG("UISM::set_audio_control_data_ext %s", _audio_control_data.as_string().c_str())

    if (!(audio_control_data == _audio_control_data))
    {
        AudioControlData old_audio_control_data = audio_control_data;
        audio_control_data = _audio_control_data;
        
        selected_volume_0_to_10 = reverse_map_volume_to_0_to_10(audio_control_data.volume);

        if (audio_control_data.source == AudioControlData::sNone)
        {
            if (state == sAudio)
            {
                selected_audio_source = AudioControlData::sNone;
                change_state(sIdle);
            }
        }
        else
        {
            selected_audio_source = audio_control_data.source;

            if (audio_control_data.source == AudioControlData::sBt)
            {
            }
            else if (audio_control_data.source == AudioControlData::sWww)
            {
                int url_count = service_config.get_defined_url_count();
                
                if (url_count == 0)
                {
                    selected_url = -1;
                }
                else
                {
                    selected_url = audio_control_data.channel;

                    if (selected_url < 0)
                    {
                        selected_url = 0;
                    }
                    else if (selected_url >= url_count)
                    {
                        selected_url = url_count-1;
                    }
                }                
            }
            else if (audio_control_data.source == AudioControlData::sFm)
            {
                int fm_freq_count = service_config.get_defined_fm_freq_count();
                
                if (fm_freq_count == 0)
                {
                    selected_fm = -1;
                }
                else
                {
                    selected_fm = audio_control_data.channel;

                    if (selected_fm < 0)
                    {
                        selected_fm = 0;
                    }
                    else if (selected_fm >= fm_freq_count)
                    {
                        selected_fm = fm_freq_count-1;
                    }
                }
            }

            if (state == sAudio)
            {
                commit_audio_input();
            }
            else
            {
                change_state(sAudio);
            }
        }
    }
    else
    {
        TRACE("audio_control_data unchanged")
    }

} 

bool UISM::set_temps(const JsonVariant & json)
{    
    TRACE("UISM<%s>::set_temps", config.name.c_str())

    if (json.is<JsonArray>())
    {
        TRACE("json is array")

        temps.clear();
        
        const JsonArray & jsonArray = json.as<JsonArray>();
        auto iterator = jsonArray.begin();

        while(iterator != jsonArray.end())
        {
            const JsonVariant & _json = *iterator;

            if (_json.containsKey("item") && _json.containsKey("temp"))
            {
                String item = _json["item"];
                float temp = _json["temp"];

                TRACE("item name %s temp %f", item.c_str(), temp)

                temps.push_back(std::make_pair(item,temp));
            }

            ++iterator;
        }
    
        handle_state();
        return true;
    }
    
    return false;
}


bool UISM::set_temp_corr(float _temp_corr)
{    
    TRACE("UISM<%s>::set_temp_corr %f", config.name.c_str(), temp_corr);

    if (state != sThermostat)
    {
        temp_corr = _temp_corr;
        temp_corr_set = true;

        if (temp_corr < thermostat_config.temp_corr_min)
        {
            temp_corr = thermostat_config.temp_corr_min;
        }
        else if (temp_corr > thermostat_config.temp_corr_max)
        {
            temp_corr = thermostat_config.temp_corr_max;
        }
        
        TRACE("set temp_corr from REST to %f", temp_corr)
    }
    else
    {
        TRACE("skip setting temp_corr from REST, state=sThermostat (value setting in terminal ongoing)")
    }
    
    return true;
}

void UISM::tick()
{
    terminal->_tableau.tick();
        
    if (state == sThermostat && last_thermostat_click_millis != (unsigned long) -1)
    {
        unsigned long now_millis = millis();

        if (now_millis < last_thermostat_click_millis || (now_millis-last_thermostat_click_millis) >= THERMOSTAT_TIMEOUT * 1000)
        {
            change_state(sIdle);
        }
    }
}

bool UISM::poll_buttons()
{
    unsigned long now_millis = millis();

    if (last_poll_buttons_millis != (unsigned long) -1 && now_millis > last_poll_buttons_millis)
    {
        unsigned long millis_interval = now_millis-last_poll_buttons_millis;
    
        if (millis_interval >= 100)
        {
            DEBUG("poll_buttons interval too long, %ul", millis_interval)
        }
    }

    last_poll_buttons_millis = now_millis;
    
    terminal->poll_buttons();
         
    SmartButton::EventType et = pop_event_with_longpress(bLeft);
    
    if (et != SmartButton::etNone)
    {
        handle_left_right_button_event(et, bLeft);
        return true;
    }

    et = pop_event_with_longpress(bRight);
    
    if (et != SmartButton::etNone)
    {
        handle_left_right_button_event(et, bRight);
        return true;
    }

    et = pop_event_with_longpress(bUp);
    
    if (et != SmartButton::etNone)
    {
        handle_up_down_button_event(et, bUp);
        return true;
    }

    et = pop_event_with_longpress(bDown);
    
    if (et != SmartButton::etNone)
    {
        handle_up_down_button_event(et, bDown);
        return true;
    }

    et = pop_event_with_longpress(bBlue);
    
    if (et != SmartButton::etNone)
    {
        handle_blue_button_event(et);
        return true;
    }

    et = pop_event_with_longpress(bTest);
    
    if (et != SmartButton::etNone)
    {
        handle_test_button_event(et);
        return true;
    }

    return false;

    /*
    for i in range(len(self._terminal._buttons)):
        button = self._terminal._buttons[i] 
        event = button.pop_last_event()
        
        if event != None:
            print("button " + str(self._button_index_2_name[i]) + ", event  " + button.event_2_str(event))
    */
}

SmartButton::EventType UISM::pop_event_with_longpress(Button button)
{         
    if (terminal != NULL)
    {    
        int button_index = (int) button;
        SmartButton::EventType event = terminal->buttons_pop_last_event(button_index);

        if (longpress != bNone)
        {
            if (longpress == button)
            {
                if (event == SmartButton::etUnpressed)
                {
                    event = SmartButton::etNone;
                    longpress_check = -1;
                    longpress = bNone;
                }   
                else
                {                
                    if (longpress_check > 0)
                    {
                        longpress_check -= 1;
                        event = SmartButton::etNone;
                    }
                    else
                    {    
                        longpress_check = LONGPRESS_PERFORATION;
                        event = SmartButton::etPressed;
                    }
                }
            }
        }
        else
        {
            if (event == SmartButton::etLongpress)
            {
                longpress = button;

                longpress_check = LONGPRESS_PERFORATION;
                event = SmartButton::etPressed;
            }
        }   
        
        return event;     
    }
    
    return SmartButton::etNone;    
}

void UISM::change_state(State to_state)
{    
    if (to_state == state)
    {
        handle_state();
    }
    else
    {
        handle_transition_from(state);
        handle_transition_to(to_state);   
        state = to_state;            
    }
}

void UISM::handle_state()
{    
    if (state == sIdle)
    {
        handle_state_idle();
    }
    else
    if (state == sAudio)
    {
        // handle_state_audio();
    }
    else
    if (state == sTest)
    {
        handle_state_test();
    }
}

void UISM::handle_transition_from(State to_state)
{    
    if (state == sIdle)
    {
        handle_transition_from_idle(to_state);
    }
    else
    if (state == sThermostat)
    {
        handle_transition_from_thermostat(to_state);
    }
    else
    if (state == sAudio)
    {
        handle_transition_from_audio(to_state);
    }
    else
    if (state == sTest)
    {
        handle_transition_from_test(to_state);
    }
}

void UISM::handle_transition_to(State to_state)
{    
    if (to_state == sIdle)
    {
        handle_transition_to_idle(state);
    }
    else
    if (to_state == sThermostat)
    {
        handle_transition_to_thermostat(state);
    }
    else
    if (to_state == sAudio)
    {
        handle_transition_to_audio(state);
    }
    else
    if (to_state == sTest)
    {
        handle_transition_to_test(state);
    }
}

void UISM::handle_state_idle()
{
    if (terminal)
    {
        //terminal->_tableau.clear();

        TRACE("adding standby tableau items")

        if (temps.size() > 0)
        {
            for (auto it=temps.begin(); it!=temps.end(); ++it)
            {
                char buf[16];
                sprintf(buf, "%.1f", it->second);
                String item = it->first + " " + buf + "*C";
                
                TRACE("%s display %s", config.name.c_str(), item.c_str())
                terminal->_tableau.set_standby_item(std::make_pair(item, std::make_pair(Tm1638Tableau::sRepetitive, 0)));
            }
        }
        else
        {
            terminal->_tableau.set_standby_item(std::make_pair(String("welcome ") + config.name, std::make_pair(Tm1638Tableau::sRepetitive, 0)));
        }

        terminal->_tableau.enable_standby_2_active();
    }
}

void UISM::handle_state_thermostat()
{
}

void UISM::handle_state_audio()
{
    commit_audio_input();
}

void UISM::handle_state_test()
{
}

void UISM::handle_transition_to_idle(State from_state)
{
    handle_state_idle();
}

void UISM::handle_transition_to_thermostat(State from_state)
{
    if (terminal)
    {
        terminal->_tableau.clear();
    }

    handle_state_thermostat();
}

void UISM::handle_transition_to_audio(State from_state)
{
    DEBUG("UISM transition to audio")

    if (terminal)
    {
        terminal->set_led(bBlue, true);
        terminal->_tableau.clear();

        terminal->_tableau.set_item(std::make_pair("audio on", std::make_pair(Tm1638Tableau::sRepetitive, 0)), 0);
        commit_audio_input();
    }
}

void UISM::handle_transition_to_test(State from_state)
{

}

void UISM::handle_transition_from_idle(State to_state)
{

}

void UISM::handle_transition_from_thermostat(State to_state)
{
    last_thermostat_click_millis = (unsigned long)-1;
}

void UISM::handle_transition_from_audio(State to_state)
{
    DEBUG("UISM transition from audio")

    if (terminal)
    {
        terminal->set_led(bBlue, false);
        terminal->_tableau.clear();

        commit_audio_input();
    }
}

void UISM::handle_transition_from_test(State to_state)
{

}

void UISM::handle_left_right_button_event(SmartButton::EventType et, Button button)
{
    DEBUG("handle_left_right_button_event et=%s button=%s", SmartButton::event_type_2_str(et), button_2_str(button))

    if (et == SmartButton::etPressed)
    {
        if (state == sAudio)
        {
            if (selected_audio_source == AudioControlData::sWww)
            {
                int old_selected_url = selected_url;
                int url_count = service_config.get_defined_url_count();
                
                if (url_count > 1)
                {
                    if (button == bLeft)
                    {
                        if (selected_url > 0)
                        {
                            selected_url--;
                        }
                    }
                    else
                    if (button == bRight)
                    {
                        if (selected_url < url_count-1)
                        {
                            selected_url++;
                        }
                    }
                }

                DEBUG("www channel step from %d to %d", (int) old_selected_url, (int) selected_url)
            }
            else
            if (selected_audio_source == AudioControlData::sFm)
            {
                int old_selected_fm = selected_fm;
                int fm_freq_count = service_config.get_defined_fm_freq_count();
                
                if (fm_freq_count > 1)
                {
                    if (button == bLeft)
                    {
                        if (selected_fm > 0)
                        {
                            selected_fm--;
                        }
                    }
                    else
                    if (button == bRight)
                    {
                        if (selected_fm < fm_freq_count-1)
                        {
                            selected_fm++;
                        }
                    }
                }
                DEBUG("fm channel step from %d to %d", (int) old_selected_fm, (int) selected_fm)
            }

            commit_audio_input();
        }
    }
}

void UISM::handle_up_down_button_event(SmartButton::EventType et, Button button)
{
    DEBUG("handle_up_down_button_event et=%s button=%s", SmartButton::event_type_2_str(et), button_2_str(button))

    bool first_thermostat_click = false;

    if (state == sIdle)
    {
        if (thermostat_enabled)
        {
            change_state(sThermostat);
            first_thermostat_click = true;
        }           
    }
    
    if (state == sThermostat)
    {   
        if (et == SmartButton::etPressed)
        {
            last_thermostat_click_millis = millis();

            // logfile.add_string(datetime.now().strftime("%Y-%m-%d %H:%M:%S") + " " + str(self._audio_name) + " " + button[1] + " button pressed")
        
            if (first_thermostat_click == false)
            {
                if (button == bUp)
                {
                    if (temp_corr < thermostat_config.temp_corr_max)
                    {
                        temp_corr += thermostat_config.temp_corr_step;
                    }
                }
                else
                {
                    if (temp_corr > thermostat_config.temp_corr_min)
                    {
                        temp_corr -= thermostat_config.temp_corr_step;
                    }
                }
            
                temp_corr_set = true;
            }
            
            // at first click up/down from idle state show current value, no step
            
            display_temp_corr();
                                
            DEBUG("temp_corr set to %.1f",  temp_corr)
        }
    }
    else if (state == sAudio)
    {
        if (et == SmartButton::etPressed)
        {
            // logfile.add_string(datetime.now().strftime("%Y-%m-%d %H:%M:%S") + " " + str(self._audio_name) + " " + button[1] + " button pressed")
        
            if (audio_enabled)
            {
                int old_volume_0_to_10 = selected_volume_0_to_10;

                if (button == bUp)
                {
                    if (selected_volume_0_to_10 < sizeof(volume_0_to_10_mapping)/sizeof(volume_0_to_10_mapping[0])-1)
                    {
                        selected_volume_0_to_10++;
                    }
                }
                else
                {
                    if (selected_volume_0_to_10 > 0)
                    {
                        selected_volume_0_to_10--;
                    }
                }
                
                if (old_volume_0_to_10 != selected_volume_0_to_10)
                {
                    audio_control_data.volume = volume_0_to_10_mapping[selected_volume_0_to_10];
                    set_audio_control_data(audio_control_data);
                }

                display_volume();

                DEBUG("volume set to %d", (int) selected_volume_0_to_10)
            }
        }
    }                    
}

void UISM::handle_blue_button_event(SmartButton::EventType et)
{
    DEBUG("handle_blue_button_event et=%s", SmartButton::event_type_2_str(et))

    if (et == SmartButton::etPressed)
    {
        if (audio_enabled == false)
        {
            terminal->_tableau.set_item(std::make_pair("no audio", std::make_pair(Tm1638Tableau::sShot, -1)));
        }
        else
        {
            selected_audio_source = AudioControlData::next_source(selected_audio_source);
            DEBUG("changing audio source to %s", AudioControlData::source_2_str(selected_audio_source))

            if (state != sAudio)
            {
                change_state(sAudio);
            }
            else
            {
                if (selected_audio_source == AudioControlData::sNone)
                {
                    change_state(sIdle);
                }
                else
                {
                    handle_state_audio();
                }
            }
        }  
    }
}

void UISM::handle_test_button_event(SmartButton::EventType et)
{
    DEBUG("handle_test_button_event et=%s", SmartButton::event_type_2_str(et))
    /*
        if event == ui.SmartButton.EVENT_PRESSED:

            #logfile.add_string(datetime.now().strftime("%Y-%m-%d %H:%M:%S") + " " + str(self._audio_name) + " test button pressed")

            if self._state != self.sTest:
                self._change_state(self.sTest)
            else:    
                self._handle_state_test()
*/
}

void UISM::display_temp_corr()
{
    char buf[64];
    sprintf(buf, "cr %.1f*C", temp_corr);
    terminal->_tableau.clear();
    terminal->_tableau.set_item(std::make_pair(buf, std::make_pair(Tm1638Tableau::sShot, THERMOSTAT_TIMEOUT)));
}

void UISM::display_volume()
{
    char buf[64];
    sprintf(buf, "vol %d", (int) selected_volume_0_to_10);
    terminal->_tableau.set_item(std::make_pair(buf, std::make_pair(Tm1638Tableau::sShot, 2)), 2);

}

void UISM::commit_audio_input()
{
    AudioControlData new_audio_control_data = audio_control_data;

    if (selected_audio_source != AudioControlData::sNone)    
    {
        String tableau_string = AudioControlData::source_2_str(selected_audio_source);
        
        if (selected_audio_source == AudioControlData::sWww)
        {
            int url_count = service_config.get_defined_url_count();

            if (selected_url >= 0 && selected_url < url_count)
            {
                char buf[32];
                sprintf(buf, "-%d ", selected_url+1);
                tableau_string += buf + service_config.url[selected_url].name;
            }
            new_audio_control_data.source = AudioControlData::sWww;
            new_audio_control_data.channel = selected_url;
        }
        else if (selected_audio_source == AudioControlData::sFm)
        {
            int fm_freq_count = service_config.get_defined_fm_freq_count();

            if (selected_fm >= 0 && selected_fm < fm_freq_count)
            {
                char buf[32];
                sprintf(buf, " %.1f ", service_config.fm_freq[selected_fm].value);
                tableau_string += buf + service_config.fm_freq[selected_fm].name;
            }
            new_audio_control_data.source = AudioControlData::sFm;
            new_audio_control_data.channel = selected_fm;
        }
        else if (selected_audio_source == AudioControlData::sBt)
        {
            new_audio_control_data.source = AudioControlData::sBt;
            new_audio_control_data.channel = 0;
        }
        
        if (terminal)
        {
            terminal->_tableau.set_item(std::make_pair(tableau_string, std::make_pair(Tm1638Tableau::sRepetitive, 0)), 1);
        }
    }
    else
    {
        new_audio_control_data.source = AudioControlData::sNone;
    }

    new_audio_control_data.volume = volume_0_to_10_mapping[selected_volume_0_to_10];
    set_audio_control_data(new_audio_control_data);
}


bool multi_handler_uart_command_func(const String & command, AtResponse & response, String * error);
String multi_uart_command(const String & command, String & response, String * error);

class MultiHandler
{
public:

    static const unsigned LOG_FOR_STATS_INTERVAL_SECONDS = 60;
    static const int UART_BUFFER_SIZE = 1024;
    static const size_t UART_READ_WAIT_MS = 10;
    static const size_t UART_POLL_INTERVAL_MS = 1000;

    MultiHandler()
    {
        _is_active = false;
        _task_finished = true;
        _audio_task_finished = true;
        _i2c_scan_task_finished = true;
        _ui_task_finished = true;
        _should_reconnect_www = false;
        _reconnect_count = 0;

        #ifdef USE_HARDWARE_SERIAL
        hardware_serial = NULL;
        #else
        uart_setup_ok = false;
        #endif

        fsc_bt = NULL;

        audio_engine = NULL;

        two_wire = &Wire;
        tda8425 = NULL;
        rda5807 = NULL;
        tm1638_bus = NULL;
        uism = NULL;
    }

    ~MultiHandler()
    {
        #ifdef USE_HARDWARE_SERIAL
        delete hardware_serial;
        hardware_serial = NULL;
        #endif

        delete fsc_bt;
        fsc_bt = NULL;

        delete tda8425;
        tda8425 = NULL;

        delete rda5807;
        rda5807 = NULL;

        delete audio_engine;
        audio_engine = NULL;

        delete tm1638_bus;
        tm1638_bus = NULL;

        delete uism;
        uism = NULL;
    }

    bool is_active() const { return _is_active; }

    void start(const MultiConfig &config);
    void stop();
    void reconfigure(const MultiConfig &config);

    MultiStatus get_status()
    {
        MultiStatus _status;

        {
            Lock lock(semaphore);
            _status = status;
        }

        return _status;
    }

    bool audio_control_ext(const String & source, const String & channel, const String & volume, String & response, String * error = NULL);
    bool set_volatile(const JsonVariant & json, String * error = NULL);

protected:
    void configure_uart();
    void configure_i2c();
    void configure_bt();
    void configure_fm();
    void configure_audio_engine();
    void configure_sound();
    void configure_tm1638_bus();
    void configure_ui();

    void set_mute(bool);

    bool uart_command(const String & command, AtResponse & response, String * error = NULL);
    bool uart_poll_indications(AtResponse & response, String * error = NULL);

    static void task(void *parameter);
    static void audio_task(void *parameter);
    static void i2c_scan_task(void *parameter);
    static void ui_task(void *parameter);

    bool audio_control(const String & source, const String & channel, const String & volume, String & response, String * error = NULL);
    bool audio_control(const AudioControlData &);

    String choose_url();
    float choose_fm_freq();
    uint8_t get_max_volume(bool trace=false) const;
    uint8_t get_volume() const;

    void commit_volume();
    void commit_fm_freq();

    BinarySemaphore semaphore;
    BinarySemaphore new_delete_semaphore;
    BinarySemaphore uart_semaphore;

    MultiConfig config;
    MultiStatus status;
    bool _should_reconnect_www;
    int _reconnect_count;
    bool _is_active;
    bool _task_finished;
    bool _audio_task_finished;
    bool _i2c_scan_task_finished;
    bool _ui_task_finished;

    Audio * audio_engine;
    
    #ifdef USE_HARDWARE_SERIAL
    HardwareSerial * hardware_serial;
    #else
    bool uart_setup_ok;
    #endif 

    FscBt * fsc_bt;
    AtLatestIndications at_latest_indications;

    TwoWire * two_wire;

    Tda8425 * tda8425;

    Rda5807 * rda5807;

    Tm1638Bus * tm1638_bus;
    UISM * uism;

    friend void audio_showstreamtitle(const char *info);
    friend void audio_bitrate(const char *info);
    friend bool multi_handler_uart_command_func(const String & command, AtResponse & response, String * error);
    friend String multi_uart_command(const String & command, String & response);
};


static MultiHandler handler;

static String esp_err_2_string(esp_err_t esp_r)
{
    char buf[128];
    esp_err_to_name_r(esp_r, buf, sizeof(buf));
    return String("ERR=") + String((int) esp_r) + ", " + buf;
}

void audio_showstreamtitle(const char *info)
{
    Lock lock(handler.semaphore);
    handler.status.title = info;
}

void audio_bitrate(const char *info)
{
    uint32_t bitrate = atol(info);
    Lock lock(handler.semaphore);
    handler.status.www.bitrate = bitrate;
}

bool multi_handler_uart_command_func(const String & command, AtResponse & response, String * error = NULL)
{
    return handler.uart_command(command, response, error);
}

void MultiHandler::start(const MultiConfig &_config)
{
    if (_is_active)
    {
        return; // already running
    }

    while(_task_finished == false  || _audio_task_finished == false || _i2c_scan_task_finished == false || _ui_task_finished == false)
    {
        delay(100);
    }

    Lock lock(semaphore);
    config = _config;
    configure_uart();
    configure_i2c();
    configure_bt();
    configure_fm();
    configure_audio_engine();
    configure_sound();
    configure_tm1638_bus();
    configure_ui();

    _is_active = true;
    _task_finished = false;
    _audio_task_finished = false;
    _i2c_scan_task_finished = false;
    _ui_task_finished = false;

    xTaskCreate(
        task,                // Function that should be called
        "multi_task",        // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
    xTaskCreate(
        audio_task,          // Function that should be called
        "multi_task-audio",  // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
    xTaskCreate(
        i2c_scan_task,          // Function that should be called
        "multi_task-i2c_scan",  // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
    xTaskCreate(
        ui_task,          // Function that should be called
        "multi_task-ui",  // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void MultiHandler::stop()
{
    _is_active = false;

    while(_task_finished == false || _audio_task_finished == false || _i2c_scan_task_finished == false || _ui_task_finished == false)
    {
        delay(100);
    }
}

void MultiHandler::reconfigure(const MultiConfig &_config)
{
    TRACE("multi_task: reconfigure enters")

    MultiConfig current_config;

    {Lock lock(semaphore);
    current_config = config;}

    bool should_configure_uart = false;
    bool should_configure_i2c = false;
    bool should_configure_bt = false;
    bool should_configure_fm = false;
    bool should_configure_audio_engine = false;
    bool should_configure_sound = false;
    bool should_configure_tm1638_bus = false;
    bool should_configure_ui = false;
    bool should_commit_fm_freq = false;
    bool will_reconnect_www = false;

    if (!(current_config.uart == _config.uart))
    {
        TRACE("multi_task: uart config changed")
        should_configure_uart = true;
    }

    if (!(current_config.bt == _config.bt))
    {
        TRACE("multi_task: bt config changed")
        should_configure_bt = true;
    }

    if (!(current_config.fm == _config.fm))
    {
        TRACE("multi_task: fm config changed")
        should_configure_fm = true;
    }

    if (!(current_config.i2s == _config.i2s))
    {
        TRACE("multi_task: i2s config changed")
        should_configure_audio_engine = true;
    }

    if (!(current_config.i2c == _config.i2c))
    {
        TRACE("multi_task: i2c config changed")
        should_configure_i2c = true;
    }

    if (!(current_config.sound == _config.sound))
    {
        TRACE("multi_task: sound config changed")
        should_configure_sound = true;
    }

    if (!(current_config.service == _config.service))
    {
        TRACE("multi_task: service config changed")
        will_reconnect_www = true;
        should_commit_fm_freq = true;
        should_configure_ui = true;
    }

    if (!(current_config.tm1638 == _config.tm1638))
    {
        TRACE("multi_task: tm1638 config changed")
        should_configure_tm1638_bus = true;
    }

    if (!(current_config.thermostat == _config.thermostat))
    {
        TRACE("multi_task: thermostat config changed")
        should_configure_ui = true;
    }

    if (!(current_config.ui == _config.ui))
    {
        TRACE("multi_task: UI config changed")
        should_configure_ui = true;
    }

    if (!(current_config == _config))
    {
        TRACE("multi_task: config changed")
        Lock lock(semaphore);
        config = _config;
    }

    if (should_configure_uart == true)
    {
        configure_uart();
    }

    if (should_configure_i2c == true)
    {
        configure_i2c();
    }

    if (should_configure_bt == true)
    {
        configure_bt();
    }

    if (should_configure_fm == true)
    {
        configure_fm();
    }

    if (should_configure_audio_engine == true)
    {
        configure_audio_engine();
        will_reconnect_www = true;
    }

    if (should_commit_fm_freq == true)
    {
        commit_fm_freq();
    }

    if (should_configure_sound == true)
    {
        configure_sound();
    }

    if (should_configure_tm1638_bus == true)
    {
        configure_tm1638_bus();
    }

    if (should_configure_ui == true)
    {
        configure_ui();
    }

    if (will_reconnect_www)
    {
        _should_reconnect_www = true;
    }

    TRACE("multi_task: reconfigure leaves")
}

void MultiHandler::task(void *parameter)
{
    MultiHandler *_this = (MultiHandler *)parameter;

    TRACE("multi_task: started")
    
    uint32_t now_millis = millis();
    uint32_t task_start_millis = now_millis;
    uint32_t wait_retry_audio_millis = 0;
    AudioControlData::Source source = AudioControlData::sNone;

    bool should_log_for_stats = true;
    uint32_t last_log_for_stats_millis = 0;

    uint32_t last_check_hour_shift_millis = 0;
    const uint32_t CHECK_HOUR_SHIFT_INTERVAL_MILLIS = 60000;

    uint32_t last_uart_poll_millis = 0;

    const size_t BT_SETUP_DELAY_MS = 8000;
    bool bt_setup_done = false;

    int hour = -1;

    while (_this->_is_active)
    {
        now_millis = millis();

        if (bt_setup_done == false)
        {
            // there is no technical reason for this delay other than to be able to see log during startup

            if ((now_millis > task_start_millis && now_millis - task_start_millis >= BT_SETUP_DELAY_MS) ||
                (now_millis < task_start_millis && now_millis + (0xffffffff - task_start_millis) >= BT_SETUP_DELAY_MS))
            {
                {   //DEBUG("new_delete_semaphore 1")
                    Lock lock(_this->new_delete_semaphore);

                if (_this->fsc_bt)
                {
                    bool r = _this->fsc_bt->module_setup();

                    if (r == false)
                    {
                        ERROR("BT module setup failed")
                    }
                    else
                    {
                        TRACE("BT module setup OK")
                    }
                }
                else
                {
                    ERROR("BT module setup failed, fsc_bt is not allocated")
                }}
            
                bt_setup_done = true;
            }
        }

        if (last_check_hour_shift_millis > now_millis || (now_millis-last_check_hour_shift_millis) >= CHECK_HOUR_SHIFT_INTERVAL_MILLIS)
        {
            last_check_hour_shift_millis = now_millis;

            // check if we need to limit max volume at hour shift

            time_t _time_t;
            time(& _time_t);
            tm _tm = {0};
            _tm = *localtime(&_time_t);
        
            if (_tm.tm_year+1900 > 2000) // otherwise - NTP failed to fetch time 
            {
                if (_tm.tm_hour != hour)
                {
                    hour = _tm.tm_hour;
                    _this->commit_volume();
                }
            }
        }

        if (last_uart_poll_millis > now_millis || (now_millis-last_uart_poll_millis) >= _this->UART_POLL_INTERVAL_MS)
        {
            last_uart_poll_millis = now_millis;
            //DEBUG("uart_poll interval")

            AtResponse dummy;
            _this->uart_poll_indications(dummy);  // this will update at_poll_indications

            Lock lock(_this->semaphore);
            _this->status.bt.latest_indications = _this->at_latest_indications.indications; 
            //DEBUG("_this->status.bt.latest_indications.size() %d", (int) _this->status.bt.latest_indications.size())

            // TODO: update BT title, etc.
        }

        should_log_for_stats = false;

        AudioControlData::Source last_source = source;

        source = _this->status.audio_control_data.source;

        if (last_source != source)
        {
            should_log_for_stats = true;
            //DEBUG("should_log_for_stats: streaming_on")

            _this->status.www.is_streaming = _this->status.audio_control_data.source == AudioControlData::sWww;
            _this->status.fm.is_streaming = _this->status.audio_control_data.source == AudioControlData::sFm;
        }

        if (last_log_for_stats_millis > now_millis || ((now_millis-last_log_for_stats_millis)/1000) >= _this->LOG_FOR_STATS_INTERVAL_SECONDS)
        {
            should_log_for_stats = true;
            last_log_for_stats_millis = now_millis;
            //DEBUG("should_log_for_stats: interval")
        }

        // log for stats if something has changed or every LOG_FOR_STATS_INTERVAL_SECONDS seconds

        if (should_log_for_stats == true)
        {
            MultiStatus status_copy;

            { Lock lock(_this->semaphore);
            status_copy = _this->status; }
            
            String www_title;
            
            if  (status_copy.www.is_streaming)
            {
                www_title = status_copy.title;
            }
            
            TRACE("* \"www\":{\"is_streaming\":%d, \"url_index\":\"%d\", \"url_name\":\"%s\", \"bitrate\":\"%d\"}, \"commited_volume\":%d, \"title\":\"%s\"",
                   (int)status_copy.www.is_streaming, status_copy.www.url_index, status_copy.www.url_name.c_str(), status_copy.www.bitrate, 
                   (int)status_copy.commited_volume, www_title.c_str())

            TRACE("* \"fm\":{\"is_streaming\":%d, \"index\":\"%d\", \"name\":\"%s\", \"freq\":%f}, \"commited_volume\":%d, \"title\":\"%s\"",
                    (int)status_copy.fm.is_streaming, status_copy.fm.index, status_copy.fm.name.c_str(), status_copy.fm.freq, 
                    (int)status_copy.commited_volume, "")

            // TODO: BT        
         }

        delay(100);            
    }

    _this->_task_finished = true;
    TRACE("multi_task: finished")
    vTaskDelete(NULL);
}

void MultiHandler::audio_task(void *parameter)
{
    MultiHandler *_this = (MultiHandler *)parameter;

    TRACE("audio_task: started")
    
    esp_task_wdt_deinit();
    esp_task_wdt_init(30000, true);

    uint32_t now_millis = millis();
    uint32_t wait_retry_audio_millis = 0;
    bool streaming_on = false;

    while (_this->_is_active)
    {
        now_millis = millis();
        bool last_streaming_on = streaming_on;

        streaming_on = _this->status.audio_control_data.source == AudioControlData::sWww;

        if (last_streaming_on != streaming_on)
        {
            TRACE("streaming_on %d", (int) streaming_on)

            if (streaming_on == false)
            {
                Lock lock(_this->semaphore);
                _this->status.www.bitrate = 0;
                _this->status.title = "";
            }
        }

        if (_this->_should_reconnect_www == true)
        {
            TRACE("audio_task: requested reconnecting stream")
            //DEBUG("new_delete_semaphore 2")
            Lock lock(_this->new_delete_semaphore);
            
            _this->_should_reconnect_www = false;
            _this->_reconnect_count = 0;

            if (_this->audio_engine && _this->audio_engine->isRunning() == true)
            {
                _this->audio_engine->stopSong();
                TRACE("audio_task: stream disconnected (off)")
            }
        }

        // ATTENTION: no delay while semaphore is taken due to risk of deadlocks

        if (_this->_reconnect_count < AUDIO_MAX_RECONNECT_ATTEMPTS && (wait_retry_audio_millis == 0 || 
            wait_retry_audio_millis+AUDIO_RECONNECT_TIMEOUT <= now_millis || now_millis < wait_retry_audio_millis))
        {
            if (_this->audio_engine != NULL)
            {
                // DEBUG("new_delete_semaphore 3")
                if (streaming_on == true)
                {
                    {Lock lock(_this->new_delete_semaphore);
                    if (_this->audio_engine)
                    {
                        if (_this->audio_engine->isRunning() == false)
                        {
                            DEBUG("audio_task: chosing url")
                            //esp_log_level_set("*", ESP_LOG_ERROR);  
                            String url = _this->choose_url();
                            bool connect_ok = _this->audio_engine->connecttohost(url.c_str());
                            _this->audio_engine->setVolume(100);

                            TRACE("audio_task: connection attempt %d/%d to URL %s, result %d", (int) _this->_reconnect_count+1, (int) AUDIO_MAX_RECONNECT_ATTEMPTS, 
                                  url.c_str(), (int) connect_ok)

                            if (connect_ok == false)
                            {
                                wait_retry_audio_millis = now_millis;
                                _this->_reconnect_count++;

                                if (_this->_reconnect_count >= AUDIO_MAX_RECONNECT_ATTEMPTS)
                                {
                                    TRACE("audio_task: maximum connection retries reached %d", (int) AUDIO_MAX_RECONNECT_ATTEMPTS)
                                }
                                else
                                {
                                    TRACE("audio_task: will retry connection in %d millis (now_millis %d) ", (int) AUDIO_RECONNECT_TIMEOUT, (int) now_millis)
                                }
                            }
                            else
                            {
                                wait_retry_audio_millis = 0;  
                                _this->_reconnect_count = 0;
                                TRACE("audio_task: stream connected (on)")
                            }
                        }
                        else
                        {
                            _this->audio_engine->loop();
                        }
                    }}
                    delay(0);
                }
                else
                {
                    {Lock lock(_this->new_delete_semaphore);
                    if (_this->audio_engine && _this->audio_engine->isRunning() == true)
                    {
                        TRACE("audio_task: disconnecting stream")
                        _this->audio_engine->stopSong();
                        TRACE("audio_task: stream disconnected (off)")
                    }}
                    delay(100);
                }
            }
            else
            {
                delay(100);
            }
        }
        else
        {
            delay(100);
        }    
    }

    if (_this->audio_engine != NULL)
    {
        //DEBUG("new_delete_semaphore 4")
        Lock lock(_this->new_delete_semaphore);

        if (_this->audio_engine && _this->audio_engine->isRunning() == true)
        {
            TRACE("audio_task: disconnecting stream")
            _this->audio_engine->stopSong();
            TRACE("audio_task: stream disconnected (off)")
        }
    }

    _this->_audio_task_finished = true;
    TRACE("audio_task: finished")
    vTaskDelete(NULL);
}

void MultiHandler::i2c_scan_task(void *parameter)
{
    MultiHandler *_this = (MultiHandler *)parameter;

    TRACE("i2c_scan_task: started")
    
    uint32_t now_millis = millis();

    uint32_t last_i2c_scan_millis = 0;
    const uint32_t I2C_SCAN_INTERVAL_MILLIS = 600000; // 10 minutes

    while (_this->_is_active)
    {
        now_millis = millis();

        if (last_i2c_scan_millis > now_millis || (now_millis-last_i2c_scan_millis) >= I2C_SCAN_INTERVAL_MILLIS)
        {
            last_i2c_scan_millis = now_millis;
            
            int n_devices = 0;
            TRACE("Scanning I2C...")

            for(int address = 1; address < 127; address++ )
            {
                n_devices += i2c_scan(*(_this->two_wire), address) ? 1 : 0;
            }

            if (n_devices == 0)
            {
                TRACE("No I2C devices found")
            }
            else
            {
                TRACE("done")
            }
            
            delay(0);
        }

        delay(100);            
    }

    _this->_i2c_scan_task_finished = true;
    TRACE("multi_task: i2c_scan_task finished")
    vTaskDelete(NULL);
}

void MultiHandler::ui_task(void *parameter)
{
    MultiHandler *_this = (MultiHandler *)parameter;
    
    TRACE("ui_task: started")
    
    uint32_t now_millis = millis();

    const int DEFAULT_SHORT_LOOP_DELAY = 50;
    int short_loop_delay = DEFAULT_SHORT_LOOP_DELAY;

    int display_shift_ratio = 4;
    int poll_button_ratio = 1;

    size_t i=0;
    size_t j=0;

    while (_this->_is_active)
    {
        uint32_t time_in = millis();

        if ((i % display_shift_ratio) == 0)
        {
            // DISPLAY

            if (_this->uism != NULL)
            {
                Lock lock(_this->new_delete_semaphore);
                
                if (_this->uism)
                {
                    _this->uism->tick();

                    if (_this->uism->is_audio_control_data_changed())
                    {
                        AudioControlData new_audio_control_data = _this->uism->get_audio_control_data();
                        _this->audio_control(new_audio_control_data);

                        _this->uism->audio_control_data_change_commited();
                    }
                
                    _this->status.ui.temp_corr = _this->uism->get_temp_corr();
                    _this->status.ui.temp_corr_set = _this->uism->is_temp_corr_set();
                }
            }
        }

        if ((j % poll_button_ratio) == 0)
        {
            // POLL BUTTONs

            if (_this->uism != NULL)
            {
                Lock lock(_this->new_delete_semaphore);
                
                if (_this->uism)
                {
                    _this->uism->poll_buttons();
                }
            }
        }

        i++;
        j++;

        delay(short_loop_delay);            

        uint32_t time_out = millis();

        int current_interval = time_out > time_in ? time_out - time_in : DEFAULT_SHORT_LOOP_DELAY;        
        short_loop_delay = calc_loop_delay(current_interval, short_loop_delay, DEFAULT_SHORT_LOOP_DELAY);
    }

    _this->_ui_task_finished = true;
    TRACE("multi_task: ui_task finished")
    vTaskDelete(NULL);
}

bool MultiHandler::audio_control_ext(const String & source, const String & channel, const String & volume, String & response, String * error)
{
    TRACE("audio_control_ext enters")
    bool r = audio_control(source, channel, volume, response, error);

    Lock lock(new_delete_semaphore);

    if (uism != NULL)
    {
        uism->set_audio_control_data_ext(status.audio_control_data);
    }

    return r;
}

bool MultiHandler::audio_control(const String & source, const String & channel, const String & volume, String & response, String * error)
{
    TRACE("audio_control1 enters")

    AudioControlData new_audio_control_data;
    
    { Lock lock(semaphore);
    new_audio_control_data = status.audio_control_data; }

    DEBUG("audio_control copied data")

    if (!source.isEmpty())
    {
        new_audio_control_data.source = AudioControlData::str_2_source(source);

        if (new_audio_control_data.source == (AudioControlData::Source)-1)
        {
            char buf[80];
            sprintf(buf, "source %s is invalid", source.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
            return false;
        }
    }

    DEBUG("audio_control checked source")

    if (!channel.isEmpty())
    {
        new_audio_control_data.channel = (uint8_t) channel.toInt();
    }

    DEBUG("audio_control processed channel")

    if (!volume.isEmpty())
    {
        new_audio_control_data.volume = (uint8_t) volume.toInt();

        if (new_audio_control_data.volume > 100)
        {
            new_audio_control_data.volume = 100;
        }
    }

    DEBUG("audio_control processed volume")

    if (new_audio_control_data.source != status.audio_control_data.source)
    {
        TRACE("audio_control: changing source to %s", source)

        if (status.audio_control_data.source == AudioControlData::sBt) // changing from Bt
        {
            //DEBUG("new_delete_semaphore 5")
            Lock lock(new_delete_semaphore);
            //DEBUG("has taken new_delete_semaphore")

            if (fsc_bt != NULL)
            {
                fsc_bt->module_enable_bt(false);
                fsc_bt->module_enable_i2s(false);
            }
            DEBUG("switching off bt done")
        }
        else if (status.audio_control_data.source == AudioControlData::sWww) // changing from Www
        {
            //DEBUG("new_delete_semaphore 6")
            Lock lock(new_delete_semaphore);

            if (audio_engine)
            {
                delete audio_engine;
                audio_engine = NULL;
            }

            configure_audio_engine(); // make sure to put all GPIOs in tri-state
        }
        else if (status.audio_control_data.source == AudioControlData::sFm) // changing from FM
        {
            //DEBUG("new_delete_semaphore 7")
            Lock lock(new_delete_semaphore);

            if (rda5807)
            {
                rda5807->set_mute(true);
            }
        }

        if (new_audio_control_data.source == AudioControlData::sBt) // changing to Bt
        {
            //DEBUG("new_delete_semaphore 8")
            Lock lock(new_delete_semaphore);

            if (fsc_bt != NULL)
            {
                fsc_bt->module_enable_bt(true);
                fsc_bt->module_enable_i2s(true);
            }

            if (tda8425)
            {
                tda8425->set_input(0);
            }
        }
        else if (new_audio_control_data.source == AudioControlData::sWww) // changing to Www
        {
            {   //DEBUG("new_delete_semaphore 9")
                Lock lock(new_delete_semaphore);
            //DEBUG("free heap before audio_engine alloc %ul", (long)ESP.getFreeHeap())

            if (audio_engine)
            {
                delete audio_engine;
                audio_engine = NULL;
            }

            audio_engine = new Audio(); }
            //DEBUG("free heap after audio_engine alloc %ul", (long)ESP.getFreeHeap())
            configure_audio_engine(); 

            {   //DEBUG("new_delete_semaphore 10")
                Lock lock(new_delete_semaphore);
            if (tda8425)
            {
                tda8425->set_input(0);
            }}
        }
        else if (new_audio_control_data.source == AudioControlData::sFm) // changing to FM
        {
            //DEBUG("new_delete_semaphore 11")
            Lock lock(new_delete_semaphore);

            if (rda5807)
            {
                rda5807->set_mute(false);
                rda5807->set_max_volume();
            }

            if (tda8425)
            {
                tda8425->set_input(1);
            }
        }
    }

    DEBUG("audio_control processed source (change)")

    bool will_reconnect_www = false;

    if (new_audio_control_data.channel != status.audio_control_data.channel)
    {
        if (new_audio_control_data.source == AudioControlData::sWww)
        {
            DEBUG("changing www channel to %d", (int) new_audio_control_data.channel)
            will_reconnect_www = true;
        }
    }

    bool r = audio_control(new_audio_control_data);
    response = new_audio_control_data.as_string();

    TRACE("audio_control1 leaves")

    return r;
}

bool MultiHandler::audio_control(const AudioControlData & new_audio_control_data)
{
    TRACE("audio_control2 enters")

    if (new_audio_control_data.source != status.audio_control_data.source)
    {
        _reconnect_count = 0;

        TRACE("audio_control: changing source to %s", AudioControlData::source_2_str(new_audio_control_data.source).c_str())

        if (status.audio_control_data.source == AudioControlData::sBt) // changing from Bt
        {
            //DEBUG("new_delete_semaphore 5")
            Lock lock(new_delete_semaphore);
            //DEBUG("has taken new_delete_semaphore")

            if (fsc_bt != NULL)
            {
                fsc_bt->module_enable_bt(false);
                fsc_bt->module_enable_i2s(false);
            }
            DEBUG("switching off bt done")
        }
        else if (status.audio_control_data.source == AudioControlData::sWww) // changing from Www
        {
            //DEBUG("new_delete_semaphore 6")
            Lock lock(new_delete_semaphore);

            if (audio_engine)
            {
                delete audio_engine;
                audio_engine = NULL;
            }

            configure_audio_engine(); // make sure to put all GPIOs in tri-state
        }
        else if (status.audio_control_data.source == AudioControlData::sFm) // changing from FM
        {
            //DEBUG("new_delete_semaphore 7")
            Lock lock(new_delete_semaphore);

            if (rda5807)
            {
                rda5807->set_mute(true);
            }
        }

        if (new_audio_control_data.source == AudioControlData::sNone) 
        {
            if (tda8425)
            {
                tda8425->set_mute(true);
            }            
        }
        else
        {
            if (tda8425)
            {
                tda8425->set_mute(false);
            }            
        }

        if (new_audio_control_data.source == AudioControlData::sBt) // changing to Bt
        {
            //DEBUG("new_delete_semaphore 8")
            Lock lock(new_delete_semaphore);

            if (fsc_bt != NULL)
            {
                fsc_bt->module_enable_bt(true);
                fsc_bt->module_enable_i2s(true);
            }

            if (tda8425)
            {
                tda8425->set_input(0);
            }
        }
        else if (new_audio_control_data.source == AudioControlData::sWww) // changing to Www
        {
            {   //DEBUG("new_delete_semaphore 9")
                Lock lock(new_delete_semaphore);
            //DEBUG("free heap before audio_engine alloc %ul", (long)ESP.getFreeHeap())

            if (audio_engine)
            {
                delete audio_engine;
                audio_engine = NULL;
            }

            audio_engine = new Audio(); }
            //DEBUG("free heap after audio_engine alloc %ul", (long)ESP.getFreeHeap())
            configure_audio_engine(); 

            {   //DEBUG("new_delete_semaphore 10")
                Lock lock(new_delete_semaphore);
            if (tda8425)
            {
                tda8425->set_input(0);
            }}
        }
        else if (new_audio_control_data.source == AudioControlData::sFm) // changing to FM
        {
            //DEBUG("new_delete_semaphore 11")
            Lock lock(new_delete_semaphore);

            if (rda5807)
            {
                rda5807->set_mute(false);
                rda5807->set_max_volume();
            }

            if (tda8425)
            {
                tda8425->set_input(1);
            }
        }
    }

    DEBUG("audio_control processed source (change)")

    bool will_reconnect_www = false;

    if (new_audio_control_data.channel != status.audio_control_data.channel)
    {
        if (new_audio_control_data.source == AudioControlData::sWww)
        {
            DEBUG("changing www channel to %d", (int) new_audio_control_data.channel)
            will_reconnect_www = true;
        }
    }

    // update audo_control_data

    {Lock lock(semaphore);
    status.audio_control_data = new_audio_control_data;

    if (will_reconnect_www == true)
    {
        _should_reconnect_www = true; 
    }}

    // commit what might change from other places

    if (status.audio_control_data.source == AudioControlData::sFm)
    {
        commit_fm_freq();
    }

    get_max_volume(true);  // trace selection once
    commit_volume();

    // TODO: save to EPROM 

    TRACE("audio_control2 leaves")

    return true;
}

bool MultiHandler::set_volatile(const JsonVariant & json, String * error)
{
    TRACE("set_volatile enters")

    Lock lock(new_delete_semaphore);

    bool r = true;

    if (uism != NULL)
    {
        if (json.is<JsonArray>())
        {
            // DEBUG("json is array")

            const JsonArray & jsonArray = json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & _json = *iterator;

                //DEBUG("item")

                if (_json.containsKey("ui_name"))
                {
                    String ui_name = _json["ui_name"];
                    DEBUG("ui_name %s", ui_name.c_str())
                    
                    if (ui_name == uism->get_config().name)
                    {
                        if (_json.containsKey("temps"))
                        {
                            if (uism->set_temps(_json["temps"]) == false)
                            {
                                r = false;
                            }
                        }

                        if (_json.containsKey("temp_corr"))
                        {
                            if (uism->set_temp_corr(_json["temp_corr"]) == false)
                            {
                                r = false;
                            }
                        }

                        break;
                    }
                }

                ++iterator;
            }
        }
        else
        {
            r = false;
        }
    }

    return r;
}

String MultiHandler::choose_url() 
{
    Lock lock(semaphore);

    int index = 0;

    if (status.audio_control_data.channel >= 0 && status.audio_control_data.channel < config.service.NUM_URLS)
    {
        index = status.audio_control_data.channel;
    }
    else
    {
        if (config.service.url_select >= 0)
        {
            if (config.service.url_select < config.service.NUM_URLS)
            {
                index = config.service.url_select;
            }
        }
        else
        {
            // handle enum
        }
    }

    status.www.url_index = index;
    status.www.url_name = config.service.url[index].name;
    return config.service.url[index].value;
}

float MultiHandler::choose_fm_freq() 
{
    Lock lock(semaphore);

    int index = 0;

    if (status.audio_control_data.channel >= 0 && status.audio_control_data.channel < config.service.NUM_FM_FREQS)
    {
        index = status.audio_control_data.channel;
    }
    else
    {
        if (config.service.fm_freq_select >= 0)
        {
            if (config.service.fm_freq_select < config.service.NUM_FM_FREQS)
            {
                index = config.service.fm_freq_select;
            }
        }
        else
        {
            // handle enum
        }
    }

    status.fm.index = index;
    status.fm.name = config.service.fm_freq[index].name;
    status.fm.freq = config.service.fm_freq[index].value;
    return config.service.fm_freq[index].value;
}

uint8_t MultiHandler::get_max_volume(bool trace) const
{
    if (trace)
    {
        DEBUG("get_max_volume")
    }

    time_t _time_t;
    time(& _time_t);
    tm _tm = {0};
    _tm = *localtime(&_time_t);

    if (_tm.tm_year+1900 > 2000) // otherwise - NTP failed to fetch time 
    {
        uint8_t hour = (uint8_t)((size_t) _tm.tm_hour % (sizeof(config.sound.schedule)/sizeof(config.sound.schedule[0])));

        if (trace)
        {
            TRACE("get_max_volume, datetime is valid, _tm.tm_hour %d, hour %d, schedule[hour] %d", (int) _tm.tm_hour, (int) hour, (int)config.sound.schedule[hour])
        }

        switch(config.sound.schedule[hour])
        {
            case MultiConfig::Sound::smOff:
                if (trace)
                {
                    TRACE("get_max_volume, returning volume 0, smOff")
                }
                return 0;
            case MultiConfig::Sound::smOn:
                if (trace)
                {
                    TRACE("get_max_volume, returning volume %d, smOn", (int) config.sound.volume)
                }
                return config.sound.volume;
            case MultiConfig::Sound::smOnLowVolume:
                if (trace)
                {
                    TRACE("get_max_volume, returning volume_low %d, smLowVolume", (int) config.sound.volume_low)
                }
                return config.sound.volume_low;
            default:
                break;    
        }
    }
    else
    {
        if (trace)
        {
            TRACE("get_max_volume, datetime invalid, _tm.tm_year %d", (int) _tm.tm_year)
        }
    }
    
    if (trace)
    {
        TRACE("get_max_volume, returning default, not by schedule")
    }
    return config.sound.volume;
}

uint8_t MultiHandler::get_volume() const
{
    uint8_t volume = status.audio_control_data.volume;
    uint8_t max_volume = get_max_volume();

    if (volume <= max_volume)
    {
        return volume;
    }

    return max_volume;
}

void MultiHandler::commit_volume()
{
    if (tda8425)
    {
        //DEBUG("new_delete_semaphore 12")
        Lock lock(new_delete_semaphore);
        
        status.commited_volume = get_volume();

        if (tda8425)
        {
            tda8425->set_volume(status.commited_volume);
        }
    }
}

void MultiHandler::commit_fm_freq()
{
    if (rda5807)
    {
        float freq = 0;
        {Lock lock(semaphore);
        freq = choose_fm_freq();}
        //DEBUG("new_delete_semaphore 13")
        Lock lock(new_delete_semaphore);

        if (rda5807)
        {
            rda5807->set_freq(freq);
        }
    }
}

void MultiHandler::configure_audio_engine()
{
    TRACE("multi_task: configure_audio_engine")
    //DEBUG("new_delete_semaphore 14")
    Lock lock(new_delete_semaphore);
    //TRACE("configure_audio_engine, semaphore taken")
    
    if (audio_engine)
    {
        audio_engine->setConnectionTimeout(AUDIO_CONNECTION_TIMEOUT, AUDIO_CONNECTION_TIMEOUT);
        audio_engine->setPinout(config.i2s.bclk.gpio, config.i2s.lrc.gpio, config.i2s.dout.gpio);

        // audio_engine->forceMono(true);
        // configure_sound();
    }
    else
    {
        // put GPIOs to tri-state to allow bt to take over i2s DAC
        pinMode(config.i2s.bclk.gpio, INPUT);
        pinMode(config.i2s.lrc.gpio, INPUT);
        pinMode(config.i2s.dout.gpio, INPUT);
    }

    TRACE("multi_task: configure_audio_engine done")
}

#ifndef USE_HARDWARE_SERIAL

static uart_port_t uart_num_2_port(uint8_t uart_num)
{
    return uart_num == 0 ? UART_NUM_0 : (uart_num == 1 ? UART_NUM_1 : UART_NUM_2);
}

#endif

void MultiHandler::configure_uart()
{
    TRACE("multi_task: configure_uart")
    
    Lock lock(uart_semaphore);

    #ifdef USE_HARDWARE_SERIAL
    
    if (hardware_serial != NULL)
    {
        delete hardware_serial;
        hardware_serial = NULL;
    }
    
    DEBUG("creating hardware_serial for uart_num %d", (int) config.uart.uart_num)
    hardware_serial = new HardwareSerial(config.uart.uart_num);    
    hardware_serial->begin(115200, SERIAL_8N1, config.uart.rx.gpio, config.uart.tx.gpio);
        
    #else

    uart_setup_ok = false;
    const uart_port_t uart_num = uart_num_2_port(config.uart.uart_num);

    uart_driver_delete(uart_num);

    // Install UART driver using an event queue here
    esp_err_t esp_r = uart_driver_install(uart_num, UART_BUFFER_SIZE, 0, 0, NULL,0); 

    if (esp_r != ESP_OK)
    {
        status.status = String("failed to install uart driver, ") + esp_err_2_string(esp_r); 
        ERROR(status.status.c_str())
        // but continue anyway
    }
    else
    {
        TRACE("install uart driver OK")
    }

    uart_config_t uart_config = {
    
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl =  UART_HW_FLOWCTRL_DISABLE, 
    .rx_flow_ctrl_thresh = 120,
    .source_clk = UART_SCLK_APB
    };

    esp_r = uart_param_config(uart_num, &uart_config);

    if (esp_r != ESP_OK)
    {
        status.status = String("failed to configure uart, ") + esp_err_2_string(esp_r); 
        ERROR(status.status.c_str())
    }
    else
    {
        TRACE("configure uart OK")

        esp_r = uart_set_pin(uart_num, config.uart.tx.gpio, config.uart.rx.gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

        if (esp_r != ESP_OK)
        {
            status.status = String("failed to configure uart gpio, ") + esp_err_2_string(esp_r); 
            ERROR(status.status.c_str())
        }
        else
        {
            TRACE("configure uart gpio OK")
            uart_setup_ok = true;
        }
    }
    
    #endif

    TRACE("multi_task: configure_uart done")
}

void MultiHandler::configure_i2c()
{
    TRACE("multi_task: configure_i2c")
    
    if (two_wire)
    {
        two_wire->end();
        two_wire->begin(config.i2c.sda.gpio, config.i2c.scl.gpio);
        i2c_scan(*two_wire);
    }

    TRACE("multi_task: configure_i2c done")
}

void MultiHandler::configure_bt()
{
    TRACE("multi_task: configure_bt")
    
    {   //DEBUG("new_delete_semaphore 15")
        Lock lock(new_delete_semaphore);
    
    if (fsc_bt != NULL)
    {
        delete fsc_bt;
        fsc_bt = NULL;
    }

    fsc_bt = new FscBt(config.bt.hw, config.bt.name, config.bt.pin);}

    fsc_bt->install_uart_command_func(multi_handler_uart_command_func);
}

void MultiHandler::configure_fm()
{
    TRACE("multi_task: configure_fm")
    //DEBUG("new_delete_semaphore 16")
    Lock lock(new_delete_semaphore);
    
    if (config.fm.hw != "RDA5807")
    {
        if (rda5807)
        {
            delete rda5807;
            rda5807 = NULL;
        }
    }
    else if (config.fm.hw == "RDA5807")
    {
        if (rda5807 == NULL)
        {
            rda5807 = new Rda5807(two_wire);
        }
    }
    
    if (rda5807)
    {
        rda5807->set_addr(config.fm.addr.c_str());
        rda5807->commit_all();
    }
}

void MultiHandler::configure_sound()
{
    TRACE("multi_task: configure_sound")

    {   //DEBUG("new_delete_semaphore 17")
        Lock lock(new_delete_semaphore);

    if (config.sound.hw != "TDA8425")
    {
        if (tda8425)
        {
            delete tda8425;
            tda8425 = NULL;
        }
    }
    else if (config.sound.hw == "TDA8425")
    {
        if (tda8425 == NULL)
        {
            tda8425 = new Tda8425(two_wire);
        }
    }
    
    if (tda8425)
    {
        tda8425->set_addr(config.sound.addr.c_str());

        int gain_low_pass = config.sound.gain_low_pass - config.sound.gain_band_pass;
        int gain_high_pass = config.sound.gain_high_pass - config.sound.gain_band_pass;

        tda8425->set_bass(gain_low_pass);
        tda8425->set_treble(gain_high_pass);

        tda8425->commit_all();
    }

    pinMode(config.sound.mute.gpio, OUTPUT);}

    /*
    if (audio_engine)
    {
        audio_engine->setTone(config.sound.gain_low_pass, config.sound.gain_band_pass, config.sound.gain_high_pass);
    }
    */

    get_max_volume(true);  // trace selection once
    commit_volume();

    TRACE("multi_task: configure_sound done")
}

void MultiHandler::configure_tm1638_bus()
{
    TRACE("multi_task: configure_tm1638_bus")
    //DEBUG("new_delete_semaphore 18")
    Lock lock(new_delete_semaphore);
    
    if (tm1638_bus == NULL)
    {
        tm1638_bus = new Tm1638Bus(config.tm1638.dio, config.tm1638.clk, & config.tm1638.dir);

        if (uism)
        {
            uism->set_bus(tm1638_bus);
        }
    }
    else
    {
        tm1638_bus->reconfigure(config.tm1638.dio, config.tm1638.clk, & config.tm1638.dir);
    }
}

void MultiHandler::configure_ui()
{
    TRACE("multi_task: configure_ui")
    //DEBUG("new_delete_semaphore 18")
    Lock lock(new_delete_semaphore);
    
    if (uism == NULL)
    {
        uism = new UISM(config.ui, config.service, config.thermostat);
        uism->set_bus(tm1638_bus);
    }
    else
    {
        uism->reconfigure(config.ui, config.service, config.thermostat);
    }

    status.ui.name = uism->get_config().name;
}

void MultiHandler::set_mute(bool is_mute)
{
    int gpio_state = config.sound.mute.inverted ? (is_mute ? 0 : 1) : (is_mute ? 1 : 0);
    digitalWrite(config.sound.mute.gpio, gpio_state);
}

bool MultiHandler::uart_command(const String & command, AtResponse & response, String * error)
{
    char crlf[] = {0x0d, 0x0a, 0};
    String command_crlf = command + (const char *) crlf;
    TRACE("¤ %s", command.c_str())

    #ifdef USE_HARDWARE_SERIAL
    
    if (hardware_serial)
    {
        { Lock lock(uart_semaphore);
        hardware_serial->write(command_crlf.c_str(), command_crlf.length());

        response.start_timing();

        while ((hardware_serial->available() || !response.is_ready()) && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf);

            length = hardware_serial->read(buf, length); // hej

            if (length > 0)
            {
                response.feed((const char*) buf, length);
            }

            if (!hardware_serial->available())
            {
                delay(UART_READ_WAIT_MS);
            }
        }}

        if (response.is_timeout())
        {
            char buf[80];
            sprintf(buf, "AT command %s timeout", command.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }
        
        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return response.is_ready();
    }

    #else
    
    if (uart_setup_ok == true)
    {
        const uart_port_t uart_num = uart_num_2_port(config.uart.uart_num);

        { Lock lock(uart_semaphore);

        int bytes_written = 
        uart_write_bytes(uart_num, (const char*)command_crlf.c_str(), command_crlf.length());

        if (bytes_written < 0)
        {
            char buf[80];
            sprintf(buf, "uart_write_bytes parameter error, command %s", command.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }

            return false;
        }

        if (bytes_written < command_crlf.length())
        {
            char buf[80];
            sprintf(buf, "uart_write_bytes written %d bytes, expected %d", bytes_written, (int) command_crlf.length());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }

        response.start_timing();
            
        while (!response.is_ready() && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf)-1;

            //DEBUG("about to uart_read_bytes, length %d", length)

            length = uart_read_bytes(uart_num, buf, length, 100);

            //DEBUG("uart_read_bytes returned length %d", length)

            if (length < 0)
            {
                status.status = String("failed uart_read_bytes"); 
                ERROR(status.status.c_str())

                if (error != NULL)
                {
                    *error = status.status;
                }
            }
            else
            if (length > 0)
            {
                //buf[length] = 0;
                //DEBUG("uart_read_bytes: %s", buf)
                response.feed((const char*) buf, length);
            }

            delay(UART_READ_WAIT_MS);
        }}

        if (response.is_timeout())
        {
            char buf[80];
            sprintf(buf, "AT command %s timeout", command.c_str());
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }

        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return response.is_ready();        
    }

    #endif

    return false; 
}

bool MultiHandler::uart_poll_indications(AtResponse & response, String * error)
{
    #ifdef USE_HARDWARE_SERIAL
    
    if (hardware_serial)
    {
        { Lock lock(uart_semaphore);

        response.start_timing();

        while ((hardware_serial->available() || response.is_line_pending()) && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf);

            length = hardware_serial->read(buf, length); // hej

            if (length > 0)
            {
                response.feed((const char*) buf, length);
            }

            if (!hardware_serial->available())
            {
                delay(UART_READ_WAIT_MS);
            }
        }}

        if (response.is_timeout())
        {
            const char * buf = "AT poll timeout";
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }
        
        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return true;
    }

    #else
    
    if (uart_setup_ok == true)
    {
        const uart_port_t uart_num = uart_num_2_port(config.uart.uart_num);

        { Lock lock(uart_semaphore);
        int length = 0;

        if (uart_get_buffered_data_len(uart_num, (size_t*)&length) == ESP_FAIL)
        {
            ERROR("Failed to uart_get_buffered_data_len")
        }
        else
        {
            if (length == 0)
            {
                // nothing is waiting so return immediately
                return true;
            }
        }
        
        response.start_timing();
            
        while (response.is_line_pending() && !response.is_timeout())
        {
            uint8_t buf[128];
            int length = sizeof(buf)-1;

            //DEBUG("about to uart_read_bytes, length %d", length)

            length = uart_read_bytes(uart_num, buf, length, 100);

            //DEBUG("uart_read_bytes returned length %d", length)

            if (length < 0)
            {
                status.status = String("failed uart_read_bytes"); 
                ERROR(status.status.c_str())

                if (error != NULL)
                {
                    *error = status.status;
                }
            }
            else
            if (length > 0)
            {
                //buf[length] = 0;
                //DEBUG("uart_read_bytes: %s", buf)
                response.feed((const char*) buf, length);
            }

            delay(UART_READ_WAIT_MS);
        }}

        if (response.is_timeout())
        {
            const char * buf = "AT poll timeout";
            ERROR(buf)

            if (error != NULL)
            {
                *error = buf;
            }
        }

        Lock lock(semaphore);
        at_latest_indications.feed(response);
        return true;
    }

    #endif

    return false; 
}

void start_multi_task(const MultiConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start multi_task while it is running, redirecting to reconfigure")
        reconfigure_multi(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_multi_task()
{
    handler.stop();
}

void reconfigure_multi(const MultiConfig &_config)
{
    handler.reconfigure(_config);
}

String multi_uart_command(const String & command, String & response)
{
    if (!command.isEmpty())
    {
        AtResponse at_response;
        String r;

        if (handler.uart_command(command, at_response, & r))
        {
            response = at_response.to_string();
            return "";
        }
        else
        {
            return r;
        }        
    }
    
    return "Parameter error";
}

String multi_audio_control(const String & source, const String & channel, const String & volume, String & response)
{
    String error;

    if (handler.audio_control_ext(source, channel, volume, response, & error))
    {
        return "";
    }
    else
    {
        return error;
    }        
}

String multi_set_volatile(const JsonVariant & json)
{
    String error;

    if (handler.set_volatile(json, & error))
    {
        return "";
    }
    else
    {
        return error;
    }        
}

MultiStatus get_multi_status()
{
    return handler.get_status();
}

#endif // INCLUDE_MULTI