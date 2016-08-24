/*
 * Author: BertrandR
 */

#include <SPI.h>
#include "IthoCC1101.h"
#include "IthoPacket.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

IthoCC1101 rf;
IthoPacket packet;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Configuration
// WiFi
const char* ssid = "";
const char* password = "";

// MQTT
const char* mqttServer = "";
const char* mqttTopic = "";
const char* mqttUsername = "";
const char* mqttPassword = "";

void setup() {
  Serial.begin(115200);
  delay(1500);

  setup_wifi();
  rf.init();
  Serial.println("RF setup");

  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(callback);

  reconnectMQTT();
}

void setup_wifi() {
  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // I've found that sometimes the unit gets stuck on connecting, mainly after flashing 
  // It will try for 30x then reboot
  int reset = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    reset++;
    if (reset > 30) ESP.restart();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    // Don't hammer it though
    delay(1000);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect using either username/password or without
    if (strlen(mqttUsername) > 0) {
      mqttClient.connect("ESP8266Client",mqttUsername,mqttPassword);
    }
    else {
      mqttClient.connect("ESP8266Client");
    }
    if (mqttClient.connected()) {
      Serial.println("connected");

      // Say hello to the channel
      String helloMessage = "ESP8226 ";
      helloMessage += WiFi.macAddress();
      helloMessage += " connected";
      char helloMessageChar[helloMessage.length()+1];
      helloMessage.toCharArray(helloMessageChar, helloMessage.length()+1);
      mqttClient.publish(mqttTopic,helloMessageChar);

      // Subscribe to the channel
      mqttClient.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}

// Send command to RF module, every command is send 3x to be sure it arrives (since we can't check if it has effect)
void sendCommand(IthoCommand command) {
  for( int i = 0; i < 3; i++ ) {
       rf.sendCommand(command);
       delay(1000);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String s = String((char*)payload);
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.print(s);
  Serial.println();

  // Let's not be picky
  s.toLowerCase();
  if (s == "high") {
    sendCommand(full);
    mqttClient.publish(mqttTopic,"High (full) speed command send RF");
  }
  else if (s == "medium") {
    sendCommand(medium);
    mqttClient.publish(mqttTopic,"Medium speed command send over RF");
  }
  else if (s == "low") {
    sendCommand(low);
    mqttClient.publish(mqttTopic,"Low speed command send over RF");
  }
  else if (s == "timer") {
    sendCommand(timer1);
    mqttClient.publish(mqttTopic,"Timer command send over RF");
  }
  else if (s == "join") {
    sendCommand(join);
    mqttClient.publish(mqttTopic,"Join command send over RF");
  }
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi failed, try reconnect in 5 seconds");
    delay(5000);
    setup_wifi();
  }
  // Check MQTT connection
  if (!mqttClient.connected()) {
    Serial.print("MQTT Connection failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try reconnect in 5 seconds");
    delay(5000);
    reconnectMQTT();
  }
  // Handle MQTT events
  mqttClient.loop();
  delay(100);
}
