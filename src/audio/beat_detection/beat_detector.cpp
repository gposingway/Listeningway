#include "beat_detector.h"
#include "simple_energy_beat_detector.h"
#include "spectral_flux_auto_beat_detector.h"
#include "logging.h"
#include <unordered_map>
#include <functional>

std::unique_ptr<IBeatDetector> IBeatDetector::Create(int algorithm) {
    // Use a static map of algorithm code to factory lambdas for maintainability
    static const std::unordered_map<int, std::function<std::unique_ptr<IBeatDetector>()>> factory_map = {
        {0, []() {
            LOG_DEBUG("[BeatDetector] Creating SimpleEnergyBeatDetector");
            return std::make_unique<SimpleEnergyBeatDetector>();
        }},
        {1, []() {
            LOG_DEBUG("[BeatDetector] Creating SpectralFluxAutoBeatDetector");
            return std::make_unique<SpectralFluxAutoBeatDetector>();
        }}
    };
    auto it = factory_map.find(algorithm);
    if (it != factory_map.end()) {
        return it->second();
    } else {
        LOG_ERROR("[BeatDetector] Unknown algorithm: " + std::to_string(algorithm) + ", falling back to SimpleEnergyBeatDetector");
        return std::make_unique<SimpleEnergyBeatDetector>();
    }
}