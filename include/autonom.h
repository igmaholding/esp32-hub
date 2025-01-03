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

String actionAutonomPhaseChangerCalibrateV(const String & channel_str, const String & value_str);
String actionAutonomPhaseChangerCalibrateIHigh(const String & channel_str, const String & value_str);
String actionAutonomPhaseChangerCalibrateILow(const String & channel_str, const String & value_str);
String actionAutonomPhaseChangerInputV(const String & channel_str, String & value_str);
String actionAutonomPhaseChangerInputIHigh(const String & channel_str, String & value_str);
String actionAutonomPhaseChangerInputILow(const String & channel_str, String & value_str);


enum FunctionType
{
    ftShowerGuard =  1,
    ftKeybox =       2,
    ftAudio =        3,
    ftRfidLock =     4,
    ftProportional = 5,
    ftZero2ten =     6,
    ftPhaseChanger = 7
};

const char * function_type_2_str(FunctionType);