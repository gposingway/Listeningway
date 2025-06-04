#include "ConfigurationManager.h"
#include "../logging.h"

ConfigurationManager& ConfigurationManager::Instance() {
    static ConfigurationManager instance;
    return instance;
}

ConfigurationManager::ConfigurationManager() {
    // Load configuration on startup
    Load();
}

const Listeningway::Configuration& ConfigurationManager::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

Listeningway::Configuration& ConfigurationManager::GetConfig() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void ConfigurationManager::NotifyConfigurationChanged() {
    // Validate configuration after any change
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.Validate();
    }
    
    // Notify listeners
    NotifyListeners();
}

bool ConfigurationManager::Save(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.Save(filename.empty() ? "listeningway_config.json" : filename);
}

bool ConfigurationManager::Load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool result = m_config.Load(filename.empty() ? "listeningway_config.json" : filename);
    
    // Always validate after loading
    m_config.Validate();
    
    return result;
}

void ConfigurationManager::ResetToDefaults() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.ResetToDefaults();
    }
    
    NotifyListeners();
}

void ConfigurationManager::AddChangeListener(std::shared_ptr<IConfigurationChangeListener> listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.push_back(listener);
}

void ConfigurationManager::RemoveChangeListener(std::shared_ptr<IConfigurationChangeListener> listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.erase(
        std::remove_if(m_listeners.begin(), m_listeners.end(),
            [&listener](const std::weak_ptr<IConfigurationChangeListener>& weak_listener) {
                return weak_listener.expired() || weak_listener.lock() == listener;
            }), 
        m_listeners.end());
}

void ConfigurationManager::NotifyListeners() {
    std::vector<std::shared_ptr<IConfigurationChangeListener>> listeners_copy;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Copy listeners and remove expired ones
        for (auto it = m_listeners.begin(); it != m_listeners.end();) {
            if (auto listener = it->lock()) {
                listeners_copy.push_back(listener);
                ++it;
            } else {
                it = m_listeners.erase(it);
            }
        }
    }
    
    // Notify listeners outside the lock to avoid deadlocks
    for (auto& listener : listeners_copy) {
        try {
            listener->OnConfigurationChanged("*", ConfigValue(), ConfigValue());
        } catch (const std::exception& e) {
            LOG_ERROR("[ConfigurationManager] Exception in listener notification: " + std::string(e.what()));
        }
    }
}
