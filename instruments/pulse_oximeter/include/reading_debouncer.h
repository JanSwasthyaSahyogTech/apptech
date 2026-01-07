#ifndef READING_DEBOUNCER_H
#define READING_DEBOUNCER_H

#include <cmath>

/**
 * ReadingDebouncer - Generic debouncer for sensor readings
 * 
 * Template class that works with int, float, or other numeric types.
 * Ensures readings are stable within a configurable tolerance
 * for a configurable duration before reporting them as valid.
 */
template<typename T>
class ReadingDebouncer {
public:
    /**
     * Constructor with configurable parameters
     * @param tolerance - readings within this range are considered equal
     * @param stabilityDurationMs - how long readings must be stable
     * @param sampleIntervalMs - minimum time between samples
     * @param minValid - minimum valid reading (below this is invalid)
     * @param maxValid - maximum valid reading (above this is invalid)
     */
    ReadingDebouncer(T tolerance, unsigned long stabilityDurationMs, unsigned long sampleIntervalMs,
                     T minValid, T maxValid)
        : tolerance_(tolerance)
        , stabilityDurationMs_(stabilityDurationMs)
        , sampleIntervalMs_(sampleIntervalMs)
        , minValid_(minValid)
        , maxValid_(maxValid)
        , lastReading_(T())
        , stableReading_(T())
        , stabilityStartTime_(0)
        , lastSampleTime_(0)
        , isStable_(false)
        , hasReading_(false)
        , lastReadingValid_(false)
    {
    }

    /**
     * Update with a new reading
     * @param currentReading - the new reading
     * @param currentTimeMs - current timestamp in milliseconds
     */
    void update(T currentReading, unsigned long currentTimeMs) {
        // Check if enough time has passed since last sample
        if (hasReading_ && (currentTimeMs - lastSampleTime_) < sampleIntervalMs_) {
            return; // Too soon, skip this sample
        }

        lastSampleTime_ = currentTimeMs;

        // Check validity
        bool isValid = isValidReading(currentReading);
        lastReadingValid_ = isValid;

        if (!isValid) {
            // Invalid reading resets stability
            reset();
            return;
        }

        // Handle first valid reading
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

    /**
     * Check if the reading has stabilized
     */
    bool isStable() const {
        return isStable_;
    }

    /**
     * Get the current stable reading
     * @return the stable value, or default T() if not yet stable
     */
    T getStableReading() const {
        return isStable_ ? stableReading_ : T();
    }

    /**
     * Get the last raw reading
     */
    T getLastReading() const {
        return lastReading_;
    }

    /**
     * Check if the last reading was valid
     */
    bool isLastReadingValid() const {
        return lastReadingValid_;
    }

    /**
     * Check if we have any valid reading
     */
    bool hasValidReading() const {
        return hasReading_;
    }

    /**
     * Reset the debouncer state
     */
    void reset() {
        lastReading_ = T();
        stableReading_ = T();
        stabilityStartTime_ = 0;
        lastSampleTime_ = 0;
        isStable_ = false;
        hasReading_ = false;
        lastReadingValid_ = false;
    }

    // Getters for configuration
    T getTolerance() const { return tolerance_; }
    unsigned long getStabilityDurationMs() const { return stabilityDurationMs_; }
    unsigned long getSampleIntervalMs() const { return sampleIntervalMs_; }
    T getMinValid() const { return minValid_; }
    T getMaxValid() const { return maxValid_; }

private:
    // Configuration
    T tolerance_;
    unsigned long stabilityDurationMs_;
    unsigned long sampleIntervalMs_;
    T minValid_;
    T maxValid_;

    // State
    T lastReading_;
    T stableReading_;
    unsigned long stabilityStartTime_;
    unsigned long lastSampleTime_;
    bool isStable_;
    bool hasReading_;
    bool lastReadingValid_;

    /**
     * Check if reading is within valid range
     */
    bool isValidReading(T reading) const {
        return reading >= minValid_ && reading <= maxValid_;
    }

    /**
     * Check if two readings are within tolerance
     */
    bool isWithinTolerance(T reading1, T reading2) const {
        T diff = reading1 - reading2;
        if (diff < T()) diff = -diff;  // abs
        return diff <= tolerance_;
    }
};

#endif // READING_DEBOUNCER_H
