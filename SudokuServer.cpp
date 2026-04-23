#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#include <chrono>
#include <cstring>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <ctime>
#include "SudokuCommon.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ================= SAFE UTIL =================
void safeStringCopy(char* dest, const char* src, size_t size) {
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

// ================= SOCKET SAFE SEND/RECV =================
bool sendAll(SOCKET sock, const char* data, int totalSize) {
    int sent = 0;
    while (sent < totalSize) {
        int n = send(sock, data + sent, totalSize - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

bool recvAll(SOCKET sock, char* buffer, int totalSize) {
    int received = 0;
    while (received < totalSize) {
        int n = recv(sock, buffer + received, totalSize - received, 0);
        if (n <= 0) return false;
        received += n;
    }
    return true;
}

// ================= GAME ROOM =================
struct GameRoom {
    int roomId;
    char roomName[50];
    int difficulty;

    SOCKET clients[MAX_CLIENTS];
    PlayerInfo players[MAX_CLIENTS];
    int playerCount;

    int puzzle[N][N];
    int solution[N][N];
    int locked[N][N];

    bool gameActive;
    CRITICAL_SECTION roomMutex;
};

// ================= GLOBALS =================
map<int, GameRoom> rooms;
CRITICAL_SECTION roomsMutex;

int nextRoomId = 1;
int nextPlayerId = 1;

// ================= LOG =================
void logMessage(const string& msg) {
    cout << "[SERVER] " << msg << endl;
}

// ================= SUDOKU GENERATION =================
void fillBasePattern(int sol[N][N]) {
    // Valid Sudoku base pattern
    int base[9][9] = {
        {1,2,3,4,5,6,7,8,9},
        {4,5,6,7,8,9,1,2,3},
        {7,8,9,1,2,3,4,5,6},
        {2,3,4,5,6,7,8,9,1},
        {5,6,7,8,9,1,2,3,4},
        {8,9,1,2,3,4,5,6,7},
        {3,4,5,6,7,8,9,1,2},
        {6,7,8,9,1,2,3,4,5},
        {9,1,2,3,4,5,6,7,8}
    };
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            sol[i][j] = base[i][j];
}

void swapRows(int sol[N][N], int r1, int r2) {
    for (int c = 0; c < N; c++)
        swap(sol[r1][c], sol[r2][c]);
}

void shuffleRows(int sol[N][N]) {
    // Shuffle within each 3x3 row block to maintain validity
    for (int block = 0; block < 3; block++) {
        int start = block * 3;
        for (int i = 0; i < 20; i++) {
            int r1 = start + (rand() % 3);
            int r2 = start + (rand() % 3);
            swapRows(sol, r1, r2);
        }
    }
}

void swapCols(int sol[N][N], int c1, int c2) {
    for (int r = 0; r < N; r++)
        swap(sol[r][c1], sol[r][c2]);
}

void shuffleCols(int sol[N][N]) {
    // Shuffle within each 3x3 column block
    for (int block = 0; block < 3; block++) {
        int start = block * 3;
        for (int i = 0; i < 20; i++) {
            int c1 = start + (rand() % 3);
            int c2 = start + (rand() % 3);
            swapCols(sol, c1, c2);
        }
    }
}

void generateSolution(int sol[N][N]) {
    fillBasePattern(sol);
    shuffleRows(sol);
    shuffleCols(sol);
}

void generatePuzzle(int sol[N][N], int puzzle[N][N], int locked[N][N]) {
    generateSolution(sol);

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            puzzle[i][j] = sol[i][j];
            locked[i][j] = sol[i][j];
        }

    int remove = 45;
    while (remove > 0) {
        int i = rand() % N;
        int j = rand() % N;
        if (puzzle[i][j] != 0) {
            puzzle[i][j] = 0;
            locked[i][j] = 0;
            remove--;
        }
    }
}

// ================= VALIDATION =================
bool isValidMove(int grid[N][N], int locked[N][N], int r, int c, int num) {
    if (locked[r][c] != 0) return false;

    for (int i = 0; i < N; i++) {
        if (grid[r][i] == num || grid[i][c] == num)
            return false;
    }

    int br = (r / 3) * 3;
    int bc = (c / 3) * 3;

    for (int i = br; i < br + 3; i++)
        for (int j = bc; j < bc + 3; j++)
            if (grid[i][j] == num)
                return false;

    return true;
}

bool isSolved(int p[N][N], int s[N][N]) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (p[i][j] != s[i][j]) return false;

    return true;
}

// ================= NETWORK =================
void sendMessage(SOCKET sock, NetworkMessage& msg) {
    sendAll(sock, (char*)&msg, sizeof(msg));
}

// ================= ROOM =================
int createRoom(const char* name) {
    EnterCriticalSection(&roomsMutex);

    int id = nextRoomId++;
    GameRoom& room = rooms[id];

    room.roomId = id;
    safeStringCopy(room.roomName, name, sizeof(room.roomName));
    room.playerCount = 0;

    InitializeCriticalSection(&room.roomMutex);
    generatePuzzle(room.solution, room.puzzle, room.locked);

    logMessage("Room created: " + to_string(id));
    LeaveCriticalSection(&roomsMutex);
    return id;
}

// ================= JOIN =================
int handleJoin(SOCKET sock, NetworkMessage& msg) {
    EnterCriticalSection(&roomsMutex);

    if (rooms.find(msg.roomId) == rooms.end())
        return -1;

    GameRoom& room = rooms[msg.roomId];

    int pid = nextPlayerId++;
    int idx = room.playerCount++;

    room.clients[idx] = sock;
    room.players[idx].playerId = pid;

    NetworkMessage res{};
    res.type = MSG_ROOM_JOINED;
    res.playerId = pid;
    res.roomId = room.roomId;

    for (int i = 0; i < N * N; i++) {
        res.puzzleData[i] = ((int*)room.puzzle)[i];
        res.solutionData[i] = ((int*)room.solution)[i];
        res.lockedData[i] = ((int*)room.locked)[i];
    }

    sendMessage(sock, res);
    LeaveCriticalSection(&roomsMutex);
    return pid;
}

// ================= MOVE =================
void handleMove(SOCKET sock, int roomId, int pid, int r, int c, int num) {
    EnterCriticalSection(&roomsMutex);

    GameRoom& room = rooms[roomId];

    NetworkMessage res{};
    res.type = MSG_MOVE_RESPONSE;
    res.playerId = pid;
    res.roomId = roomId;

    if (!isValidMove(room.puzzle, room.locked, r, c, num)) {
        res.data[0] = 0; 
        sendMessage(sock, res);
        LeaveCriticalSection(&roomsMutex);
        return;
    }

    room.puzzle[r][c] = num;

    res.data[0] = 1;
    res.data[2] = r;
    res.data[3] = c;
    res.data[4] = num;

    for (int i = 0; i < room.playerCount; i++)
        sendMessage(room.clients[i], res);

    if (isSolved(room.puzzle, room.solution)) {
        NetworkMessage done{};
        done.type = MSG_GAME_COMPLETE;
        done.playerId = pid;

        for (int i = 0; i < room.playerCount; i++)
            sendMessage(room.clients[i], done);
    }
    LeaveCriticalSection(&roomsMutex);
}

// ================= CLIENT THREAD =================
void clientThread(SOCKET sock);
DWORD WINAPI clientThreadFunc(LPVOID param) {
    SOCKET client = (SOCKET)param;
    clientThread(client);
    return 0;
}

void clientThread(SOCKET sock) {
    NetworkMessage msg{};
    int roomId = -1;
    int playerId = -1;

    while (true) {
        if (!recvAll(sock, (char*)&msg, sizeof(msg)))
            break;

        switch (msg.type) {
            case MSG_CREATE_ROOM:
                roomId = createRoom(msg.buffer);
                msg.roomId = roomId;
                playerId = handleJoin(sock, msg);
                break;

            case MSG_JOIN_ROOM:
                roomId = msg.roomId;
                playerId = handleJoin(sock, msg);
                break;

            case MSG_MOVE:
                handleMove(sock, roomId, playerId,
                    msg.data[0], msg.data[1], msg.data[2]);
                break;
        }
    }

    closesocket(sock);
}

// ================= SERVER =================
void startServer() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (sockaddr*)&addr, sizeof(addr));
    listen(server, SOMAXCONN);

    logMessage("Server started...");

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        HANDLE h = CreateThread(NULL, 0, clientThreadFunc, (LPVOID)client, 0, NULL);
        CloseHandle(h);
    }
}

// ================= MAIN =================
int main() {
    srand((unsigned)time(0)); // ✅ FIXED
    InitializeCriticalSection(&roomsMutex);

    cout << "=== Sudoku Server ===\n";
    startServer();
    return 0;
}