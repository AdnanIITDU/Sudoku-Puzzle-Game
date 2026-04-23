#include <bits/stdc++.h>
#define N 9

using namespace std;
using namespace std::chrono;

class Stopwatch
{
private:
    steady_clock::time_point start_time;
    steady_clock::time_point stop_time;
    bool running = false;

public:
    void start()
    {
        start_time = steady_clock::now();
        running = true;
        cout << "The game has begun" << endl;
    }

    void stop()
    {
        if (running)
        {
            stop_time = steady_clock::now();
            running = false;
            auto duration = duration_cast<seconds>(stop_time - start_time);
            cout << "Total time spent: " << duration.count() << " seconds" << endl;
        }
    }

    void elapsed()
    {
        if (running)
        {
            auto current_time = steady_clock::now();
            auto duration = duration_cast<seconds>(current_time - start_time);
            cout << "Elapsed time: " << duration.count() << " seconds" << endl;
        }
    }
};

int L;
int puzzle[N][N], locked[N][N], solution[N][N];

// generating sudoku

void fillBasePattern()
{
    int base = 3;
    for (int row = 0; row < N; row++)
        for (int col = 0; col < N; col++)
            solution[row][col] = (row * base + row / base + col) % N + 1;
}

void swapRows(int r1, int r2)
{
    for (int c = 0; c < N; c++)
        swap(solution[r1][c], solution[r2][c]);
}

void swapCols(int c1, int c2)
{
    for (int r = 0; r < N; r++)
        swap(solution[r][c1], solution[r][c2]);
}

void shuffleRows()
{
    int base = 3;
    for (int block = 0; block < base; block++)
    {
        int start = block * base;
        for (int i = 0; i < L; i++)
            swapRows(start + rand() % base, start + rand() % base);
    }
}

void shuffleCols()
{
    int base = 3;
    for (int block = 0; block < base; block++)
    {
        int start = block * base;
        for (int i = 0; i < L; i++)
            swapCols(start + rand() % base, start + rand() % base);
    }
}

void shuffleRowBlocks()
{
    int base = 3;
    for (int i = 0; i < L; i++)
    {
        int b1 = rand() % base;
        int b2 = rand() % base;
        for (int r = 0; r < base; r++)
            swapRows(b1 * base + r, b2 * base + r);
    }
}

void shuffleColBlocks()
{
    int base = 3;
    for (int i = 0; i < L; i++)
    {
        int b1 = rand() % base;
        int b2 = rand() % base;
        for (int c = 0; c < base; c++)
            swapCols(b1 * base + c, b2 * base + c);
    }
}

void makeSolutionSudoku()
{
    fillBasePattern();
    shuffleRows();
    shuffleCols();
    shuffleRowBlocks();
    shuffleColBlocks();
}

void copySolutionToPuzzle()
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            locked[i][j] = puzzle[i][j] = solution[i][j];
}

void printPuzzle()
{
    cout << " -----------------------\n";
    for (int i = 0; i < 9; i++)
    {
        cout << "| ";
        for (int j = 0; j < 9; j++)
        {
            cout << puzzle[i][j] << " ";
            if ((j + 1) % 3 == 0)
                cout << "| ";
        }
        cout << endl;
        if ((i + 1) % 3 == 0)
            cout << " -----------------------\n";
    }
}

void printSolution()
{
    cout << " -----------------------\n";
    for (int i = 0; i < N; i++)
    {
        cout << "| ";
        for (int j = 0; j < N; j++)
        {
            cout << solution[i][j] << " ";
            if ((j + 1) % 3 == 0)
                cout << "| ";
        }
        cout << endl;
        if ((i + 1) % 3 == 0)
            cout << " -----------------------\n";
    }
}

// validation

bool checkRow(int row, int col, int num)
{
    for (int i = 0; i < 9; i++)
        if (puzzle[row][i] == num)
            return false;
    return true;
}

bool checkCol(int row, int col, int num)
{
    for (int i = 0; i < 9; i++)
        if (puzzle[i][col] == num)
            return false;
    return true;
}

bool checkSquare(int row, int col, int num)
{
    row = (row / 3) * 3;
    col = (col / 3) * 3;
    for (int i = row; i < row + 3; i++)
        for (int j = col; j < col + 3; j++)
            if (puzzle[i][j] == num)
                return false;
    return true;
}

bool isValid(int row, int col, int num)
{
    return checkRow(row, col, num) &&
           checkCol(row, col, num) &&
           checkSquare(row, col, num);
}

// backtracking

bool isSafe(int grid[N][N], int row, int col, int num)
{
    for (int x = 0; x < 9; x++)
        if (grid[row][x] == num || grid[x][col] == num)
            return false;

    int sr = (row / 3) * 3;
    int sc = (col / 3) * 3;

    for (int i = sr; i < sr + 3; i++)
        for (int j = sc; j < sc + 3; j++)
            if (grid[i][j] == num)
                return false;

    return true;
}

bool solveSudoku(int grid[N][N])
{
    for (int row = 0; row < N; row++)
    {
        for (int col = 0; col < N; col++)
        {
            if (grid[row][col] == 0)
            {
                for (int num = 1; num <= 9; num++)
                {
                    if (isSafe(grid, row, col, num))
                    {
                        grid[row][col] = num;
                        if (solveSudoku(grid))
                            return true;
                        grid[row][col] = 0;
                    }
                }
                return false;
            }
        }
    }
    return true;
}

// generating puzzle

void generatePuzzle(int difficulty)
{
    int remove;
    if (difficulty == 1)
    {
        remove = 41;
        L = 3;
    }
    else if (difficulty == 2)
    {
        remove = 46;
        L = 5;
    }
    else if (difficulty == 3)
    {
        remove = 51;
        L = 7;
    }
    else
    {
        remove = 61;
        L = 10;
    }

    makeSolutionSudoku();
    copySolutionToPuzzle();

    srand(time(0));
    while (remove > 0)
    {
        int i = rand() % N;
        int j = rand() % N;
        if (puzzle[i][j] != 0)
        {
            locked[i][j] = puzzle[i][j] = 0;
            remove--;
        }
    }
}

void playSudoku()
{
    Stopwatch stopwatch;
    stopwatch.start();

    int move = 70;
    while (move--)
    {
        stopwatch.elapsed();
        printPuzzle();

        cout << "Enter row col num (1-9) | -1 to auto-solve | 0 to quit\n";

        int row, col, num;
        cin >> row;

        if (row == -1)
        {
            solveSudoku(puzzle);
            printPuzzle();
            break;
        }

        if (row == 0)
        {
            cout << "Auto-solving...\n";
            solveSudoku(puzzle);
            printPuzzle();
            break;
        }

        cin >> col >> num;
        row--;
        col--;

        if (locked[row][col] != 0 || !isValid(row, col, num))
        {
            cout << "Invalid move\n";
            move++;
            continue;
        }

        puzzle[row][col] = num;

        if (memcmp(puzzle, solution, sizeof(puzzle)) == 0)
        {
            cout << "Congratulations! You solved it!\n";
            break;
        }
    }
    stopwatch.stop();
}

int main()
{
    cout << "Choose Difficulty:\n1.Easy\n2.Medium\n3.Hard\n4.Extreme\n";
    int d;
    cin >> d;

    generatePuzzle(d);
    playSudoku();

    cout << "Final Solution:\n";
    printSolution();
    return 0;
}
