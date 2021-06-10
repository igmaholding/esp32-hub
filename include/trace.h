
#include <logBuffer.h>

extern char * _trace_buf;

#define TRACE(...) (sprintf(_trace_buf, __VA_ARGS__), traceLog(_trace_buf));
#define DEBUG(...) (sprintf(_trace_buf, __VA_ARGS__), debugLog(_trace_buf));
#define ERROR(...) (sprintf(_trace_buf, __VA_ARGS__), errorLog(_trace_buf));

//#define TRACE(...) (Serial.printf("TRACE "), Serial.printf(__VA_ARGS__),Serial.println(""));
//#define DEBUG(...) (Serial.printf("DEBUG "), Serial.printf(__VA_ARGS__),Serial.println(""));
//#define ERROR(...) (Serial.printf("ERROR "), Serial.printf(__VA_ARGS__),Serial.println(""));

//#define TRACE(...) ;
//#define DEBUG(...) ;
//#define ERROR(...) ;
