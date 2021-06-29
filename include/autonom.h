#include <ArduinoJson.h>

void setupAutonom(const JsonVariant &);
void cleanupAutonom();

void restoreAutonom(); // from EPROM

enum FunctionType
{
    ftShowerGuard = 1,
};

const char * function_type_2_str(FunctionType);