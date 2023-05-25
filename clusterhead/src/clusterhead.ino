/*
 * Project clusterhead
 * Description:
 * Author:
 * Date:
 */

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

SYSTEM_MODE(MANUAL);

// Swap these around if they are wrong.
#define LED_1_R D8
#define LED_1_G D7
#define LED_2_R D6
#define LED_2_G D5

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
float flLight, flSound2, flDistance;

enum SoundState {
  SAFE,   // < 55 dBA
  GREEN,  // 55 - 70 dBA
  RED_1,  // 70 - 80 dBA
  RED_2   // > 80 dBA
};
SoundState sound_state = SAFE, new_sound_state = SAFE;

enum LEDState {
  OFF,      // off
  GREEN_05, // 0.5 Hz
  GREEN_2,  // 2 Hz
  RED_1,    // 1 Hz
  RED_2     // 2 Hz
};
LEDState led_state_1 = OFF, new_led_state_1 = OFF, 
  led_state_2 = OFF, new_led_state_2 = OFF;

struct {
  uint8_t node = 0;
  bool sound_event = true;
  float sound_level = 0;
  float distance = 0;
  float time = 0;
  float duration = 0;
} event;
bool new_event = false;

Timer sound_timer(30000, sound_timer_callback);
bool sound_timer_flag = false;

Timer movement_timer(60000, movement_timer_callback);
bool movement_timer_flag = false;

Timer led_timer_1(1000, led_timer_1_callback);
Timer led_timer_2(1000, led_timer_2_callback);

void onTempReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

void sound_timer_callback();
void movement_timer_callback();

void led_timer_1_callback();
void led_timer_2_callback();

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
  if (!BLE.connected()) {
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

  // Choose the next state based on the level of sound.
  if ((flSound > 80) || (flSound2 > 80)) {
    new_sound_state = SoundState::RED_2;
  } else if ((flSound > 70) || (flSound2 > 70)) {
    new_sound_state = SoundState::RED_1;
  } else if ((flSound > 55) || (flSound2 > 55)) {
    new_sound_state = SoundState::GREEN;
  } else {
    sound_timer.stop();
  }

  // If movement has been detected, then register an event and turn the second 
  // LED on.
  if (flMovement) {
    // Set the distance here.
    // event.distance = ?
    event.node = 1;
    event.sound_event = false;
    movement_timer.reset();
    new_led_state_2 = GREEN_05;
  }

  // If the timer is done, then choose the next state of the LED based on the 
  // state of the sound.
  if (sound_timer_flag) {
    switch (sound_state) {
      default:
      case RED_2:
        break;
      case RED_1:
        new_led_state_1 = RED_1;
        new_event = true;
        break;
      case GREEN:
        new_led_state_1 = GREEN_2;
        new_event = true;
        break;
      case SAFE:
        new_led_state_1 = OFF;
        break;
    }
  }

  // Start the timer based on the transition of the state.
  if (new_sound_state != sound_state) {
    switch (new_sound_state) {
      case RED_2:
        new_led_state_1 = RED_2;
        new_event = true;
        // No break here! let the same timer begin.
      case RED_1:
        // Either change the period or restart the timer at 10 s.
        sound_timer.changePeriod(10000);
        break;
      case GREEN:
        // Either change the period or restart the timer at 30 s.
        sound_timer.changePeriod(30000);
        break;
      default:
      case SAFE:
        // Restart the timer at 60s.
        sound_timer.changePeriod(60000);
        sound_timer.reset();
        break;
    }

    // Need to set the time and duration.
    if (flSound > flSound2) {
      event.sound_level = flSound;
      event.node = 1;
    } else {
      event.sound_level = flSound2;
      event.node = 2;
    }

    sound_state = new_sound_state;
  }

  // If there has been an event, print it.
  // Need to print time and duration too.
  if (new_event) {
    display.print("Alarm:");
    // Sound
    if (event.sound_event) {
      display.print(" Sound ");
      display.print(event.sound_level);
      display.print(" dBA");
      if (event.node == 1) {
        display.print(" at Node-1");
      } else {
        display.print(" at Node-2");
      }
    // Movement
    } else {
        display.print(" Movement ");
        display.print(event.distance);
        display.print(" cm at Node-1");
    }
    display.display();

    new_event = false;
  }

  // Turn on the LEDs if there has been a new state.
  if (new_led_state_1 != led_state_1) {
    switch (new_led_state_1) {
      case RED_2:
        analogWrite(LED_1_R, 128, 2);
        analogWrite(LED_1_G, 255, 2);
        break;
      case RED_1:
        analogWrite(LED_1_R, 128, 1);
        analogWrite(LED_1_G, 255, 2);
        break;
      case GREEN_2:
        analogWrite(LED_1_R, 255, 2);
        analogWrite(LED_1_G, 128, 2);
        break;
      case GREEN_05:
        analogWrite(LED_1_R, 255, 2);
        led_timer_1.reset();
        break;
      default:
      case OFF:
        break;
    }
  }
  if (new_led_state_2 != led_state_2) {
    switch (new_led_state_2) {
      case RED_2:
        analogWrite(LED_2_R, 128, 2);
        analogWrite(LED_2_G, 255, 2);
        break;
      case RED_1:
        analogWrite(LED_2_R, 128, 1);
        analogWrite(LED_2_G, 255, 2);
        break;
      case GREEN_2:
        analogWrite(LED_2_R, 255, 2);
        analogWrite(LED_2_G, 128, 2);
        break;
      case GREEN_05:
        analogWrite(LED_2_R, 255, 2);
        led_timer_2.reset();
        break;
      default:
      case OFF:
        break;
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

void sound_timer_callback() {
  sound_timer_flag = true;
}

void movement_timer_callback() {
  //movement_timer_flag = true;
  // Just turn the second LED off.
  new_led_state_2 = OFF;
}

void led_timer_1_callback() {
  static bool on_state = 0;
  digitalWrite(LED_1_G, on_state);
  on_state ^= 1;
  led_timer_1.reset();
}

void led_timer_2_callback() {
  static bool on_state = 0;
  digitalWrite(LED_2_G, on_state);
  on_state ^= 1;
  led_timer_2.reset();
}