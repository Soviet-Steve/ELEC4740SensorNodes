/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/particle/Documents/4740/clusterhead/src/clusterhead.ino"
/*
 * Project clusterhead
 * Description:
 * Author:
 * Date:
 */

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

void setup();
void loop();
#line 11 "c:/Users/particle/Documents/4740/clusterhead/src/clusterhead.ino"
SYSTEM_MODE(MANUAL);

// 6ab80ed5-ca62-43fc-b937-4c6807fac686 Node 1 Service
// 784474ab-65a9-42ec-9a79-11e279a247c3 Node 1 Temp
// 72219c13-077d-4f34-8978-c9fa03716c2c Node 1 Sound
// e263ab90-26ac-47e1-bc23-3d4bcec72d5e Node 1 Movement
const char* NodeOneServiceUUID = "6ab80ed5-ca62-43fc-b937-4c6807fac686";
const char* NodeOneTempUUID = "784474ab-65a9-42ec-9a79-11e279a247c3";
const char* NodeOneSoundUUID = "72219c13-077d-4f34-8978-c9fa03716c2c";
const char* NodeOneMovementUUID = "e263ab90-26ac-47e1-bc23-3d4bcec72d5e";


BlePeerDevice NodeOne; 
BleUuid NodeOneServiceUuid(NodeOneServiceUUID);

BleCharacteristic tempSensorCharacteristic;
BleCharacteristic soundSensorCharacteristicOne;
BleCharacteristic motionSensorCharacteristic;

BleScanFilter NodeOneFilter;

#define SCAN_RESULT_MAX 30
BleScanResult scanResults[SCAN_RESULT_MAX];

Adafruit_SSD1306 display(-1);


float flTemp, flSound, flMovement;


void onTempReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

// setup() runs once, when the device is first turned on.
void setup() {
  flTemp = -1;
  flSound = -1;
  flMovement = -1;

  NodeOneFilter.serviceUUID(NodeOneServiceUUID);
  // Put initialization like pinMode and begin functions here.
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

  BLE.on();

  tempSensorCharacteristic.onDataReceived(onTempReceived, NULL);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  // uint32_t uinCount = BLE.scan(scanResults, SCAN_RESULT_MAX);
  Vector<BleScanResult> scanResults = BLE.scanWithFilter(NodeOneFilter);
  for(int i = 0; i< scanResults.size(); i++){
    BleUuid foundService;
    uint32_t len;
    len = scanResults[i].advertisingData().serviceUUID(&foundService, 1);
    if(len > 0 && foundService == NodeOneServiceUUID){
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Found a Device");
      display.print("Address: ");
      display.print(scanResults[i].address().toString());
      display.display();
      if(len > 0){
        NodeOne = BLE.connect(scanResults[i].address());
        display.setCursor(0, 30);
        if(NodeOne.connected()){
          display.print("Node :)");
        }else{
          display.print("Node :(");
        }
      display.display();

      }
      if(NodeOne.connected()){
        NodeOne.getCharacteristicByUUID(tempSensorCharacteristic, NodeOneTempUUID);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Temp: ");
        display.print(flTemp);
        display.display();
      }
      // NodeOne = BLE.connect(scanResults[i].address());
    }
  }


}

void onTempReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context){
    memcpy(&flTemp, &data[0], sizeof(flTemp));
    display.clearDisplay();
    display.setCursor(0,50);
    display.print("Raw Temp: ");
    display.print(flTemp);
    display.display();
    // tempSensorCharacteristic.getValue(&flTemp);
}
