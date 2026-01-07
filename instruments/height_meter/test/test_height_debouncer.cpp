#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "height_debouncer.h"

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
        throw std::runtime_error("Assertion failed: " #expected " == " #actual \
            " (expected: " + std::to_string(expected) + ", actual: " + std::to_string(actual) + ")"); \
    } \
} while(0)

// ============================================
// Test Cases
// ============================================

TEST(test_constructor_with_params) {
    HeightDebouncer debouncer(5, 2000, 50);
    ASSERT_EQ(5, debouncer.getToleranceCm());
    ASSERT_EQ(2000UL, debouncer.getStabilityDurationMs());
    ASSERT_EQ(50UL, debouncer.getSampleIntervalMs());
}

TEST(test_initial_state) {
    HeightDebouncer debouncer(2, 3000, 100);
    ASSERT_FALSE(debouncer.isStable());
    ASSERT_EQ(-1, debouncer.getStableReading());
    ASSERT_EQ(-1, debouncer.getLastReading());
}

TEST(test_first_reading_not_stable) {
    HeightDebouncer debouncer(2, 3000, 100);
    debouncer.update(100, 0);
    ASSERT_FALSE(debouncer.isStable());
    ASSERT_EQ(100, debouncer.getLastReading());
    ASSERT_EQ(-1, debouncer.getStableReading());
}

TEST(test_stable_after_duration) {
    HeightDebouncer debouncer(2, 1000, 100); // 1 second stability, 100ms interval
    
    // Simulate consistent readings over time
    debouncer.update(100, 0);
    ASSERT_FALSE(debouncer.isStable());
    
    debouncer.update(100, 200);
    ASSERT_FALSE(debouncer.isStable());
    
    debouncer.update(100, 500);
    ASSERT_FALSE(debouncer.isStable());
    
    debouncer.update(100, 800);
    ASSERT_FALSE(debouncer.isStable());
    
    // After 1000ms, should be stable
    debouncer.update(100, 1000);
    ASSERT_TRUE(debouncer.isStable());
    ASSERT_EQ(100, debouncer.getStableReading());
}

TEST(test_readings_within_tolerance_are_stable) {
    HeightDebouncer debouncer(3, 1000, 100); // tolerance of 3cm
    
    debouncer.update(100, 0);
    debouncer.update(101, 200);  // within tolerance
    debouncer.update(99, 400);   // within tolerance
    debouncer.update(102, 600);  // within tolerance
    debouncer.update(100, 800);  // within tolerance
    debouncer.update(101, 1000); // should be stable now
    
    ASSERT_TRUE(debouncer.isStable());
}

TEST(test_reading_outside_tolerance_resets_stability) {
    HeightDebouncer debouncer(2, 1000, 100);
    
    // Build up stability
    debouncer.update(100, 0);
    debouncer.update(100, 200);
    debouncer.update(100, 400);
    debouncer.update(100, 600);
    debouncer.update(100, 800);
    
    // Reading outside tolerance - resets stability
    debouncer.update(110, 900); // 10cm change, outside 2cm tolerance
    ASSERT_FALSE(debouncer.isStable());
    
    // Need to wait another full duration
    debouncer.update(110, 1100);
    ASSERT_FALSE(debouncer.isStable());
    
    debouncer.update(110, 1500);
    ASSERT_FALSE(debouncer.isStable());
    
    debouncer.update(110, 1900);
    ASSERT_TRUE(debouncer.isStable());
    ASSERT_EQ(110, debouncer.getStableReading());
}

TEST(test_sample_interval_respected) {
    HeightDebouncer debouncer(2, 500, 100); // 100ms sample interval
    
    debouncer.update(100, 0);
    debouncer.update(100, 50);  // Too soon, should be ignored
    debouncer.update(100, 80);  // Too soon, should be ignored
    
    // Only 1 valid sample so far, not enough time for stability
    ASSERT_FALSE(debouncer.isStable());
    
    debouncer.update(100, 100); // Valid sample
    debouncer.update(100, 200); // Valid sample
    debouncer.update(100, 300); // Valid sample
    debouncer.update(100, 400); // Valid sample
    debouncer.update(100, 500); // Valid sample - should be stable now
    
    ASSERT_TRUE(debouncer.isStable());
}

TEST(test_reset_clears_state) {
    HeightDebouncer debouncer(2, 500, 100);
    
    // Build up to stable state
    debouncer.update(100, 0);
    debouncer.update(100, 200);
    debouncer.update(100, 400);
    debouncer.update(100, 600);
    ASSERT_TRUE(debouncer.isStable());
    
    // Reset
    debouncer.reset();
    
    ASSERT_FALSE(debouncer.isStable());
    ASSERT_EQ(-1, debouncer.getStableReading());
    ASSERT_EQ(-1, debouncer.getLastReading());
}

TEST(test_zero_reading_handling) {
    HeightDebouncer debouncer(2, 500, 100);
    
    // Zero readings (no object detected)
    debouncer.update(0, 0);
    debouncer.update(0, 200);
    debouncer.update(0, 400);
    debouncer.update(0, 600);
    
    ASSERT_TRUE(debouncer.isStable());
    ASSERT_EQ(0, debouncer.getStableReading());
}

TEST(test_fluctuating_readings_never_stabilize) {
    HeightDebouncer debouncer(2, 1000, 100);
    
    // Readings that keep changing significantly
    debouncer.update(100, 0);
    debouncer.update(110, 200);  // +10, resets
    debouncer.update(95, 400);   // -15, resets
    debouncer.update(105, 600);  // +10, resets
    debouncer.update(90, 800);   // -15, resets
    debouncer.update(100, 1000); // +10, resets
    debouncer.update(85, 1200);  // -15, resets
    
    ASSERT_FALSE(debouncer.isStable());
}

TEST(test_stability_maintained_with_small_variations) {
    HeightDebouncer debouncer(5, 1000, 100); // 5cm tolerance
    
    debouncer.update(100, 0);
    debouncer.update(102, 200);  // +2, within tolerance
    debouncer.update(98, 400);   // -4, within tolerance
    debouncer.update(103, 600);  // +5, within tolerance
    debouncer.update(99, 800);   // -4, within tolerance
    debouncer.update(101, 1000); // +2, within tolerance
    
    ASSERT_TRUE(debouncer.isStable());
}

TEST(test_edge_case_exact_tolerance_boundary) {
    HeightDebouncer debouncer(5, 500, 100);
    
    debouncer.update(100, 0);
    debouncer.update(105, 200);  // Exactly at tolerance boundary (+5)
    debouncer.update(100, 400);  // Back to original (-5)
    debouncer.update(105, 600);  // Exactly at tolerance boundary (+5)
    
    ASSERT_TRUE(debouncer.isStable());
}

TEST(test_edge_case_just_outside_tolerance) {
    HeightDebouncer debouncer(5, 500, 100);
    
    debouncer.update(100, 0);
    debouncer.update(100, 200);
    debouncer.update(100, 400);
    debouncer.update(106, 600);  // Just outside tolerance (+6), resets
    
    ASSERT_FALSE(debouncer.isStable());
}

TEST(test_continuous_update_after_stable) {
    HeightDebouncer debouncer(2, 500, 100);
    
    // Reach stable state
    debouncer.update(100, 0);
    debouncer.update(100, 200);
    debouncer.update(100, 400);
    debouncer.update(100, 600);
    ASSERT_TRUE(debouncer.isStable());
    ASSERT_EQ(100, debouncer.getStableReading());
    
    // Continue updating with same value - should remain stable
    debouncer.update(101, 800);
    ASSERT_TRUE(debouncer.isStable());
    
    debouncer.update(99, 1000);
    ASSERT_TRUE(debouncer.isStable());
    
    // Stable reading updates to latest
    ASSERT_EQ(99, debouncer.getStableReading());
}

TEST(test_large_values) {
    HeightDebouncer debouncer(2, 500, 100);
    
    debouncer.update(199, 0);    // Near max distance
    debouncer.update(200, 200);
    debouncer.update(198, 400);
    debouncer.update(199, 600);
    
    ASSERT_TRUE(debouncer.isStable());
    ASSERT_EQ(199, debouncer.getStableReading());
}

// ============================================
// Main Test Runner
// ============================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "HeightDebouncer Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    RUN_TEST(test_constructor_with_params);
    RUN_TEST(test_initial_state);
    RUN_TEST(test_first_reading_not_stable);
    RUN_TEST(test_stable_after_duration);
    RUN_TEST(test_readings_within_tolerance_are_stable);
    RUN_TEST(test_reading_outside_tolerance_resets_stability);
    RUN_TEST(test_sample_interval_respected);
    RUN_TEST(test_reset_clears_state);
    RUN_TEST(test_zero_reading_handling);
    RUN_TEST(test_fluctuating_readings_never_stabilize);
    RUN_TEST(test_stability_maintained_with_small_variations);
    RUN_TEST(test_edge_case_exact_tolerance_boundary);
    RUN_TEST(test_edge_case_just_outside_tolerance);
    RUN_TEST(test_continuous_update_after_stable);
    RUN_TEST(test_large_values);
    
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << testsPassed << "/" << testsRun << " passed";
    if (testsFailed > 0) {
        std::cout << " (" << testsFailed << " failed)";
    }
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    
    return testsFailed > 0 ? 1 : 0;
}
