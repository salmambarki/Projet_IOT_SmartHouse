#include <WiFi.h>
#include <FirebaseESP8266.h>  // Use FirebaseESP32 if you are using an ESP32
#include <PicoMQTT.h>

// Wi-Fi Point d'accès
const char* ap_ssid = "Gateway_AP";
const char* ap_password = "gateway12345";

// Accès Internet
const char* internet_ssid = "RedmiNote11";
const char* internet_password = "papa12345";

// Firebase Configuration
#define FIREBASE_HOST "https://smarthouse-29faf-default-rtdb.firebaseio.com/"
#define API_KEY "AIzaSyC2s89Wq3qMVdfVWDSFOtpIHEBZ8AbzULA"


FirebaseData firebaseData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

// MQTT Configuration
PicoMQTT::Server mqttBroker;

WiFiClient espClient;

void setup() {
  Serial.begin(115200);

  // Configuration du Point d'accès
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("Point d'accès démarré !");
  Serial.print("Adresse IP AP : ");
  Serial.println(WiFi.softAPIP());

  // Connexion à Internet
  WiFi.begin(internet_ssid, internet_password);
  Serial.print("Connexion à Internet...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnecté à Internet !");
  Serial.print("Adresse IP Internet : ");
  Serial.println(WiFi.localIP());

  // Configuration Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.api_key = API_KEY;

  // Authentification anonyme
  if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")) {
    Serial.println("Authentification anonyme réussie !");
  } else {
    Serial.print("Erreur d'authentification : ");
    Serial.println(firebaseConfig.signer.signupError.message.c_str());
    return;
  }

  // Initialisation Firebase
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialisé !");


  // Initialisation du Broker MQTT
  mqttBroker.begin();
}

void loop() {
  mqttBroker.loop();

  // Read data from Firebase and publish to MQTT
  if (Firebase.getBool(firebaseData, "/chambres/chambre1/led_status")) {
    bool ledRoomStatus = firebaseData.boolData();
    Serial.println(ledRoomStatus ? "LED ON" : "LED OFF");
    mqttBroker.publish("gateway/led_room", ledRoomStatus ? "1" : "0");
  }

  if (Firebase.getBool(firebaseData, "/chambres/chambre1/porte_status")) {
    bool doorStatus = firebaseData.boolData();
    mqttBroker.publish("gateway/door_status", doorStatus ? "1" : "0");
  }

  // Subscribe to MQTT topics and update Firebase
  mqttBroker.subscribe("edge/temperature", [](const char* topic, const char* payload) {
    Serial.printf("Received temperature: %s\n", payload);
    Firebase.setFloat(firebaseData, "/environment/temperature", atof(payload));
  });

  mqttBroker.subscribe("edge/humidity", [](const char* topic, const char* payload) {
    Serial.printf("Received humidity: %s\n", payload);
    Firebase.setFloat(firebaseData, "/environment/humidity", atof(payload));
  });

  mqttBroker.subscribe("edge/leds_chaud", [](const char* topic, const char* payload) {
    Serial.printf("Received LEDs status (chaud): %s\n", payload);
    Firebase.setBool(firebaseData, "/chambres/chambre1/climatiseur/chaud", atoi(payload));
  });

  mqttBroker.subscribe("edge/leds_froid", [](const char* topic, const char* payload) {
    Serial.printf("Received LEDs status (froid): %s\n", payload);
    Firebase.setBool(firebaseData, "/chambres/chambre1/climatiseur/froid", atoi(payload));
  });

  mqttBroker.subscribe("edge/leds_etat", [](const char* topic, const char* payload) {
    Serial.printf("Received LEDs status (etat): %s\n", payload);
    Firebase.setBool(firebaseData, "/chambres/chambre1/climatiseur/etat", atoi(payload));
  });

  mqttBroker.subscribe("edge/window_status", [](const char* topic, const char* payload) {
    Serial.printf("Received window status: %s\n", payload);
    Firebase.setBool(firebaseData, "/chambres/chambre1/fenetre_status", atoi(payload));
  });

  Serial.println("Published data to Firebase.");
  delay(3000); // Pause for 5 seconds
}
