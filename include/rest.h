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

String restActionAutonomProportionalCalibrate(const String & channel_str);
String restActionAutonomProportionalActuate(const String & channel_str, const String & value_str, 
                                            const String & ref_str);

String restActionAutonomZero2tenCalibrateInput(const String & channel_str, const String & value_str);
String restActionAutonomZero2tenInput(const String & channel_str, String & value_str);
String restActionAutonomZero2tenCalibrateOutput(const String & channel_str, const String & value_str);
String restActionAutonomZero2tenOutput(const String & channel_str, const String & value_str);

String restActionAutonomPhaseChangerCalibrateV(const String & channel_str, const String & value_str);
String restActionAutonomPhaseChangerCalibrateIHigh(const String & channel_str, const String & value_str);
String restActionAutonomPhaseChangerCalibrateILow(const String & channel_str, const String & value_str);
String restActionAutonomPhaseChangerInputV(const String & channel_str, String & value_str);
String restActionAutonomPhaseChangerInputIHigh(const String & channel_str, String & value_str);
String restActionAutonomPhaseChangerInputILow(const String & channel_str, String & value_str);

String restReset(const String & resetStamp);
String restResetPm(const String & resetStamp);

String restGet(const String & resetStamp);
String restGetPm(const String & resetStamp);
String restGetAutonom();

String restPopLog();

#define REST_VERSION  "1.3" 