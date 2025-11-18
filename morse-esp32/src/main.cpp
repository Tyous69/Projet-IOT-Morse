#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ==================== CONFIGURATION ====================
const char* ssid = "AD";
const char* password = "adrien21";

// MQTT Broker
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// ID de l'appareil
const char* deviceId = "ESP32_MORSE_001";

// Broches du joystick HW-504
#define JOYSTICK_X 34    // VRx - Axe X
#define JOYSTICK_Y 35    // VRy - Axe Y
#define JOYSTICK_BUTTON 32  // SW - Bouton

// SEUILS SPÉCIFIQUES POUR HW-504
#define THRESHOLD_LOW 1000    // En dessous = GAUCHE/HAUT
#define THRESHOLD_HIGH 3000   // Au dessus = DROITE/BAS
#define CENTER_MIN 1500       // Zone centrale min
#define CENTER_MAX 2500       // Zone centrale max
#define DEBOUNCE_DELAY 300    // Délai anti-rebond

// ==================== VARIABLES GLOBALES ====================
WiFiClient espClient;
PubSubClient client(espClient);

String currentMorse = "";
unsigned long lastJoystickAction = 0;
bool lastButtonState = HIGH;

// ==================== DÉCLARATIONS DES FONCTIONS ====================
void addToMorse(String symbol);
void playDot();
void playDash();
void validateCharacter();
void clearMorse();
void handleJoystick();
void printJoystickDebug();  // Pour debug

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

// ==================== FONCTIONS MORSE ====================
void addToMorse(String symbol) {
  currentMorse += symbol;
  Serial.println("Code Morse: " + currentMorse);
  
  String morseTopic = String("morse/") + deviceId + "/from_device";
  client.publish(morseTopic.c_str(), ("MORSE:" + currentMorse).c_str());
}

void playDot() {
  Serial.println("BIP: Point (•)");
  delay(100);
}

void playDash() {
  Serial.println("BIP: Tiret (-)");
  delay(300);
}

void validateCharacter() {
  if (currentMorse.length() > 0) {
    Serial.println("Validation du caractère: " + currentMorse);
    
    String translationTopic = String("morse/") + deviceId + "/from_device";
    client.publish(translationTopic.c_str(), ("TRANSLATION:" + currentMorse).c_str());
    
    currentMorse = "";
  }
}

void clearMorse() {
  currentMorse = "";
  Serial.println("Code Morse effacé");
}

// ==================== FONCTIONS MQTT ====================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  if (String(topic) == String("morse/") + deviceId + "/from_web") {
    if (message == "DOT") {
      addToMorse(".");
      playDot();
    } else if (message == "DASH") {
      addToMorse("-");
      playDash();
    } else if (message == "SPACE") {
      addToMorse(" ");
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connecté!");
      
      String subscribeTopic = String("morse/") + deviceId + "/from_web";
      client.subscribe(subscribeTopic.c_str());
      
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

// ==================== FONCTIONS JOYSTICK HW-504 ====================
void printJoystickDebug() {
  int xValue = analogRead(JOYSTICK_X);
  int yValue = analogRead(JOYSTICK_Y);
  int buttonState = digitalRead(JOYSTICK_BUTTON);
  
  Serial.print("X:");
  Serial.print(xValue);
  Serial.print(" Y:");
  Serial.print(yValue);
  Serial.print(" BTN:");
  Serial.println(buttonState);
}

void handleJoystick() {
  int xValue = analogRead(JOYSTICK_X);
  int yValue = analogRead(JOYSTICK_Y);
  int buttonState = digitalRead(JOYSTICK_BUTTON);

  // Anti-rebond
  if (millis() - lastJoystickAction < DEBOUNCE_DELAY) {
    return;
  }

  // DEBUG - Affiche les valeurs toutes les 2 secondes
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    printJoystickDebug();
    lastDebug = millis();
  }

  // Détection des directions avec vérification de la zone centrale
  bool isInCenterX = (xValue > CENTER_MIN && xValue < CENTER_MAX);
  bool isInCenterY = (yValue > CENTER_MIN && yValue < CENTER_MAX);
  
  if (xValue < THRESHOLD_LOW && isInCenterY) {
    // Joystick à GAUCHE - POINT
    Serial.println("Joystick: GAUCHE → Point (•)");
    addToMorse(".");
    playDot();
    lastJoystickAction = millis();
    
  } else if (xValue > THRESHOLD_HIGH && isInCenterY) {
    // Joystick à DROITE - TIRET
    Serial.println("Joystick: DROITE → Tiret (-)");
    addToMorse("-");
    playDash();
    lastJoystickAction = millis();
    
  } else if (yValue > THRESHOLD_HIGH && isInCenterX) {
    // Joystick en BAS - ESPACE
    Serial.println("Joystick: BAS → Espace");
    addToMorse(" ");
    lastJoystickAction = millis();
    
  } else if (yValue < THRESHOLD_LOW && isInCenterX) {
    // Joystick en HAUT - VALIDER
    Serial.println("Joystick: HAUT → Validation");
    validateCharacter();
    lastJoystickAction = millis();
  }

  // Détection du bouton (appuyer)
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Bouton appuyé → Effacer");
    clearMorse();
    delay(DEBOUNCE_DELAY);
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
  
  Serial.println("=== JOYSTICK HW-504 TEST ===");
  Serial.println("Attente de la calibration...");
  delay(2000);
  
  // Affiche les valeurs initiales
  printJoystickDebug();
  
  // Connexions
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  Serial.println("Setup terminé");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  handleJoystick();
  
  delay(50);
}