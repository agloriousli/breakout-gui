#pragma once

#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>
#include <QWidget>
#include <QPushButton>
#include <deque>
#include <vector>
#include <map>
#include <QPointF>
#include <QColor>
#include <optional>

#include "core/game/game_engine.h"
#include "../data/config_manager.h"
#include "core/game/endgame_state.h"

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QWidget* parent = nullptr);

    void startGame();
    void restartGame();
    void stopGame();
    void applyConfig(const breakout::GameConfig& config);
    breakout::EndgameSnapshot captureEndgame(const QString& name) const;
    void loadEndgame(const QString& filename, const breakout::EndgameSnapshot& state);
    QString loadedEndgameFilename() const { return loadedEndgameFilename_; }

signals:
    void gameOver();
    void returnToMenuRequested();
    void saveEndgameRequested();
    void reloadEndgameRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void tick();

private:
    enum class PlayState { PreLaunch, Active, Paused, LevelComplete, Victory, GameOver };
    enum class OverlayType { None, LifeLoss, LevelComplete, GameOver, Victory };

    void updatePaddle(double deltaSeconds);
    void drawScene(QPainter& painter);
    void drawHUDBar(QPainter& painter);
    void drawBannerRegion(QPainter& painter);
    void drawTimerRegion(QPainter& painter);
    void drawOverlay(QPainter& painter, const QString& title, const QString& body);
    void drawVictoryOverlay(QPainter& painter);
    void drawPowerBanner(QPainter& painter);
    void updatePlayfieldBounds();
    void layoutButtons();
    void updateButtonsForState();
        void layoutPauseMenuButtons();
    void applyEngineConfig();
    double mapBallSpeed(int sliderValue) const;
    void enterLevelCompleteState();
    void proceedFromLevelComplete();
    void showPowerBanner(const QString& text, const QColor& color = QColor(255, 80, 120));
    
    // Visual effects helpers
    void updateEffects();
    void spawnParticles(const QPointF& center, const QColor& color);
    void spawnScorePopup(const QPointF& position, int scoreValue);
    void spawnImpact(const QPointF& position);
    void drawParticles(QPainter& painter);
    void drawBallTrail(QPainter& painter);
    void drawImpactFlashes(QPainter& painter);
    void drawScorePopups(QPainter& painter);

    // Visual effect data structures
    struct ImpactFlash {
        QPointF position;
        qint64 startTime;
        float maxRadius = 25.0f;
    };
    
    struct Particle {
        QPointF position;
        QPointF velocity;
        QColor color;
        float size;
        qint64 startTime;
    };
    
    struct ScorePopup {
        QPointF position;
        QPointF velocity;
        int scoreValue;
        qint64 startTime;
    };

    breakout::GameEngine engine_;
    PlayState state_ { PlayState::PreLaunch };
    OverlayType activeOverlay_ { OverlayType::None };
    bool leftPressed_ {false};
    bool rightPressed_ {false};
    bool paused_ {false};
    int lastLives_ {3};
    QTimer updateTimer_;
    QElapsedTimer frameTimer_;
    QElapsedTimer levelCompleteTimer_;
    bool lifeLossFlash_ {false};
    QPixmap buffer_;
    breakout::GameConfig config_;
    bool finalLevel_ {false};
    QPushButton* pauseButton_ {nullptr};
    QPushButton* restartButton_ {nullptr};
    QPushButton* saveButton_ {nullptr};
    QPushButton* menuButton_ {nullptr};

    // Power-up banner state
    bool powerBannerVisible_ {false};
    QString powerBannerText_;
    QColor powerBannerColor_ {255, 80, 120};  // Default pink
    QElapsedTimer powerBannerTimer_;
    int powerBannerDurationMs_ {1800};
    double lastExpandSeconds_ {0.0};
    double lastSpeedBoostSeconds_ {0.0};
    double lastPointMultSeconds_ {0.0};
    double lastMultiBallSeconds_ {0.0};
    
    // Region boundaries (calculated in updatePlayfieldBounds)
    QRect hudRegion_;
    QRect bannerRegion_;
    QRect timerRegion_;
    QRect playfieldRegion_;
    
    // Visual effects state
    std::deque<QPointF> ballTrail_;
    int maxTrailLength_ {8};
    std::vector<ImpactFlash> impactFlashes_;
    std::vector<Particle> particles_;
    std::vector<ScorePopup> scorePopups_;
    QElapsedTimer effectsTimer_;
    int previousBrickCount_ {0};
    int previousScore_ {0};

    // Preserve layout when loading custom endgame snapshots
    bool useSnapshotBounds_ {false};
    std::optional<breakout::EndgameSnapshot> loadedEndgame_;
    bool endgameMode_ {false};
    QString loadedEndgameFilename_;
};
