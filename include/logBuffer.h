#include <Arduino.h>
#include <ArduinoJson.h>

String tm_2_str(const tm &);
String time_t_2_str(time_t _time_t);

void popLog(JsonVariant &);

void writeLog(const char *);

void traceLog(const char *);
void debugLog(const char *);
void errorLog(const char *);
