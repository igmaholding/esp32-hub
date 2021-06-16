
String restPing(bool include_info);
String restWifiInfo();

String restSetup(const String & body, const String & resetStamp);
String restSetupPm(const String & body, const String & resetStamp);
String restSetupAutonom(const String & body);

String restCleanup();
String restCleanupPm();

String restReset(const String & resetStamp);
String restResetPm(const String & resetStamp);

String restGet(const String & resetStamp);
String restGetPm(const String & resetStamp);

String restPopLog();

#define REST_VERSION  "1.2" 