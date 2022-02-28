#include <ArduinoJson.h>

String setupAutonom(const JsonVariant &); // returns non-empty string with error message if error
void cleanupAutonom();
void getAutonom(JsonVariant &);

void restoreAutonom(); // from EPROM

enum FunctionType
{
    ftShowerGuard = 1,
    ftKeyBox =      2,
};

const char * function_type_2_str(FunctionType);