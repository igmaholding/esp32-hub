#include <Arduino.h>
#include <WiFi.h>
#include <wifiHandler.h>
#include <onboardLed.h>
#include <trace.h>


//const char *SSID = "dlink-15A8";
//const char *PWD = "uiacw71798";

//const char *SSID = "igmagarpgarden2";
//const char *PWD = "IGMASoder1";




const char * WifiNetwork::authMode_2_str(wifi_auth_mode_t _authMode)
{
  switch (_authMode) 
  {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
    default: return "";    
  }
}


String WifiNetwork::to_string() const
{
  return String("name: ") + name + 
         //String(", password: ") + password + 
         String(", signalStrength: ") + String(signalStrength) + 
         String(", channel: ") + String(channel) + 
         String(", authMode: ") + authMode_2_str(authMode); 
}


bool WifiHandler::scan()
{
  scannedNetworks.clear();

  int numNetworks = WiFi.scanNetworks();

  if (numNetworks < 0)
  {
    ERROR("Scan for WIFI networks failed")
    return false;
  }
  else
  {
    TRACE("Found %d WIFI networks:", numNetworks)

    for (int i=0; i<numNetworks; ++i)
    {
      WifiNetwork wifiNetwork(i);
      scannedNetworks[wifiNetwork.name] = wifiNetwork;

      TRACE("Network %d: %s", i, wifiNetwork.to_string().c_str()) 
    }
  }
  return true;
}


bool WifiHandler::connectStrongest(const std::vector<std::pair<String, String>> knownNetworks)
{
  scan();

  if (scannedNetworks.empty() == false)
  {
    TRACE("Matching scanned networks against known networks")

    std::map<int, WifiNetwork> matched; 
    
    for (auto iterator = knownNetworks.begin(); iterator != knownNetworks.end(); ++iterator)
    {
      auto scannedNetworkIterator = scannedNetworks.find(iterator->first);
      {
        if (scannedNetworkIterator != scannedNetworks.end())
        {
          scannedNetworkIterator->second.password = iterator->second;
          matched[scannedNetworkIterator->second.signalStrength] = scannedNetworkIterator->second;
          TRACE("Matched network: %s", scannedNetworkIterator->second.to_string().c_str()) 
        }
      }    
    }

    if (matched.empty() == false)
    {
      // start from the strongest but try all until success if the stronger ones are having problems (e.g. password incorrect)      

      for (auto matchedRIterator = matched.rbegin(); matchedRIterator != matched.rend(); --matchedRIterator)
      {
        WifiNetwork selectedNetwork = matchedRIterator->second;
      
        if (connect(selectedNetwork.name, selectedNetwork.password) == true)
        {
           connectedNetwork = selectedNetwork; 
           return true;
        }
      }
    }
  }
  return false;
}


bool WifiHandler::connect(const String name, const String password) 
{
  TRACE("Connecting WIFI network: %s", name.c_str()) 
  WiFi.begin(name.c_str(), password.c_str());
  
  const int MAX_RETRIES = 10; 
  int retries = 0;

  while (true)
  {
    auto status = WiFi.status();
    
    if (status == WL_CONNECTED) 
    {
      Serial.print("Connected. IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }

    retries++;

    if (retries < MAX_RETRIES)
    {
      Serial.print(".");
      delay(500);
      continue;
    }

    if (status == WL_NO_SSID_AVAIL) 
    {
      ERROR("Connection failed, NO_SSID_AVAIL");
      return false;
    }

    if (status == WL_CONNECT_FAILED) 
    {
      ERROR("Connection failed, CONNECT_FAILED");
      return false;
    }

    break; 
  }
 
  ERROR("Connection failed, other status");
  return false;
}


void WifiHandler::disconnect() 
{
  WiFi.disconnect();
}


bool WifiHandler::isConnected() const
{
  return WiFi.status() == WL_CONNECTED;
}  


String WifiHandler::getWifiInfo() const
{
  String info;

  info += "*** SCANNED NETWORKS ***\n";

  for (auto iterator = scannedNetworks.begin(); iterator != scannedNetworks.end(); ++iterator)
  {
    info += iterator->second.to_string();
    info += "\n";
  }

  info += "*** CONNECTED NETWORK ***\n";
  info += connectedNetwork.to_string();
  info += "\n";

  return info;
}