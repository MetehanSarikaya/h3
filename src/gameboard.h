#pragma once

#include <vector>
#include <stack>
#include <utility>

#include "constants.h"

// We store the board and score together so we can save and restore
// the full game state in a single object for the undo feature.
struct GameState {
    std::vector<std::vector<int>> board;
    int score;
};

// Direction enum used to pass slide direction between the UI and game logic.
enum class Direction { Up, Down, Left, Right };

// GameBoard handles all game logic: sliding, merging, spawning tiles,
// win/lose detection, and the undo history.
// We kept it completely separate from Qt so it is easy to test and reuse.
class GameBoard {
public:
    GameBoard();

    // Resets everything and places two starting tiles, both forced to value 2
    // as required by the assignment. P/Q probabilities only apply to tiles
    // spawned after a player move, not to the initial board state.
    void reset();

    // Tries to slide in the given direction.
    // Returns true only if at least one tile moved or merged (valid move).
    bool slide(Direction dir);

    // Reverts the board and score to the state before the last move.
    // Returns false if there is nothing to undo.
    bool undo();

    // Returns true when every cell is filled and no adjacent tiles can merge.
    bool isGameOver() const;

    // Returns true when any tile on the board equals the winning value K.
    bool isGameWon() const;

    const std::vector<std::vector<int>>& getBoard() const { return board_; }
    int getScore() const { return score_; }

    // Returns every direction that would produce a valid move.
    // We use this in hard mode to pick a random legal automatic slide.
    std::vector<Direction> getValidMoves() const;

private:
    std::vector<std::vector<int>> board_;
    int score_;
    std::stack<GameState> history_;

    // Places a new tile in a random empty cell.
    // If forcedValue is non-zero, that value is used directly (bypassing P/Q).
    // This lets reset() always spawn two 2-tiles regardless of the probabilities.
    void spawnTile(int forcedValue = 0);

    // Saves the current board and score to the history stack.
    void saveState();
};