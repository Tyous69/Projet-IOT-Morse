#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Configuration WiFi - MODIFIEZ CES VALEURS
const char* ssid = "VOTRE_WIFI_SSID";
const char* password = "VOTRE_MOT_DE_PASSE_WIFI";

// Configuration MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* device_id = "ESP32_MORSE_001";

// Pins pour ESP32
const int JOYSTICK_X_PIN = 34;
const int JOYSTICK_BUTTON_PIN = 35;
const int BUZZER_PIN = 25;
const int LED_PIN = 2;

// Variables Morse
String currentMorse = "";
unsigned long lastInputTime = 0;
const unsigned long MORSE_TIMEOUT = 2000;

WiFiClient espClient;
PubSubClient client(espClient);

// Dictionnaire Morse
const char* morseLetters[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", 
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."
};

const char* morseNumbers[] = {
  "-----", ".----", "..---", "...--", "....-", ".....", 
  "-....", "--...", "---..", "----."
};

void setupWiFi();
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void addMorseSymbol(char symbol);
void addSpace();
void translateMorse();
char morseToChar(String morse);
void beep(int duration);
void blinkLED(int times);
void readJoystick();
void checkMorseTimeout();

void setup() {
  Serial.begin(115200);
  
  // Configuration des pins
  pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(LED_PIN, LOW);
  
  setupWiFi();
  setupMQTT();
  
  Serial.println("Setup terminé - Prêt à utiliser");
  digitalWrite(LED_PIN, HIGH);
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connecté");
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("");
    Serial.println("Échec de connexion WiFi");
    digitalWrite(LED_PIN, LOW);
  }
}

void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void reconnectMQTT() {
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    Serial.print("Tentative de connexion MQTT...");
    
    if (client.connect(device_id)) {
      Serial.println("connecté");
      String subscribeTopic = "morse/" + String(device_id) + "/from_web";
      client.subscribe(subscribeTopic.c_str());
      
      String statusTopic = "morse/" + String(device_id) + "/status";
      client.publish(statusTopic.c_str(), "ESP32 connecté");
    } else {
      Serial.print("échec, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
      attempts++;
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (message == "DOT") {
    addMorseSymbol('.');
  } else if (message == "DASH") {
    addMorseSymbol('-');
  } else if (message == "SPACE") {
    addSpace();
  } else if (message.startsWith("TRANSLATION:")) {
    String translation = message.substring(12);
    String responseTopic = "morse/" + String(device_id) + "/from_device";
    String responseMsg = "TRANSLATION_RECEIVED:" + translation;
    client.publish(responseTopic.c_str(), responseMsg.c_str());
  }
}

void addMorseSymbol(char symbol) {
  currentMorse += symbol;
  lastInputTime = millis();
  
  beep(symbol == '.' ? 100 : 300);
  blinkLED(symbol == '.' ? 1 : 3);
  
  String topic = "morse/" + String(device_id) + "/from_device";
  String message = "MORSE:" + currentMorse;
  client.publish(topic.c_str(), message.c_str());
  
  Serial.print("Symbole ajouté: ");
  Serial.println(symbol);
}

void addSpace() {
  if (currentMorse.length() == 0) {
    String topic = "morse/" + String(device_id) + "/from_device";
    client.publish(topic.c_str(), "SPACE");
    Serial.println("Espace ajouté");
  } else {
    translateMorse();
    currentMorse = "";
  }
  lastInputTime = millis();
}

void translateMorse() {
  if (currentMorse.length() == 0) return;
  
  char translatedChar = morseToChar(currentMorse);
  
  if (translatedChar != '?') {
    beep(200);
    delay(100);
    beep(200);
  }
  
  String topic = "morse/" + String(device_id) + "/from_device";
  String message = "CHAR:";
  message += translatedChar;
  client.publish(topic.c_str(), message.c_str());
  
  Serial.print("Caractère traduit: '");
  Serial.print(translatedChar);
  Serial.println("'");
}

char morseToChar(String morse) {
  // Lettres (A-Z)
  for (int i = 0; i < 26; i++) {
    if (morse == morseLetters[i]) {
      return 'A' + i;
    }
  }
  
  // Chiffres (0-9)
  for (int i = 0; i < 10; i++) {
    if (morse == morseNumbers[i]) {
      return '0' + i;
    }
  }
  
  // Caractères spéciaux
  if (morse == ".-.-.-") return '.';
  if (morse == "--..--") return ',';
  if (morse == "..--..") return '?';
  if (morse == "-....-") return '-';
  if (morse == "-..-.") return '/';
  
  return '?';
}

void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    if (i < times - 1) delay(100);
  }
}

void readJoystick() {
  static unsigned long lastJoystickRead = 0;
  if (millis() - lastJoystickRead < 200) return;
  
  int xValue = analogRead(JOYSTICK_X_PIN);
  int buttonState = digitalRead(JOYSTICK_BUTTON_PIN);
  
  if (xValue < 1000) {
    addMorseSymbol('.');
    lastJoystickRead = millis();
  } else if (xValue > 3000) {
    addMorseSymbol('-');
    lastJoystickRead = millis();
  }
  
  if (buttonState == LOW) {
    addSpace();
    lastJoystickRead = millis();
  }
}

void checkMorseTimeout() {
  if (currentMorse.length() > 0 && millis() - lastInputTime > MORSE_TIMEOUT) {
    Serial.println("Timeout - Traduction automatique");
    translateMorse();
    currentMorse = "";
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi déconnecté - tentative de reconnexion");
    setupWiFi();
  }
  
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  readJoystick();
  checkMorseTimeout();
  
  delay(50);
}