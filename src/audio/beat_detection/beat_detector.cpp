#include "beat_detector_simple_energy.h"
#include "beat_detector_spectral_flux_auto.h"
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