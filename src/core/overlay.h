#pragma once
#include <vector>
#include <mutex>
#include "audio_analysis.h"

// Draws the Listeningway debug overlay with real-time audio data
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex);
