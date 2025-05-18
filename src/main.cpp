#include <Arduino.h>
#include "web-server.h"
#include "mqttmanager.h"
#include "filesystem.h"
#include "types.h"
#include "leds.h"
#include "serialmanager.h"
#include "wifi-manager.h"

int wifi_reconnect_count = 0;
void defaultcolors(){
    Serial.println(F("Setting default customisable colors"));
    printerConfig.runningColor = hex2rgb("#000000",255,255);//WHITE Running
    printerConfig.testColor = hex2rgb("#3F3CFB");           //Violet Test
    printerConfig.finishColor = hex2rgb("#00FF00");         //Green Finish

    printerConfig.stage14Color = hex2rgb("#000000");        //OFF Cleaning Nozzle
    printerConfig.stage1Color = hex2rgb("#000055");         //OFF Bed Leveling
    printerConfig.stage8Color = hex2rgb("#000000");         //OFF Calibrating Extrusion
    printerConfig.stage9Color = hex2rgb("#000000");         //OFF Scanning Bed Surface
    printerConfig.stage10Color = hex2rgb("#000000");        //OFF First Layer Inspection

    printerConfig.wifiRGB = hex2rgb("#FFA500");             //Orange Wifi Scan
    
    printerConfig.pauseRGB = hex2rgb("#0000FF");            //Blue Pause
    printerConfig.firstlayerRGB = hex2rgb("#0000FF");       //Blue
    printerConfig.nozzleclogRGB = hex2rgb("#0000FF");       //Blue
    printerConfig.hmsSeriousRGB = hex2rgb("#FF0000");       //Red
    printerConfig.hmsFatalRGB = hex2rgb("#FF0000");         //Red
    printerConfig.filamentRunoutRGB = hex2rgb("#FF0000");   //Red
    printerConfig.frontCoverRGB = hex2rgb("#FF0000");       //Red
    printerConfig.nozzleTempRGB = hex2rgb("#FF0000");       //Red
    printerConfig.bedTempRGB = hex2rgb("#FF0000");          //Red


}
void WiFiEvent(WiFiEvent_t event){
    Serial.printf("[WiFi-event] event: %d - ", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            Serial.println("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            Serial.println("STA IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP6:
            Serial.println("Ethernet IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
        default: break;
    }
}
void setup(){
    Serial.begin(115200);
    delay(100);
    Serial.println(F("Initializing"));
    Serial.println(ESP.getFreeHeap());
    Serial.println("");
    Serial.print(F("** Using firmware version: "));
    Serial.print(globalVariables.FWVersion);
    Serial.println(F(" **"));
    Serial.println("");
    defaultcolors();
    setupLeds();
    tweenToColor(100,100,100,100,100); //ALL LEDS ON
    Serial.println(F(""));
    delay(1000);

    tweenToColor(255,0,0,0,0); //RED
    setupFileSystem();
    loadFileSystem();
    Serial.println(F(""));
    delay(500);

    tweenToColor(printerConfig.wifiRGB); //Customisable - Default is ORANGE
    setupSerial();

    if (strlen(globalVariables.SSID) == 0 || strlen(globalVariables.APPW) == 0) {
        Serial.println(F("SSID or password is missing. Please configure both by going to: https://dutchdevelop.com/blled-configuration-setup/"));
        tweenToColor(100,0,100,0,0); //PINK
        return;
    }
   
    scanNetwork(); //Sets the MAC address for following connection attempt
    if(!connectToWifi()){
        return;
    }

    tweenToColor(0,0,255,0,0); //BLUE
    setupWebserver();
    delay(500);

    
    tweenToColor(34,224,238,0,0); //CYAN
    setupMqtt();

    Serial.println();
    Serial.print(F("** BLLED Controller started "));
    Serial.print(F("using firmware version: "));
    Serial.print(globalVariables.FWVersion);
    Serial.println(F(" **"));
    Serial.println();
    globalVariables.started = true;
    Serial.println(F("Updating LEDs from Setup"));
    updateleds();
}

void loop(){
    serialLoop();
    if (globalVariables.started){
        mqttloop();
        webserverloop();
        ledsloop();
        
        if (WiFi.status() != WL_CONNECTED){
            Serial.print(F("Wifi connection dropped.  "));
            Serial.print(F("Wifi Status: ")); 
            Serial.println(wl_status_to_string(WiFi.status()));
            Serial.println(F("Attempting to reconnect to WiFi..."));
            wifi_reconnect_count += 1;
            if(wifi_reconnect_count <= 2){
                WiFi.disconnect();
                delay(100);
                WiFi.reconnect();
            } else {
                //Not connecting after 10 simple disconnect / reconnects
                //Do something more drastic in case needing to switch to new AP
                scanNetwork();
                connectToWifi();
                wifi_reconnect_count = 0;
            }
        }
    }
    if(printerConfig.rescanWiFiNetwork)
    {
        Serial.println(F("Web submitted refresh of Wifi Scan (assigning Strongest AP)"));
        tweenToColor(printerConfig.wifiRGB); //Customisable - Default is ORANGE
        scanNetwork(); //Sets the MAC address for following connection attempt
        printerConfig.rescanWiFiNetwork = false;
        updateleds();
    }
}