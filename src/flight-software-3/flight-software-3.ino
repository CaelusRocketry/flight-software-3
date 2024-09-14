#include "Arduino.h"

#include <SPI.h>
#include <HX711.h>
#include <Adafruit_MAX31856.h>

#include <vector>

#define SCK         4
#define MISO        19
#define MOSI        23
#define CS1         5
#define CS2         18

#define PT_1        12
#define PT_2        14
#define PT_3        33
#define PT_4        32
#define PT_5        35
#define PT_6        34
#define PT_DELAY    500

#define MIN_TEMP    0.0
#define MAX_THERMOS 3
#define MAX_PTS     6

const int PT[MAX_PTS] = {PT_1, PT_2, PT_3, PT_4, PT_5, PT_6};
const int PT_BAR[MAX_PTS] = {5, 5, 5, 5, 5, 5};

unsigned long prev_time = millis();

std::vector<float> pt_vals;

std::vector<int> thermo_pins;
std::vector<float> thermo_vals;
std::vector<Adafruit_MAX31856*> afmax_thermos;

std::vector<int> load_cell_pins;
std::vector<float> force_vals;

void setup() {
  Serial.begin(9600);
  SPI.begin(SCK, MISO, MOSI, CS1);
  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, HIGH);
}

void loop() {
  read_thermos();
  read_load();
  read_pts();
}

void read_pts() {
  const int R = 150;

  for (int i = 0; i < MAX_PTS; i++) {
    int raw_reading = analogRead(PT[i]);
    double reading = (3.3 * (raw_reading / 4096)) / R;
    double pt_value = ((reading - 0.004) / 0.016);

    if (PT_BAR[i] == 5) {
      // capped pressure at 725.19
      pt_vals[i] += 725.19 * pt_value;
    } else if (PT_BAR[i] == 10) {
      // capped pressure at 1450.38
      pt_vals[i] += 1450.38 * pt_value;
    }
  }

  unsigned long time_elapsed = millis();

  if ((time_elapsed - prev_time) > PT_DELAY) {
    Serial.print("PT ");

    for (int i = 0; i < MAX_PTS; i++) {
      Serial.print(" ");
      Serial.print(pt_vals[i] / MAX_PTS);
    }
    Serial.println();

    pt_vals = {0, 0, 0, 0, 0, 0};
    prev_time = time_elapsed;
  }
}

void thermo_driver(std::vector<std::vector<int>> pins) {
  for (int i = 0; i < pins.size() && i < MAX_THERMOS; i++) {
    thermo_pins.push_back(pins[i][0]);
    thermo_vals.push_back(MIN_TEMP);
    
    Adafruit_MAX31856* afmax_thermo = new Adafruit_MAX31856(pins[i][0], pins[i][1], pins[i][2], pins[i][3]);
    afmax_thermo->begin();
    afmax_thermos.push_back(afmax_thermo);
  }
}

void read_thermos() {
  for (int i = 0; i < thermo_pins.size(); i++) {
    thermo_vals[i] = read_sensor(thermo_pins[i]);
  }
}

float get_thermo_value(int pin) {
  int pin_index = -1;
  
  for (int i = 0; i < MAX_THERMOS; i++) {
    if (thermo_pins[i] == pin) {
      pin_index = i;
      break;
    }
  }

  if (pin_index == -1)
    return 0.0;

  return thermo_vals[pin_index];
}

void read_load() {
  for (int i = 0; i < load_cell_pins.size(); i++) {
    force_vals[i] = read_sensor(load_cell_pins[i]);
  }
}

float read_sensor(int pin) {
  int pin_index = -1;

  for (int i = 0; i < MAX_THERMOS; i++) {
    if (thermo_pins[i] == pin) {
      pin_index = i;
      break;
    }
  }
  if (pin_index == -1)
    return 0.0;

  float temperature = afmax_thermos[pin_index]->readThermocoupleTemperature();
  uint8_t fault = afmax_thermos[pin_index]->readFault();
  Serial.println("Thermo " + String(pin_index) + ": " + String(temperature) + " " + String(fault));

  if (fault)
    temperature = 420.0;

  return temperature;
}

float read_pt(int pin) {
  analogRead(pin);
}

float displayPT() {
  for (int i = 0; i < MAX_PTS; i++) {
    Serial.println("PT at pin " + String(PT[i]) + "reads " + String(read_pt(PT[i])));
  }
}
