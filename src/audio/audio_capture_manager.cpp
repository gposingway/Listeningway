// ---------------------------------------------
// Audio Capture Manager Implementation
// Manages audio capture providers and handles provider selection
// ---------------------------------------------
#include "audio_capture_manager.h"
#include "providers/system_audio_provider.h"
#include "providers/process_audio_provider.h"
#include "providers/off_audio_provider.h"
#include "../utils/logging.h"
#include <algorithm>
#include <climits>

AudioCaptureManager::AudioCaptureManager() 
    : preferred_provider_type_(AudioCaptureProviderType::SYSTEM_AUDIO),
      current_provider_(nullptr),
      initialized_(false) {
}

AudioCaptureManager::~AudioCaptureManager() {
    Uninitialize();
}

bool AudioCaptureManager::Initialize() {
    if (initialized_) {
        return true;
    }

    LOG_DEBUG("[AudioCaptureManager] Initializing audio capture manager");
    
    RegisterProviders();
    
    // Initialize all providers
    for (auto& provider : providers_) {
        if (!provider->Initialize()) {
            LOG_WARNING("[AudioCaptureManager] Failed to initialize provider: " + provider->GetProviderName());
        }
    }
      // Select the initial provider
    current_provider_ = SelectBestProvider();
    if (!current_provider_) {
        LOG_ERROR("[AudioCaptureManager] No available audio capture providers");
        return false;
    }
    
    LOG_INFO("[AudioCaptureManager] Initialized with provider: " + current_provider_->GetProviderName());
    initialized_ = true;
    return true;
}

void AudioCaptureManager::Uninitialize() {
    if (!initialized_) {
        return;
    }    LOG_DEBUG("[AudioCaptureManager] Uninitializing audio capture manager");
    
    current_provider_ = nullptr;
    
    // Uninitialize all providers
    for (auto& provider : providers_) {
        provider->Uninitialize();
    }
    
    providers_.clear();
    initialized_ = false;
}

void AudioCaptureManager::RegisterProviders() {
    // Register system audio provider (always available)
    providers_.push_back(std::make_unique<SystemAudioCaptureProvider>());
    
    // Register process audio provider
    providers_.push_back(std::make_unique<ProcessAudioCaptureProvider>());
    
    // Register off audio provider (dummy provider)
    providers_.push_back(std::make_unique<OffAudioCaptureProvider>());
    
    LOG_DEBUG("[AudioCaptureManager] Registered " + std::to_string(providers_.size()) + " audio capture providers");
}

std::vector<AudioCaptureProviderType> AudioCaptureManager::GetAvailableProviders() const {
    std::vector<AudioCaptureProviderType> available;
    
    for (const auto& provider : providers_) {
        if (provider->IsAvailable()) {
            available.push_back(provider->GetProviderType());
        }
    }
    
    return available;
}

std::vector<AudioProviderInfo> AudioCaptureManager::GetAvailableProviderInfos() const {
    std::vector<AudioProviderInfo> infos;
    for (const auto& provider : providers_) {
        if (provider->IsAvailable()) {
            infos.push_back(provider->GetProviderInfo());
        }
    }
    
    // Sort providers by their order field
    std::sort(infos.begin(), infos.end(), [](const AudioProviderInfo& a, const AudioProviderInfo& b) {
        return a.order < b.order;
    });
    
    return infos;
}

std::string AudioCaptureManager::GetProviderName(AudioCaptureProviderType type) const {
    IAudioCaptureProvider* provider = FindProvider(type);
    if (provider) {
        return provider->GetProviderName();
    }
    return "Unknown Provider";
}

bool AudioCaptureManager::SetPreferredProvider(AudioCaptureProviderType type) {
    IAudioCaptureProvider* provider = FindProvider(type);
    if (!provider || !provider->IsAvailable()) {
        LOG_WARNING("[AudioCaptureManager] Preferred provider not available: " + GetProviderName(type));
        return false;
    }
    
    preferred_provider_type_ = type;
    // Set analysisEnabled based on provider's activates_capture
    Listeningway::ConfigurationManager::Instance().SetAnalysisEnabled(provider->GetProviderInfo().activates_capture);
    LOG_INFO("[AudioCaptureManager] Set preferred provider to: " + provider->GetProviderName());
    
    // If we're currently using a different provider, switch to the preferred one
    if (!current_provider_ || current_provider_->GetProviderType() != type) {
        // Stop current capture if running
        if (current_provider_) {
            LOG_INFO("[AudioCaptureManager] Stopping capture for old provider: " + current_provider_->GetProviderName());
            // We need to stop the thread. We'll require a reference to the running/thread/data_mutex/data from the caller.
            // Instead, let's add a new method to handle this at a higher level.
        }
        current_provider_ = provider;
        LOG_INFO("[AudioCaptureManager] Switched to preferred provider: " + current_provider_->GetProviderName());
    }
    
    return true;
}

bool AudioCaptureManager::SetPreferredProviderByCode(const std::string& providerCode) {
    // Find provider by code
    for (const auto& provider : providers_) {
        if (provider->GetProviderInfo().code == providerCode) {
            return SetPreferredProvider(provider->GetProviderType());
        }
    }
    
    LOG_WARNING("[AudioCaptureManager] Provider with code '" + providerCode + "' not found");
    return false;
}

// New method: Switch provider and restart capture thread if running
bool AudioCaptureManager::SwitchProviderAndRestart(AudioCaptureProviderType type, const Listeningway::Configuration& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    bool was_running = running.load();
    if (was_running) {
        StopCapture(running, thread);
    }
    bool set_ok = SetPreferredProvider(type);
    if (!set_ok) return false;
    if (was_running) {
        return StartCapture(config, running, thread, data_mutex, data);
    }
    return true;
}

// New method: Switch provider by code and restart capture thread if running
bool AudioCaptureManager::SwitchProviderByCodeAndRestart(const std::string& providerCode, const Listeningway::Configuration& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    // Handle "off" code specially - stop capture and don't switch to any provider
    if (providerCode == "off") {
        if (running.load()) {
            StopCapture(running, thread);
        }
        LOG_DEBUG("[AudioCaptureManager] Switched to 'off' - audio analysis disabled");
        return true;
    }
    
    // Find provider by code
    IAudioCaptureProvider* target_provider = nullptr;
    for (const auto& provider : providers_) {
        if (provider->IsAvailable() && provider->GetProviderInfo().code == providerCode) {
            target_provider = provider.get();
            break;
        }
    }
    
    if (!target_provider) {
        LOG_ERROR("[AudioCaptureManager] Provider with code '" + providerCode + "' not found or not available");
        return false;
    }
    
    // Use the existing method with the provider type
    return SwitchProviderAndRestart(target_provider->GetProviderType(), config, running, thread, data_mutex, data);
}

AudioCaptureProviderType AudioCaptureManager::GetCurrentProvider() const {
    if (current_provider_) {
        return current_provider_->GetProviderType();
    }
    return AudioCaptureProviderType::SYSTEM_AUDIO; // Default fallback
}

IAudioCaptureProvider* AudioCaptureManager::FindProvider(AudioCaptureProviderType type) const {
    for (const auto& provider : providers_) {
        if (provider->GetProviderType() == type) {
            return provider.get();
        }
    }
    return nullptr;
}

IAudioCaptureProvider* AudioCaptureManager::SelectBestProvider() {
    // First, try the preferred provider if available
    IAudioCaptureProvider* preferred = FindProvider(preferred_provider_type_);
    if (preferred && preferred->IsAvailable()) {
        LOG_DEBUG("[AudioCaptureManager] Selected preferred provider: " + preferred->GetProviderName());
        return preferred;
    }
    
    // If no preferred provider, look for a provider marked as default
    for (const auto& provider : providers_) {
        if (provider->IsAvailable() && provider->GetProviderInfo().is_default) {
            LOG_DEBUG("[AudioCaptureManager] Selected default provider: " + provider->GetProviderName());
            return provider.get();
        }
    }
    
    // Fall back to any available provider, prioritizing by order
    IAudioCaptureProvider* best_provider = nullptr;
    int best_order = INT_MAX;
    
    for (const auto& provider : providers_) {
        if (provider->IsAvailable()) {
            const auto& info = provider->GetProviderInfo();
            if (info.order < best_order) {
                best_provider = provider.get();
                best_order = info.order;
            }
        }
    }
    
    if (best_provider) {
        LOG_DEBUG("[AudioCaptureManager] Selected fallback provider: " + best_provider->GetProviderName());
        return best_provider;
    }
    
    LOG_ERROR("[AudioCaptureManager] No available providers found");
    return nullptr;
}

bool AudioCaptureManager::StartCapture(const Listeningway::Configuration& config, 
                                      std::atomic_bool& running, 
                                      std::thread& thread, 
                                      std::mutex& data_mutex, 
                                      AudioAnalysisData& data) {
    if (!initialized_ || !current_provider_) {
        LOG_ERROR("[AudioCaptureManager] Cannot start capture - not initialized or no provider");
        return false;
    }
    
    LOG_DEBUG("[AudioCaptureManager] Starting capture with provider: " + current_provider_->GetProviderName());
    return current_provider_->StartCapture(config, running, thread, data_mutex, data);
}

void AudioCaptureManager::StopCapture(std::atomic_bool& running, std::thread& thread) {
    if (current_provider_) {
        LOG_DEBUG("[AudioCaptureManager] Stopping capture");
        current_provider_->StopCapture(running, thread);
    }
}

void AudioCaptureManager::CheckAndRestartCapture(const Listeningway::Configuration& config, 
                                                 std::atomic_bool& running, 
                                                 std::thread& thread, 
                                                 std::mutex& data_mutex, 
                                                 AudioAnalysisData& data) {
    if (!current_provider_) {
        return;
    }
    
    // Check if current provider needs restart
    if (current_provider_->ShouldRestart()) {
        LOG_DEBUG("[AudioCaptureManager] Provider requesting restart");
        StopCapture(running, thread);
        current_provider_->ResetRestartFlags();
        StartCapture(config, running, thread, data_mutex, data);
        return;
    }
    
    // Check if we should switch to a different provider
    IAudioCaptureProvider* best_provider = SelectBestProvider();    if (best_provider && best_provider != current_provider_) {
        LOG_INFO("[AudioCaptureManager] Switching to better provider: " + best_provider->GetProviderName());
        StopCapture(running, thread);
        current_provider_ = best_provider;
        StartCapture(config, running, thread, data_mutex, data);
    }
}
