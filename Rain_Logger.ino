// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"  //adafruits RTClib https://github.com/adafruit/RTClib

// include the SD library:
#include <SPI.h>
#include <SD.h>
 
//***********STUFF to change depending on setup************//
  /* SD card attached to SPI bus as follows:
 ** MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila, 51 on mega
 ** MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila, 50 on mega
 ** CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila,  52 on mega
 ** CS - depends on your SD card shield or module.*/
const int chipSelect = 53;

//define external input pins used here
const int status_led = 15;

const int input_0 = 2;     // the number of the input pins
const int input_1 = 3;

const int INPUT_COUNT = 2; //number of input sensors, should match total defined above

const int TIME_INCREMENT = 10;  //time increment to log data in seconds, only works for increments >=2
const int EVENT_END_TIMER = 5; //time of zero activity needed to trigger new log file in  minutes
//**********************************************************//

///****************Do not change these*********************///
///program global variables and CONTSTANTS
const char delimeter = ' ';
int counts[INPUT_COUNT];   //array of counters to store number of triggers
int prev_state[INPUT_COUNT]; //array to compare against for state changes
boolean EVENT_ACTIVE = false; //flag to determine if event is active
int last_change_time = 0; //time of last event activity
boolean logged = true; //flag to indicate if logged this cycle
String filename;
File dataFile;

RTC_DS1307 rtc;
//DS1307 is connected via I2C pins
/*SDA on DS1307 to UNO SDA
 * SCL on DS1307 to UNO SCL
 */
///******************************************************///

void setup () {

  // init output pins
  pinMode(status_led, OUTPUT);
  digitalWrite(status_led, LOW);
  
  // initialize the input pins: 
  pinMode(input_0, INPUT_PULLUP);
  pinMode(input_1, INPUT_PULLUP);

 
    // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial); // for Leonardo/Micro/Zero

  Serial.print("\nInitializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed! You fucked up.");
    return; 
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
     //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     //rtc.adjust(DateTime(2018, 4, 8, 18, 24, 0));
  }

/// intialize counts to 0 ////
  reset_counts();

//// initialize previous states array values ////
  for (int i=0; i<INPUT_COUNT; i++){
    prev_state[i] = 1;  ///set original state to 1 since we are using internal pullups on inputs
  }

}

void loop () {
    DateTime now = rtc.now();  

    update_counts();  //call function which polls inputs and updates counts array

    if(counts_changed(now) == true && EVENT_ACTIVE == false) {  
      //counts changed so something is happening and event is not already active 
              EVENT_ACTIVE = true;  //set event to active
              filename = build_filename(now); //set filename to use for this event
              log_to_SD(filename, build_header());  //log data header to file
              digitalWrite(status_led, HIGH);
    }
    
    //see if EVENT_END_TIMER minutes have passed since last activity      
    else if (now.hour()*60 + now.minute() > last_change_time + EVENT_END_TIMER) {  
          //if so then disable EVENT_ACTIVE flag and close out log file
          EVENT_ACTIVE = false;  //no longer in an event so set flag to false
          digitalWrite(status_led, LOW);
    }

    
    if(EVENT_ACTIVE == true && now.second()%TIME_INCREMENT == 0){ 
      //if during an active Event then log data to file every TIME_INCREMENT seconds

          if(logged == false){ //havent logged data for this period yet
              String log_string;
              //build string to log to file
              log_string = build_time_stamp(now); //add time_stamp
              log_string += build_data_fields(); //append data fields to string

              log_to_SD(filename, log_string); //log to file

              reset_counts();  //reset counts to 0 after logging

              logged = true;  //logged flag prevents logging the same data multiple times
          }
        
    }
    else{//time has changed enough to log again so set logged flag back to false
      logged = false; 
    }
    
}

//Reset all counts back to 0
void reset_counts (){  
  for (int i=0; i<INPUT_COUNT; i++){
  counts[i] = 0;
}
  
}

//update all count values depending on interrupts, modifys global arrays counts and prev_states
void update_counts(){

  int current_state[INPUT_COUNT];

  //read inputs
  current_state[0] = digitalRead(input_0);
  current_state[1] = digitalRead(input_1);

 for(int i=0; i<INPUT_COUNT; i++){
  if(current_state[i] != prev_state[i]){
  // if state changed then increment counts for respective inputs
  counts[i]++;
  prev_state[i] = current_state[i];
  }
 }
}

//checks to see if any counts have changed and returns true or false
//also updated globalvariable last_time_changed which is used to check against the EVENT_END_TIMER
boolean counts_changed(DateTime now){

  boolean changed = false;

  int x = 0;

  for(int i=0; i<INPUT_COUNT; i++){
    x += counts[i];
  }

  if (x > 0){
    changed = true;
    last_change_time = now.hour()*60 + now.minute(); //time change in minutes
  }

  return changed;
  
}

//builds and returns logfile timestamp string
String build_time_stamp(DateTime now){
     String time_stamp;
     
      //build string to log to file
      time_stamp = now.month();
      time_stamp += "/";
      time_stamp += now.day();
      time_stamp += "/";
      time_stamp += now.year();
      time_stamp += delimeter;
      time_stamp += now.hour();
      time_stamp += ":";
      
      if(now.minute() <10){ time_stamp +="0";}
      time_stamp += now.minute();      
      time_stamp += ":";

      if(now.second() <10){ time_stamp +="0";} 
      time_stamp += now.second();

      return time_stamp;  
}

//builds and returns logfile data field string
String build_data_fields(){
     
     String data_fields;

     for(int i=0; i<INPUT_COUNT; i++){
        data_fields += delimeter;
        data_fields += counts[i]/2; //divide by 2 to account for double counts by rain guages
      }

      return data_fields;  
}

//builds and returns filename string
String build_filename(DateTime now){
     String time_stamp;
     
      //build string to log to file
      //filename needs to comply with the 8.3 filenaming convention
      //example:  12345678.123, textfile.txt, 
      //current filename format is: 03090759.csv, or MMDDHHMM.csv

      if(now.month() <10){ time_stamp ="0";} 
      time_stamp += now.month(); 
      if(now.day() <10){ time_stamp +="0";} 
      time_stamp += now.day();
      if(now.hour() <10){ time_stamp +="0";}
      time_stamp += now.hour();
      if(now.minute() <10){ time_stamp +="0";}
      time_stamp += now.minute();
      time_stamp += ".csv";

      return time_stamp;  
}

//builds and returns header string
String build_header(){
     String header;
     
      //build log file header

      header = "Date";
      header += delimeter;
      header += "Time";

      for(int i=0; i<INPUT_COUNT; i++){
        header += delimeter;
        header += "Sensor_";
        header += i+1;
      }

      return header;  
}

//Opens filename on SD card and logs log_string to file
void log_to_SD(String file_name, String log_string){

               dataFile = SD.open(file_name, FILE_WRITE);
              // if the file is available, write to it:
              if (dataFile) {
                dataFile.println(log_string);
                dataFile.close();
                // print to the serial port too:
                Serial.println(log_string);  //send string to serial connection
              }
              // if the file isn't open, pop up an error:
              else {
                Serial.print("error opening: ");
                Serial.println(file_name);
              }

}

