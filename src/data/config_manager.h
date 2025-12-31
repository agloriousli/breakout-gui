#pragma once

#include <QString>
#include <QStringList>

namespace breakout {

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

    bool loadConfig(const QString& baseName, GameConfig& outConfig, QString* error = nullptr) const;
    bool saveConfig(const QString& baseName, const GameConfig& config, QString* error = nullptr) const;
    QStringList validate(const GameConfig& config) const;
};

} // namespace breakout
