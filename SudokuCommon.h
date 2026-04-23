#ifndef SUDOKU_COMMON_H
#define SUDOKU_COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

#define N 9
#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define MAX_ROOMS 5

// Message types for network communication
enum MessageType {
    // Authentication & Room Management
    MSG_REGISTER = 1,
    MSG_REGISTER_ACK = 2,
    MSG_CREATE_ROOM = 3,
    MSG_JOIN_ROOM = 4,
    MSG_ROOM_CREATED = 5,
    MSG_ROOM_JOINED = 6,
    MSG_ROOM_FULL = 7,
    MSG_PLAYER_JOINED = 8,
    
    // Game State
    MSG_START_GAME = 9,
    MSG_GAME_STARTED = 10,
    MSG_PUZZLE = 11,
    MSG_SOLUTION = 12,
    MSG_GAME_STATE = 13,
    
    // Game Play
    MSG_MOVE = 14,
    MSG_MOVE_RESPONSE = 15,
    MSG_PLAYER_MOVE = 16,
    MSG_GAME_COMPLETE = 17,
    MSG_GAME_ENDED = 18,
    
    // Special Actions
    MSG_DEBUG = 19,
    MSG_LIFELINE = 20,
    MSG_CHAT = 21,
    
    // Utility
    MSG_HEARTBEAT = 22,
    MSG_ERROR = 23,
    MSG_DISCONNECT = 24
};

// Player information
struct PlayerInfo {
    int playerId;
    char playerName[50];
    int score;
    int difficulty;
    int moves;
    int mistakes;
    int rewrites;
    int debugCount;
    bool lifelineUsed;
    int elapsedTime;
};

// Game Room information
struct RoomInfo {
    int roomId;
    char roomName[50];
    int difficulty;
    int maxPlayers;
    int currentPlayers;
    int puzzle[N][N];
    int solution[N][N];
    int locked[N][N];
    bool gameActive;
    int gameStartTime;
};

// Network message structure
struct NetworkMessage {
    MessageType type;
    int playerId;
    int roomId;
    char playerName[50];
    int data[11]; // For various data: row, col, num, score, etc.
    char buffer[1024];
    int difficulty;
    int puzzleData[N * N];
    int solutionData[N * N];
    int lockedData[N * N];
};

// Move validation response
struct MoveResponse {
    bool valid;
    int score;
    int newMistakes;
    int newRewrites;
};

#endif // SUDOKU_COMMON_H
