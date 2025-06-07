#include "config_value.h"
#include <sstream>

std::string ConfigValue::ToString() const {
    return std::visit([](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<T, bool>) {
            return value ? "true" : "false";
        } else {
            return std::to_string(value);
        }
    }, m_value);
}
