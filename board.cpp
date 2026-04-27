#include "board.h"
#include <cstring>

Board::Board() {
    clear();
}

void Board::clear() {
    memset(board, 0, sizeof(board));
}

bool Board::inBoard(int x, int y) {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

void Board::place(int x, int y, int color) {
    board[x][y] = color;
}

void Board::undo(int x, int y) {
    board[x][y] = 0;
}

void Board::resetVisited() {
    memset(visited, false, sizeof(visited));
}

// DFS 计算连通块的气
void Board::dfs(int x, int y, int color, int &liberties) {
    visited[x][y] = true;

    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};

    for(int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if(!inBoard(nx, ny)) continue;

        if(board[nx][ny] == 0) {
            liberties++;
        } else if(board[nx][ny] == color && !visited[nx][ny]) {
            dfs(nx, ny, color, liberties);
        }
    }
}

// 计算某个点所在连通块的气
int Board::countLiberties(int x, int y) {
    resetVisited();
    int liberties = 0;
    dfs(x, y, board[x][y], liberties);
    return liberties;
}

// 是否会吃子（NoGo中这是非法）
bool Board::willCapture(int x, int y, int color) {
    int opponent = -color;

    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};

    place(x, y, color);

    for(int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if(inBoard(nx, ny) && board[nx][ny] == opponent) {
            if(countLiberties(nx, ny) == 0) {
                undo(x, y);
                return true;
            }
        }
    }

    undo(x, y);
    return false;
}

// 是否自杀
bool Board::isSuicide(int x, int y, int color) {
    place(x, y, color);

    bool result = (countLiberties(x, y) == 0);

    undo(x, y);
    return result;
}

// 合法性判断（核心）
bool Board::isLegal(int x, int y, int color) {
    if(!inBoard(x, y) || board[x][y] != 0)
        return false;

    if(willCapture(x, y, color))   // 吃子直接判负
        return false;

    if(isSuicide(x, y, color))     // 自杀判负
        return false;

    return true;
}

// 获取所有合法点
vector<pair<int,int>> Board::getLegalMoves(int color) {
    vector<pair<int,int>> moves;

    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            if(isLegal(i, j, color)) {
                moves.push_back({i, j});
            }
        }
    }

    return moves;
}