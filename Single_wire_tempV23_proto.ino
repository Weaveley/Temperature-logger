//  Datalogger for temperature using up to DS18B20 sensors and a thermocouple
//  Has an LCD display and push-button controlled logging on/off

//  Uses the DS18B20 temperature sensors
//  http://www.hobbytronics.co.uk/ds18b20-arduino
//  Red connects to 3-5V, Blue/Black connects to ground and Yellow/White is data if using encased version
//  Needs 4.7k pull up on signal line even if operating in three wire mode
//  The thermocouple (TC) is interfaced via a MAX6675 module
//  The LCD is a 1602A LCD display
//  See http://www.dreamdealer.nl/tutorials/connecting_a_1602a_lcd_display_and_a_light_sensor_to_arduino_uno.html though the pins used are different
//  Uses a datalogger shield similar to Adafruit. Chipselect is pin 10.
//  The datalogger shield uses 6 pins.
//  Analog 4 and 5 are the i2c hardware pins. The SD card uses digital pins 13, 12,11, and 10.
//  Connections are:
//  Connect: GND > pin 1 on LCD (VSS)
//  Connect: +5v > pin 2 on LCD (VDD)
//  Connect: +5v > pin 15 on LCD (A)
//  Connect: GND > pin 16 on LCD (K)
//  Connect: first pin of the 10k potentiometer > GND 
//  Connect: middle pin of the potmeter > pin 3 of the LCD display (VO)
//  Connect: end pin of the 10k potentiommeter > +5V
//  Connect: pin 4 of the LCD display (RS) > pin 16 of the Arduino
//  Connect: pin 5 of the LCD display (RW) > GND 
//  Connect: pin 6 of the LCD display (E) > pin 17 of the Arduino 
//  Connect: pin 11 of the LCD display (D4) > pin 6 of the Arduino
//  Connect: pin 12 of the LCD display (D5) > pin 7 of the Arduino
//  Connect: pin 13 of the LCD display (D6) > pin 8 of the Arduino
//  Connect: pin 14 of the LCD display (D7) > pin 9 of the Arduino
//  Pin 2 Arduino to DS18B20 temperature sensor bus
//  Pin 3 Arduino to SO on TC board
//  Pin 4 Arduino to CS on TC board
//  Pin 5 Arduino to SCK on TC board
//  Pin 14 (A5) to button and pull-down
//  Pin 15 (A4) to button and pull-down


#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <max6675.h>
#include <Wire.h>
#include <RTClib.h>

#include <SPI.h>
#include <SD.h>

// RTC stuff
// Date and time functions using a DS1307 RTC connected via I2C and Wire lib

RTC_DS1307 rtc;
char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Data wire DS18B20 temperature sensors is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Set a lower temperature resolution. This seems to set a precision to the nearest 0.5 degrees C (a 9 bit resolution).
// The default without this was a much higher resolution but the sensors.requestTemperatures() command took 0.8-0.9s
// to execute with 4 sensors.
// The basic resolution is 0.5 degrees C on the datasheet (https://datasheets.maximintegrated.com/en/ds/DS18S20.pdf)
// but can be increased.
#define TEMPERATURE_PRECISION 9
 
// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Set up LCD display
LiquidCrystal lcd(16, 17, 6, 7, 8, 9);

// Declare temperature variables
float Temp1 = 1.0;
float Temp2 = 1.0;
float Tcouple = 1.0;

// Number of one wire temperature devices found
int numberOfDevices; 

// Set up thermocouple via MAX6675 module
// Resolution seems to be 0.25 degrees C
int thermoDO = 3;
int thermoCS = 4;
int thermoCLK = 5;

// set up a variable to hold a millisecond reading for the looping display
unsigned long millisOld = millis();

// Set up a variable to trigger logging
unsigned long millisLogCount = millis();

// set up a variable that is the logging interval in milliseconds
unsigned long millisLog = 5000;

// Set up bebounce interval counter
unsigned long millisBounce = 0;

// Set up debounce interval in ms. Has to be longer than loop time or interval set by loop
int bounceInterval = 600;


// Loop-through delay for temp display, dwell time in ms
int loopDelay = 5000;

// constants won't change. They're used here to set pin numbers:
const int buttonPin1 = 14;    // the number of the pushbutton pin 1
const int buttonPin2 = 15;    // the number of the pushbutton pin 2
const int chipSelect = 10;    // the number of the chipselect pin

// initiate the thermocouple interface
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// Set up variables for button press detect
boolean wasHigh1 = false;
boolean wasHigh2 = false;
int reading1 = 0;
int reading2 = 0;

// set up Boolean variables to indicate button press to main program
boolean buttonPress1 = false;
boolean buttonPress2 = false;

// integer counter to set top display
int topDisplay = 1;

//Set up toggle for storing
boolean storeToggle = false;
boolean fileInUse = false;

//Set up variables to generate logging filename
  int t_month = 1;
  int t_day = 1;
  int t_hour = 0;
  int t_minute = 1;
  int t_second = 0;

// set up variables using the SD utility library functions:
// This bit is needed to ID the card type but is not otherwise needed
Sd2Card card;
SdVolume volume;
SdFile root;

  File dataFile;

void setup(void)
{
  
  //start serial port in case you need to monitor what is happening
  Serial.begin(115200);
  delay(500);


  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperature log");
  lcd.setCursor(0, 1);
  lcd.print("Software V23.0");
  delay (5000);
  lcd.clear();
  
  // Start up the library
  sensors.begin();

  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();

  
  if (! rtc.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("No RTC found    ");
    while (1);
  }
  if (! rtc.isrunning()) {
    lcd.setCursor(0, 0);
    lcd.print("RTC not running ");
    while (1);
  }

    // following line sets the RTC to the date & time this sketch was compiled (from PC clock). Uncomment to reset the clock
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // Check SD card
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    lcd.setCursor(0, 0);
    lcd.print("No card found   ");
    delay (5000);
    lcd.clear();
    }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Card initalised ");
    
 
//This bit IDs the card type but is not needed. However it doesn't compile if removed?
    lcd.setCursor(0, 1);
    lcd.print("Card type: ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      lcd.print("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      lcd.print("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      lcd.print("SDHC");
      break;
    default:
      lcd.print("NK");

   
    delay (5000);
    lcd.clear();
    }}
    
   lcd.setCursor(0, 0);
   lcd.print(numberOfDevices, DEC);
   lcd.print(" onewire probes");
   delay (5000);

   // report parasite power requirements
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("Parasitic power");
   lcd.setCursor(0, 1); 
  if (sensors.isParasitePowerMode()) lcd.print("is on");
  else lcd.print("is off");
  delay (5000);
  lcd.clear();

  // Set a bunch of pin functions
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);

  // make it start on Temp 1
  millisOld = millis();

  //Make date/time the default upper display
  topDisplay = (numberOfDevices + 2);

  // Debug


}
 
void loop(void)
{
  
/* Debug
Serial.println("Start of loop");
Serial.println(buttonPress1);
Serial.println(buttonPress2);
Serial.println(millis());
*/

// Send the command to get the time
 
  DateTime now = rtc.now();

  
  // stuff to generate filename
  t_month = now.month();
  t_day = now.day();
  t_hour = now.hour();
  t_minute = now.minute();

//Debug

  t_second = now.second();
  
  String temp;
  temp.reserve(12);
  if (t_month < 10) {
    temp = String("0");
    temp += String(t_month);
  }
  else {
    temp = String(t_month);
  }
  if (t_day < 10) {
    temp += String("0");
    temp += String(t_day);
  }
  else {
    temp += String(t_day);
  }
  if (t_hour < 10) {
    temp += String("0");
    temp += String(t_hour);
  }
  else {
    temp += String(t_hour);
  }
  if (t_minute < 10) {
    temp += String("0");
    temp += String(t_minute);
  }
  else {
    temp += String(t_minute);
  }

  String timeStamp;
  timeStamp.reserve(10);
  timeStamp = temp;
  if (t_second < 10) {
    timeStamp += String("0");
    timeStamp += String(t_second);
  }
  else {
    timeStamp += String(t_second);
  }

// Debug
  //Serial.println(timeStamp);
  
  temp += String(".txt");
  char filename[temp.length()+1];
  temp.toCharArray(filename, sizeof(filename));
  //Serial.println(temp);
  //Serial.println(" ");

  //collect data into string
  String dataString = "";
  for(int j=0;j<numberOfDevices; j++)
  {
  Temp1 = (sensors.getTempCByIndex(j));
  dataString += String(Temp1);
  dataString += ",";
  }
  Tcouple = (thermocouple.readCelsius());
  dataString += String(Tcouple);
  
  // Serial.println(dataString);

/*Debug
//Serial.println("End time call");
//Serial.println(millis());
*/
  
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures();

  // get the TC temp
  Tcouple = (thermocouple.readCelsius());

 /*Debug
  Serial.println("End global onewire call");
  Serial.println(millis());
  */

// Increment top display counter after button press
  if (buttonPress1==true) {
  topDisplay = topDisplay + 1;
  lcd.clear();
  }
  
 // Roll over if counter is greater than (probes+TC+date)
  if (topDisplay > (2+numberOfDevices)) {
  topDisplay = 1;  
  }
  
  /*Debug
  lcd.setCursor(15, 1);
  lcd.print(topDisplay);
   */

  // If the counter is in the probe range
   if (topDisplay <= numberOfDevices) {
   Temp2 = (sensors.getTempCByIndex(topDisplay-1));
   lcd.setCursor(0,0);
   lcd.print("Temp ");
   lcd.print((topDisplay),DEC);
   lcd.print(" ");
   lcd.print(Temp2, 1);//change here
   lcd.print((char)223);
   lcd.print("C ");
  }

  // Always display the thermocouple next
    if (topDisplay == (numberOfDevices + 1)) {
    lcd.setCursor(0,0);
    lcd.print("Temp TC ");
    lcd.print(Tcouple, 1);  // Change here
    lcd.print((char)223);
    lcd.print("C ");  
  }

  // Always display date/time next
  if (topDisplay == (numberOfDevices + 2)) {
  lcd.setCursor(0, 0);
  lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
  lcd.print(' ');
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  lcd.print("   ");
  }

  /* Short bit of code to show button2 working
  if (buttonPress2==true) {
  storeToggle = !storeToggle;
  }
  
  if (storeToggle==true) {
  lcd.setCursor(15, 0);
  lcd.print("S");
  }
  else
  {
  lcd.setCursor(15, 0);
  lcd.print(" ");
  }
  */
  
  /*Debug
  Serial.println("After time");
  Serial.println(millis());
  */
  
   // Reset the button pressed indicators
   buttonPress1=false;
   buttonPress2=false;

  // This code loops through the temperature sensors on the lower line of the display
  for(int i=0;i<numberOfDevices; i++)
  {
  // Loop through 4 IC sensors on the lower line
  if ((millis()- millisOld) > (i * loopDelay) && ((millis()- millisOld) < ((i + 1) *loopDelay))) {
  Temp1 = (sensors.getTempCByIndex(i));
  lcd.setCursor(0,1);
  lcd.print("Temp ");
  lcd.print((i+1),DEC);
  lcd.print(" ");
  lcd.print(Temp1, 1);//change here
  lcd.print((char)223);
  lcd.print("C  ");
  }
  }
  
  // Puts thermocouple reading on lower line
  if ((millis()- millisOld) > (numberOfDevices * loopDelay) && (millis()- millisOld) < ((numberOfDevices+1) * loopDelay)) {
    //Tcouple = (thermocouple.readCelsius()); Change here
    lcd.setCursor(0,1);
    lcd.print("Temp TC ");
    lcd.print(Tcouple, 1);//Change here
    lcd.print((char)223);
    lcd.print("C  ");
  }
  // Reset the counter for the lower display
  if ((millis()- millisOld) >= ((numberOfDevices+1) * loopDelay)) {
    millisOld = millis();
  }

// The switch has to be down for twice the whole debounce interval to activate

if ((millis()- millisBounce) > bounceInterval) {

// Read the first switch

  // read the state of the switch into a variable:
     reading1 = digitalRead(buttonPin1);

  // If it was high last round and is high this time
  
  if (reading1 == 1  && wasHigh1 == true) {
    buttonPress1 = true;
    // Reset the marker
    wasHigh1 = false;
    millisBounce=millis();
  }

  // If it was low last time toggle the marker

  if (reading1 == 1 && wasHigh1 == false) {
// Set the marker
    wasHigh1 = true;
     millisBounce=millis();
  }

if (reading1 == 0) {
    // Set the marker
    wasHigh1 = false;
     millisBounce=millis();
  }

  // Read the second switch
  
  // read the state of the switch into a local variable:
     reading2 = digitalRead(buttonPin2);

  // If it was high last round and is high this time
  
  if (reading2 == 1  && wasHigh2 == true) {    
  buttonPress2 = true;
 // Reset the marker
    wasHigh2 = false;
     millisBounce=millis();
  }

  // If it was low last time toggle the marker

  if (reading2 == 1 && wasHigh2 == false) {
   // Set the marker
    wasHigh2 = true;
     millisBounce=millis();
  }

  if (reading2 == 0) {
    // Set the marker
    wasHigh2 = false;
    millisBounce=millis();
  }}
  
  // Short bit of code to show button2 working
  if (buttonPress2==true) {
  storeToggle = !storeToggle;
  }
  
  if (storeToggle==true && fileInUse == false) {
  lcd.setCursor(15, 0);
  lcd.print("L");
  String nameLock = temp;
  //nameLock = filename;
  dataFile = SD.open(nameLock, FILE_WRITE);
  fileInUse = true;
  dataFile.print(timeStamp);
  dataFile.print(" ");
  dataFile.println(dataString);
  //Debug
  // dataFile.close();
    // print to the serial port too:
  Serial.println("Logging #1");
  Serial.println(nameLock);
  Serial.print(timeStamp);
  Serial.print(" ");
  Serial.println(dataString);
  Serial.println(storeToggle);
  Serial.println(fileInUse);
  Serial.println();
  millisLogCount = millis();
  }

  
  if (storeToggle==true && fileInUse == true) {
  lcd.setCursor(15, 0);
  lcd.print("L");
  if ((millis()-millisLogCount) >= millisLog) {
  dataFile.print(timeStamp);
  dataFile.print(" ");
  dataFile.println(dataString);
  millisLogCount=millis();
  // Debug
  // dataFile.close();
    // print to the serial port too:
  Serial.println("Logging subsequent");
//  Serial.println(nameLock);
  Serial.print(timeStamp);
  Serial.print(" ");
  Serial.println(dataString);
  Serial.println(storeToggle);
  Serial.println(fileInUse);
  Serial.println();}
  }
  
  if (storeToggle==false && fileInUse == true) {
  dataFile.close();
  fileInUse = false;
  lcd.setCursor(15, 0);
  lcd.print(" ");
  }
  if (storeToggle==false && fileInUse == false)
  {
  lcd.setCursor(15, 0);
  lcd.print(" ");
  }
    
  
}
  






