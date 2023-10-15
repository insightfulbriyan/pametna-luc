#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

// Replace the next variables with your SSID/Password combination
char *ssid = "REPLACE_WITH_YOUR_SSID";
const char *password = "REPLACE_WITH_YOUR_PASSWORD";

// Add your MQTT Broker IP address, example:
// const char* mqtt_server = "192.168.1.144";
const char *mqtt_server = "YOUR_MQTT_BROKER_IP_ADDRESS";
const char *mqtt_usernme = "YOUR_MQTT_BROKER_USERNAME";
const char *mqtt_password = "YOUR_MQTT_BROKER_PASSWORD";

WiFiClient espClient;
PubSubClient client(espClient);

long lastReconnectAttempt = 0;

int lucPin = 22;
bool lucState = LOW;

void setLuc(bool state) {
    digitalWrite(lucPin, state);
	Serial.println(state ? "on" : "off");
}

void toggleLuc() {
	lucState = !lucState;
    digitalWrite(lucPin, lucState);
	Serial.println(lucState ? "on" : "off");
}
void callback(char *topic, byte *message, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    // Feel free to add more if statements to control more GPIOs with MQTT

    // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
    // Changes the output state according to the message
    if (String(topic) == "esp32/input") {
        Serial.print("Changing output to ");
        if (messageTemp == "on") {
            setLuc(HIGH);
        } else if (messageTemp == "off") {
            setLuc(LOW);
        } else if (messageTemp == "toggle") {
            toggleLuc();
        }
    }
}

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

boolean reconnect() {
    Serial.println("Reconnection attemp");
    if (client.connect("ESP32", mqtt_usernme, mqtt_password)) {
        Serial.println("connected");
        client.subscribe("esp32/input");
        client.publish("esp32/output", "hello world");
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
    }

    return client.connected();
}



void setup() {
    Serial.begin(115200);
    client.setServer(mqtt_server, 8883);
    client.setCallback(callback);

    WiFi.begin(ssid, password);

    delay(1500);
    lastReconnectAttempt = 0;
}

void loop() {
    if (!client.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        // Client connected

        client.loop();
    }
}