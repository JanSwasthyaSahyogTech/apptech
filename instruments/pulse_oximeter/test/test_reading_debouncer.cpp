#include <iostream>
#include <cassert>
#include <string>
#include <cmath>
#include "reading_debouncer.h"

// ============================================
// Platform-Specific Parameters
// ============================================
// Arduino Uno: BPM stability=2000ms, interval=200ms; SpO2 stability=2000ms, interval=200ms
// ESP32/ESP8266: BPM stability=3000ms, interval=100ms; SpO2 stability=3000ms, interval=100ms
// Tests use Arduino Uno optimized parameters for compatibility
// Display: 16x2 LCD with I2C backpack (address 0x27)
// Sensor: MAX30100 pulse oximeter (address 0x57)

// Simple test framework
int testsRun = 0;
int testsPassed = 0;
int testsFailed = 0;

#define TEST(name) void name()
#define RUN_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    testsRun++; \
    try { \
        name(); \
        testsPassed++; \
        std::cout << "PASSED" << std::endl; \
    } catch (const std::exception& e) { \
        testsFailed++; \
        std::cout << "FAILED: " << e.what() << std::endl; \
    } \
} while(0)

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    } \
} while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        throw std::runtime_error("Assertion failed: " #expected " == " #actual); \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(expected, actual, epsilon) do { \
    if (std::fabs((expected) - (actual)) > (epsilon)) { \
        throw std::runtime_error("Assertion failed: " #expected " ~= " #actual); \
    } \
} while(0)

// ============================================
// BPM (float) Tests
// ============================================

TEST(test_bpm_initial_state) {
    // BPM: tolerance=5, stability=2000ms (Uno) or 3000ms (ESP), interval=200ms (Uno) or 100ms (ESP), valid range 40-200
    ReadingDebouncer<float> bpmDebouncer(5.0f, 2000, 200, 40.0f, 200.0f);
    ASSERT_FALSE(bpmDebouncer.isStable());
    ASSERT_FALSE(bpmDebouncer.hasValidReading());
    ASSERT_FLOAT_EQ(0.0f, bpmDebouncer.getStableReading(), 0.01f);
}

TEST(test_bpm_invalid_reading_below_range) {
    ReadingDebouncer<float> bpmDebouncer(5.0f, 1000, 100, 40.0f, 200.0f);
    
    // BPM of 0 (no finger) should be invalid
    bpmDebouncer.update(0.0f, 0);
    ASSERT_FALSE(bpmDebouncer.hasValidReading());
    ASSERT_FALSE(bpmDebouncer.isLastReadingValid());
    
    // BPM of 30 (too low) should be invalid
    bpmDebouncer.update(30.0f, 200);
    ASSERT_FALSE(bpmDebouncer.hasValidReading());
}

TEST(test_bpm_invalid_reading_above_range) {
    ReadingDebouncer<float> bpmDebouncer(5.0f, 1000, 100, 40.0f, 200.0f);
    
    // BPM of 250 (too high) should be invalid
    bpmDebouncer.update(250.0f, 0);
    ASSERT_FALSE(bpmDebouncer.hasValidReading());
    ASSERT_FALSE(bpmDebouncer.isLastReadingValid());
}

TEST(test_bpm_stable_after_duration) {
    ReadingDebouncer<float> bpmDebouncer(5.0f, 1000, 100, 40.0f, 200.0f);
    
    bpmDebouncer.update(72.0f, 0);
    ASSERT_TRUE(bpmDebouncer.hasValidReading());
    ASSERT_FALSE(bpmDebouncer.isStable());
    
    bpmDebouncer.update(73.0f, 200);  // within tolerance
    bpmDebouncer.update(71.0f, 400);  // within tolerance
    bpmDebouncer.update(72.0f, 600);  // within tolerance
    bpmDebouncer.update(74.0f, 800);  // within tolerance
    ASSERT_FALSE(bpmDebouncer.isStable());
    
    bpmDebouncer.update(72.0f, 1000); // should be stable now
    ASSERT_TRUE(bpmDebouncer.isStable());
    ASSERT_FLOAT_EQ(72.0f, bpmDebouncer.getStableReading(), 0.01f);
}

TEST(test_bpm_fluctuating_readings_reset_stability) {
    ReadingDebouncer<float> bpmDebouncer(5.0f, 1000, 100, 40.0f, 200.0f);
    
    bpmDebouncer.update(72.0f, 0);
    bpmDebouncer.update(73.0f, 200);
    bpmDebouncer.update(74.0f, 400);
    bpmDebouncer.update(90.0f, 600);  // Big jump, resets stability
    ASSERT_FALSE(bpmDebouncer.isStable());
    
    // Need to wait full duration again
    bpmDebouncer.update(90.0f, 800);
    bpmDebouncer.update(91.0f, 1000);
    bpmDebouncer.update(89.0f, 1200);
    bpmDebouncer.update(90.0f, 1400);
    bpmDebouncer.update(90.0f, 1600);
    ASSERT_TRUE(bpmDebouncer.isStable());
    ASSERT_FLOAT_EQ(90.0f, bpmDebouncer.getStableReading(), 0.01f);
}

TEST(test_bpm_invalid_reading_resets_state) {
    ReadingDebouncer<float> bpmDebouncer(5.0f, 1000, 100, 40.0f, 200.0f);
    
    // Build up stability
    bpmDebouncer.update(72.0f, 0);
    bpmDebouncer.update(72.0f, 200);
    bpmDebouncer.update(72.0f, 400);
    bpmDebouncer.update(72.0f, 600);
    bpmDebouncer.update(72.0f, 800);
    bpmDebouncer.update(72.0f, 1000);
    ASSERT_TRUE(bpmDebouncer.isStable());
    
    // Invalid reading (finger removed) resets everything
    bpmDebouncer.update(0.0f, 1200);
    ASSERT_FALSE(bpmDebouncer.isStable());
    ASSERT_FALSE(bpmDebouncer.hasValidReading());
}

// ============================================
// SpO2 (int) Tests
// ============================================

TEST(test_spo2_initial_state) {
    // SpO2: tolerance=2, stability=2000ms (Uno) or 3000ms (ESP), interval=200ms (Uno) or 100ms (ESP), valid range 50-100
    ReadingDebouncer<int> spo2Debouncer(2, 2000, 200, 50, 100);
    ASSERT_FALSE(spo2Debouncer.isStable());
    ASSERT_FALSE(spo2Debouncer.hasValidReading());
}

TEST(test_spo2_invalid_reading_below_range) {
    ReadingDebouncer<int> spo2Debouncer(2, 1000, 100, 50, 100);
    
    // SpO2 of 0 (no finger) should be invalid
    spo2Debouncer.update(0, 0);
    ASSERT_FALSE(spo2Debouncer.hasValidReading());
    
    // SpO2 of 40 (too low) should be invalid
    spo2Debouncer.update(40, 200);
    ASSERT_FALSE(spo2Debouncer.hasValidReading());
}

TEST(test_spo2_stable_after_duration) {
    ReadingDebouncer<int> spo2Debouncer(2, 1000, 100, 50, 100);
    
    spo2Debouncer.update(98, 0);
    ASSERT_TRUE(spo2Debouncer.hasValidReading());
    ASSERT_FALSE(spo2Debouncer.isStable());
    
    spo2Debouncer.update(97, 200);  // within tolerance
    spo2Debouncer.update(99, 400);  // within tolerance
    spo2Debouncer.update(98, 600);  // within tolerance
    spo2Debouncer.update(98, 800);  // within tolerance
    ASSERT_FALSE(spo2Debouncer.isStable());
    
    spo2Debouncer.update(98, 1000); // should be stable now
    ASSERT_TRUE(spo2Debouncer.isStable());
    ASSERT_EQ(98, spo2Debouncer.getStableReading());
}

TEST(test_spo2_fluctuating_readings) {
    ReadingDebouncer<int> spo2Debouncer(2, 1000, 100, 50, 100);
    
    spo2Debouncer.update(98, 0);
    spo2Debouncer.update(98, 200);
    spo2Debouncer.update(92, 400);  // Big drop, resets stability
    ASSERT_FALSE(spo2Debouncer.isStable());
    
    spo2Debouncer.update(92, 600);
    spo2Debouncer.update(93, 800);
    spo2Debouncer.update(92, 1000);
    spo2Debouncer.update(92, 1200);
    spo2Debouncer.update(91, 1400);
    ASSERT_TRUE(spo2Debouncer.isStable());
}

TEST(test_spo2_boundary_valid_values) {
    ReadingDebouncer<int> spo2Debouncer(2, 500, 100, 50, 100);
    
    // Test lower boundary (50)
    spo2Debouncer.update(50, 0);
    ASSERT_TRUE(spo2Debouncer.hasValidReading());
    ASSERT_TRUE(spo2Debouncer.isLastReadingValid());
    
    spo2Debouncer.reset();
    
    // Test upper boundary (100)
    spo2Debouncer.update(100, 0);
    ASSERT_TRUE(spo2Debouncer.hasValidReading());
    ASSERT_TRUE(spo2Debouncer.isLastReadingValid());
}

// ============================================
// Sample Interval Tests
// ============================================

TEST(test_sample_interval_respected) {
    ReadingDebouncer<int> debouncer(2, 500, 200, 50, 100);
    
    debouncer.update(98, 0);
    debouncer.update(98, 50);   // Too soon, ignored
    debouncer.update(98, 80);   // Too soon, ignored
    
    // Only 1 valid sample, not enough for stability
    ASSERT_FALSE(debouncer.isStable());
    
    debouncer.update(98, 200);  // Valid
    debouncer.update(98, 400);  // Valid
    debouncer.update(98, 600);  // Valid
    debouncer.update(98, 800);  // Valid
    debouncer.update(98, 1000); // Valid - stable now
    
    ASSERT_TRUE(debouncer.isStable());
}

// ============================================
// Reset Tests
// ============================================

TEST(test_reset_clears_all_state) {
    ReadingDebouncer<float> debouncer(5.0f, 500, 200, 40.0f, 200.0f);
    
    // Build to stable
    debouncer.update(72.0f, 0);
    debouncer.update(72.0f, 200);
    debouncer.update(72.0f, 400);
    debouncer.update(72.0f, 600);
    ASSERT_TRUE(debouncer.isStable());
    
    debouncer.reset();
    
    ASSERT_FALSE(debouncer.isStable());
    ASSERT_FALSE(debouncer.hasValidReading());
    ASSERT_FLOAT_EQ(0.0f, debouncer.getStableReading(), 0.01f);
}

// ============================================
// Continuous Update Tests
// ============================================

TEST(test_continuous_update_maintains_stability) {
    ReadingDebouncer<int> debouncer(2, 500, 200, 50, 100);
    
    // Reach stable
    debouncer.update(98, 0);
    debouncer.update(98, 200);
    debouncer.update(98, 400);
    debouncer.update(98, 600);
    ASSERT_TRUE(debouncer.isStable());
    
    // Continue updating - should stay stable
    debouncer.update(97, 800);
    ASSERT_TRUE(debouncer.isStable());
    
    debouncer.update(99, 1000);
    ASSERT_TRUE(debouncer.isStable());
    
    // Stable reading updates to latest
    ASSERT_EQ(99, debouncer.getStableReading());
}

TEST(test_edge_tolerance_boundary) {
    ReadingDebouncer<int> debouncer(5, 500, 200, 50, 100);
    
    debouncer.update(95, 0);
    debouncer.update(100, 200);  // Exactly at tolerance (+5)
    debouncer.update(95, 400);   // Back (-5)
    debouncer.update(100, 600);  // Exactly at tolerance
    
    ASSERT_TRUE(debouncer.isStable());
}

TEST(test_just_outside_tolerance) {
    ReadingDebouncer<int> debouncer(5, 500, 200, 50, 100);
    
    debouncer.update(90, 0);
    debouncer.update(90, 200);
    debouncer.update(90, 400);
    debouncer.update(96, 600);  // Just outside tolerance (+6), resets
    
    ASSERT_FALSE(debouncer.isStable());
}

// ============================================
// Main Test Runner
// ============================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "ReadingDebouncer Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // BPM (float) tests
    std::cout << "\n--- BPM (float) Tests ---" << std::endl;
    RUN_TEST(test_bpm_initial_state);
    RUN_TEST(test_bpm_invalid_reading_below_range);
    RUN_TEST(test_bpm_invalid_reading_above_range);
    RUN_TEST(test_bpm_stable_after_duration);
    RUN_TEST(test_bpm_fluctuating_readings_reset_stability);
    RUN_TEST(test_bpm_invalid_reading_resets_state);
    
    // SpO2 (int) tests
    std::cout << "\n--- SpO2 (int) Tests ---" << std::endl;
    RUN_TEST(test_spo2_initial_state);
    RUN_TEST(test_spo2_invalid_reading_below_range);
    RUN_TEST(test_spo2_stable_after_duration);
    RUN_TEST(test_spo2_fluctuating_readings);
    RUN_TEST(test_spo2_boundary_valid_values);
    
    // General tests
    std::cout << "\n--- General Tests ---" << std::endl;
    RUN_TEST(test_sample_interval_respected);
    RUN_TEST(test_reset_clears_all_state);
    RUN_TEST(test_continuous_update_maintains_stability);
    RUN_TEST(test_edge_tolerance_boundary);
    RUN_TEST(test_just_outside_tolerance);
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << testsPassed << "/" << testsRun << " passed";
    if (testsFailed > 0) {
        std::cout << " (" << testsFailed << " failed)";
    }
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    
    return testsFailed > 0 ? 1 : 0;
}
