// ---------------------------------------------
// Audio Capture Manager Implementation
// Manages audio capture providers and handles provider selection
// ---------------------------------------------
#include "audio_capture_manager.h"
#include "providers/system_audio_provider.h"
#include "providers/process_audio_provider.h"
#include "../utils/logging.h"

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
    LOG_INFO("[AudioCaptureManager] Set preferred provider to: " + provider->GetProviderName());
    
    // If we're currently using a different provider, switch to the preferred one
    if (!current_provider_ || current_provider_->GetProviderType() != type) {
        current_provider_ = provider;
        LOG_INFO("[AudioCaptureManager] Switched to preferred provider: " + current_provider_->GetProviderName());
    }
    
    return true;
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
    
    // Fall back to any available provider, prioritizing system audio
    for (const auto& provider : providers_) {
        if (provider->IsAvailable()) {
            LOG_DEBUG("[AudioCaptureManager] Selected fallback provider: " + provider->GetProviderName());
            return provider.get();
        }
    }
    
    LOG_ERROR("[AudioCaptureManager] No available providers found");
    return nullptr;
}

bool AudioCaptureManager::StartCapture(const AudioAnalysisConfig& config, 
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

void AudioCaptureManager::CheckAndRestartCapture(const AudioAnalysisConfig& config, 
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
