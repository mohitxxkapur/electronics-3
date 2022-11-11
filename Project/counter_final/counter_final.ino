#define trigPin_USS 4
#define echoPin_USS 18

int counter = 0;  //Initialize the counter

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  pinMode(trigPin_USS, OUTPUT);
  pinMode(echoPin_USS, INPUT);

}  //setup

//statemachine state names
#define WAIT_FOR_VEHICLE 0
#define WAIT_FOR_NO_VEHICLE 1
//
void loop() {
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
        counter++;
        Serial.print(F("Count is: "));
        Serial.println(counter);
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

}  //loop

float MeasureDistance(void) {
  unsigned long
    duration;

  digitalWrite(trigPin_USS, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_USS, LOW);

  duration = pulseIn(echoPin_USS, HIGH);
  return ((float)duration / 2.0) / 29.1;

}  //MeasureDistance