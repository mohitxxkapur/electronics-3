#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "time.h"

// helper functions
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Various credentials
#define WIFI_SSID "Sauga Strawhats"
#define WIFI_PASSWORD "gabagool"

#define API_KEY "AIzaSyDM_2SgfLlHz7ghEMBv7xnqW6KxHi0RECs"

#define DATABASE_URL "https://electronics3-4fcf1-default-rtdb.firebaseio.com/"

//authorized user information
#define USER_EMAIL "mohit19k9@gmail.com"
#define USER_PASSWORD "mohito"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String precipPath = "/precipitation";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

int rainPin = 36;
int dryValue = 4095;
int lightValue = 3250;
int heavyValue = 1000;

//send data every 10 seconds
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

//getting current time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(9600);

  pinMode(rainPin, INPUT);
  
  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}

void loop() {

  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    int sensorValue = analogRead(rainPin);
    Serial.print(sensorValue);
    if (sensorValue < dryValue) {
      json.set(precipPath.c_str(), "It's wet");
    } else if (sensorValue < lightValue) {
      json.set(precipPath.c_str(), "It's light wet");
    } else if (sensorValue < heavyValue) {
      json.set(precipPath.c_str(), "It's heavy wet");
    } else {
      json.set(precipPath.c_str(), String(sensorValue)+"It's dry");
    }

    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}