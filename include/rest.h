#include <vector>

String restPing(bool include_info);
String restWifiInfo();

String restSetup(const String & body, const String & resetStamp);
String restSetupPm(const String & body, const String & resetStamp);
String restSetupAutonom(const String & body);

String restCleanup();
String restCleanupPm();
String restCleanupAutonom();

String restActionAutonomKeyboxActuate(const String & channel_str);

String restActionAutonomRfidLockProgram(const String & code_str, uint16_t timeout);

String restActionAutonomRfidLockAdd(const String & name_str, const String & code_str, const std::vector<String> & locks, 
                                    const String & type_str);

String restReset(const String & resetStamp);
String restResetPm(const String & resetStamp);

String restGet(const String & resetStamp);
String restGetPm(const String & resetStamp);
String restGetAutonom();

String restPopLog();

#define REST_VERSION  "1.3" 