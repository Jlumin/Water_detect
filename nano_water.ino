#include "src\Arduino-Statistic-Library\Statistic.h"  // without trailing s
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <DS3231.h>
//#include <RTClib.h>
//#include <histogram.h>
#include <I2CSoilMoistureSensor.h>
#include "src\SIM7020\SIMcore.h"
#include <SoftwareSerial.h>
#include <histogram.h>
Statistic myStats;


/*** uSD SPI pin definition ***/
const int chipSelect = 8;
//const uint8_t carddetect = 4;
/*** Soil Moisture Sensor (SMS) Vcc pin definition ***/
//int NumberOfSMS = 2;    // Amount of SMS
unsigned long lastMillis = 0;   // last measururing millis
#define sensorPin  4

float Tempra;
float distance = 0;
//float duration = 0;
float ocm = 0.0;
String timeStamp1;
String timeStamp2;
float cm;
const byte PROGMEM samplingNum = 20;
float sampleArray[samplingNum];
int timeCount   = -1;
bool bootFlag = 1; 
float pipeLength;
const float PROGMEM minDepth         = -0.6; //  Unit in cm
const float PROGMEM officalThreshold = 5.0;  //  Unit in cm
unsigned long timeNow;
unsigned long timeFuture;
const int PROGMEM histIntNum = 60;
float histIntVal[histIntNum];
const float PROGMEM freqThreshold = 9.0;
float water_means;
float water_sta;

float gettemperature() {
  int t; 
//  t= Clock.getTemperature();
  float celsius = t / 4.0;
  return celsius;
}

float pipeLgMeasurement() {
  // Take a measurement
  //
  Serial.println(F("Start to measure the total length of pipeline......"));
  float Tempra = 25;
  float dist;
  for ( uint32_t ii = 0; ii < samplingNum;) {
    dist = sampling(Tempra);
    //depend on the feild situation  
    if (dist>60){   //Manual setting
    sampleArray[ii] = dist;
    myStats.add(sampleArray[ii]);
    ii++;
  }
  }
//Find the standard devation and mean value
    water_means = float(myStats.average());
    water_sta = float(myStats.pop_stdev());
  // Find a median in the sampling array 
  //    
  BubbleSort(sampleArray,samplingNum);
  float pipeLgMedian = sampleArray[samplingNum/2];  
  return pipeLgMedian;  
}

float sampling(float Tempra) {
  float duration; // the roundtrip duration of the signal in us
  // Turn off the trigger
  digitalWrite(sensorPin, LOW); 
  delayMicroseconds(5);
  //noInterrupts(); // disable interrupts as they might interfere with the measurement

  // Turn on the trigger
  digitalWrite(sensorPin, HIGH);
  delayMicroseconds(10); // wait for 10us (module sends signal only, if trigger had a HIGH signal for at least 10 us)
  digitalWrite(sensorPin, LOW); // module sends signal now
  pinMode(sensorPin, INPUT); 
   // Receive a HIGH signal on the echo pin
   duration = pulseIn(sensorPin, HIGH); 
   //interrupts(); 
                                                                          
   // Convert roundtrip duration to distance
   float durationOneWay = duration / 2; // divided by two, since duration is a roundtrip signal
   float distance = durationOneWay * ((331.4 + 0.606 * Tempra)/10000); // Unit in cm
   delay(500); // wait for 500ms
   return distance;
}

void BubbleSort(float a[], uint32_t size) {
    for(uint32_t ii = 0; ii <(size-1); ii ++) {
        for(uint32_t jj = 0; jj <(size-(ii+1)); jj++) {
                if(a[jj] > a[jj+1]) {
                    float temp = a[jj];
                    a[jj] = a[jj+1];
                    a[jj+1] = temp;
                }
        }
    }
}

float *periodicMeasurement() {
   static float MeasData[3]; // [Median, Freq, pipeLength]
   //float MeasData[4]; // [Median, Freq, pipeLength]
  // Take periodic measurements every half hour
  Serial.println(pipeLength);
  Serial.println(F("Start measurement......"));
  float Tempra = 25;
  for ( int ii = 0; ii < samplingNum; ii++) {
    float dist = sampling(Tempra);
    if (dist <= water_means + 5 * water_sta && dist >= water_means - 5 * water_sta ){   
    float tempData = pipeLength - dist;
    if (tempData >= minDepth) {
      sampleArray[ii] = tempData;      
    } else if (tempData < minDepth) {      
      Serial.println(F("Error!(dep)"));
    } else {
      ii--;
    }     
  }
  }
  
  // Histogram Analysis
  //
  for ( int jj = 0; jj < histIntNum; jj++) {
    histIntVal[jj] = jj * (pipeLength/(histIntNum-1));
  }  
  
  Histogram hist(histIntNum, histIntVal);
  
  for (int kk = 0; kk < samplingNum; kk++){
    hist.add(sampleArray[kk]);
  } 
    
  // Find a median in the sampling array 
  //    
  BubbleSort(sampleArray,samplingNum);
  MeasData[0] = sampleArray[samplingNum/2];

  // Find a frequency distribution in the sampling array
  MeasData[1] = hist.frequency(0) + hist.frequency(1);
  hist.clear(); 
  
  return MeasData;  
}
int scenarioDetect(float dep, float feq) {
  // Scenario detection
  // 
  // Mode 1 (wait for 180 min) : 1
  // Mode 2 (wait for 10  min) : 2 
  // Mode 3 (wait for 05  min) : 3
  int modeNum;  
  if ( dep > 0.0 && feq <= freqThreshold && dep < officalThreshold) {
    modeNum = 2;
  } else if ( dep > 0.0 && feq <= freqThreshold && dep >= officalThreshold) {
    modeNum = 3;
  } else {
    modeNum = 1;
  }
  return modeNum;
}

void setup(void)
{
 Wire.begin();
 Serial.begin(19200);
 pinMode(sensorPin, INPUT);
 Serial.print("Demo Statistic lib ");
 //Serial.println(STATISTIC_LIB_VERSION);
 myStats.clear(); //explicitly start clean
 
}

void loop(void)
{
   while (timeCount < 0) {
  pipeLength = pipeLgMeasurement();
  Serial.print(F("Pipe length: "));
  Serial.print(pipeLength);
  Serial.println(F(" cm"));
  Serial.println(F("----------------------------"));
//  String presentStr;
//  RTCread(presentStr);
//  upload(presentStr.c_str(), String(pipeLength).c_str()); // Timestamp & pipe length
//  sim7000.turnOFF();
  delay(3000); 
  timeCount =1;
  break;
  }
  float *GetMeasData = periodicMeasurement(); // GetMeasData = [Median, Freq]
  Serial.print(GetMeasData[0]);
//  int modeNum = scenarioDetect(GetMeasData[0], GetMeasData[1]);
//  Serial.print(F("Mode number: "));
//  Serial.println(modeNum);
//  Serial.print(F("Time count: "));
//  Serial.println(timeCount);
////   Mode 1 (wait for 180 min) : modeNum = 1
////   Mode 2 (wait for 10  min) : modeNum = 2 
////   Mode 3 (wait for 05  min) : modeNum = 3
//  if (modeNum == 1 && timeCount < 6) {
//      timeCount ++;
////      RTCread(present);
//      //DateTime present = Clock.now();
//      Serial.println(F("Current Time(B): "));
////      Serial.println(present);      
//      Serial.println(F("It is time to sleep."));    
//      delay(1000);
//      
//    } else if (modeNum == 1 && timeCount == 6) {
//      Serial.println(F("No flood."));
//      Serial.println(F("Communication mode: 1"));
//      Serial.println(F("Sending a text message"));
//      Serial.print(F("Water depth: "));
//      Serial.print(GetMeasData[0]);
//      Serial.println(F(" cm"));      
////      if (GetMeasData[0]<abs((water_means + water_sta)-pipeLength)){
////        GetMeasData[0] = 0.0;
////      }
//      GetMeasData[0] = 0.0;
//      //Serial.println(DateTimeString(present));
//      //Serial.println(F("-----------Look at here!-----------------"));
////      String presentStr =  present;                   
//      timeCount = 0;
//         
//  }  else if (modeNum == 2) {      
//      timeCount = 0;
////      RTCread(present);
//      //DateTime present = Clock.now();
//      //Serial.println(F("Current Time: "));
//      //Serial.println(DateTimeString(present));
//      Serial.println(F("Communication mode: 2"));
//      Serial.println(F("Sending a text message"));
//      Serial.print(F("Water depth: "));
//      Serial.print(GetMeasData[0]);
//      Serial.println(F(" cm"));
//                                       
//      
//          
////      Serial.println(F("----------------------------")); 
////      timeNow    = millis();
////      timeFuture = timeNow ;   
////      
////      while ( millis() < timeFuture) { 
////        delay(5000); // In this while loop, wait for 20 seconds 
////      }   
//  } else if (modeNum == 3) {      
//      timeCount = 0;
////      RTCread(present);
//      Serial.println(F("Current Time: "));
////      Serial.println(present);
//      Serial.println(F("Communication mode: 3"));
//      Serial.println(F("Sending a text message"));
      Serial.print(F("Water depth: "));
      Serial.print(GetMeasData[0]);
      Serial.println(F(" cm"));
                                     
      
      
//      Serial.println(F("----------------------------"));    
//      timeNow    = millis();
      //Serial.println(String(timeNow));
////      timeFuture = timeNow ;      
////      while ( millis() < timeFuture) { 
////        delay(5000); // In this while loop, wait for 10 seconds 
////      }  
//  }
Serial.println(timeCount);
timeCount = 0;
Serial.println(timeCount);
Serial.println(GetMeasData[0]);
Serial.println(GetMeasData[1]);
Serial.println(GetMeasData[2]);
Serial.println(String(water_means));
Serial.println(String(water_sta));
delay(1000);
}
