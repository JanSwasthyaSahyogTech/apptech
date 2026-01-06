#ifndef HEIGHT_DEBOUNCER_H
#define HEIGHT_DEBOUNCER_H

#include <cstdint>

/**
 * HeightDebouncer - Stabilizes height readings from ultrasonic sensor
 * 
 * Ensures that height measurements are stable within a configurable tolerance
 * for a configurable duration before reporting them as valid stable readings.
 */
class HeightDebouncer {
public:
    /**
     * Constructor with configurable parameters
     * @param toleranceCm - readings within this range are considered equal
     * @param stabilityDurationMs - how long readings must be stable
     * @param sampleIntervalMs - minimum time between samples
     */
    HeightDebouncer(int toleranceCm, unsigned long stabilityDurationMs, unsigned long sampleIntervalMs);

    /**
     * Default constructor using config.h values
     */
    HeightDebouncer();

    /**
     * Update with a new reading
     * @param currentReading - the new height reading in cm
     * @param currentTimeMs - current timestamp in milliseconds
     */
    void update(int currentReading, unsigned long currentTimeMs);

    /**
     * Check if the reading has stabilized
     * @return true if readings have been stable for the required duration
     */
    bool isStable() const;

    /**
     * Get the current stable reading
     * @return the stable height value, or -1 if not yet stable
     */
    int getStableReading() const;

    /**
     * Get the last raw reading
     * @return the most recent reading regardless of stability
     */
    int getLastReading() const;

    /**
     * Get how long the current reading has been stable (in ms)
     * @return duration in milliseconds
     */
    unsigned long getStableDuration() const;

    /**
     * Reset the debouncer state
     */
    void reset();

    // Getters for configuration
    int getToleranceCm() const { return toleranceCm_; }
    unsigned long getStabilityDurationMs() const { return stabilityDurationMs_; }
    unsigned long getSampleIntervalMs() const { return sampleIntervalMs_; }

private:
    // Configuration
    int toleranceCm_;
    unsigned long stabilityDurationMs_;
    unsigned long sampleIntervalMs_;

    // State
    int lastReading_;
    int stableReading_;
    unsigned long stabilityStartTime_;
    unsigned long lastSampleTime_;
    bool isStable_;
    bool hasReading_;

    /**
     * Check if two readings are within tolerance
     */
    bool isWithinTolerance(int reading1, int reading2) const;
};

#endif // HEIGHT_DEBOUNCER_H
