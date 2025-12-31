#pragma once

#include <QString>
#include <QStringList>

#include "core/game/endgame_state.h"

namespace breakout {

class EndgameManager {
public:
    EndgameManager() = default;

    // Persist an endgame. When overwrite is false and the file exists, the call fails with an error.
    bool saveEndgame(const QString& baseName,
                     const EndgameSnapshot& state,
                     bool overwrite,
                     QString* error = nullptr) const;

    // Convenience overload: defaults to no-overwrite (preserves legacy signature behavior).
    bool saveEndgame(const QString& baseName, const EndgameSnapshot& state, QString* error = nullptr) const {
        return saveEndgame(baseName, state, /*overwrite=*/false, error);
    }

    // Load an endgame into outState.
    bool loadEndgame(const QString& baseName, EndgameSnapshot& outState, QString* error = nullptr) const;

    // Validate fields only (no disk access). Returns list of human-readable errors, empty if valid.
    QStringList validate(const QString& baseName, const EndgameSnapshot& state) const;

    // Utility helpers for UI flows
    bool endgameExists(const QString& baseName) const;
    QStringList listEndgames() const;
    QString filePathFor(const QString& baseName) const;
};

} // namespace breakout
