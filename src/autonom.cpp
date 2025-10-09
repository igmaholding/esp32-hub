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

#ifdef INCLUDE_PROPORTIONAL
#include <proportional.h>
#endif

#ifdef INCLUDE_ZERO2TEN
#include <zero2ten.h>
#endif

#ifdef INCLUDE_MAINSPROBE
#include <mainsProbe.h>
#endif

#ifdef INCLUDE_MULTI
#include <multi.h>
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
            proportionalActive = false;
            zero2tenActive = false;
            mainsProbeActive = false;
            multiActive = false;
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
        
        String rfidLockProgram(const String & code_str, uint16_t timeout);

        String rfidLockAddCode(const String & name_str, const String & code_str, const std::vector<String> & locks, 
                               const String & type_str);

        String rfidLockDeleteCode(const String & name_str);

        String rfidLockDeleteAllCodes();

        String rfidLockUnlock(const String & lock_channel_str);

        void rfidLockGetCodes(JsonVariant &);

        RfidLockStatus getRfidLockStatus() const;

        bool isRfidLockActive() const { return rfidLockActive; }
        
        #endif

        #ifdef INCLUDE_PROPORTIONAL
        
        void startProportional(const ProportionalConfig &);
        void stopProportional();
        void reconfigureProportional(const ProportionalConfig &);
        
        String proportionalCalibrate(const String & channel_str);
        String proportionalActuate(const String & channel_str, const String & value_str, const String & ref_str);

        ProportionalStatus getProportionalStatus() const;

        bool isProportionalActive() const { return proportionalActive; }
        
        #endif

        #ifdef INCLUDE_ZERO2TEN
        
        void startZero2ten(const Zero2tenConfig &);
        void stopZero2ten();
        void reconfigureZero2ten(const Zero2tenConfig &);
        
        String zero2tenCalibrateInput(const String & channel_str, const String & value_str);
        String zero2tenInput(const String & channel_str, String & value_str);
        String zero2tenCalibrateOutput(const String & channel_str, const String & value_str);
        String zero2tenOutput(const String & channel_str, const String & value_str);

        Zero2tenStatus getZero2tenStatus() const;

        bool isZero2tenActive() const { return zero2tenActive; }
        
        #endif

        #ifdef INCLUDE_MAINSPROBE
        
        void startMainsProbe(const MainsProbeConfig &);
        void stopMainsProbe();
        void reconfigureMainsProbe(const MainsProbeConfig &);
        
        String mainsProbeCalibrateV(const String & addr_str, const String & channel_str, const String & value_str);
        String mainsProbeCalibrateAHigh(const String & addr_str, const String & channel_str, const String & value_str);
        String mainsProbeCalibrateALow(const String & channel_str, const String & value_str);
        String mainsProbeInputV(const String & addr_str, const String & channel_str, String & value_str);
        String mainsProbeInputAHigh(const String & addr_str, const String & channel_str, String & value_str);
        String mainsProbeInputALow(const String & channel_str, String & value_str);
        void mainsProbeGetCalibrationData(JsonVariant &);
        String mainsProbeImportCalibrationData(const JsonVariant & json);

        MainsProbeStatus getMainsProbeStatus() const;

        bool isMainsProbeActive() const { return mainsProbeActive; }
        
        #endif

        #ifdef INCLUDE_MULTI
        
        void startMulti(const MultiConfig &);
        void stopMulti();
        void reconfigureMulti(const MultiConfig &);
        
        String multiUartCommand(const String & command, String & response);
        String multiAudioControl(const String & source, const String & channel, const String & volume, String & response);
        String multiSetVolatile(const JsonVariant & json);

        MultiStatus getMultiStatus() const;

        bool isMultiActive() const { return multiActive; }
        
        #endif

        void stopAll();


    protected:

        bool showerGuardActive;
        bool keyboxActive;
        bool audioActive;
        bool rfidLockActive;
        bool proportionalActive;
        bool zero2tenActive;
        bool mainsProbeActive;
        bool multiActive;
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

String AutonomTaskManager::rfidLockProgram(const String & code_str, uint16_t timeout)
{
    return rfid_lock_program(code_str, timeout);
}

String AutonomTaskManager::rfidLockAddCode(const String & name_str, const String & code_str, const std::vector<String> & locks, 
                                           const String & type_str)
{
    String r;

    RfidLockConfig::Codes::Code::Type type = RfidLockConfig::Codes::Code::str_2_type(type_str.c_str());

    if (type == RfidLockConfig::Codes::Code::Type(-1))
    {
        r = "Invalid type";
    }
    else
    {
        RfidLockConfig::Codes::Code code;
        code.type = type;
        code.value = code_str;
        code.locks = locks;

        rfid_lock_add_code(name_str, code);
    }

    return r;
}

String AutonomTaskManager::rfidLockDeleteCode(const String & name_str)
{
    return rfid_lock_delete_code(name_str);
}

String AutonomTaskManager::rfidLockDeleteAllCodes()
{
    String r;

    r = rfid_lock_delete_all_codes();

    return r;
}

String AutonomTaskManager::rfidLockUnlock(const String & lock_channel_str)
{
    return rfid_lock_unlock(lock_channel_str);
}

void AutonomTaskManager::rfidLockGetCodes(JsonVariant & json_variant)
{
    TRACE("rfidLockGetCodes")
    rfid_lock_get_codes(json_variant);
}

RfidLockStatus AutonomTaskManager::getRfidLockStatus() const
{
    TRACE("getRfidLockStatus")
    return get_rfid_lock_status();
}

#endif // INCLUDE_RFIDLOCK

#ifdef INCLUDE_PROPORTIONAL

void AutonomTaskManager::startProportional(const ProportionalConfig & config)
{
    TRACE("Starting autonom task proportional")
    start_proportional_task(config);
    proportionalActive = true;
}

void AutonomTaskManager::stopProportional()
{
    TRACE("stopping autonom task proportional")
    stop_proportional_task();
    proportionalActive = false;
}

void AutonomTaskManager::reconfigureProportional(const ProportionalConfig & config)
{
    TRACE("reconfiguring autonom task proportional")
    reconfigure_proportional(config);
}

ProportionalStatus AutonomTaskManager::getProportionalStatus() const
{
    TRACE("getProportionalStatus")
    return get_proportional_status();
}

String AutonomTaskManager::proportionalCalibrate(const String & channel_str)
{
    TRACE("proportionalCalibrate")
    return proportional_calibrate(channel_str);
}

String AutonomTaskManager::proportionalActuate(const String & channel_str, const String & value_str, 
                                               const String & ref_str)
{
    TRACE("proportionalActuate")
    return proportional_actuate(channel_str, value_str, ref_str);
}

#endif // INCLUDE_PROPORTIONAL


#ifdef INCLUDE_ZERO2TEN

void AutonomTaskManager::startZero2ten(const Zero2tenConfig & config)
{
    TRACE("Starting autonom task zero2ten")
    start_zero2ten_task(config);
    zero2tenActive = true;
}

void AutonomTaskManager::stopZero2ten()
{
    TRACE("stopping autonom task zero2ten")
    stop_zero2ten_task();
    zero2tenActive = false;
}

void AutonomTaskManager::reconfigureZero2ten(const Zero2tenConfig & config)
{
    TRACE("reconfiguring autonom task zero2ten")
    reconfigure_zero2ten(config);
}

Zero2tenStatus AutonomTaskManager::getZero2tenStatus() const
{
    TRACE("getZero2tenStatus")
    return get_zero2ten_status();
}

String AutonomTaskManager::zero2tenCalibrateInput(const String & channel_str, const String & value_str)
{
    TRACE("zero2tenCalibrateInput")
    return zero2ten_calibrate_input(channel_str, value_str);
}

String AutonomTaskManager::zero2tenInput(const String & channel_str, String & value_str)
{
    TRACE("zero2tenInput")
    return zero2ten_input(channel_str, value_str);
}

String AutonomTaskManager::zero2tenCalibrateOutput(const String & channel_str, const String & value_str)
{
    TRACE("zero2tenCalibrateOutput")
    return zero2ten_calibrate_output(channel_str, value_str);
}

String AutonomTaskManager::zero2tenOutput(const String & channel_str, const String & value_str)
{
    TRACE("zero2tenOutput")
    return zero2ten_output(channel_str, value_str);
}


#endif // INCLUDE_ZERO2TEN

#ifdef INCLUDE_MAINSPROBE

void AutonomTaskManager::startMainsProbe(const MainsProbeConfig & config)
{
    TRACE("Starting autonom task mainsProbe")
    start_mains_probe_task(config);
    mainsProbeActive = true;
}

void AutonomTaskManager::stopMainsProbe()
{
    TRACE("stopping autonom task mainsProbe")
    stop_mains_probe_task();
    mainsProbeActive = false;
}

void AutonomTaskManager::reconfigureMainsProbe(const MainsProbeConfig & config)
{
    TRACE("reconfiguring autonom task mainsProbe")
    reconfigure_mains_probe(config);
}

MainsProbeStatus AutonomTaskManager::getMainsProbeStatus() const
{
    TRACE("getZero2tenStatus")
    return get_mains_probe_status();
}

String AutonomTaskManager::mainsProbeCalibrateV(const String & addr_str, const String & channel_str, const String & value_str)
{
    TRACE("mainsProbeCalibrateV")
    return mains_probe_calibrate_v(addr_str, channel_str, value_str);
}

String AutonomTaskManager::mainsProbeCalibrateAHigh(const String & addr_str, const String & channel_str, const String & value_str)
{
    TRACE("mainsProbeCalibrateAHigh")
    return mains_probe_calibrate_a_high(addr_str, channel_str, value_str);
}

String AutonomTaskManager::mainsProbeCalibrateALow(const String & channel_str, const String & value_str)
{
    TRACE("mainsProbeCalibrateALow")
    return mains_probe_calibrate_a_low(channel_str, value_str);
}

String AutonomTaskManager::mainsProbeInputV(const String & addr_str, const String & channel_str, String & value_str)
{
    TRACE("mainsProbeInputV")
    return mains_probe_input_v(addr_str, channel_str, value_str);
}

String AutonomTaskManager::mainsProbeInputAHigh(const String & addr_str, const String & channel_str, String & value_str)
{
    TRACE("mainsProbeInputAHigh")
    return mains_probe_input_a_high(addr_str, channel_str, value_str);
}

String AutonomTaskManager::mainsProbeInputALow(const String & channel_str, String & value_str)
{
    TRACE("mainsProbeInputALow")
    return mains_probe_input_a_low(channel_str, value_str);
}

void AutonomTaskManager::mainsProbeGetCalibrationData(JsonVariant & json_variant)
{
    TRACE("mainsProbeGetCalibrationData")
    mains_probe_get_calibration_data(json_variant);
}

String AutonomTaskManager::mainsProbeImportCalibrationData(const JsonVariant & json)
{
    TRACE("mainsProbeImportCalibrationData")
    return mains_probe_import_calibration_data(json);
}

#endif // INCLUDE_MAINSPROBE


#ifdef INCLUDE_MULTI

void AutonomTaskManager::startMulti(const MultiConfig & config)
{
    TRACE("Starting autonom task multi")
    start_multi_task(config);
    multiActive = true;
}

void AutonomTaskManager::stopMulti()
{
    TRACE("stopping autonom task multi")
    stop_multi_task();
    multiActive = false;
}

void AutonomTaskManager::reconfigureMulti(const MultiConfig & config)
{
    TRACE("reconfiguring autonom task multi")
    reconfigure_multi(config);
}

String AutonomTaskManager::multiUartCommand(const String & command, String & response)
{
    TRACE("multiUartCommand")
    return multi_uart_command(command, response);
}

String AutonomTaskManager::multiAudioControl(const String & source, const String & channel, const String & volume, String & response)
{
    TRACE("multiAudioControl")
    return multi_audio_control(source, channel, volume, response);
}

String AutonomTaskManager::multiSetVolatile(const JsonVariant & json)
{
    TRACE("multiSetVolatile")
    return multi_set_volatile(json);
}

MultiStatus AutonomTaskManager::getMultiStatus() const
{
    TRACE("getMultiStatus")
    return get_multi_status();
}

#endif // INCLUDE_MULTI


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

    # if INCLUDE_RFIDLOCK

    if (rfidLockActive)
    {
        stopRfidLock();
    }

    #endif

    # if INCLUDE_PROPORTIONAL

    if (proportionalActive)
    {
        stopProportional();
    }

    #endif

    # if INCLUDE_ZERO2TEN

    if (zero2tenActive)
    {
        stopZero2ten();
    }

    #endif

    # if INCLUDE_MAINSPROBE

    if (mainsProbeActive)
    {
        stopMainsProbe();
    }

    #endif

    # if INCLUDE_MULTI

    if (multiActive)
    {
        stopMulti();
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
        case ftProportional:
            return "proportional";
        case ftZero2ten:
            return "zero2ten";
        case ftMainsProbe:
            return "mains-probe";
        case ftMulti:
            return "multi";
        default:
            return "<unknown>";    
    }
}

String setupAutonom(const JsonVariant & json) 
{
    TRACE("setupAutonom")

    Lock lock(AutonomConfigVolumeSemaphore);
    EpromImage configVolume(AUTONOM_CONFIG_VOLUME);

    EpromImage currentConfigVolume(AUTONOM_CONFIG_VOLUME);
    currentConfigVolume.read();

    char buf[512];

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

    #ifdef INCLUDE_PROPORTIONAL
    ProportionalConfig proportionalConfig;
    #endif

    #ifdef INCLUDE_ZERO2TEN
    Zero2tenConfig zero2tenConfig;
    #endif

    #ifdef INCLUDE_MAINSPROBE
    MainsProbeConfig mainsProbeConfig;
    #endif

    #ifdef INCLUDE_MULTI
    MultiConfig multiConfig;
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
                            configVolume.blocks.insert({(uint8_t) ftShowerGuard, buffer});
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
                            configVolume.blocks.insert({(uint8_t) ftKeybox, buffer});
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
                            configVolume.blocks.insert({(uint8_t) ftAudio, buffer});
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

                            /*
                            // we need to preserve codes while reconfiguring, so have to load from the
                            // currentConfigVolume first

                            for (auto it = currentConfigVolume.blocks.begin(); it != currentConfigVolume.blocks.end(); ++it)
                            {
                                const char * function_type_str = function_type_2_str((FunctionType) it->first);

                                if(it->first == ftRfidLock)
                                {
                                    std::istringstream is(it->second);
                                    TRACE("EPROM image, found ftRfidLock block")

                                    RfidLockConfig current_config;
                                    
                                    if (current_config.from_eprom(is) == true)
                                    {
                                        TRACE("Config is_valid=%s", (current_config.is_valid() ? "true" : "false"))
                                        TRACE("Config %s", current_config.as_string().c_str())

                                        TRACE("codes %s", current_config.codes.as_string().c_str())

                                        rfidLockConfig.codes = current_config.codes;
                                    }
                                    else
                                    {
                                        ERROR("Failed to read codes from current config, codes will be lost")
                                    }

                                    break;
                                }
                            }
                            */

                            // do not stop saving if current codes retrieval fails - just inform

                            std::ostringstream os;

                            rfidLockConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            configVolume.blocks.insert({(uint8_t) ftRfidLock, buffer});
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

                    #endif // INCLUDE_RFIDLOCK
                }
                else
                if (function == "proportional")
                {
                    #ifdef INCLUDE_PROPORTIONAL

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        proportionalConfig.from_json(config_json);

                        TRACE("function %s: proportionalConfig.is_valid=%s", function.c_str(), (proportionalConfig.is_valid() ? "true" : "false"))
                        TRACE(proportionalConfig.as_string().c_str())

                        if (proportionalConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            proportionalConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            configVolume.blocks.insert({(uint8_t) ftProportional, buffer});
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

                    #endif // INCLUDE_PROPORTIONAL
                }
                else
                if (function == "zero2ten")
                {
                    #ifdef INCLUDE_ZERO2TEN

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        zero2tenConfig.from_json(config_json);

                        TRACE("function %s: zero2tenConfig.is_valid=%s", function.c_str(), (zero2tenConfig.is_valid() ? "true" : "false"))
                        TRACE(zero2tenConfig.as_string().c_str())

                        if (zero2tenConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            zero2tenConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            configVolume.blocks.insert({(uint8_t) ftZero2ten, buffer});
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

                    #endif // INCLUDE_ZERO2TEN
                }
                else
                if (function == "mains-probe")
                {
                    #ifdef INCLUDE_MAINSPROBE

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        mainsProbeConfig.from_json(config_json);

                        TRACE("function %s: mainsProbeConfig.is_valid=%s", function.c_str(), (mainsProbeConfig.is_valid() ? "true" : "false"))
                        TRACE(mainsProbeConfig.as_string().c_str())

                        if (mainsProbeConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            mainsProbeConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            configVolume.blocks.insert({(uint8_t) ftMainsProbe, buffer});
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

                    #endif // INCLUDE_ZERO2TEN
                }
                else
                if (function == "multi")
                {
                    #ifdef INCLUDE_MULTI

                    if (_json.containsKey("config"))
                    {
                        DEBUG("contains config")
                        const JsonVariant & config_json = _json["config"];

                        multiConfig.from_json(config_json);

                        TRACE("function %s: multiConfig.is_valid=%s", function.c_str(), (multiConfig.is_valid() ? "true" : "false"))
                        TRACE(multiConfig.as_string().c_str())

                        if (multiConfig.is_valid())
                        {
                            TRACE("adding function %s to EEPROM image", function.c_str())

                            std::ostringstream os;

                            multiConfig.to_eprom(os);
                            
                            std::string buffer = os.str();
                            TRACE("block size %d", (int) os.tellp())
                            configVolume.blocks.insert({(uint8_t) ftMulti, buffer});
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

                    #endif // INCLUDE_MULTI
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

        std::vector<uint8_t> added, removed, changed;

        if (configVolume.diff(currentConfigVolume, & added, & removed, & changed) == true)
        {
            TRACE("New autonom configuration is different from one stored in EEPROM: updating EEPROM image")
            configVolume.write();

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
                else
                if (*it == ftProportional)
                {
                    #ifdef INCLUDE_PROPORTIONAL
                    autonomTaskManager.startProportional(proportionalConfig);
                    #endif
                }
                else
                if (*it == ftZero2ten)
                {
                    #ifdef INCLUDE_ZERO2TEN
                    autonomTaskManager.startZero2ten(zero2tenConfig);
                    #endif
                }
                else
                if (*it == ftMainsProbe)
                {
                    #ifdef INCLUDE_MAINSPROBE
                    autonomTaskManager.startMainsProbe(mainsProbeConfig);
                    #endif
                }
                else
                if (*it == ftMulti)
                {
                    #ifdef INCLUDE_MULTI
                    autonomTaskManager.startMulti(multiConfig);
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
                else
                if (*it == ftProportional)
                {
                    #ifdef INCLUDE_PROPORTIONAL
                    autonomTaskManager.stopProportional();
                    #endif
                }
                else
                if (*it == ftZero2ten)
                {
                    #ifdef INCLUDE_ZERO2TEN
                    autonomTaskManager.stopZero2ten();
                    #endif
                }
                else
                if (*it == ftMainsProbe)
                {
                    #ifdef INCLUDE_MAINSPROBE
                    autonomTaskManager.stopMainsProbe();
                    #endif
                }
                else
                if (*it == ftMulti)
                {
                    #ifdef INCLUDE_MULTI
                    autonomTaskManager.stopMulti();
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
                else
                if (*it == ftProportional)
                {
                    #ifdef INCLUDE_PROPORTIONAL
                    autonomTaskManager.reconfigureProportional(proportionalConfig);
                    #endif
                }
                else
                if (*it == ftZero2ten)
                {
                    #ifdef INCLUDE_ZERO2TEN
                    autonomTaskManager.reconfigureZero2ten(zero2tenConfig);
                    #endif
                }
                else
                if (*it == ftMainsProbe)
                {
                    #ifdef INCLUDE_MAINSPROBE
                    autonomTaskManager.reconfigureMainsProbe(mainsProbeConfig);
                    #endif
                }
                else
                if (*it == ftMulti)
                {
                    #ifdef INCLUDE_MULTI
                    autonomTaskManager.reconfigureMulti(multiConfig);
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

    // remove all functions/config from EEPROM 

    {Lock lock(AutonomConfigVolumeSemaphore);
    EpromImage configVolume(AUTONOM_CONFIG_VOLUME), currentConfigVolume(AUTONOM_CONFIG_VOLUME); 
    currentConfigVolume.read();

    std::vector<uint8_t> added, removed, changed;

    if (configVolume.diff(currentConfigVolume, & added, & removed, & changed) == true)
    {
        TRACE("New autonom configuration is different from one stored in EEPROM: updating EEPROM image")
        configVolume.write();
    }}

    // remove all data from EEPROM

    {Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME); 
    dataVolume.write();}

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

String actionAutonomRfidLockProgram(const String & code_str, uint16_t timeout)
{
    #ifdef INCLUDE_RFIDLOCK
    if (autonomTaskManager.isRfidLockActive())
    {
        return autonomTaskManager.rfidLockProgram(code_str, timeout);
    }
    else
    {
        return "RfidLock not active";
    }
    
    #else

    return "RfidLock is not built in currrent module";

    #endif // INCLUDE_RFIDLOCK
}



String actionAutonomRfidLockAddCode(const String & name_str, const String & code_str, const std::vector<String> & locks, 
                                    const String & type_str)
{
    #ifdef INCLUDE_RFIDLOCK

    TRACE("Adding RfidLock code")

    if (autonomTaskManager.isRfidLockActive())
    {
        String r = autonomTaskManager.rfidLockAddCode(name_str, code_str, locks, type_str);
        return r;
    }
    else
    {
        String r = "RfidLock not active";
        ERROR(r.c_str())
        return r;
    }

    #else

    return "RfidLock is not built in currrent module";

    #endif // INCLUDE_RFIDLOCK
}

String actionAutonomRfidLockDeleteCode(const String & name_str)
{
    #ifdef INCLUDE_RFIDLOCK

    TRACE("Deleting RfidLock code")

    if (autonomTaskManager.isRfidLockActive())
    {
        String r = autonomTaskManager.rfidLockDeleteCode(name_str);
        return r;
    }
    else
    {
        String r = "RfidLock not active";
        ERROR(r.c_str())
        return r;
    }

    #else

    return "RfidLock is not built in currrent module";

    #endif // INCLUDE_RFIDLOCK
}

String actionAutonomRfidLockDeleteAllCodes()
{
    #ifdef INCLUDE_RFIDLOCK

    TRACE("Deleting all RfidLock codes")

    if (autonomTaskManager.isRfidLockActive())
    {
        String r = autonomTaskManager.rfidLockDeleteAllCodes();
        return r;
    }
    else
    {
        String r = "RfidLock not active";
        ERROR(r.c_str())
        return r;
    }

    #else

    return "RfidLock is not built in currrent module";

    #endif // INCLUDE_RFIDLOCK
}

String actionAutonomRfidLockUnlock(const String & lock_channel_str)
{
    #ifdef INCLUDE_RFIDLOCK

    TRACE("Unlocking RfidLock")

    if (autonomTaskManager.isRfidLockActive())
    {
        String r = autonomTaskManager.rfidLockUnlock(lock_channel_str);
        return r;
    }
    else
    {
        String r = "RfidLock not active";
        ERROR(r.c_str())
        return r;
    }

    #else

    return "RfidLock is not built in currrent module";

    #endif // INCLUDE_RFIDLOCK
}

void getAutonomRfidLockCodes(JsonVariant & json_variant)
{
    #ifdef INCLUDE_RFIDLOCK
    if (autonomTaskManager.isRfidLockActive())
    {
        autonomTaskManager.rfidLockGetCodes(json_variant);
    }    
    #endif // INCLUDE_RFIDLOCK
}

String actionAutonomProportionalCalibrate(const String & channel_str)
{
    #ifdef INCLUDE_PROPORTIONAL
    if (autonomTaskManager.isProportionalActive())
    {
        return autonomTaskManager.proportionalCalibrate(channel_str);
    }
    else
    {
        return "proportional not active";
    }
    
    #else

    return "proportional is not built in currrent module";

    #endif // INCLUDE_PROPORTIONAL
}

String actionAutonomProportionalActuate(const String & channel_str, const String & value_str, 
                                        const String & ref_str)
{
    #ifdef INCLUDE_PROPORTIONAL
    if (autonomTaskManager.isProportionalActive())
    {
        return autonomTaskManager.proportionalActuate(channel_str, value_str, ref_str);
    }
    else
    {
        return "proportional not active";
    }
    
    #else

    return "proportional is not built in currrent module";

    #endif // INCLUDE_PROPORTIONAL
}

String actionAutonomZero2tenCalibrateInput(const String & channel_str, const String & value_str)
{
    #ifdef INCLUDE_ZERO2TEN
    if (autonomTaskManager.isZero2tenActive())
    {
        return autonomTaskManager.zero2tenCalibrateInput(channel_str, value_str);
    }
    else
    {
        return "zero2ten not active";
    }
    
    #else

    return "zero2ten is not built in currrent module";

    #endif // INCLUDE_ZERO2TEN
}

String actionAutonomZero2tenInput(const String & channel_str, String & value_str)
{
    #ifdef INCLUDE_ZERO2TEN
    if (autonomTaskManager.isZero2tenActive())
    {
        return autonomTaskManager.zero2tenInput(channel_str, value_str);
    }
    else
    {
        return "zero2ten not active";
    }
    
    #else

    return "zero2ten is not built in currrent module";

    #endif // INCLUDE_ZERO2TEN
}

String actionAutonomZero2tenCalibrateOutput(const String & channel_str, const String & value_str)
{
    #ifdef INCLUDE_ZERO2TEN
    if (autonomTaskManager.isZero2tenActive())
    {
        return autonomTaskManager.zero2tenCalibrateOutput(channel_str, value_str);
    }
    else
    {
        return "zero2ten not active";
    }
    
    #else

    return "zero2ten is not built in currrent module";

    #endif // INCLUDE_ZERO2TEN
}

String actionAutonomZero2tenOutput(const String & channel_str, const String & value_str)
{
    #ifdef INCLUDE_ZERO2TEN
    if (autonomTaskManager.isZero2tenActive())
    {
        return autonomTaskManager.zero2tenOutput(channel_str, value_str);
    }
    else
    {
        return "zero2ten not active";
    }
    
    #else

    return "zero2ten is not built in currrent module";

    #endif // INCLUDE_ZERO2TEN
}

String actionAutonomMainsProbeCalibrateV(const String & addr_str, const String & channel_str, const String & value_str)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        return autonomTaskManager.mainsProbeCalibrateV(addr_str, channel_str, value_str);
    }
    else
    {
        return "mains-probe not active";
    }
    
    #else

    return "mains-probe is not built in currrent module";

    #endif // INCLUDE_MAINSPROBE
}

String actionAutonomMainsProbeCalibrateAHigh(const String & addr_str, const String & channel_str, const String & value_str)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        return autonomTaskManager.mainsProbeCalibrateAHigh(addr_str, channel_str, value_str);
    }
    else
    {
        return "mains-probe not active";
    }
    
    #else

    return "mains-probe is not built in currrent module";

    #endif // INCLUDE_MAINSPROBE
}

String actionAutonomMainsProbeCalibrateALow(const String & channel_str, const String & value_str)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        return autonomTaskManager.mainsProbeCalibrateALow(channel_str, value_str);
    }
    else
    {
        return "mains-probe not active";
    }
    
    #else

    return "mains-probe is not built in currrent module";

    #endif // INCLUDE_MAINSPROBE
}

String actionAutonomMainsProbeInputV(const String & addr_str, const String & channel_str, String & value_str)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        return autonomTaskManager.mainsProbeInputV(addr_str, channel_str, value_str);
    }
    else
    {
        return "mains-probe not active";
    }
    
    #else

    return "mains-probe is not built in currrent module";

    #endif // INCLUDE_MAINSPROBE
}

String actionAutonomMainsProbeInputAHigh(const String & addr_str, const String & channel_str, String & value_str)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        return autonomTaskManager.mainsProbeInputAHigh(addr_str, channel_str, value_str);
    }
    else
    {
        return "mains-probe not active";
    }
    
    #else

    return "mains-probe is not built in currrent module";

    #endif // INCLUDE_MAINSPROBE
}

String actionAutonomMainsProbeInputALow(const String & channel_str, String & value_str)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        return autonomTaskManager.mainsProbeInputALow(channel_str, value_str);
    }
    else
    {
        return "mains-probe not active";
    }
    
    #else

    return "mains-probe is not built in currrent module";

    #endif // INCLUDE_MAINSPROBE
}

void getAutonomMainsProbeCalibrationData(JsonVariant & json_variant)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        autonomTaskManager.mainsProbeGetCalibrationData(json_variant);
    }    
    #endif // INCLUDE_MAINSPROBE
}

String actionAutonomMainsProbeImportCalibrationData(const JsonVariant & json)
{
    #ifdef INCLUDE_MAINSPROBE
    if (autonomTaskManager.isMainsProbeActive())
    {
        return autonomTaskManager.mainsProbeImportCalibrationData(json);
    }
    else
    {
        return "mains-probe not active";
    }
    
    #else

    return "mains-probe is not built in currrent module";

    #endif // INCLUDE_MAINSPROBE
}

String actionAutonomMultiUartCommand(const String & command, String & response)
{
    #ifdef INCLUDE_MULTI
    if (autonomTaskManager.isMultiActive())
    {
        return autonomTaskManager.multiUartCommand(command, response);
    }
    else
    {
        return "multi not active";
    }
    
    #else

    return "multi is not built in currrent module";

    #endif // INCLUDE_MULTI
}

String actionAutonomMultiAudioControl(const String & source, const String & channel, const String & volume, String & response)
{
    #ifdef INCLUDE_MULTI
    TRACE("multiAudioControl")
    if (autonomTaskManager.isMultiActive())
    {
        return autonomTaskManager.multiAudioControl(source, channel, volume, response);
    }
    else
    {
        return "multi not active";
    }
    
    #else

    return "multi is not built in currrent module";

    #endif // INCLUDE_MULTI
}

String actionAutonomMultiSetVolatile(const JsonVariant & json)
{
    #ifdef INCLUDE_MULTI
    if (autonomTaskManager.isMultiActive())
    {
        return autonomTaskManager.multiSetVolatile(json);
    }
    else
    {
        return "multi not active";
    }
    
    #else

    return "multi is not built in currrent module";

    #endif // INCLUDE_MULTI
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

  #ifdef INCLUDE_PROPORTIONAL
  if (autonomTaskManager.isProportionalActive())
  {
        ProportionalStatus status = autonomTaskManager.getProportionalStatus();
        status.to_json(json);
  }

  #endif

  #ifdef INCLUDE_ZERO2TEN
  if (autonomTaskManager.isZero2tenActive())
  {
        Zero2tenStatus status = autonomTaskManager.getZero2tenStatus();
        status.to_json(json);
  }

  #endif

  #ifdef INCLUDE_MAINSPROBE
  if (autonomTaskManager.isMainsProbeActive())
  {
        MainsProbeStatus status = autonomTaskManager.getMainsProbeStatus();
        status.to_json(json);
  }

  #endif

  #ifdef INCLUDE_MULTI
  if (autonomTaskManager.isMultiActive())
  {
        MultiStatus status = autonomTaskManager.getMultiStatus();
        status.to_json(json);
  }

  #endif
}

void restoreAutonom() 
{
  TRACE("restoreAutonom")

    Lock lock(AutonomConfigVolumeSemaphore);
    EpromImage configVolume(AUTONOM_CONFIG_VOLUME);
  
  if (configVolume.read() == true)
  {
      for (auto it = configVolume.blocks.begin(); it != configVolume.blocks.end(); ++it)
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
                    //TRACE("Codes %s", config.codes.as_string().c_str())

                    autonomTaskManager.startRfidLock(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_RFIDLOCK
                break;

              case ftProportional:
                
                #ifdef INCLUDE_PROPORTIONAL
                {ProportionalConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startProportional(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_PROPORTIONAL
                break;

              case ftZero2ten:
                
                #ifdef INCLUDE_ZERO2TEN
                {Zero2tenConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startZero2ten(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_ZERO2TEN
                break;

              case ftMainsProbe:
                
                #ifdef INCLUDE_MAINSPROBE
                {MainsProbeConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startMainsProbe(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_MAINSPROBE
                break;

              case ftMulti:
                
                #ifdef INCLUDE_MULTI
                {MultiConfig config;
                
                if (config.from_eprom(is) == true)
                {
                    TRACE("Config is_valid=%s", (config.is_valid() ? "true" : "false"))
                    TRACE("Config %s", config.as_string().c_str())

                    autonomTaskManager.startMulti(config);
                }
                else
                {
                    TRACE("Config read failure")
                }}
                #endif // INCLUDE_MULTI
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
