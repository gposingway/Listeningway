// ---------------------------------------------
// Overlay Module Implementation
// Provides a debug ImGui overlay for real-time audio analysis data
// ---------------------------------------------
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define ImTextureID ImU64
#include <imgui.h>
#include <reshade.hpp>
#include "overlay.h"
#include "audio/analysis/audio_analysis.h"
#include "constants.h"
#include "audio_format_utils.h"
#include "settings.h"
#include "logging.h"
#include "thread_safety_manager.h"
#include "audio/capture/audio_capture.h"
#include "configuration/configuration_manager.h"
using Listeningway::ConfigurationManager;
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm> // For std::clamp

extern std::atomic_bool g_audio_analysis_enabled;
extern bool g_listeningway_debug_enabled;

// External declarations for global variables used in overlay
extern std::atomic_bool g_switching_provider;
extern std::mutex g_provider_switch_mutex;

// Declare SwitchAudioProvider for use in overlay.cpp
extern "C" bool SwitchAudioProvider(int providerType, int timeout_ms = 2000);

// Static reference to avoid repeated Instance() calls - safe since ConfigurationManager is a singleton
static auto& g_configManager = ConfigurationManager::Instance();

// Overlay/ImGui color constants (must be defined after ImGui headers)
constexpr ImU32 OVERLAY_BAR_COLOR_BG = IM_COL32(40, 40, 40, 128);           // Dark background for bars
constexpr ImU32 OVERLAY_BAR_COLOR_OUTLINE = IM_COL32(60, 60, 60, 128);      // Outline for frequency bars
constexpr ImU32 OVERLAY_BAR_COLOR_CENTER_MARKER = IM_COL32(255, 255, 255, 180); // Center marker (white, semi-transparent)

// Helper: Draw toggles (audio analysis, debug logging)
static void DrawToggles() {
    auto& config = g_configManager.GetConfig();
    
    // Audio Provider Selection Dropdown
    std::vector<AudioProviderInfo> available_providers = GetAvailableAudioCaptureProviders();
    std::vector<const char*> provider_names;
    for (const auto& info : available_providers) {
        provider_names.push_back(info.name.c_str());
    }

    // Find current selection index by code
    int display_selection_index = 0;
    std::string current_code = config.audio.captureProviderCode;
    for (size_t i = 0; i < available_providers.size(); ++i) {
        if (available_providers[i].code == current_code) {
            display_selection_index = static_cast<int>(i);
            break;
        }
    }

    if (ImGui::BeginCombo("Audio Analysis", provider_names[display_selection_index])) {
        int previous_selection = display_selection_index;
        bool switching_provider = g_switching_provider;
        for (int i = 0; i < provider_names.size(); ++i) {
            const bool is_selected = (display_selection_index == i);
            bool selectable = !switching_provider;            if (ImGui::Selectable(provider_names[i], is_selected, selectable ? 0 : ImGuiSelectableFlags_Disabled)) {
                if (previous_selection != i && !switching_provider) {
                    const auto& selected_info = available_providers[i];
                    config.audio.captureProviderCode = selected_info.code;
                    
                    // Map provider code to provider type for existing SwitchAudioProvider function
                    int provider_type = -1; // Default to "None/Off"
                    if (selected_info.code == "system") {
                        provider_type = 0; // SYSTEM_AUDIO
                    } else if (selected_info.code == "game") {
                        provider_type = 1; // PROCESS_AUDIO
                    }
                    // "off" code stays as -1 for None
                    
                    // Switch the provider using the existing robust function
                    bool switch_ok = SwitchAudioProvider(provider_type, 2000);
                    
                    if (switch_ok) {
                        LOG_DEBUG(std::string("[Overlay] Audio Provider changed to: ") + selected_info.name + 
                                 " (code: " + selected_info.code + ", type: " + std::to_string(provider_type) + ")");
                    } else {
                        LOG_ERROR(std::string("[Overlay] Failed to switch to provider: ") + selected_info.name);
                    }
                }
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Use the global debug flag directly, then synchronize with configManager through SetDebugEnabled
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
    auto& config = g_configManager.GetConfig();
    float amp = config.frequency.amplifier;
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Volume:");
    ImGui::SameLine();
    ImGui::ProgressBar(std::clamp(data.volume * amp, 0.0f, 1.0f), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
}

// Helper: Draw beat meter
static void DrawBeat(const AudioAnalysisData& data) {
    auto& config = g_configManager.GetConfig();
    float amp = config.frequency.amplifier;
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Beat:");
    ImGui::SameLine();
    ImGui::ProgressBar(std::clamp(data.beat * amp, 0.0f, 1.0f), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
}

// Helper: Draw frequency bands with view mode
static void DrawFrequencyBands(const AudioAnalysisData& data) {
    auto& config = g_configManager.GetConfig();
    float amp = config.frequency.amplifier;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Frequency Bands");
    
    size_t band_count = data.freq_bands.size();
    
    // Always use compact visualization with thin horizontal bars stacked vertically
    // Calculate total height for all bars with no spacing
    float barHeight = OVERLAY_BAR_HEIGHT_THIN; // Thin bar height
    float totalHeight = barHeight * band_count;
    
    // Create a child window to contain all the bars - removed scrollbars
    ImGui::BeginChild("FreqBandsCompact", ImVec2(0, totalHeight + 15), true, ImGuiWindowFlags_NoScrollbar);
    
    // Get starting cursor position for drawing bars
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // Draw each frequency band as a thin bar
    for (size_t i = 0; i < band_count; ++i) {
        float value = std::clamp(data.freq_bands[i] * amp, 0.0f, 1.0f);
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
            OVERLAY_BAR_ROUNDING  // No rounding
        );
        // Draw background/outline for the full bar area
        ImGui::GetWindowDrawList()->AddRect(
            barStart,
            ImVec2(startPos.x + windowSize.x, barStart.y + barHeight),
            OVERLAY_BAR_COLOR_OUTLINE, // Dark gray
            OVERLAY_BAR_ROUNDING  // No rounding
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
    auto& config = g_configManager.GetConfig();
    
    ImGui::Text("Beat Detection Algorithm:");
    
    // Create a combo box for algorithm selection
    const char* algorithms[] = { "Simple Energy (Original)", "Spectral Flux + Autocorrelation (Advanced)" };    int algorithm = config.beat.algorithm;    if (ImGui::Combo("Algorithm", &algorithm, algorithms, IM_ARRAYSIZE(algorithms))) {
        config.beat.algorithm = algorithm;
        LOG_DEBUG(std::string("[Overlay] Beat Detection Algorithm changed to ") + 
                 (algorithm == 0 ? "Simple Energy" : "Spectral Flux + Autocorrelation"));
        // Update the audio analyzer with the new algorithm
        g_audio_analyzer.SetBeatDetectionAlgorithm(algorithm);
    }
    
    if (ImGui::IsItemHovered(-1)) {
        if (config.beat.algorithm == 0) {
            ImGui::SetTooltip("Simple Energy: Works well with strong bass beats");
        } else {
            ImGui::SetTooltip("Advanced: Better for complex rhythms and various music genres");
        }
    }
    
    // Show advanced settings for the Spectral Flux + Autocorrelation algorithm
    if (config.beat.algorithm == 1) {
        // Create a collapsing section for advanced parameters
        if (ImGui::CollapsingHeader("Advanced Algorithm Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Spectral Flux threshold adjustment
            float spectral_flux_threshold = config.beat.spectralFluxThreshold;
            if (ImGui::SliderFloat("##SpectralFluxThreshold", &spectral_flux_threshold, 0.01f, 0.2f, "%.3f")) {
                config.beat.spectralFluxThreshold = spectral_flux_threshold;
            }
            ImGui::SameLine();
            ImGui::Text("Spectral Flux Threshold");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Lower value = more sensitive to subtle changes");            }
            
            // Tempo change threshold adjustment
            float tempo_change_threshold = config.beat.tempoChangeThreshold;
            if (ImGui::SliderFloat("##TempoChangeThreshold", &tempo_change_threshold, 0.1f, 0.5f, "%.2f")) {
                config.beat.tempoChangeThreshold = tempo_change_threshold;
            }
            ImGui::SameLine();
            ImGui::Text("Tempo Change Threshold");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Higher value = more stable tempo, lower = adapts faster");
            }
              // Beat induction window adjustment
            float beat_induction_window = config.beat.beatInductionWindow;
            if (ImGui::SliderFloat("##BeatInductionWindow", &beat_induction_window, 0.05f, 0.2f, "%.2f")) {
                config.beat.beatInductionWindow = beat_induction_window;
            }
            ImGui::SameLine();
            ImGui::Text("Beat Induction Window");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Larger window = more adaptive phase adjustment");
            }
              // Octave error weight adjustment
            float octave_error_weight = config.beat.octaveErrorWeight;
            if (ImGui::SliderFloat("##OctaveErrorWeight", &octave_error_weight, 0.5f, 0.9f, "%.2f")) {
                config.beat.octaveErrorWeight = octave_error_weight;
            }
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
    auto& config = g_configManager.GetConfig();
    ImGui::Text("Beat Decay Settings:");
    if (config.beat.algorithm == 0) {
        float beat_falloff_default = config.beat.falloffDefault;
        if (ImGui::SliderFloat("##DefaultFalloffRate", &beat_falloff_default, 0.5f, 5.0f, "%.2f")) {
            config.beat.falloffDefault = beat_falloff_default;
        }
        ImGui::SameLine();
        ImGui::Text("Default Falloff Rate");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Controls how quickly the beat indicator fades out\nHigher values = faster decay");
        }
        if (ImGui::CollapsingHeader("Adaptive Falloff Settings")) {
            float beat_time_scale = config.beat.timeScale;
            if (ImGui::SliderFloat("##TimeScale", &beat_time_scale, 1e-10f, 1e-8f, "%.2e")) {
                config.beat.timeScale = beat_time_scale;
            }
            ImGui::SameLine();
            ImGui::Text("Time Scale");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls time scaling for beat interval");
            }
            float beat_time_initial = config.beat.timeInitial;
            if (ImGui::SliderFloat("##InitialTime", &beat_time_initial, 0.1f, 1.0f, "%.2f")) {
                config.beat.timeInitial = beat_time_initial;
            }
            ImGui::SameLine();
            ImGui::Text("Initial Time");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls initial time since last beat");
            }
            float beat_time_min = config.beat.timeMin;
            if (ImGui::SliderFloat("##MinTime", &beat_time_min, 0.01f, 0.5f, "%.2f")) {
                config.beat.timeMin = beat_time_min;
            }
            ImGui::SameLine();
            ImGui::Text("Min Time");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls minimum time for adaptive falloff");
            }
            float beat_time_divisor = config.beat.timeDivisor;
            if (ImGui::SliderFloat("##TimeDivisor", &beat_time_divisor, 0.01f, 1.0f, "%.2f")) {
                config.beat.timeDivisor = beat_time_divisor;
            }
            ImGui::SameLine();
            ImGui::Text("Time Divisor");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls divisor for adaptive falloff\nThese settings control how decay adapts to beat timing");
            }
        }
    } else {
        float spectral_flux_decay_multiplier = config.beat.spectralFluxDecayMultiplier;
        if (ImGui::SliderFloat("##DecayMultiplier", &spectral_flux_decay_multiplier, 0.5f, 5.0f, "%.2f")) {
            config.beat.spectralFluxDecayMultiplier = spectral_flux_decay_multiplier;
        }
        ImGui::SameLine();
        ImGui::Text("Decay Multiplier");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Controls how quickly the beat indicator fades out\nHigher values = faster decay relative to tempo\nThis algorithm automatically adapts to music tempo");
        }
    }
}

// Frequency Boost Settings section
static void DrawFrequencyBoostSettings() {
    auto& config = g_configManager.GetConfig();
    
    if (ImGui::CollapsingHeader("Frequency Boost Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Equalizer");
          // Use the same compact style as Frequency Band Mapping with text on the right
        float equalizer_band1 = config.frequency.equalizerBands[0];
        if (ImGui::SliderFloat("##band1", &equalizer_band1, OVERLAY_EQ_BAND_MIN, OVERLAY_EQ_BAND_MAX, "%.2f")) {
            config.frequency.equalizerBands[0] = equalizer_band1;
        }
        ImGui::SameLine();
        ImGui::Text("Low (Bass)");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Boost for lowest frequency bands (bass)");
        }
          float equalizer_band2 = config.frequency.equalizerBands[1];
        if (ImGui::SliderFloat("##band2", &equalizer_band2, OVERLAY_EQ_BAND_MIN, OVERLAY_EQ_BAND_MAX, "%.2f")) {
            config.frequency.equalizerBands[1] = equalizer_band2;
        }
        ImGui::SameLine();
        ImGui::Text("Low-Mid");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Boost for low-mid frequency bands");
        }
          float equalizer_band3 = config.frequency.equalizerBands[2];
        if (ImGui::SliderFloat("##band3", &equalizer_band3, OVERLAY_EQ_BAND_MIN, OVERLAY_EQ_BAND_MAX, "%.2f")) {
            config.frequency.equalizerBands[2] = equalizer_band3;
        }
        ImGui::SameLine();
        ImGui::Text("Mid");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Boost for mid frequency bands");
        }
          float equalizer_band4 = config.frequency.equalizerBands[3];
        if (ImGui::SliderFloat("##band4", &equalizer_band4, OVERLAY_EQ_BAND_MIN, OVERLAY_EQ_BAND_MAX, "%.2f")) {
            config.frequency.equalizerBands[3] = equalizer_band4;
        }
        ImGui::SameLine();
        ImGui::Text("Mid-High");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Boost for mid-high frequency bands");
        }
          float equalizer_band5 = config.frequency.equalizerBands[4];
        if (ImGui::SliderFloat("##band5", &equalizer_band5, OVERLAY_EQ_BAND_MIN, OVERLAY_EQ_BAND_MAX, "%.2f")) {
            config.frequency.equalizerBands[4] = equalizer_band5;
        }
        ImGui::SameLine();
        ImGui::Text("High (Treble)");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Boost for highest frequency bands (treble)");
        }
        
        ImGui::PopID();
          // Add the equalizer width slider
        float equalizer_width = config.frequency.equalizerWidth;
        if (ImGui::SliderFloat("##EqualizerWidth", &equalizer_width, OVERLAY_EQ_WIDTH_MIN, OVERLAY_EQ_WIDTH_MAX, "%.2f")) {
            config.frequency.equalizerWidth = equalizer_width;
        }
        ImGui::SameLine();
        ImGui::Text("Equalizer Width");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Controls band influence on neighboring frequencies\nLower = narrow bands with less overlap\nHigher = wider bands with more influence");
        }
    }
}

// Helper: Draw spatialization info (left/right volume and pan)
static void DrawSpatialization(const AudioAnalysisData& data) {
    // Align all progress bars to the same X position
    float label_width = ImGui::CalcTextSize("Pan Angle:").x;
    float bar_start_x = ImGui::GetCursorPosX() + label_width + ImGui::GetStyle().ItemSpacing.x * 2.0f;
    float bar_width = ImGui::GetContentRegionAvail().x - (bar_start_x - ImGui::GetCursorPosX());

    // Left Volume
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Left:");
    ImGui::SameLine(bar_start_x);
    ImGui::ProgressBar(data.volume_left, ImVec2(bar_width, 0.0f));
    ImGui::SameLine();
    ImGui::Text("%.2f", data.volume_left);

    // Right Volume
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Right:");
    ImGui::SameLine(bar_start_x);
    ImGui::ProgressBar(data.volume_right, ImVec2(bar_width, 0.0f));
    ImGui::SameLine();
    ImGui::Text("%.2f", data.volume_right);

    // Pan Angle (legacy, remove or update to new range)
    // ImGui::AlignTextToFramePadding();
    // ImGui::Text("Pan Angle:");
    // ImGui::SameLine(bar_start_x);
    // ImGui::ProgressBar((data.audio_pan + 180.0f) / 360.0f, ImVec2(bar_width, 0.0f));
    // ImGui::SameLine();
    // ImGui::Text("%.1f deg", data.audio_pan);
}

// Helper: Draw all volume, spatialization, and beat info in a single aligned block
static void DrawVolumeSpatializationBeat(const AudioAnalysisData& data) {
    auto& config = g_configManager.GetConfig();
    float amp = config.frequency.amplifier;
    // Find the widest label
    float label_width = ImGui::CalcTextSize("Pan Angle:").x;
    label_width = std::max(label_width, ImGui::CalcTextSize("Volume:").x);
    label_width = std::max(label_width, ImGui::CalcTextSize("Left:").x);
    label_width = std::max(label_width, ImGui::CalcTextSize("Right:").x);
    label_width = std::max(label_width, ImGui::CalcTextSize("Beat:").x);
    label_width = std::max(label_width, ImGui::CalcTextSize("Format:").x);
    label_width = std::max(label_width, ImGui::CalcTextSize("Pan Smooth:").x);
    float bar_start_x = ImGui::GetCursorPosX() + label_width + ImGui::GetStyle().ItemSpacing.x * 2.0f;
    float bar_width = ImGui::GetContentRegionAvail().x - (bar_start_x - ImGui::GetCursorPosX());    // Volume (overall)
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Volume:");
    ImGui::SameLine(bar_start_x);
    
    // Get the screen position right after the progress bar for proper alignment
    ImVec2 progress_bar_screen_pos = ImGui::GetCursorScreenPos();
    ImGui::ProgressBar(std::clamp(data.volume * amp, 0.0f, 1.0f), ImVec2(bar_width, 0.0f));
    ImGui::SameLine();
    ImGui::Text("%.2f", data.volume * amp);    // Compact Left/Right display under the main volume bar
    const float thin_bar_height = OVERLAY_BAR_HEIGHT_THIN;  // Same height as frequency bands
    const float small_spacing = OVERLAY_BAR_SPACING_SMALL;    // Small gap between left and right bars
    const float half_bar_width = (bar_width - small_spacing) * 0.5f;
    
    // Use the captured progress bar position for perfect alignment
    ImVec2 start_pos = progress_bar_screen_pos;
    start_pos.y += ImGui::GetFrameHeight() + OVERLAY_BAR_SPACING_SMALL;  // Position below the progress bar
    
    // Calculate center point for both bars
    float center_x = start_pos.x + bar_width * 0.5f;
    
    // Draw Left volume bar (grows from center leftward)
    ImVec2 left_bar_bg_min = ImVec2(start_pos.x, start_pos.y);
    ImVec2 left_bar_bg_max = ImVec2(center_x - small_spacing * 0.5f, start_pos.y + thin_bar_height);
    ImVec2 left_bar_fill_min = ImVec2(center_x - small_spacing * 0.5f - std::clamp(data.volume_left * amp, 0.0f, 1.0f) * half_bar_width, start_pos.y);
    ImVec2 left_bar_fill_max = ImVec2(center_x - small_spacing * 0.5f, start_pos.y + thin_bar_height);
      // Draw Left bar background
    ImGui::GetWindowDrawList()->AddRectFilled(
        left_bar_bg_min, left_bar_bg_max,
        OVERLAY_BAR_COLOR_BG,  // Dark background
        OVERLAY_BAR_ROUNDING  // No rounding
    );
    
    // Draw Left bar fill (grows from right edge leftward)
    if (data.volume_left > 0.0f) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            left_bar_fill_min, left_bar_fill_max,
            ImGui::GetColorU32(ImGuiCol_PlotHistogram),  // Match main volume bar color
            OVERLAY_BAR_ROUNDING  // No rounding
        );
    }
    
    // Draw Right volume bar (grows from center rightward)
    ImVec2 right_bar_bg_min = ImVec2(center_x + small_spacing * 0.5f, start_pos.y);
    ImVec2 right_bar_bg_max = ImVec2(start_pos.x + bar_width, start_pos.y + thin_bar_height);
    ImVec2 right_bar_fill_min = ImVec2(center_x + small_spacing * 0.5f, start_pos.y);
    ImVec2 right_bar_fill_max = ImVec2(center_x + small_spacing * 0.5f + std::clamp(data.volume_right * amp, 0.0f, 1.0f) * half_bar_width, start_pos.y + thin_bar_height);
    
    // Draw Right bar background
    ImGui::GetWindowDrawList()->AddRectFilled(
        right_bar_bg_min, right_bar_bg_max,
        OVERLAY_BAR_COLOR_BG,  // Dark background
        OVERLAY_BAR_ROUNDING  // No rounding
    );
    
    // Draw Right bar fill (grows from left edge rightward)
    if (data.volume_right > 0.0f) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            right_bar_fill_min, right_bar_fill_max,
            ImGui::GetColorU32(ImGuiCol_PlotHistogram),  // Match main volume bar color
            OVERLAY_BAR_ROUNDING  // No rounding
        );    }    // Reserve space for the custom drawn left/right bars and move cursor down
    // Use minimal spacing between the left/right bars and the pan bar
    // This should be just enough to visually separate the bars (2-4 pixels)
    ImGui::Dummy(ImVec2(0, OVERLAY_BAR_SPACING_LARGE)); // Small spacing after left/right bars
    
    // Pan bar (without label and without text overlay)
    // Use invisible dummy element for alignment with bars above
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::SameLine(bar_start_x); // Align exactly with the bars above
    
    // Now get cursor position after alignment
    ImVec2 pan_cursor_pos = ImGui::GetCursorScreenPos();
    float pan_clamped = std::clamp(data.audio_pan, -1.0f, 1.0f);
    
    // Draw pan bar background - full width to match other bars
    ImVec2 pan_bar_bg_min = ImVec2(pan_cursor_pos.x, pan_cursor_pos.y);
    ImVec2 pan_bar_bg_max = ImVec2(pan_cursor_pos.x + bar_width, pan_cursor_pos.y + thin_bar_height);
    ImGui::GetWindowDrawList()->AddRectFilled(
        pan_bar_bg_min, pan_bar_bg_max,
        OVERLAY_BAR_COLOR_BG,  // Dark background (same as other bars)
        OVERLAY_BAR_ROUNDING  // No rounding
    );
    
    // Calculate center position
    float pan_center_x = pan_cursor_pos.x + (bar_width * 0.5f);
    
    // Draw pan bar fill based on value
    // For negative values (-1 to 0), extend from center to left
    // For positive values (0 to +1), extend from center to right
    if (pan_clamped < 0.0f) {
        // Extend to the left for negative values
        float width = -pan_clamped * (bar_width * 0.5f); // Scale to half width
        ImVec2 pan_bar_fill_min = ImVec2(pan_center_x - width, pan_cursor_pos.y);
        ImVec2 pan_bar_fill_max = ImVec2(pan_center_x, pan_cursor_pos.y + thin_bar_height);
        
        ImGui::GetWindowDrawList()->AddRectFilled(
            pan_bar_fill_min, pan_bar_fill_max,
            ImGui::GetColorU32(ImGuiCol_PlotHistogram),  // Match main volume bar color
            OVERLAY_BAR_ROUNDING  // No rounding
        );
    } else if (pan_clamped > 0.0f) {
        // Extend to the right for positive values
        float width = pan_clamped * (bar_width * 0.5f); // Scale to half width
        ImVec2 pan_bar_fill_min = ImVec2(pan_center_x, pan_cursor_pos.y);
        ImVec2 pan_bar_fill_max = ImVec2(pan_center_x + width, pan_cursor_pos.y + thin_bar_height);
        
        ImGui::GetWindowDrawList()->AddRectFilled(
            pan_bar_fill_min, pan_bar_fill_max,
            ImGui::GetColorU32(ImGuiCol_PlotHistogram),  // Match main volume bar color
            OVERLAY_BAR_ROUNDING  // No rounding
        );
    }
    
    // Add center marker line
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(pan_center_x, pan_cursor_pos.y),
        ImVec2(pan_center_x, pan_cursor_pos.y + thin_bar_height),
        OVERLAY_BAR_COLOR_CENTER_MARKER,  // White semi-transparent
        OVERLAY_BAR_CENTER_MARKER_THICKNESS
    );
    
    // Reserve space for the pan bar (using Dummy to advance cursor)
    ImGui::Dummy(ImVec2(bar_width, thin_bar_height));
    
    // Add spacing after the pan bar (same as other bars)
    ImGui::Dummy(ImVec2(0, OVERLAY_BAR_SPACING_LARGE));
    // Beat
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Beat:");
    ImGui::SameLine(bar_start_x);
    ImGui::ProgressBar(data.beat, ImVec2(bar_width, 0.0f));
    ImGui::SameLine();
    ImGui::Text("%.2f", data.beat * amp);    // Audio Format
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Format:");
    ImGui::SameLine(bar_start_x);
    AudioFormat format = AudioFormatUtils::IntToFormat(static_cast<int>(data.audio_format));
    const char* format_name = AudioFormatUtils::FormatToString(format);
    ImGui::Text("%s (%.0f)", format_name, data.audio_format);// Pan Smoothing
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Pan Smooth:");
    ImGui::SameLine(bar_start_x);
    float pan_smoothing = config.audio.panSmoothing;
    ImGui::PushItemWidth(bar_width);
    if (ImGui::SliderFloat("##PanSmoothing", &pan_smoothing, 0.0f, 1.0f, "%.2f")) {
        config.audio.panSmoothing = pan_smoothing;
    }
    ImGui::PopItemWidth();
    if (ImGui::IsItemHovered(-1)) {
        ImGui::SetTooltip("Reduces pan jitter. 0.0 = no smoothing (current behavior), higher values = more smoothing");
    }
    // Amplifier slider (full width, aligned under Pan Smooth, with label)
    float amplifier = config.frequency.amplifier;
    bool amp_is_spinal = amplifier > 10.0f;
    if (amp_is_spinal) {
        float t = std::clamp((amplifier - 10.0f) / 1.0f, 0.0f, 1.0f);
        ImVec4 base = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive);
        ImVec4 red = ImVec4(1.0f, 0.1f, 0.1f, 1.0f);
        ImVec4 lerped = ImVec4(
            base.x * (1.0f - t) + red.x * t,
            base.y * (1.0f - t) + red.y * t,
            base.z * (1.0f - t) + red.z * t,
            base.w * (1.0f - t) + red.w * t
        );
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, lerped);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, lerped);
    }
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Amplifier:");
    ImGui::SameLine(bar_start_x);
    ImGui::PushItemWidth(bar_width);
    if (ImGui::SliderFloat("##Amplifier", &amplifier, OVERLAY_AMPLIFIER_MIN, OVERLAY_AMPLIFIER_MAX, "%.2f")) {
        config.frequency.amplifier = amplifier;
    }
    ImGui::PopItemWidth();
    if (amp_is_spinal) {
        ImGui::PopStyleColor(2);
    }
    if (ImGui::IsItemHovered(-1)) {
        ImGui::SetTooltip("Scales all visualization values (volume, beat, bands). Useful for low-volume systems.");
    }
}

// Draws the Listeningway debug overlay using ImGui.
// Shows volume, beat, and frequency bands in real time.
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data) {
    try {
        ImGui::GetIO().UserData = (void*)&data;
        LOCK_AUDIO_DATA();
        DrawToggles();
        DrawLogInfo();
        ImGui::Separator();
        DrawWebsite();
        ImGui::Separator();        DrawVolumeSpatializationBeat(data);
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
        ImGui::Separator();        ImGui::Text("Frequency Band Mapping:");
        auto& config = g_configManager.GetConfig();
        bool band_log_scale = config.frequency.logScaleEnabled;
        if (ImGui::Checkbox("Logarithmic Bands", &band_log_scale)) {
            config.frequency.logScaleEnabled = band_log_scale;
        }
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Log scale better matches hearing; linear is legacy");
        }
        
        // Display log-specific settings only when log scale is enabled
        if (config.frequency.logScaleEnabled) {
            float band_log_strength = config.frequency.logStrength;
            if (ImGui::SliderFloat("##LogStrength", &band_log_strength, OVERLAY_LOG_STRENGTH_MIN, OVERLAY_LOG_STRENGTH_MAX, "%.2f")) {
                config.frequency.logStrength = band_log_strength;
            }
            ImGui::SameLine();
            ImGui::Text("Log Strength");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Controls bass detail in logarithmic scale");
            }
        
            float band_min_freq = config.frequency.minFreq;
            if (ImGui::SliderFloat("##MinFreq", &band_min_freq, OVERLAY_MIN_FREQ_MIN, OVERLAY_MIN_FREQ_MAX, "%.0f")) {
                config.frequency.minFreq = band_min_freq;
            }
            ImGui::SameLine();
            ImGui::Text("Min Freq (Hz)");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Minimum frequency for frequency bands");
            }
            
            float band_max_freq = config.frequency.maxFreq;
            if (ImGui::SliderFloat("##MaxFreq", &band_max_freq, OVERLAY_MAX_FREQ_MIN, OVERLAY_MAX_FREQ_MAX, "%.0f")) {
                config.frequency.maxFreq = band_max_freq;
            }
            ImGui::SameLine();
            ImGui::Text("Max Freq (Hz)");
            if (ImGui::IsItemHovered(-1)) {
                ImGui::SetTooltip("Maximum frequency for frequency bands");
            }

        }

        
        // 5-band equalizer settings
        DrawFrequencyBoostSettings();
        ImGui::Separator();        // Band-limited Beat Detection Settings
        ImGui::Text("Band-Limited Beat Detection:");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Focus beat detection on specific frequency range, e.g., bass/kick drums");
        }
        
        float beat_min_freq = config.beat.minFreq;
        if (ImGui::SliderFloat("##BeatMinFreq", &beat_min_freq, OVERLAY_BEAT_MIN_FREQ_MIN, OVERLAY_BEAT_MIN_FREQ_MAX, "%.1f")) {
            config.beat.minFreq = beat_min_freq;
        }
        ImGui::SameLine();
        ImGui::Text("Beat Min Freq (Hz)");
        
        float beat_max_freq = config.beat.maxFreq;
        if (ImGui::SliderFloat("##BeatMaxFreq", &beat_max_freq, OVERLAY_BEAT_MAX_FREQ_MIN, OVERLAY_BEAT_MAX_FREQ_MAX, "%.1f")) {
            config.beat.maxFreq = beat_max_freq;
        }
        ImGui::SameLine();
        ImGui::Text("Beat Max Freq (Hz)");
        
        float flux_low_alpha = config.beat.fluxLowAlpha;
        if (ImGui::SliderFloat("##LowFluxSmoothing", &flux_low_alpha, OVERLAY_FLUX_SMOOTH_MIN, OVERLAY_FLUX_SMOOTH_MAX, "%.3f")) {
            config.beat.fluxLowAlpha = flux_low_alpha;
        }
        ImGui::SameLine();
        ImGui::Text("Low Flux Smoothing");
        if (ImGui::IsItemHovered(-1)) {
            ImGui::SetTooltip("Lower value = smoother, higher = more responsive");
        }
        
        float flux_low_threshold_multiplier = config.beat.fluxLowThresholdMultiplier;
        if (ImGui::SliderFloat("##LowFluxThreshold", &flux_low_threshold_multiplier, OVERLAY_FLUX_THRESH_MIN, OVERLAY_FLUX_THRESH_MAX, "%.2f")) {
            config.beat.fluxLowThresholdMultiplier = flux_low_threshold_multiplier;
        }
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
            if (g_configManager.Save()) {
                LOG_DEBUG("[Overlay] Settings saved to file");
            } else {
                LOG_ERROR("[Overlay] Failed to save settings to file");
            }
        }
        ImGui::NextColumn();
        // Load button
        if (ImGui::Button("Load Settings", ImVec2(-1, 0))) {
            if (g_configManager.Load()) {
                LOG_DEBUG("[Overlay] Settings loaded from file");
                // No longer re-apply config here; ConfigurationManager handles it
            } else {
                LOG_ERROR("[Overlay] Failed to load settings to file");
            }
        }
        ImGui::NextColumn();
        // Default button
        if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
            g_configManager.ResetToDefaults();
            LOG_DEBUG("[Overlay] Settings reset to default values");
            // No longer re-apply config here; ConfigurationManager handles it
        }
        
        // Reset columns
        ImGui::Columns(1);
        
        ImGui::Separator();
    } catch (const std::exception& ex) {
        LOG_ERROR(std::string("[Overlay] Exception in DrawListeningwayDebugOverlay: ") + ex.what());
    } catch (...) {
        LOG_ERROR("[Overlay] Unknown exception in DrawListeningwayDebugOverlay.");    }
}
