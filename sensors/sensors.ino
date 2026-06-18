#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_VL53L0X.h>

// ---------------- Pins ----------------
#define TRIG_PIN 18
#define ECHO_PIN 19
#define LED_PIN 4

// ---------------- Settings ----------------
const float threshold = 20.0;  // cm

// ---------------- Objects ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change to 0x3F if needed
Adafruit_VL53L0X lox;

// ----------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  if (!lox.begin()) {
    lcd.clear();
    lcd.print("VL53L0X Error");
    while (1);
  }

  lcd.clear();
}

// ----------------------------------------
float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
    return -1;

  return duration * 0.0343 / 2.0;
}

// ----------------------------------------
float readToF() {
  VL53L0X_RangingMeasurementData_t measure;

  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {  // Valid reading
    return measure.RangeMilliMeter / 10.0;
  } else {
    return -1;
  }
}

// ----------------------------------------
void loop() {

  float us = readUltrasonic();
  float tof = readToF();

  // LED logic
  if ((us > 0 && us <= threshold) || (tof > 0 && tof <= threshold)) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // LCD
  lcd.setCursor(0, 0);
  lcd.print("US: ");
  if (us < 0)
    lcd.print("--");
  else
    lcd.print(us, 0);
  lcd.print("cm   ");

  lcd.setCursor(0, 1);
  lcd.print("ToF:");
  if (tof < 0)
    lcd.print("--.-");
  else
    lcd.print(tof, 1);
  lcd.print("cm  ");

  // Serial (optional)
  Serial.print("US: ");
  Serial.print(us);
  Serial.print(" cm\tToF: ");
  Serial.print(tof);
  Serial.println(" cm");

  delay(1000);
}
