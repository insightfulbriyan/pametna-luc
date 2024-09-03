#include <WiFi.h>
#include <PubSubClient.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <time.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

IPAddress local_ip;

// GPIO pin you want to control
const int pin = 5; // Example: GPIO5
const int buttonPin = 4; // Example: GPIO4 for the external button

// Topics
const char* set_topic = "esp32/set_pin";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
AsyncMqttClient mqttBroker;

Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

// Time variables
struct tm timeinfo;

// Function prototypes
void setup_wifi();
void connectToMqtt();
void onMqttMessage(char* topic, byte* payload, unsigned int length);
void checkButton();
void checkTimeForReset();
void resetESP32();

volatile bool buttonPressed = false;

void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);

  // Initialize GPIO
  pinMode(pin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Button with pull-up resistor

  attachInterrupt(buttonPin, handleButtonPress, FALLING);

  // Connect to WiFi
  setup_wifi();

  // Setup MQTT broker
  mqttBroker.onClientConnected([](void*, AsyncMqttClientClient*) {
    Serial.println("Client connected to MQTT broker");
  });
  mqttBroker.setServer(local_ip, 1883);
  mqttBroker.begin();

  // Setup MQTT client
  mqttClient.setServer(local_ip, 1883);
  mqttClient.setCallback(onMqttMessage);

  connectToMqtt();

  // Setup time
  configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // Set up NTP
}

void setup_wifi() {
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
  
  local_ip = WiFi.localIP();
  Serial.print("IP address: ");
  Serial.println(local_ip);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");

  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
      mqttClient.subscribe(set_topic, 0);
    } else {
      Serial.print("Failed to connect, retrying in 5 seconds - state: ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == set_topic) {
    if (message == "ON") {
      digitalWrite(pin, HIGH);
    } else if (message == "OFF") {
      digitalWrite(pin, LOW);
    }
  }
}

void checkButton() {
  if (buttonPressed) {
    buttonPressed = false;
    // Toggle the state of the pin
    digitalWrite(pin, !digitalRead(pin));

    // Optional: Publish the new state to the MQTT topic
    if (digitalRead(pin) == HIGH) {
      mqttClient.publish(set_topic, "ON");
    } else {
      mqttClient.publish(set_topic, "OFF");
    }
  }
}

void checkTimeForReset() {
  if (getLocalTime(&timeinfo)) {
    // Check if it's noon
    if (timeinfo.tm_hour == 12 && timeinfo.tm_min == 0) {
      resetESP32();
    }
  }
}

void resetESP32() {
  Serial.println("Resetting ESP32...");
  ESP.restart();
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMqtt();
  }
  mqttClient.loop();

  checkButton();
  checkTimeForReset();
}
