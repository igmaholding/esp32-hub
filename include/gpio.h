#include <Arduino.h>
#include <map>

typedef void (*native_gpio_interrupt_handler_t)();
typedef void (*gpio_interrupt_handler_t)(gpio_num_t);

class GpioChannel 
{
  public:

      GpioChannel(gpio_num_t _gpioNum, unsigned _gpioFunction, bool _inverted) :
          gpioNum(_gpioNum), gpioFunction(_gpioFunction), inverted(_inverted) {}

      GpioChannel()
      {
        gpioNum = GPIO_NUM_0;
        gpioFunction = INPUT;
        inverted = false;
      }

      // if gpio num is valid returns corresponding enum, otherwise - ((gpio_num_t)-1)
      static gpio_num_t validateGpioNum(unsigned unvalidatedGpioNum);

      gpio_num_t gpioNum;
      unsigned gpioFunction;
      bool inverted;
};


class GpioHandler 
{
  public:

      GpioHandler()
      {
      }

      void setupChannel(gpio_num_t, unsigned _gpioFunction, bool _inverted, gpio_interrupt_handler_t = NULL);
      void cleanupChannel(gpio_num_t);
      
      void enumerateChannels(std::vector<gpio_num_t> &) const;

  protected:

      static std::map<gpio_num_t, GpioChannel> gpioChannels;
};

