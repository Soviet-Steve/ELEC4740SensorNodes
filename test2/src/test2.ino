/*
 * Project test2
 * Description:
 * Author:
 * Date:
 */
SYSTEM_MODE(MANUAL);

#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"



// 6ab80ed5-ca62-43fc-b937-4c6807fac686 Node 1 Service
// 784474ab-65a9-42ec-9a79-11e279a247c3 Node 1 Temp
// 72219c13-077d-4f34-8978-c9fa03716c2c Node 1 Sound
// e263ab90-26ac-47e1-bc23-3d4bcec72d5e Node 1 Movement

const char* NodeOneServiceUUID = "6ab80ed5-ca62-43fc-b937-4c6807fac686";
const char* NodeOneTempUUID = "784474ab-65a9-42ec-9a79-11e279a247c3";
const char* NodeOneSoundUUID = "72219c13-077d-4f34-8978-c9fa03716c2c";
const char* NodeOneMovementUUID = "e263ab90-26ac-47e1-bc23-3d4bcec72d5e";

BleCharacteristic compressedSensorCharacteristic("compressed",BleCharacteristicProperty::NOTIFY, NodeOneTempUUID, NodeOneServiceUUID);

// BleCharacteristic tempSensorCharacteristic("temp",BleCharacteristicProperty::NOTIFY, NodeOneTempUUID, NodeOneServiceUUID);
// BleCharacteristic soundSensorCharacteristic("sound",BleCharacteristicProperty::NOTIFY, NodeOneSoundUUID, NodeOneServiceUUID);
// BleCharacteristic movementSensorCharacteristic("move",BleCharacteristicProperty::NOTIFY, NodeOneMovementUUID, NodeOneServiceUUID);


// BleUuid node_one_service(0xc1a8);

// BleCharacteristic temp_characteristic(
//     "light", 
//     BleCharacteristicProperty::NOTIFY,  // ?
//     BleUuid(0x0543),                    // illuminance
//     node_one_service
// );

// BleCharacteristic sound_characteristic(
//     "sound", 
//     BleCharacteristicProperty::NOTIFY,  // ?
//     BleUuid(0x27C3),                    // sound pressure (decibel) 
//     node_one_service
// ); // Could not find a better UUID.

// BleCharacteristic movement_characteristic(
//     "Motion", 
//     BleCharacteristicProperty::NOTIFY,  // ?
//     BleUuid(0x0541),                    // Motion Sensor
//     node_one_service
// );


#define LDR_PIN 16 // A3
#define SOUND_PIN 19 // A0
#define TEMP_PIN 17 // A2
#define PIR_PIN 5 // D5


#define PIXEL_COUNT 2
#define PIXEL_PIN A3
#define PIXEL_TYPE WS2812B



#define MIC_BUFFER_SIZE 512


#define WAITTIME 1000 // Relates to 1Hz

Adafruit_SSD1306 display(-1);



int ldrValue, soundValue, temperatureValue;
volatile uint64_t uinMovementTime = 0;
uint64_t uinMicTime = 0;
uint32_t aruinMicBuffer[MIC_BUFFER_SIZE];
uint64_t uinUpdateTime = 0;
float flTemp, flSound;
uint8_t uinMotion;

void fnvdMovementIsr();
bool fnboCheckMovement();
float fnflGetMicRMS();
float fnflRMStodBa(float flRmsInput);
void fnvdReadADCMic();
float fnflGetTemperaturemV();
float fnflGetTemperatureC(float flTempmV);
void callbackFunc(const char *event, const char *data);
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
// float fnflGetTempOneWire();









void setup(){
  

  pinMode(D2, OUTPUT);
  // analogWrite(D2, 0, 4);
  digitalWrite(D2, LOW);
  // analogWrite(A3, 65000);

  attachInterrupt(PIR_PIN, fnvdMovementIsr, CHANGE);
  pinMode(PIR_PIN, INPUT);

  // pinMode(D2, OUTPUT);

  // digitalWrite(D2, HIGH);
  // delay(1000);
  // digitalWrite(D2, LOW);

  // // analogWrite(D0, 255);
  // // delay(1000);
  // // analogWrite(D0, 0);


  // BLE.on();

  // BLE.setDeviceName("Monke");
  // BLE.addCharacteristic(temp_characteristic);
  // temp_characteristic.setValue(flTemp);
  // temp_characteristic.onDataReceived(onDataReceived, NULL);
  // BLE.addCharacteristic(sound_characteristic);
  // sound_characteristic.setValue(flSound);
  // BLE.addCharacteristic(movement_characteristic);
  // movement_characteristic.setValue(flMotion);

  // BleAdvertisingData advData;
  // advData.appendServiceUUID(temp_characteristic);
  // BLE.advertise(&advData);

  BLE.on();
  // BLE.addCharacteristic(tempSensorCharacteristic);
  // BLE.addCharacteristic(soundSensorCharacteristic);
  // BLE.addCharacteristic(movementSensorCharacteristic);
  BLE.addCharacteristic(compressedSensorCharacteristic);
  BleAdvertisingData advData;
  advData.appendServiceUUID(NodeOneServiceUUID);
  BLE.advertise(&advData);

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

  // for(int i = 0; i < 25; i++){
  //   analogWrite(D2, i * 10, 4);
  //   delay(1000);
  // }
  // for(int i = 25; i > 0; i++){
  //   analogWrite(D2, i * 10, 4);
  //   delay(1000);
  // }


  // digitalWrite(D2, HIGH);
  // delay(1000);
  // digitalWrite(D2, LOW);
  // delay(1000);


  // fnvdReadADCMic();
  float flmVRms = fnflGetMicRMS(), flTempmV = fnflGetTemperaturemV();
  for(int i = 0; i < 9; i++){flTempmV += fnflGetTemperaturemV();}
  flTempmV /= 10;
  fnflRMStodBa(flmVRms);
  fnflGetTemperatureC(flTempmV);
  fnboCheckMovement();



  if(millis() - uinUpdateTime > WAITTIME){

    if(BLE.connected()){
      time32_t inTime = Time.now();
      uint8_t* pchTxBuff[sizeof(flTemp) + sizeof(flSound) + sizeof(uinMotion) + sizeof(inTime)];
      memcpy(pchTxBuff, &flTemp, sizeof(flTemp));
      memcpy(&pchTxBuff[sizeof(flTemp)], &flSound , sizeof(flSound));
      memcpy(&pchTxBuff[sizeof(flTemp) + sizeof(flSound)] , &uinMotion , sizeof(uinMotion));
      memcpy(&pchTxBuff[sizeof(flTemp) + sizeof(flSound) + sizeof(uinMotion)], &inTime , sizeof(inTime));
      compressedSensorCharacteristic.setValue(pchTxBuff);
      // memcpy(pchTxBuff, &flTemp, sizeof(flTemp));
      // tempSensorCharacteristic.setValue(flTemp);
      // memcpy(pchTxBuff, &flSound, sizeof(flSound));
      // soundSensorCharacteristic.setValue(flSound);
      // memcpy(pchTxBuff, &flMotion, sizeof(flMotion));wsaerft5yuiolp;
      // movementSensorCharacteristic.setValue(flMotion);
    }

    if(flTemp > 24){
      digitalWrite(D2, HIGH); // Fan Speed 2
    }else if(flTemp < 20){
      digitalWrite(D2, LOW); // Fan Off
    }else{
      analogWrite(D2, 128, 4); // Fan Speed 1
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("MIC:");
    // display.print(flmVRms);
    // display.println(" mV");
    display.print(flSound);
    display.println(" dBA");
    display.println("Temperture");
    // display.print(flTempmV);
    // display.println(" mV");
    display.print(flTemp);
    display.println(" C");
    // display.println("1-Wire Temp");
    // display.print(fnflGetTempOneWire());
    // display.println(" C");
    if(uinMotion > 0){display.println("MOVEMENT DETECTED!");}
    display.display();

    // if (BLE.connected()) {
    //   // Set the values of the characteristics.
    //   temp_characteristic.setValue(flTemp);
    //   sound_characteristic.setValue(flSound);
    //   movement_characteristic.setValue(flMotion);
    // }
    uinUpdateTime = millis();
  }
  
  
}

void fnvdMovementIsr(){
  uinMovementTime = millis();
//   display.clearDisplay();
//   display.setCursor(0, 0);
//   display.println("MOVEMENT DETECTED!");
//   display.display();
}

bool fnboCheckMovement(){
  bool bMotion = (millis() - uinMovementTime < 1000);
  uinMotion = bMotion;
  return bMotion;
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
  flSound = 10.1 * logf(flRmsInput) + 26.223;
  return flSound;
}

float fnflGetTemperaturemV(){
  
  return (analogRead(TEMP_PIN) * 3300 / 4096);
  
}


float fnflGetTemperatureC(float flTempmV){
  flTemp = flTempmV * 0.0445 + 13.137;
  return flTemp;
  // return ((flTempmV) * 0.1) - 1.24;
}

void callbackFunc(const char *event, const char *data){
  Log.info("Event: %s, Data: %s", event, data);
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

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    // Check the length of the incoming data. It should be 4 bytes for a uint32_t.
    if (len == sizeof(uint32_t)) {
        // memcpy(&receivedValue, data, sizeof(uint32_t));
        Serial.println("Recieved");
    } else {
        Serial.println("Unexpected data length");
    }
}
