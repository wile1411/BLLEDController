#ifndef _MQTTMANAGER
#define _MQTTMANAGER

#include <Arduino.h>
#include <WiFi.h>
static int mqttbuffer = 32768;
static int mqttdocument = 32768; //16384

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 

#include "mqttparsingutility.h"
#include "AutoGrowBufferStream.h"
#include "types.h"
#include "leds.h"

WiFiClientSecure wifiSecureClient;
PubSubClient mqttClient(wifiSecureClient);

String device_topic;
String report_topic;
String clientId = "BLLED-";

AutoGrowBufferStream stream;

unsigned long mqttattempt = (millis()-3000);

void connectMqtt(){
    if(WiFi.status() != WL_CONNECTED){
        //Abort MQTT connection attempt when no Wifi
        return;
    }
    if (!mqttClient.connected() && (millis() - mqttattempt) >= 3000){   
        Serial.println(F("Connecting to mqtt"));
        if (mqttClient.connect(clientId.c_str(),"bblp",printerConfig.accessCode)){
            Serial.println(F("MQTT connected, subscribing to topic:"));
            Serial.println(report_topic);
            mqttClient.subscribe(report_topic.c_str());
            printerVariables.online = true;
            printerVariables.disconnectMQTTms = 0;
            Serial.println(F("updating from MQTT connect"));
            updateleds();
        }else{
            Serial.println(F("Failed to connect with error code: "));
            Serial.println(mqttClient.state());
            ParseMQTTState(mqttClient.state());
            if(mqttClient.state() == 5){
                Serial.println(F("Restarting Device"));
                ESP.restart();                
            }
        }
    }
}

void ParseCallback(char *topic, byte *payload, unsigned int length){
    DynamicJsonDocument messageobject(mqttdocument);

    DynamicJsonDocument filter(128);
    filter["print"]["*"] =  true;
    filter["camera"]["*"] =  true;
    
    auto deserializeError = deserializeJson(messageobject, payload, length, DeserializationOption::Filter(filter));
    if (!deserializeError){

        if (printerConfig.debuging){
            Serial.println(F("Mqtt message received,"));
            Serial.println(F("FreeHeap: "));
            Serial.print(ESP.getFreeHeap());
            Serial.println();
        }

        if (printerConfig.mqttdebug){
            Serial.println(F("Mqtt payload:"));
            Serial.println();
            serializeJson(messageobject, Serial);
            Serial.println();
        }

        bool Changed = false;

        //Ideally, we should only monitor MQTT meessages with command = "push_status"
        //stg_cur & lights_report will be missing from other command types
        if (messageobject["print"].containsKey("command")){
            //gcode_line used a lot during print initialisations
            if (messageobject["print"]["command"].as<String>() != "push_status"){
                if (printerConfig.debuging) Serial.println(F("unknown MQTT message - Ignored"));
                return;
            }
        }
        else{
            if (printerConfig.debuging){
                Serial.println(F("Missing command Key - Ignored"));
            } 
            return;
        }

        //Check BBLP Stage
        if (messageobject["print"].containsKey("stg_cur")){
            if (printerVariables.stage != messageobject["print"]["stg_cur"].as<int>() ){
                printerVariables.stage = messageobject["print"]["stg_cur"];
                if (printerConfig.debugingchange || printerConfig.debuging){
                    Serial.print(F("MQTT update - stg_cur now: "));
                    Serial.println(printerVariables.stage);
                }
                Changed = true;
            }
        }else{
            if (printerConfig.debuging){
                Serial.println(F("MQTT stg_cur not in message"));
            }
        }

        //Check BBLP GCode State
        if (messageobject["print"].containsKey("gcode_state")){
            if(printerVariables.gcodeState != messageobject["print"]["gcode_state"].as<String>()){
                printerVariables.gcodeState = messageobject["print"]["gcode_state"].as<String>();
                if (messageobject["print"]["gcode_state"].as<String>() == "FINISH"){
                    printerVariables.finished = true;
                    printerVariables.finishstartms = millis();
                }
                
                if (printerConfig.debugingchange || printerConfig.debuging){
                    Serial.print(F("MQTT update - gcode_state now: "));
                    Serial.println(printerVariables.gcodeState);
                }
                Changed = true;
            }
        }


        if (messageobject["print"].containsKey("lights_report")) {
            JsonArray lightsReport = messageobject["print"]["lights_report"];

            for (JsonObject light : lightsReport) {
                if (light["node"] == "chamber_light") {
                    if(printerVariables.ledstate != (light["mode"] == "on")){
                        printerVariables.ledstate = light["mode"] == "on";
                        if (printerConfig.debugingchange || printerConfig.debuging){
                            Serial.print(F("MQTT chamber_light now: "));
                            Serial.println(printerVariables.ledstate);
                        }
                        Changed = true;
                    }
                }
            }
        }else{
            if (printerConfig.debuging){
                Serial.println(F("MQTT lights_report not in message"));
            }
        }

        if (messageobject["print"].containsKey("hms")){
            String oldHMS = "";
            oldHMS = printerVariables.parsedHMS;

            printerVariables.hmsstate = false;
            printerVariables.parsedHMS = "";
            for (const auto& hms : messageobject["print"]["hms"].as<JsonArray>()) {
                if (ParseHMSSeverity(hms["code"]) != ""){
                    printerVariables.hmsstate = true;
                    printerVariables.parsedHMS = ParseHMSSeverity(hms["code"]);
                }
            }
            if(oldHMS != printerVariables.parsedHMS){
                if (printerConfig.debuging  || printerConfig.debugingchange){
                    Serial.print(F("MQTT update - parsedHMS now: "));
                    Serial.println(printerVariables.parsedHMS);
                }
                Changed = true;
            }
        }

        if (messageobject["print"].containsKey("home_flag")){
            //https://github.com/greghesp/ha-bambulab/blob/main/custom_components/bambu_lab/pybambu/const.py#L324

            bool doorState = false;
            long homeFlag = 0;
            homeFlag = messageobject["print"]["home_flag"];
            doorState = homeFlag >> 23; //shift left 23 to the Door bit
            doorState = doorState & 1;  // remove any bits above Door bit

            if (printerVariables.doorOpen != doorState){
                printerVariables.doorOpen = doorState;
                printerVariables.idleLightsOff = false;
                printerVariables.idleStartms = millis();

                if (printerConfig.debugingchange)Serial.print(F("MQTT Door "));
                if (printerVariables.doorOpen){
                   printerVariables.lastdoorOpenms  = millis();
                   if (printerConfig.debugingchange) Serial.println(F("Opened"));
                }
                else{
                    if ((millis() - printerVariables.lastdoorClosems) < 6000){
                        printerVariables.doorSwitchenabled = true;
                    }
                    printerVariables.lastdoorClosems = millis();
                    if (printerConfig.debugingchange) Serial.println(F("Closed"));
                }
                Changed = true;
            }
        }

        if (Changed == true){
            if (printerConfig.debuging){
                Serial.println(F("Change from mqtt"));
            }
            updateleds();
        }
    }else{
        Serial.println(F("Deserialize error while parsing mqtt"));
        return;
    }
}


void mqttCallback(char *topic, byte *payload, unsigned int length){
    ParseCallback(topic, (byte *)stream.get_buffer(), stream.current_length());
    stream.flush();
}

void setupMqtt(){
    clientId += String(random(0xffff), HEX);
    Serial.println(F("Setting up MQTT with ip: "));
    Serial.println(printerConfig.printerIP);

    device_topic = String("device/") + printerConfig.serialNumber;
    report_topic = device_topic + String("/report");

    wifiSecureClient.setInsecure();
    mqttClient.setBufferSize(1024); //1024
    mqttClient.setServer(printerConfig.printerIP, 8883);
    mqttClient.setStream(stream);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setSocketTimeout(20);
    Serial.println(F("Finished setting up MQTT"));
    connectMqtt();
}

void mqttloop(){
    if(WiFi.status() != WL_CONNECTED){
        //Abort MQTT connection attempt when no Wifi
        return;
    }
    if (!mqttClient.connected()){
        printerVariables.online = false;
        //Only sent the timer from the first instance of a MQTT disconnect
        if(printerVariables.disconnectMQTTms == 0) {
            printerVariables.disconnectMQTTms = millis();
            //Record last time MQTT dropped connection
            Serial.println(F("MQTT dropped during mqttloop"));
            ParseMQTTState(mqttClient.state());
        }
        delay(500);
        connectMqtt();
        delay(32);
        return;
    }
    else{
        printerVariables.disconnectMQTTms = 0;
    }
    mqttClient.loop();
    delay(10);
}

#endif
