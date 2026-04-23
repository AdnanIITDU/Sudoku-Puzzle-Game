#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include <queue>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "SudokuCommon.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace std::chrono;

/* ================= STOPWATCH ================= */
class Stopwatch
{
public:
    steady_clock::time_point start_time;
    steady_clock::time_point stop_time;
    bool running = false;

    void start()
    {
        start_time = steady_clock::now();
        running = true;
        cout << "The game has begun\n";
    }

    void stop()
    {
        if (running)
        {
            stop_time = steady_clock::now();
            running = false;
            auto duration = duration_cast<seconds>(stop_time - start_time);
            cout << "Total time: " << duration.count() << " sec\n";
        }
    }

    void elapsed()
    {
        if (running)
        {
            auto now = steady_clock::now();
            cout << "Elapsed: "
                 << duration_cast<seconds>(now - start_time).count()
                 << " sec\n";
        }
    }
};

/* ================= GLOBAL ================= */
SOCKET serverSocket;
int myPlayerId = -1;
int currentRoomId = -1;
bool gameActive = false;
bool gameSolved = false;

int puzzle[N][N], solution[N][N], locked[N][N];

Stopwatch stopwatch;

queue<NetworkMessage> messageQueue;
mutex msgQueueMutex;

/* ================= HANDLERS ================= */
void handleRoomJoined(NetworkMessage &msg);
void handleMoveResponse(NetworkMessage &msg);
void handleGameComplete(NetworkMessage &msg);

/* ================= UI ================= */
void printPuzzle();

/* ================= NETWORK ================= */
void sendMessage(NetworkMessage &msg)
{
    send(serverSocket, (char *)&msg, sizeof(msg), 0);
}

void receiveMessages()
{
    NetworkMessage msg;

    while (true)
    {
        int bytes = recv(serverSocket, (char *)&msg, sizeof(msg), 0);
        if (bytes <= 0)
        {
            cout << "Server disconnected\n";
            break;
        }

        lock_guard<mutex> lock(msgQueueMutex);
        messageQueue.push(msg);
    }
}

/* ================= HANDLERS ================= */
void handleRoomJoined(NetworkMessage &msg)
{
    currentRoomId = msg.roomId;
    myPlayerId = msg.playerId;

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
        {
            puzzle[i][j] = msg.puzzleData[i * N + j];
            solution[i][j] = msg.solutionData[i * N + j];
            locked[i][j] = msg.lockedData[i * N + j];
        }

    cout << "Joined room successfully!\n";
    gameActive = true;
    stopwatch.start();
}

void handleMoveResponse(NetworkMessage &msg)
{
    if (msg.data[0]) // valid
    {
        int r = msg.data[2];
        int c = msg.data[3];
        int n = msg.data[4];

        puzzle[r][c] = n;
        cout << "Move OK\n";
        printPuzzle(); // Print updated board
    }
    else
    {
        cout << "Invalid move\n";
    }
}

void handleGameComplete(NetworkMessage &msg)
{
    cout << "\nPlayer " << msg.playerId << " solved the puzzle!\n";

    if (msg.playerId == myPlayerId)
    {
        gameSolved = true;
        gameActive = false;
    }
}

/* ================= UI ================= */
void printPuzzle()
{
    cout << " -----------------------\n";
    for (int i = 0; i < 9; i++)
    {
        cout << "| ";
        for (int j = 0; j < 9; j++)
        {
            cout << puzzle[i][j] << " ";
            if ((j + 1) % 3 == 0) cout << "| ";
        }
        cout << endl;
        if ((i + 1) % 3 == 0)
            cout << " -----------------------\n";
    }
}

/* ================= MESSAGE PROCESSOR ================= */
void processMessages()
{
    while (true)
    {
        lock_guard<mutex> lock(msgQueueMutex);

        while (!messageQueue.empty())
        {
            NetworkMessage msg = messageQueue.front();
            messageQueue.pop();

            switch (msg.type)
            {
            case MSG_ROOM_JOINED:
                handleRoomJoined(msg);
                break;

            case MSG_MOVE_RESPONSE:
                handleMoveResponse(msg);
                break;

            case MSG_GAME_COMPLETE:
                handleGameComplete(msg);
                break;
            }
        }

        this_thread::sleep_for(chrono::milliseconds(50));
    }
}


/* ================= GAME ================= */
void playSudoku()
{
    cout << "\n=== GAME START ===\n";

    while (gameActive && !gameSolved)
    {
        stopwatch.elapsed();
        printPuzzle();

        cout << "Enter row col num (0 to exit): ";

        int r;
        cin >> r;

        if (r == 0)
        {
            gameActive = false;
            break;
        }

        int c, n;
        cin >> c >> n;
        r--; c--;

        if (locked[r][c])
        {
            cout << "Locked cell!\n";
            continue;
        }

        NetworkMessage msg;
        msg.type = MSG_MOVE;
        msg.playerId = myPlayerId;
        msg.roomId = currentRoomId;
        msg.data[0] = r;
        msg.data[1] = c;
        msg.data[2] = n;

        sendMessage(msg);
    }

    stopwatch.stop();
}

/* ================= CONNECTION ================= */
bool connectToServer(string ip)
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(serverSocket, (sockaddr *)&server, sizeof(server)) < 0)
    {
        cout << "Connection failed\n";
        return false;
    }

    cout << "Connected to server\n";
    return true;
}

/* ================= ROOM ================= */
void createRoom(const string &roomName)
{
    NetworkMessage msg;
    msg.type = MSG_CREATE_ROOM;
    strncpy(msg.buffer, roomName.c_str(), sizeof(msg.buffer) - 1);
    msg.buffer[sizeof(msg.buffer) - 1] = '\0';
    sendMessage(msg);
}

void joinRoom(int roomId)
{
    NetworkMessage msg;
    msg.type = MSG_JOIN_ROOM;
    msg.roomId = roomId;
    sendMessage(msg);
}

/* ================= MAIN ================= */
int main()
{
    cout << "Enter server IP: ";
    string ip;
    getline(cin, ip);
    if (ip.empty()) ip = "127.0.0.1";

    if (!connectToServer(ip)) return 0;

    // start threads
    thread t1(receiveMessages);
    thread t2(processMessages);

    t1.detach();
    t2.detach();

    // register
    NetworkMessage reg;
    reg.type = MSG_REGISTER;
    sendMessage(reg);

    cout << "\n=== ROOM MENU ===\n";
    cout << "1. Create new room\n";
    cout << "2. Join existing room\n";
    cout << "Enter choice: ";
    int choice = 0;
    cin >> choice;
    cin.ignore(1000, '\n');

    if (choice == 1)
    {
        cout << "Enter room name: ";
        string roomName;
        getline(cin, roomName);
        if (roomName.empty()) roomName = "Default Room";
        createRoom(roomName);
    }
    else if (choice == 2)
    {
        cout << "Enter room ID: ";
        int roomId;
        cin >> roomId;
        cin.ignore(1000, '\n');
        joinRoom(roomId);
    }
    else
    {
        cout << "Invalid choice. Exiting.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    cout << "Waiting for server...\n";

    // IMPORTANT FIX
    while (!gameActive)
        this_thread::sleep_for(chrono::milliseconds(100));

    playSudoku();

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}