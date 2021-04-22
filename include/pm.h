#include <ArduinoJson.h>

void pingPm(JsonVariant &);
void setupPm(const JsonVariant &, const String & resetStamp);
void cleanupPm();
void resetPm(const String & resetStamp);
void getPm(JsonVariant &, const String & resetStamp);