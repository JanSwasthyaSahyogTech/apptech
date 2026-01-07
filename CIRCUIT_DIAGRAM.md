# AppTech - Circuit Diagrams

Complete circuit diagrams and wiring guides for all AppTech instruments.

## Overview

All AppTech instruments use I2C communication for sensors and displays. This document provides wiring information for each instrument.

## Instruments

- **Height Meter:** HC-SR04 ultrasonic sensor + 16x2 LCD display
- **Pulse Oximeter:** MAX30100 pulse sensor + 16x2 LCD Display with I2C Backpack

LCD Display (16x2 with I2C backpack)
├── VCC ────────── 5V or 3.3V (depends on backpack)
├── GND ────────── GND
├── SDA ────────── Microcontroller SDA
└── SCL ────────── Microcontroller SCL

**I2C Address:** 0x27 (default, may vary by module - use I2C scanner to verify)

---

# Pulse Oximeter Circuit Diagram

## Components Required

1. **Microcontroller** (choose one):
   - Arduino Uno
   - ESP32
   - ESP8266

2. **Sensors & Display**:
   - MAX30100 Pulse Oximeter Sensor (I2C address: 0x57)
   - 16x2 LCD Display with I2C backpack (address: 0x27)

3. **Power & Connections**:
   - USB Power or 5V Power Supply
   - Jumper Wires
   - I2C Pull-up Resistors (optional, usually built-in)