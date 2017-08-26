/*
 * A simple IoT WiFi electrical outlet with MQTT control.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define LED_PIN 0
#define OUTPUT_PIN 5
#define POWER_ON 0
#define POWER_OFF 1

const char* ssid = "yourWIFInetwork";
const char* password = "yourWIFIpassword";
const char* mqtt_server = "yourMQTTserver";
int mqtt_port = 8883;
const char* mqtt_username = "yourMQTTuser";
const char* mqtt_password = "yourMQTTpassword";
const char* controlTopic = "coffee-maker";

WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

const char* ON_MESSAGE = "ON";
const char* OFF_MESSAGE = "OFF";
byte powerState = POWER_OFF;
long lastOnTime = 0;
long autoOff = 0;

void mqtt_connect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    String mqtt_clientId = "client-";
    mqtt_clientId += String(random(0xffff), HEX);
    if (mqtt_client.connect(mqtt_clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");

      mqtt_client.publish(controlTopic, ((powerState == POWER_ON) ? ON_MESSAGE : OFF_MESSAGE), true);
      mqtt_client.subscribe(controlTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqtt_client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char buf[16];
  if (strcmp(topic, controlTopic)) {
    Serial.print("bad topic: ");
    Serial.println(topic);
    return;
  }
  memcpy(buf, (char *)payload, length);
  buf[length] = '\0';
  Serial.println(buf);

  int l = strlen(ON_MESSAGE);
  if ((length >= l) && (!strncmp(buf, ON_MESSAGE, l))) {
    powerState = POWER_ON;
    autoOff = 0;
    digitalWrite(OUTPUT_PIN, powerState);
    lastOnTime = millis();
    if ((length > l+1) && (buf[l] == ':')) {
      autoOff = strtol(buf+l+1, NULL, 10) * 1000;
      Serial.print("autoOff = ");
      Serial.println(autoOff);
    }
  }
  l = strlen(OFF_MESSAGE);
  if ((length >= l) && (!strncmp(buf, OFF_MESSAGE, l))) {
    autoOff = 0;
    powerState = POWER_OFF;
    digitalWrite(OUTPUT_PIN, powerState);
  }
}

void setup() {
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, POWER_OFF);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  delay(10);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    digitalWrite(LED_PIN, LOW);
    Serial.print(".");
    delay(20);
    digitalWrite(LED_PIN, HIGH);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  digitalWrite(LED_PIN, HIGH);

  // Print the IP address
  Serial.println(WiFi.localIP());

  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);

}

void loop(void){
  if (!mqtt_client.connected()) {
    mqtt_connect();
  }
  mqtt_client.loop();
  if ((autoOff > 0) && (powerState == POWER_ON) && ((millis() - lastOnTime) > autoOff)) {
    powerState = POWER_OFF;
    digitalWrite(OUTPUT_PIN, powerState);
    Serial.println("AUTO OFF");
    mqtt_client.publish(controlTopic, OFF_MESSAGE, true);
  }

}


