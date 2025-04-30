#include "beat_detector.h"
#include "simple_energy_beat_detector.h"
#include "spectral_flux_auto_beat_detector.h"
#include "logging.h"

std::unique_ptr<IBeatDetector> IBeatDetector::Create(int algorithm) {
    switch (algorithm) {
        case 0:
            LOG_DEBUG("[BeatDetector] Creating SimpleEnergyBeatDetector");
            return std::make_unique<SimpleEnergyBeatDetector>();
        case 1:
            LOG_DEBUG("[BeatDetector] Creating SpectralFluxAutoBeatDetector");
            return std::make_unique<SpectralFluxAutoBeatDetector>();
        default:
            LOG_ERROR("[BeatDetector] Unknown algorithm: " + std::to_string(algorithm) + ", falling back to SimpleEnergyBeatDetector");
            return std::make_unique<SimpleEnergyBeatDetector>();
    }
}