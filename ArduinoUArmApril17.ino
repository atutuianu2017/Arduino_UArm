/***************************************************************************
  Arduino Code for the UArm
  Senior project developed by Naiara Foster, Eliezer Montes,
  Leonardo Rivas, and Alina Tutuianu
 ***************************************************************************/
//library for servos
#include <Servo.h>
//library for bluetooth
#include <SoftwareSerial.h>
//library for non-volatile memory
#include <EEPROM.h>
//library for Voltage reading and LCD display
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
SSD1306AsciiWire oled;

//Instances of Servo class
Servo thumbServo, indexServo, middleServo, ringPinkyServo, thumbSideServo;

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
#define bicepsPin A1

//define pin for button
#define buttonPin 12

//define display screen
// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C
// Define proper RST_PIN if required.
#define RST_PIN -1

//declare and initialize variables
//thresholds for each sensor
int lowerArmThreshold = 0;
int upperArmThreshold = 0;
int bicepsThreshold = 0;
//a minimum and a maximum threshold used to check if calculated thresholds are correct
int minThreshold = 150;
int maxThreshold = 600;

//variables to read Myoware Sensor input
int lowerArmValue = 0;
int upperArmValue = 0;
int bicepsValue = 0;

//flag
int flag = 1;


//variable to ceck if button pressed
int buttonState = 0;
//variable for mode (can be 1 and 2)
int mode = 1;
//variable for gesture name for LCD display
String gesture = " open hand";

//boolean variables for calibration
bool calibrated = false;
bool calibratedLowerArmSensor = false;
bool calibratedUpperArmSensor = false;
bool calibratedBicepsSensor = false;

//array and size of array to hold data read by bluetooth
const byte numCharacters = 32;
char receivedChars[numCharacters];
//boolean variable to keep track when new data received from BT
bool newData = false;

//addresses where thresholds are stored in EEPROM
int addressLowerArmThreshold = 0;
int addressUpperArmThreshold = 2;
int addressBicepsThreshold = 4;

//functions declaration

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
void doAllGestures();

//Bluetooth communications functions
void showNewData();
void receiveData();
void processIncomingData();

//writing thresholds to EEPROM
void writeThresholdsToEEPROM();
void readThresholdsFromEEPROM();

//functions to get the battery value
double readVcc();
//functions that display battery life into the LCD screen
void displayingBatteryLife();


/**********************************************************************
  This is the setup function. It runs once when the Arduino is turned on
 **********************************************************************/
void setup() {
  // put your setup code here, to run once:

  //start serial communication with Serial Monitor, bluetooth module
  Serial.begin(9600);
  BTserial.begin(9600);

  //LCD screen setup
  Wire.begin();
  Wire.setClock(400000L);
  #if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  //attach servos
  thumbServo.attach(thumbPin);
  indexServo.attach(indexPin);
  middleServo.attach(middlePin);
  ringPinkyServo.attach(ringPinkyPin);
  thumbSideServo.attach(thumbSidePin);

  //set UArm to open position
  open_hand();

  // button colors
   randomSeed(analogRead(0));
   analogWrite(11, HIGH);
   
  //display batttery life
   displayingBatteryLife();

  //We need to read all the thresholds stored in EEPROM and turn the calibrated boolean variables true
  //writeThresholdsToEEPROM();
  //we need to check that there exist a valid threshold in eeprom.
  readThresholdsFromEEPROM();
  calibrated = true;

  //Run next code if you need to see on the Serial Monitor what values were read from EEPROM
  //Serial.println(lowerArmThreshold);
  //Serial.println(upperArmThreshold);
  //Serial.println(bicepsThreshold);



  //run this function to check that all servos are working properly
 // doAllGestures();

}


/******************************************
  This is the main loop that repeats forever
 *****************************************/
void loop() {
  //read data coming from Android app via bluetooth
  receiveData();
  //display data on Serial Monitor
  showNewData();
  // process data 
  //(calibrate sensors based on input from Android app; make calibrated false to prevent the UArm from moving while calibrating)
  processIncomingData();
  
  // put your main code here, to run repeatedly:
   displayingBatteryLife();
  
  //check if calibrated; 
  if(calibrated)
  {
    //check if thresholds read from EEPROM are valid 
    //if they are not valid display an error message on the LCD
    if(!isThresholdValid(lowerArmThreshold) || !isThresholdValid(upperArmThreshold) || !isThresholdValid(bicepsThreshold)) {
      //print a message to the LCD screen stating that UArm needs calibration
    }
    else //movement code
    { 
      //get sensor values at lowerArmPin, upperArmPin, bicepsPin
      lowerArmValue = getSensorValue(lowerArmPin);
      upperArmValue = getSensorValue(upperArmPin);
      bicepsValue = getSensorValue(bicepsPin);

      //check if sensor value is higher then threshold and convert to digital
       if (lowerArmValue > lowerArmThreshold) { lowerArmValue = HIGH; upperArmValue = LOW; bicepsValue=LOW;}
       else  if (bicepsValue > bicepsThreshold) { bicepsValue = HIGH; lowerArmValue = LOW; upperArmValue = LOW;}
       else if (upperArmValue > upperArmThreshold) { upperArmValue = HIGH; bicepsValue=LOW; lowerArmValue = LOW; }


      //Read button state, if pressed
      buttonState = digitalRead(buttonPin);

      // check if the pushbutton was pressed and change modes accordingly
      if (buttonState == HIGH && mode == 1) { 
        mode = 2;           //change mode
        analogWrite(10, 0); //turn blue RGB LED off
        analogWrite(11, 255); //turn green RGB LED on
       }

      else if (buttonState == HIGH && mode == 2) { 
        mode = 1;           //change mode
        analogWrite(11, 0); //turn green RGB LED off
        analogWrite(10, 255); //turn blue RGB LED on
        }

      //Do UArm gesture according to mode
      if (mode == 1) {
        //when flag=1 do gesture, when flag=2 open hand
        if(flag == 1) {
          if(bicepsValue == HIGH) {
            close_hand();
            Serial.println(F("close hand"));
            gesture = " close hand";
            flag = 2;
          }
          else if (lowerArmValue == HIGH) {
            pincing();
            Serial.println(F("pincing"));
            gesture = " pincing";
            flag = 2;
          }
          else if(upperArmValue == HIGH) {
            pointing();
            gesture = " pointing";
            Serial.println(F("pointing"));
            flag = 2;
          }
        }
        else if(flag == 2) {
          if(bicepsValue == HIGH || lowerArmValue == HIGH || upperArmValue == HIGH) {
            open_hand();
            Serial.println(F("open hand"));
            gesture = " open hand";
            flag = 1;
          } 
        }
      }
      else if (mode == 2) {

        //when flag=1 do gesture, when flag=2 open hand
        if(flag == 1) {
          if(bicepsValue == HIGH) {
            peace_out();
            Serial.println(F("peace out"));
            gesture = " peace out";
            flag = 2;
          }
          else if (lowerArmValue == HIGH) {
            alright();
            Serial.println(F("alright"));
            gesture = " alright";
            flag = 2;
          }
          else if(upperArmValue == HIGH) {
            thumb_up();
            gesture = " thumb up";
            Serial.println(F("thumb up"));
            flag = 2;
          }
        }
        else if(flag == 2) {
          if(bicepsValue == HIGH || lowerArmValue == HIGH || upperArmValue == HIGH) {
            open_hand();
            Serial.println(F("open hand"));
            gesture = " open hand";
            flag = 1;
          } 
        }
      }
      //display battery life, gesture, and mode on the LCD
      displayingBatteryLife();
      //a delay for better sensor readings
      delay(1500);
    }
  }
}
//end of main loop


/**********************************************
  Function definitions
 ***********************************************/
 
//This function reads numReadings times from the analog pin and returns the average
//The function is used to calibrate each sensor (finding a threshold for each sensor)
int sensorCalibration(int pin)
{
  const int numReadings = 150; //number of readings for each sensor
  int reading = 0; //variable to hold reading from sensor
  long average = 0; //accumulator
  for (int i = 0; i < numReadings; i++) //performing numReadings readings
  {
    reading = analogRead(pin);    //analog read from sensor pin
    average+=reading;             //add current reading to accumulator
    delay(50);                    //delay for more accurate reading
  }
 
  return average / numReadings;   //return average 
}

//This function is used to get a current reading from the sensors
//We do 10 readings and return an average in order to assure an accurate reading
//It is used inside the movement part of the code
int getSensorValue(int pin)
{
  int value = 0;
  for (int i = 0; i < 10; i++)
  {
    value += analogRead(pin);
  }
  return value / 10;
}

//This function is used to read data coming from Android app via Bluetooth
//Data sent from app needs to have a start (<) and end marker (>)
void receiveData() {
  static boolean recvInProgress = false;
  static int ndx = 0; //current position in array of readings
  char startMarker = '<';
  char endMarker = '>';
  char character;

  while (BTserial.available() > 0 && newData == false) {
    character = BTserial.read();  //reading from the bluetooth buffer

    if (recvInProgress == true) {
      if (character != endMarker) {//reading meaningfull data
        receivedChars[ndx] = character; //storing in the array
        ndx++;
        if (ndx >= numCharacters) { //if end of the array was reached 
          ndx = numCharacters - 1;
        }
      }
      else { //we got an endmarker
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;        //reset pointer to first position in the array
        newData = true; //turn flag true to be able to read other incoming data
      }
    }

    else if (character == startMarker) { //when we get a startmarker we start reading data
      recvInProgress = true;             //turn flag true
    }
  }
}

//Print data coming from Android app via bluetooth on the serial monitor
void showNewData() {
  if (newData == true) {
    Serial.print(F("This just in ... "));
    Serial.println(receivedChars);

  }
}

//Function that processes the data read from bluetooth
void processIncomingData() {
  //used for millis
  unsigned long startMillis = 0;
  unsigned long currentMillis = 0;
  int period = 4000;
  //constant characters to be sent or received via bluetooth
  //these caracters will be received from Android app
  const char CALIBRATE_LAS = '1';
  const char CALIBRATE_UAS = '2';
  const char CALIBRATE_BS = '3';
  const char DONE = '4';
  if (newData == true) { //if we got new data
    char data = receivedChars[0]; //Android app is sending only one character, so we are reading only the first value stored in the array
    if (data == CALIBRATE_LAS) { //A "1" means calibrate lower arm sensor
      calibrated = false;
      calibratedLowerArmSensor = false;
      calibratedUpperArmSensor = false;
      calibratedBicepsSensor = false;
      startMillis = millis();
      while(millis() < startMillis + period) {//do nothing, a delay that runs exactly period/1000 seconds
        } //This 4 seconds delay is used to give the user time to start doing the necessary movements for the calibration of the sensor 
      lowerArmThreshold = sensorCalibration(lowerArmPin);   //store average of numReadings readings into the lowerArmThreshold
      Serial.println(F("The new value of lower arm threshold is: ")); //print to serial monitor for debugging purposes
      Serial.println(lowerArmThreshold);
      if (isThresholdValid(lowerArmThreshold)) { //check if threshold is valid
        calibratedLowerArmSensor = true;        //turn flag true
      }  
    }
    if (data == CALIBRATE_UAS) { //A "2" means calibrate upper arm sensor
      startMillis = millis();
      while(millis() < startMillis + period) {//do nothing, a delay that runs exactly period/1000 seconds
        } 
      upperArmThreshold = sensorCalibration(upperArmPin);
      Serial.println(F("The new value of upper arm threshold is: "));
      Serial.println(upperArmThreshold);
      if (isThresholdValid(upperArmThreshold)) {
        calibratedUpperArmSensor = true;
      } 
    }
    if (data == CALIBRATE_BS) { //A "3" means calibrate biceps sensor
      startMillis = millis();
      while(millis() < startMillis + period) {//do nothing, a delay that runs exactly period/1000 seconds
        } 
      bicepsThreshold = sensorCalibration(bicepsPin);
      Serial.println(F("The new value of biceps threshold is: "));
      Serial.println(bicepsThreshold);
      if (isThresholdValid(bicepsThreshold)) {
        calibratedBicepsSensor = true;
      } 
    }
    if(data == DONE) {  //a "4" means the timer on the Android app has finished
      if(calibratedLowerArmSensor && calibratedUpperArmSensor && calibratedBicepsSensor) { // if all the sensors were calibrated
        BTserial.print(F("S"));   //send a "S" to the Android app; the app will then display a success message
        Serial.println("Sent a S to the app");
        writeThresholdsToEEPROM();//write thresholds to EEPROM
        calibrated = true;        //turn flag true to enable the movement code
      }
      else
      {
        BTserial.print(F("F")); //send a "F" to the Android app to display an error message and restart the calibration
        Serial.println("Sent a F to the app");
      }
    }
  }
  newData = false;
}

//Function that checks if a threshold is valid
bool isThresholdValid(int thresh) {
  if (thresh >= minThreshold && thresh <= maxThreshold) {
    return true;
  }
  else
  {
    return false;
  }
}
//Function used to write thresholds to nonvolatile memory (EEPROM)
void writeThresholdsToEEPROM() {
  EEPROM.put(addressLowerArmThreshold, lowerArmThreshold); 
  EEPROM.put(addressUpperArmThreshold, upperArmThreshold);
  EEPROM.put(addressBicepsThreshold, bicepsThreshold);
}
//Function used to read thresholds from EEPROM
void readThresholdsFromEEPROM() {
  EEPROM.get(addressLowerArmThreshold, lowerArmThreshold);
  EEPROM.get(addressUpperArmThreshold, upperArmThreshold);
  EEPROM.get(addressBicepsThreshold, bicepsThreshold);
}


//Function used to display battery life, mode, and gesture
void displayingBatteryLife(){
  
  oled.setFont(TimesNewRoman16_italic);
  oled.clear(0, 0, 0, 0);
  oled.println("           UArm  ");
  oled.setFont(  utf8font10x16);
  double voltage = readVcc();
  double min1 = 3255;
  double max1 = 4673;
  double percentage = (voltage- min1)/(max1-min1)*100;
  //displying battery percentage
  oled.print("Battery:  ");
  oled.setFont(fixed_bold10x15);
  if(percentage>90){  oled.println("100%");}
  
  else if(percentage>70 && percentage<=90){  oled.println("80%");}
  
  else if(percentage>50 && percentage<=70){  oled.println("60%");}
  
  else if(percentage>30 && percentage<=50){  oled.println("40%");}
  
  else if(percentage>10 && percentage<=30){  oled.println("20%");}
  else {  oled.println("10%");}
  
  //dysplaying mode
  oled.setFont(utf8font10x16);
  String myString2 = String(mode);
  oled.print("Mode: "); 
  oled.setFont(TimesNewRoman16_italic);
  oled.println(myString2 + " ");
  oled.setFont(utf8font10x16);
  //displaying gesture
  oled.print("Gesture: ");
  String myString3 = String(gesture);
  oled.println(myString3+ "       ");
  //turn RGB LED red if battery level below 20%
  if(percentage < 20)
  { analogWrite(9, 0);}
  else
  { analogWrite(9, 255);} 
}

//Function used to read internal voltage of Arduino
double readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  
  double finalResult=result;  //convert to double
  return finalResult;
}

void doAllGestures()
{
  close_hand();
  delay(1000);
  open_hand();
  delay(1000);
  pincing();
  delay(1000);
  open_hand();
  delay(1000);
  pointing();
  delay(1000);
  open_hand();
  delay(1000);
  peace_out();
  delay(1000);
  open_hand();
  delay(1000);
  thumb_up();
  delay(1000);
  open_hand();
  delay(1000);
}


void open_hand()
{
  gesture = "open hand";
  thumbServo.write(100);
  indexServo.write(180);
  middleServo.write(0);
  ringPinkyServo.write(0);
  thumbSideServo.write(70);
}

void close_hand()
{
  gesture = " close hand";
  thumbServo.write(10);
  indexServo.write(0);
  middleServo.write(180);
  ringPinkyServo.write(180);
  thumbSideServo.write(30);
  
}

void pointing()
{
  gesture = " pointing";
  thumbServo.write(10);
  indexServo.write(180);
  middleServo.write(180);
  ringPinkyServo.write(180);
  thumbSideServo.write(30);
}

void pincing()
{
  gesture = " pincing";
  thumbServo.write(30);
  indexServo.write(20);
  middleServo.write(100);
  ringPinkyServo.write(0);
  thumbSideServo.write(0);

}

void peace_out()
{
  gesture = " peace out";
  thumbServo.write(10);
  indexServo.write(180);
  middleServo.write(0);
  ringPinkyServo.write(180);
  thumbSideServo.write(20);
}

void alright()
{
  gesture = "alright";
  thumbServo.write(40);
  indexServo.write(20);
  middleServo.write(0);
  ringPinkyServo.write(0);
  thumbSideServo.write(0);
}

void  thumb_up()
{
  gesture = " thumb up";
  thumbServo.write(100);
  indexServo.write(0);
  middleServo.write(180);
  ringPinkyServo.write(180);
  thumbSideServo.write(70);
}
