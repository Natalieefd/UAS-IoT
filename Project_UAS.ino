#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DHTPIN D1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WiFi
const char *ssid = "o.o"; // Enter your WiFi name
const char *password = "mauapakamu";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic_suhu = "kodular/UasIoT/suhu";
const char *topic_data = "kodular/UasIoT/data";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

bool ledState = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    // Set software serial baud to 115200
    Serial.begin(115200);
    delay(1000); // Delay for stability

    // Connecting to a WiFi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to the WiFi network");

    // Setting LED pin as output
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);  // Turn off the LED initially

    // Connecting to an MQTT broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);

    while (!client.connected()) {
        String client_id = "mqttx_c711dd82";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Public EMQX MQTT broker connected");
            client.subscribe(topic_suhu);
            client.subscribe(topic_data); // Subscribing to the data topic if necessary
            Serial.println("Subscribed to topics");
        } else {
            Serial.print("Failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }

    dht.begin();
}

void loop() {
    client.loop();
    client.setCallback(callback);

    float t = dht.readTemperature();

    if (isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));

    } else {
        client.publish(topic_data, String(t).c_str());
    }

}

void callback(char *receivedTopic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(receivedTopic);

    Serial.print("Message: ");
    String message;
    float t = dht.readTemperature();
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];  // Convert *byte to string
    }
    Serial.println(message);
    
    if (strcmp(receivedTopic, topic_suhu) == 0) {
        if (message == "off" && ledState) {
          digitalWrite(LED_BUILTIN, HIGH);  // Turn off the LED
          ledState = false;

        } else if (message == "on" && !ledState) {
          digitalWrite(LED_BUILTIN, LOW);  // Turn on the LED
          ledState = true;

        }
    } else if (strcmp(receivedTopic, topic_data) == 0) {
        if (message == "suhu" && !ledState) {
          client.publish(topic_data, String(t).c_str());
        }
    }
    Serial.println("-----------------------");
}