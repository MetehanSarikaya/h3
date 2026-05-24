#pragma once

// All game parameters are defined here as constants.
// We put them in one place so they are easy to change without touching the logic.

// Board dimensions — N rows and M columns.
constexpr int N = 4;
constexpr int M = 4;

// The tile value the player needs to reach to win.
constexpr int K = 2048;

// Probability of spawning a 2-tile (P%) or a 4-tile (Q%) after each move.
// P and Q must add up to 100.
constexpr int P = 90;
constexpr int Q = 10;

// Seconds of inactivity before the hard mode timer fires an automatic move.
constexpr int HARD_MODE_TIMEOUT_SEC = 5;
