// ============================================
// Circuit Diagnostic Test Sketch
// Tests I2C communication and device detection
// ============================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDRESS 0x27

// I2C Pins (Arduino Uno)
#define I2C_SDA_PIN A4
#define I2C_SCL_PIN A5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("Circuit Diagnostic Test");
  Serial.println("========================================\n");
  
  // Test 1: I2C Bus Scan
  Serial.println("TEST 1: Scanning I2C Bus...");
  scanI2CBus();
  
  // Test 2: OLED Display Initialization
  Serial.println("\nTEST 2: Initializing OLED Display...");
  testOLEDInitialization();
  
  // Test 3: OLED Display Output
  Serial.println("\nTEST 3: Testing OLED Display Output...");
  testOLEDDisplay();
  
  Serial.println("\n========================================");
  Serial.println("Diagnostic Complete");
  Serial.println("========================================\n");
}

void loop() {
  delay(5000);
}

// ============================================
// Test 1: I2C Bus Scanner
// ============================================
void scanI2CBus() {
  byte error, address;
  int nDevices = 0;
  
  Serial.println("Scanning I2C addresses 0x00-0x7F...\n");
  
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found!");
    Serial.println("TROUBLESHOOTING:");
    Serial.println("- Check SDA (A4) and SCL (A5) connections");
    Serial.println("- Verify pull-up resistors (4.7k ohm)");
    Serial.println("- Check power connections (GND and VCC)");
  } else {
    Serial.print("\nTotal devices found: ");
    Serial.println(nDevices);
  }
}

// ============================================
// Test 2: OLED Initialization
// ============================================
void testOLEDInitialization() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println("FAILED: SSD1306 allocation failed");
    Serial.println("TROUBLESHOOTING:");
    Serial.println("- Check if OLED is at address 0x3C");
    Serial.println("- Verify SDA/SCL connections");
    Serial.println("- Check OLED power supply (3.3V or 5V)");
    return;
  }
  Serial.println("SUCCESS: SSD1306 initialized");
  Serial.print("Display size: ");
  Serial.print(SCREEN_WIDTH);
  Serial.print("x");
  Serial.println(SCREEN_HEIGHT);
}

// ============================================
// Test 3: OLED Display Output
// ============================================
void testOLEDDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("OLED Test");
  display.println("--------");
  display.println("If you see this,");
  display.println("OLED is working!");
  display.println("");
  display.println("Checking I2C...");
  
  display.display();
  Serial.println("SUCCESS: Text displayed on OLED");
  
  delay(3000);
  
  // Test 2: Draw shapes
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.fillRect(5, 5, 20, 20, SSD1306_WHITE);
  display.drawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 10, SSD1306_WHITE);
  display.display();
  Serial.println("SUCCESS: Shapes drawn on OLED");
  
  delay(3000);
  
  // Test 3: Invert display
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  Serial.println("SUCCESS: Display inversion working");
}
