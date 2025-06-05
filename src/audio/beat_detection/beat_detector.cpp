#include "beat_detector_simple_energy.h"
#include "beat_detector_spectral_flux_auto.h"
#include "logging.h"

std::unique_ptr<IBeatDetector> IBeatDetector::Create(int algorithm) {
    switch (algorithm) {
        case 0:
            LOG_DEBUG("[BeatDetector] Creating BeatDetectorSimpleEnergy");
            return std::make_unique<BeatDetectorSimpleEnergy>();
        case 1:
            LOG_DEBUG("[BeatDetector] Creating BeatDetectorSpectralFluxAuto");
            return std::make_unique<BeatDetectorSpectralFluxAuto>();
        default:
            LOG_ERROR("[BeatDetector] Unknown algorithm: " + std::to_string(algorithm) + ", falling back to BeatDetectorSimpleEnergy");
            return std::make_unique<BeatDetectorSimpleEnergy>();
    }
}