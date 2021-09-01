#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <map>

class WifiNetwork
{
  public:

    WifiNetwork() 
    {
        signalStrength = 0;
        channel = 0;
        authMode = WIFI_AUTH_OPEN;
    }

    WifiNetwork(int scanIndex)
    {
        name = WiFi.SSID(scanIndex);
        signalStrength = WiFi.RSSI(scanIndex);
        channel = WiFi.channel(scanIndex);
        authMode = WiFi.encryptionType(scanIndex);
    }

    static const char * authMode_2_str(wifi_auth_mode_t authMode);

    String to_string() const;

    String name;
    int signalStrength;
    int channel;
    wifi_auth_mode_t authMode;

    String password;
};


class WifiHandler 
{
  public:

      WifiHandler()
      {
      }

      bool scan();
      bool connectStrongest(const std::vector<std::pair<String, String>>);
      bool connect(const String name, const String password);
      void disconnect();

      String getWifiInfo() const;
      
      bool isConnected() const;

      std::map<String, WifiNetwork> scannedNetworks;
      WifiNetwork connectedNetwork;
      String lastFailedNetworkName;
};