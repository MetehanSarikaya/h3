#include "gameboard.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <numeric>

// We initialize the board as an N×M grid filled with zeros.
// srand is seeded with the current time so we get different results each run.
GameBoard::GameBoard()
    : score_(0)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    board_.assign(N, std::vector<int>(M, 0));
}

// We clear everything and place two starting tiles.
// The history stack is also wiped so undo cannot go before the new game.
void GameBoard::reset()
{
    while (!history_.empty()) history_.pop();
    score_ = 0;
    board_.assign(N, std::vector<int>(M, 0));
    spawnTile();
    spawnTile();
}

// We process a single row/column as a vector of ints.
// First we remove zeros, then we merge equal adjacent pairs from left to right.
// The leftmost pair merges first, which satisfies the "closer to border" rule.
// Finally we pad the result back to its original length with zeros.
std::vector<int> GameBoard::slideLine(const std::vector<int>& line, int& mergeScore)
{
    int len = static_cast<int>(line.size());

    std::vector<int> compact;
    compact.reserve(len);
    for (int v : line)
        if (v != 0)
            compact.push_back(v);

    std::vector<int> merged;
    merged.reserve(len);
    for (int i = 0; i < static_cast<int>(compact.size()); ) {
        if (i + 1 < static_cast<int>(compact.size()) &&
            compact[i] == compact[i + 1])
        {
            // Two equal tiles merge into one tile worth double the value.
            // We add the new value to mergeScore so the caller can update the score.
            int newVal = compact[i] * 2;
            merged.push_back(newVal);
            mergeScore += newVal;
            i += 2;
        } else {
            merged.push_back(compact[i]);
            ++i;
        }
    }

    while (static_cast<int>(merged.size()) < len)
        merged.push_back(0);

    return merged;
}

// We use a static helper to simulate the slide without touching the real board.
// This lets us check whether a move is valid before committing to it,
// which is important because invalid moves should not trigger tile spawns or undo saves.
// For Right/Down we reverse the line before sliding so we can always slide "left".
static bool computeSlide(
    const std::vector<std::vector<int>>& board,
    Direction dir,
    std::vector<std::vector<int>>& newBoard,
    int& totalMerge,
    GameBoard*)
{
    auto slideLineLocal = [](const std::vector<int>& line, int& mergeScore) {
        int len = static_cast<int>(line.size());
        std::vector<int> compact;
        for (int v : line)
            if (v != 0)
                compact.push_back(v);

        std::vector<int> merged;
        for (int i = 0; i < static_cast<int>(compact.size()); ) {
            if (i + 1 < static_cast<int>(compact.size()) &&
                compact[i] == compact[i + 1])
            {
                int nv = compact[i] * 2;
                merged.push_back(nv);
                mergeScore += nv;
                i += 2;
            } else {
                merged.push_back(compact[i]);
                ++i;
            }
        }
        while (static_cast<int>(merged.size()) < len)
            merged.push_back(0);
        return merged;
    };

    newBoard = board;
    totalMerge = 0;
    bool changed = false;

    if (dir == Direction::Left || dir == Direction::Right) {
        for (int r = 0; r < N; ++r) {
            std::vector<int> line(board[r]);

            // We reverse the line for rightward slides so the same
            // slideLine logic always works towards index 0.
            if (dir == Direction::Right)
                std::reverse(line.begin(), line.end());

            int ms = 0;
            std::vector<int> result = slideLineLocal(line, ms);
            totalMerge += ms;

            if (dir == Direction::Right)
                std::reverse(result.begin(), result.end());

            if (result != board[r]) {
                newBoard[r] = result;
                changed = true;
            }
        }
    } else {
        // For Up/Down we extract each column as a temporary vector,
        // slide it, then write the result back into the column.
        for (int c = 0; c < M; ++c) {
            std::vector<int> line(N);
            for (int r = 0; r < N; ++r)
                line[r] = board[r][c];

            if (dir == Direction::Down)
                std::reverse(line.begin(), line.end());

            int ms = 0;
            std::vector<int> result = slideLineLocal(line, ms);
            totalMerge += ms;

            if (dir == Direction::Down)
                std::reverse(result.begin(), result.end());

            for (int r = 0; r < N; ++r) {
                if (board[r][c] != result[r]) {
                    newBoard[r][c] = result[r];
                    changed = true;
                }
            }
        }
    }

    return changed;
}

// We first simulate the slide to check validity.
// Only if something actually changed do we save the state and spawn a new tile.
// This order matters: saveState must be called before board_ is overwritten.
bool GameBoard::slide(Direction dir)
{
    std::vector<std::vector<int>> newBoard;
    int totalMerge = 0;

    bool valid = computeSlide(board_, dir, newBoard, totalMerge, this);
    if (!valid)
        return false;

    saveState();
    board_  = newBoard;
    score_ += totalMerge;
    spawnTile();
    return true;
}

// We restore the board and score from the top of the history stack.
// Since there is no limit on undo steps, the player can go back to the start.
bool GameBoard::undo()
{
    if (history_.empty())
        return false;

    GameState prev = history_.top();
    history_.pop();

    board_ = prev.board;
    score_ = prev.score;
    return true;
}

// We check for game over in two steps to keep it fast.
// If any cell is empty, the game is clearly not over.
// Otherwise we look for any two adjacent equal tiles (horizontal or vertical).
bool GameBoard::isGameOver() const
{
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < M; ++c)
            if (board_[r][c] == 0)
                return false;

    for (int r = 0; r < N; ++r)
        for (int c = 0; c + 1 < M; ++c)
            if (board_[r][c] == board_[r][c + 1])
                return false;

    for (int r = 0; r + 1 < N; ++r)
        for (int c = 0; c < M; ++c)
            if (board_[r][c] == board_[r + 1][c])
                return false;

    return true;
}

// We scan every cell and return true as soon as we find the winning tile value K.
bool GameBoard::isGameWon() const
{
    for (const auto& row : board_)
        for (int v : row)
            if (v == K)
                return true;
    return false;
}

// We use this in hard mode to find all directions that would result in a valid move.
// The random automatic move is chosen from this list so it is never wasted.
std::vector<Direction> GameBoard::getValidMoves() const
{
    static const Direction all[] = {
        Direction::Up, Direction::Down, Direction::Left, Direction::Right
    };
    std::vector<Direction> valid;
    for (Direction d : all) {
        std::vector<std::vector<int>> nb;
        int tm = 0;
        if (computeSlide(board_, d, nb, tm, nullptr))
            valid.push_back(d);
    }
    return valid;
}

// We collect all empty cells first, then pick one at random.
// The new tile is 2 with P% probability and 4 with Q% probability.
void GameBoard::spawnTile()
{
    std::vector<std::pair<int,int>> empties;
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < M; ++c)
            if (board_[r][c] == 0)
                empties.emplace_back(r, c);

    if (empties.empty())
        return;

    auto [r, c] = empties[std::rand() % empties.size()];
    int roll = std::rand() % 100;
    board_[r][c] = (roll < P) ? 2 : 4;
}

// We save a full copy of the board and the current score.
// Using a stack means the most recent state is always on top, which is exactly what undo needs.
void GameBoard::saveState()
{
    history_.push({ board_, score_ });
}
