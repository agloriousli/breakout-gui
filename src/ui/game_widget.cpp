#include "game_widget.h"

// Standard library includes
#include <algorithm>
#include <cmath>

// Qt includes
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPushButton>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QLinearGradient>
#include <QPolygonF>

using namespace std;
using breakout::Brick;
using breakout::BrickType;
using breakout::Rect;

namespace {
// Region Layout Constants
constexpr int kHudHeight = 60;                   // HUD top bar height
constexpr int kHudPaddingBottom = 10;           // Spacing below HUD
constexpr int kPowerupRegionHeight = 70;        // Combined banner + timer region
constexpr int kBannerHeight = 40;               // Banner animation area (top of region)
constexpr int kTimerHeight = 30;                // Timer text area (bottom of region)

// Total offset from top where playfield starts
constexpr int kPlayfieldTopOffset = kHudHeight + kHudPaddingBottom + kPowerupRegionHeight;
constexpr int kPlayfieldMarginBottom = 40;      // Bottom margin for playfield
constexpr double kMaxDelta = 0.05;              // Max delta time for frame stability

// Helper function to convert custom Rect to Qt rect
QRectF toQRectF(const Rect& r) {
    return QRectF(r.x, r.y, r.width, r.height);
}
// Reposition and optionally scale a saved endgame snapshot to fit the current widget
// viewport while keeping it below the HUD and centered horizontally.
breakout::EndgameSnapshot scaleSnapshotToViewport(const breakout::EndgameSnapshot& state,
                                                  int widgetWidth, int widgetHeight) {
    breakout::EndgameSnapshot snap = state;
    Rect src = snap.bounds;
    if (src.width <= 0.0 || src.height <= 0.0) {
        return snap;
    }

    const int playfieldY = kPlayfieldTopOffset;
    const int playfieldMaxHeight = widgetHeight - playfieldY - kPlayfieldMarginBottom;
    const int playfieldMaxWidth = widgetWidth - 32; // 16px margins each side
    if (playfieldMaxHeight <= 0 || playfieldMaxWidth <= 0) {
        return snap;
    }

    double scale = std::min(1.0, std::min(playfieldMaxWidth / src.width, playfieldMaxHeight / src.height));
    double targetW = src.width * scale;
    double targetH = src.height * scale;
    double targetX = 16 + (playfieldMaxWidth - targetW) * 0.5;
    double targetY = playfieldY + (playfieldMaxHeight - targetH) * 0.5;

    auto transformPoint = [&](const breakout::Vector2D& p) {
        return breakout::Vector2D(targetX + (p.x() - src.x) * scale,
                                  targetY + (p.y() - src.y) * scale);
    };

    snap.bounds = {targetX, targetY, targetW, targetH};
    snap.ball.position = transformPoint(snap.ball.position);
    snap.ball.velocity *= scale;
    snap.ball.radius *= scale;
    snap.paddle.position = transformPoint(snap.paddle.position);
    snap.paddle.width *= scale;
    snap.paddle.height *= scale;

    for (auto& brick : snap.bricks) {
        brick.bounds.x = targetX + (brick.bounds.x - src.x) * scale;
        brick.bounds.y = targetY + (brick.bounds.y - src.y) * scale;
        brick.bounds.width *= scale;
        brick.bounds.height *= scale;
    }

    for (auto& p : snap.powerups) {
        p.position = transformPoint(p.position);
        p.velocity *= scale;
        p.size *= scale;
    }

    return snap;
}
} // namespace

// ============================================================================
// GameWidget Constructor - Initialize game widget and UI components
// ============================================================================
GameWidget::GameWidget(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);

    // Setup game loop timer (60 FPS)
    updateTimer_.setInterval(16);
    connect(&updateTimer_, &QTimer::timeout, this, &GameWidget::tick);

    // Start timers
    frameTimer_.start();
    effectsTimer_.start();

    // Create and style pause button (clean blue style)
    pauseButton_ = new QPushButton(tr("PAUSE"), this);
    auto style = QStringLiteral(R"(QPushButton {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0099FF, stop:1 #0055CC);
        color: #FFFFFF;
        border: 2px solid #0099FF;
        border-radius: 4px;
        padding: 6px 12px;
        font-weight: bold;
        font-size: 11px;
        font-family: 'Courier New', monospace;
    }
    QPushButton:hover {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00BBFF, stop:1 #0077FF);
        border: 2px solid #00BBFF;
    }
    QPushButton:pressed {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0055CC, stop:1 #003388);
        border: 2px solid #0055CC;
    })");
    pauseButton_->setStyleSheet(style);
    pauseButton_->setFocusPolicy(Qt::NoFocus);
    pauseButton_->setMinimumHeight(36);

    // Connect pause button logic
    connect(pauseButton_, &QPushButton::clicked, this, [this]() {
        if (state_ == PlayState::Active || state_ == PlayState::PreLaunch) {
            state_ = PlayState::Paused;
            paused_ = true;
        } else if (state_ == PlayState::Paused) {
            // Resume to pre-launch if ball attached, otherwise active play
            state_ = engine_.isBallAttached() ? PlayState::PreLaunch : PlayState::Active;
            paused_ = false;
            frameTimer_.restart();
        }
        updateButtonsForState();
        update();
    });

    // Create pause menu buttons (hidden by default) - retro arcade style
    restartButton_ = new QPushButton(tr("RESTART"), this);
    saveButton_ = new QPushButton(tr("SAVE"), this);
    menuButton_ = new QPushButton(tr("MENU"), this);
    
    // Clean modern button styling with blue theme
    auto menuStyle = QStringLiteral(R"(QPushButton {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0099FF, stop:1 #0055CC);
        color: #FFFFFF;
        border: 2px solid #0099FF;
        border-radius: 4px;
        padding: 8px 20px;
        font-weight: bold;
        font-size: 14px;
        font-family: 'Courier New', monospace;
    }
    QPushButton:hover {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00BBFF, stop:1 #0077FF);
        border: 2px solid #00BBFF;
    }
    QPushButton:pressed {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0055CC, stop:1 #003388);
        border: 2px solid #0055CC;
    })");
    
    for (auto* btn : {restartButton_, saveButton_, menuButton_}) {
        btn->hide();
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setStyleSheet(menuStyle);
        btn->setMinimumWidth(160);
        btn->setMinimumHeight(48);
    }

    // Connect pause menu button actions
    connect(restartButton_, &QPushButton::clicked, this, [this]() {
        if (endgameMode_ && !loadedEndgameFilename_.isEmpty()) {
            // Request reload from disk via MainWindow
            emit reloadEndgameRequested();
        } else {
            // Normal restart from level 1
            restartGame();
        }
    });
    connect(saveButton_, &QPushButton::clicked, this, [this]() {
        if (state_ == PlayState::Paused) emit saveEndgameRequested();
    });
    connect(menuButton_, &QPushButton::clicked, this, [this]() {
        emit returnToMenuRequested();
    });

    layoutButtons();
    updateButtonsForState();
}

// ============================================================================
// Configuration Management
// ============================================================================
double GameWidget::mapBallSpeed(int sliderValue) const {
    int clamped = clamp(sliderValue, 1, 10);
    return 160.0 + 20.0 * static_cast<double>(clamped);
}

void GameWidget::applyEngineConfig() {
    engine_.setBallSpeed(mapBallSpeed(config_.ballSpeed));
    engine_.setStartingLevel(max(1, config_.startingLevel));
    engine_.setRandomSeed(config_.randomSeed);
}

void GameWidget::applyConfig(const breakout::GameConfig& config) {
    config_ = config;
    if (updateTimer_.isActive()) {
        restartGame();
    } else {
        applyEngineConfig();
    }
}

breakout::EndgameSnapshot GameWidget::captureEndgame(const QString& name) const {
    string cfgName = config_.name.isEmpty() ? string("default") : config_.name.toStdString();
    breakout::EndgameSnapshot snap = engine_.snapshot(name.toStdString(), cfgName);
    snap.configBallSpeed = config_.ballSpeed;
    snap.configRandomSeed = config_.randomSeed;
    snap.configStartingLevel = config_.startingLevel;
    return snap;
}

// ============================================================================
// Game State Management
// ============================================================================
void GameWidget::loadEndgame(const QString& filename, const breakout::EndgameSnapshot& state) {
    loadedEndgameFilename_ = filename;
    loadedEndgame_ = state;
    breakout::EndgameSnapshot adjusted = scaleSnapshotToViewport(state, width(), height());
    config_.name = QString::fromStdString(state.configName);
    config_.ballSpeed = state.configBallSpeed;
    config_.randomSeed = state.configRandomSeed;
    config_.startingLevel = state.configStartingLevel;
    applyEngineConfig();
    engine_.loadFromSnapshot(adjusted);
    useSnapshotBounds_ = true;
    endgameMode_ = true;
    activeOverlay_ = OverlayType::None;
    updatePlayfieldBounds();
    lastLives_ = engine_.lives();
    state_ = state.ballAttached ? PlayState::PreLaunch : PlayState::Active;
    finalLevel_ = false;
    paused_ = false;
    powerBannerVisible_ = false;
    powerBannerTimer_ = QElapsedTimer();
    lastExpandSeconds_ = engine_.expandTimeRemaining();
    lastSpeedBoostSeconds_ = engine_.speedBoostTimeRemaining();
    lastPointMultSeconds_ = engine_.pointMultiplierTimeRemaining();
    lastMultiBallSeconds_ = engine_.bigBallTimeRemaining();
    
    // Clear visual effects
    ballTrail_.clear();
    impactFlashes_.clear();
    particles_.clear();
    scorePopups_.clear();
    previousBrickCount_ = static_cast<int>(engine_.bricks().size());
    previousScore_ = engine_.score();
    effectsTimer_.restart();
    
    updateTimer_.start();
    frameTimer_.restart();
    updateButtonsForState();
    update();
}

void GameWidget::startGame() {
    applyEngineConfig();
    state_ = PlayState::PreLaunch;
    engine_.newGame();
    useSnapshotBounds_ = false;
    loadedEndgame_.reset();
    loadedEndgameFilename_.clear();
    endgameMode_ = false;
    finalLevel_ = false;
    leftPressed_ = false;
    rightPressed_ = false;
    updatePlayfieldBounds();
    lastLives_ = engine_.lives();
    powerBannerVisible_ = false;
    powerBannerTimer_ = QElapsedTimer();
    lastExpandSeconds_ = engine_.expandTimeRemaining();
    lastSpeedBoostSeconds_ = engine_.speedBoostTimeRemaining();
    lastPointMultSeconds_ = engine_.pointMultiplierTimeRemaining();
    lastMultiBallSeconds_ = engine_.bigBallTimeRemaining();
    
    // Clear visual effects
    ballTrail_.clear();
    impactFlashes_.clear();
    particles_.clear();
    scorePopups_.clear();
    previousBrickCount_ = static_cast<int>(engine_.bricks().size());
    previousScore_ = engine_.score();
    effectsTimer_.restart();
    
    updateTimer_.start();
    frameTimer_.restart();
    updateButtonsForState();
    update();
}

void GameWidget::restartGame() {
    applyEngineConfig();
    state_ = PlayState::PreLaunch;
    engine_.newGame();
    paused_ = false;
    finalLevel_ = false;
    useSnapshotBounds_ = false;
    loadedEndgame_.reset();
    loadedEndgameFilename_.clear();
    endgameMode_ = false;
    activeOverlay_ = OverlayType::None;
    leftPressed_ = false;
    rightPressed_ = false;
    updatePlayfieldBounds();
    lastLives_ = engine_.lives();
    powerBannerVisible_ = false;
    powerBannerTimer_ = QElapsedTimer();
    lastExpandSeconds_ = engine_.expandTimeRemaining();
    lastSpeedBoostSeconds_ = engine_.speedBoostTimeRemaining();
    lastPointMultSeconds_ = engine_.pointMultiplierTimeRemaining();
    lastMultiBallSeconds_ = engine_.bigBallTimeRemaining();
    
    // Clear visual effects
    ballTrail_.clear();
    impactFlashes_.clear();
    particles_.clear();
    scorePopups_.clear();
    previousBrickCount_ = static_cast<int>(engine_.bricks().size());
    previousScore_ = engine_.score();
    effectsTimer_.restart();
    
    updateTimer_.start();
    frameTimer_.restart();
    updateButtonsForState();
    update();
}

void GameWidget::stopGame() {
    updateTimer_.stop();
    state_ = PlayState::GameOver;
    powerBannerVisible_ = false;
    updateButtonsForState();
    update();
}

void GameWidget::tick() {
    double deltaSeconds = frameTimer_.restart() / 1000.0;
    if (deltaSeconds > kMaxDelta) deltaSeconds = kMaxDelta;

    // Block all game logic when overlay is active (user must dismiss first)
    if (activeOverlay_ != OverlayType::None) {
        updateButtonsForState();
        update();
        return;
    }

    if (state_ == PlayState::Paused || state_ == PlayState::GameOver || state_ == PlayState::Victory) {
        updateButtonsForState();
        update();
        return;
    }

    if (state_ == PlayState::LevelComplete) {
        // Auto-advance after a short pause
        if (levelCompleteTimer_.isValid() && levelCompleteTimer_.elapsed() > 4000 && !finalLevel_) {
            proceedFromLevelComplete();
        }
        updateButtonsForState();
        update();
        return;
    }

    updatePaddle(deltaSeconds);

    if (state_ == PlayState::PreLaunch) {
        engine_.attachBallToPaddle();
        update();
        return;
    }

    // Track ball position for trail
    QPointF ballCenter(engine_.ball().bounds().x + engine_.ball().bounds().width / 2,
                       engine_.ball().bounds().y + engine_.ball().bounds().height / 2);
    ballTrail_.push_back(ballCenter);
    if (ballTrail_.size() > static_cast<size_t>(maxTrailLength_)) {
        ballTrail_.pop_front();
    }

    // Track current state for effect detection
    int currentBrickCount = static_cast<int>(engine_.bricks().size());
    int currentScore = engine_.score();
    
    engine_.update(deltaSeconds);

    // Detect brick destruction and spawn effects
    if (currentBrickCount > static_cast<int>(engine_.bricks().size())) {
        // Bricks were destroyed - spawn impact at ball position
        spawnImpact(ballCenter);
    }
    
    // Detect score change for popups
    if (currentScore < engine_.score()) {
        int pointsGained = engine_.score() - currentScore;
        spawnScorePopup(ballCenter, pointsGained);
    }
    
    // Update visual effects
    updateEffects();

    // Detect new power-up applications by observing effect timers increase/reset.
    double expandNow = engine_.expandTimeRemaining();
    double speedBoostNow = engine_.speedBoostTimeRemaining();
    double pointMultNow = engine_.pointMultiplierTimeRemaining();
    double bigBallNow = engine_.bigBallTimeRemaining();
    
    if (expandNow > lastExpandSeconds_ + 0.01) {
        showPowerBanner(tr("Power-up: Expand Paddle (%1s)").arg(QString::number(expandNow, 'f', 1)), QColor(100, 255, 150));  // Green
    }
    if (speedBoostNow > lastSpeedBoostSeconds_ + 0.01) {
        showPowerBanner(tr("Power-up: Speed Boost (%1s)").arg(QString::number(speedBoostNow, 'f', 1)), QColor(255, 255, 100));  // Yellow
    }
    if (pointMultNow > lastPointMultSeconds_ + 0.01) {
        showPowerBanner(tr("Power-up: Points x%1 (%2s)").arg(engine_.pointMultiplier()).arg(QString::number(pointMultNow, 'f', 1)), QColor(255, 150, 255));  // Magenta
    }
    if (bigBallNow > lastMultiBallSeconds_ + 0.01) {
        showPowerBanner(tr("Power-up: Big Ball (%1s) [Larger Attack Radius!]").arg(QString::number(bigBallNow, 'f', 1)), QColor(255, 180, 100));  // Orange
    }
    
    lastExpandSeconds_ = expandNow;
    lastSpeedBoostSeconds_ = speedBoostNow;
    lastPointMultSeconds_ = pointMultNow;
    lastMultiBallSeconds_ = bigBallNow;

    // Detect life gained (ExtraLife powerup)
    if (engine_.lives() > lastLives_) {
        int livesGained = engine_.lives() - lastLives_;
        showPowerBanner(tr("Extra Life! +%1 ♥").arg(livesGained), QColor(255, 100, 200));  // Pink
    }

    if (engine_.levelComplete() && state_ != PlayState::LevelComplete && state_ != PlayState::Victory) {
        enterLevelCompleteState();
    }

    // Detect life loss to return to pre-launch state.
    if (!engine_.isGameOver() && engine_.lives() < lastLives_) {
        lifeLossFlash_ = true;
        activeOverlay_ = OverlayType::LifeLoss;
        state_ = PlayState::PreLaunch;
        ballTrail_.clear();  // Clear trail effect when ball dies
        scorePopups_.clear();  // Clear score popups
        impactFlashes_.clear();  // Clear impact effects
        particles_.clear();  // Clear particles
    }
    lastLives_ = engine_.lives();

    if (engine_.isGameOver()) {
        lifeLossFlash_ = false;
        state_ = PlayState::GameOver;
        activeOverlay_ = OverlayType::GameOver;
        emit gameOver();
    }

    updateButtonsForState();
    update();
}

void GameWidget::updatePaddle(double deltaSeconds) {
    if (leftPressed_ && !rightPressed_) {
        engine_.movePaddleLeft(deltaSeconds);
    } else if (rightPressed_ && !leftPressed_) {
        engine_.movePaddleRight(deltaSeconds);
    }
}

void GameWidget::paintEvent(QPaintEvent* event) {
    if (buffer_.size() != size()) {
        buffer_ = QPixmap(size());
    }
    buffer_.fill(Qt::black);

    QPainter bufferPainter(&buffer_);
    bufferPainter.setRenderHint(QPainter::Antialiasing, true);
    drawScene(bufferPainter);

    QPainter painter(this);
    painter.drawPixmap(0, 0, buffer_);
    QWidget::paintEvent(event);
}

void GameWidget::resizeEvent(QResizeEvent* event) {
    updatePlayfieldBounds();
    layoutButtons();
    layoutPauseMenuButtons();
    QWidget::resizeEvent(event);
}

void GameWidget::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        QWidget::keyPressEvent(event);
        return;
    }

    // Block all inputs during overlay except space/enter to dismiss and M for menu
    if (activeOverlay_ != OverlayType::None) {
        switch (event->key()) {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // Handle overlay dismissal (see below in main switch)
            break;
        case Qt::Key_M:
            // Allow menu request even during overlay
            emit returnToMenuRequested();
            return;
        default:
            // Ignore all other inputs when overlay is active
            QWidget::keyPressEvent(event);
            return;
        }
    }

    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:
        leftPressed_ = true;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        rightPressed_ = true;
        break;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // Handle overlay dismissal first (Victory overlay cannot be dismissed with spacebar)
        if (activeOverlay_ == OverlayType::Victory) {
            // Victory overlay can only be closed via buttons, not spacebar
            QWidget::keyPressEvent(event);
            return;
        } else if (activeOverlay_ == OverlayType::LifeLoss) {
            activeOverlay_ = OverlayType::None;
            lifeLossFlash_ = false;
            // Stay in PreLaunch state - ball remains attached, user must press space again to launch
        } else if (activeOverlay_ == OverlayType::LevelComplete) {
            activeOverlay_ = OverlayType::None;
            proceedFromLevelComplete();
        } else if (activeOverlay_ == OverlayType::GameOver) {
            activeOverlay_ = OverlayType::None;
            restartGame();
        } else if (state_ == PlayState::PreLaunch) {
            engine_.launchBall();
            state_ = PlayState::Active;
            lifeLossFlash_ = false;
        } else if (state_ == PlayState::GameOver) {
            restartGame();
        } else if (state_ == PlayState::LevelComplete) {
            proceedFromLevelComplete();
        }
        updateButtonsForState();
        break;
    case Qt::Key_Escape:
        [[fallthrough]];
    case Qt::Key_P:
        if (state_ == PlayState::Paused) {
            state_ = engine_.isBallAttached() ? PlayState::PreLaunch : PlayState::Active;
            paused_ = false;
        } else if (state_ == PlayState::Active || state_ == PlayState::PreLaunch) {
            state_ = PlayState::Paused;
            paused_ = true;
        }
        updateButtonsForState();
        break;
    case Qt::Key_S:
        if (state_ == PlayState::Paused) {
            emit saveEndgameRequested();
        }
        break;
    case Qt::Key_M:
        emit returnToMenuRequested();
        break;
    case Qt::Key_R:
        restartGame();
        break;
    default:
        break;
    }

    QWidget::keyPressEvent(event);
}

void GameWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        QWidget::keyReleaseEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:
        leftPressed_ = false;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        rightPressed_ = false;
        break;
    default:
        break;
    }

    QWidget::keyReleaseEvent(event);
}

void GameWidget::drawScene(QPainter& painter) {
    Rect bounds = engine_.playfieldBounds();

    // Clip all gameplay rendering to the playfield region
    painter.save();
    painter.setClipRect(playfieldRegion_);

    // Enhanced gradient background - vibrant arcade colors
    QLinearGradient bg(bounds.x, bounds.y, bounds.x, bounds.y + bounds.height);
    bg.setColorAt(0.0, QColor(20, 10, 40));      // Dark purple top
    bg.setColorAt(0.5, QColor(15, 15, 35));      // Dark blue middle
    bg.setColorAt(1.0, QColor(10, 20, 30));      // Dark cyan bottom
    painter.fillRect(QRectF(bounds.x, bounds.y, bounds.width, bounds.height), bg);

    // Draw subtle grid background pattern
    painter.setPen(QPen(QColor(100, 150, 200, 20), 1));
    const int gridSize = 40;
    for (double x = bounds.x; x <= bounds.x + bounds.width; x += gridSize) {
        painter.drawLine(QPointF(x, bounds.y), QPointF(x, bounds.y + bounds.height));
    }
    for (double y = bounds.y; y <= bounds.y + bounds.height; y += gridSize) {
        painter.drawLine(QPointF(bounds.x, y), QPointF(bounds.x + bounds.width, y));
    }

    painter.setPen(Qt::NoPen);

    // Draw particles first (behind gameplay)
    drawParticles(painter);

    // Enhanced brick rendering with color coding and gradients
    for (const auto& brickPtr : engine_.bricks()) {
        // Skip rendering destroyed bricks
        if (brickPtr->isDestroyed()) {
            continue;
        }
        
        const Brick& brick = *brickPtr;
        QRectF rect = toQRectF(brick.bounds());
        QColor color;
        QColor glowColor;
        
        switch (brick.type()) {
        case BrickType::Normal: {
            // Color code by position: cyan, pink, yellow pattern
            int posIndex = static_cast<int>(brick.bounds().x / 40) % 3;
            if (posIndex == 0) {
                color = QColor(100, 220, 255);      // Cyan
                glowColor = QColor(0, 255, 255);
            } else if (posIndex == 1) {
                color = QColor(255, 100, 180);      // Pink
                glowColor = QColor(255, 0, 255);
            } else {
                color = QColor(255, 220, 100);      // Yellow
                glowColor = QColor(255, 255, 0);
            }
            break;
        }
        case BrickType::Durable: {
            // Darken durable brick as it takes damage
            int hits = brick.hitsRemaining();
            int intensity = hits == 2 ? 255 : (hits == 1 ? 180 : 100);
            color = QColor(intensity, intensity * 0.7, intensity * 0.3);
            glowColor = QColor(255, 150, 0);
            break;
        }
        case BrickType::Indestructible: {
            color = QColor(150, 150, 160);
            glowColor = QColor(200, 200, 220);
            break;
        }
        }
        
        // Draw glowing border for enhanced effect
        painter.setPen(QPen(glowColor, 1.5));
        painter.setBrush(color);
        painter.drawRect(rect);
        
        // Add subtle gradient to brick
        QLinearGradient brickGrad(rect.topLeft(), rect.bottomLeft());
        brickGrad.setColorAt(0.0, color.lighter(130));
        brickGrad.setColorAt(0.5, color);
        brickGrad.setColorAt(1.0, color.darker(120));
        painter.setBrush(brickGrad);
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect);
        
        // Add damage cracks for durable bricks
        if (brick.type() == BrickType::Durable && brick.hitsRemaining() == 1) {
            painter.setPen(QPen(QColor(60, 40, 20), 2));
            painter.drawLine(rect.topLeft() + QPointF(rect.width() * 0.3, 0), 
                           rect.bottomLeft() + QPointF(rect.width() * 0.4, 0));
            painter.drawLine(rect.topRight() + QPointF(-rect.width() * 0.4, 0), 
                           rect.bottomRight() + QPointF(-rect.width() * 0.3, 0));
            painter.setPen(Qt::NoPen);
        }
    }

    // Enhanced paddle with shadow and glow
    QRectF paddleRect = toQRectF(engine_.paddle().bounds());
    
    // Paddle shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 80));
    painter.drawRect(paddleRect.adjusted(2, 4, 2, 6));
    
    // Paddle gradient
    QLinearGradient paddleGrad(paddleRect.topLeft(), paddleRect.bottomLeft());
    paddleGrad.setColorAt(0.0, QColor(150, 255, 150));
    paddleGrad.setColorAt(1.0, QColor(100, 200, 100));
    painter.setBrush(paddleGrad);
    painter.drawRect(paddleRect);
    
    // Paddle border glow (enhanced if expanded)
    if (engine_.expandTimeRemaining() > 0.0) {
        painter.setPen(QPen(QColor(0, 255, 200, 180), 3));
    } else {
        painter.setPen(QPen(QColor(100, 255, 150), 2));
    }
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(paddleRect);

    // Draw ball trail
    drawBallTrail(painter);

    // Enhanced ball with glow effect
    QRectF ballRect = toQRectF(engine_.ball().bounds());
    
    // Ball glow halo (multiple layers for better effect)
    for (int i = 3; i > 0; --i) {
        int alpha = 60 / i;
        painter.setBrush(QColor(240, 240, 255, alpha));
        painter.setPen(Qt::NoPen);
        double haloSize = i * 2;
        painter.drawEllipse(ballRect.adjusted(-haloSize, -haloSize, haloSize, haloSize));
    }
    
    // Ball itself
    painter.setBrush(QColor(255, 255, 240));
    painter.setPen(QPen(QColor(200, 200, 255), 1));
    painter.drawEllipse(ballRect);
    
    // Big ball visual enhancement
    if (engine_.isBigBallActive()) {
        // Add pulsing glow effect for big ball
        double pulse = 0.5 + 0.5 * std::sin(static_cast<double>(effectsTimer_.elapsed()) / 150.0);
        int alpha = static_cast<int>(100 * pulse);
        painter.setBrush(QColor(255, 200, 100, alpha));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(ballRect.adjusted(-3, -3, 3, 3));
    }

    // Powerups with enhanced styling and unique icons
    for (const auto& p : engine_.powerups()) {
        QColor c;
        QRectF r(p.position.x() - p.size * 0.5, p.position.y() - p.size * 0.5, p.size, p.size);
        
        painter.save();
        
        // Choose color based on type
        switch (p.type) {
        case breakout::GameEngine::PowerupType::ExpandPaddle:
            c = QColor(100, 255, 150); // Green
            break;
        case breakout::GameEngine::PowerupType::ExtraLife:
            c = QColor(255, 80, 120); // Pink/Red
            break;
        case breakout::GameEngine::PowerupType::SpeedBoost:
            c = QColor(255, 255, 100); // Yellow
            break;
        case breakout::GameEngine::PowerupType::PointMultiplier:
            c = QColor(255, 150, 255); // Magenta/Purple
            break;
        case breakout::GameEngine::PowerupType::MultiBall:
            c = QColor(100, 200, 255); // Cyan
            break;
        }
        
        // Powerup glow
        painter.setBrush(QColor(c.red(), c.green(), c.blue(), 100));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(r.adjusted(-4, -4, 4, 4));
        
        // Draw icon based on type
        painter.setPen(QPen(c.lighter(150), 2));
        painter.setBrush(c);
        
        switch (p.type) {
        case breakout::GameEngine::PowerupType::ExpandPaddle:
            // Circle for expand
            painter.drawEllipse(r);
            break;
            
        case breakout::GameEngine::PowerupType::ExtraLife: {
            // Heart shape
            QPainterPath heart;
            float cx = r.center().x();
            float cy = r.center().y();
            float size = r.width() * 0.35;
            heart.moveTo(cx, cy + size * 0.3);
            heart.cubicTo(cx, cy - size * 0.2, cx - size * 0.5, cy - size * 0.6, cx - size, cy - size * 0.2);
            heart.cubicTo(cx - size * 1.5, cy - size * 0.2, cx - size * 1.5, cy + size * 0.3, cx - size * 1.5, cy + size * 0.3);
            heart.cubicTo(cx - size * 1.5, cy + size * 1.0, cx, cy + size * 1.5, cx, cy + size * 1.5);
            heart.cubicTo(cx, cy + size * 1.5, cx + size * 1.5, cy + size * 1.0, cx + size * 1.5, cy + size * 0.3);
            heart.cubicTo(cx + size * 1.5, cy + size * 0.3, cx + size * 1.5, cy - size * 0.2, cx + size, cy - size * 0.2);
            heart.cubicTo(cx + size * 0.5, cy - size * 0.6, cx, cy - size * 0.2, cx, cy + size * 0.3);
            painter.drawPath(heart);
            break;
        }
            
        case breakout::GameEngine::PowerupType::SpeedBoost: {
            // Lightning bolt
            QPainterPath bolt;
            float cx = r.center().x();
            float cy = r.center().y();
            float w = r.width() * 0.35;
            bolt.moveTo(cx + w * 0.3, cy - w * 1.2);
            bolt.lineTo(cx - w * 0.2, cy);
            bolt.lineTo(cx + w * 0.5, cy);
            bolt.lineTo(cx - w * 0.3, cy + w * 1.2);
            bolt.lineTo(cx + w * 0.2, cy + w * 0.2);
            bolt.lineTo(cx - w * 0.5, cy + w * 0.2);
            bolt.closeSubpath();
            painter.drawPath(bolt);
            break;
        }
            
        case breakout::GameEngine::PowerupType::PointMultiplier: {
            // Diamond/gem shape
            QPainterPath gem;
            float cx = r.center().x();
            float cy = r.center().y();
            float w = r.width() * 0.4;
            gem.moveTo(cx, cy - w);
            gem.lineTo(cx + w * 0.7, cy - w * 0.3);
            gem.lineTo(cx + w, cy + w * 0.5);
            gem.lineTo(cx, cy + w);
            gem.lineTo(cx - w, cy + w * 0.5);
            gem.lineTo(cx - w * 0.7, cy - w * 0.3);
            gem.closeSubpath();
            painter.drawPath(gem);
            break;
        }
            
        case breakout::GameEngine::PowerupType::MultiBall: {
            // Star shape
            QPainterPath star;
            float cx = r.center().x();
            float cy = r.center().y();
            float outerR = r.width() * 0.45;
            float innerR = r.width() * 0.2;
            for (int i = 0; i < 10; ++i) {
                float angle = i * 3.14159 / 5.0 - 3.14159 / 2.0;
                float radius = (i % 2 == 0) ? outerR : innerR;
                float x = cx + radius * std::cos(angle);
                float y = cy + radius * std::sin(angle);
                if (i == 0) star.moveTo(x, y);
                else star.lineTo(x, y);
            }
            star.closeSubpath();
            painter.drawPath(star);
            break;
        }
        }
        
        painter.restore();
    }
    
    // Restore clipping after playfield rendering
    painter.restore();

    // Draw HUD, banner, and timer regions (outside playfield clip)
    drawHUDBar(painter);
    drawBannerRegion(painter);
    drawTimerRegion(painter);
    drawPowerBanner(painter);
    
    // Draw impact flashes over gameplay
    drawImpactFlashes(painter);
    
    // Draw score popups on top
    drawScorePopups(painter);

    // Draw overlays based on activeOverlay_ state (persistent until dismissed by user)
    if (activeOverlay_ == OverlayType::GameOver) {
        drawOverlay(painter, tr("Game Over"), tr(""));
    } else if (activeOverlay_ == OverlayType::Victory) {
        drawVictoryOverlay(painter);
    } else if (activeOverlay_ == OverlayType::LevelComplete) {
        QString body = tr("Score %1   Lives %2\nNext: Level %3")
                           .arg(engine_.score())
                           .arg(engine_.lives())
                           .arg(engine_.currentLevel() + 1);
        drawOverlay(painter, tr("Level Up"), body);
    } else if (activeOverlay_ == OverlayType::LifeLoss) {
        QString body = tr("You have %1 lives left")
                           .arg(engine_.lives());
        drawOverlay(painter, tr("Life Lost!"), body);
    } else if (state_ == PlayState::Paused) {
        // Draw simple pause menu background
        painter.save();
        
        // Dark overlay
        painter.setBrush(QColor(0, 0, 0, 180));
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect());
        
        // Simple pause menu card with blue border
        const int cardW = 420;
        const int cardH = 300;
        int cardX = (width() - cardW) / 2;
        int cardY = (height() - cardH) / 2;
        
        // Simple blue border
        painter.setPen(QPen(QColor(0, 150, 255), 4));  // Clean blue border
        painter.setBrush(QColor(10, 15, 30, 220));      // Dark blue background
        painter.drawRect(cardX, cardY, cardW, cardH);
        
        // "PAUSED" text
        painter.setPen(QColor(0, 200, 255));
        QFont titleFont("Courier New", 32, QFont::Bold);
        titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 3);
        painter.setFont(titleFont);
        painter.drawText(QRect(cardX, cardY + 20, cardW, 50), Qt::AlignCenter, tr("GAME PAUSED"));
        
        painter.restore();
    }
    
    // Draw decorative corner brackets
    painter.save();
    // Dim corners when overlay is active
    int alpha = (activeOverlay_ != OverlayType::None) ? 40 : 255;
    painter.setPen(QPen(QColor(0, 255, 255, alpha), 2));
    painter.setBrush(Qt::NoBrush);
    int px = playfieldRegion_.x();
    int py = playfieldRegion_.y();
    int pw = playfieldRegion_.width();
    int ph = playfieldRegion_.height();
    // Top-left corner bracket
    painter.drawLine(px + 5, py + 5, px + 20, py + 5);
    painter.drawLine(px + 5, py + 5, px + 5, py + 20);
    // Top-right corner bracket
    painter.drawLine(px + pw - 5, py + 5, px + pw - 20, py + 5);
    painter.drawLine(px + pw - 5, py + 5, px + pw - 5, py + 20);
    // Bottom-left corner bracket
    painter.drawLine(px + 5, py + ph - 5, px + 20, py + ph - 5);
    painter.drawLine(px + 5, py + ph - 5, px + 5, py + ph - 20);
    // Bottom-right corner bracket
    painter.drawLine(px + pw - 5, py + ph - 5, px + pw - 20, py + ph - 5);
    painter.drawLine(px + pw - 5, py + ph - 5, px + pw - 5, py + ph - 20);
    
    painter.restore();
    
    // Draw single thin cyan border frame
    painter.setPen(QPen(QColor(0, 255, 255), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(px - 2, py - 2, pw + 4, ph + 4));
}

void GameWidget::drawHUDBar(QPainter& painter) {
    painter.save();
    
    // Gradient background
    QLinearGradient hudGradient(0, 0, 0, hudRegion_.height());
    hudGradient.setColorAt(0.0, QColor(15, 20, 35, 200));
    hudGradient.setColorAt(1.0, QColor(8, 12, 22, 220));
    painter.setBrush(hudGradient);
    painter.setPen(Qt::NoPen);
    painter.drawRect(hudRegion_);
    
    // Bottom accent line
    QLinearGradient accentGradient(0, hudRegion_.bottom() - 2, width(), hudRegion_.bottom() - 2);
    accentGradient.setColorAt(0.0, QColor(30, 190, 210, 0));
    accentGradient.setColorAt(0.3, QColor(30, 190, 210, 180));
    accentGradient.setColorAt(0.7, QColor(30, 190, 210, 180));
    accentGradient.setColorAt(1.0, QColor(30, 190, 210, 0));
    painter.fillRect(0, hudRegion_.bottom() - 2, width(), 2, accentGradient);
    
    // Corner decorations
    painter.setPen(QPen(QColor(30, 190, 210, 150), 2));
    // Top-left corner brackets
    painter.drawLine(8, 12, 8, 22);
    painter.drawLine(8, 12, 18, 12);
    // Top-right corner brackets
    painter.drawLine(width() - 8, 12, width() - 8, 22);
    painter.drawLine(width() - 8, 12, width() - 18, 12);
    
    painter.restore();

    // Layout constants
    const int hudPadding = 26;
    const int hudHeight = hudRegion_.height();

    // Base font
    QFont baseFont("JetBrains Mono", 12);
    baseFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    painter.setFont(baseFont);
    
    // Score display (left side)
    QString scoreText = tr("SCORE: %1").arg(engine_.score());
    painter.setPen(QColor(255, 230, 80));
    painter.drawText(QRectF(hudPadding, 0, 200, hudHeight), Qt::AlignLeft | Qt::AlignVCenter, scoreText);

    // Lives display (center-left)
    int livesX = hudPadding + 220;
    painter.drawText(QRectF(livesX, 0, 60, hudHeight), Qt::AlignLeft | Qt::AlignVCenter, tr("LIVES:"));
    
    // Lives heart icons
    int heartStartX = livesX + 70;
    int heartY = (hudHeight - 16) / 2;
    const int maxLives = 5;
    for (int i = 0; i < maxLives; ++i) {
        painter.save();
        QPainterPath heartPath;
        float x = heartStartX + i * 20 + 8;
        float y = heartY + 4;
        float size = 6;
        
        // Create heart shape
        heartPath.moveTo(x, y + size * 0.3f);
        heartPath.cubicTo(x, y, x - size * 0.5f, y - size * 0.4f, x - size, y);
        heartPath.cubicTo(x - size * 1.5f, y, x - size * 1.5f, y + size * 0.5f, x - size * 1.5f, y + size * 0.5f);
        heartPath.cubicTo(x - size * 1.5f, y + size * 1.2f, x, y + size * 1.8f, x, y + size * 1.8f);
        heartPath.cubicTo(x, y + size * 1.8f, x + size * 1.5f, y + size * 1.2f, x + size * 1.5f, y + size * 0.5f);
        heartPath.cubicTo(x + size * 1.5f, y + size * 0.5f, x + size * 1.5f, y, x + size, y);
        heartPath.cubicTo(x + size * 0.5f, y - size * 0.4f, x, y, x, y + size * 0.3f);
        
        if (i < engine_.lives()) {
            painter.setBrush(QColor(255, 80, 120));
            painter.setPen(Qt::NoPen);
        } else {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(70, 75, 85), 2));
        }
        painter.drawPath(heartPath);
        painter.restore();
    }

    // Level display (center)
    int levelX = heartStartX + 130;
    QString levelText = tr("LEVEL: %1").arg(engine_.currentLevel());
    painter.drawText(QRectF(levelX, 0, 150, hudHeight), Qt::AlignLeft | Qt::AlignVCenter, levelText);
}

void GameWidget::drawBannerRegion(QPainter&) {
    // Banner animation is rendered via drawPowerBanner() which uses bannerRegion_ bounds
    // Banner occupies top 40px of the combined powerup region (y: 70-110)
    // Timer text occupies bottom 30px of the same region (y: 110-140)
}

void GameWidget::drawTimerRegion(QPainter& painter) {
    painter.save();
    
    // Base font for timers
    QFont timerFont("JetBrains Mono", 11);
    timerFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    painter.setFont(timerFont);
    
    // Collect active powerup timers
    QStringList timers;
    if (engine_.expandTimeRemaining() > 0.0) {
        timers << tr("EXPAND %1s").arg(QString::number(engine_.expandTimeRemaining(), 'f', 1));
    }
    if (engine_.speedBoostTimeRemaining() > 0.0) {
        timers << tr("SPEED %1s").arg(QString::number(engine_.speedBoostTimeRemaining(), 'f', 1));
    }
    if (engine_.pointMultiplierTimeRemaining() > 0.0) {
        timers << tr("POINTS x%1 (%2s)").arg(engine_.pointMultiplier())
                    .arg(QString::number(engine_.pointMultiplierTimeRemaining(), 'f', 1));
    }
    if (engine_.bigBallTimeRemaining() > 0.0) {
        timers << tr("BIG BALL %1s").arg(QString::number(engine_.bigBallTimeRemaining(), 'f', 1));
    }
    
    // Render timers horizontally arranged in the bottom part of the combined region
    if (!timers.isEmpty()) {
        int totalWidth = 0;
        for (const QString& timer : timers) {
            QFontMetrics fm(timerFont);
            totalWidth += fm.horizontalAdvance(timer) + 20;  // 20px spacing
        }
        
        int x = (width() - totalWidth) / 2;  // Center horizontally
        int y = timerRegion_.y() + timerRegion_.height() / 2;  // Vertically center in timer region
        
        for (const QString& timer : timers) {
            QColor color;
            if (timer.startsWith("EXPAND")) color = QColor(100, 255, 150);
            else if (timer.startsWith("SPEED")) color = QColor(255, 255, 100);
            else if (timer.startsWith("POINTS")) color = QColor(255, 150, 255);
            else if (timer.startsWith("BIG")) color = QColor(255, 180, 100);
            else color = QColor(80, 180, 255);
            
            painter.setPen(color);
            QFontMetrics fm(timerFont);
            int textWidth = fm.horizontalAdvance(timer);
            painter.drawText(QRectF(x, y - 8, textWidth, 16), Qt::AlignCenter, timer);
            x += textWidth + 20;
        }
    }
    
    painter.restore();
}

void GameWidget::drawOverlay(QPainter& painter, const QString& title, const QString& body) {
    painter.save();
    
    // Retro arcade dark overlay with scanlines effect
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
    
    // Scanlines effect (horizontal lines for CRT feel)
    painter.setPen(QPen(QColor(0, 0, 0, 60), 1));
    for (int y = 0; y < height(); y += 3) {
        painter.drawLine(0, y, width(), y);
    }
    
    // Retro arcade text styling - bright yellow/cyan
    painter.setPen(QColor(255, 255, 0));  // Bright yellow
    QFont font("Courier New", 28);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    painter.setFont(font);
    
    // Draw title
    QFontMetrics fm = QFontMetrics(font);
    int titleWidth = fm.horizontalAdvance(title);
    painter.drawText((width() - titleWidth) / 2 + 10, height() / 2 - 40, title);
    
    // Draw body text with cyan color for contrast - tighter spacing
    painter.setPen(QColor(0, 255, 255));
    QFont bodyFont("Courier New", 14);
    bodyFont.setBold(true);
    painter.setFont(bodyFont);
    
    // Use QTextOption for tighter line spacing
    QTextOption textOption(Qt::AlignCenter);
    textOption.setWrapMode(QTextOption::WordWrap);
    
    QFontMetrics bodyFm(bodyFont);
    int bodyHeight = bodyFm.height() * body.count('\n') + bodyFm.height();
    QRectF bodyRect(40, height() / 2 - 10, width() - 80, bodyHeight + 20);
    painter.drawText(bodyRect, body, textOption);
    
    // Draw spacebar instruction at bottom (only for non-game over overlays)
    if (title != tr("Game Over")) {
        painter.setPen(QColor(0, 200, 100));  // Green color for instruction
        QFont instructionFont("Courier New", 12);
        instructionFont.setBold(true);
        painter.setFont(instructionFont);
        painter.drawText(rect().adjusted(40, 0, -40, -200), Qt::AlignBottom | Qt::AlignHCenter, 
                         tr("Press spacebar to return to game"));
    }
    
    painter.restore();
}

void GameWidget::drawVictoryOverlay(QPainter& painter) {
    painter.save();
    
    // Dark overlay with scanlines
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
    
    // Scanlines effect
    painter.setPen(QPen(QColor(0, 0, 0, 60), 1));
    for (int y = 0; y < height(); y += 3) {
        painter.drawLine(0, y, width(), y);
    }
    
    // Victory title - bright yellow
    painter.setPen(QColor(255, 255, 0));
    QFont titleFont("Courier New", 36);
    titleFont.setBold(true);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    painter.setFont(titleFont);
    
    QFontMetrics titleFm(titleFont);
    QString title = tr("VICTORY!");
    int titleWidth = titleFm.horizontalAdvance(title);
    painter.drawText((width() - titleWidth) / 2 + 10, height() / 2 - 80, title);
    
    // Score display - cyan
    painter.setPen(QColor(0, 255, 255));
    QFont scoreFont("Courier New", 16);
    scoreFont.setBold(true);
    painter.setFont(scoreFont);
    
    QString scoreText = tr("Final Score: %1").arg(engine_.score());
    QFontMetrics scoreFm(scoreFont);
    int scoreWidth = scoreFm.horizontalAdvance(scoreText);
    painter.drawText((width() - scoreWidth) / 2, height() / 2 - 20, scoreText);
    
    // Note: Buttons are rendered separately, not part of this overlay
    painter.restore();
}

void GameWidget::drawPowerBanner(QPainter& painter) {
    if (!powerBannerVisible_) return;
    if (!powerBannerTimer_.isValid()) {
        powerBannerVisible_ = false;
        return;
    }
    int elapsed = powerBannerTimer_.elapsed();
    if (elapsed >= powerBannerDurationMs_) {
        powerBannerVisible_ = false;
        return;
    }
    double progress = std::clamp(static_cast<double>(elapsed) / static_cast<double>(powerBannerDurationMs_), 0.0, 1.0);
    
    // Use banner region for positioning (top of combined powerup region)
    int bannerY = bannerRegion_.y();
    int bannerHeight = bannerRegion_.height();
    double barWidth = width() * progress;
    QRectF barRect(0.0, bannerY, barWidth, bannerHeight);

    painter.save();
    painter.setPen(Qt::NoPen);
    // Use stored color for banner (allows per-powerup colors)
    QColor bannerColor = powerBannerColor_;
    bannerColor.setAlpha(190);
    painter.setBrush(bannerColor);
    painter.drawRect(barRect);

    painter.setFont(QFont("Helvetica", 14, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(QRectF(12.0, bannerY + 5.0, barRect.width() - 24.0, bannerHeight - 10.0), Qt::AlignLeft | Qt::AlignVCenter, powerBannerText_);
    painter.restore();
}

void GameWidget::showPowerBanner(const QString& text, const QColor& color) {
    powerBannerText_ = text;
    powerBannerColor_ = color;
    powerBannerVisible_ = true;
    powerBannerTimer_.restart();
}

void GameWidget::enterLevelCompleteState() {
    if (endgameMode_) {
        finalLevel_ = true;
        state_ = PlayState::Victory;
        activeOverlay_ = OverlayType::Victory;
        updateButtonsForState();
        return;
    }

    finalLevel_ = !engine_.hasNextLevel();
    if (finalLevel_) {
        state_ = PlayState::Victory;
        activeOverlay_ = OverlayType::Victory;
    } else {
        state_ = PlayState::LevelComplete;
        activeOverlay_ = OverlayType::LevelComplete;
        levelCompleteTimer_.restart();
    }
    updateButtonsForState();
}

void GameWidget::proceedFromLevelComplete() {
    if (finalLevel_) {
        state_ = PlayState::Victory;
        updateButtonsForState();
        update();
        return;
    }
    if (engine_.advanceToNextLevel()) {
        state_ = PlayState::PreLaunch;
        lastLives_ = engine_.lives();
        frameTimer_.restart();
        finalLevel_ = false;
        
        // Clear visual effects when advancing to next level
        ballTrail_.clear();
        impactFlashes_.clear();
        particles_.clear();
        scorePopups_.clear();
        previousBrickCount_ = static_cast<int>(engine_.bricks().size());
        previousScore_ = engine_.score();
        effectsTimer_.restart();
    }
    updateButtonsForState();
    update();
}

void GameWidget::updatePlayfieldBounds() {
    // Calculate region boundaries
    hudRegion_ = QRect(0, 0, width(), kHudHeight);
    
    // Combined powerup region (banner animation + timer text, no gap between them)
    int powerupRegionY = kHudHeight + kHudPaddingBottom;
    bannerRegion_ = QRect(0, powerupRegionY, width(), kBannerHeight);
    timerRegion_ = QRect(0, powerupRegionY + kBannerHeight, width(), kTimerHeight);
    
    // Playfield starts after all fixed regions
    int playfieldY = kPlayfieldTopOffset;
    int playfieldMaxHeight = height() - playfieldY - kPlayfieldMarginBottom;
    int playfieldMaxWidth = width() - 32; // 16px margins each side
    
    // Use snapshot bounds verbatim when loading a saved endgame
    if (useSnapshotBounds_) {
        breakout::Rect bounds = engine_.playfieldBounds();
        playfieldRegion_ = QRect(static_cast<int>(bounds.x),
                                 static_cast<int>(bounds.y),
                                 static_cast<int>(bounds.width),
                                 static_cast<int>(bounds.height));
        engine_.setPlayfield(bounds);
        return;
    }

    // Use full available space for standard games
    double actualWidth = playfieldMaxWidth;
    double actualHeight = playfieldMaxHeight;
    
    // Center horizontally if narrower than available space
    int playfieldX = 16 + (playfieldMaxWidth - static_cast<int>(actualWidth)) / 2;
    
    playfieldRegion_ = QRect(playfieldX, playfieldY, 
                              static_cast<int>(actualWidth), 
                              static_cast<int>(actualHeight));
    
    // Update engine with actual playfield bounds
    breakout::Rect bounds{
        static_cast<double>(playfieldX),
        static_cast<double>(playfieldY),
        actualWidth,
        actualHeight
    };
    engine_.setPlayfield(bounds);
}

void GameWidget::layoutButtons() {
    const int hudHeight = 52;
    const int btnW = 80;
    const int btnH = 20;
    const int margin = 35;
    int x = width() - margin - btnW;  // Top-right corner
    int y = (hudHeight - btnH) / 2 - 3;
    pauseButton_->setGeometry(x, y, btnW, btnH);
}

void GameWidget::layoutPauseMenuButtons() {
    if (!restartButton_) return;
    
    // Arcade-style pause menu with retro frame layout
    const int btnW = 130;
    const int btnH = 50;
    const int spacing = 20;
    
    // Center the menu frame, with extra space for "PAUSED" title
    int totalH = 3 * btnH + 3 * spacing;
    int startY = (height() - totalH) / 2 + 25;  // Lower to avoid title overlap
    int centerX = width() / 2 - btnW / 2;
    
    // Position buttons vertically centered with larger spacing
    saveButton_->setGeometry(centerX, startY, btnW, btnH);
    restartButton_->setGeometry(centerX, startY + (btnH + spacing), btnW, btnH);
    menuButton_->setGeometry(centerX, startY + 2 * (btnH + spacing), btnW, btnH);
}

void GameWidget::updateButtonsForState() {
    pauseButton_->setText(state_ == PlayState::Paused ? tr("RESUME") : tr("PAUSE"));
    bool allowPause = (state_ == PlayState::Active || state_ == PlayState::PreLaunch || state_ == PlayState::Paused) 
                      && activeOverlay_ == OverlayType::None;
    pauseButton_->setEnabled(allowPause);

    // Show menu buttons for Paused, GameOver, or Victory states
    bool showMenuButtons = (state_ == PlayState::Paused || state_ == PlayState::GameOver || state_ == PlayState::Victory);
    for (auto* btn : {restartButton_, saveButton_, menuButton_}) {
        btn->setVisible(showMenuButtons);
        btn->setEnabled(showMenuButtons);
    }
    // Save only makes sense while paused (not victory or game over)
    saveButton_->setVisible(state_ == PlayState::Paused);
    saveButton_->setEnabled(state_ == PlayState::Paused);
}

void GameWidget::updateEffects() {
    qint64 currentTime = effectsTimer_.elapsed();
    
    // Remove expired impact flashes (300ms lifetime)
    impactFlashes_.erase(
        std::remove_if(impactFlashes_.begin(), impactFlashes_.end(),
            [currentTime](const ImpactFlash& flash) {
                return (currentTime - flash.startTime) > 300;
            }),
        impactFlashes_.end()
    );
    
    // Update and remove expired particles (600ms lifetime)
    const double deltaSeconds = 0.016; // ~60fps
    for (auto& particle : particles_) {
        particle.velocity.setY(particle.velocity.y() + 300.0 * deltaSeconds); // Gravity
        particle.position += particle.velocity * deltaSeconds;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [currentTime, this](const Particle& p) {
                qint64 age = currentTime - p.startTime;
                return age > 600 || 
                       p.position.x() < 0 || p.position.x() > width() ||
                       p.position.y() < 0 || p.position.y() > height();
            }),
        particles_.end()
    );
    
    // Update score popups (float upward)
    for (auto& popup : scorePopups_) {
        popup.position += popup.velocity * deltaSeconds;
    }
    scorePopups_.erase(
        std::remove_if(scorePopups_.begin(), scorePopups_.end(),
            [currentTime](const ScorePopup& popup) {
                return (currentTime - popup.startTime) > 1500;
            }),
        scorePopups_.end()
    );
    
    // Limit particle count for performance
    if (particles_.size() > 100) {
        particles_.erase(particles_.begin(), particles_.begin() + (particles_.size() - 100));
    }
}

void GameWidget::spawnParticles(const QPointF& center, const QColor& color) {
    qint64 currentTime = effectsTimer_.elapsed();
    
    // Spawn 6-8 particles
    int count = 6 + (rand() % 3);
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = center;
        
        // Random angle and speed
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 80.0f + (rand() % 70);
        p.velocity = QPointF(std::cos(angle) * speed, std::sin(angle) * speed);
        
        p.color = color;
        p.size = 3.0f + (rand() % 3);
        p.startTime = currentTime;
        
        particles_.push_back(p);
    }
}

void GameWidget::spawnScorePopup(const QPointF& position, int scoreValue) {
    qint64 currentTime = effectsTimer_.elapsed();
    
    ScorePopup popup;
    popup.position = position;
    popup.velocity = QPointF(0, -30.0); // Float upward
    popup.scoreValue = scoreValue;
    popup.startTime = currentTime;
    
    scorePopups_.push_back(popup);
    
    // Also spawn particles at brick location
    QColor particleColor;
    if (scoreValue >= 200) {
        particleColor = QColor(255, 200, 80); // Durable brick - orange
    } else {
        particleColor = QColor(100, 200, 255); // Normal brick - blue
    }
    spawnParticles(position, particleColor);
}

void GameWidget::spawnImpact(const QPointF& position) {
    qint64 currentTime = effectsTimer_.elapsed();
    
    ImpactFlash flash;
    flash.position = position;
    flash.startTime = currentTime;
    flash.maxRadius = 25.0f;
    
    impactFlashes_.push_back(flash);
    
    // Limit to 10 active flashes
    if (impactFlashes_.size() > 10) {
        impactFlashes_.erase(impactFlashes_.begin());
    }
}

void GameWidget::drawParticles(QPainter& painter) {
    qint64 currentTime = effectsTimer_.elapsed();
    
    painter.save();
    painter.setPen(Qt::NoPen);
    
    for (const auto& particle : particles_) {
        qint64 age = currentTime - particle.startTime;
        float progress = age / 600.0f; // 600ms lifetime
        int alpha = static_cast<int>(255 * (1.0f - progress));
        alpha = std::max(0, std::min(255, alpha)); // Clamp to valid range
        
        QColor color = particle.color;
        color.setAlpha(alpha);
        painter.setBrush(color);
        
        painter.drawEllipse(particle.position, particle.size, particle.size);
    }
    
    painter.restore();
}

void GameWidget::drawBallTrail(QPainter& painter) {
    if (ballTrail_.size() < 2) return;
    
    painter.save();
    painter.setPen(Qt::NoPen);
    
    size_t count = ballTrail_.size();
    for (size_t i = 0; i < count; ++i) {
        float progress = static_cast<float>(i) / static_cast<float>(count);
        int alpha = static_cast<int>(10 + 50 * progress); // 10% to 60% opacity
        float radius = 3.0f + 5.0f * progress; // Smaller at old positions
        
        QColor trailColor(180, 200, 255, alpha);
        painter.setBrush(trailColor);
        painter.drawEllipse(ballTrail_[i], radius, radius);
    }
    
    painter.restore();
}

void GameWidget::drawImpactFlashes(QPainter& painter) {
    qint64 currentTime = effectsTimer_.elapsed();
    
    painter.save();
    painter.setPen(Qt::NoPen);
    
    for (const auto& flash : impactFlashes_) {
        qint64 age = currentTime - flash.startTime;
        float progress = age / 300.0f; // 300ms lifetime
        
        // Expand radius
        float radius = flash.maxRadius * progress;
        
        // Fade out
        int alpha = static_cast<int>(180 * (1.0f - progress));
        
        QColor flashColor(255, 255, 255, alpha);
        painter.setBrush(flashColor);
        painter.drawEllipse(flash.position, radius, radius);
    }
    
    painter.restore();
}

void GameWidget::drawScorePopups(QPainter& painter) {
    qint64 currentTime = effectsTimer_.elapsed();
    
    painter.save();
    
    QFont popupFont("JetBrains Mono", 14, QFont::Bold);
    painter.setFont(popupFont);
    
    for (const auto& popup : scorePopups_) {
        qint64 age = currentTime - popup.startTime;
        float progress = age / 1500.0f; // 1500ms lifetime
        
        // Fade out
        int alpha = static_cast<int>(255 * (1.0f - progress));
        
        // Color based on score value
        QColor textColor;
        if (popup.scoreValue >= 200) {
            textColor = QColor(255, 180, 80, alpha); // Orange for high scores
        } else {
            textColor = QColor(255, 255, 100, alpha); // Yellow for normal
        }
        
        painter.setPen(textColor);
        QString text = QString("+%1").arg(popup.scoreValue);
        
        QRectF textRect(popup.position.x() - 30, popup.position.y() - 10, 60, 20);
        painter.drawText(textRect, Qt::AlignCenter, text);
    }
    
    painter.restore();
}
