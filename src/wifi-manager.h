#ifndef _BLLEDWIFI_MANAGER
#define _BLLEDWIFI_MANAGER

#include <Arduino.h>
#include <ArduinoJson.h> 
#include <WiFi.h>

#include "filesystem.h"
#include "types.h"

bool shouldSaveConfig = true;
int connectionAttempts = 1;
int wifimode = 0;
uint8_t bssid[6] = {0};

void configModeCallback() {
  Serial.println(F("Entered config mode"));
  Serial.print(F("AP IP address: "));
  Serial.println(WiFi.softAPIP());
}
const char* wl_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
  }
  return "UNKNOWN";
}

void ReasontoString(int errorReason){
    Serial.printf("[WiFi-ConnectinError] Reason: %d - ", errorReason);

    switch (errorReason) {
        case WIFI_REASON_UNSPECIFIED:
            Serial.println("UNSPECIFIED");
            break;
        case WIFI_REASON_AUTH_EXPIRE:
            Serial.println("AUTH_EXPIRE");
            break;
        case WIFI_REASON_AUTH_LEAVE:
            Serial.println("AUTH_LEAVE");
            break;
        case WIFI_REASON_ASSOC_EXPIRE:
            Serial.println("ASSOC_EXPIRE");
            break;
        case WIFI_REASON_ASSOC_TOOMANY:
            Serial.println("ASSOC_TOOMANY");
            break;
        case WIFI_REASON_NOT_AUTHED:
            Serial.println("NOT_AUTHED");
            break;
        case WIFI_REASON_NOT_ASSOCED:
            Serial.println("NOT_ASSOCED");
            break;
        case WIFI_REASON_ASSOC_LEAVE:
            Serial.println("ASSOC_LEAVE");
            break;
        case WIFI_REASON_ASSOC_NOT_AUTHED:
            Serial.println("ASSOC_NOT_AUTHED");
            break;
        case WIFI_REASON_DISASSOC_PWRCAP_BAD:
            Serial.println("DISASSOC_PWRCAP_BAD");
            break;
        case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
            Serial.println("DISASSOC_SUPCHAN_BAD");
            break;
        case WIFI_REASON_IE_INVALID:
            Serial.println("IE_INVALID");
            break;
        case WIFI_REASON_MIC_FAILURE:
            Serial.println("MIC_FAILURE");
            break;
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            Serial.println("4WAY_HANDSHAKE_TIMEOUT");
            break;
        case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
            Serial.println("GROUP_KEY_UPDATE_TIMEOUT");
            break;
        case WIFI_REASON_IE_IN_4WAY_DIFFERS:
            Serial.println("IE_IN_4WAY_DIFFERS");
            break;
        case WIFI_REASON_GROUP_CIPHER_INVALID:
            Serial.println("GROUP_CIPHER_INVALID");
            break;
        case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
            Serial.println("PAIRWISE_CIPHER_INVALID");
            break;
        case WIFI_REASON_AKMP_INVALID:
            Serial.println("AKMP_INVALID");
            break;
        case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
            Serial.println("UNSUPP_RSN_IE_VERSION");
            break;
        case WIFI_REASON_INVALID_RSN_IE_CAP:
            Serial.println("INVALID_RSN_IE_CAP");
            break;
        case WIFI_REASON_802_1X_AUTH_FAILED:
            Serial.println("802_1X_AUTH_FAILED");
            break;
        case WIFI_REASON_CIPHER_SUITE_REJECTED:
            Serial.println("CIPHER_SUITE_REJECTED");
            break;
        case WIFI_REASON_BEACON_TIMEOUT:
            Serial.println("BEACON_TIMEOUT");
            break;
        case WIFI_REASON_NO_AP_FOUND:
            Serial.println("NO_AP_FOUND");
            break;
        case WIFI_REASON_AUTH_FAIL:
            Serial.println("AUTH_FAIL");
            break;
        case WIFI_REASON_ASSOC_FAIL:
            Serial.println("ASSOC_FAIL");
            break;
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            Serial.println("HANDSHAKE_TIMEOUT");
            break;
        default: break;
    }
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("Disconnected from WiFi access point Reason: ");
  ReasontoString(info.wifi_sta_disconnected.reason);
}

int str2mac(const char* mac, uint8_t* values){
    if( 6 == sscanf( mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&values[0], &values[1], &values[2],&values[3], &values[4], &values[5] ) ){
        return 1;
    }else{
        return 0;
    }
}
bool connectToWifi(){
    Serial.println(F("-------------------------------------"));
    WiFi.mode(WIFI_STA);   //ESP32 connects to an access point
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setAutoReconnect(true);
    delay(10);
    WiFi.disconnect();
    //WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    if(strlen(printerConfig.BSSID) == 0){
        WiFi.begin(globalVariables.SSID, globalVariables.APPW);
        wifimode = 1;
    }
    else{
        // Reference https://stackoverflow.com/questions/70415075/esp32-select-one-from-multiple-wifi-ap-with-same-name-ssid

        //Function to convert String MAC address to 6 byte array
        if(str2mac(printerConfig.BSSID,bssid)){
            WiFi.begin(globalVariables.SSID, globalVariables.APPW); //, 0, bssid);
            wifimode = 0;
        }
        else{
            if (printerConfig.debuging || printerConfig.debugingchange){
                Serial.print(F("Parsing MAC Address Failed, reverting to "));
                Serial.println(globalVariables.SSID);
            } 
            WiFi.begin(globalVariables.SSID, globalVariables.APPW);
            wifimode = 1;
        }
    }
    
    wl_status_t status = WiFi.status();
    if(connectionAttempts > 10){
        //WiFi.begin(globalVariables.SSID, globalVariables.APPW);
        WiFi.disconnect();
        WiFi.begin(globalVariables.SSID, globalVariables.APPW); //, 0, bssid);
        connectionAttempts = 1;
    }
    Serial.print(F("Connecting to WIFI.. Status check #"));
    if (connectionAttempts < 10){Serial.print(" ");}
    Serial.print(connectionAttempts);
    Serial.print(F(" / 10      SSID: '"));
    Serial.print(globalVariables.SSID);
    if(strlen(printerConfig.BSSID) > 0){
        Serial.print(F("'   BSSID: '"));
        Serial.print(printerConfig.BSSID);
    }
    Serial.print(F("'     PWD: '"));
    Serial.print(globalVariables.APPW);
    Serial.print(F("'    Wifi Status: "));
    Serial.print(wl_status_to_string(status));
    Serial.println();

    while (status != WL_CONNECTED) {
        if(status != WiFi.status()){
            status = WiFi.status();
            switch (status)
            {
                case WL_CONNECTED:
                    Serial.print(F("Wifi Status: "));
                    Serial.println(wl_status_to_string(status));
                    break;
                case WL_IDLE_STATUS:
                    Serial.print(F("Wifi Status: "));
                    Serial.println(wl_status_to_string(status));
                case WL_CONNECT_FAILED:
                    Serial.print(F("Wifi Status: "));
                    Serial.println(wl_status_to_string(status));
                    break;
                case WL_NO_SSID_AVAIL:
                    Serial.print(F("Wifi Status: "));
                    Serial.println(wl_status_to_string(status));
                    Serial.println(F("Bad WiFi credentials"));
                    return false;
                case WL_DISCONNECTED:
                    Serial.print(F("Wifi Status: "));
                    Serial.println(wl_status_to_string(status));
                    Serial.println(F("Disconnected. (Check low RSSI)"));
                    return false;
                default:
                    Serial.print(F("Uncaught Status - Wifi Status: "));
                    Serial.println(wl_status_to_string(status));
                    break;
            }
        }
        delay(2000); // Giving time to connect
        Serial.print(F("."));
    }
    connectionAttempts++;

    #ifdef ARDUINO_ARCH_ESP32
        WiFi.setTxPower(WIFI_POWER_19_5dBm); // https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/blob/main/README.md
    #endif
    
    #ifdef ESP32
        WiFi.setTxPower(WIFI_POWER_19_5dBm); // https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/blob/main/README.md
    #endif
    Serial.print(F("IP_ADDRESS:"));    // !!! Line required in this format for WifiSetup.html page to show IP Address correct.
    Serial.print(WiFi.localIP());      // !!! Line required in this format for WifiSetup.html page to show IP Address correct.
    Serial.println(F("\n         "));  // !!! Line required in this format for WifiSetup.html page to show IP Address correct.

    Serial.print(F("Connected To Wifi Access Point:  "));
    Serial.println(globalVariables.SSID);
    Serial.print(F("Specific BSSID:                  "));  //MAC address of connected AP
    Serial.println(WiFi.BSSIDstr());
    Serial.print(F("RSSI (Signal Strength):          "));
    Serial.println(WiFi.RSSI());
    Serial.print(F("BLLED Controller IP Address:     "));
    Serial.println(WiFi.localIP());
    Serial.println();
    Serial.print(F("Use web browser to access 'http://"));  // Instruction for user to go to Config page
    Serial.print(WiFi.localIP());                           // Instruction for user to go to Config page
    Serial.println(F("/' to view the setup page"));         // Instruction for user to go to Config page
    Serial.println();
    return true;
};

void scanNetwork()
{
    //Reference / Credit - https://www.esp32.com/viewtopic.php?t=18979#p70299
    int bestRSSI = -200;
    String bestBSSID = "";

    Serial.println(F("Wifi network scan start"));
    int n = WiFi.scanNetworks();
    Serial.println(F("Network scan complete"));

    if (n == 0) {
        Serial.println(F("No wifi networks found :("));
    } else {
        Serial.print(n); // Count
        Serial.println(F(" Wifi networks found"));
        Serial.println(F("-------------------------------------------------------------------------"));
        for (int i = 0; i < n; ++i) {                // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(F(": "));
            Serial.print(WiFi.SSID(i));
            Serial.print(F(" ("));
            Serial.print(WiFi.RSSI(i));
            Serial.print(F("dBm, "));
            if (WiFi.RSSI(i) < -50) Serial.print(F(" ")); //Spacing... because I just can't look at the results when misaligned.
            Serial.print(constrain(2 * (WiFi.RSSI(i) + 100), 0, 100));
            Serial.print(F("%)      "));
            Serial.print(F("BSSID: "));
            Serial.print(WiFi.BSSIDstr(i));
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " open" : " encrypted");

            if(WiFi.SSID(i) == globalVariables.SSID && WiFi.RSSI(i) > bestRSSI)
            {
                bestBSSID = WiFi.BSSIDstr(i);
                bestRSSI = WiFi.RSSI(i); 
            }
            delay(10);
        }
        Serial.println();
        if(printerConfig.BSSID == bestBSSID.c_str()){
                Serial.print(F("BSSID already set to: "));
                Serial.print(printerConfig.BSSID);
        }
        else{
            if(strlen(printerConfig.BSSID) == 0 || printerConfig.rescanWiFiNetwork){
                strcpy(printerConfig.BSSID,bestBSSID.c_str());
                Serial.print(F("Saving strongest AP for: "));
                Serial.println(globalVariables.SSID);
                Serial.print(F("Strongest BSSID (MAC Address): "));
                Serial.print(printerConfig.BSSID);
            }
            else{
                Serial.print(F("Ignoring strongest AP for: "));
                Serial.println(globalVariables.SSID);
                Serial.print(F("Using alrady saved BSSID (MAC Address): "));
                Serial.print(printerConfig.BSSID);
            }
            Serial.print(F("     RSSI (Signal Strength): "));
            Serial.print(bestRSSI);
            Serial.println(F("dBm"));
        }
    }
    Serial.println();
}


#endif
