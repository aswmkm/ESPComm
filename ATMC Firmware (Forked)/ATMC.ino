/*
  Sets and controls a temperature and acquires and logs temperatures 
  using MAX31856 thermocouple interface chips.

  Temperature Control:

  Using Timer 3 PWM to drive Pin 3 at 25 kHz with 10 bit dutycycle
  or using Timer 3 PWM to drive pins 2 and 3 if driving an H-bridge.

  Pin Assignments:

  PWM out on HEATER_PIN Pin 3

  When HBRIDGE is defined as 1, we define:
  PWM_PIN_A 3
  and
  PWM_PIN_B 2
  for use in controllig Peltier devices for heating and cooling.
  The Peltier device control is selected using the pre-compiler directive definition:
  #define HBRIDGE
  and deselected by commenting out the directive
  //#define HBRIDGE
  Similarly, the same code can be used for the first prototype 
  having only two thermocouples by uncommenting:
  //#define TWOTC
  and any functions you'd like to used during development can be 
  compiled in by uncommenting
  //#define DEVMODE
  If setting the speed of a computer fan, uncomment:
  //#define FANSPEED 
  and control the fan speed by setting the dutycycle of
  the Phase C PWM output.
  Temperature acquisition LED on Pin 11
  MAX31856_CS1 Pin 4 SPI CS for first MAX31856 thermocouple interface
  MAX31856_CS2 Pin 5 SPI CS for second MAX31856 thermocouple interface
  MAX31856_CS3 Pin 6 SPI CS for third MAX31856 thermocouple interface
  MAX31856_CS4 Pin 7 SPI CS for fourth MAX31856 thermocouple interface
  MAX31856_CS5 Pin 8 SPI CS for fifth MAX31856 thermocouple interface
  MAX31856_CS6 Pin 9 SPI CS for sixth MAX31856 thermocouple interface

  Copyright (c) 2017 ATMC.ino

  S&T and the University of Missouri Board of Curators
  license to you the right to use, modify, copy, and distribute this
  code subject to the MIT license:

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  Acknowledgements:

  I used the MAX31856 SPI code provided by Rx7man on this post:
  https://forum.arduino.cc/index.php?topic=390824.0

  All other functions used the standard Arduino libraries

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  The authors kindly request that any publications benefitting from the use
  of this software include the following citation:

  @Misc{ATMC2017 ,
  author =   {Stutts, Daniel S.},
  title = {{TempDAQandControl.ino: Arduino Application to 
  Acquire and Control Temperatures }},
  howpublished = {\url{https://github.com/dsstutts/ATMC.git}},
  year = {2017}}
	  
	Forked and Modified by Andrew Ward 8/24/2018**
	- Removed RTC and SD functionality, also removed related help texts and console inputs.
	- Streamlined some of the core functionality for scalability.

*/
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <TimerOne.h>
#include <TimerThree.h>
//#include "TimerFive.h"
#include <Wire.h>
#include <stdarg.h>
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MAX31856.h>
#include <string.h>
//
// The following precompiler directives are used to choose which code
// to compile throughout this file.  They allow you to select or deselect
// functionality as needed using the same code.

// Conditional precompiler directive for development mode:
//#define DEVMODE
// Conditional precompiler directive for 2 thermocouple case (1st development prototype)
#define TWOTC 
// Conditional precompiler directive for controlling H-Bridge output:
//#define HBRIDGE 
//#define THREEPHASE
#define FANSPEED
#define SINGLEPHASE
#define BS 8 //Backspace character
#define CR 13// Carrage return
#define LF 10// Line feed
#define DEL 127//Delete character
#define ECHO_//Comment out if you don't want to echo input chars to the serial monitor
#define HEATER_PIN 3 // PWM output pin
#ifdef HBRIDGE
#define PWM_PIN_A 3
#define PWM_PIN_B 2
#elifdef SINGLEPHASE
#define HEATER_PIN 3 // PWM output pin
#elifdef THREEPHASE
#define PWM_PIN_A 3
#define PWM_PIN_B 2
#define PWM_PIN_C 5
#endif
#ifdef FANSPEED
//#define PWM_PIN_B 2
#define PWM_PIN_C 5
#endif
///////// Chip Select Pins //////
#define SDCS 10// SPI CS for SD card
#define MAX31856_CS1 4// SPI CS for first MAX31856 thermocouple interface
#define MAX31856_CS2 5// SPI CS for second MAX31856 thermocouple interface
#define MAX31856_CS3 6// SPI CS for third MAX31856 thermocouple interface
#define MAX31856_CS4 7// SPI CS for fourth MAX31856 thermocouple interface
#define MAX31856_CS5 8// SPI CS for fifth MAX31856 thermocouple interface
#define MAX31856_CS6 9// SPI CS for sixth MAX31856 thermocouple interface
#ifdef TWOTC
#define MAX31856_CS1 42// SPI CS for first MAX31856 thermocouple interface
#define MAX31856_CS2 43// SPI CS for second MAX31856 thermocouple interface
#define NUM_TCs 2 // Number of thermocouples
#else
#define NUM_TCs 6 // Number of thermocouples
#endif
#define NUM31856REGs 10// Number of special function registers on the MAX31856
#define TYPE_K 0x03
#define TYPE_T 0x07
#define NOP __asm__ __volatile__ ("nop");// Inline no-operation ASM 
#define DATAREAD_LED 11//     for inserting a 62.5 ns delay used  for MAX31856
//                            SPI communication and for H-bridge deadtime. 
//The following executes 10 NOPs in a row for a 625 ns delay:
#define NOP10 __asm__ __volatile__ ("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"\
"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
////////////////////////////////

///////// Globals ////////////.
// These control the data acquisition rate from 200 ms to 6 s:
double updateIntervals[] = {100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, \
800.0, 900.0, 1000.0, 4000.0};
double *updateIntPtr = updateIntervals;
volatile char inbuff[200];
volatile char *inbuffPtr = inbuff;
volatile unsigned int InputBufferIndex;
unsigned long numDataPoints = 0;// Counts the number of datapoints acquired
boolean parseCommands = false;
char kpStr[10];
char kiStr[10];
char kdStr[10];
char dcStr[5];
char tempStr[10];
String *dcStrPtr;
//char dataString[100];
String dataString = "";
String *dataStringPtr;
String comma = ",";
String versStr;
String versDate = __DATE__;
String versTime = __TIME__;

//byte RegisterValues[] = {0x90,  0x03,   0xFC,   0x7F,   0xC0,   0x07,     \
//0xFF,     0x80,     0x00,     0x00 };//Type K Thermocouple
byte RegisterValues[] =   {0x90,  0x07,   0xFC,   0x7F,   0xC0,   0x07,     \
0xFF,     0x80,     0x00,     0x00 };// Type T Thermocouple
String RegisterNames[] =  {"CR0", "CR1", "MASK", "CJHF", "CHLF", \
"LTHFTH", "LTHFTL", "LTLFTH", "LTLFTL", "CJTO"};
byte RegisterAddresses[] = {0x00,  0x01,   0x02,   0x03,   0x04,   0x04,     \
0x06,     0x07,     0x08,     0x09 };

int CSs[] = {MAX31856_CS1, MAX31856_CS2, MAX31856_CS3, MAX31856_CS4, \
MAX31856_CS5, MAX31856_CS6};

boolean setGains = false;
boolean setKp = false;
boolean setKi = false;
boolean setKd = false;
boolean setPID = false;
boolean eeGainsSet = false;
boolean reportGains = false;
boolean readTemp = false;
boolean ledState = LOW;
boolean allOff = false;
boolean controlErr1 = false;
boolean powerOn = false;
boolean ls = false;// List SD card directory flag
unsigned int Interval = 9;//default Interval
boolean printAcqRate = false;
boolean setAcqRate = false;

double  acqInterval = 0;

double temps[NUM_TCs]; // Array for temperature data

double setTemp = 50.0;// Default target temperature
double ttime = 0;// Temperature time
double Ts = 200.0;// Default data acquisition rate.
// We're currently only using the current and previous errors:
#ifdef SINGLEPHASE
double Err[] = {0.0, 0.0, 0.0};//current error, last error, error before last
double derr = 0.0, ierr = 0.0;
#endif

#ifdef THREEPHASE
double Err1[] = {0.0, 0.0, 0.0};//current error, last error, error before last
double Err2[] = {0.0, 0.0, 0.0};//current error, last error, error before last
double Err3[] = {0.0, 0.0, 0.0};//current error, last error, error before last
double derr1 = 0.0, ierr1 = 0.0;
double derr2 = 0.0, ierr2 = 0.0;
double derr3 = 0.0, ierr3 = 0.0;
#endif

// Set some default gains:
double Kp = 200.0;
double DC = 0.0;
int iDC = 0;
int fanDC = 0;
boolean KpSet = false;

//double Ki = 2.0;
double Ki = 10.0;
boolean KiSet = false;

//double Kd = 120.0;
double Kd = 60.0;
boolean KdSet = false;
boolean pidUpdate = false;
unsigned int NumDataPoints = 0;
unsigned long writeSDtime = 0;
boolean PowerOn = false;
boolean controlOn = false;//PID control flag.
boolean openLoop = false;//Open loop flag
boolean setDC = false;//Manual open loop duty cycle setting flag.
boolean setFanSpeed = false;//Manual open loop duty cycle fan speed flag.
unsigned int i, j, numPts = 0;

volatile char InChar = '\0'; // serial input character
char myChar;
boolean Help = false;

const char controlError1[] PROGMEM = "Invalid control Interval\n";
const char HelpText[] PROGMEM = {"THC supports the following commands:\r\n \\
  A -- Control, acquire and store data\r\n \\
  C# -- Run in open loop with duty-cycle #.\r\n \\
  f# -- Set fan speed duty-cycle with duty-cycle # on phase C.\r\n \\
  h -- List of supported commands\r\n \\
  o -- Turn power off\r\n \\
  ss -- Print acquisition rate\r\n \\
  s# -- Set data acquisition interval where # is an integer option denoting:\r\n \\
    0 --> .1 sec (not implemented) \r\n \\
    1 --> .2 sec\r\n \\
    2 --> .3 sec\r\n \\
    3 --> .4 sec\r\n \\
    4 --> .5 sec\r\n \\
    5 --> .6 sec\r\n \\
    6 --> .7 sec\r\n \\
    7 --> .8 sec\r\n \\
    8 --> .9 sec\r\n \\
    9 --> 1.0 sec\r\n \\
    10 --> 4 sec\r\n \\
  T# -- Set the x = 0 boundary temperature to # degrees C\r\n \\
  th#m#s# -- set time where h,m, and s are hours, minutes, seconds and #, integers \r\n \\
  gg -- Report current PID gains.\r\n \\
     gp#i#d# -- Set PID gains, where # denotes floating point numbers.\r\n \\
     Note that while you may set one or two at a time, you must enter the PID \
     gains and times in order!\r\n \\
W#### -- Simultaneously set duty cycle and log data, where #### represents the \
duty cycle from 0 to 1023.\r\n"
};
//12 byte PID gains structure:
struct PID_Gains {
  double Kp;
  double Ki;
  double Kd;
};

struct PID_Gains pidGains;
///////// EEPROM Addresses //////
byte eeGainSetAddr = 0;// 1 byte
byte eepidGainsAddr = 1;// 12 bytes
byte eeAcqRateAddr = 13;// 2 bytes
// Total EEPROM bytes stored: 15 so far...
/////////////////////////////////
////////////////// End of Global Variables ////////


////////////// Function Definitions ////////////

///////// Interrupt Service Routines (ISRs) ///////////////
void serialEvent()
{
  while (Serial.available())
  {

    InChar = Serial.read();

    if ((InChar == DEL) && (InputBufferIndex != 0))
    {
      inbuff[--InputBufferIndex] = '\0';//Replace the erroneous byte with a NUL.
      Serial.print(BS);//Echo backspace character.
    }
    else
    {
      inbuff[InputBufferIndex] = InChar;
      InputBufferIndex++;
    }
    if (InChar == '\n' or InChar == 0xD)
    {
      parseCommands = true;
      InputBufferIndex = 0;
    }
  }
}
void ReadData(void)
{
  if (ledState) ledState = false;
  else
    ledState = true;
  digitalWrite(DATAREAD_LED, ledState);
  readTemp = true;
}
///////////// End of ISR definitions ////////

////////// MAX31856 Functions ////////////////
byte ReadSingleRegister(int Pin, byte Register) {
  digitalWrite(Pin, LOW);
  NOP;
  SPI.transfer(Register & 0x7F); //set bit 7 to 0 to ensure a read command
  NOP;
  byte data = SPI.transfer(0);
  digitalWrite(Pin, HIGH);
  return data;
}

unsigned long ReadMultipleRegisters(int Pin, byte StartRegister, int count) {
  //reads up to 4 sequential registers
  digitalWrite(Pin, LOW);
  unsigned  long data = 0;
  SPI.transfer(StartRegister & 0x7F); //force bit 7 to 0 to ensure a read command
  NOP;

  for (int i = 0; i < count; i++) {
    data = (data << 8) | SPI.transfer(0); //bitshift left 8 bits, 
  }//                                     then add the next register
  digitalWrite(Pin, HIGH);
  return data;
}

void WriteRegister(int Pin, byte Register, byte Value) {
  byte Address = Register | 0x80; //Set bit 7 high for a write command
  digitalWrite(Pin, LOW);
  NOP;
  SPI.transfer(Address);
  NOP;
  SPI.transfer(Value);
  digitalWrite(Pin, HIGH);
}

double ReadColdJunction(int Pin) {

  double temperature;

  long data, temperatureOffset;

  data = ReadMultipleRegisters(Pin, 0x08, 4);

  // Register 9 is the temperature offset
  temperatureOffset = (data & 0x00FF0000) >> 16;

  // Is this a negative number?
  if (temperatureOffset & 0x80)
    temperatureOffset |= 0xFFFFFF00;

  // Strip registers 8 and 9, taking care of negative numbers
  if (data & 0x8000)
    data |= 0xFFFF0000;
  else
    data &= 0x0000FFFF;

  // Remove the 2 LSB's - they aren't used
  data = data >> 2;

  // Add the temperature offset to the temperature
  temperature = data + temperatureOffset;

  // Convert to Celsius
  temperature *= 0.015625;


  // Return the temperature
  return (temperature);
}

double ReadTemperature(uint8_t Pin) {

  double temperature;
  long data;

  data = ReadMultipleRegisters(Pin, 0x0C, 4);

  // Strip the unused bits and the Fault Status Register
  data = data >> 13;

  // Negative temperatures have been automagically handled by the shift above :-)

  // Convert to Celsius
  temperature = (double) data * 0.0078125;


  // Return the temperature
  return temperature;
}

void initializeMAX31856Pins() {
  Serial.print("Initializing SPI chip-select pins");
  for (int i = 0; i < NUM_TCs; i++) {
    Serial.print(", ");
    Serial.print(CSs[i]);
    pinMode(CSs[i], OUTPUT);
    digitalWrite(CSs[i], HIGH);
  }
  Serial.println("  Done");
}

void InitializeChannel(int Pin) {
  Serial.print("Initializing channel on pin ");
  Serial.println(Pin);
  for (int i = 0; i < NUM31856REGs; i++) {
    WriteRegister(Pin, i, RegisterValues[i]);
  }
  //Serial.print("Finished in Initialize Channel\n");
}

void VerifyData(int CS) {
  int ErrorCount = 0;
  for (int i = 0; i < NUM31856REGs; i++) {
    byte RegVal = ReadSingleRegister(CS, i);
    if (RegVal != RegisterValues[i]) {
      Serial.print(RegisterNames[i]);
      Serial.print("\t has 0x");
      Serial.print(RegVal, HEX);
      Serial.print(" and should have 0x");
      Serial.println(RegisterValues[i], HEX);
      ErrorCount++;
    }
  }
  if (ErrorCount == 0) {
    Serial.println("No discrepancies found");
  }
}

////////// End of MAX31856 Functions /////////
void printGains(void) {
  Serial.print("Kp = ");
  Serial.print('\t');
  Serial.print(pidGains.Kp);
  Serial.print('\t');
  Serial.print("Ki = ");
  Serial.print('\t');
  Serial.print(pidGains.Ki);
  Serial.print('\t');
  Serial.print("Kd = ");
  Serial.print(pidGains.Kd);
  Serial.print('\n');
}

void stopAll()
{
#ifdef HBRIDGE
Timer3.pwm(PWM_PIN_A, 0);//Turn both phases off!
Timer3.pwm(PWM_PIN_B, 0);
#elifdef SINGLEPHASE
Timer3.pwm(HEATER_PIN, 0);//Set DC to zero!
#elifdef THREEPHASE
Timer3.pwm(PWM_PIN_A, 0);//Turn all 3 phases off!
Timer3.pwm(PWM_PIN_B, 0);
Timer3.pwm(PWM_PIN_C, 0);
#endif
    noInterrupts();
    SPI.setDataMode(SPI_MODE0);
    while (allOff); //Stop here now!
}

#ifdef HBRIDGE

void pidHBcontrol()
{
  //Err[0] = temp1 - setTemp;//If tracking a cold temp on a Peltier cooler.
  Err[0] = setTemp - temp1;
  derr = Err[0] - Err[1];
  ierr = ierr + Err[0];
  if (ierr >= 250.0) ierr = 250.0;// Saturate integral error for anti-windup.
  if (ierr <= -250.0) ierr = -250.0;
  Err[1] = Err[0];
  DC = pidGains.Kp * Err[0] + pidGains.Ki * ierr * Ts + pidGains.Kd * derr / Ts;
  iDC = (int)DC;// Cast to int and saturate:
  if (iDC >= 1023)iDC = 1023;
  if (iDC <= -1023)iDC = -1023;
  if (iDC < 0) { //Set DC for negative error:
    Timer3.pwm(PWM_PIN_B, 0);
    NOP10 // Make sure phase B is completely off!
    NOP10
    NOP10
    NOP10
    Timer3.pwm(PWM_PIN_A, iDC);// Reads low 10 bits, so sign doesn't matter.
  }
  else
  { // Positive error case:
    Timer3.pwm(PWM_PIN_A, 0);
    NOP10 // Make sure phase A is completely off!
    NOP10
    NOP10
    NOP10
    Timer3.pwm(PWM_PIN_B, iDC);
  }
}

#endif

#ifdef SINGLEPHASE
void PID_Control(void)
{
  //Err[0] = temp1 - setTemp;//If tracking a cold temp on a Peltier cooler.
  Err[0] = setTemp - temps[0];//temp1;
  derr = Err[0] - Err[1];
  ierr = ierr + Err[0];
  if (ierr >= 250.0) ierr = 250.0;// Saturate integral error for anti-windup.
  if (ierr <= -250.0) ierr = -250.0;
  Err[1] = Err[0];
  DC = pidGains.Kp * Err[0] + pidGains.Ki * ierr * Ts + pidGains.Kd * derr / Ts;
  iDC = (int)DC;// Cast to int
  if (iDC >= 1023)iDC = 1023;
  if (iDC <= 0)iDC = 0;
  Timer3.pwm(HEATER_PIN, iDC);//Set DC
}
#endif

#ifdef THREEPHASE
void PID_Control(void)
//
// Here we assume that the same PID gains can be used
// for all three temperature controls.  This assumption
// may not be valid.
//
{
  //Err[0] = temp1 - setTemp;//If tracking a cold temp on a Peltier cooler.
  Err1[0] = setTemp - temps[0];//temp1;
  Err2[0] = setTemp - temps[1];//temp2;
  Err3[0] = setTemp - temps[2];//temp3;
  derr1 = Err1[0] - Err1[1];
  derr2 = Err2[0] - Err2[1];
  derr3 = Err2[0] - Err2[1];
  ierr1 = ierr1 + Err1[0];
  ierr2 = ierr2 + Err2[0];
  ierr3 = ierr3 + Err3[0];
  if (ierr1 >= 250.0) ierr1 = 250.0;// Saturate integral error for anti-windup.
  if (ierr1 <= -250.0) ierr1 = -250.0;
  if (ierr2 >= 250.0) ierr2 = 250.0;// Saturate integral error for anti-windup.
  if (ierr2 <= -250.0) ierr2 = -250.0;
  if (ierr3 >= 250.0) ierr3 = 250.0;// Saturate integral error for anti-windup.
  if (ierr3 <= -250.0) ierr3 = -250.0;
  Err1[1] = Err1[0];
  Err2[1] = Err2[0];
  Err3[1] = Err3[0];
  DC1 = pidGains.Kp * Err1[0] + pidGains.Ki * ierr1 * Ts + pidGains.Kd * derr1 / Ts;
  DC2 = pidGains.Kp * Err2[0] + pidGains.Ki * ierr2 * Ts + pidGains.Kd * derr2 / Ts;
  DC3 = pidGains.Kp * Err3[0] + pidGains.Ki * ierr3 * Ts + pidGains.Kd * derr3 / Ts;
  iDC1 = (int)DC1;// Cast to int
  if (iDC1 >= 1023)iDC1 = 1023;
  if (iDC1 <= 0)iDC1 = 0;
  Timer3.pwm(PWM_PIN_A, iDC1);//Set DC
  iDC1 = (int)DC2;// Cast to int
  if (iDC2 >= 1023)iDC2 = 1023;
  if (iDC2 <= 0)iDC2 = 0;
  Timer3.pwm(PWM_PIN_B, iDC2);//Set DC
  iDC1 = (int)DC3;// Cast to int
  if (iDC3 >= 1023)iDC3 = 1023;
  if (iDC3 <= 0)iDC3 = 0;
  Timer3.pwm(PWM_PIN_C, iDC3);//Set DC
}
#endif


//////////////Parser//////////
void parseSerialInput(void) {
  boolean NEG = 0;
  boolean kpSet = false;
  boolean kiSet = false;
  boolean kdSet = false;
  char dataStr[2] = {'\0', '\0'};
  unsigned int data = 0;
  double setTemp;
  byte i = 0;
  byte k = 0;
  byte l = 0;
  byte j = 0;
  byte o = 0;
  byte q = 0;
  byte p = 0;
  //
  // Read the first command character and parse accordingly:
  //

  if (*inbuffPtr == 'h') { // Print help string.
    Help = true;
    return;
  }

  if (*inbuffPtr == 'o') { // Turn off power, but still log data.
    powerOn = false;
    return;
  }

  if (*inbuffPtr == 'A') { //Start controlling default temperature and saving data.
    powerOn = true;
    pidUpdate = true;
    controlOn = true;
    readTemp = true;
    numDataPoints = 0;
    ttime = 0.0;
    if(Interval>9) controlErr1 = true;
    return;
  }

  if (*inbuffPtr == 'a') { //Stop everything and save data.
    allOff = true;
    return;
  }

  // Set target temperature and log data:
  if (*inbuffPtr == 'T') {
    powerOn = true;
    pidUpdate = true;
    controlOn = true;
    readTemp = true;
    numDataPoints = 0;
    ttime = 0.0;
    while (*inbuffPtr != '\0') {
      if (((*inbuffPtr >= '0') && (*inbuffPtr <= '9')) || (*inbuffPtr == '.')) { // Make 
        tempStr[i] = *inbuffPtr;//                  sure they're numeric!
        i++;
      }//End if numeric
      *inbuffPtr++;
    }// End duty cycle set while
    setTemp = atof(tempStr);//convert to float
    for (j = 0; j < sizeof(tempStr); j++)tempStr[j] = '\0'; //Flush buffer
    return;
  }

  //Open loop duty cycle setting:
  if (*inbuffPtr == 'C') { //Control Setting case
    readTemp = true;// Always read temp when power on to shut down if T >= 90 C.
    setDC = true;
    openLoop = true;
    numDataPoints = 0;
    ttime = 0.0;
    *inbuffPtr++;
    openLoop = true;
    setDC = true;
    while (*inbuffPtr != '\0') {
      if (((*inbuffPtr >= '0') && (*inbuffPtr <= '9')) || (*inbuffPtr == '.')) { 
        dcStr[i] = *inbuffPtr;// Make sure they're numeric!        
        i++;
      }//End if numeric
      *inbuffPtr++;
    }// End duty cycle set while
    return;
  } // End of if C

  //Open loop duty cycle setting with data acquisition:
  if (*inbuffPtr == 'W') {
    setDC = true;
    openLoop = true;
    readTemp = true;
    numDataPoints = 0;
    ttime = 0.0;
    *inbuffPtr++;
    while (*inbuffPtr != '\0') {
      if (((*inbuffPtr >= '0') && (*inbuffPtr <= '9')) || (*inbuffPtr == '.')) {  
        dcStr[i] = *inbuffPtr;// Make sure they're numeric!  
        i++;
      }//End if numeric
      *inbuffPtr++;
    }// End duty cycle set while
    return;
  } // End of if W

  if (*inbuffPtr == 'c') { 
    allOff = true;
    return;
  }

  if(*inbuffPtr == 'l')
    {
      ls = true;
      return;
    }
  if (*inbuffPtr == 's') {
    *inbuffPtr++;//increment pointer to second character
    if (*inbuffPtr++ == 's') { //Set print settings flag and return.
      printAcqRate = true;
      return;
    }
    else
    {
      *inbuffPtr--;// Decrement pointer
      setAcqRate = true;
      while ((*inbuffPtr != '\0')) {
        if ((*inbuffPtr >= '0') && (*inbuffPtr <= '9')) {
          dataStr[i] = *inbuffPtr;//Make sure they're numeric!  
        }
        *inbuffPtr++;//Increment buffer pointer.
        i++;
      }
      Interval = (unsigned int)atoi(dataStr);//
      if(controlOn&&(Interval>10))controlErr1 = true;//Trap invalid control update interval error
    }//End else
  }// End if s
if (*inbuffPtr == 'f')
{
   while ((*inbuffPtr != '\0')) 
   {
        if ((*inbuffPtr >= '0') && (*inbuffPtr <= '9')) {
          dataStr[i] = *inbuffPtr;//Make sure they're numeric!  
        }
        *inbuffPtr++;//Increment buffer pointer.
        i++;
   }
      fanDC = (unsigned int)atoi(dataStr);//
      setFanSpeed = true;
   return;
   
}

  // PID control gain setting:
  if (*inbuffPtr == 'g') { //Control Setting case
    *inbuffPtr++;

    if (*inbuffPtr++ == 'g') { //Set report_Gains and return
      reportGains = true;
      return;
    }
    else
    {
      *inbuffPtr--;// Decrement pointer
      setGains = true;
      while (*inbuffPtr != '\0') {

        switch (*inbuffPtr) {
          case 'p':
            setKp = true;
            kpSet = true;
            break;
          case 'i':
            setKi = true;
            kiSet = true;
            break;
          case 'd':
            setKd = true;
            kdSet = true;
            break;
          default:
            if (((*inbuffPtr >= '0') && (*inbuffPtr <= '9')) || (*inbuffPtr == '.')) { 
              if (kdSet) {// Make sure they're numeric!
                kdStr[k] = *inbuffPtr;
                kpSet = false;
                kiSet = false;
                k++;
              }
              if (kiSet) {
                kiStr[j] = *inbuffPtr;
                kpSet = false;
                kdSet = false;
                j++;
              }
              if (kpSet) {

                kpStr[i] = *inbuffPtr;
                kiSet = false;
                kdSet = false;
                i++;
              }
            }//End if numeric
            break;
        }//End PID switch
        *inbuffPtr++;
      }// End PID while
    } // End of if else
  }// End of if g

}// End of parseSerialInput

////////////End of Parser////

void setup()
{
  versStr.reserve(30);// Reserve space for version string
  EEPROM.get(eeAcqRateAddr, Interval);// Below we handle invalid cases:
  if ((Interval <= 0) || (Interval > 9) || isnan(Interval)) Interval = 1; //Set default update index.
  // Reading an empty (not yet assigned a value) EEPROM register returns NAN.

  Ts = updateIntervals[Interval] / 1000.0; // Set sampling rate.
  // put your setup code here, to run once:
  pinMode(DATAREAD_LED, OUTPUT);//Set the data-read activity LED pin to output.
  digitalWrite(DATAREAD_LED, false);

  InputBufferIndex = 0;
  //Timer1.initialize(200000);// 200 ms
  Timer1.initialize(((long)updateIntervals[Interval]) * 1000); // Set default update interval.
  Timer1.attachInterrupt(ReadData);
  Timer3.initialize(40); // 40 us => 25 kHz PWM frequency
  //Timer5.initialize(500000); // May use later...
  //Timer5.attachInterrupt(callback function of some use...);
  while (!Serial); // for Leonardo/Micro/Zero
  Serial.begin(250000);
  //Serial.begin(500000);
  //Serial.begin(1000000);// Works!
  //Serial.begin(2000000);// Works too!  Holy Cow!

  SPI.begin();
  //SPI.setClockDivider(SPI_CLOCK_DIV2);
  //SPI.setDataMode(SPI_MODE0);// SPI Mode 0 for SD card.

  Serial.println("READY!  \n");
  
  //SPI.end();
  byte SPIERROR = SPSR;//Read SPI status reg to clear errors; doesn't work.
  delay(100);
  SPI.setClockDivider(SPI_CLOCK_DIV2);//Reset to 7.8 MHz and Mode 3 for MAX31856
  //SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.setDataMode(SPI_MODE3);
  initializeMAX31856Pins();
  for (int i = 0; i < NUM_TCs; i++) { //usually done for each channel..
    InitializeChannel(CSs[i]);
    delay(10);
    VerifyData(CSs[i]);
  }
  //noInterrupts();//The EEPROM driver apparently code uses interrupts,
  EEPROM.get(eeGainSetAddr, eeGainsSet);// so you can't disable all of them.
  if (eeGainsSet = 1) {
    EEPROM.get(eepidGainsAddr, pidGains);
  }
  Serial.print("eeGainsSet = :");
  Serial.println(eeGainsSet);
  
  printGains();
  
  //EEPROM.get( eeAcqRateAddr, Interval);
  Serial.print("Interval = ");
  Serial.println(Interval);
  //Ts = updateIntervals[Interval] / 1000.0; // Set default sampling rate.
  Timer1.initialize(200000);// 200 ms
  Timer1.initialize(((long)updateIntervals[Interval]) * 1000); // Set  
  Timer1.attachInterrupt(ReadData);                           // update interval.

#ifdef HBRIDGE
  Timer3.pwm(PWM_PIN_A, 0);
  Timer3.pwm(PWM_PIN_B, 0);
#else
  Timer3.pwm(HEATER_PIN, 0);//Start with zero duty cycle
#endif
  delay(100);
  versStr = "ATMC Built on "+versDate+" at "+versTime;
  Serial.println(versStr);
}
///////////// End setup ////////////
///////////// Main Loop ////////////
void loop()
{
  // All other function calls should occur here...
  if (parseCommands)
  {
    parseSerialInput();
    for (i = 0; i < sizeof(inbuff); i++) // Flush buffer.
    {
      inbuff[i] = '\0';
    }
    inbuffPtr = inbuff;// Point the input string pointer back to the beginning.
    parseCommands = false;

     if(controlErr1)// Trap invalid control update rate error
   {
    int errLen = strlen_P(controlError1);
    for (j = 0;j < errLen;j++)
    {
    myChar = pgm_read_byte_near(controlError1 + j);
    Serial.print(myChar);
    }
    stopAll();
   }
  }

  if (setAcqRate) {
   Serial.print("Interval = \n");
   Serial.print(Interval);
   Serial.print("\n");
    Timer1.initialize(((long)updateIntervals[Interval]) * 1000);
    Ts = updateIntervals[Interval] / 1000.0; // Set default sampling rate.
    EEPROM.put(eeAcqRateAddr, Interval);// Save setting.
    setAcqRate = false;
  }

  if (Help)
  {
    int HelpLength = strlen_P(HelpText);
    for (j = 0; j < HelpLength; j++)
    {
      myChar = pgm_read_byte_near(HelpText + j);
      Serial.print(myChar);
    }
    Help = false;
  }

  if (printAcqRate) {
    acqInterval = updateIntervals[Interval];
    Serial.print("Acquisition Interval = ");
    Serial.print(acqInterval);
    Serial.print(" mS");
    Serial.print("\n");
    printAcqRate = false;
  }
  if (reportGains) {
    printGains();
    reportGains = false;
  }
  if (setGains) {

    if (setKp) {
      pidGains.Kp = atof(kpStr);
      for (j = 0; j < sizeof(kpStr); j++)kpStr[j] = '\0'; //Flush the gain buffers
      setKp = false;
    }
    if (setKi) {
      pidGains.Ki = atof(kiStr);
      for (j = 0; j < sizeof(kiStr); j++)kiStr[j] = '\0';
      setKi = false;
    }
    if (setKd) {
      pidGains.Kd = atof(kdStr);
      for (j = 0; j < sizeof(kdStr); j++)kdStr[j] = '\0';
      setKd = false;
    }
    noInterrupts();//Don't let this transaction be interrupted!
    EEPROM.put(eepidGainsAddr, pidGains);
    EEPROM.put(eeGainSetAddr, 1);//Store the fact that the gains are stored in EPROM.
    interrupts();
    setGains = false;
  }

  if (setDC)
  {
    iDC = atoi(dcStr);
#ifdef HBRIDGE
    // Note that this logic is the opposite of that used
    // in the pidHBcontrol() function.
    if (iDC < 0)
    { // Cooling case
      Timer3.pwm(PWM_PIN_A, 0);// The number of NOPs will have 
                               // to be experimentally verified!
      NOP10 // Make sure phase A is completely off!
      NOP10
      NOP10
      NOP10
      Timer3.pwm(PWM_PIN_B, iDC);// Reads low 10 bits, so sign doesn't matter.
    }
    else
    { // Heating case:
      Timer3.pwm(PWM_PIN_B, 0);
      NOP10 // Make sure phase B is completely off!
      NOP10
      NOP10
      NOP10
      Timer3.pwm(PWM_PIN_A, iDC);
    }
#elifdef SINGLEPHASE
    if (iDC < 0)iDC = 0;// Saturate duty cycles below zero or above 1023.
    if (iDC > 1023) iDC = 1023;
    Timer3.pwm(HEATER_PIN, iDC);
#endif
    setDC = false;
    for (i = 0; i < sizeof(dcStr); i++) // Flush dcStr buffer.
    {
      dcStr[i] = '\0';
    }
  }

  if (readTemp)
  {
	for ( uint8_t x = 0; x < NUM_TCs; x++ )
		temps[x] = ReadTemperature(CSs[x]);

if (temps[0] >= 120) 
{ // Shut down if control temp > 120 degrees C.
#ifdef HBRIDGE
Timer3.pwm(PWM_PIN_A, 0);//Turn both phases off!
Timer3.pwm(PWM_PIN_B, 0);
#elifdef SINGLEPHASE
Timer3.pwm(HEATER_PIN, 0);//Set DC to zero!
#elifdef THREEPHASE
Timer3.pwm(HEATER_PIN, 0);//Set DC to zero!
Timer3.pwm(PWM_PIN_A, 0);//Turn all 3 phases off!
Timer3.pwm(PWM_PIN_B, 0);
Timer3.pwm(PWM_PIN_C, 0);
#endif
  noInterrupts();
  Serial.println("Exceeded Max Temps!");
  while (1); //Stop here now!
}

	Serial.print(ttime);
	Serial.print('\t');

	for ( uint8_t x = 0; x < NUM_TCs; x++)
	{
		Serial.print(temps[x]);
		Serial.print('\t');
	}

    Serial.print(iDC);//Current duty cycle
    Serial.print('\n');
    //     }
    numDataPoints++;
    readTemp = false;
    pidUpdate = true;

    SPI.setDataMode(SPI_MODE3);
    ttime = ttime + Ts;
  }

  //if(powerOn){//Safety wrapper around PWM output
  if (pidUpdate && controlOn)
  {
#ifdef HBRIDGE
    pidHBcontrol();
#else
    PID_Control();
#endif
    pidUpdate = false;
  }
  // }
  //else
  //Timer3.pwm(HEATER_PIN, 0);//Set DC to zero!
// Set the fan speed:
if(setFanSpeed)
{
  Timer3.pwm(PWM_PIN_C, fanDC);
  setFanSpeed = false;
}

 if (allOff)
 {
#ifdef HBRIDGE
Timer3.pwm(PWM_PIN_A, 0);//Turn both phases off!
Timer3.pwm(PWM_PIN_B, 0);
#elifdef SINGLEPHASE
Timer3.pwm(HEATER_PIN, 0);//Set DC to zero!
#elifdef THREEPHASE
Timer3.pwm(PWM_PIN_A, 0);//Turn all 3 phases off!
Timer3.pwm(PWM_PIN_B, 0);
Timer3.pwm(PWM_PIN_C, 0);
#endif
    noInterrupts();
    SPI.setDataMode(SPI_MODE0);
    while (allOff); //Stop here now!
  }
}
////////////  End Main Loop /////////////////



