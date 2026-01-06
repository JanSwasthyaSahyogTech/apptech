#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"
#include "config.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
PulseOximeter pox;
uint32_t tsLastReport = 0;

void onBeatDetected() {
  Serial.println("Beat!!!");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println("OLED initialization failed");
    for (;;);
  }
  display.clearDisplay();
  
  display.setTextSize(2);  
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("Pulse");
  display.setCursor(10, 30);
  display.println("Oximeter");
  display.display();
  delay(2000);

  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    display.clearDisplay();
    
    display.setTextSize(2);  
    display.setCursor(10, 10);
    display.print("BPM:");

    display.setTextSize(1);
    display.setCursor(75, 15);
    display.print(pox.getHeartRate());

    display.setTextSize(2);
    display.setCursor(10, 40);
    display.print("SpO2:");

    display.setTextSize(1);
    display.setCursor(75, 45);
    display.print(pox.getSpO2());
    display.print("%");

    display.display();
    tsLastReport = millis();
  }
}
