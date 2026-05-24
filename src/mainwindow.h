#pragma once

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QSettings>

#include "gameboard.h"

// We use an enum to track which of the three game modes is currently active.
enum class GameMode { Normal, Unlimited, Hard };

// TileLabel is a single cell on the board.
// It extends QLabel so we can display the tile value and update its color easily.
class TileLabel : public QLabel {
    Q_OBJECT
public:
    explicit TileLabel(QWidget *parent = nullptr);

    // Updates the displayed number and reapplies the matching background color.
    void setValue(int v);

private:
    void applyStyle(int v);
};

// MainWindow is the top-level widget that owns everything.
// It connects the GameBoard logic to the Qt UI, handles keyboard input,
// manages the hard mode timer, and shows win/lose overlays.
class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    // We override keyPressEvent to handle arrow keys, WASD, U (undo), and R (restart).
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onUndo();
    void onRestart();
    void onHardModeTimeout();  // called when the 5-second hard mode timer expires
    void onModeNormal();
    void onModeUnlimited();
    void onModeHard();

private:
    // Builds all widgets and layouts. Called once from the constructor.
    void buildUi();

    // Reads the current board from GameBoard and updates all TileLabels.
    void updateBoardDisplay();

    // Refreshes the score and best score labels.
    void updateScoreDisplay();

    // Shows a message (e.g. "You Win!") on top of the board using an overlay widget.
    void showOverlay(const QString& message);
    void hideOverlay();

    // Switches the active game mode and restarts the game.
    void setMode(GameMode mode);

    // Starts or resets the 5-second countdown used in hard mode.
    void resetHardTimer();
    void stopHardTimer();

    // Executes one slide, updates the display, and checks win/lose conditions.
    bool performSlide(Direction dir);

    // Best score is saved between sessions using QSettings.
    int bestScore_ = 0;
    void loadBestScore();
    void saveBestScore();

    GameBoard board_;
    GameMode  mode_      = GameMode::Normal;
    bool      gameEnded_ = false;  // input is disabled while this is true

    // Tile grid — one TileLabel per cell
    TileLabel*   tiles_[N][M];
    QLabel*      scoreLabel_;
    QLabel*      bestLabel_;
    QPushButton* undoBtn_;
    QPushButton* restartBtn_;
    QPushButton* normalBtn_;
    QPushButton* unlimitedBtn_;
    QPushButton* hardBtn_;

    // Overlay widget that appears on top of the board for win/game over messages
    QWidget* overlayWidget_;
    QLabel*  overlayLabel_;

    // Timer used in hard mode — fires once after HARD_MODE_TIMEOUT_SEC seconds
    QTimer* hardTimer_;
};
