#pragma once
#include <vector>
#include <mutex>
#include "audio_analysis.h"

void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex);
