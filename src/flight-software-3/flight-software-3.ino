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

char packet[128];
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
    float reading = (3.3 * (raw_reading / 4096)) / R;
    float pt_value = ((reading - 0.004) / 0.016);

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

float display_pt() {
  for (int i = 0; i < MAX_PTS; i++) {
    Serial.println("PT at pin " + String(PT[i]) + "reads " + String(read_pt(PT[i])));
  }
}

char* build_sen_packet() {
  const char PACKET_START = '^';
  const char PACKET_END = '$';

  const char PACKET_DELIMITER = '|';
  const char DATA_DELIMITER = ',';

  snprintf(packet, sizeof(packet), "%cSEN%c%lX%c", PACKET_START, PACKET_DELIMITER, millis(), PACKET_DELIMITER);

  // data is of type <sensor_type><sensor_location><...value>
  // : sensor_type ∈ {0, 1, 2} ↦ {"thermocouple", "pressure", "load"}
  // : sensor_location ∈ {1, 2, 3, 4, 5, 6, 7, 8} ↦ {"PT-1", "PT-2", "PT-3", "PT-4", "TC-1", "LC-1", "LC-2", "LC-3"}
  for (int i = 0; i < MAX_PTS; i++) {
    long pt_value = static_cast<long>(pt_vals[i]);
    
    char buffer[17];
    
    buffer[0] = '1';
    buffer[1] = i + 1;

    ltoa(pt_value, buffer + 2, 16);

    size_t buf_len = strlen(buffer);
    buffer[buf_len] = DATA_DELIMITER;
    buffer[buf_len + 1] = '\0';
    
    strcat(packet, buffer);
  }

  // TODO: read force vals and add to packet
  for (int i = 0; i < MAX_THERMOS; i++) {
    long thermo_value = static_cast<long>(thermo_vals[i]);
    
    char buffer[17];

    buffer[0] = '0';
    buffer[1] = MAX_PTS + i + 1;

    ltoa(thermo_value, buffer + 2, 16);

    size_t buf_len = strlen(buffer);
    buffer[buf_len] = DATA_DELIMITER;
    buffer[buf_len + 1] = '\0';
    
    strcat(packet, buffer);
  }
  
  size_t packet_len = strlen(packet);
  // `packet_len - 1` to remove trailing comma
  packet[packet_len - 1] = PACKET_END;
  packet[packet_len] = '\0';

  return packet;
}
