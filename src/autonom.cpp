#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>
#include <sstream>

#include <autonom.h>

#ifdef INCLUDE_SHOWERGUARD
#include <showerGuard.h>
#endif

#ifdef INCLUDE_KEYBOX
#include <keyBox.h>
#endif

#ifdef INCLUDE_AUDIO
#include <aaudio.h>
#endif

#ifdef INCLUDE_RFIDLOCK
#include <rfidLock.h>
#endif

#include <trace.h>
#include <epromImage.h>


class AutonomTaskManager
{
    public:

        AutonomTaskManager()
        {
            showerGuardActive = false;
            keyboxActive = false;
            audioActive = false;
            rfidLockActive = false;
        }

        #ifdef INCLUDE_SHOWERGUARD

        void startShowerGuard(const ShowerGuardConfig &);
        void stopShowerGuard();
        void reconfigureShowerGuard(const ShowerGuardConfig &);

        ShowerGuardStatus getShowerGuardStatus() const;

        bool isShowerGuardActive() const { return showerGuardActive; }

        #endif

        #ifdef INCLUDE_KEYBOX
        
        void startKeybox(const KeyboxConfig &);
        void stopKeybox();
        void reconfigureKeybox(const KeyboxConfig &);

        String keyboxActuate(const String & channel_str);

        KeyboxStatus getKeyboxStatus() const;

        bool isKeyboxActive() const { return keyboxActive; }
        
        #endif

        #ifdef INCLUDE_AUDIO
        
        void startAudio(const AudioConfig &);
        void stopAudio();
        void reconfigureAudio(const AudioConfig &);

        AudioStatus getAudioStatus() const;

        bool isAudioActive() const { return audioActive; }
        
        #endif

        #ifdef INCLUDE_RFIDLOCK
        
        void startRfidLock(const RfidLockConfig &);
        void stopRfidLock();
        void reconfigureRfidLock(const RfidLockConfig &);
        
        String rfidLockProgram(const String & code_str);
        
        RfidLockStatus getRfidLockStatus() const;

        bool isRfidLockActive() const { return rfidLockActive; }
        
        #endif

        void stopAll();


    protected:

        bool showerGuardActive;
        bool keyboxActive;
        bool audioActive;
        bool rfidLockActive;

};

#ifdef INCLUDE_SHOWERGUARD

void AutonomTaskManager::startShowerGuard(const ShowerGuardConfig & config)
{
    TRACE("Starting autonom task showerGuard")
    start_shower_guard_task(config);
    showerGuardActive = true;
}

void AutonomTaskManager::stopShowerGuard()
{
    TRACE("stopping autonom task showerGuard")
    stop_shower_guard_task();
    showerGuardActive = false;
}

void AutonomTaskManager::reconfigureShowerGuard(const ShowerGuardConfig & config)
{
    TRACE("reconfiguring autonom task showerGuard")
    reconfigure_shower_guard(config);
}

ShowerGuardStatus AutonomTaskManager::getShowerGuardStatus() const
{
    TRACE("getShowerGuardStatus")
    return get_shower_guard_status();
}

#endif // INCLUDE_SHOWERGUARD

#ifdef INCLUDE_KEYBOX

void AutonomTaskManager::startKeybox(const KeyboxConfig & config)
{
    TRACE("Starting autonom task keyBox")
    start_keybox_task(config);
    keyboxActive = true;
}

void AutonomTaskManager::stopKeybox()
{
    TRACE("stopping autonom task keyBox")
    stop_keybox_task();
    keyboxActive = false;
}

void AutonomTaskManager::reconfigureKeybox(const KeyboxConfig & config)
{
    TRACE("reconfiguring autonom task keyBox")
    reconfigure_keybox(config);
}

String AutonomTaskManager::keyboxActuate(const String & channel_str)
{
    return keybox_actuate(channel_str);
}

KeyboxStatus AutonomTaskManager::getKeyboxStatus() const
{
    TRACE("getKeyboxStatus")
    return get_keybox_status();
}

#endif // INCLUDE_KEYBOX

#ifdef INCLUDE_AUDIO

void AutonomTaskManager::startAudio(const AudioConfig & config)
{
    TRACE("Starting autonom task audio")
    start_audio_task(config);
    audioActive = true;
}

void AutonomTaskManager::stopAudio()
{
    TRACE("stopping autonom task audio")
    stop_audio_task();
    audioActive = false;
}

void AutonomTaskManager::reconfigureAudio(const AudioConfig & config)
{
    TRACE("reconfiguring autonom task audio")
    reconfigure_audio(config);
}

AudioStatus AutonomTaskManager::getAudioStatus() const
{
    TRACE("getAudioStatus")
    return get_audio_status();
}

#endif // INCLUDE_AUDIO

#ifdef INCLUDE_RFIDLOCK

void AutonomTaskManager::startRfidLock(const RfidLockConfig & config)
{
    TRACE("Starting autonom task rfid-lock")
    start_rfid_lock_task(config);
    rfidLockActive = true;
}

void AutonomTaskManager::stopRfidLock()
{
    TRACE("stopping autonom task rfid_lock")
    stop_rfid_lock_task();
    rfidLockActive = false;
}

void AutonomTaskManager::reconfigureRfidLock(const RfidLockConfig & config)
{
    TRACE("reconfiguring autonom task rfid-lock")
    reconfigure_rfid_lock(config);
}

String AutonomTaskManager::rfidLockProgram(const String & code_str)
{
    return rfid_lock_program(code_str);
}

RfidLockStatus AutonomTaskManager::getRfidLockStatus() const
{
    TRACE("getRfidLockStatus")
    return get_rfid_lock_status();
}

#endif // INCLUDE_RFIDLOCK

void AutonomTaskManager::stopAll()
{
    TRACE("stopping all autonom tasks")
    
    #ifdef INCLUDE_SHOWERGUARD

    if (showerGuardActive)
    {
        stopShowerGuard();
    }

    #endif

    # if INCLUDE_KEYBOX

    if (keyboxActive)
    {
        stopKeybox();
    }

    #endif

    # if INCLUDE_AUDIO

    if (audioActive)
    {
        stopAudio();
    }

    #endif
}


AutonomTaskManager autonomTaskManager;


const char * function_type_2_str(FunctionType ft)
{
    switch(ft)
    {
        case ftShowerGuard:
            return "shower-guard";
        case ftKeybox:
            return "keybox";
        case ftAudio:
            return "audio";
        case ftRfidLock:
            return "rfid-lock";
        default:
            return "<unknown>";    
    }
}

String setupAutonom(const JsonVariant & json) 
{
    TRACE("setupAutonom")

    EpromImage epromImage;
    char buf[256];

    #ifdef INCLUDE_SHOWERGUARD
    ShowerGuardConfig showerGuardConfig;
    #endif

    #ifdef INCLUDE_KEYBOX
    KeyboxConfig keyBoxConfig;
    #endif

    #ifdef INCLUDE_AUDIO
    AudioConfig audioConfig;
    #endif

    #ifdef INCLUDE_RFIDLOCK
    RfidLockConfig rfidLockConfig;
    #endif

    if (json.is<JsonArray>())
    {
        DEBUG("payload is array")
        const JsonArray & jsonArray = json.as<JsonArray>();

        auto iterator = jsonArray.begin();

        while(iterator != jsonArray.end())
        {
            DEBUG("analysing payload item")
            const JsonVariant & _json = *iterator;

            if (_json.containsKey("function"))
            {
                String function = _json["function"];
                DEBUG("contains function %s", function.c_str())

                if (function == "shower-guard")
                {
                    #ifdef INCLUDE_SHOWERGUARD

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        showerGuardConfig.from_json(config_json);

                        TRACE("function %s: showerGuardConfig.is_valid=%s", function.c_str(), (showerGuardConfig.is_valid() ? "true" : "false"))
                        TRACE(showerGuardConfig.as_string().c_str())

                        if (showerGuardConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            showerGuardConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            epromImage.blocks.insert({(uint8_t) ftShowerGuard, buffer});
                        }
                        else
                        {
                            sprintf(buf, "function %s: config invalid", function.c_str());
                            ERROR(buf)
                            return String(buf);
                        }
                    }
                    else
                    {
                        sprintf(buf, "payload item with function %s should contain config", function.c_str());
                        ERROR(buf)
                        return String(buf);
                    }

                    #else

                    sprintf(buf, "attempt to configure function %s which is not built in current module", function.c_str());
                    ERROR(buf)
                    return String(buf);

                    #endif // INCLUDE_SHOWERGUARD
                }
                else
                if (function == "keybox")
                {
                    #ifdef INCLUDE_KEYBOX

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        keyBoxConfig.from_json(config_json);

                        TRACE("function %s: keyBoxConfig.is_valid=%s", function.c_str(), (keyBoxConfig.is_valid() ? "true" : "false"))
                        TRACE(keyBoxConfig.as_string().c_str())

                        if (keyBoxConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            keyBoxConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            epromImage.blocks.insert({(uint8_t) ftKeybox, buffer});
                        }
                        else
                        {
                            sprintf(buf, "function %s: config invalid", function.c_str());
                            ERROR(buf)
                            return String(buf);
                        }
                    }
                    else
                    {
                        sprintf(buf, "payload item with function %s should contain config", function.c_str());
                        ERROR(buf)
                        return String(buf);
                    }

                    #else

                    sprintf(buf, "attempt to configure function %s which is not built in current module", function.c_str());
                    ERROR(buf)
                    return String(buf);

                    #endif // INCLUDE_KEYBOX
                }
                else
                if (function == "audio")
                {
                    #ifdef INCLUDE_AUDIO

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        audioConfig.from_json(config_json);

                        TRACE("function %s: audioConfig.is_valid=%s", function.c_str(), (audioConfig.is_valid() ? "true" : "false"))
                        TRACE(audioConfig.as_string().c_str())

                        if (audioConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            audioConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            epromImage.blocks.insert({(uint8_t) ftAudio, buffer});
                        }
                        else
                        {
                            sprintf(buf, "function %s: config invalid", function.c_str());
                            ERROR(buf)
                            return String(buf);
                        }
                    }
                    else
                    {
                        sprintf(buf, "payload item with function %s should contain config", function.c_str());
                        ERROR(buf)
                        return String(buf);
                    }

                    #else

                    sprintf(buf, "attempt to configure function %s which is not built in current module", function.c_str());
                    ERROR(buf)
                    return String(buf);

                    #endif // INCLUDE_AUDIO
                }
                else
                if (function == "rfid-lock")
                {
                    #ifdef INCLUDE_RFIDLOCK

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        rfidLockConfig.from_json(config_json);

                        TRACE("function %s: rfidLockConfig.is_valid=%s", function.c_str(), (rfidLockConfig.is_valid() ? "true" : "false"))
                        TRACE(rfidLockConfig.as_string().c_str())

                        if (rfidLockConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            rfidLockConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            epromImage.blocks.insert({(uint8_t) ftRfidLock, buffer});
                        }
                        else
                        {
                            sprintf(buf, "function %s: config invalid", function.c_str());
                            ERROR(buf)
                            return String(buf);
                        }
                    }
                    else
                    {
                        sprintf(buf, "payload item with function %s should contain config", function.c_str());
                        ERROR(buf)
                        return String(buf);
                    }

                    #else

                    sprintf(buf, "attempt to configure function %s which is not built in current module", function.c_str());
                    ERROR(buf)
                    return String(buf);

                    #endif // INCLUDE_AUDIO
                }
                else
                {
                    sprintf(buf, "function %s is unknown", function.c_str());
                    ERROR(buf)
                    return String(buf);
                }
            }
            else
            {
                const char * str = "payload item does NOT contain function";
                ERROR(str)
                return String(str);
            }
            ++iterator;
        }

        EpromImage currentEpromImage;
        currentEpromImage.read();

        std::vector<uint8_t> added, removed, changed;

        if (epromImage.diff(currentEpromImage, & added, & removed, & changed) == true)
        {
            TRACE("New autonom configuration is different from one stored in EEPROM: updating EEPROM image")
            epromImage.write();

            for (auto it=added.begin(); it!=added.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    #ifdef INCLUDE_SHOWERGUARD
                    autonomTaskManager.startShowerGuard(showerGuardConfig);
                    #endif
                }
                else
                if (*it == ftKeybox)
                {
                    #ifdef INCLUDE_KEYBOX
                    autonomTaskManager.startKeybox(keyBoxConfig);
                    #endif
                }
                else
                if (*it == ftAudio)
                {
                    #ifdef INCLUDE_AUDIO
                    autonomTaskManager.startAudio(audioConfig);
                    #endif
                }
                else
                if (*it == ftRfidLock)
                {
                    #ifdef INCLUDE_RFIDLOCK
                    autonomTaskManager.startRfidLock(rfidLockConfig);
                    #endif
                }

            }
            for (auto it=removed.begin(); it!=removed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    #ifdef INCLUDE_SHOWERGUARD
                    autonomTaskManager.stopShowerGuard();
                    #endif
                }
                else
                if (*it == ftKeybox)
                {
                    #ifdef INCLUDE_KEYBOX
                    autonomTaskManager.stopKeybox();
                    #endif
                }
                else
                if (*it == ftAudio)
                {
                    #ifdef INCLUDE_AUDIO
                    autonomTaskManager.stopAudio();
                    #endif
                }
                else
                if (*it == ftRfidLock)
                {
                    #ifdef INCLUDE_RFIDLOCK
                    autonomTaskManager.stopRfidLock();
                    #endif
                }
            }
            for (auto it=changed.begin(); it!=changed.end(); ++it)
            {
                if (*it == ftShowerGuard)
                {
                    #ifdef INCLUDE_SHOWERGUARD
                    autonomTaskManager.reconfigureShowerGuard(showerGuardConfig);
                    #endif
                }
                else
                if (*it == ftKeybox)
                {
                    #ifdef INCLUDE_KEYBOX
                    autonomTaskManager.reconfigureKeybox(keyBoxConfig);
                    #endif
                }
                else
                if (*it == ftAudio)
                {
                    #ifdef INCLUDE_AUDIO
                    autonomTaskManager.reconfigureAudio(audioConfig);
                    #endif
                }
                else
                if (*it == ftRfidLock)
                {
                    #ifdef INCLUDE_RFIDLOCK
                    autonomTaskManager.reconfigureRfidLock(rfidLockConfig);
                    #endif
                }
            }
        }
        else
        {
            TRACE("New autonom configuration is the same as the one stored in EEPROM: skip updating EEPROM image")

        }
    }
    else
    {
        const char * str = "payload is NOT array";
        ERROR(str)
        return String(str);
    }

    return String();
}

void cleanupAutonom() 
{
    TRACE("cleanupAutonom")

    // remove all functions from EEPROM 

    EpromImage epromImage, currentEpromImage; 
    currentEpromImage.read();

    std::vector<uint8_t> added, removed, changed;

    if (epromImage.diff(currentEpromImage, & added, & removed, & changed) == true)
    {
        TRACE("New autonom configuration is different from one stored in EEPROM: updating EEPROM image")
        epromImage.write();
    }

    // stop all running tasks
        
    autonomTaskManager.stopAll();
}

String actionAutonomKeyboxActuate(const String & channel_str)
{
    #ifdef INCLUDE_KEYBOX
    if (autonomTaskManager.isKeyboxActive())
    {
        return autonomTaskManager.keyboxActuate(channel_str);
    }
    else
    {
        return "Keybox not active";
    }
    
    #else

    return "Keybox is not built in currrent module";

    #endif // INCLUDE_KEYBOX
}

String actionAutonomRfidLockProgram(const String & code_str)
{
    #ifdef INCLUDE_RFIDLOCK
    if (autonomTaskManager.isRfidLockActive())
    {
        return autonomTaskManager.rfidLockProgram(code_str);
    }
    else
    {
        return "RfidLock not active";
    }
    
    #else

    return "RfidLock is not built in currrent module";

    #endif // INCLUDE_RFIDLOCK
}

void getAutonom(JsonVariant & json)
{
  TRACE("getAutonom")

  #ifdef INCLUDE_SHOWERGUARD
  if (autonomTaskManager.isShowerGuardActive())
  {
        ShowerGuardStatus status = autonomTaskManager.getShowerGuardStatus();
        status.to_json(json);
  }
  #endif

  #ifdef INCLUDE_KEYBOX
  if (autonomTaskManager.isKeyboxActive())
  {
        KeyboxStatus status = autonomTaskManager.getKeyboxStatus();
        status.to_json(json);
  }
  
  #endif // INCLUDE_KEYBOX

  #ifdef INCLUDE_AUDIO
  if (autonomTaskManager.isAudioActive())
  {
        AudioStatus status = autonomTaskManager.getAudioStatus();
        status.to_json(json);
  }

  #endif

  #ifdef INCLUDE_RFIDLOCK
  if (autonomTaskManager.isRfidLockActive())
  {
        RfidLockStatus status = autonomTaskManager.getRfidLockStatus();
        status.to_json(json);
  }

  #endif
}

void restoreAutonom() 
{
  TRACE("restoreAutonom")

  EpromImage epromImage;
  
  if (epromImage.read() == true)
  {
      for (auto it = epromImage.blocks.begin(); it != epromImage.blocks.end(); ++it)
      {
          const char * function_type_str = function_type_2_str((FunctionType) it->first);
          TRACE("Restoring from EEPROM config for function %s", function_type_str)

          std::istringstream is(it->second);

          switch(it->first)
          {
              case ftShowerGuard:
                
                #ifdef INCLUDE_SHOWERGUARD
                {ShowerGuardConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startShowerGuard(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif
                break;

              case ftKeybox:
                
                #ifdef INCLUDE_KEYBOX
                {KeyboxConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startKeybox(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_KEYBOX
                break;

              case ftAudio:
                
                #ifdef INCLUDE_AUDIO
                {AudioConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startAudio(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_AUDIO
                break;

              case ftRfidLock:
                
                #ifdef INCLUDE_RFIDLOCK
                {RfidLockConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startRfidLock(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_RFIDLOCK
                break;

              default:
                TRACE("Unhandled config for function %s", function_type_str)
                break;
          }
      }
  }
  else
  {
      ERROR("Cannot read EEPROM image")
  }
}
