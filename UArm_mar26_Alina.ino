//library for descriptive statistics
#include <QuickStats.h>
//library for servos
#include <Servo.h>
//library for bluetooth
#include <SoftwareSerial.h>
//library for non-volatile memory
#include <EEPROM.h>

//Instances of Servo class
Servo thumbServo, indexServo, middleServo, ringPinkyServo, thumbSideServo;

//Instance of QuickStats class
QuickStats stats;

//Initialize serial communication on pins 2 and 3
SoftwareSerial BTserial(2, 3);

//define Pins for each servo
#define thumbPin 4
#define indexPin 5
#define middlePin 6
#define ringPinkyPin 7
#define thumbSidePin 8

//define Pins for MyoWare sensors
#define lowerArmPin A2
#define upperArmPin A3
#define bicepsPin A4

//define pin for button
#define buttonPin 12

//declare and initialize variables
//thresholds for each sensor
int lowerArmThreshold = 300;
int upperArmThreshold = 300;
int bicepsThreshold = 400;
//a minimum and a maximum threshold used to check if calculated thresholds are correct
int minThreshold = 200;
int maxThreshold = 600;

//variables to read Myoware Sensor input
int lowerArmValue = 0;
int upperArmValue = 0;
int bicepsValue = 0;

//number of readings for each sensor
const int numReadings = 100;
//array to hold readings
float readings[numReadings] = {0};

//counter
int i = 0;

//variable to ceck if button pressed
int buttonState = 0;
//variable for mode (can be 1 and 2)
int mode = 1;

//boolean variables for calibration
bool calibrated = false;
bool calibratedLowerArmSensor = false;
bool calibratedUpperArmSensor = false;
bool calibratedBicepsSensor = false; 

//array and size of array to hold data read by bluetooth
const int numCharacters = 32;
char receivedChars[numCharacters];
//boolean variable to keep track when new data received from BT
bool newData = false;
//constant characters to be sent or received via bluetooth
//these caracters will be received from Android app
const char START_CALIBRATION = 'C';
const char CALIBRATE_LAS = '1';
const char CALIBRATE_UAS = '2';
const char CALIBRATE_BS = '3';
//these characters will be sent to Android app
const char LAS_CALIBRATED = 'A';
const char UAS_CALIBRATED = 'B';
const char BS_CALIBRATED = 'C';
const char CALIBRATION_FINISHED = 'F';



//addresses where thresholds are stored in EEPROM
int addressLowerArmThreshold = 0;
int addressUpperArmThreshold = 2;
int addressBicepsThreshold = 4; 

//functions declaration
//Prints average, median, mean, maximum, minimum of an array of floats
void printStatistics( float *array, int size);

//Reads "numReadings" times from the analog input pin and it returns the median
int sensorCalibration(int pin);
//Checks validity of a threshold
bool isThresholdValid(int thresh);

//Reads the analog input pin 10x and returns an average
int getSensorValue(int pin);

//Functions for gestures of the arm
void open_hand();
void close_hand();
void pointing();
void pincing();
void peace_out();
void alright();
void  thumb_up();
void come_here();
void doAllGestures();

//Bluetooth communications functions
void showNewData();
void receiveData();
void processIncomingData();
void sendData(char data);

//writing thresholds to EEPROM
void writeThresholdsToEEPROM();
void readThresholdsFromEEPROM();

/**********************************************************************
 This is the setup function. It runs once when the Arduino is turned on
 **********************************************************************/
void setup() {
  // put your setup code here, to run once:

   //start serial communication with Serial Monitor and bluetooth module
  Serial.begin(9600);
  BTserial.begin(9600);

  //attach servos
  thumbServo.attach(thumbPin);
  indexServo.attach(indexPin);
  middleServo.attach(middlePin);
  ringPinkyServo.attach(ringPinkyPin);
  thumbSideServo.attach(thumbSidePin);

  open_hand();

  //We need to read all the thresholds stored in EEPROM and turn the calibrated boolean variables true
  //writeThresholdsToEEPROM();
  readThresholdsFromEEPROM();
  
  //Serial.println(lowerArmThreshold);
  //Serial.println(upperArmThreshold);
  //Serial.println(bicepsThreshold);

  calibrated = true;
  //run this function to check that all servos are working properly
  //doAllGestures();

}


/******************************************
 This is the main loop that repeats forever
 *****************************************/
void loop() {
  // put your main code here, to run repeatedly:

  receiveData();
  showNewData();

  //check if calibrated
  if (!calibrated) {
    if(!calibratedLowerArmSensor) {
      //calibrate lowerArm sensor
      lowerArmThreshold = sensorCalibration(lowerArmPin);
      //check if threshold is valid
      if(isThresholdValid(lowerArmThreshold)) {
        calibratedLowerArmSensor = true;
        //sendData to app to inform that step 1 completed
        sendData(LAS_CALIBRATED);
      } 
    }
    //calibrate upperArm sensor
    if(!calibratedUpperArmSensor) {
      //calibrate lowerArm sensor
      upperArmThreshold = sensorCalibration(upperArmPin);
      //check if threshold is valid
      if(isThresholdValid(upperArmThreshold)) {
        calibratedUpperArmSensor = true;
        //sendData to app to inform that step 1 completed
        sendData(UAS_CALIBRATED);
      } 
    }
    //calibrate biceps sensor
    if(!calibratedBicepsSensor) {
      //calibrate lowerArm sensor
      bicepsThreshold = sensorCalibration(bicepsPin);
      //check if threshold is valid
      if(isThresholdValid(bicepsThreshold)) {
        calibratedBicepsSensor = true;
        //sendData to app to inform that step 1 completed
        sendData(BS_CALIBRATED);
      } 
    }
    //We should store this values in the EEPROM memory
    writeThresholdsToEEPROM();
    //If all sensor were calibrated, make calibrated variable true
    if(calibratedLowerArmSensor && calibratedUpperArmSensor && calibratedBicepsSensor) {
      calibrated = true;
      sendData(CALIBRATION_FINISHED);
    }
  }


/*

  //get sensor values at lowerArmPin, upperArmPin, bicepsPin
  lowerArmValue = getSensorValue(lowerArmPin);
  upperArmValue = getSensorValue(upperArmPin);
  bicepsValue = getSensorValue(bicepsPin);

  //Read button state, if pressed
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton was pressed and change modes accordingly
  if (buttonState == HIGH && mode == 1)
  {
    mode = 2;

  }
  else if (buttonState == HIGH && mode == 2)
  {
    mode = 1;
  }

  //Checking modes
  if (mode == 1)
  {
    //check if sensor value is higher then threshold and move the arm accordingly
    if (bicepsValue > bicepsThreshold && lowerArmValue < lowerArmThreshold)
    { close_hand();
      delay(1500);
    }
    else if (lowerArmValue > lowerArmThreshold)
    { pincing();
      delay(1500);
    }
    else if (upperArmValue > upperArmThreshold)
    { pointing();
      delay(1500);
    }
    else if (bicepsValue < bicepsThreshold && upperArmValue < upperArmThreshold && lowerArmValue < lowerArmThreshold)
    { open_hand();
      delay(1500);
    }
  }
  else if (mode == 2)
  {
    if (bicepsValue > bicepsThreshold && lowerArmValue < lowerArmThreshold)
    { peace_out();
      delay(1500);
    }
    else if (lowerArmValue > lowerArmThreshold)
    { alright();
      delay(1500);
    }
    else if (upperArmValue > upperArmThreshold)
    { come_here();
      delay(1500);
    }
    else if (bicepsValue < bicepsThreshold && upperArmValue < upperArmThreshold && lowerArmValue < lowerArmThreshold)
    { open_hand();
      delay(1500);
    }
  }



  delay(100);

*/
}



/**********************************************
 Function definition
 ***********************************************/
void printStatistics( float *array, int size)
{
  Serial.println("Descriptive Statistics");
  Serial.print("Average: ");
  Serial.println(stats.average(array, size));
  Serial.print("Geometric mean: ");
  Serial.println(stats.g_average(array, size));
  Serial.print("Median: ");
  Serial.println(stats.median(array, size));
  Serial.print("Minimum: ");
  Serial.println(stats.minimum(array, size));
  Serial.print("Maximum: ");
  Serial.println(stats.maximum(array, size));
  Serial.println("\n");
}

int sensorCalibration(int pin)
{
  float array[numReadings] = {0};
  for (int i = 0; i < numReadings; i++)
  {
    array[i] = analogRead(pin);
    delay(100);
  }

  printStatistics(array, numReadings);
  return stats.average(array, numReadings);
}

int getSensorValue(int pin)
{
  int value = 0;
  for (int i = 0; i < 10; i++)
  {
    value += analogRead(pin);
  }
  return value / 10;
}

void doAllGestures()
{
  close_hand();
  delay(500);
  open_hand();
  delay(500);
  pincing();
  delay(500);
  open_hand();
  delay(500);
  pointing();
  delay(500);
  open_hand();
  delay(500);
  peace_out();
  delay(500);
  open_hand();
  delay(500);
  thumb_up();
  delay(500);
  open_hand();
  delay(500);
}


void open_hand()
{
  thumbServo.write(100);
  indexServo.write(180);
  middleServo.write(0);
  ringPinkyServo.write(0);
  thumbSideServo.write(70);
  delay(500);
}

void close_hand()
{
  thumbServo.write(10);
  indexServo.write(0);
  middleServo.write(180);
  ringPinkyServo.write(180);
  thumbSideServo.write(30);
  delay(500);
}

void pointing()
{
  thumbServo.write(10);
  indexServo.write(180);
  middleServo.write(180);
  ringPinkyServo.write(180);
  thumbSideServo.write(30);
  delay(500);
}

void pincing()
{
  thumbServo.write(30);
  indexServo.write(20);
  middleServo.write(100);
  ringPinkyServo.write(0);
  thumbSideServo.write(0);
  delay(500);
}

void peace_out()
{
  thumbServo.write(10);
  indexServo.write(180);
  middleServo.write(0);
  ringPinkyServo.write(180);
  thumbSideServo.write(20);
  delay(500);
}

void alright()
{
  thumbServo.write(40);
  indexServo.write(20);
  middleServo.write(0);
  ringPinkyServo.write(0);
  thumbSideServo.write(0);
  delay(500);
}

void  thumb_up()
{
  thumbServo.write(100);
  indexServo.write(0);
  middleServo.write(180);
  ringPinkyServo.write(180);
  thumbSideServo.write(70);
  delay(500);
}

void come_here()
{
  while ( bicepsValue >= bicepsThreshold &&  upperArmValue >= upperArmThreshold)
  {
    close_hand();
    delay(400);
    pointing();
    delay(400);
    upperArmValue = analogRead(upperArmPin);
    // lowerarm = analogRead(A2);
    bicepsValue = analogRead(bicepsPin);
  }
}
void receiveData() {
  static boolean recvInProgress = false;
  static int ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char character;

  while (BTserial.available() > 0 && newData == false) {
    character = BTserial.read();

    if (recvInProgress == true) {
      if (character != endMarker) {
        receivedChars[ndx] = character;
        ndx++;
        if (ndx >= numCharacters) {
          ndx = numCharacters - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (character == startMarker) {
      recvInProgress = true;
    }
  }
}

void showNewData() {
  if (newData == true) {
    Serial.print("This just in ... ");
    Serial.println(receivedChars);
    newData = false;
  }
}

void processIncomingData() {
  if(newData == true) {
    char data = receivedChars[0];
    if(data == START_CALIBRATION) {
      calibrated = false;
    }
    if(data == CALIBRATE_LAS) {
      calibratedLowerArmSensor = false;
    }
    if(data == CALIBRATE_UAS) {
      calibratedUpperArmSensor = false;
    }
    if(data == CALIBRATE_BS) {
      calibratedBicepsSensor = false;
    }
  }
}

void sendData(char data) {
  BTserial.print(data);
}

bool isThresholdValid(int thresh) {
  if(thresh >= minThreshold && thresh <= maxThreshold) {
    return true;
  }
  else
  {
    return false;
    //some code to send data to UArm app to redo calibration 
  }
}

void writeThresholdsToEEPROM() {
  EEPROM.put(addressLowerArmThreshold, lowerArmThreshold);
  EEPROM.put(addressUpperArmThreshold, upperArmThreshold);
  EEPROM.put(addressBicepsThreshold, bicepsThreshold);
}
void readThresholdsFromEEPROM() {
  EEPROM.get(addressLowerArmThreshold, lowerArmThreshold);
  EEPROM.get(addressUpperArmThreshold, upperArmThreshold);
  EEPROM.get(addressBicepsThreshold, bicepsThreshold);
}



