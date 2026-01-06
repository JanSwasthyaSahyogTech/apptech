# AppTech - Arduino Sensor Projects

Complete Arduino sensor projects with debounce stabilization logic, unit tests, and ready-to-upload .ino files.

## Project Structure

```
appTech/
├── include/
│   ├── config.h                    # Configuration constants (height meter)
│   ├── height_debouncer.h          # HeightDebouncer class for ultrasonic sensor
│   └── reading_debouncer.h         # Generic ReadingDebouncer template (BPM, SpO2)
├── src/
│   ├── height_debouncer.cpp        # HeightDebouncer implementation
│   ├── height_meter.cpp            # Height meter sketch (Arduino Nano)
│   └── pulse_oximeter.cpp          # Pulse oximeter sketch (ESP8266/ESP32)
├── test/
│   ├── test_height_debouncer.cpp   # 15 unit tests for height debouncer
│   └── test_reading_debouncer.cpp  # 16 unit tests for BPM/SpO2 debouncer
├── height_meter.ino                # Single .ino file for Arduino Nano upload
├── pulse_oximeter.ino              # Single .ino file for ESP8266/ESP32 upload
├── CMakeLists.txt                  # CMake build configuration
├── Makefile                        # Simple make-based build
└── README.md
```

## Projects

### 1. Height Meter (Arduino Nano)
**File:** `height_meter.ino`

Measures distance/height using HC-SR04 ultrasonic sensor with debounce stabilization.

**Features:**
- Ultrasonic distance measurement
- Debounce logic ensures stable readings
- 16x2 I2C LCD display
- Serial output with stability status

**Configuration (in .ino file):**
```cpp
#define DEBOUNCE_TOLERANCE_CM 2              // ±2 cm tolerance
#define DEBOUNCE_STABILITY_DURATION_MS 3000  // 3 seconds stability required
#define DEBOUNCE_SAMPLE_INTERVAL_MS 100      // 100ms between samples
```

**Hardware Connections:**
| Component | Arduino Nano Pin |
|-----------|------------------|
| HC-SR04 TRIG | D3 |
| HC-SR04 ECHO | D2 |
| LCD SDA | A4 |
| LCD SCL | A5 |

**Required Libraries:**
- LiquidCrystal_I2C
- NewPing

### 2. Pulse Oximeter (ESP8266/ESP32)
**File:** `pulse_oximeter.ino`

Measures heart rate (BPM) and blood oxygen saturation (SpO2) using MAX30100 sensor with independent debounce for each reading.

**Features:**
- MAX30100 pulse oximeter sensor
- Separate debounce for BPM (float) and SpO2 (int)
- OLED display (SSD1306) with stability indicators
- Validity checks and "Place finger" detection
- Serial output with stability status
- Supports high altitude and critical patients (SpO2 down to 50%)

**Configuration (in .ino file):**
```cpp
// BPM Debounce
#define BPM_TOLERANCE 5.0f                   // ±5 BPM tolerance
#define BPM_STABILITY_DURATION_MS 3000       // 3 seconds stability
#define BPM_MIN_VALID 40.0f                  // Minimum valid BPM
#define BPM_MAX_VALID 200.0f                 // Maximum valid BPM

// SpO2 Debounce
#define SPO2_TOLERANCE 2                     // ±2% tolerance
#define SPO2_STABILITY_DURATION_MS 3000      // 3 seconds stability
#define SPO2_MIN_VALID 50                    // Minimum valid SpO2 (supports high altitude/critical)
#define SPO2_MAX_VALID 100                   // Maximum valid SpO2
```

**Required Libraries:**
- Adafruit_GFX
- Adafruit_SSD1306
- MAX30100_PulseOximeter

## Building and Testing

### Using Makefile (Recommended)
```bash
cd <to_project_root>

# Run all tests
make test

# Clean build artifacts
make clean
```

### Using g++ directly
```bash
# Test height debouncer
g++ -std=c++11 -I include src/height_debouncer.cpp test/test_height_debouncer.cpp -o test_height_debouncer
./test_height_debouncer

# Test reading debouncer (BPM/SpO2)
g++ -std=c++11 -I include test/test_reading_debouncer.cpp -o test_reading_debouncer
./test_reading_debouncer
```

### Test Results
- **Height Debouncer:** 15/15 tests passed
- **Reading Debouncer:** 16/16 tests passed (includes BPM float and SpO2 int tests)

## Arduino Deployment

### Height Meter
1. Open `height_meter.ino` in Arduino IDE
2. Select Board: Arduino Nano
3. Install required libraries (LiquidCrystal_I2C, NewPing)
4. Upload to board

### Pulse Oximeter
1. Open `pulse_oximeter.ino` in Arduino IDE
2. Select Board: ESP8266 or ESP32 (depending on your board)
3. Install required libraries (Adafruit_GFX, Adafruit_SSD1306, MAX30100_PulseOximeter)
4. Upload to board

## Debounce Logic

Both projects use configurable debounce mechanisms:

**HeightDebouncer (height_meter.ino):**
- Works with integer readings (cm)
- Ensures readings are stable within tolerance for specified duration
- Resets stability timer if reading changes significantly
- Continuously updates while maintaining stability

**ReadingDebouncer (pulse_oximeter.ino):**
- Generic template class supporting any numeric type
- Separate instances for BPM (float) and SpO2 (int)
- Validates readings against min/max ranges
- Resets on invalid readings (e.g., finger removed)
- Provides stability status and last valid reading

## Key Features

✓ Platform-independent debounce logic (testable without hardware)
✓ Configurable tolerance, stability duration, and sample intervals
✓ Comprehensive unit tests for all debounce scenarios
✓ Single .ino files ready for Arduino upload
✓ Serial output for debugging and monitoring
✓ Display feedback showing stability status
✓ Handles edge cases (no finger, invalid readings, boundary conditions)
