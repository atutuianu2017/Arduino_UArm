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
int lowerArmThreshold = 0;
int upperArmThreshold = 300;
int bicepsThreshold = 400;
//a minimum and a maximum threshold used to check if calculated thresholds are correct
int minThreshold = 100;
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

//Reads "numReadings" times from the analog input pin and it returns the average
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

//writing thresholds to EEPROM
void writeThresholdsToEEPROM();
void readThresholdsFromEEPROM();

boolean now = true;


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

  //set UArm to open position
  open_hand();

  //We need to read all the thresholds stored in EEPROM and turn the calibrated boolean variables true
  //writeThresholdsToEEPROM();
  //we need to check that there exist a valid threshold in eeprom.
  //readThresholdsFromEEPROM();

  //Run next code if you need to see on the Serial Monitor what values were read from EEPROM
  //Serial.println(lowerArmThreshold);
  //Serial.println(upperArmThreshold);
  //Serial.println(bicepsThreshold);

  calibrated = true;
  calibratedLowerArmSensor = true;
  calibratedUpperArmSensor = true;
  calibratedBicepsSensor = true;


  //run this function to check that all servos are working properly
  //doAllGestures();

}


/******************************************
  This is the main loop that repeats forever
 *****************************************/
void loop() {
  // put your main code here, to run repeatedly:

  //read data coming from Android app via bluetooth
  receiveData();
  //display data on Serial Monitor
  showNewData();
  //process data (turn calibration flags false in order to start the calibration process)
  processIncomingData();
  


  //check if calibrated
  if (!calibrated) {
    if (!calibratedLowerArmSensor) {
      //calibrate lowerArm sensor
      lowerArmThreshold = sensorCalibration(lowerArmPin);
      //check if threshold is valid
      Serial.println("The new value of lower arm threshold is: ");
      Serial.println(lowerArmThreshold);
      if (isThresholdValid(lowerArmThreshold)) {
        calibratedLowerArmSensor = true;
        //sendData to app to inform that step 1 completed
        Serial.println("The calibratedLowerArmSensor was turned: ");
        Serial.println(calibratedLowerArmSensor);
      }
    }
    //calibrate upperArm sensor
    if (!calibratedUpperArmSensor) {
      //calibrate lowerArm sensor
      upperArmThreshold = sensorCalibration(upperArmPin);
      //check if threshold is valid
      if (isThresholdValid(upperArmThreshold)) {
        calibratedUpperArmSensor = true;
        Serial.println("The calibratedUpperArmSensor was turned: ");
        Serial.println(calibratedUpperArmSensor);      }
    }
    //calibrate biceps sensor
    if (!calibratedBicepsSensor) {
      //calibrate lowerArm sensor
      bicepsThreshold = sensorCalibration(bicepsPin);
      //check if threshold is valid
      if (isThresholdValid(bicepsThreshold)) {
        calibratedBicepsSensor = true;
        Serial.println("The calibratedBicepsSensor was turned: ");
        Serial.println(calibratedBicepsSensor);
      }
    }
  
    //If all sensor were calibrated, make calibrated variable true
    if (calibratedLowerArmSensor && calibratedUpperArmSensor && calibratedBicepsSensor) {
      calibrated = true;
      //We should store this values in the EEPROM memory
      writeThresholdsToEEPROM();
    }
  }
 /* else 
  {
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
      { 
        close_hand();
        delay(1500);
      }
      else if (lowerArmValue > lowerArmThreshold)
      { 
        pincing();
        delay(1500);
      }
      else if (upperArmValue > upperArmThreshold)
      { 
        pointing();
        delay(1500);
      }
      else if (bicepsValue < bicepsThreshold && upperArmValue < upperArmThreshold && lowerArmValue < lowerArmThreshold)
      { 
        open_hand();
        delay(1500);
      }
    }
    else if (mode == 2)
    {
      if (bicepsValue > bicepsThreshold && lowerArmValue < lowerArmThreshold)
      { 
        peace_out();
        delay(1500);
      }
      else if (lowerArmValue > lowerArmThreshold)
      { 
        alright();
        delay(1500);
      }
      else if (upperArmValue > upperArmThreshold)
      { 
        come_here();
        delay(1500);
      }
      else if (bicepsValue < bicepsThreshold && upperArmValue < upperArmThreshold && lowerArmValue < lowerArmThreshold)
      { 
        open_hand();
        delay(1500);
      }
    }
  }*/
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
  unsigned long startMillis = millis();
  unsigned long endMillis;
  int array[numReadings] = {0};
  int average = 0;
  for (int i = 0; i < numReadings; i++)
  {
    array[i] = analogRead(pin);
    average += array[i];
    delay(100);
  }
 // endMillis = millis();
  //Serial.println("Time used by calibration: ");
  //Serial.println(endMillis-startMillis);

  // printStatistics(array, numReadings);
  return average / numReadings;
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

  }
}

void processIncomingData() {
  if (newData == true) {
    char data = receivedChars[0];
    //Serial.println("This is the data read from receivedChars array");
    //Serial.println(data);
    if (data == CALIBRATE_LAS) {
      calibrated = false;
      calibratedLowerArmSensor = false;
      Serial.print("calibratedLowerArmSensor value = ");
      Serial.println(calibratedLowerArmSensor);
    }
    if (data == CALIBRATE_UAS) {
      calibratedUpperArmSensor = false;
      Serial.print("calibratedUpperArmSensor value = ");
      Serial.println(calibratedUpperArmSensor);
    }
    if (data == CALIBRATE_BS) {
      calibratedBicepsSensor = false;
      Serial.print("calibratedBicepsSensor value = ");
      Serial.println(calibratedBicepsSensor);
    }
  }
  newData = false;
}


bool isThresholdValid(int thresh) {
  if (thresh >= minThreshold && thresh <= maxThreshold) {
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



