#include "mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QFrame>
#include <QPalette>
#include <QSettings>
#include <QSizePolicy>
#include <QStackedLayout>
#include <cstdlib>
#include <ctime>

// We map each tile value to the background color used in the original 2048 game.
// Values above 2048 get a dark color since they only appear in Unlimited mode.
static QColor tileBackground(int v)
{
    switch (v) {
    case    0: return QColor("#cdc1b4");
    case    2: return QColor("#eee4da");
    case    4: return QColor("#ede0c8");
    case    8: return QColor("#f2b179");
    case   16: return QColor("#f59563");
    case   32: return QColor("#f67c5f");
    case   64: return QColor("#f65e3b");
    case  128: return QColor("#edcf72");
    case  256: return QColor("#edcc61");
    case  512: return QColor("#edc850");
    case 1024: return QColor("#edc53f");
    case 2048: return QColor("#edc22e");
    default:   return QColor("#3c3a32");
    }
}

// Small tiles (2 and 4) use a dark text color; all others use white for better contrast.
static QColor tileForeground(int v)
{
    return (v <= 4) ? QColor("#776e65") : QColor("#f9f6f2");
}


//  TileLabel


TileLabel::TileLabel(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setMinimumSize(80, 80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setValue(0);
}

void TileLabel::setValue(int v)
{
    setText(v > 0 ? QString::number(v) : "");
    applyStyle(v);
}

// We use a stylesheet to change the tile's appearance instead of custom painting,
// which keeps the code simpler. Font size shrinks for larger numbers so they fit.
void TileLabel::applyStyle(int v)
{
    QColor bg = tileBackground(v);
    QColor fg = tileForeground(v);

    int fs = 32;
    if (v >= 1000)     fs = 22;
    else if (v >= 100) fs = 26;

    setStyleSheet(QString(
        "background-color: %1;"
        "color: %2;"
        "border-radius: 6px;"
        "font-size: %3px;"
        "font-weight: 800;"
        "font-family: 'Clear Sans', 'Helvetica Neue', Arial, sans-serif;"
    ).arg(bg.name()).arg(fg.name()).arg(fs));
}


//  MainWindow

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , hardTimer_(new QTimer(this))
{
    setWindowTitle("2048");
    setMinimumSize(480, 600);

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    loadBestScore();
    buildUi();

    // We set the timer to single-shot so it fires once and stops.
    // It is restarted after every valid move in hard mode.
    hardTimer_->setSingleShot(true);
    hardTimer_->setInterval(HARD_MODE_TIMEOUT_SEC * 1000);
    connect(hardTimer_, &QTimer::timeout, this, &MainWindow::onHardModeTimeout);

    board_.reset();
    updateBoardDisplay();
    updateScoreDisplay();
}

// We build the entire UI here programmatically instead of using .ui files
// so the project has fewer dependencies and is easier to understand.
void MainWindow::buildUi()
{
    setStyleSheet("QWidget { background-color: #faf8ef; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Header: big title on the left, score boxes on the right
    auto *header = new QHBoxLayout();

    auto *titleLabel = new QLabel("2048", this);
    titleLabel->setStyleSheet(
        "font-size: 56px; font-weight: 900; color: #776e65;"
        "font-family: 'Clear Sans', 'Helvetica Neue', Arial, sans-serif;");

    // We use a lambda to avoid duplicating the score box setup code.
    auto makeScoreBox = [&](const QString& label) -> QLabel* {
        auto *box = new QLabel(this);
        box->setAlignment(Qt::AlignCenter);
        box->setMinimumWidth(90);
        box->setStyleSheet(
            "background-color: #bbada0; border-radius: 6px;"
            "color: white; font-weight: 800; padding: 8px 4px;"
            "font-family: 'Clear Sans', 'Helvetica Neue', Arial, sans-serif;"
            "font-size: 13px;");
        box->setText(QString(
            "<div style='font-size:10px;'>%1</div>"
            "<div style='font-size:20px;'>0</div>").arg(label));
        return box;
    };

    scoreLabel_ = makeScoreBox("SCORE");
    bestLabel_  = makeScoreBox("BEST");

    header->addWidget(titleLabel);
    header->addStretch();
    header->addWidget(scoreLabel_);
    header->addSpacing(8);
    header->addWidget(bestLabel_);
    root->addLayout(header);

    auto *sub = new QLabel("Join the tiles, get to 2048!", this);
    sub->setStyleSheet("color: #776e65; font-size: 13px;");
    root->addWidget(sub);

    // Mode selection buttons — we make them checkable so the active mode is visible.
    auto *modeRow = new QHBoxLayout();
    auto makeBtn = [&](const QString& text) -> QPushButton* {
        auto *b = new QPushButton(text, this);
        b->setCheckable(true);
        b->setStyleSheet(
            "QPushButton { background-color: #8f7a66; color: white; border-radius: 4px;"
            " padding: 6px 14px; font-weight: 700; font-size: 13px; border: none; }"
            "QPushButton:checked { background-color: #f65e3b; }"
            "QPushButton:hover   { background-color: #a08070; }");
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };

    normalBtn_    = makeBtn("Normal");
    unlimitedBtn_ = makeBtn("Unlimited");
    hardBtn_      = makeBtn("Hard");
    normalBtn_->setChecked(true);

    modeRow->addWidget(normalBtn_);
    modeRow->addWidget(unlimitedBtn_);
    modeRow->addWidget(hardBtn_);
    modeRow->addStretch();
    root->addLayout(modeRow);

    connect(normalBtn_,    &QPushButton::clicked, this, &MainWindow::onModeNormal);
    connect(unlimitedBtn_, &QPushButton::clicked, this, &MainWindow::onModeUnlimited);
    connect(hardBtn_,      &QPushButton::clicked, this, &MainWindow::onModeHard);

    // Undo and Restart buttons — keyboard shortcuts are shown in the labels.
    auto *actionRow = new QHBoxLayout();
    auto makeActionBtn = [&](const QString& text) -> QPushButton* {
        auto *b = new QPushButton(text, this);
        b->setStyleSheet(
            "QPushButton { background-color: #8f7a66; color: white; border-radius: 4px;"
            " padding: 6px 14px; font-weight: 700; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #a08070; }");
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };

    undoBtn_    = makeActionBtn("← Undo (U)");
    restartBtn_ = makeActionBtn("↺ Restart (R)");

    actionRow->addWidget(undoBtn_);
    actionRow->addSpacing(8);
    actionRow->addWidget(restartBtn_);
    actionRow->addStretch();
    root->addLayout(actionRow);

    connect(undoBtn_,    &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(restartBtn_, &QPushButton::clicked, this, &MainWindow::onRestart);

    // We use a QStackedLayout so the win/lose overlay can sit directly on top of the board.
    auto *boardContainer = new QWidget(this);
    boardContainer->setStyleSheet("background-color: #bbada0; border-radius: 8px;");
    boardContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *stackLayout = new QStackedLayout(boardContainer);
    stackLayout->setStackingMode(QStackedLayout::StackAll);

    auto *gridWidget = new QWidget(boardContainer);
    auto *grid = new QGridLayout(gridWidget);
    grid->setSpacing(12);
    grid->setContentsMargins(12, 12, 12, 12);

    for (int r = 0; r < N; ++r)
        for (int c = 0; c < M; ++c) {
            tiles_[r][c] = new TileLabel(gridWidget);
            grid->addWidget(tiles_[r][c], r, c);
        }

    // The overlay starts hidden and is only shown when the game ends.
    overlayWidget_ = new QWidget(boardContainer);
    overlayWidget_->setStyleSheet(
        "background-color: rgba(238,228,218,0.73); border-radius: 8px;");
    overlayWidget_->hide();

    overlayLabel_ = new QLabel(overlayWidget_);
    overlayLabel_->setAlignment(Qt::AlignCenter);
    overlayLabel_->setStyleSheet(
        "font-size: 48px; font-weight: 900; color: #776e65;"
        "background: transparent;"
        "font-family: 'Clear Sans', 'Helvetica Neue', Arial, sans-serif;");

    auto *overlayLayout = new QVBoxLayout(overlayWidget_);
    overlayLayout->addWidget(overlayLabel_);

    stackLayout->addWidget(gridWidget);
    stackLayout->addWidget(overlayWidget_);

    root->addWidget(boardContainer, 1);

    auto *hint = new QLabel("Arrow keys or WASD to play  ·  U = Undo  ·  R = Restart", this);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("color: #bbada0; font-size: 11px;");
    root->addWidget(hint);
}

// We iterate over the entire board and update each TileLabel with its current value.
void MainWindow::updateBoardDisplay()
{
    const auto& b = board_.getBoard();
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < M; ++c)
            tiles_[r][c]->setValue(b[r][c]);
}

// We also update bestScore here so we only need one call after each move.
void MainWindow::updateScoreDisplay()
{
    int s = board_.getScore();
    if (s > bestScore_) {
        bestScore_ = s;
        saveBestScore();
    }

    auto fmt = [](const QString& lbl, int val) {
        return QString(
            "<div style='font-size:10px;color:#eee4da;'>%1</div>"
            "<div style='font-size:20px;font-weight:900;'>%2</div>")
            .arg(lbl).arg(val);
    };
    scoreLabel_->setText(fmt("SCORE", s));
    bestLabel_->setText(fmt("BEST",  bestScore_));
}

void MainWindow::showOverlay(const QString& message)
{
    overlayLabel_->setText(message);
    overlayWidget_->show();
    overlayWidget_->raise();
}

void MainWindow::hideOverlay()
{
    overlayWidget_->hide();
}

// performSlide is the central function that connects a key press to the game logic.
// We check validity, update the display, handle the hard mode timer, and detect end conditions.
bool MainWindow::performSlide(Direction dir)
{
    if (gameEnded_)
        return false;

    bool valid = board_.slide(dir);
    if (!valid)
        return false;

    updateBoardDisplay();
    updateScoreDisplay();

    // After a successful move in hard mode, we reset the 5-second countdown.
    if (mode_ == GameMode::Hard)
        resetHardTimer();

    // In Unlimited mode we skip the win check so the game continues past 2048.
    if (mode_ != GameMode::Unlimited && board_.isGameWon()) {
        gameEnded_ = true;
        stopHardTimer();
        showOverlay("You Win! 🎉");
        return true;
    }

    if (board_.isGameOver()) {
        gameEnded_ = true;
        stopHardTimer();
        showOverlay("Game Over!");
        return true;
    }

    return true;
}

// We override keyPressEvent instead of using QShortcut so we can handle
// both arrow keys and WASD in the same place and block input when the game ends.
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (gameEnded_) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Up:    case Qt::Key_W: performSlide(Direction::Up);    break;
    case Qt::Key_Down:  case Qt::Key_S: performSlide(Direction::Down);  break;
    case Qt::Key_Left:  case Qt::Key_A: performSlide(Direction::Left);  break;
    case Qt::Key_Right: case Qt::Key_D: performSlide(Direction::Right); break;
    case Qt::Key_U: onUndo();    break;
    case Qt::Key_R: onRestart(); break;
    default: QWidget::keyPressEvent(event); break;
    }
}

void MainWindow::onUndo()
{
    if (gameEnded_) return;

    if (board_.undo()) {
        updateBoardDisplay();
        updateScoreDisplay();
        hideOverlay();
        if (mode_ == GameMode::Hard)
            resetHardTimer();
    }
}

void MainWindow::onRestart()
{
    gameEnded_ = false;
    hideOverlay();
    board_.reset();
    updateBoardDisplay();
    updateScoreDisplay();

    if (mode_ == GameMode::Hard)
        resetHardTimer();
    else
        stopHardTimer();
}

// When the timer fires, we pick a random direction from the list of valid moves
// and execute it automatically on behalf of the player.
void MainWindow::onHardModeTimeout()
{
    if (gameEnded_ || mode_ != GameMode::Hard)
        return;

    auto valid = board_.getValidMoves();
    if (valid.empty()) {
        gameEnded_ = true;
        showOverlay("Game Over!");
        return;
    }

    Direction dir = valid[std::rand() % valid.size()];
    performSlide(dir);
}

// Switching modes always restarts the game so the player starts fresh.
void MainWindow::setMode(GameMode m)
{
    mode_ = m;
    normalBtn_->setChecked(m == GameMode::Normal);
    unlimitedBtn_->setChecked(m == GameMode::Unlimited);
    hardBtn_->setChecked(m == GameMode::Hard);
    onRestart();
}

void MainWindow::onModeNormal()    { setMode(GameMode::Normal); }
void MainWindow::onModeUnlimited() { setMode(GameMode::Unlimited); }
void MainWindow::onModeHard()      { setMode(GameMode::Hard); }

void MainWindow::resetHardTimer()
{
    hardTimer_->stop();
    if (mode_ == GameMode::Hard && !gameEnded_)
        hardTimer_->start();
}

void MainWindow::stopHardTimer()
{
    hardTimer_->stop();
}

// We use QSettings to persist the best score across sessions.
// It is stored under the organization and application name set in main.cpp.
void MainWindow::loadBestScore()
{
    QSettings settings("CMPE230", "2048");
    bestScore_ = settings.value("bestScore", 0).toInt();
}

void MainWindow::saveBestScore()
{
    QSettings settings("CMPE230", "2048");
    settings.setValue("bestScore", bestScore_);
}
