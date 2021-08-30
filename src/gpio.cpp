#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>

#include <binarySemaphore.h>
#include <gpio.h>
#include <trace.h>

GpioHandler gpioHandler;

namespace _gpio_interrupt_handlers 
{

static std::map<gpio_num_t, gpio_interrupt_handler_t> ext_gpio_interrupt_handlers;
static BinarySemaphore gpio_semaphore;

void IRAM_ATTR _gpio_common_interrupt_handler(gpio_num_t num) 
{
    LockISR lock(gpio_semaphore);

    auto iterator = ext_gpio_interrupt_handlers.find(num);

    if (iterator != _gpio_interrupt_handlers::ext_gpio_interrupt_handlers.end())
    {
        gpio_interrupt_handler_t handler = iterator->second;
        handler(num);
    }
}

template<gpio_num_t num> void IRAM_ATTR _gpio_interrupt_handler() 
{
    _gpio_common_interrupt_handler(num);
}

void IRAM_ATTR _gpio_interrupt_default_handler() 
{
    _gpio_common_interrupt_handler(gpio_num_t(-1));
}


native_gpio_interrupt_handler_t get_gpio_interrupt_handler(gpio_num_t num)
{
    switch(num)
    {
        case GPIO_NUM_0: return _gpio_interrupt_handler<GPIO_NUM_0>;
        case GPIO_NUM_1: return _gpio_interrupt_handler<GPIO_NUM_1>;
        case GPIO_NUM_2: return _gpio_interrupt_handler<GPIO_NUM_2>;
        case GPIO_NUM_3: return _gpio_interrupt_handler<GPIO_NUM_3>;
        case GPIO_NUM_4: return _gpio_interrupt_handler<GPIO_NUM_4>;
        case GPIO_NUM_5: return _gpio_interrupt_handler<GPIO_NUM_5>;
        case GPIO_NUM_6: return _gpio_interrupt_handler<GPIO_NUM_6>;
        case GPIO_NUM_7: return _gpio_interrupt_handler<GPIO_NUM_7>;
        case GPIO_NUM_8: return _gpio_interrupt_handler<GPIO_NUM_8>;
        case GPIO_NUM_9: return _gpio_interrupt_handler<GPIO_NUM_9>;

        case GPIO_NUM_10: return _gpio_interrupt_handler<GPIO_NUM_10>;
        case GPIO_NUM_11: return _gpio_interrupt_handler<GPIO_NUM_11>;
        case GPIO_NUM_12: return _gpio_interrupt_handler<GPIO_NUM_12>;
        case GPIO_NUM_13: return _gpio_interrupt_handler<GPIO_NUM_13>;
        case GPIO_NUM_14: return _gpio_interrupt_handler<GPIO_NUM_14>;
        case GPIO_NUM_15: return _gpio_interrupt_handler<GPIO_NUM_15>;
        case GPIO_NUM_16: return _gpio_interrupt_handler<GPIO_NUM_16>;
        case GPIO_NUM_17: return _gpio_interrupt_handler<GPIO_NUM_17>;
        case GPIO_NUM_18: return _gpio_interrupt_handler<GPIO_NUM_18>;
        case GPIO_NUM_19: return _gpio_interrupt_handler<GPIO_NUM_19>;

        //case GPIO_NUM_20: return _gpio_interrupt_handler<GPIO_NUM_20>;
        case GPIO_NUM_21: return _gpio_interrupt_handler<GPIO_NUM_21>;
        case GPIO_NUM_22: return _gpio_interrupt_handler<GPIO_NUM_22>;
        case GPIO_NUM_23: return _gpio_interrupt_handler<GPIO_NUM_23>;
        //case GPIO_NUM_24: return _gpio_interrupt_handler<GPIO_NUM_24>;
        case GPIO_NUM_25: return _gpio_interrupt_handler<GPIO_NUM_25>;
        case GPIO_NUM_26: return _gpio_interrupt_handler<GPIO_NUM_26>;
        case GPIO_NUM_27: return _gpio_interrupt_handler<GPIO_NUM_27>;
        //case GPIO_NUM_28: return _gpio_interrupt_handler<GPIO_NUM_28>;
        //case GPIO_NUM_29: return _gpio_interrupt_handler<GPIO_NUM_29>;

        //case GPIO_NUM_30: return _gpio_interrupt_handler<GPIO_NUM_30>;
        //case GPIO_NUM_31: return _gpio_interrupt_handler<GPIO_NUM_31>;
        case GPIO_NUM_32: return _gpio_interrupt_handler<GPIO_NUM_32>;
        case GPIO_NUM_33: return _gpio_interrupt_handler<GPIO_NUM_33>;
        case GPIO_NUM_34: return _gpio_interrupt_handler<GPIO_NUM_34>;
        case GPIO_NUM_35: return _gpio_interrupt_handler<GPIO_NUM_35>;
        case GPIO_NUM_36: return _gpio_interrupt_handler<GPIO_NUM_36>;
        case GPIO_NUM_37: return _gpio_interrupt_handler<GPIO_NUM_37>;
        case GPIO_NUM_38: return _gpio_interrupt_handler<GPIO_NUM_38>;
        case GPIO_NUM_39: return _gpio_interrupt_handler<GPIO_NUM_39>;
        case GPIO_NUM_MAX: return _gpio_interrupt_handler<GPIO_NUM_MAX>;
    }

    return _gpio_interrupt_default_handler;
}

} // namespace


std::map<gpio_num_t, GpioChannel> GpioHandler::gpioChannels;


bool GpioChannel::read() const
{
    return read(gpioNum, inverted);
}

void GpioChannel::write(bool value) const
{
    write(gpioNum, inverted, value);
}


bool GpioChannel::read(gpio_num_t _gpioNum, bool _inverted) 
{
    if (digitalRead(_gpioNum) == HIGH)
    {
        return _inverted ? false : true;
    }
    else
    {
        return _inverted ? true : false;
    }
}

void GpioChannel::write(gpio_num_t _gpioNum, bool _inverted, bool value) 
{
    bool value_to_write = _inverted ? (value ? LOW : HIGH) : (value ? HIGH : LOW);
    digitalWrite(_gpioNum, value_to_write);
}


gpio_num_t GpioChannel::validateGpioNum(unsigned unvalidatedGpioNum)
{
    switch(unvalidatedGpioNum)
    {
        case GPIO_NUM_0: case GPIO_NUM_1: case GPIO_NUM_2: case GPIO_NUM_3: case GPIO_NUM_4: 
        case GPIO_NUM_5: case GPIO_NUM_6: case GPIO_NUM_7: case GPIO_NUM_8: case GPIO_NUM_9: 

        case GPIO_NUM_10: case GPIO_NUM_11: case GPIO_NUM_12: case GPIO_NUM_13: case GPIO_NUM_14: 
        case GPIO_NUM_15: case GPIO_NUM_16: case GPIO_NUM_17: case GPIO_NUM_18: case GPIO_NUM_19: 

        //case GPIO_NUM_20: 
        case GPIO_NUM_21: case GPIO_NUM_22: case GPIO_NUM_23: 
        //case GPIO_NUM_24: return _gpio_interrupt_handler<GPIO_NUM_24>;
        case GPIO_NUM_25: case GPIO_NUM_26: case GPIO_NUM_27: 
        //case GPIO_NUM_28: 
        //case GPIO_NUM_29: 

        //case GPIO_NUM_30: 
        //case GPIO_NUM_31: 
        case GPIO_NUM_32: case GPIO_NUM_33: case GPIO_NUM_34: case GPIO_NUM_35: case GPIO_NUM_36: 
        case GPIO_NUM_37: case GPIO_NUM_38: case GPIO_NUM_39: 

            return gpio_num_t(unvalidatedGpioNum);

        case GPIO_NUM_MAX: break;
    }
  return gpio_num_t(-1);
}


bool GpioCheckpad::set_usage(gpio_num_t gpio, Usage usage)
{
    // returns false if the usage does not match capabilities, no changes will be made to the map

    if (GpioChannel::validateGpioNum(gpio) != gpio_num_t(-1) && gpio < sizeof(usage_map)/sizeof(usage_map[0]))
    {
        if ((unsigned)(usage & get_capabilities(gpio)) == (unsigned)usage)  // check if we intersect usage and capabilities, all usage bits are still not cleared
        {
            usage_map[gpio] = usage;
            return true;
        }
    }

    return false;
}


GpioCheckpad::Usage GpioCheckpad::get_usage(gpio_num_t gpio) const
{
    if (GpioChannel::validateGpioNum(gpio) != gpio_num_t(-1) && gpio < sizeof(usage_map)/sizeof(usage_map[0]))
    {
        return usage_map[gpio];
    }

    return uInvalid;
}


GpioCheckpad::Usage GpioCheckpad::get_capabilities(gpio_num_t gpio)
{
    // ADC2 should not be used together with WIFI!

    if (GpioChannel::validateGpioNum(gpio) != gpio_num_t(-1))
    {
        switch(gpio)
        {
            case GPIO_NUM_0: return uAllOutput;        // ADC2 
            case GPIO_NUM_1: return uDigitalOutput;
            case GPIO_NUM_2: return uAll;              // ADC2
            case GPIO_NUM_3: return uDigitalInput;            
            case GPIO_NUM_4: return uAll;              // ADC2
            case GPIO_NUM_5: return uDigitalAll;            
            case GPIO_NUM_6: 
            case GPIO_NUM_7: 
            case GPIO_NUM_8: 
            case GPIO_NUM_9: 
            case GPIO_NUM_10: 
            case GPIO_NUM_11: return uNone;
            case GPIO_NUM_12:                           // ADC2
            case GPIO_NUM_13:                           // ADC2
            case GPIO_NUM_14:                           // ADC2
            case GPIO_NUM_15: return uAll;              // ADC2
            case GPIO_NUM_16: 
            case GPIO_NUM_17: 
            case GPIO_NUM_18: 
            case GPIO_NUM_19: return uDigitalAll; 
            // case GPIO_NUM_20: return uNone;
            case GPIO_NUM_21: 
            case GPIO_NUM_22: 
            case GPIO_NUM_23: return uDigitalAll;
            //case GPIO_NUM_24: return uNone;
            case GPIO_NUM_25:                           // ADC2
            case GPIO_NUM_26:                           // ADC2
            case GPIO_NUM_27: return uAll;              // ADC2  
            //case GPIO_NUM_28: 
            //case GPIO_NUM_29: 
            //case GPIO_NUM_30: 
            //case GPIO_NUM_31: 
            case GPIO_NUM_32: 
            case GPIO_NUM_33: return uAll;         // ADC1
            case GPIO_NUM_34:                      // ADC1
            case GPIO_NUM_35:                      // ADC1
            case GPIO_NUM_36:                      // ADC1
            case GPIO_NUM_37:                      // ADC1 
            case GPIO_NUM_38:                      // ADC1
            case GPIO_NUM_39: return uAllInput;    // ADC1
            case GPIO_NUM_MAX: break;
        }
    }

    return uNone;
}


adc_attenuation_t GpioCheckpad::check_attenuation(int atten)
{
    switch(atten)
    {
        case ADC_0db:
        case ADC_2_5db:
        case ADC_6db:
        case ADC_11db: return adc_attenuation_t(atten);
    }

    return adc_attenuation_t(-1);
}


void GpioHandler::setupChannel(gpio_num_t gpioNum, unsigned gpioFunction, bool inverted, gpio_interrupt_handler_t gpio_interrupt_handler)
{
  TRACE("GpioHandler::setupChannel %u", unsigned(gpioNum))  

  GpioChannel gpioChannel(gpioNum, gpioFunction, inverted);  

  pinMode(gpioNum, gpioFunction);

  if (gpioFunction | INPUT)
  {
    unsigned edge = inverted ? FALLING : RISING;
    attachInterrupt(gpioNum, _gpio_interrupt_handlers::get_gpio_interrupt_handler(gpioNum), edge);

    if (gpio_interrupt_handler)
    {
      DEBUG("register interrupt handler for gpio %u", unsigned(gpioNum))

      Lock lock(_gpio_interrupt_handlers::gpio_semaphore);
      auto iterator = _gpio_interrupt_handlers::ext_gpio_interrupt_handlers.find(gpioNum);

      if (iterator != _gpio_interrupt_handlers::ext_gpio_interrupt_handlers.end())
      {
        DEBUG("replacing interrupt handler for gpio %u", unsigned(gpioNum))
        _gpio_interrupt_handlers::ext_gpio_interrupt_handlers[gpioNum] = gpio_interrupt_handler;
      }
      else
      {
        DEBUG("inserting interrupt handler for gpio %u", unsigned(gpioNum))
        _gpio_interrupt_handlers::ext_gpio_interrupt_handlers.insert(std::pair<gpio_num_t,gpio_interrupt_handler_t>(gpioNum, gpio_interrupt_handler));
      }
    }
  }

  auto iterator = gpioChannels.find(gpioNum);

  if (iterator != gpioChannels.end())
  {
      gpioChannels[gpioNum] = gpioChannel;
  }
  else
  {
      gpioChannels.insert(std::pair<gpio_num_t,GpioChannel>(gpioNum, gpioChannel));
  }
}


void GpioHandler::cleanupChannel(gpio_num_t gpioNum)
{
  TRACE("GpioHandler::cleanupChannel %u", unsigned(gpioNum))  

  auto iterator = gpioChannels.find(gpioNum);

  if (iterator != gpioChannels.end())
  {
    DEBUG("channel was previously setup %u", unsigned(gpioNum))  

    pinMode(gpioNum, INPUT_PULLUP);

    Lock lock(_gpio_interrupt_handlers::gpio_semaphore);
    auto iterator2 = _gpio_interrupt_handlers::ext_gpio_interrupt_handlers.find(gpioNum);

    if (iterator2 != _gpio_interrupt_handlers::ext_gpio_interrupt_handlers.end())
    {
      DEBUG("detaching interrupt for gpio %u", unsigned(gpioNum))  
      detachInterrupt(gpioNum);
    
      _gpio_interrupt_handlers::ext_gpio_interrupt_handlers.erase(iterator2);
    }
    else
    {
      DEBUG("external interrupt handler not found for gpio %u", unsigned(gpioNum))  
    }

    gpioChannels.erase(iterator);
  }
}


void GpioHandler::enumerateChannels(std::vector<gpio_num_t> & list) const
{
  auto iterator = gpioChannels.begin();

  while (iterator != gpioChannels.end())
  {
    list.push_back(iterator->first);
    ++iterator;
  }
}


const GpioChannel * GpioHandler::getChannel(gpio_num_t gpioNum)
{
  auto iterator = gpioChannels.find(gpioNum);

  if (iterator != gpioChannels.end())
  {
      return & iterator->second;
  }

  return NULL;
}