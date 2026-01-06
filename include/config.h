#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Height Meter Debounce Configuration
// ============================================

// Tolerance: readings within this range (in cm) are considered equal
#define DEBOUNCE_TOLERANCE_CM 2

// Stability duration: how long readings must be stable (in milliseconds)
#define DEBOUNCE_STABILITY_DURATION_MS 3000

// Sample interval: time between readings for stability check (in milliseconds)
#define DEBOUNCE_SAMPLE_INTERVAL_MS 100

// Maximum distance for ultrasonic sensor (in cm)
#define HEIGHT_MAX_DISTANCE_CM 200

// ============================================
// Hardware Pin Configuration
// ============================================

// Ultrasonic sensor pins
#define TRIG_PIN 3
#define ECHO_PIN 2

// I2C LCD address
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ============================================
// Pulse Oximeter Configuration
// ============================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define REPORTING_PERIOD_MS 1000

// I2C pins for ESP8266/ESP32
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

// OLED I2C address
#define OLED_I2C_ADDRESS 0x3C

#endif // CONFIG_H
