#pragma once

#include <QMainWindow>

#include "../data/config_manager.h"
#include "../data/endgame_manager.h"

class QStackedWidget;
class QWidget;
class QDialog;

class MenuWidget;
class GameWidget;
namespace breakout { class EndgameEditorWidget; }

enum class EndgameDialogMode { Play, Manage };

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupMenu();
    void setupConnections();
    void applyConfig(const breakout::GameConfig& cfg);
    void saveEndgame();
    void loadEndgame();
    void reloadCurrentEndgame();
    QString openEndgameEditor(const breakout::EndgameSnapshot* initial = nullptr,
                              const QString& suggestedName = QString());
    void openEndgameDialog(EndgameDialogMode mode);
    bool playEndgame(const QString& name, QWidget* contextForErrors);
    void editConfig();
    void editEndgame();
    void deleteConfig();
    void deleteEndgame();
    void openConfigDialog();
    void showHowToPlay();

    QStackedWidget* stack_ {nullptr};
    MenuWidget* menu_ {nullptr};
    GameWidget* game_ {nullptr};
    breakout::ConfigManager* configManager_ {nullptr};
    breakout::EndgameManager* endgameManager_ {nullptr};
    breakout::GameConfig currentConfig_;
};
