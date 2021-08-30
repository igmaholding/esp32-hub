#include <Arduino.h>
#include <onboardLed.h>

#define ONBOARD_LED_GPIO  2

/*

OFF - just started
FAST BLINK - searching for WIFI
SLOW BLINK - WIFI connected, not configured yet
ON - connected and configured

during any state - fast single blink - REST access 

*/



LedState led_state = ledOff;
bool led_blink_once = false;
bool led_wifi_search = false;
bool led_wifi_on = false;
bool led_configured = false;


void onboard_led_task(void * parameter)
{
  pinMode(ONBOARD_LED_GPIO,OUTPUT);

  const int led_pattern_off[] = {0,0,0,0,0,0,0,0}; 
  const int led_pattern_on[] = {1,1,1,1,1,1,1,1}; 
  const int led_pattern_50_slow[] = {1,1,1,1,0,0,0,0}; 
  const int led_pattern_50_fast[] = {1,0,1,0,1,0,1,0}; 

  int modulo = sizeof(led_pattern_off)/sizeof(led_pattern_off[0]);
  int slot_index = 0;

  while(true)
 {
    if (led_wifi_on == true)
    {
      if (led_configured == true)
      {
        led_state = ledOn;
      }
      else
      {
        led_state = led50Slow;
      }
    }
    else if (led_wifi_search == true)
    {
      led_state = led50Fast;
    }
    else 
    {
      led_state = ledOff;
    }

    const int * led_pattern = led_pattern_off;

    if (led_state == ledOn)
    {
      led_pattern = led_pattern_on;
    }
    else if (led_state == led50Slow)
    {
      led_pattern = led_pattern_50_slow;
    }
    else if (led_state == led50Fast)
    {
      led_pattern = led_pattern_50_fast;
    }
    
    int gpio_state = led_pattern[slot_index] == 0 ? LOW : HIGH;

    if (led_blink_once)
    {
      led_blink_once = false;
      gpio_state = gpio_state == LOW ? HIGH : LOW;
    }

    digitalWrite(ONBOARD_LED_GPIO, gpio_state);
    slot_index = (slot_index+1)%modulo;

    delay(1000 / modulo);
  }
}


void start_onboard_led_task()
{ 
  xTaskCreate(
    onboard_led_task,    // Function that should be called
    "onboard_led_task",   // Name of the task (for debugging)
    1000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
}

