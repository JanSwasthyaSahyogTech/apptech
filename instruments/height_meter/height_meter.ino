// ============================================
// Height Meter with Debounce - Arduino Nano
// ============================================
// Ultrasonic sensor (HC-SR04) with I2C LCD display
// Includes debounce logic for stable height readings
// ============================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NewPing.h>

// ============================================
// CONFIGURATION - Adjust these values as needed
// ============================================

// Debounce Settings
#define DEBOUNCE_TOLERANCE_CM 2              // Readings within this range (cm) are considered equal
#define DEBOUNCE_STABILITY_DURATION_MS 3000  // How long readings must be stable (milliseconds)
#define DEBOUNCE_SAMPLE_INTERVAL_MS 100      // Time between readings (milliseconds)

// Ultrasonic Sensor Settings
#define TRIG_PIN 3
#define ECHO_PIN 2
#define HEIGHT_MAX_DISTANCE_CM 200

// I2C LCD Settings
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ============================================
// HeightDebouncer Class
// ============================================

class HeightDebouncer {
public:
    HeightDebouncer(int toleranceCm, unsigned long stabilityDurationMs, unsigned long sampleIntervalMs)
        : toleranceCm_(toleranceCm)
        , stabilityDurationMs_(stabilityDurationMs)
        , sampleIntervalMs_(sampleIntervalMs)
        , lastReading_(-1)
        , stableReading_(-1)
        , stabilityStartTime_(0)
        , lastSampleTime_(0)
        , isStable_(false)
        , hasReading_(false)
    {
    }

    HeightDebouncer()
        : HeightDebouncer(DEBOUNCE_TOLERANCE_CM, DEBOUNCE_STABILITY_DURATION_MS, DEBOUNCE_SAMPLE_INTERVAL_MS)
    {
    }

    void update(int currentReading, unsigned long currentTimeMs) {
        // Check if enough time has passed since last sample
        if (hasReading_ && (currentTimeMs - lastSampleTime_) < sampleIntervalMs_) {
            return; // Too soon, skip this sample
        }

        lastSampleTime_ = currentTimeMs;

        // Handle first reading
        if (!hasReading_) {
            lastReading_ = currentReading;
            stabilityStartTime_ = currentTimeMs;
            hasReading_ = true;
            isStable_ = false;
            return;
        }

        // Check if current reading is within tolerance of last reading
        if (isWithinTolerance(currentReading, lastReading_)) {
            // Reading is consistent, check if we've been stable long enough
            unsigned long stableDuration = currentTimeMs - stabilityStartTime_;
            
            if (stableDuration >= stabilityDurationMs_) {
                isStable_ = true;
                stableReading_ = currentReading;
            }
        } else {
            // Reading changed significantly, reset stability timer
            stabilityStartTime_ = currentTimeMs;
            isStable_ = false;
        }

        lastReading_ = currentReading;
    }

    bool isStable() const {
        return isStable_;
    }

    int getStableReading() const {
        return isStable_ ? stableReading_ : -1;
    }

    int getLastReading() const {
        return lastReading_;
    }

    void reset() {
        lastReading_ = -1;
        stableReading_ = -1;
        stabilityStartTime_ = 0;
        lastSampleTime_ = 0;
        isStable_ = false;
        hasReading_ = false;
    }

private:
    int toleranceCm_;
    unsigned long stabilityDurationMs_;
    unsigned long sampleIntervalMs_;
    int lastReading_;
    int stableReading_;
    unsigned long stabilityStartTime_;
    unsigned long lastSampleTime_;
    bool isStable_;
    bool hasReading_;

    bool isWithinTolerance(int reading1, int reading2) const {
        int diff = reading1 - reading2;
        if (diff < 0) diff = -diff;  // Arduino-compatible abs
        return diff <= toleranceCm_;
    }
};

// ============================================
// Global Objects
// ============================================

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS);
NewPing sonar(TRIG_PIN, ECHO_PIN, HEIGHT_MAX_DISTANCE_CM);
HeightDebouncer debouncer;

// ============================================
// Setup
// ============================================

void setup() {
    Serial.begin(115200);
    
    // Initialize LCD (Arduino Nano uses default I2C pins A4/A5)
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Height:");
}

// ============================================
// Main Loop
// ============================================

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
