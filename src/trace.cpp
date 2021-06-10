#include <Arduino.h>
#include <trace.h>

char __trace_buf[1024];
char * _trace_buf = __trace_buf;