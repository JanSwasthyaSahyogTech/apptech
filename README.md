# AppTech - Arduino Sensor Projects

Complete Arduino sensor projects with debounce stabilization logic, unit tests, and ready-to-upload .ino files.

## Overview

AppTech is a modular collection of Arduino-based measurement instruments. Each instrument has independent hardware, firmware, and testing components. All instruments use configurable debounce logic to ensure stable, reliable readings.

## Project Structure

```
appTech/
├── instruments/
│   ├── height_meter/
│   │   ├── height_meter.ino              # Ready-to-upload sketch
│   │   ├── src/
│   │   │   └── height_meter.cpp          # C++ implementation
│   │   ├── test/
│   │   │   └── test_height_debouncer.cpp # 15 unit tests
│   │   ├── include/
│   │   │   ├── config.h
│   │   │   └── height_debouncer.h
│   │   └── CIRCUIT_DIAGRAM.md            # Wiring and setup guide
│   │
│   └── pulse_oximeter/
│       ├── pulse_oximeter.ino            # Ready-to-upload sketch
│       ├── src/
│       │   └── pulse_oximeter.cpp        # C++ implementation
│       ├── test/
│       │   └── test_reading_debouncer.cpp # 16 unit tests
│       ├── include/
│       │   └── reading_debouncer.h
│       ├── CIRCUIT_DIAGRAM.md            # Wiring and setup guide
│       └── resources/
│           └── PulseOximeterCircuit.webp # Circuit diagram image
│
├── include/
│   ├── config.h                    # Shared configuration
│   ├── height_debouncer.h          # HeightDebouncer class
│   └── reading_debouncer.h         # Generic ReadingDebouncer template
├── src/
│   ├── height_debouncer.cpp        # HeightDebouncer implementation
│   ├── height_meter.cpp            # Height meter implementation
│   └── pulse_oximeter.cpp          # Pulse oximeter implementation
├── test/
│   ├── test_height_debouncer.cpp   # Height debouncer tests
│   └── test_reading_debouncer.cpp  # BPM/SpO2 debouncer tests
├── CMakeLists.txt                  # CMake build configuration
├── Makefile                        # Make-based build
├── CIRCUIT_DIAGRAM.md              # Overall circuit documentation
└── README.md
```

## Instruments

### 1. Height Meter
**Location:** `instruments/height_meter/`  
**File:** `height_meter.ino`

Measures distance/height using HC-SR04 ultrasonic sensor with debounce stabilization.

**Features:**
- HC-SR04 ultrasonic distance measurement
- Debounce logic for stable readings
- 16x2 I2C LCD display (address 0x27)
- Serial output with stability status
- Arduino Uno/Nano compatible

**Configuration:**
```cpp
#define DEBOUNCE_TOLERANCE_CM 2              // ±2 cm tolerance
#define DEBOUNCE_STABILITY_DURATION_MS 3000  // 3 seconds stability
#define DEBOUNCE_SAMPLE_INTERVAL_MS 100      // 100ms between samples
```

**Hardware:**
- Microcontroller: Arduino Uno/Nano
- Sensor: HC-SR04 Ultrasonic
- Display: 16x2 LCD with I2C backpack (0x27)
- Pins: TRIG=D3, ECHO=D2, SDA=A4, SCL=A5

**Libraries:**
- LiquidCrystal_I2C
- NewPing

**See:** `instruments/height_meter/CIRCUIT_DIAGRAM.md`

### 2. Pulse Oximeter
**Location:** `instruments/pulse_oximeter/`  
**File:** `pulse_oximeter.ino`

Measures heart rate (BPM) and blood oxygen saturation (SpO2) using MAX30100 sensor with independent debounce.

**Features:**
- MAX30100 pulse oximeter sensor
- Separate debounce for BPM (float) and SpO2 (int)
- 16x2 LCD with I2C backpack (address 0x27)
- Validity checks and "Place finger" detection
- Serial output with stability status
- Supports high altitude and critical patients (SpO2 ≥50%)
- Platform-specific optimizations (Arduino Uno, ESP32, ESP8266)

**Configuration:**
```cpp
// BPM Debounce
#define BPM_TOLERANCE 5.0f                   // ±5 BPM tolerance
#define BPM_STABILITY_DURATION_MS 2000       // 2 seconds (Uno) / 3 seconds (ESP)
#define BPM_MIN_VALID 40.0f                  // Minimum valid BPM
#define BPM_MAX_VALID 200.0f                 // Maximum valid BPM

// SpO2 Debounce
#define SPO2_TOLERANCE 2                     // ±2% tolerance
#define SPO2_STABILITY_DURATION_MS 2000      // 2 seconds (Uno) / 3 seconds (ESP)
#define SPO2_MIN_VALID 50                    // Minimum valid SpO2
#define SPO2_MAX_VALID 100                   // Maximum valid SpO2
```

**Hardware:**
- Microcontroller: Arduino Uno, ESP32, or ESP8266
- Sensor: MAX30100 (I2C address 0x57)
- Display: 16x2 LCD with I2C backpack (0x27)
- Arduino Uno: SDA=A4, SCL=A5
- ESP32: SDA=GPIO21, SCL=GPIO22
- ESP8266: SDA=GPIO4, SCL=GPIO5

**Libraries:**
- LiquidCrystal_I2C
- MAX30100_PulseOximeter

**See:** `instruments/pulse_oximeter/CIRCUIT_DIAGRAM.md` and `resources/PulseOximeterCircuit.webp`

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
- **Height Debouncer:** 15/15 tests passed ✓
- **Reading Debouncer:** 16/16 tests passed ✓

## Arduino Deployment

### Height Meter
1. Open `instruments/height_meter/height_meter.ino` in Arduino IDE
2. Select Board: Arduino Uno or Nano
3. Install required libraries:
   - LiquidCrystal_I2C
   - NewPing
4. Upload to board
5. Open Serial Monitor (9600 baud) to view readings

### Pulse Oximeter
1. Open `instruments/pulse_oximeter/pulse_oximeter.ino` in Arduino IDE
2. Select Board: Arduino Uno, ESP32, or ESP8266
3. Install required libraries:
   - LiquidCrystal_I2C
   - MAX30100_PulseOximeter
4. Upload to board
5. Open Serial Monitor (9600 baud for Uno, 115200 for ESP) to view readings

## Debounce Logic

Both projects use configurable debounce mechanisms:

**HeightDebouncer:**
- Works with integer readings (cm)
- Ensures readings are stable within tolerance for specified duration
- Resets stability timer if reading changes significantly
- Continuously updates while maintaining stability
- Used in: Height Meter

**ReadingDebouncer (Template Class):**
- Generic template supporting any numeric type
- Separate instances for BPM (float) and SpO2 (int)
- Validates readings against min/max ranges
- Resets on invalid readings (e.g., finger removed)
- Provides stability status and last valid reading
- Used in: Pulse Oximeter

## Key Features

✓ **Modular Design:** Each instrument is self-contained with its own code, tests, and documentation  
✓ **Platform Support:** Arduino Uno, Nano, ESP32, ESP8266  
✓ **Configurable Debounce:** Adjustable tolerance, stability duration, and sample intervals  
✓ **Comprehensive Testing:** 31 total unit tests covering all debounce scenarios  
✓ **Ready-to-Upload:** Single .ino files for direct Arduino IDE deployment  
✓ **Serial Logging:** Detailed output for debugging and monitoring  
✓ **Display Feedback:** Real-time stability status on LCD displays  
✓ **Edge Case Handling:** Handles no-finger detection, invalid readings, boundary conditions  
✓ **Multi-Platform:** Conditional compilation for platform-specific optimizations  

## Troubleshooting

### LCD Display Not Showing
- Verify I2C address (default 0x27) using I2C scanner
- Check SDA/SCL connections (A4/A5 on Arduino Uno)
- Ensure LCD power supply (3.3V or 5V depending on module)
- Run `test_circuit.ino` to diagnose I2C communication

### Sensor Not Detected
- Verify sensor I2C address using I2C scanner
- Check sensor power supply voltage
- Confirm SDA/SCL connections
- Ensure pull-up resistors (typically 4.7kΩ) on I2C lines

### Unstable Readings
- Adjust debounce parameters (tolerance, stability duration)
- Increase sample interval for slower stabilization
- Check for electrical noise or loose connections
- Verify sensor calibration

## Resources

- **Circuit Diagrams:** See `CIRCUIT_DIAGRAM.md` and instrument-specific guides
- **Hardware Images:** `instruments/pulse_oximeter/resources/PulseOximeterCircuit.webp`
- **Test Files:** `test/` directory contains all unit tests
- **Implementation:** `src/` directory contains C++ implementations
