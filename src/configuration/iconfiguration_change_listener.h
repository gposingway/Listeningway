#pragma once
#include <string>
#include <functional>

/**
 * @brief Interface for objects that want to be notified of configuration changes
 */
class IConfigurationChangeListener {
public:
    virtual ~IConfigurationChangeListener() = default;
    
    /**
     * @brief Called when a configuration setting has changed
     * @param settingName The name of the setting that changed
     * @param oldValue The previous value (as string for simplicity)
     * @param newValue The new value (as string for simplicity)
     */
    virtual void OnConfigurationChanged(const std::string& settingName, 
                                      const std::string& oldValue, 
                                      const std::string& newValue) = 0;
};
