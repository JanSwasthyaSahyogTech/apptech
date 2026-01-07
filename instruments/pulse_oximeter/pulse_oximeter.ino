// ============================================
// Pulse Oximeter with Debounce - ESP8266/ESP32
// ============================================
// MAX30100 sensor with LCD or OLED display
// Includes debounce logic for stable BPM and SpO2 readings
// ============================================

#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

// ============================================
// DISPLAY TYPE AUTO-DETECTION
// ============================================
// Display type is automatically detected via I2C scan:
// - LCD (16x2) at address 0x27
// - OLED (128x64) at address 0x3C

#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================
// CONFIGURATION - Adjust these values as needed
// ============================================

// Display Settings
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Display type enum
enum DisplayType {
  DISPLAY_NONE,
  DISPLAY_LCD,
  DISPLAY_OLED
};

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

LiquidCrystal_I2C lcdDisplay(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS);
Adafruit_SSD1306 oledDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

PulseOximeter pox;
uint32_t tsLastReport = 0;
DisplayType displayType = DISPLAY_NONE;
bool displayInitialized = false;

// Helper functions for display abstraction
void displayInit();
void displayClear();
void displaySetCursor(uint8_t col, uint8_t row);
void displayPrint(const char* text);
void displayPrintInt(int value);
void displayUpdate();
void displayBacklight(bool on);

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
// Display Detection and Abstraction Layer
// ============================================

DisplayType detectDisplayType() {
    Serial.println("\nDetecting display type via I2C scan...");
    
    Wire.beginTransmission(LCD_I2C_ADDRESS);
    if (Wire.endTransmission() == 0) {
        Serial.print("Found device at 0x");
        Serial.println(LCD_I2C_ADDRESS, HEX);
        Serial.println("Display Type: LCD (16x2)");
        return DISPLAY_LCD;
    }
    
    Wire.beginTransmission(OLED_I2C_ADDRESS);
    if (Wire.endTransmission() == 0) {
        Serial.print("Found device at 0x");
        Serial.println(OLED_I2C_ADDRESS, HEX);
        Serial.println("Display Type: OLED (128x64)");
        return DISPLAY_OLED;
    }
    
    Serial.println("No display detected at 0x27 or 0x3C");
    return DISPLAY_NONE;
}

void displayInit() {
    if (displayType == DISPLAY_LCD) {
        lcdDisplay.init();
        lcdDisplay.backlight();
        displayInitialized = true;
        Serial.println("SUCCESS: LCD initialized");
    } else if (displayType == DISPLAY_OLED) {
        if (oledDisplay.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
            displayInitialized = true;
            Serial.println("SUCCESS: OLED initialized");
        } else {
            Serial.println("WARNING: OLED initialization failed");
        }
    }
}

void displayClear() {
    if (!displayInitialized) return;
    if (displayType == DISPLAY_LCD) {
        lcdDisplay.clear();
    } else if (displayType == DISPLAY_OLED) {
        oledDisplay.clearDisplay();
    }
}

void displaySetCursor(uint8_t col, uint8_t row) {
    if (!displayInitialized) return;
    if (displayType == DISPLAY_LCD) {
        lcdDisplay.setCursor(col, row);
    } else if (displayType == DISPLAY_OLED) {
        oledDisplay.setCursor(col, row * 8);
    }
}

void displayPrint(const char* text) {
    if (!displayInitialized) return;
    if (displayType == DISPLAY_LCD) {
        lcdDisplay.print(text);
    } else if (displayType == DISPLAY_OLED) {
        oledDisplay.print(text);
    }
}

void displayPrintInt(int value) {
    if (!displayInitialized) return;
    if (displayType == DISPLAY_LCD) {
        lcdDisplay.print(value);
    } else if (displayType == DISPLAY_OLED) {
        oledDisplay.print(value);
    }
}

void displayUpdate() {
    if (!displayInitialized) return;
    if (displayType == DISPLAY_OLED) {
        oledDisplay.display();
    }
}

void displayBacklight(bool on) {
    if (!displayInitialized) return;
    if (displayType == DISPLAY_LCD) {
        if (on) {
            lcdDisplay.backlight();
        } else {
            lcdDisplay.noBacklight();
        }
    }
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

    delay(1000);
    Serial.println("\n========================================");
    Serial.println("Pulse Oximeter Initialization");
    Serial.println("========================================");
    
    // Log I2C configuration
    #ifdef ESP32
        Serial.println("Board: ESP32");
        Serial.print("I2C SDA: GPIO ");
        Serial.println(I2C_SDA_PIN);
        Serial.print("I2C SCL: GPIO ");
        Serial.println(I2C_SCL_PIN);
    #elif defined(ESP8266)
        Serial.println("Board: ESP8266");
        Serial.print("I2C SDA: GPIO ");
        Serial.println(I2C_SDA_PIN);
        Serial.print("I2C SCL: GPIO ");
        Serial.println(I2C_SCL_PIN);
    #else
        Serial.println("Board: Arduino Uno");
        Serial.println("I2C SDA: A4");
        Serial.println("I2C SCL: A5");
    #endif
    Serial.println();

    // Auto-detect display type
    displayType = detectDisplayType();
    
    // Initialize detected display
    if (displayType != DISPLAY_NONE) {
        Serial.println("Initializing display...");
        displayInit();
        
        if (displayInitialized) {
            displayClear();
            displaySetCursor(0, 0);
            displayPrint("Pulse Oximeter");
            displaySetCursor(0, 1);
            displayPrint("Initializing...");
            displayUpdate();
            Serial.println("Display: Splash screen shown");
            delay(2000);
        }
    } else {
        Serial.println("WARNING: No display detected");
        Serial.println("Continuing without display...");
    }
    Serial.println();

    // Initialize pulse oximeter
    Serial.println("\nInitializing MAX30100 sensor...");
    if (!pox.begin()) {
        Serial.println("ERROR: MAX30100 initialization failed!");
        Serial.println("Troubleshooting:");
        Serial.println("- Check I2C address (expected 0x57)");
        Serial.println("- Verify SDA/SCL connections");
        Serial.println("- Check sensor power supply (3.3V)");
        for (;;);
    }
    Serial.println("SUCCESS: MAX30100 initialized");
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    
    Serial.println("\n========================================");
    Serial.println("Initialization Complete - Ready to measure");
    Serial.println("========================================\n");
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
        
        // Log raw sensor readings
        Serial.print("[");
        Serial.print(currentTime);
        Serial.print("ms] RAW - BPM:");
        Serial.print(rawBpm);
        Serial.print(" SpO2:");
        Serial.print(rawSpo2);
        Serial.println("%");
        
        // Update debouncers
        bpmDebouncer.update(rawBpm, currentTime);
        spo2Debouncer.update((int)rawSpo2, currentTime);
        
        // Log debouncer status
        Serial.print("      DEBOUNCE - BPM:");
        Serial.print(bpmDebouncer.hasValidReading() ? "valid" : "invalid");
        Serial.print(" SpO2:");
        Serial.print(spo2Debouncer.hasValidReading() ? "valid" : "invalid");
        Serial.println();
        
        // Update Display if initialized
        if (displayInitialized) {
            bool fingerDetected = bpmDebouncer.hasValidReading() || spo2Debouncer.hasValidReading();
            
            displayClear();
            
            if (!fingerDetected && rawBpm == 0 && rawSpo2 == 0) {
                displaySetCursor(0, 0);
                displayPrint("Place Finger   ");
                displaySetCursor(0, 1);
                displayPrint("                ");
                Serial.println("      DISPLAY: 'Place Finger'");
            } else {
                displaySetCursor(0, 0);
                displayPrint("BPM:");
                if (bpmDebouncer.isStable()) {
                    displayPrintInt((int)bpmDebouncer.getStableReading());
                    displayPrint("* ");
                } else if (bpmDebouncer.hasValidReading()) {
                    displayPrintInt((int)rawBpm);
                    displayPrint("? ");
                } else {
                    displayPrint("-- ");
                }
                displayPrint("O2:");
                if (spo2Debouncer.isStable()) {
                    displayPrintInt(spo2Debouncer.getStableReading());
                    displayPrint("*");
                } else if (spo2Debouncer.hasValidReading()) {
                    displayPrintInt(rawSpo2);
                    displayPrint("?");
                } else {
                    displayPrint("--");
                }
                displaySetCursor(0, 1);
                if (bpmDebouncer.isStable() && spo2Debouncer.isStable()) {
                    displayPrint("STABLE          ");
                    Serial.println("      DISPLAY: Readings STABLE");
                } else {
                    displayPrint("Stabilizing...  ");
                    Serial.println("      DISPLAY: Stabilizing...");
                }
            }
            displayUpdate();
            Serial.println("      Display: Updated");
        } else {
            Serial.println("      Display: Skipped (not initialized)");
        }
        
        // Serial output with full details
        Serial.print("      OUTPUT - BPM:");
        Serial.print(rawBpm);
        Serial.print("(");
        Serial.print(bpmDebouncer.isStable() ? "OK" : "...");
        Serial.print(") O2:");
        Serial.print(rawSpo2);
        Serial.print("(");
        Serial.print(spo2Debouncer.isStable() ? "OK" : "...");
        Serial.println(")");
        Serial.println();
        
        tsLastReport = millis();
    }
}
