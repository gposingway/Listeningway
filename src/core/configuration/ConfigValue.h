#pragma once
#include <string>
#include <variant>

/**
 * @brief Configuration value that can hold different types
 */
class ConfigValue {
public:
    using ValueType = std::variant<bool, int, float, std::string, size_t>;
    
    ConfigValue() = default;
    ConfigValue(bool value) : m_value(value) {}
    ConfigValue(int value) : m_value(value) {}
    ConfigValue(float value) : m_value(value) {}
    ConfigValue(size_t value) : m_value(value) {}
    ConfigValue(const std::string& value) : m_value(value) {}
    ConfigValue(const char* value) : m_value(std::string(value)) {}
    
    template<typename T>
    T Get() const {
        return std::get<T>(m_value);
    }
    
    template<typename T>
    bool IsType() const {
        return std::holds_alternative<T>(m_value);
    }
    
    std::string ToString() const;
    
    bool operator==(const ConfigValue& other) const {
        return m_value == other.m_value;
    }
    
    bool operator!=(const ConfigValue& other) const {
        return !(*this == other);
    }
    
private:
    ValueType m_value;
};
