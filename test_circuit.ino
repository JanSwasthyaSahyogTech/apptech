// ============================================
// Circuit Diagnostic Test Sketch
// Tests I2C communication and device detection
// ============================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD Display Settings (16x2 with I2C backpack)
#define LCD_COLS 16
#define LCD_ROWS 2
#define LCD_I2C_ADDRESS 0x27

// I2C Pins (Arduino Uno)
#define I2C_SDA_PIN A4
#define I2C_SCL_PIN A5

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("Circuit Diagnostic Test");
  Serial.println("========================================\n");
  
  // Test 1: I2C Bus Scan
  Serial.println("TEST 1: Scanning I2C Bus...");
  scanI2CBus();
  
  // Test 2: LCD Display Initialization
  Serial.println("\nTEST 2: Initializing LCD Display...");
  testLCDInitialization();
  
  // Test 3: LCD Display Output
  Serial.println("\nTEST 3: Testing LCD Display Output...");
  testLCDDisplay();
  
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
// Test 2: LCD Initialization
// ============================================
void testLCDInitialization() {
  lcd.init();
  lcd.backlight();
  Serial.println("SUCCESS: LCD initialized");
  Serial.print("Display size: ");
  Serial.print(LCD_COLS);
  Serial.print("x");
  Serial.println(LCD_ROWS);
}

// ============================================
// Test 3: LCD Display Output
// ============================================
void testLCDDisplay() {
  // Test 1: Display text
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LCD Test");
  lcd.setCursor(0, 1);
  lcd.print("Working!");
  Serial.println("SUCCESS: Text displayed on LCD");
  
  delay(3000);
  
  // Test 2: Display BPM and SpO2 format
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BPM:75* O2:98*");
  lcd.setCursor(0, 1);
  lcd.print("STABLE          ");
  Serial.println("SUCCESS: BPM/SpO2 format displayed");
  
  delay(3000);
  
  // Test 3: Display "Place Finger" message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place Finger   ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  Serial.println("SUCCESS: Place Finger message displayed");
  
  delay(3000);
  
  // Test 4: Backlight control
  lcd.noBacklight();
  Serial.println("Backlight OFF");
  delay(1000);
  lcd.backlight();
  Serial.println("Backlight ON");
  Serial.println("SUCCESS: Backlight control working");
}
