#include <HCSR04.h>

#include <Arduino.h>
#include <heltec.h>

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire ds(25);
DallasTemperature sensors(&ds);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int rainPin = 36;
int dryValue = 4095;
int lightValue = 3250;
int heavyValue = 1000;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define trigPin_USS 4
#define echoPin_USS 18
#define WAIT_FOR_VEHICLE 0
#define WAIT_FOR_NO_VEHICLE 1

int counter_uss = 0;  //Initialize the counter_uss

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);  // Initialize serial communication

  pinMode(trigPin_USS, OUTPUT);
  pinMode(echoPin_USS, INPUT);

  pinMode(rainPin, INPUT);

  //Serial.println(" Serial Begins");
  // Start the DS18B20 sensor
  sensors.begin();

  //uss_setup();
  //precipitation_setup();
}

void loop() {

  uss_readings();
  precipitation_readings();
  temp_readings();
}

//SETUPS////////////////////////////////
void uss_setup() {
  pinMode(trigPin_USS, OUTPUT);
  pinMode(echoPin_USS, INPUT);
}

void precipitation_setup() {
  //precipitation se2nsor setup
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
        Serial.print(F("Count is: "));
        Serial.println(counter_uss);
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
  if (sensorValue == dryValue){
    Serial.println(" - Dry");
  }
    else if (lightValue <= sensorValue < dryValue) {
    Serial.println(" - Drizzling");
  } else if (heavyValue <= sensorValue < lightValue) {
    Serial.println(" - Light Rain");
  } else if (sensorValue < heavyValue) {
    Serial.println(" - Heavy Rain");
  }
  delay(250);
}
void temp_readings() {
  sensors.requestTemperatures();
  float Temperature = (float)(sensors.getTempCByIndex(0));
  float temperatureC = sensors.getTempCByIndex(0);
  //float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(Temperature);
  Serial.println(" C");
  // Serial.print(temperatureF);
  // Serial.println("ºF");
  delay(250);
  // Serial.println("Go to sleep");
  // esp_deep_sleep(30e6);
}

//extra methods//////////////////////////////////////////////////////////////////////////////////////////
float MeasureDistance(void) {
  unsigned long
    duration;

  digitalWrite(trigPin_USS, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_USS, LOW);

  duration = pulseIn(echoPin_USS, HIGH);
  return ((float)duration / 2.0) / 29.1;

}  //MeasureDistance