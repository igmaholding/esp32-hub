
#define TRACE(...) (Serial.printf("TRACE "), Serial.printf(__VA_ARGS__),Serial.println(""));
#define DEBUG(...) (Serial.printf("DEBUG "), Serial.printf(__VA_ARGS__),Serial.println(""));
#define ERROR(...) (Serial.printf("ERROR "), Serial.printf(__VA_ARGS__),Serial.println(""));

//#define TRACE(...) ;
//#define DEBUG(...) ;
//#define ERROR(...) ;
