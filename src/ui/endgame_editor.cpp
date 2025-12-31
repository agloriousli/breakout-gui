#include "endgame_editor.h"

#include <algorithm>

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {
constexpr int kMinSize = 8;
constexpr int kMaxSize = 20;
constexpr double kBrickW = 48.0;
constexpr double kBrickH = 20.0;
constexpr double kOffsetX = 32.0;
constexpr double kOffsetY = 40.0;
// Max playfield dimensions to prevent oversized endgames
constexpr double kMaxPlayfieldWidth = 1400.0;  // ~960px usable space
constexpr double kMaxPlayfieldHeight = 600.0;  // ~460px usable space
}

namespace breakout {

EndgameEditorWidget::EndgameEditorWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    auto* topRow = new QHBoxLayout();
    widthSpin_ = new QSpinBox(this);
    heightSpin_ = new QSpinBox(this);
    levelSpin_ = new QSpinBox(this);
    widthSpin_->setRange(kMinSize, kMaxSize);
    heightSpin_->setRange(kMinSize, kMaxSize);
    levelSpin_->setRange(1, 999);
    widthSpin_->setValue(12);
    heightSpin_->setValue(12);
    levelSpin_->setValue(1);

    auto* normalBtn = new QPushButton(tr("@"), this);
    auto* durableBtn = new QPushButton(tr("#"), this);
    auto* indestructibleBtn = new QPushButton(tr("*"), this);
    auto* eraseBtn = new QPushButton(tr("Erase"), this);
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    brushLabel_ = new QLabel(tr("Brush: @"), this);

    topRow->addWidget(new QLabel(tr("Width"), this));
    topRow->addWidget(widthSpin_);
    topRow->addWidget(new QLabel(tr("Height"), this));
    topRow->addWidget(heightSpin_);
    topRow->addWidget(new QLabel(tr("Start Level"), this));
    topRow->addWidget(levelSpin_);
    topRow->addSpacing(12);
    topRow->addWidget(normalBtn);
    topRow->addWidget(durableBtn);
    topRow->addWidget(indestructibleBtn);
    topRow->addWidget(eraseBtn);
    topRow->addWidget(clearBtn);
    topRow->addWidget(brushLabel_);

    table_ = new QTableWidget(this);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table_->horizontalHeader()->hide();
    table_->verticalHeader()->hide();
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setFocusPolicy(Qt::NoFocus);

    layout->addLayout(topRow);
    layout->addWidget(table_);

    connect(table_, &QTableWidget::cellClicked, this, &EndgameEditorWidget::handleCellClick);
    connect(widthSpin_, &QSpinBox::valueChanged, this, &EndgameEditorWidget::handleResize);
    connect(heightSpin_, &QSpinBox::valueChanged, this, &EndgameEditorWidget::handleResize);

    connect(normalBtn, &QPushButton::clicked, this, &EndgameEditorWidget::setBrushNormal);
    connect(durableBtn, &QPushButton::clicked, this, &EndgameEditorWidget::setBrushDurable);
    connect(indestructibleBtn, &QPushButton::clicked, this, &EndgameEditorWidget::setBrushIndestructible);
    connect(eraseBtn, &QPushButton::clicked, this, &EndgameEditorWidget::setBrushErase);
    connect(clearBtn, &QPushButton::clicked, this, &EndgameEditorWidget::clearGrid);

    handleResize();
}

void EndgameEditorWidget::handleResize() {
    int rows = heightSpin_->value();
    int cols = widthSpin_->value();

    std::vector<std::string> newGrid(rows, std::string(static_cast<size_t>(cols), ' '));
    int oldRows = static_cast<int>(grid_.size());
    int oldCols = oldRows > 0 ? static_cast<int>(grid_[0].size()) : 0;
    int copyRows = std::min(rows, oldRows);
    int copyCols = std::min(cols, oldCols);
    for (int r = 0; r < copyRows; ++r) {
        for (int c = 0; c < copyCols; ++c) {
            newGrid[static_cast<size_t>(r)][static_cast<size_t>(c)] = grid_[static_cast<size_t>(r)][static_cast<size_t>(c)];
        }
    }
    grid_ = std::move(newGrid);

    table_->setRowCount(rows);
    table_->setColumnCount(cols);
    for (int c = 0; c < cols; ++c) {
        table_->setColumnWidth(c, 24);
    }
    for (int r = 0; r < rows; ++r) {
        table_->setRowHeight(r, 24);
        for (int c = 0; c < cols; ++c) {
            auto* item = new QTableWidgetItem();
            char ch = grid_[static_cast<size_t>(r)][static_cast<size_t>(c)];
            item->setText(QString(ch));
            table_->setItem(r, c, item);
        }
    }
}

void EndgameEditorWidget::handleCellClick(int row, int col) {
    applyBrush(row, col);
}

void EndgameEditorWidget::applyBrush(int row, int col) {
    if (row < 0 || col < 0 || row >= static_cast<int>(grid_.size()) || col >= static_cast<int>(grid_[0].size())) {
        return;
    }
    char ch = brushChar();
    grid_[static_cast<size_t>(row)][static_cast<size_t>(col)] = ch;
    table_->item(row, col)->setText(QString(ch));
}

char EndgameEditorWidget::brushChar() const {
    switch (brush_) {
    case Brush::Normal: return '@';
    case Brush::Durable: return '#';
    case Brush::Indestructible: return '*';
    case Brush::Erase: return ' ';
    }
    return '@';
}

void EndgameEditorWidget::setBrushNormal() { brush_ = Brush::Normal; brushLabel_->setText(tr("Brush: @")); }
void EndgameEditorWidget::setBrushDurable() { brush_ = Brush::Durable; brushLabel_->setText(tr("Brush: #")); }
void EndgameEditorWidget::setBrushIndestructible() { brush_ = Brush::Indestructible; brushLabel_->setText(tr("Brush: *")); }
void EndgameEditorWidget::setBrushErase() { brush_ = Brush::Erase; brushLabel_->setText(tr("Brush: Erase")); }

void EndgameEditorWidget::clearGrid() {
    for (auto& row : grid_) {
        std::fill(row.begin(), row.end(), ' ');
    }
    for (int r = 0; r < table_->rowCount(); ++r) {
        for (int c = 0; c < table_->columnCount(); ++c) {
            table_->item(r, c)->setText(" ");
        }
    }
}

breakout::EndgameSnapshot EndgameEditorWidget::buildSnapshot(const QString& name) const {
    EndgameSnapshot snap;
    snap.name = name.toStdString();
    snap.configName = "editor";
    snap.configBallSpeed = 5;
    snap.configRandomSeed = -1;
    snap.configStartingLevel = levelSpin_->value();
    snap.level = levelSpin_->value();
    snap.score = 0;
    snap.lives = 3;

    int rows = static_cast<int>(grid_.size());
    int cols = rows > 0 ? static_cast<int>(grid_[0].size()) : 0;
    
    // Validate dimensions don't exceed reasonable limits
    validateEndgameDimensions(const_cast<int&>(cols), const_cast<int&>(rows));

    snap.bounds = {kOffsetX, kOffsetY, cols * kBrickW, rows * kBrickH};
    snap.ball.position = {snap.bounds.x + snap.bounds.width * 0.5, snap.bounds.y + snap.bounds.height - 40.0};
    snap.ball.velocity = {0.0, -260.0};
    snap.ball.radius = 6.0;
    snap.paddle.position = {snap.bounds.x + snap.bounds.width * 0.5 - 40.0, snap.bounds.bottom() - 12.0 - 16.0};
    snap.paddle.width = 80.0;
    snap.paddle.height = 16.0;
    snap.ballAttached = true;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            char ch = grid_[static_cast<size_t>(r)][static_cast<size_t>(c)];
            if (ch == ' ') continue;
            BrickState bs;
            bs.type = (ch == '@') ? BrickType::Normal : (ch == '#') ? BrickType::Durable : BrickType::Indestructible;
            bs.hitsRemaining = (bs.type == BrickType::Durable) ? 2 : 1;
            bs.bounds = {kOffsetX + c * kBrickW, kOffsetY + r * kBrickH, kBrickW, kBrickH};
            snap.bricks.push_back(bs);
        }
    }
    return snap;
}

void EndgameEditorWidget::loadSnapshot(const breakout::EndgameSnapshot& snap) {
    int cols = std::clamp(static_cast<int>(std::round(snap.bounds.width / kBrickW)), kMinSize, kMaxSize);
    int rows = std::clamp(static_cast<int>(std::round(snap.bounds.height / kBrickH)), kMinSize, kMaxSize);
    
    // Validate loaded dimensions don't exceed safe limits
    validateEndgameDimensions(cols, rows);
    
    widthSpin_->setValue(cols);
    heightSpin_->setValue(rows);
    levelSpin_->setValue(std::max(1, snap.level));

    grid_.assign(rows, std::string(static_cast<size_t>(cols), ' '));
    for (const auto& b : snap.bricks) {
        int c = static_cast<int>(std::round((b.bounds.x - kOffsetX) / kBrickW));
        int r = static_cast<int>(std::round((b.bounds.y - kOffsetY) / kBrickH));
        if (r >= 0 && r < rows && c >= 0 && c < cols) {
            char ch = '@';
            if (b.type == BrickType::Durable) ch = '#';
            else if (b.type == BrickType::Indestructible) ch = '*';
            grid_[static_cast<size_t>(r)][static_cast<size_t>(c)] = ch;
        }
    }

    handleResize();
}

bool EndgameEditorWidget::validateEndgameDimensions(int& width, int& height) const {
    // Check if playfield dimensions exceed safe limits
    double playfieldWidth = width * kBrickW;
    double playfieldHeight = height * kBrickH;
    
    bool valid = true;
    if (playfieldWidth > kMaxPlayfieldWidth) {
        valid = false;
        width = static_cast<int>(kMaxPlayfieldWidth / kBrickW);
    }
    if (playfieldHeight > kMaxPlayfieldHeight) {
        valid = false;
        height = static_cast<int>(kMaxPlayfieldHeight / kBrickH);
    }
    return valid;
}

} // namespace breakout
