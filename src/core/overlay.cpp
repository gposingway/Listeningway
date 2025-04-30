// ---------------------------------------------
// Overlay Module Implementation
// Provides a debug ImGui overlay for real-time audio analysis data
// ---------------------------------------------
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define ImTextureID ImU64
#include <imgui.h>
#include <reshade.hpp>
#include "overlay.h"
#include "audio_analysis.h"
#include "constants.h"
#include "settings.h"
#include "logging.h"
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <chrono>
#include <cmath>

extern std::atomic_bool g_audio_analysis_enabled;
extern bool g_listeningway_debug_enabled;

// Helper: Draw toggles (audio analysis, debug logging)
static void DrawToggles() {
    bool enabled = g_settings.audio_analysis_enabled;
    if (ImGui::Checkbox("Enable Audio Analysis", &enabled)) {
        SetAudioAnalysisEnabled(enabled);
        g_settings.audio_analysis_enabled.store(enabled);
        LOG_DEBUG(std::string("[Overlay] Audio Analysis toggled ") + (enabled ? "ON" : "OFF"));
    }
    
    // Use the global debug flag directly, then synchronize with g_settings through SetDebugEnabled
    bool debug_enabled = g_listeningway_debug_enabled;
    if (ImGui::Checkbox("Enable Debug Logging", &debug_enabled)) {
        SetDebugEnabled(debug_enabled);
        LOG_DEBUG(std::string("[Overlay] Debug Logging toggled ") + (debug_enabled ? "ON" : "OFF"));
    }
}

// Helper: Draw log file info
static void DrawLogInfo() {
    if (g_listeningway_debug_enabled) {
        ImGui::Text("Log file: ");
        ImGui::SameLine();
        std::string logPath = GetLogFilePath();
        if (ImGui::Selectable(logPath.c_str())) {
            ShellExecuteA(nullptr, "open", logPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        ImGui::Text("(Click to open log file)");
    }
}

// Helper: Draw website link
static void DrawWebsite() {
    ImGui::Text("Website:");
    ImGui::SameLine();
    if (ImGui::Selectable("https://github.com/gposingway/Listeningway")) {
        ShellExecuteA(nullptr, "open", "https://github.com/gposingway/Listeningway", nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
}

// Helper: Draw volume meter
static void DrawVolume(const AudioAnalysisData& data) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Volume:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.volume, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
}

// Helper: Draw beat meter
static void DrawBeat(const AudioAnalysisData& data) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Beat:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.beat, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
}

// Helper: Draw frequency bands with view mode
static void DrawFrequencyBands(const AudioAnalysisData& data) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Frequency Bands");
    
    size_t band_count = data.freq_bands.size();
    
    // Always use compact visualization with thin horizontal bars stacked vertically
    // Calculate total height for all bars with no spacing
    float barHeight = 6.0f; // Thin bar height
    float totalHeight = barHeight * band_count;
    
    // Create a child window to contain all the bars - removed scrollbars
    ImGui::BeginChild("FreqBandsCompact", ImVec2(0, totalHeight + 15), true, ImGuiWindowFlags_NoScrollbar);
    
    // Get starting cursor position for drawing bars
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // Draw each frequency band as a thin bar
    for (size_t i = 0; i < band_count; ++i) {
        float value = data.freq_bands[i];
        
        // Calculate bar coordinates - each bar directly below the previous one
        ImVec2 barStart(startPos.x, startPos.y + i * barHeight);
        ImVec2 barEnd(startPos.x + value * windowSize.x, barStart.y + barHeight);
        
        // Draw filled bar
        ImGui::GetWindowDrawList()->AddRectFilled(
            barStart,
            barEnd,
            ImGui::GetColorU32(IM_COL32(25 + 230 * (1.0f - (float)i / band_count), // Red gradient (high to low frequencies)
                                      25 + 230 * ((float)i / band_count),         // Green gradient (low to high frequencies)
                                      230,                                        // Constant blue
                                      255)),                                       // Alpha
            0.0f  // No rounding
        );
        
        // Draw background/outline for the full bar area
        ImGui::GetWindowDrawList()->AddRect(
            barStart,
            ImVec2(startPos.x + windowSize.x, barStart.y + barHeight),
            ImGui::GetColorU32(IM_COL32(60, 60, 60, 128)), // Dark gray
            0.0f  // No rounding
        );
    }
    
    ImGui::Dummy(ImVec2(0, totalHeight)); // Reserve space for our custom drawing
    ImGui::EndChild();
}

// Helper: Draw time/phase info
static void DrawTimePhaseInfo() {
    // Calculate time since start (same as in listeningway_addon.cpp)
    static const auto start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - start_time;
    float time_seconds = elapsed.count();
    float phase_60hz = std::fmod(time_seconds * 60.0f, 1.0f);
    float phase_120hz = std::fmod(time_seconds * 120.0f, 1.0f);
    float total_phases_60hz = time_seconds * 60.0f;
    float total_phases_120hz = time_seconds * 120.0f;
    ImGui::Text("Time/Phase Uniforms:");
    ImGui::Text("  Seconds: %.3f", time_seconds);
    ImGui::Text("  Phase 60Hz: %.3f", phase_60hz);
    ImGui::Text("  Phase 120Hz: %.3f", phase_120hz);
    ImGui::Text("  Total 60Hz cycles: %.1f", total_phases_60hz);
    ImGui::Text("  Total 120Hz cycles: %.1f", total_phases_120hz);
}

// Helper: Draw Beat Detection Algorithm settings
static void DrawBeatDetectionAlgorithm(const AudioAnalysisData& data) {
    ImGui::Text("Beat Detection Algorithm:");
    
    // Create a combo box for algorithm selection
    const char* algorithms[] = { "Simple Energy (Original)", "Spectral Flux + Autocorrelation (Advanced)" };
    int algorithm = g_settings.beat_detection_algorithm;
    if (ImGui::Combo("Algorithm", &g_settings.beat_detection_algorithm, algorithms, IM_ARRAYSIZE(algorithms))) {
        LOG_DEBUG(std::string("[Overlay] Beat Detection Algorithm changed to ") + 
                 (g_settings.beat_detection_algorithm == 0 ? "Simple Energy" : "Spectral Flux + Autocorrelation"));
        // Update the audio analyzer with the new algorithm
        g_audio_analyzer.SetBeatDetectionAlgorithm(g_settings.beat_detection_algorithm);
    }
    
    if (ImGui::IsItemHovered(-1)) {
        if (g_settings.beat_detection_algorithm == 0) {
            ImGui::SetTooltip("Simple Energy: Works well with strong bass beats");
        } else {
            ImGui::SetTooltip("Advanced: Better for complex rhythms and various music genres");
        }
    }
    
    // Show advanced settings for the Spectral Flux + Autocorrelation algorithm
    if (g_settings.beat_detection_algorithm == 1) {
        // Create a collapsing section for advanced parameters
        if (ImGui::CollapsingHeader("Advanced Algorithm Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Spectral Flux threshold adjustment
            ImGui::SliderFloat("##SpectralFluxThreshold", &g_settings.spectral_flux_threshold, 0.01f, 0.2f, "%.3f");
            ImGui::SameLine();
            ImGui::Text("Spectral Flux Threshold");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Lower value = more sensitive to subtle changes");
            }
            
            // Tempo change threshold adjustment
            ImGui::SliderFloat("##TempoChangeThreshold", &g_settings.tempo_change_threshold, 0.1f, 0.5f, "%.2f");
            ImGui::SameLine();
            ImGui::Text("Tempo Change Threshold");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Higher value = more stable tempo, lower = adapts faster");
            }
            
            // Beat induction window adjustment
            ImGui::SliderFloat("##BeatInductionWindow", &g_settings.beat_induction_window, 0.05f, 0.2f, "%.2f");
            ImGui::SameLine();
            ImGui::Text("Beat Induction Window");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Larger window = more adaptive phase adjustment");
            }
            
            // Octave error weight adjustment
            ImGui::SliderFloat("##OctaveErrorWeight", &g_settings.octave_error_weight, 0.5f, 0.9f, "%.2f");
            ImGui::SameLine();
            ImGui::Text("Octave Error Weight");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Higher values favor base tempo vs half/double detection");
            }
            
            // Display current tempo info directly (no tree node)
            if (data.tempo_detected) {
                ImGui::Text("Current Tempo: %.1f BPM (Confidence: %.2f)", data.tempo_bpm, data.tempo_confidence);
                ImGui::Text("Beat Phase: %.2f", data.beat_phase);
            } else {
                ImGui::Text("No tempo detected yet");
            }
        }
    }
}

// Helper: Draw Beat Decay Settings for both algorithms
static void DrawBeatDecaySettings() {
    ImGui::Text("Beat Decay Settings:");
    
    // Different settings based on which algorithm is selected
    if (g_settings.beat_detection_algorithm == 0) { // SimpleEnergy
        // SimpleEnergy decay settings
        ImGui::SliderFloat("##DefaultFalloffRate", &g_settings.beat_falloff_default, 0.5f, 5.0f, "%.2f");
        ImGui::SameLine();
        ImGui::Text("Default Falloff Rate");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Controls how quickly the beat indicator fades out\nHigher values = faster decay");
        }
        
        // More advanced adaptive settings
        if (ImGui::CollapsingHeader("Adaptive Falloff Settings")) {
            ImGui::SliderFloat("##TimeScale", &g_settings.beat_time_scale, 1e-10f, 1e-8f, "%.2e");
            ImGui::SameLine();
            ImGui::Text("Time Scale");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls time scaling for beat interval");
            }
            
            ImGui::SliderFloat("##InitialTime", &g_settings.beat_time_initial, 0.1f, 1.0f, "%.2f");
            ImGui::SameLine();
            ImGui::Text("Initial Time");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls initial time since last beat");
            }
            
            ImGui::SliderFloat("##MinTime", &g_settings.beat_time_min, 0.01f, 0.5f, "%.2f");
            ImGui::SameLine();
            ImGui::Text("Min Time");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls minimum time for adaptive falloff");
            }
            
            ImGui::SliderFloat("##TimeDivisor", &g_settings.beat_time_divisor, 0.01f, 1.0f, "%.2f");
            ImGui::SameLine();
            ImGui::Text("Time Divisor");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls divisor for adaptive falloff\nThese settings control how decay adapts to beat timing");
            }
        }
    } else { // SpectralFluxAuto
        // SpectralFluxAuto decay settings
        ImGui::SliderFloat("##DecayMultiplier", &g_settings.spectral_flux_decay_multiplier, 0.5f, 5.0f, "%.2f");
        ImGui::SameLine();
        ImGui::Text("Decay Multiplier");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Controls how quickly the beat indicator fades out\nHigher values = faster decay relative to tempo\nThis algorithm automatically adapts to music tempo");
        }
    }
}

// Draws the Listeningway debug overlay using ImGui.
// Shows volume, beat, and frequency bands in real time.
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex) {
    try {
        // Store data pointer for access in UI components (used for displaying current tempo)
        ImGui::GetIO().UserData = (void*)&data;
        
        std::lock_guard<std::mutex> lock(data_mutex);
        DrawToggles();
        DrawLogInfo();
        ImGui::Separator();
        DrawWebsite();
        ImGui::Separator();
        DrawVolume(data);
        ImGui::Separator();
        DrawBeat(data);
        ImGui::Separator();
        DrawFrequencyBands(data);
        ImGui::Separator();
        DrawTimePhaseInfo();
        ImGui::Separator();
        
        // Beat Detection Algorithm Selection and Configuration
        DrawBeatDetectionAlgorithm(data);
        ImGui::Separator();
        
        // Beat Decay Settings
        DrawBeatDecaySettings();
        ImGui::Separator();
        
        ImGui::Text("Frequency Band Mapping:");
        ImGui::Checkbox("Logarithmic Bands", &g_settings.band_log_scale);
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Log scale better matches hearing; linear is legacy");
        }
        
        // Display log-specific settings only when log scale is enabled
        if (g_settings.band_log_scale) {
            ImGui::SliderFloat("##LogStrength", &g_settings.band_log_strength, 0.2f, 3.0f, "%.2f");
            ImGui::SameLine();
            ImGui::Text("Log Strength");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls bass detail in logarithmic scale");
            }
        }
        
        // Always show min/max frequency controls
        ImGui::SliderFloat("##MinFreq", &g_settings.band_min_freq, 10.0f, 500.0f, "%.0f");
        ImGui::SameLine();
        ImGui::Text("Min Freq (Hz)");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Minimum frequency for frequency bands");
        }
        
        ImGui::SliderFloat("##MaxFreq", &g_settings.band_max_freq, 2000.0f, 22050.0f, "%.0f");
        ImGui::SameLine();
        ImGui::Text("Max Freq (Hz)");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Maximum frequency for frequency bands");
        }
        
        // Always show frequency boost settings, regardless of log scale
        if (ImGui::CollapsingHeader("Frequency Boost Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Toggle between old and new frequency modifier systems
            ImGui::Checkbox("Use 5-Band Equalizer", &g_settings.use_equalizer);
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Toggle between modern 5-band equalizer and legacy boost system");
            }
            
            if (g_settings.use_equalizer) {
                // Apply standard spacing for the sliders
                ImGui::PushID("Equalizer");
                
                // Use the same compact style as Frequency Band Mapping with text on the right
                ImGui::SliderFloat("##band1", &g_settings.equalizer_band1, 0.0f, 4.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Low (Bass)");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Boost for lowest frequency bands (bass)");
                }
                
                ImGui::SliderFloat("##band2", &g_settings.equalizer_band2, 0.0f, 4.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Low-Mid");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Boost for low-mid frequency bands");
                }
                
                ImGui::SliderFloat("##band3", &g_settings.equalizer_band3, 0.0f, 4.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Mid");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Boost for mid frequency bands");
                }
                
                ImGui::SliderFloat("##band4", &g_settings.equalizer_band4, 0.0f, 4.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Mid-High");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Boost for mid-high frequency bands");
                }
                
                ImGui::SliderFloat("##band5", &g_settings.equalizer_band5, 0.0f, 4.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("High (Treble)");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Boost for highest frequency bands (treble)");
                }
                
                ImGui::PopID();
                
                // Add the equalizer width slider
                ImGui::SliderFloat("##EqualizerWidth", &g_settings.equalizer_width, 0.05f, 0.5f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Equalizer Width");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Controls band influence on neighboring frequencies\nLower = narrow bands with less overlap\nHigher = wider bands with more influence");
                }
            }
            else {
                // Legacy mid/high boost controls
                ImGui::Text("Mid-Range Boost:");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Boost specific frequency ranges with bell curve multipliers");
                }
                
                ImGui::SliderFloat("##MidBoostAmount", &g_settings.band_mid_boost, 1.0f, 3.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Mid Boost Amount");
                
                ImGui::SliderFloat("##MidCenterFreq", &g_settings.band_mid_center, 500.0f, 2000.0f, "%.0f");
                ImGui::SameLine();
                ImGui::Text("Mid Center Freq (Hz)");
                
                ImGui::Text("High-Range Boost:");
                ImGui::SliderFloat("##HighBoostAmount", &g_settings.band_high_boost, 1.0f, 3.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("High Boost Amount");
                
                ImGui::SliderFloat("##HighCenterFreq", &g_settings.band_high_center, 3000.0f, 12000.0f, "%.0f");
                ImGui::SameLine();
                ImGui::Text("High Center Freq (Hz)");
                
                ImGui::SliderFloat("##BellWidth", &g_settings.band_bell_width, 0.5f, 3.0f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Bell Width (octaves)");
                if (ImGui::IsItemHovered(-1)) {
                    ImGui::SetTooltip("Wider value = broader frequency boost, narrower = more focused");
                }
            }
        }
        ImGui::Separator();
        
        // Band-limited Beat Detection Settings
        ImGui::Text("Band-Limited Beat Detection:");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Focus beat detection on specific frequency range, e.g., bass/kick drums");
        }
        
        ImGui::SliderFloat("##BeatMinFreq", &g_settings.beat_min_freq, 0.0f, 100.0f, "%.1f");
        ImGui::SameLine();
        ImGui::Text("Beat Min Freq (Hz)");
        
        ImGui::SliderFloat("##BeatMaxFreq", &g_settings.beat_max_freq, 100.0f, 500.0f, "%.1f");
        ImGui::SameLine();
        ImGui::Text("Beat Max Freq (Hz)");
        
        ImGui::SliderFloat("##LowFluxSmoothing", &g_settings.flux_low_alpha, 0.01f, 0.5f, "%.3f");
        ImGui::SameLine();
        ImGui::Text("Low Flux Smoothing");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Lower value = smoother, higher = more responsive");
        }
        
        ImGui::SliderFloat("##LowFluxThreshold", &g_settings.flux_low_threshold_multiplier, 1.0f, 3.0f, "%.2f");
        ImGui::SameLine();
        ImGui::Text("Low Flux Threshold");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Lower value = more sensitive, higher = less false positives");
        }
        
        ImGui::Separator();
        
        // Consolidated buttons for Save, Load and Default
        ImGui::Text("Settings Management:");
        
        // Use columns to position the buttons side by side with equal width
        ImGui::Columns(3, "settings_buttons", false);
        
        // Save button
        if (ImGui::Button("Save Settings", ImVec2(-1, 0))) {
            SaveAllTunables();
            LOG_DEBUG("[Overlay] Settings saved to file");
        }
        ImGui::NextColumn();
        
        // Load button
        if (ImGui::Button("Load Settings", ImVec2(-1, 0))) {
            LoadAllTunables();
            LOG_DEBUG("[Overlay] Settings loaded from file");
        }
        ImGui::NextColumn();
        
        // Default button
        if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
            // Use the dedicated settings module function to reset defaults
            ResetAllTunablesToDefaults();
            LOG_DEBUG("[Overlay] Settings reset to default values");
        }
        
        // Reset columns
        ImGui::Columns(1);
        
        ImGui::Separator();
    } catch (const std::exception& ex) {
        LOG_ERROR(std::string("[Overlay] Exception in DrawListeningwayDebugOverlay: ") + ex.what());
    } catch (...) {
        LOG_ERROR("[Overlay] Unknown exception in DrawListeningwayDebugOverlay.");
    }
}
