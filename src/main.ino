/*
  Embedded Voltage Protection System
  Author: Abeer Mehra

  Description:
  Monitors input voltage using ADC and disconnects the load
  during over-voltage or under-voltage conditions.
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int sensorPin = A0;
const int relayPin  = 8;

float voltageFactor = 740.0;

const float UNDER_TRIP_ON  = 185.0;
const float UNDER_TRIP_OFF = 190.0;

const float OVER_TRIP_ON   = 235.0;
const float OVER_TRIP_OFF  = 230.0;

const unsigned long MEASURE_TIME = 200;
const unsigned long MIN_RELAY_INTERVAL = 2000;

enum StatusType { STATUS_NORMAL, STATUS_UNDER, STATUS_OVER };
StatusType status = STATUS_NORMAL;

bool relayOn = true;
unsigned long lastRelayChange = 0;

float readVoltageRMS() {
  unsigned long startTime = millis();

  double sumV = 0.0;
  double sumV2 = 0.0;
  long count = 0;

  while (millis() - startTime < MEASURE_TIME) {
    int adc = analogRead(sensorPin);
    double v = adc * (5.0 / 1023.0);

    sumV += v;
    sumV2 += v * v;
    count++;
  }

  if (count == 0) return 0;

  double mean  = sumV / count;
  double mean2 = sumV2 / count;

  double vRmsPin = sqrt(fmax(0.0, mean2 - mean * mean));
  return vRmsPin * voltageFactor;
}

void applyRelay() {
  digitalWrite(relayPin, relayOn ? LOW : HIGH);
}

void setup() {
  pinMode(relayPin, OUTPUT);

  relayOn = true;
  applyRelay();
  lastRelayChange = millis();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Under/Over Volt");
  lcd.setCursor(0, 1);
  lcd.print("Protection Sys");
  delay(2000);
}

void loop() {
  float Vrms = readVoltageRMS();
  unsigned long now = millis();

  switch (status) {
    case STATUS_NORMAL:
      if (Vrms < UNDER_TRIP_ON) status = STATUS_UNDER;
      else if (Vrms > OVER_TRIP_ON) status = STATUS_OVER;
      break;

    case STATUS_UNDER:
      if (Vrms > UNDER_TRIP_OFF) status = STATUS_NORMAL;
      break;

    case STATUS_OVER:
      if (Vrms < OVER_TRIP_OFF) status = STATUS_NORMAL;
      break;
  }

  bool desiredRelayOn = (status == STATUS_NORMAL);

  if (desiredRelayOn != relayOn) {
    if (now - lastRelayChange >= MIN_RELAY_INTERVAL) {
      relayOn = desiredRelayOn;
      applyRelay();
      lastRelayChange = now;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V: ");
  lcd.print(Vrms, 1);
  lcd.print(" VAC");

  lcd.setCursor(0, 1);
  if (status == STATUS_UNDER)      lcd.print("UNDER VOLTAGE   ");
  else if (status == STATUS_OVER) lcd.print("OVER VOLTAGE    ");
  else                            lcd.print("STATUS: NORMAL  ");

  delay(200);
}

