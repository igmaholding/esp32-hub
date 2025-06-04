#include <ArduinoJson.h>
#include <vector>

String setupAutonom(const JsonVariant &); // returns non-empty string with error message if error
void cleanupAutonom();
void getAutonom(JsonVariant &);

void restoreAutonom(); // from EPROM

String actionAutonomKeyboxActuate(const String & channel_str);

String actionAutonomRfidLockProgram(const String & code_str, uint16_t timeout);
String actionAutonomRfidLockAdd(const String & name_str, const String & code_str, const std::vector<String> & locks, 
                                const String & type_str);

String actionAutonomProportionalCalibrate(const String & channel_str);
String actionAutonomProportionalActuate(const String & channel_str, const String & value_str, 
                                        const String & ref_str);

String actionAutonomZero2tenCalibrateInput(const String & channel_str, const String & value_str);
String actionAutonomZero2tenInput(const String & channel_str, String & value_str);
String actionAutonomZero2tenCalibrateOutput(const String & channel_str, const String & value_str);
String actionAutonomZero2tenOutput(const String & channel_str, const String & value_str);

String actionAutonomMainsProbeCalibrateV(const String & addr_str, const String & channel_str, const String & value_str);
String actionAutonomMainsProbeCalibrateAHigh(const String & addr_str, const String & channel_str, const String & value_str);
String actionAutonomMainsProbeCalibrateALow(const String & channel_str, const String & value_str);
String actionAutonomMainsProbeInputV(const String & addr_str, const String & channel_str, String & value_str);
String actionAutonomMainsProbeInputAHigh(const String & addr_str, const String & channel_str, String & value_str);
String actionAutonomMainsProbeInputALow(const String & channel_str, String & value_str);
void getAutonomMainsProbeCalibrationData(JsonVariant &);
String actionAutonomMainsProbeImportCalibrationData(const JsonVariant & json);

String actionAutonomMultiUartCommand(const String & command, String & response);
String actionAutonomMultiAudioControl(const String & source, const String & channel, const String & volume, String & response);


enum FunctionType
{
    ftShowerGuard =  1,
    ftKeybox =       2,
    ftAudio =        3,
    ftRfidLock =     4,
    ftProportional = 5,
    ftZero2ten =     6,
    ftMainsProbe = 7,
    ftMulti        = 8
};

const char * function_type_2_str(FunctionType);