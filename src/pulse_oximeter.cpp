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
  #ifdef ESP32
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  #elif defined(ESP8266)
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  #else
    Serial.begin(9600);
    Wire.begin();
  #endif

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
    Serial.println("MAX30100 initialization failed");
    for (;;);
  } else {
    Serial.println("MAX30100 initialized");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    unsigned long currentTime = millis();
    
    float rawBpm = pox.getHeartRate();
    uint8_t rawSpo2 = pox.getSpO2();
    
    bpmDebouncer.update(rawBpm, currentTime);
    spo2Debouncer.update((int)rawSpo2, currentTime);
    
    display.clearDisplay();
    
    bool fingerDetected = bpmDebouncer.hasValidReading() || spo2Debouncer.hasValidReading();
    
    if (!fingerDetected && rawBpm == 0 && rawSpo2 == 0) {
      display.setTextSize(1);
      display.setCursor(20, 25);
      display.println("Place finger");
    } else {
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("BPM:");
      
      if (bpmDebouncer.isStable()) {
        display.print((int)bpmDebouncer.getStableReading());
        display.print("*");
      } else if (bpmDebouncer.hasValidReading()) {
        display.print((int)rawBpm);
        display.print("?");
      } else {
        display.print("--");
      }
      
      display.setCursor(70, 0);
      display.print("O2:");
      if (spo2Debouncer.isStable()) {
        display.print(spo2Debouncer.getStableReading());
        display.print("*");
      } else if (spo2Debouncer.hasValidReading()) {
        display.print(rawSpo2);
        display.print("?");
      } else {
        display.print("--");
      }
      
      display.setCursor(0, 15);
      if (bpmDebouncer.isStable() && spo2Debouncer.isStable()) {
        display.println("STABLE");
      } else {
        display.println("Stabilizing");
      }
    }
    
    display.display();
    
    Serial.print("BPM:");
    Serial.print(rawBpm);
    Serial.print("(");
    Serial.print(bpmDebouncer.isStable() ? "OK" : "...");
    Serial.print(") O2:");
    Serial.print(rawSpo2);
    Serial.print("(");
    Serial.print(spo2Debouncer.isStable() ? "OK" : "...");
    Serial.println(")");
    
    tsLastReport = millis();
  }
}
