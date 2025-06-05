#pragma once
#include <string>
#include <variant>
#include <functional>

namespace Listeningway {

/**
 * @brief Error information for Result<T, Error> pattern
 */
struct Error {
    enum class Type {
        COM_ERROR,          // HRESULT failures
        AUDIO_DEVICE_ERROR, // Device not found/unavailable
        CONFIGURATION_ERROR,// Invalid configuration
        THREAD_ERROR,       // Threading issues
        MEMORY_ERROR,       // Allocation failures
        UNKNOWN_ERROR       // Catch-all
    };
    
    Type type;
    std::string message;
    int error_code = 0;  // HRESULT or system error code
    
    Error(Type t, const std::string& msg, int code = 0)
        : type(t), message(msg), error_code(code) {}
    
    // Helper constructors for common error types
    static Error ComError(const std::string& msg, HRESULT hr) {
        return Error(Type::COM_ERROR, msg + " (HRESULT: " + std::to_string(hr) + ")", hr);
    }
    
    static Error AudioDeviceError(const std::string& msg) {
        return Error(Type::AUDIO_DEVICE_ERROR, msg);
    }
    
    static Error ConfigurationError(const std::string& msg) {
        return Error(Type::CONFIGURATION_ERROR, msg);
    }
    
    static Error ThreadError(const std::string& msg) {
        return Error(Type::THREAD_ERROR, msg);
    }
};

/**
 * @brief Result<T, Error> type for standardized error handling
 * Replaces mixed exception/return code/logging patterns
 */
template<typename T>
class Result {
private:
    std::variant<T, Error> data_;
    
public:
    // Success constructor
    Result(T&& value) : data_(std::move(value)) {}
    Result(const T& value) : data_(value) {}
    
    // Error constructor
    Result(Error&& error) : data_(std::move(error)) {}
    Result(const Error& error) : data_(error) {}
    
    // Status checking
    bool IsOk() const { return std::holds_alternative<T>(data_); }
    bool IsError() const { return std::holds_alternative<Error>(data_); }
    
    // Value access (only call if IsOk() is true)
    const T& Value() const { return std::get<T>(data_); }
    T& Value() { return std::get<T>(data_); }
    
    // Error access (only call if IsError() is true)
    const Error& GetError() const { return std::get<Error>(data_); }
    
    // Unwrap with default value
    T ValueOr(const T& default_value) const {
        return IsOk() ? Value() : default_value;
    }
    
    // Monadic operations
    template<typename F>
    auto Map(F&& func) -> Result<decltype(func(Value()))> {
        if (IsOk()) {
            return Result<decltype(func(Value()))>(func(Value()));
        }
        return Result<decltype(func(Value()))>(GetError());
    }
    
    template<typename F>
    auto FlatMap(F&& func) -> decltype(func(Value())) {
        if (IsOk()) {
            return func(Value());
        }
        return decltype(func(Value()))(GetError());
    }
    
    // Execute function only if result is Ok
    template<typename F>
    Result& OnSuccess(F&& func) {
        if (IsOk()) {
            func(Value());
        }
        return *this;
    }
    
    // Execute function only if result is Error
    template<typename F>
    Result& OnError(F&& func) {
        if (IsError()) {
            func(GetError());
        }
        return *this;
    }
};

// Helper macros for converting HRESULT to Result
#define RETURN_IF_FAILED(hr, msg) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            return Error::ComError(msg, _hr); \
        } \
    } while(0)

#define TRY_ASSIGN(var, result_expr) \
    do { \
        auto _result = (result_expr); \
        if (_result.IsError()) { \
            return _result.GetError(); \
        } \
        var = _result.Value(); \
    } while(0)

} // namespace Listeningway
