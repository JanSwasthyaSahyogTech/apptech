#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NewPing.h>
#include "config.h"
#include "height_debouncer.h"

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS);
NewPing sonar(TRIG_PIN, ECHO_PIN, HEIGHT_MAX_DISTANCE_CM);
HeightDebouncer debouncer;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Height:");
}

void loop() {
  delay(DEBOUNCE_SAMPLE_INTERVAL_MS);
  int distance = sonar.ping_cm();
  unsigned long currentTime = millis();

  debouncer.update(distance, currentTime);

  lcd.setCursor(0, 1);

  if (distance == 0) {
    lcd.print("No object       ");
  } else {
    // Show raw reading
    lcd.print(distance);
    lcd.print(" cm ");
    
    // Indicate stability status
    if (debouncer.isStable()) {
      lcd.print("OK  ");
    } else {
      lcd.print("... ");
    }
  }

  // Serial output with stability info
  Serial.print("Raw: ");
  Serial.print(distance);
  Serial.print(" cm | Stable: ");
  if (debouncer.isStable()) {
    Serial.print("YES (");
    Serial.print(debouncer.getStableReading());
    Serial.println(" cm)");
  } else {
    Serial.println("NO");
  }
}
