#include "Arduino.h"
#include <vector> 

using namespace std;  

#include <HX711.h>
#include <Adafruit_MAX31856.h>

#define MIN_TEMP 0.0  

const int MAX_THERMOS = 3;  

vector<int> thermo_pins;
vector<float> thermo_vals;
vector<Adafruit_MAX31856*> maxthermos;

vector<int> load_cell_pins;
vector<float> force_vals;

float readSensor(int pin);
void readThermo();
void thermoDriver(vector<vector<int>> pins);
void readLoad();

void setup() {
  Serial.begin(9600);
}

void loop() {
  readThermo();
  readLoad();
}

void thermoDriver(vector<vector<int>> pins) {
  for (int i = 0; i < pins.size() && i < MAX_THERMOS; i++) {
    thermo_pins.push_back(pins[i][0]);
    thermo_vals.push_back(MIN_TEMP);
    Adafruit_MAX31856* maxthermo = new Adafruit_MAX31856(pins[i][0], pins[i][1], pins[i][2], pins[i][3]);
    maxthermo->begin();
    maxthermos.push_back(maxthermo);
  }
}

void readThermo() {
  for (int i = 0; i < thermo_pins.size(); i++) {
    thermo_vals[i] = readSensor(thermo_pins[i]);
  }
}

float getThermoValue(int pin) {
  int id = -1;
  for (int i = 0; i < thermo_pins.size(); i++) {
    if (thermo_pins[i] == pin) {
      id = i;
      break;
    }
  }
  if (id == -1) {
    return 0.0;
  }
  return thermo_vals[id];
}

void readLoad() {
  for (unsigned int i = 0; i < load_cell_pins.size(); i++) {
    force_vals[i] = readSensor(load_cell_pins[i]);
  }
}

float readSensor(int pin) {
  int id = -1;
  for (int i = 0; i < thermo_pins.size(); i++) {
    if (thermo_pins[i] == pin) {
      id = i;
      break;
    }
  }
  if (id == -1) {
    return 0.0;
  }

  float ret = maxthermos[id]->readThermocoupleTemperature();
  uint8_t fault = maxthermos[id]->readFault();
  Serial.println("Thermo " + to_string(id) + ": " + to_string(ret) + " " + to_string(fault));

  if (fault) {
    ret = 420.0;  
  }

  return ret;
}
