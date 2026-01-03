#include "main_window.h"

#include <limits>

#include <QFont>
#include <QDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QListWidget>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QStyle>
#include <QPainter>

#include "game_widget.h"
#include "endgame_editor.h"
#include "../data/config_manager.h"
#include "../data/endgame_manager.h"

using namespace std;
class MenuWidget : public QWidget {
    Q_OBJECT
public:
    explicit MenuWidget(QWidget* parent = nullptr) : QWidget(parent) {
        constexpr int kButtonWidth = 220;

        auto* root = new QVBoxLayout(this);
        root->setAlignment(Qt::AlignCenter);
        root->setContentsMargins(32, 32, 32, 32);

        auto* card = new QFrame(this);
        card->setObjectName("menuCard");
        card->setStyleSheet(QStringLiteral(
            "#menuCard {"
            " background: rgba(20,24,40,180);"
            " border: 1px solid rgba(80,120,140,180);"
            " border-radius: 14px;"
            "}"
            "#menuCard QPushButton {"
            " background: rgba(40,50,70,200);"
            " color: #f3f8fb;"
            " border: 1px solid rgba(30,190,210,180);"
            " border-radius: 10px;"
            " padding: 10px 14px;"
            " font-weight: 600;"
            "}"
            "#menuCard QPushButton:hover {"
            " background: rgba(60,90,120,220);"
            "}"
            "#menuCard QPushButton:pressed {"
            " background: rgba(30,50,70,220);"
            " border-color: rgba(30,190,210,230);"
            "}"
        ));

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        cardLayout->setContentsMargins(28, 24, 28, 24);
        cardLayout->setSpacing(12);

        auto* title = new QLabel(tr("MAIN MENU"), card);
        QFont titleFont = title->font();
        titleFont.setPointSize(24);
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("color: #f5fbff; text-shadow: 0px 1px 3px rgba(0,0,0,160);");

        auto* underline = new QFrame(card);
        underline->setFixedHeight(3);
        underline->setStyleSheet(QStringLiteral("background-color: rgba(30,190,210,220); border-radius: 2px;"));

        auto* playDefaultButton = new QPushButton(tr("Play Default"), card);
        auto* playEndgameButton = new QPushButton(tr("Play Endgame"), card);
        auto* editConfigsButton = new QPushButton(tr("Configs Menu"), card);
        auto* editEndgamesButton = new QPushButton(tr("Edit Endgames"), card);
        auto* helpButton = new QPushButton(tr("How to Play"), card);
        auto* quitButton = new QPushButton(tr("Quit"), card);

        playDefaultButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        playEndgameButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
        editConfigsButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        editEndgamesButton->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
        helpButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
        quitButton->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));

        for (auto* btn : {playDefaultButton, playEndgameButton, editConfigsButton, editEndgamesButton, helpButton, quitButton}) {
            btn->setMinimumWidth(kButtonWidth);
            btn->setCursor(Qt::PointingHandCursor);
        }

        auto* footer = new QLabel(tr("Breakout Game"), card);
        QFont footFont = footer->font();
        footFont.setPointSize(10);
        footer->setFont(footFont);
        footer->setAlignment(Qt::AlignCenter);
        footer->setStyleSheet("color: rgba(230,240,250,160);");

        cardLayout->addWidget(title);
        cardLayout->addWidget(underline);
        cardLayout->addSpacing(6);
        cardLayout->addWidget(playDefaultButton);
        cardLayout->addWidget(playEndgameButton);
        cardLayout->addWidget(editConfigsButton);
        cardLayout->addWidget(editEndgamesButton);
        cardLayout->addWidget(helpButton);
        cardLayout->addWidget(quitButton);
        cardLayout->addSpacing(6);
        cardLayout->addWidget(footer);

        root->addWidget(card, 0, Qt::AlignCenter);

        auto* eff = new QGraphicsOpacityEffect(card);
        card->setGraphicsEffect(eff);
        auto* fade = new QPropertyAnimation(eff, "opacity", card);
        fade->setDuration(300);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->start(QAbstractAnimation::DeleteWhenStopped);

        connect(playDefaultButton, &QPushButton::clicked, this, &MenuWidget::playDefaultRequested);
        connect(playEndgameButton, &QPushButton::clicked, this, &MenuWidget::playEndgameRequested);
        connect(editConfigsButton, &QPushButton::clicked, this, &MenuWidget::editConfigsRequested);
        connect(editEndgamesButton, &QPushButton::clicked, this, &MenuWidget::editEndgamesRequested);
        connect(helpButton, &QPushButton::clicked, this, &MenuWidget::helpRequested);
        connect(quitButton, &QPushButton::clicked, this, &MenuWidget::quitRequested);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter p(this);
        QLinearGradient g(0, 0, width(), height());
        g.setColorAt(0.0, QColor(10, 20, 40));
        g.setColorAt(1.0, QColor(6, 40, 60));
        p.fillRect(rect(), g);

        QRadialGradient vignette(width() * 0.5, height() * 0.5, max(width(), height()) * 0.6);
        vignette.setColorAt(0.0, QColor(0, 0, 0, 0));
        vignette.setColorAt(1.0, QColor(0, 0, 0, 140));
        p.fillRect(rect(), vignette);

        QWidget::paintEvent(event);
    }

signals:
    void playDefaultRequested();
    void playEndgameRequested();
    void editConfigsRequested();
    void editEndgamesRequested();
    void helpRequested();
    void quitRequested();
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    stack_ = new QStackedWidget(this);
    setCentralWidget(stack_);

    menu_ = new MenuWidget(this);
    game_ = new GameWidget(this);
    configManager_ = new breakout::ConfigManager();
    endgameManager_ = new breakout::EndgameManager();

    stack_->addWidget(menu_);
    stack_->addWidget(game_);

    // Set minimum window size to prevent layout breaking
    setMinimumSize(700, 600);
    resize(800, 700);

    setupConnections();
    setupMenu();
}

void MainWindow::setupMenu() {
    stack_->setCurrentWidget(menu_);
}

void MainWindow::setupConnections() {
    connect(menu_, &MenuWidget::playDefaultRequested, this, [this]() {
        setFixedSize(size());  // Lock window size during gameplay
        stack_->setCurrentWidget(game_);
        game_->startGame();
        game_->setFocus();
    });

    connect(menu_, &MenuWidget::quitRequested, this, [this]() {
        close();
    });

    connect(game_, &GameWidget::returnToMenuRequested, this, [this]() {
        game_->stopGame();
        setMinimumSize(700, 600);  // Unlock window resizing
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        stack_->setCurrentWidget(menu_);
        menu_->setFocus();
    });

    connect(menu_, &MenuWidget::editConfigsRequested, this, [this]() {
        openConfigDialog();
    });
    connect(menu_, &MenuWidget::playEndgameRequested, this, [this]() {
        openEndgameDialog(EndgameDialogMode::Play);
    });
    connect(menu_, &MenuWidget::editEndgamesRequested, this, [this]() {
        openEndgameDialog(EndgameDialogMode::Manage);
    });
    connect(menu_, &MenuWidget::helpRequested, this, [this]() {
        showHowToPlay();
    });
    // Game over now shows overlay in-game; user can press M to return.
    connect(game_, &GameWidget::gameOver, this, [this]() {
        game_->stopGame();
        game_->setFocus();
    });

    connect(game_, &GameWidget::saveEndgameRequested, this, [this]() {
        saveEndgame();
    });
    
    connect(game_, &GameWidget::reloadEndgameRequested, this, [this]() {
        reloadCurrentEndgame();
    });
}

void MainWindow::applyConfig(const breakout::GameConfig& cfg) {
    game_->applyConfig(cfg);
    game_->setFocus();
}

bool MainWindow::playEndgame(const QString& name, QWidget* contextForErrors) {
    breakout::EndgameSnapshot snap;
    QString err;
    if (!endgameManager_->loadEndgame(name, snap, &err)) {
        QMessageBox::warning(contextForErrors, tr("Load failed"), err);
        return false;
    }
    setFixedSize(size());
    stack_->setCurrentWidget(game_);
    game_->loadEndgame(name, snap);
    game_->setFocus();
    return true;
}

QString MainWindow::openEndgameEditor(const breakout::EndgameSnapshot* initial,
                                      const QString& suggestedName) {
    QDialog dlg(this);
    dlg.setWindowTitle(initial ? tr("Edit Endgame") : tr("New Endgame"));
    dlg.resize(760, 740);

    auto* layout = new QVBoxLayout(&dlg);

    auto* nameRow = new QHBoxLayout();
    auto* nameLabel = new QLabel(tr("Name"), &dlg);
    auto* nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(tr("Letters, digits, and underscores"));
    QString namePrefill = suggestedName;
    if (namePrefill.isEmpty() && initial && !initial->name.empty()) {
        namePrefill = QString::fromStdString(initial->name);
    }
    if (namePrefill.isEmpty()) {
        namePrefill = QStringLiteral("endgame");
    }
    nameEdit->setText(namePrefill);
    nameEdit->selectAll();

    nameRow->addWidget(nameLabel);
    nameRow->addWidget(nameEdit);
    layout->addLayout(nameRow);

    auto* editor = new breakout::EndgameEditorWidget(&dlg);
    if (initial) {
        editor->loadSnapshot(*initial);
    }
    layout->addWidget(editor, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);

    QString savedName;

    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, [&]() {
        QString baseName = nameEdit->text().trimmed();
        breakout::EndgameSnapshot snap = editor->buildSnapshot(baseName);
        QStringList errors = endgameManager_->validate(baseName, snap);
        if (!errors.isEmpty()) {
            QMessageBox::warning(&dlg, tr("Invalid endgame"), errors.join('\n'));
            return;
        }

        bool overwrite = false;
        if (endgameManager_->endgameExists(baseName)) {
            auto reply = QMessageBox::question(&dlg,
                                               tr("Overwrite?"),
                                               tr("Endgame '%1' already exists. Overwrite?").arg(baseName),
                                               QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return;
            }
            overwrite = true;
        }

        QString err;
        if (!endgameManager_->saveEndgame(baseName, snap, overwrite, &err)) {
            QMessageBox::warning(&dlg, tr("Save failed"), err);
            return;
        }

        QMessageBox::information(&dlg, tr("Saved"), tr("Endgame saved as %1").arg(baseName));
        savedName = baseName;
        dlg.accept();
    });

    int result = dlg.exec();
    return result == QDialog::Accepted ? savedName : QString();
}

void MainWindow::openConfigDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Configs"));
    auto* layout = new QVBoxLayout(&dlg);
    auto* list = new QListWidget(&dlg);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    auto* btnRow = new QHBoxLayout();
    auto* loadBtn = new QPushButton(tr("Load"), &dlg);
    auto* newBtn = new QPushButton(tr("New"), &dlg);
    auto* editBtn = new QPushButton(tr("Edit"), &dlg);
    auto* delBtn = new QPushButton(tr("Delete"), &dlg);
    auto* closeBtn = new QPushButton(tr("Close"), &dlg);
    btnRow->addWidget(loadBtn);
    btnRow->addWidget(newBtn);
    btnRow->addWidget(editBtn);
    btnRow->addWidget(delBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);

    layout->addWidget(list);
    layout->addLayout(btnRow);

    auto refresh = [list]() {
        list->clear();
        QDir dir("config");
        QStringList files = dir.entryList(QStringList() << "*.config", QDir::Files, QDir::Name);
        for (const auto& f : files) {
            list->addItem(QFileInfo(f).completeBaseName());
        }
    };
    refresh();

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    connect(loadBtn, &QPushButton::clicked, this, [this, list, &dlg]() {
        auto* item = list->currentItem();
        if (!item) return;
        QString name = item->text();
        breakout::GameConfig cfg;
        QString err;
        if (!configManager_->loadConfig(name, cfg, &err)) {
            QMessageBox::warning(&dlg, tr("Load failed"), err);
            return;
        }
        currentConfig_ = cfg;
        applyConfig(cfg);
        QMessageBox::information(&dlg, tr("Loaded"), tr("Config %1 loaded").arg(name));
    });

    connect(newBtn, &QPushButton::clicked, this, [this, list, &dlg, refresh]() {
        bool ok = false;
        breakout::GameConfig cfg = currentConfig_;
        cfg.name = QInputDialog::getText(&dlg, tr("Config name"), tr("Name:"), QLineEdit::Normal, cfg.name, &ok);
        if (!ok || cfg.name.isEmpty()) return;
        cfg.ballSpeed = QInputDialog::getInt(&dlg, tr("Ball speed"), tr("1-10"), cfg.ballSpeed, 1, 10, 1, &ok);
        if (!ok) return;
        cfg.randomSeed = QInputDialog::getInt(&dlg, tr("Random seed"), tr("-1 for time-based"), cfg.randomSeed, -1, numeric_limits<int>::max(), 1, &ok);
        if (!ok) return;
        cfg.startingLevel = QInputDialog::getInt(&dlg, tr("Starting level"), tr(">=1"), cfg.startingLevel, 1, 999, 1, &ok);
        if (!ok) return;
        QString err;
        if (!configManager_->saveConfig(cfg.name, cfg, &err)) {
            QMessageBox::warning(&dlg, tr("Save failed"), err);
            return;
        }
        currentConfig_ = cfg;
        applyConfig(cfg);
        refresh();
        list->setCurrentRow(list->count() - 1);
        QMessageBox::information(&dlg, tr("Saved"), tr("Config %1 saved").arg(cfg.name));
    });

    connect(editBtn, &QPushButton::clicked, this, [this, list, &dlg, refresh]() {
        auto* item = list->currentItem();
        if (!item) return;
        QString name = item->text();
        breakout::GameConfig cfg;
        QString err;
        if (!configManager_->loadConfig(name, cfg, &err)) {
            QMessageBox::warning(&dlg, tr("Load failed"), err);
            return;
        }
        bool ok = false;
        cfg.ballSpeed = QInputDialog::getInt(&dlg, tr("Ball speed"), tr("1-10"), cfg.ballSpeed, 1, 10, 1, &ok);
        if (!ok) return;
        cfg.randomSeed = QInputDialog::getInt(&dlg, tr("Random seed"), tr("-1 for time-based"), cfg.randomSeed, -1, numeric_limits<int>::max(), 1, &ok);
        if (!ok) return;
        cfg.startingLevel = QInputDialog::getInt(&dlg, tr("Starting level"), tr(">=1"), cfg.startingLevel, 1, 999, 1, &ok);
        if (!ok) return;
        if (!configManager_->saveConfig(name, cfg, &err)) {
            QMessageBox::warning(&dlg, tr("Save failed"), err);
            return;
        }
        currentConfig_ = cfg;
        applyConfig(cfg);
        refresh();
        QMessageBox::information(&dlg, tr("Saved"), tr("Config %1 updated").arg(name));
    });

    connect(delBtn, &QPushButton::clicked, this, [list, &dlg, refresh]() {
        auto* item = list->currentItem();
        if (!item) return;
        QString name = item->text();
        QString path = QStringLiteral("config/%1.config").arg(name);
        if (!QFile::exists(path)) {
            QMessageBox::information(&dlg, tr("Not found"), tr("Config %1 does not exist.").arg(name));
            refresh();
            return;
        }
        auto reply = QMessageBox::question(&dlg, tr("Confirm"), tr("Delete config %1?").arg(name));
        if (reply != QMessageBox::Yes) return;
        if (!QFile::remove(path)) {
            QMessageBox::warning(&dlg, tr("Delete failed"), tr("Could not delete %1").arg(path));
            return;
        }
        refresh();
    });

    dlg.exec();
}

void MainWindow::openEndgameDialog(EndgameDialogMode mode) {
    auto promptCreateAndMaybePlay = [&](bool playAfterCreate) {
        auto reply = QMessageBox::question(this,
                                           tr("No endgames"),
                                           tr("No endgames found. Create one now?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return false;
        QString saved = openEndgameEditor(nullptr);
        if (saved.isEmpty()) return false;
        if (playAfterCreate) {
            breakout::EndgameSnapshot snap;
            QString err;
            if (endgameManager_->loadEndgame(saved, snap, &err)) {
                setFixedSize(size());
                stack_->setCurrentWidget(game_);
                game_->loadEndgame(saved, snap);
                game_->setFocus();
                return true;
            }
            QMessageBox::warning(this, tr("Load failed"), err);
        }
        return true;
    };

    QStringList initialNames = endgameManager_->listEndgames();
    if (initialNames.isEmpty()) {
        bool played = promptCreateAndMaybePlay(mode == EndgameDialogMode::Play);
        if (!played && mode == EndgameDialogMode::Manage) {
            // If user created but didn't want to play, just fall through to dialog refresh
            initialNames = endgameManager_->listEndgames();
            if (initialNames.isEmpty()) return;
        } else {
            return;
        }
    }

    QDialog dlg(this);
    dlg.setWindowTitle(mode == EndgameDialogMode::Play ? tr("Play Endgame") : tr("Manage Endgames"));
    dlg.resize(540, 560);

    auto* layout = new QVBoxLayout(&dlg);

    auto* intro = new QLabel(mode == EndgameDialogMode::Play
                                 ? tr("Pick an endgame to play, or create a new one.")
                                 : tr("Create, edit, or delete endgames."),
                             &dlg);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto* list = new QListWidget(&dlg);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list, 1);

    auto* emptyLabel = new QLabel(tr("No endgames found. Click New to create one."), &dlg);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet(QStringLiteral("color: #888;"));
    layout->addWidget(emptyLabel);

    auto* btnRow = new QHBoxLayout();
    auto* loadBtn = mode == EndgameDialogMode::Play ? new QPushButton(tr("Play"), &dlg) : nullptr;
    auto* newBtn = new QPushButton(tr("New"), &dlg);
    auto* editBtn = new QPushButton(tr("Edit"), &dlg);
    auto* delBtn = new QPushButton(tr("Delete"), &dlg);
    auto* closeBtn = new QPushButton(tr("Close"), &dlg);

    if (loadBtn) btnRow->addWidget(loadBtn);
    btnRow->addWidget(newBtn);
    btnRow->addWidget(editBtn);
    btnRow->addWidget(delBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    editBtn->setVisible(mode == EndgameDialogMode::Manage);
    delBtn->setVisible(mode == EndgameDialogMode::Manage);

    auto updateButtonStates = [list, loadBtn, editBtn, delBtn]() {
        bool hasSelection = list->currentItem() != nullptr;
        if (loadBtn) loadBtn->setEnabled(hasSelection);
        editBtn->setEnabled(hasSelection);
        delBtn->setEnabled(hasSelection);
    };

    auto refresh = [this, list, emptyLabel, updateButtonStates]() {
        list->clear();
        QStringList names = endgameManager_->listEndgames();
        for (const auto& name : names) {
            list->addItem(name);
        }
        bool empty = names.isEmpty();
        list->setVisible(!empty);
        emptyLabel->setVisible(empty);
        updateButtonStates();
    };
    refresh();

    auto loadSelection = [this, list, &dlg]() {
        auto* item = list->currentItem();
        if (!item) return;
        QString name = item->text();
        if (!playEndgame(name, &dlg)) return;
        dlg.accept();
    };

    auto editSelection = [this, list, refresh, &dlg]() {
        auto* item = list->currentItem();
        if (!item) return;
        QString name = item->text();
        breakout::EndgameSnapshot snap;
        QString err;
        if (!endgameManager_->loadEndgame(name, snap, &err)) {
            QMessageBox::warning(&dlg, tr("Load failed"), err);
            refresh();
            return;
        }
        QString saved = openEndgameEditor(&snap, name);
        if (!saved.isEmpty()) {
            refresh();
            auto matches = list->findItems(saved, Qt::MatchExactly);
            if (!matches.isEmpty()) {
                list->setCurrentItem(matches.first());
            }
        }
    };

    connect(list, &QListWidget::itemSelectionChanged, this, [updateButtonStates]() { updateButtonStates(); });
    connect(list, &QListWidget::itemDoubleClicked, this, [mode, loadSelection, editSelection]() {
        if (mode == EndgameDialogMode::Play) {
            loadSelection();
        } else {
            editSelection();
        }
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (loadBtn) {
        connect(loadBtn, &QPushButton::clicked, this, [loadSelection]() {
            loadSelection();
        });
    }

    connect(newBtn, &QPushButton::clicked, this, [this, list, refresh, mode, loadSelection]() {
        QString saved = openEndgameEditor(nullptr);
        if (saved.isEmpty()) return;
        refresh();
        auto matches = list->findItems(saved, Qt::MatchExactly);
        if (!matches.isEmpty()) {
            list->setCurrentItem(matches.first());
        }
        if (mode == EndgameDialogMode::Play) {
            loadSelection();
        }
    });

    connect(editBtn, &QPushButton::clicked, this, [editSelection]() { editSelection(); });

    connect(delBtn, &QPushButton::clicked, this, [list, refresh, &dlg]() {
        auto* item = list->currentItem();
        if (!item) return;
        QString name = item->text();
        QString path = QStringLiteral("endgames/%1.end").arg(name);
        auto reply = QMessageBox::question(&dlg, tr("Confirm"), tr("Delete endgame %1?").arg(name));
        if (reply != QMessageBox::Yes) return;
        if (!QFile::remove(path)) {
            QMessageBox::warning(&dlg, tr("Delete failed"), tr("Could not delete %1").arg(path));
            return;
        }
        refresh();
    });

    dlg.exec();
}

void MainWindow::showHowToPlay() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("How to Play"));
    dlg.resize(540, 520);

    auto* layout = new QVBoxLayout(&dlg);
    auto* title = new QLabel(tr("Breakout Primer"), &dlg);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    auto* browser = new QTextBrowser(&dlg);
    browser->setReadOnly(true);
    browser->setOpenExternalLinks(true);
    browser->setHtml(R"(
        <h3>Goal</h3>
        <p>Keep the ball in play, clear bricks, and chase high scores. Power-ups and endgames add variety.</p>

        <h3>Controls</h3>
        <ul>
            <li><b>Move:</b> Left/Right arrows or A/D</li>
            <li><b>Launch ball:</b> Space</li>
            <li><b>Pause/Resume:</b> P or Esc</li>
            <li><b>Save endgame snapshot:</b> S (when available)</li>
            <li><b>Load last endgame:</b> L (when available)</li>
        </ul>

        <h3>Menus</h3>
        <ul>
            <li><b>Start:</b> Begin a new run with the current config.</li>
            <li><b>Config Menu:</b> Create, load, edit, or delete game configs (speed, seed, starting level).</li>
            <li><b>Endgames Menu:</b> Load, create, edit, or delete endgame snapshots for custom setups.</li>
            <li><b>How to Play:</b> You are here.</li>
        </ul>

        <h3>Power-ups & Combos</h3>
        <ul>
            <li>Catch power-ups to gain perks (speed tweaks, paddle changes, multiball, etc.).</li>
            <li>Maintain hit streaks to trigger score combos and faster clears.</li>
        </ul>

        <h3>Tips</h3>
        <ul>
            <li>Use pause to plan angles before tricky shots.</li>
            <li>Configs let you practice: lower speed or start at higher levels.</li>
            <li>Save endgames when you have an interesting board to revisit.</li>
        </ul>
    )");

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(browser);
    layout->addWidget(buttons);

    dlg.exec();
}

void MainWindow::saveEndgame() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Save Endgame"), tr("Name:"), QLineEdit::Normal, QStringLiteral("endgame"), &ok);
    if (!ok || name.isEmpty()) return;

    breakout::EndgameSnapshot snap = game_->captureEndgame(name);

    QStringList errors = endgameManager_->validate(name, snap);
    if (!errors.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid endgame"), errors.join('\n'));
        return;
    }

    bool overwrite = false;
    if (endgameManager_->endgameExists(name)) {
        auto reply = QMessageBox::question(this,
                                           tr("Overwrite?"),
                                           tr("Endgame '%1' already exists. Overwrite?").arg(name),
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
        overwrite = true;
    }

    QString err;
    if (!endgameManager_->saveEndgame(name, snap, overwrite, &err)) {
        QMessageBox::warning(this, tr("Save failed"), err);
        return;
    }

    QMessageBox::information(this, tr("Saved"), tr("Endgame saved as %1").arg(name));
}

void MainWindow::loadEndgame() {
    QDir dir("endgames");
    QStringList files = dir.entryList(QStringList() << "*.end", QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        QMessageBox::information(this, tr("No endgames"), tr("No .end files found in endgames/"));
        return;
    }
    bool ok = false;
    QString file = QInputDialog::getItem(this, tr("Load Endgame"), tr("Choose endgame:"), files, 0, false, &ok);
    if (!ok || file.isEmpty()) return;
    QString name = QFileInfo(file).completeBaseName();
    playEndgame(name, this);
}

void MainWindow::reloadCurrentEndgame() {
    // Get the filename from GameWidget and reload from disk
    QString filename = game_->loadedEndgameFilename();
    if (filename.isEmpty()) {
        qWarning() << "reloadCurrentEndgame called but no filename stored";
        return;
    }
    
    breakout::EndgameSnapshot snap;
    QString err;
    if (!endgameManager_->loadEndgame(filename, snap, &err)) {
        QMessageBox::warning(this, tr("Reload failed"), err);
        return;
    }
    
    game_->loadEndgame(filename, snap);
}

void MainWindow::editConfig() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Edit Config"), tr("Config name:"), QLineEdit::Normal, currentConfig_.name, &ok);
    if (!ok || name.isEmpty()) return;
    breakout::GameConfig cfg;
    QString err;
    if (!configManager_->loadConfig(name, cfg, &err)) {
        QMessageBox::warning(this, tr("Load failed"), err);
        return;
    }

    cfg.ballSpeed = QInputDialog::getInt(this, tr("Ball speed"), tr("1-10"), cfg.ballSpeed, 1, 10, 1, &ok);
    if (!ok) return;
    cfg.randomSeed = QInputDialog::getInt(this, tr("Random seed"), tr("-1 for time-based"), cfg.randomSeed, -1, numeric_limits<int>::max(), 1, &ok);
    if (!ok) return;
    cfg.startingLevel = QInputDialog::getInt(this, tr("Starting level"), tr(">=1"), cfg.startingLevel, 1, 999, 1, &ok);
    if (!ok) return;

    cfg.name = name;
    if (!configManager_->saveConfig(name, cfg, &err)) {
        QMessageBox::warning(this, tr("Save failed"), err);
        return;
    }
    currentConfig_ = cfg;
    applyConfig(cfg);
    QMessageBox::information(this, tr("Saved"), tr("Config %1 updated").arg(name));
}

void MainWindow::deleteConfig() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Delete Config"), tr("Config name:"), QLineEdit::Normal, currentConfig_.name, &ok);
    if (!ok || name.isEmpty()) return;
    QString path = QStringLiteral("config/%1.config").arg(name);
    if (!QFile::exists(path)) {
        QMessageBox::information(this, tr("Not found"), tr("Config %1 does not exist.").arg(name));
        return;
    }
    auto reply = QMessageBox::question(this, tr("Confirm"), tr("Delete config %1?").arg(name));
    if (reply != QMessageBox::Yes) return;
    if (!QFile::remove(path)) {
        QMessageBox::warning(this, tr("Delete failed"), tr("Could not delete %1").arg(path));
        return;
    }
    QMessageBox::information(this, tr("Deleted"), tr("Config %1 deleted.").arg(name));
}

void MainWindow::editEndgame() {
    QDir dir("endgames");
    QStringList files = dir.entryList(QStringList() << "*.end", QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        QMessageBox::information(this, tr("No endgames"), tr("No .end files found in endgames/"));
        return;
    }
    bool ok = false;
    QString file = QInputDialog::getItem(this, tr("Edit Endgame"), tr("Choose endgame:"), files, 0, false, &ok);
    if (!ok || file.isEmpty()) return;
    QString name = QFileInfo(file).completeBaseName();

    breakout::EndgameSnapshot snap;
    QString err;
    if (!endgameManager_->loadEndgame(name, snap, &err)) {
        QMessageBox::warning(this, tr("Load failed"), err);
        return;
    }
    openEndgameEditor(&snap, name);
}

void MainWindow::deleteEndgame() {
    QDir dir("endgames");
    QStringList files = dir.entryList(QStringList() << "*.end", QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        QMessageBox::information(this, tr("No endgames"), tr("No .end files found in endgames/"));
        return;
    }
    bool ok = false;
    QString file = QInputDialog::getItem(this, tr("Delete Endgame"), tr("Choose endgame:"), files, 0, false, &ok);
    if (!ok || file.isEmpty()) return;
    QString path = QStringLiteral("endgames/%1").arg(file);
    auto reply = QMessageBox::question(this, tr("Confirm"), tr("Delete endgame %1?").arg(file));
    if (reply != QMessageBox::Yes) return;
    if (!QFile::remove(path)) {
        QMessageBox::warning(this, tr("Delete failed"), tr("Could not delete %1").arg(path));
        return;
    }
    QMessageBox::information(this, tr("Deleted"), tr("Endgame %1 deleted.").arg(file));
}

#include "main_window.moc"
