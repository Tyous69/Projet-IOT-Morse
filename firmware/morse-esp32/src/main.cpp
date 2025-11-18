#include <WiFi.h>
#include <PubSubClient.h>

// ==================== CONFIGURATION ====================
const char* ssid = "TON_WIFI_SSID";
const char* password = "TON_WIFI_PASSWORD";

// MQTT Broker
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 8000;

// ID de l'appareil - À CHANGER SI BESOIN
const char* deviceId = "ESP32_MORSE_001";

// Broches du joystick
#define JOYSTICK_X 34    // Analog pin pour X
#define JOYSTICK_Y 35    // Analog pin pour Y
#define JOYSTICK_BUTTON 32  // Digital pin pour le bouton

// Seuils pour la détection des directions
#define THRESHOLD_LOW 1000
#define THRESHOLD_HIGH 3000
#define DEBOUNCE_DELAY 200  // Délai anti-rebond en ms

// ==================== VARIABLES GLOBALES ====================
WiFiClient espClient;
PubSubClient client(espClient);

String currentMorse = "";  // Stocke le code morse en cours
unsigned long lastJoystickAction = 0;
bool lastButtonState = HIGH;

// ==================== FONCTIONS WiFi ====================
void setupWiFi() {
  Serial.print("Connexion à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.println("Adresse IP: ");
  Serial.println(WiFi.localIP());
}

// ==================== FONCTIONS MQTT ====================
void callback(char* topic, byte* payload, unsigned int length) {
  // Convertir le payload en String
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Traiter les commandes du site web
  if (String(topic) == String("morse/") + deviceId + "/from_web") {
    if (message == "DOT") {
      addToMorse(".");
      playDot();
    } else if (message == "DASH") {
      addToMorse("-");
      playDash();
    } else if (message == "SPACE") {
      addToMorse(" ");
    } else if (message.startsWith("TRANSLATION:")) {
      String translation = message.substring(12);
      Serial.println("Traduction reçue: " + translation);
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    
    // ID client unique
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connecté!");
      
      // S'abonner aux topics
      String subscribeTopic = String("morse/") + deviceId + "/from_web";
      client.subscribe(subscribeTopic.c_str());
      
      // Publier le statut "prêt"
      String statusTopic = String("morse/") + deviceId + "/status";
      client.publish(statusTopic.c_str(), "DEVICE_READY");
      
    } else {
      Serial.print("échec, rc=");
      Serial.print(client.state());
      Serial.println(" nouvel essai dans 5s");
      delay(5000);
    }
  }
}

// ==================== FONCTIONS MORSE ====================
void addToMorse(String symbol) {
  currentMorse += symbol;
  Serial.println("Code Morse: " + currentMorse);
  
  // Envoyer le code morse actuel au site web
  String morseTopic = String("morse/") + deviceId + "/from_device";
  client.publish(morseTopic.c_str(), ("MORSE:" + currentMorse).c_str());
}

void playDot() {
  Serial.println("BIP: Point (•)");
  // Ici tu peux ajouter un buzzer ou une LED
  // tone(BUZZER_PIN, 1000, 100); // Bip court
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}

void playDash() {
  Serial.println("BIP: Tiret (-)");
  // tone(BUZZER_PIN, 1000, 300); // Bip long
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
  digitalWrite(LED_BUILTIN, LOW);
}

void validateCharacter() {
  if (currentMorse.length() > 0) {
    Serial.println("Validation du caractère: " + currentMorse);
    
    // Ici tu peux ajouter la logique de traduction morse→texte
    // Pour l'instant on envoie juste le code morse
    
    String translationTopic = String("morse/") + deviceId + "/from_device";
    client.publish(translationTopic.c_str(), ("TRANSLATION:" + currentMorse).c_str());
    
    currentMorse = "";  // Reset pour le prochain caractère
  }
}

void clearMorse() {
  currentMorse = "";
  Serial.println("Code Morse effacé");
}

// ==================== FONCTIONS JOYSTICK ====================
void handleJoystick() {
  // Lire les valeurs analogiques
  int xValue = analogRead(JOYSTICK_X);
  int yValue = analogRead(JOYSTICK_Y);
  int buttonState = digitalRead(JOYSTICK_BUTTON);

  // Anti-rebond
  if (millis() - lastJoystickAction < DEBOUNCE_DELAY) {
    return;
  }

  // Détection des directions
  if (xValue < THRESHOLD_LOW) {
    // Joystick à GAUCHE - POINT
    Serial.println("Joystick: GAUCHE → Point (•)");
    addToMorse(".");
    playDot();
    lastJoystickAction = millis();
    
  } else if (xValue > THRESHOLD_HIGH) {
    // Joystick à DROITE - TIRET
    Serial.println("Joystick: DROITE → Tiret (-)");
    addToMorse("-");
    playDash();
    lastJoystickAction = millis();
    
  } else if (yValue > THRESHOLD_HIGH) {
    // Joystick en BAS - ESPACE
    Serial.println("Joystick: BAS → Espace");
    addToMorse(" ");
    lastJoystickAction = millis();
    
  } else if (yValue < THRESHOLD_LOW) {
    // Joystick en HAUT - VALIDER
    Serial.println("Joystick: HAUT → Validation");
    validateCharacter();
    lastJoystickAction = millis();
  }

  // Détection du bouton (appuyer)
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Bouton appuyé → Effacer");
    clearMorse();
    delay(DEBOUNCE_DELAY);  // Anti-rebond supplémentaire
  }
  
  lastButtonState = buttonState;
}

// ==================== SETUP & LOOP ====================
void setup() {
  Serial.begin(115200);
  
  // Configuration des broches
  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_Y, INPUT);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Connexions
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  Serial.println("Setup terminé - En attente de connexion MQTT...");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  // Gérer le joystick
  handleJoystick();
  
  delay(50);  // Petit délai pour stabilité
}