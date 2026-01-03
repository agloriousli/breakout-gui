#pragma once

#include <QWidget>
#include <vector>
#include <string>
#include <map>

#include "../data/endgame_manager.h"

class QTableWidget;
class QSpinBox;
class QPushButton;
class QComboBox;
class QLabel;

namespace breakout {

class EndgameEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit EndgameEditorWidget(QWidget* parent = nullptr);

    breakout::EndgameSnapshot buildSnapshot(const QString& name) const;
    void loadSnapshot(const breakout::EndgameSnapshot& snap);

signals:
    void saveRequested();

private slots:
    void handleCellClick(int row, int col);
    void handleResize();
    void setBrushNormal();
    void setBrushDurable();
    void setBrushIndestructible();
    void setBrushErase();
    void clearGrid();
    void onPowerupChanged(int index);

private:
    enum class Brush { Normal, Durable, Indestructible, Erase };

    void refreshTable();
    void applyBrush(int row, int col);
    char brushChar() const;
    bool validateEndgameDimensions(int& width, int& height) const;
    void updateCellDisplay(int row, int col);

    QTableWidget* table_ {nullptr};
    QSpinBox* widthSpin_ {nullptr};
    QSpinBox* heightSpin_ {nullptr};
    QSpinBox* levelSpin_ {nullptr};
    QSpinBox* livesSpin_ {nullptr};
    QComboBox* powerupCombo_ {nullptr};
    QLabel* brushLabel_ {nullptr};
    Brush brush_ {Brush::Normal};
    std::vector<std::string> grid_;
    // Map from (row, col) to assigned powerup: -1=none, 0-4=powerup type
    std::map<std::pair<int, int>, int> powerupMap_;
    int currentPowerup_ {-1};  // -1=none/random
};

} // namespace breakout
