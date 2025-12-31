#include "config_manager.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

namespace breakout {

namespace {
constexpr int kMinSpeed = 1;
constexpr int kMaxSpeed = 10;
}

QStringList ConfigManager::validate(const GameConfig& config) const {
    QStringList errors;
    if (config.ballSpeed < kMinSpeed || config.ballSpeed > kMaxSpeed) {
        errors << QObject::tr("Ball speed must be between %1 and %2").arg(kMinSpeed).arg(kMaxSpeed);
    }
    if (config.startingLevel < 1) {
        errors << QObject::tr("Starting level must be >= 1");
    }
    if (config.name.isEmpty()) {
        errors << QObject::tr("Config name cannot be empty");
    }
    return errors;
}

bool ConfigManager::loadConfig(const QString& baseName, GameConfig& outConfig, QString* error) const {
    QString filePath = QStringLiteral("config/%1.config").arg(baseName);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = QObject::tr("Cannot open config file: %1").arg(filePath);
        return false;
    }

    QTextStream in(&file);
    int lineNo = 0;
    auto readInt = [&](int& target, const QString& what) -> bool {
        if (in.atEnd()) {
            if (error) *error = QObject::tr("Unexpected end of file reading %1").arg(what);
            return false;
        }
        bool ok = false;
        QString line = in.readLine();
        ++lineNo;
        int val = line.toInt(&ok);
        if (!ok) {
            if (error) *error = QObject::tr("Invalid integer for %1 at line %2").arg(what).arg(lineNo);
            return false;
        }
        target = val;
        return true;
    };

    int speed = 0;
    if (!readInt(speed, QObject::tr("ball speed"))) return false;
    int seed = 0;
    if (!readInt(seed, QObject::tr("random seed"))) return false;
    int startLevel = 0;
    if (!readInt(startLevel, QObject::tr("starting level"))) return false;

    GameConfig cfg;
    cfg.name = baseName;
    cfg.ballSpeed = speed;
    cfg.randomSeed = seed;
    cfg.startingLevel = startLevel;
    if (!in.atEnd()) {
        cfg.playerName = in.readLine();
    }

    QStringList errors = validate(cfg);
    if (!errors.isEmpty()) {
        if (error) *error = errors.join('\n');
        return false;
    }

    outConfig = cfg;
    return true;
}

bool ConfigManager::saveConfig(const QString& baseName, const GameConfig& config, QString* error) const {
    GameConfig cfg = config;
    cfg.name = baseName;
    QStringList errors = validate(cfg);
    if (!errors.isEmpty()) {
        if (error) *error = errors.join('\n');
        return false;
    }

    QDir().mkpath(QStringLiteral("config"));
    QString filePath = QStringLiteral("config/%1.config").arg(baseName);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = QObject::tr("Cannot write config file: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out << cfg.ballSpeed << "\n";
    out << cfg.randomSeed << "\n";
    out << cfg.startingLevel << "\n";
    if (!cfg.playerName.isEmpty()) {
        out << cfg.playerName << "\n";
    }
    return true;
}

} // namespace breakout
