/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/Users/Clayton/Documents/Particle/node_2/src/node_2.ino"
/*
Includes:
*/
#include "math.h"   // needed for the inverse transfer-functions, etc.
#include "Particle.h"

/*
~~~ Pins ~~~
*/
void setup();
void loop();
const float convert_from_peak(const float peak);
const float convert_from_rms(const float rms);
const float get_raw_voltage(const float v);
const float convert_to_lux(const float v);
const float calibrate_light(const float measured);
const float calibrate_sound(const float measured);
const float calibrate_distance(const float measured);
#line 10 "c:/Users/Clayton/Documents/Particle/node_2/src/node_2.ino"
static const uint8_t trigger_pin = D7;
static const uint8_t echo_pin = D8;
static const uint8_t sound_pin = A0;
static const uint8_t light_pin = A1;
static const uint8_t led_pin = D2;

// A watchdog counter to time the printing to the terminal.
static uint16_t watchdog = 0;

// For some reason, this is needed to skip the WiFi connection.
SYSTEM_MODE(MANUAL);

SYSTEM_THREAD(ENABLED);

/*
~~~ BLE ~~~
*/

// We are going to have one service for each node and two characteristeristics 
// for each service.

BleUuid node_two_service(0xc1a7);

BleCharacteristic light_characteristic(
    "light", 
    BleCharacteristicProperty::NOTIFY,  // ?
    BleUuid(0x2afb),                    // illuminance
    node_two_service
);

BleCharacteristic sound_characteristic(
    "sound", 
    BleCharacteristicProperty::NOTIFY,  // ?
    BleUuid(0x2b83),                    // audio output description
    node_two_service
); // Could not find a better UUID.

BleCharacteristic distance_characteristic(
    "distance", 
    BleCharacteristicProperty::NOTIFY,  // ?
    BleUuid(0x2701),                    // distance (arbitrary)
    node_two_service
);

// Declare the variables for each characteristic.
float light_value = 0;      // lux
float sound_value = 0;      // dBA
float distance_value = 0;   // mm

/*
~~~ Set-up ~~~
*/
void setup() {
    // Begin the serial connection.
    Serial.begin(9600);

    // Turn the BLE on and add the stuff on.
    BLE.on();

    BLE.setDeviceName("Clayton_Node_Two");

    BLE.addCharacteristic(light_characteristic);
    light_characteristic.setValue(light_value);
    BLE.addCharacteristic(sound_characteristic);
    sound_characteristic.setValue(sound_value);
    BLE.addCharacteristic(distance_characteristic);
    distance_characteristic.setValue(distance_value);

    BleAdvertisingData adv_data;

    adv_data.appendServiceUUID(node_two_service);
    adv_data.appendLocalName("Clayton_Node_Two");

    BLE.advertise(&adv_data);

    // Set the pins.
    pinMode(trigger_pin, OUTPUT);
    pinMode(echo_pin, INPUT);
    pinMode(D2, OUTPUT);

    // Print a greeting eight times.
	delay(1000);
	for (int i = 0; i < 8; i++) {
		Serial.println("Hello, World!");
		delay(1000);
	}
}

/*
~~~ Loop ~~~
*/
void loop() {
    // This is the maximum voltage on the ADC, used for conversion, and the 
    const float max_volt = 3.3;

    // ~~~ Sound sensor ~~~

    // This is the likely DC offset of the processing circuit of the microphone.
    const float mid_volt = 1.65;

    // The output of the sound-sensor in volts:
    static float sound_volts[2] = {0.0,0.0};
    // The weights for the digital filter:
    static float weights[2] = {0.5, 0.5};
    // The highest voltage read from the sound-sensor:
    static float highest_volt = 0.0;
    // The sum of the squares of the voltage:
    static float square_sum = 0.0;

    // The peak voltage of the sound-sensor's amplitude:
    float peak = 0.0;
    // The highest peak so far:
    static float highest_peak = 0.0;

    // Read the voltage from the sound-sensor and filter it.
    sound_volts[1] = analogRead(sound_pin) * max_volt / 4096;
    sound_volts[0] += weights[0]*sound_volts[1] + weights[1]*sound_volts[0];
    if (sound_volts[1] > highest_volt)
        highest_volt = sound_volts[1];

    // Calculate the peak voltage.
    peak = abs(sound_volts[1] - mid_volt);
    if (peak > highest_peak)
        highest_peak = peak;
    
    // Update the sum of squares.
    square_sum += (sound_volts[1] - mid_volt) * (sound_volts[1] - mid_volt);
    
    // ~~~ Light sensor ~~~

    // Read the voltage from the light-sensor.
    static float light_volt = 0.0;
    light_volt += analogRead(light_pin) * max_volt / 4096;

    // Every now and then, process the data and print it.
    if (!watchdog) {
        // Calculate the RMS as the square-root of the mean of the squares.
        const float rms = sqrt(square_sum/UINT16_MAX);

        // ~~~ Distance sensor ~~~
        // I wanted this to be done once every frame lest it slows down the 
        // processing of the microphone, etc.
        uint32_t lag;
        uint32_t mm;

        // Send the pulse.
        digitalWrite(trigger_pin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigger_pin, HIGH);
        delayMicroseconds(5);
        digitalWrite(trigger_pin, LOW);

        // Measure the delay and calculate the distance.
        lag = pulseIn(echo_pin, HIGH);
        // mm = ((lag / 1000000) * (343)) * 1000 / 2
        mm = lag * 343 / 2 / 1000;

        // Calculate the output values for all three sensors.
        sound_value = calibrate_sound(convert_from_peak(highest_peak));
        light_value = calibrate_light(convert_to_lux(light_volt/UINT16_MAX));
        distance_value = calibrate_distance(mm);

        /*
        - highest voltage read from the sound-sensor in V,
        - the highest peak from the sound-sensor in V,
        - the RMS from the sound-sensor in V,
        - the sound-intensity converted from the peak voltage in dBA,
        - the sound-intensity alternatively converted from the RMS in dBA,
        - the light-intensity in lux,
        - the distance in mm,
        */
        Serial.print(highest_volt);                     Serial.print(",\t");
        Serial.print(highest_peak);                     Serial.print(",\t");
        Serial.print(rms);                              Serial.print(",\t");
        Serial.print(sound_value);                      Serial.print(",\t");
        Serial.print(calibrate_sound(convert_from_rms(rms)));
        Serial.print(",\t");
        Serial.print(light_value);                      Serial.print(",\t");
        Serial.print(distance_value);                   Serial.print("\r\n");

        if (BLE.connected()) {
            // Set the values of the characteristics.
            light_characteristic.setValue(light_value);
            sound_characteristic.setValue(sound_value);
            distance_characteristic.setValue(distance_value);
        }

        // Set the duty-cycle of the LED.
        if (light_value < 200)
            analogWrite(led_pin, 255); // 100 %
        else if (light_value < 400)
            analogWrite(led_pin, 192); // 75 %
        else
            analogWrite(led_pin, 128); // 50 %
        
        // Reset the running totals.
        highest_volt = 0.0;
        highest_peak = 0.0;
        square_sum = 0.0;
        light_volt = 0.0;
    }

    watchdog++;
}

/*
@brief  converts the peak voltage read on the ADC to the sound-intensity 
        according to the inverse transfer-function.
@param  the peak voltage in V,
@return the sound-intensity in dBA,
*/
const float convert_from_peak(const float peak) {
    const float b = 15.412;
    const float k = 8.1549;
    return (log10(2*get_raw_voltage(peak)*1000 - b) + k)*10;
}

/*
@brief  converts the RMS voltage read on the ADC to the sound-intensity 
        according to the inverse transfer-function.
@param  the RMS voltage in V,
@return the sound-intensity in dBA,
*/
const float convert_from_rms(const float rms) {
    const float b = 3.3676;
    const float k = 9;
    return (log10(get_raw_voltage(rms)*1000 - b) + k)*10;
}

/*
@brief  converts the voltage read on the ADC to voltage from the microphone 
        back through the voltage divider and the amplifier.
@param  the peak voltage in V,
@return the sound-intensity in dBA,
*/
const float get_raw_voltage(const float v) {
    return (v * 5.0 / 3.3) / 100.0;
}

/*
@brief  converts the voltage read on the ADC to the light-intensity
        according to the inverse transfer-function.
@param  the voltage in V,
@return the light-intensity in lux,
*/
const float convert_to_lux(const float v) {
    const float b = 2.4037;
    const float m = -0.5275;
    return pow(v/(exp(b)),1.0/m);
}

/*
@brief  calibrates and corrects the output of the transfer-function for the 
        light-sensor.
@param  the measured light-intensity in lux,
@return the calbirated light-intensity in lux,
*/
const float calibrate_light(const float measured) {
    const float b = -0.2592;
    const float m = 1.0851;
    return exp((log(measured)-b)/m);
}

/*
@brief  calibrates and corrects the output of the transfer-function for the 
        sound-sensor.
@param  the measured sound-intensity in dBA,
@return the calbirated sound-intensity in dBA,
*/
const float calibrate_sound(const float measured) {
    const float b = 72.783;
    const float m = 0.2689;
    return (measured-b)/m;
}

/*
@brief  calibrates and corrects the output of the transfer-function for the 
        distance-sensor.
@param  the measured distance in mm,
@return the calbirated distance in mm,
*/
const float calibrate_distance(const float measured) {
    const float b = -15.571;
    const float m = 0.9971;
    return (measured-b)/m;
}