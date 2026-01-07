#include "height_debouncer.h"
#include "config.h"
#include <cstdlib>

HeightDebouncer::HeightDebouncer(int toleranceCm, unsigned long stabilityDurationMs, unsigned long sampleIntervalMs)
    : toleranceCm_(toleranceCm)
    , stabilityDurationMs_(stabilityDurationMs)
    , sampleIntervalMs_(sampleIntervalMs)
    , lastReading_(-1)
    , stableReading_(-1)
    , stabilityStartTime_(0)
    , lastSampleTime_(0)
    , isStable_(false)
    , hasReading_(false)
{
}

HeightDebouncer::HeightDebouncer()
    : HeightDebouncer(DEBOUNCE_TOLERANCE_CM, DEBOUNCE_STABILITY_DURATION_MS, DEBOUNCE_SAMPLE_INTERVAL_MS)
{
}

void HeightDebouncer::update(int currentReading, unsigned long currentTimeMs) {
    // Check if enough time has passed since last sample
    if (hasReading_ && (currentTimeMs - lastSampleTime_) < sampleIntervalMs_) {
        return; // Too soon, skip this sample
    }

    lastSampleTime_ = currentTimeMs;

    // Handle first reading
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

bool HeightDebouncer::isStable() const {
    return isStable_;
}

int HeightDebouncer::getStableReading() const {
    return isStable_ ? stableReading_ : -1;
}

int HeightDebouncer::getLastReading() const {
    return lastReading_;
}

unsigned long HeightDebouncer::getStableDuration() const {
    if (!hasReading_) {
        return 0;
    }
    // This would need current time passed in for accurate calculation
    // For now, return 0 if not stable, or stabilityDurationMs_ if stable
    return isStable_ ? stabilityDurationMs_ : 0;
}

void HeightDebouncer::reset() {
    lastReading_ = -1;
    stableReading_ = -1;
    stabilityStartTime_ = 0;
    lastSampleTime_ = 0;
    isStable_ = false;
    hasReading_ = false;
}

bool HeightDebouncer::isWithinTolerance(int reading1, int reading2) const {
    return std::abs(reading1 - reading2) <= toleranceCm_;
}
