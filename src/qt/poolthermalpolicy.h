// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_QT_POOLTHERMALPOLICY_H
#define AGRARIAN_QT_POOLTHERMALPOLICY_H

#include <algorithm>
#include <cmath>

namespace poolthermal {
constexpr int kCpuDefault = 75;
constexpr int kGpuDefault = 70;
constexpr int kCpuMin = 50, kCpuMax = 85;
constexpr int kGpuMin = 45, kGpuMax = 80;
constexpr int kHotspotOffset = 10;
enum class State { Normal, Throttle, Cooling, Unavailable, LowLoad };

struct Load {
    State state{State::Unavailable};
    unsigned threads{0}; // also gates GPU dispatch
    int delayMs{0};
};

// Delta is the warmest sensor relative to its own target. Missing readings
// can run only in explicit fallback mode; an observed overheat latches until
// valid cool readings return, even when fallback was enabled.
class Policy {
public:
    Load Sample(double delta, unsigned threads, bool fallback)
    {
        if (!std::isfinite(delta)) {
            coolSamples = 0;
            return fallback && !latched ? Load{State::LowLoad, 1, 250}
                                        : Load{State::Unavailable, 0, 0};
        }
        if (delta >= 0) {
            latched = true;
            coolSamples = 0;
        }
        if (latched) {
            if (delta <= -5) ++coolSamples;
            else coolSamples = 0;
            if (coolSamples < 5) return {State::Cooling, 0, 0};
            latched = false;
            coolSamples = 0;
            ramp = 1;
        }
        if (ramp > 0 && ramp <= 3) {
            const auto active = std::max(1u, threads * ramp++ / 4);
            return {State::Throttle, active, 100};
        }
        if (delta >= -5) {
            const auto half = std::max(1u, threads / 2);
            return {State::Throttle, half, 65};
        }
        ramp = 0;
        return {State::Normal, threads, 0};
    }

private:
    bool latched{false};
    unsigned coolSamples{0}, ramp{0};
};
} // namespace poolthermal
#endif
