  Using TTGO T1 ESP32 dev board with built-in microSD slot and ublox NEO-M8N GPS uart on rx 16, tx 17 reading NMEA, 
  parsed into a useble format and saved to onboard fat32 formatted SD card. 
  
  As a data logger it uses integrated deep sleep functions to save power as needed.
  
  Switch `#define serial` allows serial bluetooth to repeat the data at cost of library size and power use when BT transmitting
  as well as having to "pair" devices after a sleep cycle. More of a debug function for now.
   

 Format
  - Expected serial output: `$lat,lng$mm/dd/yy$hh:mm:ss.ss` (and `\n`)
    - note time is UTC
  
  - Sample line: `$51.5074,-0.1278$11/10/2018$20:39:55.50`
 
  - If no data: `$INVALID$INVALID$INVALID`
    - example - no location data: `$INVALID$11/10/2018$20:39:55.25` (likely poor signal)
    
  To Do: 
  - less spaghetti, more object-oriented ravioli
      - iron out timer issues: make AWAKE_TIME dynamic, simplify `wait` and `*timer` shenanigans, possibly rework the whole redundant Cstring shuffle business
 - enable trigger for "dump" command to keep awake, pair to a phone, and read back the SD card over bluetooth (control and read SD card without removing)
  - better file handling: new file at some interval, just append a number--as long as there's a clear sequence and nothing is rewritten to
 - optimize for battery, sense voltage and implement warnings or safe shutdown, *possibly* solar power
 - probably just keep it in my car   
 
Based on:
 - https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/DeepSleep
 - https://github.com/espressif/arduino-esp32/tree/master/libraries/BluetoothSerial
 - https://github.com/mikalhart/TinyGPSPlus/tree/master/examples
 - everything [Andreas Spiess](https://www.youtube.com/channel/UCu7_D0o48KbfhpEohoP7YSQ) has on youtube
