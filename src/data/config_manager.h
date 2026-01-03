#pragma once

#include <QString>
#include <QStringList>

namespace breakout {

// Name of the protected default configuration
constexpr const char* kDefaultConfigName = "default";

struct GameConfig {
    int ballSpeed {5};      // 1-10
    int randomSeed {-1};    // -1 = time-based
    int startingLevel {1};
    QString playerName;
    QString name; // config file base name (without extension)
};

class ConfigManager {
public:
    ConfigManager() = default;

    // Returns the default config with standard settings
    static GameConfig defaultConfig();
    
    // Check if a config name is the protected default
    static bool isDefaultConfig(const QString& name);
    
    // Ensure the default config file exists (creates if missing)
    void ensureDefaultConfigExists() const;

    bool loadConfig(const QString& baseName, GameConfig& outConfig, QString* error = nullptr) const;
    bool saveConfig(const QString& baseName, const GameConfig& config, QString* error = nullptr) const;
    QStringList validate(const GameConfig& config) const;
};

} // namespace breakout
