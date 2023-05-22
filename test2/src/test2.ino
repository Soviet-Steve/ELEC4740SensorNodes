/*
 * Project test2
 * Description:
 * Author:
 * Date:
 */

#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"



#define LDR_PIN 16 // A3
#define SOUND_PIN 19 // A0
#define TEMP_PIN 17 // A2
#define PIR_PIN 5 // D5

#define FAN_PIN D2 // D2
#define FAN_OFF 0
#define FAN_HALF 128
#define FAN_FULL 255

#define PIXEL_COUNT 2
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B

#define MIC_BUFFER_SIZE 512

Adafruit_SSD1306 display(-1);



int ldrValue, soundValue, temperatureValue;
volatile uint64_t uinMovementTime = 0;
uint64_t uinMicTime = 0;
uint32_t aruinMicBuffer[MIC_BUFFER_SIZE];

void fnvdMovementIsr();
bool fnboCheckMovement();
float fnflGetMicRMS();
float fnflRMStodBa(float flRmsInput);
void fnvdReadADCMic();
float fnflGetTemperaturemV();
float fnflGetTemperatureC(float flTempmV);
// float fnflGetTempOneWire();

void setup(){
  
  attachInterrupt(PIR_PIN, fnvdMovementIsr, CHANGE);
  pinMode(PIR_PIN, INPUT);

  // sensor.begin();
  // sensor.setResolution(11);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.display(); // show splashscreen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello World");
  display.display();
  display.clearDisplay();   // clears the screen and buffer


  for(uint32_t i = 0; i < MIC_BUFFER_SIZE; i++){aruinMicBuffer[i] = 2048;}

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // fnvdReadADCMic();
  float flmVRms = fnflGetMicRMS(), flTempmV = fnflGetTemperaturemV();
  for(int i = 0; i < 9; i++){flTempmV += fnflGetTemperaturemV();}
  flTempmV /= 10;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("MIC:");
  // display.print(flmVRms);
  // display.println(" mV");
  display.print(fnflRMStodBa(flmVRms));
  display.println(" dBA");
  display.println("Temperture");
  // display.print(flTempmV);
  // display.println(" mV");
  float flTempC = fnflGetTemperatureC(flTempmV);
  display.print(fnflGetTemperatureC(flTempmV));
  display.println(" C");
  // display.println("1-Wire Temp");
  // display.print(fnflGetTempOneWire());
  // display.println(" C");
  if(fnboCheckMovement()){display.println("MOVEMENT DETECTED!");}
  display.display();
  
  if (flTempC < 20.0)
    analogWrite(FAN_PIN, FAN_OFF);
  else if (flTempC < 24.0)
    analogWrite(FAN_PIN, FAN_HALF);
  else
    analogWrite(FAN_PIN, FAN_FULL);

  
}

void fnvdMovementIsr(){
  uinMovementTime = millis();
//   display.clearDisplay();
//   display.setCursor(0, 0);
//   display.println("MOVEMENT DETECTED!");
//   display.display();
}

bool fnboCheckMovement(){
  return (millis() - uinMovementTime < 1000);
}


float fnflGetMicRMS(){
  float flRms = 0;
  uint64_t uinMean = 0;
  


  for(uint32_t i = 0; i < MIC_BUFFER_SIZE; i++){
    aruinMicBuffer[i] = analogRead(SOUND_PIN);
    uinMean += aruinMicBuffer[i];
  }
  uinMean /= MIC_BUFFER_SIZE;

  for(uint32_t i = 0; i < MIC_BUFFER_SIZE; i++){
    flRms += (aruinMicBuffer[i] - uinMean) * (aruinMicBuffer[i] - uinMean);
  }
  flRms /= MIC_BUFFER_SIZE;
  flRms = sqrt(flRms);

  flRms = (flRms - 2.4595) / 1.2254;
  return flRms;
}

void fnvdReadADCMic(){
  static uint32_t stuinCounter = 0;
  if(stuinCounter >= MIC_BUFFER_SIZE){stuinCounter = 0;}
  aruinMicBuffer[stuinCounter] = analogRead(SOUND_PIN);
  stuinCounter++;
}

float fnflRMStodBa(float flRmsInput){
  return 10.1 * logf(flRmsInput) + 26.223;
}

float fnflGetTemperaturemV(){
  return (analogRead(TEMP_PIN) * 3300 / 4096);
}


float fnflGetTemperatureC(float flTempmV){
  return flTempmV * 0.0445 + 13.137;
  // return ((flTempmV) * 0.1) - 1.24;
}

// float fnflGetTempOneWire(){
//   static float tmp = 0;
//   if (sensor.isConversionComplete())
//   {
//   // Serial.print("Temp: ");
//   float tmp= sensor.getCRCTempC();
//   sensor.requestTemperatures();
//   }
//   return tmp;
// }


/*
  // The core of your code will likely live here.
  float sum = 0, level = 0;
  for ( int i = 0 ; i < 1024; i++){  
    sum += analogRead(SOUND_PIN) - 2048;
  } 
  level = sum / 1024; // Calculate the average value 
  // ldrValue = analogRead(LDR_PIN);
  soundValue = analogRead(SOUND_PIN);
  temperatureValue = analogRead(TEMP_PIN);
  // Log.info("analogvalue=%d", ldrValue);
  
  display.clearDisplay();
  display.setCursor(0, 0);
  // display.print("LDR value: ");
  // display.println(ldrValue);
  display.print("MIC value: ");
  display.println(soundValue);
  display.print("DB value: ");
  display.println(level);
  display.print("TMP value: ");
  display.println(temperatureValue);
  // display.print("D5 Val: ");
  // display.println(digitalRead(PIR_PIN));
  // display.print("MOV: ");
  // display.println(fninCheckMovement());
  if(fnboCheckMovement())
    display.println("MOVEMENT DETECTED!");
  display.display();

*/