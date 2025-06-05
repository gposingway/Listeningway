#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "../core/result.h"

namespace Listeningway::Audio {

/**
 * @brief RAII wrapper for COM interfaces with automatic resource management
 * Eliminates manual Release() calls and reduces resource leaks
 */
template<typename T>
class ComPtr {
private:
    T* ptr_;
    
public:
    ComPtr() : ptr_(nullptr) {}
    
    explicit ComPtr(T* p) : ptr_(p) {}
    
    ~ComPtr() {
        Reset();
    }
    
    // Move semantics
    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            Reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    // Disable copying to prevent double-release
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    
    // Access operators
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* Get() const { return ptr_; }
    T** GetAddressOf() { Reset(); return &ptr_; }
    
    // Release current pointer and optionally assign new one
    void Reset(T* p = nullptr) {
        if (ptr_) {
            ptr_->Release();
        }
        ptr_ = p;
    }
    
    // Detach pointer (caller takes ownership)
    T* Detach() {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }
    
    // Boolean conversion
    explicit operator bool() const { return ptr_ != nullptr; }
    
    // For use with APIs that expect T**
    T** ReleaseAndGetAddressOf() {
        Reset();
        return &ptr_;
    }
};

/**
 * @brief RAII wrapper for WASAPI resources
 * Replaces manual WasapiResources destructors across providers
 */
class WasapiResourceManager {
public:
    ComPtr<IMMDevice> device;
    ComPtr<IAudioClient> audio_client;
    ComPtr<IAudioCaptureClient> capture_client;
    WAVEFORMATEX* wave_format = nullptr;
    HANDLE audio_event = nullptr;
    
    WasapiResourceManager() = default;
    
    ~WasapiResourceManager() {
        Cleanup();
    }
    
    // Disable copying
    WasapiResourceManager(const WasapiResourceManager&) = delete;
    WasapiResourceManager& operator=(const WasapiResourceManager&) = delete;
    
    // Move semantics
    WasapiResourceManager(WasapiResourceManager&& other) noexcept
        : device(std::move(other.device))
        , audio_client(std::move(other.audio_client))
        , capture_client(std::move(other.capture_client))
        , wave_format(other.wave_format)
        , audio_event(other.audio_event)
    {
        other.wave_format = nullptr;
        other.audio_event = nullptr;
    }
    
    WasapiResourceManager& operator=(WasapiResourceManager&& other) noexcept {
        if (this != &other) {
            Cleanup();
            device = std::move(other.device);
            audio_client = std::move(other.audio_client);
            capture_client = std::move(other.capture_client);
            wave_format = other.wave_format;
            audio_event = other.audio_event;
            other.wave_format = nullptr;
            other.audio_event = nullptr;
        }
        return *this;
    }
    
    /**
     * @brief Initialize WASAPI resources for system audio capture
     */
    Result<void> InitializeSystemCapture(IMMDeviceEnumerator* enumerator);
    
    /**
     * @brief Initialize WASAPI resources for process-specific capture
     */
    Result<void> InitializeProcessCapture(IMMDeviceEnumerator* enumerator, DWORD target_pid = 0);
    
    /**
     * @brief Start the audio capture session
     */
    Result<void> StartCapture();
    
    /**
     * @brief Stop the audio capture session
     */
    void StopCapture();
    
    /**
     * @brief Get available audio data
     */
    Result<std::pair<BYTE*, UINT32>> GetAudioData();
    
    /**
     * @brief Release audio buffer after processing
     */
    void ReleaseAudioBuffer(UINT32 frames_read);
    
private:
    void Cleanup();
    Result<void> SetupAudioClient(DWORD stream_flags);
};

/**
 * @brief RAII wrapper for Windows handles
 */
class HandleWrapper {
private:
    HANDLE handle_;
    
public:
    HandleWrapper() : handle_(nullptr) {}
    explicit HandleWrapper(HANDLE h) : handle_(h) {}
    
    ~HandleWrapper() {
        Reset();
    }
    
    // Move semantics
    HandleWrapper(HandleWrapper&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    
    HandleWrapper& operator=(HandleWrapper&& other) noexcept {
        if (this != &other) {
            Reset();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    // Disable copying
    HandleWrapper(const HandleWrapper&) = delete;
    HandleWrapper& operator=(const HandleWrapper&) = delete;
    
    HANDLE Get() const { return handle_; }
    HANDLE* GetAddressOf() { return &handle_; }
    
    void Reset(HANDLE h = nullptr) {
        if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
        handle_ = h;
    }
    
    explicit operator bool() const { 
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; 
    }
    
    HANDLE Detach() {
        HANDLE temp = handle_;
        handle_ = nullptr;
        return temp;
    }
};

} // namespace Listeningway::Audio
