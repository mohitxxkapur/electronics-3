#include <HCSR04.h>

#include <Arduino.h>
#include <heltec.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "time.h"

#include "DHT.h"
#define DHT11PIN 16

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int rainPin = 36;
int dryValue = 4095;
int lightValue = 3250;
int heavyValue = 1000;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DHT11PIN 16
#define trigPin_USS 4
#define echoPin_USS 18
#define WAIT_FOR_VEHICLE 0
#define WAIT_FOR_NO_VEHICLE 1

int counter_uss = 0;  //Initialize the counter_uss
DHT dht(DHT11PIN, DHT11);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//firebase connection setup
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
String countPath = "/count";
String tempPath = "/temperature";
String humiPath = "/humidity";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

//send data every 10 seconds
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Additional methods

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

float MeasureDistance(void) {
  unsigned long
    duration;

  digitalWrite(trigPin_USS, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_USS, LOW);

  duration = pulseIn(echoPin_USS, HIGH);
  return ((float)duration / 2.0) / 29.1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);  // Initialize serial communication

  pinMode(trigPin_USS, OUTPUT);
  pinMode(echoPin_USS, INPUT);
  dht.begin();
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

  uss_readings();
  Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

  update_firebase();
}
/////////////////////////////

void update_firebase() {

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    temp_readings();
    precipitation_readings();

    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}

// sensor readings ////////////////////////////////////////////////////
void uss_readings() {
  static byte
    state = WAIT_FOR_VEHICLE;  //start with waiting for a vehicle
  float
    dist;

  switch (state) {
    case WAIT_FOR_VEHICLE:
      //get the distance...
      dist = MeasureDistance();
      //...if it's less than 30cm and greater than zero (pulseIn returns 0 if no pulse seen...)...
      if (dist <= 30.0 && dist > 0.0) {
        //bump the count then move to the state where we wait for the sensor to show
        //clear again
        counter_uss++;
        // Serial.print(F("Count is: "));
        // Serial.println(counter_uss);
        json.set(countPath.c_str(), "Cars passed: " + String(counter_uss));
        state = WAIT_FOR_NO_VEHICLE;

      }  //if

      break;

    case WAIT_FOR_NO_VEHICLE:
      //sensor must show something >40cm away (or show ~0.0) before we say the coast is clear
      //I used 0.01 instead of "==0.0" in case the math returns a float that's tiny but not
      //_equal_ to zero.
      dist = MeasureDistance();
      if (dist > 40.0 || dist <= 0.01)
        //when condition met, go back to waiting for vehicle again
        state = WAIT_FOR_VEHICLE;

      break;

  }  //switch
}

void precipitation_readings() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(rainPin);
  Serial.print(sensorValue);
  if (sensorValue == dryValue) {
    json.set(precipPath.c_str(), String(sensorValue) + " - It's dry");
  } else if (lightValue <= sensorValue < dryValue) {
    json.set(precipPath.c_str(), String(sensorValue) + " - Drizzling");
  } else if (heavyValue <= sensorValue < lightValue) {
    json.set(precipPath.c_str(), String(sensorValue) + " - Light precipitation");
  } else if (sensorValue < heavyValue) {
    json.set(precipPath.c_str(), String(sensorValue) + " - Heavy precipitation");
  }
}
void temp_readings() {
  float humi = dht.readHumidity();
  float temp = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" degrees C. ");
  Serial.print("Humidity: ");
  Serial.print(humi);
  Serial.println("%");
  json.set(tempPath.c_str(), String(temp) + " degrees celcius");
  json.set(humiPath.c_str(), "Humidity of " + String(humi) + " %");
}
