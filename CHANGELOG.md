# Changelog

All notable changes to Listeningway will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2025-06-02

### Added
- **Stereo Spatialization Support**: New uniforms for enhanced stereo audio effects
  - `Listeningway_VolumeLeft`: Volume level for left audio channels (0.0 to 1.0)
  - `Listeningway_VolumeRight`: Volume level for right audio channels (0.0 to 1.0)  
  - `Listeningway_AudioPan`: Stereo pan position (-1.0 = full left, 0.0 = center, +1.0 = full right)
- **Audio Format Detection**: New uniform `Listeningway_AudioFormat` 
  - Detects audio format: 0.0=none, 1.0=mono, 2.0=stereo, 6.0=5.1 surround, 8.0=7.1 surround
  - Enables format-specific shader effects and optimizations
- **Pan Smoothing Control**: Configurable smoothing to reduce pan calculation jitter
  - Setting: `PanSmoothing` in Listeningway.ini (0.0 = no smoothing, higher = more smoothing)
  - Default: 0.0 (preserves current behavior)
  - Accessible via overlay UI for real-time adjustment

### Enhanced
- **Surround Sound Support**: Improved pan calculation for 5.1 and 7.1 audio
  - Smart detection of effectively stereo content in surround formats
  - Full [-1, +1] pan range for stereo content, even in surround sound modes
- **Overlay Interface**: Added real-time display of all spatialization metrics
  - Shows left/right volumes, pan position, and detected audio format
  - Aligned display with existing volume and beat indicators

### Technical
- Extended `AudioAnalysisData` structure with new spatialization fields
- Improved pan calculation algorithm with surround sound awareness
- Added exponential moving average smoothing for pan stability
- Updated uniform manager to expose all new audio spatialization uniforms

## [1.0.x] - Previous Versions

### Features
- Real-time audio analysis with FFT processing
- Beat detection with multiple algorithms (Simple Energy, Spectral Flux + Autocorrelation)
- 32-band frequency analysis with logarithmic/linear mapping options
- 5-band equalizer system for enhanced frequency visualization
- Time-based uniforms for continuous animations
- Comprehensive settings system with overlay UI
- Support for System Audio and Process Audio capture
- Thread-safe audio provider switching
