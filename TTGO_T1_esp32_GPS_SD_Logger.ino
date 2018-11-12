#include <TinyGPS++.h>
#include "BluetoothSerial.h"
#include <sstream>
#include <iomanip>
#include <mySD.h>
#include <SPI.h>


//TTGO T1 SD card pins: 13,15,2,14

//GPS pins
#define RXD2 16
#define TXD2 17

//deep sleep variables
#define uS_TO_S_FACTOR 1000000   /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60         /* Time asleep (in seconds) */
#define TIME_AWAKE 10            /* Time awake (in seconds) */
RTC_DATA_ATTR int bootCount = -1; //persistent RTC memory counter
const uint64_t wait = millis(); //start time, used with TIME_AWAKE

//change 'serial' to true for BT and USB serial output of parsed data 
//(much more power and you need to re-"pair" after sleeping)
//also takes up ~50% of flash and ~15% of RAM on its own
#define serial false   
#if serial
  BluetoothSerial SerialBT;
#endif

//initialize serial and gps objects
HardwareSerial SerialGPS(2);
TinyGPSPlus gps;

//serial command for GPS copied from u-center packet console
const byte hz[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xFA, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x96,
             0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30 
            }; // "hey buddy please transmit at 4Hz"
            
//spooky global variable, fight me                           
String line = "";

//function prototypes
void GPSread(uint64_t *, bool);
void sdLog(std::string);
const char *parseData();

void setup()
{
  //initialize bluetooth serial (device name)
  #if serial 
    SerialBT.begin("GPS Serial"); 
  #endif

  Serial.begin(115200);
  delay(500); //to open serial and read bootCount
  
  //press reset button while running to get 3 second (total) grace period to cut power 
  //(so normal wake cycles don't get delay)
  //(just possibly ignorant SD card write-failure paranoia)
  if (bootCount < 0)  {
    delay(2000); 
    bootCount++;
    delay(500); //forums say RTC variables can be buggy without delay, idk
  }
    
  //old magic initializes SD card on TTGO T1
  pinMode(SS, OUTPUT);  //from the "mySD.h" esp32 example
  bool sdGood = false;
  if(!SD.begin(13,15,2,14)) //SD pins from T1 github pinout
    Serial.println("SD mount failed");
  else{ 
      sdGood = true;
  }

  //even older magic sets GPS tx rate to 4Hz (comment out to stay at 1Hz)
  SerialGPS.begin(9600, SERIAL_8N1, RXD2, TXD2); //gps on pin 16,17
  SerialGPS.write(hz, sizeof(hz)); 

  //GPS read update timer (passed by ref to GPSread), initialized with start time
  uint64_t timer = wait; 
  
  //deep sleep timer 
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  //main "loop" must run from setup if using deep sleep, condition ensures sleep only after TIME_AWAKE seconds 
  while (millis() - wait < TIME_AWAKE*1000)
      GPSread(&timer,sdGood); 

  //clear USB serial output buffer and sleep
  Serial.flush();
  esp_deep_sleep_start();
}

//parse GPS and write to SD
void GPSread(uint64_t *timer, bool sdGood){
  if (millis() - *timer > 100 && SerialGPS.available() && gps.encode(SerialGPS.read())){
      //timer ensures minimal repeat updates and minimal missed updates at 4Hz
      /*validates GPS serial data and SD before writing*/
      if (sdGood){ 
        *timer = millis(); //ensures minimal repeat updates and minimal missed updates
        sdLog(parseData()); //send data
      }
  }
}

//for some reason if I don't cast the returned Cstring to a string then write it 
//to the SD as a Cstring everything catches on fire and sirens go off in the distance... 
void sdLog(std::string ss){ 
  //if file doesn't exist then create, else append
  File myFile = SD.open("NMEA.txt", FILE_WRITE);
  
  //if the file opened then write
  if (myFile) {
     myFile.print(ss.c_str());
     myFile.close();
     delay(10); //10ms wait for ignorant paranoia
     Serial.println(" done."); //confirm file closed to serial   
  }
  //if file didn't open, print error
  else    
    Serial.println("error opening NMEA.txt");
}

//get data from NMEA and return parsed line as Cstring
const char * parseData()
{
  //out stream containing line to be printed
  std::ostringstream ss;

  //get $lat,lng
  if (gps.location.isValid())
      ss << "$" << std::setprecision(9) << gps.location.lat() << "," << gps.location.lng();         
  else
    ss << "$INVALID";
  
  //get $mm/dd/yy
  if (gps.date.isValid())
    ss << "$" << (int)gps.date.month() << "/" << (int)gps.date.day() << "/" << (int)gps.date.year();
  else
    ss << "$INVALID";

  //get $hh:mm:ss.ss
  if (gps.time.isValid())
  {
    ss << "$";
    if (gps.time.hour() < 10) ss << "0";
    ss << (int)gps.time.hour() << ":";
    if (gps.time.minute() < 10) ss << "0";
    ss << (int)gps.time.minute() << ":";
    if (gps.time.second() < 10) ss << "0";
    ss << (int)gps.time.second() << "." ;
    if (gps.time.centisecond() < 10) ss << "0";
    ss << (int)gps.time.centisecond();
  }
  else
    ss << "$INVALID";

  //write stream to BT serial also if true 
  #if serial 
    Serial.print(ss.str().c_str());
    Serial.flush();
    SerialBT.print(ss.str().c_str());
    SerialBT.flush();
  #endif 
   
  ss << "\n";
  return ss.str().c_str();
}

void loop(){
  Serial.println("This is literally impossible");
}

