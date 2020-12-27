//
// ESP8266 WiFi
//
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>

ESP8266WiFiMulti wifi;

const char* wifi_ssid = "fake_ssid";
const char* wifi_pass = "fake_pass";

void initWiFi() {
    WiFi.begin(wifi_ssid, wifi_pass);
    while (WiFi.status() != WL_CONNECTED) {
        // Wait for WiFi connection
        delay(500);
        Serial.println("Waiting to connect...");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }
}

//
//  ESP8266 Webserver
//
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

void rootHandler() {
    StaticJsonDocument<200> doc;
    String output;
    String door;
    String light;

    if ( daytime() ) {
        light = "day";
    } else {
        light = "night";
    }

    if ( doorOpen() ) {
        door = "open";
    } else {
        door = "closed";
    }

    doc["light"] = light;
    doc["door"] = door;
    serializeJson(doc, output);

    server.send(200, "application/json", output);
}

void networkHandler() {
    StaticJsonDocument<200> doc;
    String output;
    String ip;
    String mac;

    ip = WiFi.localIP().toString();
    mac = WiFi.BSSIDstr();

    if (WiFi.status() == WL_CONNECTED) {
        doc["status"] = "connected";
        doc["ssid"] = wifi_ssid;
        doc["address"] = ip;
        doc["mac"] = mac;
    } else {
        // How can we handle a request if we don't have WiFi???
        doc["status"] = "disconnected";
    }
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

int doorState = 0;

void doorCloseHandler() {
    StaticJsonDocument<200> doc;
    String output;
    String door;

    triggerDoorClose();

    if ( doorOpen() ) {
        door = "open";
    } else {
        door = "closed";
    }

    doc["door"] = door;
    doc["state"] = doorState;
    serializeJson(doc, output);

    server.send(200, "application/json", output);
}

const int lightSensorPin = A0;
int lightValue = 0, lastLightValue = 0;
int averageLightValue = 0;

void lightHandler() {
    StaticJsonDocument<200> doc;
    String output;
    String light;

    if ( daytime() ) {
        light = "day";
    } else {
        light = "night";
    }

    doc["light"] = light;
    doc["state"] = lightValue;
    serializeJson(doc, output);

    server.send(200, "application/json", output);
}

void initHttpServer() {
    server.on("/", HTTP_GET, rootHandler);
    server.on("/light", HTTP_GET, lightHandler);
    server.on("/door/close", HTTP_POST, doorCloseHandler);
    server.on("/network", HTTP_GET, networkHandler);
    server.begin();
}

/**********************************************************************
* Source: http://www.bajdi.com/continuous-rotation-servos-and-arduino/
* Counter clock-wise rotation is achieved using values from 45-89.
* Clock-wise rotation is achieved using values from 91-135.
**********************************************************************/

#include <Servo.h>
Servo servo;

const int servoPin = 0;
const int servoSpeedLow = 15;
const int speedMedium = 30;
const int speedHigh = 45;

void clockwiseRotation(int speed, int millis){
  int rotationSpeed;
  if ( speed > 45 ) {
    speed = 45;
  }

  rotationSpeed = 90 + speed;

  servo.attach(servoPin);
  servo.write(rotationSpeed);
  delay(millis);
  servo.write(90);
  servo.detach();
}

void counterclockwiseRotation(int speed, int millis){
  int rotationSpeed;
  if ( speed > 45 ) {
    speed = 45;
  }

  rotationSpeed = 90 - speed;

  servo.attach(servoPin);
  servo.write(rotationSpeed);
  delay(millis);
  servo.write(90);
  servo.detach();
}

//
//  Light Sensor
//

void initLightSensor() {
  readLightSensor();
}


void readLightSensor() {  
  int average;
  int current;
  current = analogRead(lightSensorPin);

  average = ((current + lightValue + lastLightValue) / 3);

  Serial.print("light average: ");
  Serial.println(average);
  
  lastLightValue = lightValue;
  lightValue = current;  
}

//const int breakBeamPin = 4;
//int beamValue = 0, lastBeamValue = 0;

//void readBreakBeam() {
//  lastBeamValue = beamValue;
//  beamValue = digitalRead(breakBeamPin);
//}

//void initBreakBeam() {
//  pinMode(breakBeamPin, INPUT);
//  digitalWrite(breakBeamPin, HIGH);
//  readBreakBeam();
//}


// ========================================== //

void setDoorState(int state) {
    doorState = state;
}

bool doorClosed() {
  return doorState == 0;
}

bool doorOpen() {
  return doorState == 1;
}

void closeDoor() {
    counterclockwiseRotation(45, 5000);
    setDoorState(0);
}

void openDoor() {
    clockwiseRotation(45, 5000);
    setDoorState(1);
}

void triggerDoorClose() {
    if ( doorOpen() ) {
        closeDoor();
    }
}

bool daytime() {
  readLightSensor();
  return lightValue > 35;
}

void checkDoorStatus() {
    // If it is daytime, the door should be open
    if ( daytime() && doorClosed()) {
        openDoor();
    }
}

void setup() {
    Serial.begin(115200);
    initLightSensor();
//    initBreakBeam();
    initWiFi();
    initHttpServer();
}

void loop() {
    server.handleClient();
    MDNS.update();
}
