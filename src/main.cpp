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

class Luc {
private:
    int pin;

public:
    bool state;

    Luc(int output_pin, int state = LOW) {
        pin = output_pin;
        state = state;
        pinMode(output_pin, OUTPUT);
        digitalWrite(output_pin, state);
    }

    void setLuc(bool state) {
        digitalWrite(pin, state);
        Serial.println(state ? "on" : "off");
    }

    void toggleLuc() {
        state = !state;
        digitalWrite(pin, state);
        Serial.println(state ? "on" : "off");
    }
};

class Stikalo {
private:
    int pin;
public:
    int state;
    int lastRead;

    Stikalo(int input_pin){
        pin = input_pin;
        pinMode(input_pin, OUTPUT);
        state = digitalRead(input_pin);
        lastRead = millis();
    }

    bool read(){
        state = digitalRead(pin);
        lastRead = millis();
        return state;
    }
};

Luc jaka(22);
Stikalo btn(10);

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

    if (String(topic) == "esp32/input") {
        Serial.print("Changing output to ");
        if (messageTemp == "on") {
            jaka.setLuc(HIGH);
        } else if (messageTemp == "off") {
            jaka.setLuc(LOW);
        } else if (messageTemp == "toggle") {
            jaka.toggleLuc();
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
    long now = millis();
    if (!client.connected()) {
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        client.loop();
    }

    if(now - btn.lastRead > 500){
        bool old_state = btn.state;
        delay(100);

        if(old_state != btn.read() && btn.state == true){
            jaka.toggleLuc();
        }
    }
}