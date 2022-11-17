int rainPin = 36;
int dryValue = 4095;
int lightValue = 3250;
int heavyValue = 1000;

void setup(){
  pinMode(rainPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(rainPin);
  Serial.print(sensorValue);
  if(sensorValue < dryValue){
    Serial.println(" - It's wet");
  }
  else if(sensorValue < lightValue){
    Serial.println(" - It's light wet");
  }
  else if(sensorValue < heavyValue){
    Serial.println(" - It's heavy wet");
  }
  else {
    Serial.println(" - It's dry");
  }
  delay(500);
}