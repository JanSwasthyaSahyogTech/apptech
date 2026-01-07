// ============================================
// Pulse Oximeter with Debounce - ESP8266/ESP32
// ============================================
// MAX30100 sensor with OLED display (SSD1306)
// Includes debounce logic for stable BPM and SpO2 readings
// ============================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"

// ============================================
// CONFIGURATION - Adjust these values as needed
// ============================================

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDRESS 0x27

// I2C Pins (Platform-specific)
#ifdef ESP32
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
#elif defined(ESP8266)
  #define I2C_SDA_PIN 4
  #define I2C_SCL_PIN 5
#else
  // Arduino Uno uses fixed I2C pins (A4=SDA, A5=SCL)
  // No need to define custom pins
#endif

// Reporting interval
#define REPORTING_PERIOD_MS 1000

// BPM Debounce Settings (optimized for Arduino Uno)
#ifdef ARDUINO_AVR_UNO
  #define BPM_TOLERANCE 5.0f
  #define BPM_STABILITY_DURATION_MS 2000
  #define BPM_SAMPLE_INTERVAL_MS 200
  #define BPM_MIN_VALID 40.0f
  #define BPM_MAX_VALID 200.0f
#else
  #define BPM_TOLERANCE 5.0f
  #define BPM_STABILITY_DURATION_MS 3000
  #define BPM_SAMPLE_INTERVAL_MS 100
  #define BPM_MIN_VALID 40.0f
  #define BPM_MAX_VALID 200.0f
#endif

// SpO2 Debounce Settings (optimized for Arduino Uno)
#ifdef ARDUINO_AVR_UNO
  #define SPO2_TOLERANCE 2
  #define SPO2_STABILITY_DURATION_MS 2000
  #define SPO2_SAMPLE_INTERVAL_MS 200
  #define SPO2_MIN_VALID 50
  #define SPO2_MAX_VALID 100
#else
  #define SPO2_TOLERANCE 2
  #define SPO2_STABILITY_DURATION_MS 3000
  #define SPO2_SAMPLE_INTERVAL_MS 100
  #define SPO2_MIN_VALID 50
  #define SPO2_MAX_VALID 100
#endif

// ============================================
// ReadingDebouncer Template Class
// ============================================

template<typename T>
class ReadingDebouncer {
public:
    ReadingDebouncer(T tolerance, unsigned long stabilityDurationMs, unsigned long sampleIntervalMs,
                     T minValid, T maxValid)
        : tolerance_(tolerance)
        , stabilityDurationMs_(stabilityDurationMs)
        , sampleIntervalMs_(sampleIntervalMs)
        , minValid_(minValid)
        , maxValid_(maxValid)
        , lastReading_(T())
        , stableReading_(T())
        , stabilityStartTime_(0)
        , lastSampleTime_(0)
        , isStable_(false)
        , hasReading_(false)
        , lastReadingValid_(false)
    {
    }

    void update(T currentReading, unsigned long currentTimeMs) {
        if (hasReading_ && (currentTimeMs - lastSampleTime_) < sampleIntervalMs_) {
            return;
        }

        lastSampleTime_ = currentTimeMs;

        bool isValid = isValidReading(currentReading);
        lastReadingValid_ = isValid;

        if (!isValid) {
            reset();
            return;
        }

        if (!hasReading_) {
            lastReading_ = currentReading;
            stabilityStartTime_ = currentTimeMs;
            hasReading_ = true;
            isStable_ = false;
            return;
        }

        if (isWithinTolerance(currentReading, lastReading_)) {
            unsigned long stableDuration = currentTimeMs - stabilityStartTime_;
            
            if (stableDuration >= stabilityDurationMs_) {
                isStable_ = true;
                stableReading_ = currentReading;
            }
        } else {
            stabilityStartTime_ = currentTimeMs;
            isStable_ = false;
        }

        lastReading_ = currentReading;
    }

    bool isStable() const { return isStable_; }
    T getStableReading() const { return isStable_ ? stableReading_ : T(); }
    T getLastReading() const { return lastReading_; }
    bool isLastReadingValid() const { return lastReadingValid_; }
    bool hasValidReading() const { return hasReading_; }

    void reset() {
        lastReading_ = T();
        stableReading_ = T();
        stabilityStartTime_ = 0;
        lastSampleTime_ = 0;
        isStable_ = false;
        hasReading_ = false;
        lastReadingValid_ = false;
    }

private:
    T tolerance_;
    unsigned long stabilityDurationMs_;
    unsigned long sampleIntervalMs_;
    T minValid_;
    T maxValid_;
    T lastReading_;
    T stableReading_;
    unsigned long stabilityStartTime_;
    unsigned long lastSampleTime_;
    bool isStable_;
    bool hasReading_;
    bool lastReadingValid_;

    bool isValidReading(T reading) const {
        return reading >= minValid_ && reading <= maxValid_;
    }

    bool isWithinTolerance(T reading1, T reading2) const {
        T diff = reading1 - reading2;
        if (diff < T()) diff = -diff;
        return diff <= tolerance_;
    }
};

// ============================================
// Global Objects
// ============================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
PulseOximeter pox;
uint32_t tsLastReport = 0;

// Debouncers for BPM and SpO2
ReadingDebouncer<float> bpmDebouncer(BPM_TOLERANCE, BPM_STABILITY_DURATION_MS, 
                                      BPM_SAMPLE_INTERVAL_MS, BPM_MIN_VALID, BPM_MAX_VALID);
ReadingDebouncer<int> spo2Debouncer(SPO2_TOLERANCE, SPO2_STABILITY_DURATION_MS,
                                     SPO2_SAMPLE_INTERVAL_MS, SPO2_MIN_VALID, SPO2_MAX_VALID);

// ============================================
// Callbacks
// ============================================

void onBeatDetected() {
    Serial.println("Beat!");
}

// ============================================
// Setup
// ============================================

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

    // Initialize OLED
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

    // Initialize pulse oximeter
    if (!pox.begin()) {
        Serial.println("MAX30100 initialization failed");
        for (;;);
    } else {
        Serial.println("MAX30100 initialized");
    }
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
}

// ============================================
// Main Loop
// ============================================

void loop() {
    pox.update();
    
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        unsigned long currentTime = millis();
        
        // Get raw readings
        float rawBpm = pox.getHeartRate();
        uint8_t rawSpo2 = pox.getSpO2();
        
        // Update debouncers
        bpmDebouncer.update(rawBpm, currentTime);
        spo2Debouncer.update((int)rawSpo2, currentTime);
        
        // Clear display
        display.clearDisplay();
        
        // Check if finger is detected
        bool fingerDetected = bpmDebouncer.hasValidReading() || spo2Debouncer.hasValidReading();
        
        if (!fingerDetected && rawBpm == 0 && rawSpo2 == 0) {
            // No finger detected - minimal display
            display.setTextSize(1);
            display.setCursor(20, 25);
            display.println("Place finger");
        } else {
            // Compact display format for Arduino Uno
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
            
            // SpO2 on same line
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
            
            // Status indicator
            display.setCursor(0, 15);
            if (bpmDebouncer.isStable() && spo2Debouncer.isStable()) {
                display.println("STABLE");
            } else {
                display.println("Stabilizing");
            }
        }
        
        display.display();
        
        // Serial output (minimal for Arduino Uno)
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
