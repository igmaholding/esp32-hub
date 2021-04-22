
enum LedState 
{
  ledOff,
  ledOn,
  led50Slow,
  led50Fast,
};

extern LedState led_state;
extern bool led_blink_once;
extern bool led_wifi_search;
extern bool led_wifi_on;
extern bool led_configured;

void start_onboard_led_task();
