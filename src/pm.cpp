#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>

#include <gpio.h>
#include <trace.h>

extern GpioHandler gpioHandler;


unsigned counter = 0;

#define DEBOUNCE_TIME 250
uint32_t DebounceTimer = 0;

class PmChannel 
{
  public:

      PmChannel(gpio_num_t _gpioNum, unsigned _debounce) :

          gpioNum(_gpioNum), debounce(_debounce), counter(0), 
          lastMillis(0) {}

      PmChannel()
      {
        gpioNum = GPIO_NUM_0;
        debounce = 0;
        counter = 0;
        lastMillis = 0;
      }

      void reset()
      {
        counter = 0;
        lastMillis = 0;
      } 

      void interruptHandler();

      gpio_num_t gpioNum;
      unsigned debounce;
      unsigned counter;

      uint32_t lastMillis;
};


class PmHandler 
{
  public:

      PmHandler()
      {
        isConfigured = false;
      }

      void setupChannel(gpio_num_t, unsigned debounce);
      void cleanupChannel(gpio_num_t);
      void resetChannel(gpio_num_t);
      
      unsigned getCounter(gpio_num_t) const;

      void enumerateChannels(std::vector<gpio_num_t> &) const;
      void reset(const String& _resetStamp);

      static void interruptHandler(gpio_num_t num);

      String resetStamp;
      bool isConfigured;

  protected:

      static std::map<gpio_num_t, PmChannel> pmChannels;
};


PmHandler pmHandler;
std::map<gpio_num_t, PmChannel> PmHandler::pmChannels;



void IRAM_ATTR PmChannel::interruptHandler()
{
  uint32_t _millis = millis();

  if (_millis - lastMillis >= debounce) 
  {
    lastMillis = _millis;
    counter++;

    //DEBUG("interruptHandler for gpio %u", (unsigned) gpioNum);
    //DEBUG("counter %u", counter);
  }
}


void PmHandler::setupChannel(gpio_num_t gpioNum, unsigned debounce)
{
  TRACE("PmHandler::setupChannel %u", (unsigned) gpioNum)

  PmChannel pmChannel(gpioNum, debounce);

  if (pmChannels.find(gpioNum) == pmChannels.end())
  {
    pmChannels.insert(std::pair<gpio_num_t, PmChannel>(gpioNum, pmChannel));
  }
  else
  {
    pmChannels[gpioNum] = pmChannel;
  }
}


void PmHandler::cleanupChannel(gpio_num_t gpioNum)
{
  TRACE("PmHandler::cleanupChannel %u", (unsigned) gpioNum)

  auto iterator = pmChannels.find(gpioNum);
  
  if (iterator != pmChannels.end())
  {
    pmChannels.erase(gpioNum);
  }
}


void PmHandler::resetChannel(gpio_num_t gpioNum)
{
  TRACE("PmHandler::resetChannel %u", (unsigned) gpioNum)

  auto iterator = pmChannels.find(gpioNum);
  
  if (iterator != pmChannels.end())
  {
    iterator->second.reset();
  }
}


unsigned PmHandler::getCounter(gpio_num_t gpioNum) const
{
  auto iterator = pmChannels.find(gpioNum);

  if (iterator != pmChannels.end())
  {
    return iterator->second.counter;
  }
  else
  {
    return unsigned(-1);
  }
}


void PmHandler::reset(const String & _resetStamp)
{
  auto iterator = pmChannels.begin();
  
  while (iterator != pmChannels.end())
  {
    iterator->second.reset();
    ++iterator;
  }

  resetStamp = _resetStamp;
}


void PmHandler::enumerateChannels(std::vector<gpio_num_t> & list) const
{
  auto iterator = pmChannels.begin();

  while (iterator != pmChannels.end())
  {
    list.push_back(iterator->first);
    ++iterator;
  }
}


void IRAM_ATTR PmHandler::interruptHandler(gpio_num_t gpioNum)
{
  auto iterator = pmChannels.find(gpioNum);

  if (iterator != pmChannels.end())
  {
    iterator->second.interruptHandler();
  }
}


void pingPm(JsonVariant & json)
{
  TRACE("pingPm")
  
  json["reset_stamp"] = pmHandler.resetStamp;
  json["is_configured"] = pmHandler.isConfigured;
}


void setupPm(const JsonVariant & json, const String & resetStamp) 
{
  TRACE("setupPm")

  if (json.containsKey("channels"))
  {
    DEBUG("contains channels")
    const JsonVariant & channelsJson = json["channels"];

    if (channelsJson.is<JsonArray>())
    {
      DEBUG("channels is array")
      const JsonArray & jsonArray = channelsJson.as<JsonArray>();

      auto iterator = jsonArray.begin();

      while(iterator != jsonArray.end())
      {
        const JsonVariant & channelJson = *iterator;

        if (channelJson.containsKey("gpio"))
        {
          DEBUG("channel contains gpio")
          bool inverted = false;
          unsigned debounce = 0;
          unsigned gpio = (unsigned)((int) channelJson["gpio"]);

          if (channelJson.containsKey("inverted"))
          {
            DEBUG("channel contains inverted")
            inverted = channelJson["inverted"];
          }

          if (channelJson.containsKey("debounce"))
          {
            DEBUG("channel contains debounce")
            debounce = (unsigned)((int) channelJson["debounce"]);
          }

          DEBUG("Configuration request for gpio channel %u, inverted %d, debounce %u", gpio, (int) inverted, debounce)

          // we need to validate parameters so that it is not possible to burn or hang the hw by an erroneous REST request

          gpio_num_t gpioNum = GpioChannel::validateGpioNum(gpio);

          if (gpioNum != gpio_num_t(-1))
          {
            TRACE("Setting up channel %u, inverted %d, debounce %u", gpio, (int) inverted, debounce)

            gpioHandler.setupChannel(gpioNum, INPUT_PULLUP, inverted, PmHandler::interruptHandler);
            pmHandler.setupChannel(gpioNum, debounce);
          }
          else
          {
            ERROR("Failed setup request for gpio channel %u, gpio invalid", gpio)
          }
        }

        ++iterator;
      }
    }
  }

  if (!resetStamp.isEmpty())
  {
    DEBUG("contains resetStamp")

    pmHandler.resetStamp = resetStamp;
    DEBUG("resetStamp %s", pmHandler.resetStamp.c_str())
  }
  
  pmHandler.isConfigured = true;
}

void cleanupPm() 
{
  TRACE("cleanupPm")

  std::vector<gpio_num_t> gpioNumList;
  pmHandler.enumerateChannels(gpioNumList);

  auto iterator = gpioNumList.begin();

  while(iterator != gpioNumList.end())
  {
    pmHandler.cleanupChannel(*iterator);
    gpioHandler.cleanupChannel(*iterator);

    ++iterator;
  }

  pmHandler.resetStamp.clear();
  pmHandler.isConfigured = false;
}


void resetPm(const String & resetStamp)
{
  TRACE("resetPm")
  DEBUG("resetStamp %s", resetStamp.c_str())
  
  pmHandler.reset(resetStamp);
}


void getPm(JsonVariant & json, const String & resetStamp)
{
  TRACE("getPm")
  
  json["reset_stamp"] = pmHandler.resetStamp;
  json["is_configured"] = pmHandler.isConfigured;

  json.createNestedArray("gpio");
  json.createNestedArray("counter");
  JsonArray jsonArrayGpio = json["gpio"]; 
  JsonArray jsonArrayCounter = json["counter"]; 

  std::vector<gpio_num_t> gpioNumList;
  pmHandler.enumerateChannels(gpioNumList);

  auto iterator = gpioNumList.begin();

  while(iterator != gpioNumList.end())
  {
    counter = pmHandler.getCounter(*iterator);
    jsonArrayGpio.add((unsigned) *iterator);
    jsonArrayCounter.add(counter);
    ++iterator;
  }

  if (resetStamp.isEmpty() == false)  // ordered reset after get
  {
    DEBUG("reset with get, resetStamp %s", resetStamp.c_str())
    pmHandler.reset(resetStamp);
  }
}
