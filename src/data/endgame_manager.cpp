#include "endgame_manager.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QObject>
#include <QRegularExpression>

#include <algorithm>

using namespace std;

namespace breakout {
namespace {
QString boolToString(bool v) { return v ? QStringLiteral("1") : QStringLiteral("0"); }
bool stringToBool(const QString& s) { return s.trimmed() == QStringLiteral("1"); }

QString brickTypeToString(BrickType t) {
    switch (t) {
    case BrickType::Normal: return QStringLiteral("@");
    case BrickType::Durable: return QStringLiteral("#");
    case BrickType::Indestructible: return QStringLiteral("*");
    }
    return QStringLiteral("@");
}

BrickType stringToBrickType(const QString& s) {
    if (s == QStringLiteral("@")) return BrickType::Normal;
    if (s == QStringLiteral("#")) return BrickType::Durable;
    if (s == QStringLiteral("*")) return BrickType::Indestructible;
    return BrickType::Normal;
}
} // namespace

// Helper to ensure name validity: A-Z, a-z, 0-9, underscore only
static bool isValidName(const QString& baseName) {
    static const QRegularExpression kNameRe(QStringLiteral("^[A-Za-z0-9_]+$"));
    return kNameRe.match(baseName).hasMatch();
}

QStringList EndgameManager::validate(const QString& baseName, const EndgameSnapshot& state) const {
    QStringList errors;
    const QString trimmed = baseName.trimmed();
    if (trimmed.isEmpty()) {
        errors << QObject::tr("Endgame name cannot be empty");
    } else if (!isValidName(trimmed)) {
        errors << QObject::tr("Endgame name may contain only letters, digits, and underscores");
    }
    if (state.bounds.width <= 0 || state.bounds.height <= 0) {
        errors << QObject::tr("Map size must be positive");
    }
    // Optional reasonable upper bound to avoid absurdly large saved maps
    if (state.bounds.width > 5000 || state.bounds.height > 5000) {
        errors << QObject::tr("Map size is too large");
    }
    if (state.level < 1) {
        errors << QObject::tr("Starting level must be >= 1");
    }
    return errors;
}

QString EndgameManager::filePathFor(const QString& baseName) const {
    return QStringLiteral("endgames/%1.end").arg(baseName);
}

bool EndgameManager::endgameExists(const QString& baseName) const {
    QFile f(filePathFor(baseName));
    return f.exists();
}

QStringList EndgameManager::listEndgames() const {
    QDir dir(QStringLiteral("endgames"));
    if (!dir.exists()) return {};
    QStringList files = dir.entryList(QStringList() << QStringLiteral("*.end"), QDir::Files, QDir::Name);
    // strip extensions
    for (QString& f : files) {
        if (f.endsWith(QStringLiteral(".end"))) {
            f.chop(4);
        }
    }
    return files;
}

bool EndgameManager::saveEndgame(const QString& baseName,
                                 const EndgameSnapshot& state,
                                 bool overwrite,
                                 QString* error) const {
    EndgameSnapshot tmp = state;
    QStringList errors = validate(baseName, tmp);
    if (!errors.isEmpty()) {
        if (error) *error = errors.join('\n');
        return false;
    }

    QDir().mkpath(QStringLiteral("endgames"));
    QString filePath = filePathFor(baseName);
    if (!overwrite && QFile::exists(filePath)) {
        if (error) *error = QObject::tr("Endgame '%1' already exists").arg(baseName);
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = QObject::tr("Cannot write endgame file: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out << baseName << "\n";
    out << QString::fromStdString(tmp.configName) << "\n";
    out << tmp.configBallSpeed << " " << tmp.configRandomSeed << " " << tmp.configStartingLevel << "\n";
    out << tmp.level << " " << tmp.score << " " << tmp.lives << "\n";
    out << tmp.bounds.x << " " << tmp.bounds.y << " " << tmp.bounds.width << " " << tmp.bounds.height << "\n";
    out << tmp.ball.position.x() << " " << tmp.ball.position.y() << " "
        << tmp.ball.velocity.x() << " " << tmp.ball.velocity.y() << " " << tmp.ball.radius << "\n";
    out << tmp.paddle.position.x() << " " << tmp.paddle.position.y() << " "
        << tmp.paddle.width << " " << tmp.paddle.height << "\n";
    out << boolToString(tmp.ballAttached) << "\n";
    out << tmp.bricks.size() << "\n";
    for (const auto& b : tmp.bricks) {
        out << brickTypeToString(b.type) << " " << b.hitsRemaining << " "
            << b.bounds.x << " " << b.bounds.y << " " << b.bounds.width << " " << b.bounds.height << " "
            << boolToString(b.destroyed) << " " << b.assignedPowerup << "\n";
    }
    return true;
}

bool EndgameManager::loadEndgame(const QString& baseName, EndgameSnapshot& outState, QString* error) const {
    QString filePath = filePathFor(baseName);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = QObject::tr("Cannot open endgame file: %1").arg(filePath);
        return false;
    }

    QTextStream in(&file);
    auto readLineTokens = [&](int expectedTokens, QString* err) -> QStringList {
        if (in.atEnd()) {
            if (err) *err = QObject::tr("Unexpected end of file in %1").arg(filePath);
            return {};
        }
        QStringList parts = in.readLine().split(' ', Qt::SkipEmptyParts);
        if (parts.size() < expectedTokens) {
            if (err) *err = QObject::tr("Malformed line in %1").arg(filePath);
            return {};
        }
        return parts;
    };

    QString err;
    QString nameLine = in.readLine();
    if (nameLine.isNull()) { if (error) *error = QObject::tr("Missing name"); return false; }
    QString configLine = in.readLine();
    if (configLine.isNull()) { if (error) *error = QObject::tr("Missing config name"); return false; }

    QStringList lsl = readLineTokens(3, &err);
    if (!err.isEmpty()) { if (error) *error = err; return false; }
    int level = std::max(1, lsl[0].toInt());
    int score = std::max(0, lsl[1].toInt());
    int lives = std::max(1, lsl[2].toInt());

    QStringList cfgLine = readLineTokens(3, &err);
    if (!err.isEmpty()) { if (error) *error = err; return false; }
    int cfgSpeed = cfgLine[0].toInt();
    int cfgSeed = cfgLine[1].toInt();
    int cfgStart = cfgLine[2].toInt();

    QStringList bdl = readLineTokens(4, &err);
    if (!err.isEmpty()) { if (error) *error = err; return false; }
    Rect bounds { bdl[0].toDouble(), bdl[1].toDouble(), bdl[2].toDouble(), bdl[3].toDouble() };

    QStringList ballLine = readLineTokens(5, &err);
    if (!err.isEmpty()) { if (error) *error = err; return false; }
    BallState ballState {
        { ballLine[0].toDouble(), ballLine[1].toDouble() },
        { ballLine[2].toDouble(), ballLine[3].toDouble() },
        ballLine[4].toDouble()
    };

    QStringList padLine = readLineTokens(4, &err);
    if (!err.isEmpty()) { if (error) *error = err; return false; }
    PaddleState padState {
        { padLine[0].toDouble(), padLine[1].toDouble() },
        padLine[2].toDouble(),
        padLine[3].toDouble()
    };

    QString attachedLine = in.readLine();
    if (attachedLine.isNull()) { if (error) *error = QObject::tr("Missing attached flag"); return false; }
    bool attached = stringToBool(attachedLine);

    QStringList countLine = readLineTokens(1, &err);
    if (!err.isEmpty()) { if (error) *error = err; return false; }
    int brickCount = countLine[0].toInt();
    std::vector<BrickState> bricks;
    bricks.reserve(std::max(0, brickCount));
    for (int i = 0; i < brickCount; ++i) {
        // Support legacy saves with 6 tokens (without destroyed flag). If the 7th token is absent, default to not destroyed.
        QStringList parts = readLineTokens(6, &err);
        if (!err.isEmpty()) { if (error) *error = err; return false; }
        if (parts.size() < 6) {
            if (error) *error = QObject::tr("Malformed brick line in %1").arg(filePath);
            return false;
        }
        BrickState bs;
        bs.type = stringToBrickType(parts[0]);
        bs.hitsRemaining = parts[1].toInt();
        bs.bounds = { parts[2].toDouble(), parts[3].toDouble(), parts[4].toDouble(), parts[5].toDouble() };
        if (parts.size() >= 7) {
            bs.destroyed = stringToBool(parts[6]);
        }
        if (parts.size() >= 8) {
            bs.assignedPowerup = parts[7].toInt();
        }
        bricks.push_back(bs);
    }

    EndgameSnapshot loaded;
    loaded.name = nameLine.toStdString();
    loaded.configName = configLine.toStdString();
    loaded.level = level;
    loaded.score = score;
    loaded.lives = lives;
    loaded.configBallSpeed = cfgSpeed;
    loaded.configRandomSeed = cfgSeed;
    loaded.configStartingLevel = cfgStart;
    loaded.bounds = bounds;
    loaded.ball = ballState;
    loaded.paddle = padState;
    loaded.ballAttached = attached;
    loaded.bricks = std::move(bricks);

    QStringList errors = validate(baseName, loaded);
    if (!errors.isEmpty()) {
        if (error) *error = errors.join('\n');
        return false;
    }

    outState = std::move(loaded);
    return true;
}

} // namespace breakout
