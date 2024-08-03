#include "Arduino.h"
#include <vector>

using namespace std;

#include <HX711.h>
#include <Adafruit_MAX31856.h>
#include <SPI.h>

#define MIN_TEMP 0.0
#define SCK 4
#define MISO 19
#define MOSI 23
#define CS1 5
#define CS2 18
//PT's Top --> Bottom on PCB
#define PT_1 12
#define PT_2 14
#define PT_3 33
#define PT_4 32
#define PT_5 35
#define PT_6 34
#define PT_INTVL 500

const int MAX_THERMOS = 3;
int PT_BAR[] = {5, 5, 5, 5, 5, 5};
int PT[] = {PT_1, PT_2, PT_3, PT_4, PT_5, PT_6};
int num_meas = 0;
long tim  = millis();

vector<int> thermo_pins;
vector<float> thermo_vals;
vector<Adafruit_MAX31856*> maxthermos;
vector<float> pt_vals;
vector<int> load_cell_pins;
vector<float> force_vals;

float readPt();
float readSensor(int pin);
void readThermo();
void thermoDriver(vector<vector<int>> pins);
void readLoad();

void setup() {
  Serial.begin(9600);
  spi.begin(SCK, MISO, MOSI, CS1);
  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, HIGH);
}

void loop() {
  readThermo();
  readLoad();
  readPT();

}

void readPT(){
  int R = 150;
  for(int i = 0; i < MAX_PTS; i++){
    int inp  = analogRead(PT[i]);
    double curr =  (3.3*(inp/4096))/R;
     if( PT_BAR[i] == 5){
       //capped pressure 725.19
       pt_vals[i] += 725.19*((curr -.004)/.016);
     }
     if(PT_BAR[i] ==10){
       //capped pressure 1450.38
       pt_vals[i] += 1450.38*((curr -.004)/.016);
     }
     num_meas++;
  }
  if(millis()-tim > PT_INTVL){
    Serial.print("PT ");
    for(int i= 0; i < MAX_PTS; i++){
      Serial.print(" ");
      Serial.print(pt_vals[i]/num_meas);
    }
    Serial.println();
    pt_vals[i] = {0,0 0, 0, 0, 0};
    num_meas = 0;
    tim = millis();
  }
  
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

float readPT(int pin) {
  analogRead(pin);
}

float displayPT() {
  for (int i = 0; i < 6; i++) {
    Serial.println("PT at pin " + myPins[i] + "reads " readPT(myPins[i]));
  }
}
