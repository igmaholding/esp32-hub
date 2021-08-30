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

        bool read() const;
        void write(bool value) const;

        static bool read(gpio_num_t _gpioNum, bool _inverted);
        static void write(gpio_num_t _gpioNum, bool _inverted, bool value);

        // if gpio num is valid returns corresponding enum, otherwise - ((gpio_num_t)-1)
        static gpio_num_t validateGpioNum(unsigned unvalidatedGpioNum);

        gpio_num_t gpioNum;
        unsigned gpioFunction;
        bool inverted;
};


class GpioCheckpad 
{
    public:

        enum Usage
        {
            uNone = 0,
            
            uDigital = 1,
            uAnalog  = 2,

            uInput   = 4,
            uOutput  = 8,

            uDigitalInput  = uDigital | uInput,
            uDigitalOutput = uDigital | uOutput,
            uAnalogInput   = uAnalog | uInput,
            uAnalogOutput  = uAnalog | uOutput,

            uAllOutput = uDigitalOutput | uAnalogOutput,
            uAllInput  = uDigitalInput  | uAnalogInput,

            uDigitalAll = uDigitalInput | uDigitalOutput,
            uAnalogAll  = uAnalogInput  | uAnalogOutput,

            uAll = uDigitalAll | uAnalogAll,

            uInvalid = -1
        };

        GpioCheckpad() 
        {
            for (size_t i=0;i<sizeof(usage_map)/sizeof(usage_map[0]); ++i)
            {
                usage_map[i] = uNone;
            }
        }

        bool set_usage(gpio_num_t gpio, Usage);  // returns false if the usage does not match capabilities, no changes will be made to the map
        Usage get_usage(gpio_num_t gpio) const;
        
        static Usage get_capabilities(gpio_num_t gpio);
        static adc_attenuation_t check_attenuation(int);

    protected:

        Usage usage_map[GPIO_NUM_MAX];
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

        const GpioChannel * getChannel(gpio_num_t);

  protected:

      static std::map<gpio_num_t, GpioChannel> gpioChannels;
};

