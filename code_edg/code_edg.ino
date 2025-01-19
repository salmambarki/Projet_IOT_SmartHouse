#include <WiFi.h>
#include <PicoMQTT.h>
#include <ESP32Servo.h>
#include <DHT.h>

// Wi-Fi credentials
const char* ssid = "Gateway_AP";
const char* password = "gateway12345";

// MQTT client setup
WiFiClient espClient;
PicoMQTT::Client mqttClient("192.168.4.1", 1883);

// DHT sensor configuration
#define DHTPIN 4  // GPIO where the DHT is connected
#define DHTTYPE DHT11 // Type of sensor (DHT11 or DHT22)
DHT dht(DHTPIN, DHTTYPE);

// Servo and LED configuration
#define LED_CHAUD 27
#define LED_FROID 26
#define LED_ETAT 25
#define LED_ROOM 33
const int SERVO_FENETRE_PIN = 18;
const int SERVO_PORTE_PIN = 19;

Servo servoFenetre;
Servo servoPorte;

void setup() {
  Serial.begin(115200);

  // Configure pins
  pinMode(LED_CHAUD, OUTPUT);
  pinMode(LED_FROID, OUTPUT);
  pinMode(LED_ETAT, OUTPUT);
  pinMode(LED_ROOM, OUTPUT);

  digitalWrite(LED_CHAUD,LOW );
  digitalWrite(LED_FROID, LOW);
  digitalWrite(LED_ETAT, LOW);

  // Attach servos
  servoFenetre.attach(SERVO_FENETRE_PIN, 500, 2400);
  servoPorte.attach(SERVO_PORTE_PIN, 500, 2400);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize MQTT
  mqttClient.begin();

  // Subscribe to topics
  mqttClient.subscribe("gateway/door_status", [](const char* topic, const char* payload) {
    Serial.printf("Received message in topic %s : %s\n", topic, payload);
    bool porteStatus = atoi(payload);
    if (porteStatus) {
      servoPorte.write(90); // Open the door
      Serial.println("Door opened");
    } else {
      servoPorte.write(0); // Close the door
      Serial.println("Door closed");
    }
  });

  mqttClient.subscribe("gateway/led_room", [](const char* topic, const char* payload) {
    Serial.printf("Received message in topic %s : %s\n", topic, payload);
    bool ledStatus = atoi(payload);
    digitalWrite(LED_ROOM, ledStatus ? HIGH : LOW);
    Serial.println(ledStatus ? "LED ON" : "LED OFF");
  });
}

void loop() {
  mqttClient.loop();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Error reading sensor!");
  } else {
    Serial.println("Temperature: " + String(temperature) + " \u00B0C");
    Serial.println("Humidity: " + String(humidity) + " %");

    // Adjust LED and servo based on temperature
    if (temperature < 20) {
      digitalWrite(LED_CHAUD, HIGH);
      digitalWrite(LED_FROID, LOW);
      digitalWrite(LED_ETAT, LOW);
      servoFenetre.write(0); // Close the window
    } else if (temperature >= 20 && temperature < 30) {
      digitalWrite(LED_CHAUD, LOW);
      digitalWrite(LED_FROID, LOW);
      digitalWrite(LED_ETAT, HIGH);
      servoFenetre.write(90); // Half-open the window
    } else {
      digitalWrite(LED_CHAUD, LOW);
      digitalWrite(LED_FROID, HIGH);
      digitalWrite(LED_ETAT, LOW);
      servoFenetre.write(0); // Close the window
    }

    // Publish data to MQTT
    mqttClient.publish("edge/temperature", String(temperature));
    mqttClient.publish("edge/humidity", String(humidity));
    mqttClient.publish("edge/window_status", String(servoFenetre.read()));
    mqttClient.publish("edge/leds_chaud", String(digitalRead(LED_CHAUD)));
    mqttClient.publish("edge/leds_froid", String(digitalRead(LED_FROID)) );
    mqttClient.publish("edge/leds_etat", String(digitalRead(LED_ETAT)));
    Serial.println("Data published to gateway:");
    Serial.println("Temperature: " + String(temperature, 2) + " \u00B0C");
    Serial.println("Humidity: " + String(humidity, 2) + " %");
    Serial.println("Window Status: " + String(servoFenetre.read()));
    Serial.println("LEDs Status: " + String(digitalRead(LED_CHAUD)) + "," + String(digitalRead(LED_FROID)) + "," + String(digitalRead(LED_ETAT)));
  }

  delay(3000); // Pause for 5 seconds
}
