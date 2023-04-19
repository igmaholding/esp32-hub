#include <ArduinoJson.h>

String setupAutonom(const JsonVariant &); // returns non-empty string with error message if error
void cleanupAutonom();
void getAutonom(JsonVariant &);

void restoreAutonom(); // from EPROM

String actionAutonomKeyboxActuate(const String & channel_str);
String actionAutonomRfidLockProgram(const String & code_str);

enum FunctionType
{
    ftShowerGuard = 1,
    ftKeybox =      2,
    ftAudio =       3,
    ftRfidLock =    4
};

const char * function_type_2_str(FunctionType);