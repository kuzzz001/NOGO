#include "rule.h"
#include <cstring>

bool Rule::visited[BOARD_SIZE][BOARD_SIZE];

// 辅助函数
void Rule::resetVisited() {
    memset(visited, false, sizeof(visited));
}

bool Rule::inBoard(int x, int y) {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

// DFS 计算气
void Rule::dfs(Board &b, int x, int y, int color, int &liberties) {
    visited[x][y] = true;

    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};

    for(int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if(!inBoard(nx, ny)) continue;

        if(b.board[nx][ny] == 0) {
            liberties++;
        } else if(b.board[nx][ny] == color && !visited[nx][ny]) {
            dfs(b, nx, ny, color, liberties);
        }
    }
}

// 计算气
int Rule::countLiberties(Board &b, int x, int y) {
    resetVisited();
    int liberties = 0;
    dfs(b, x, y, b.board[x][y], liberties);
    return liberties;
}

// 判断是否吃子（NoGo中非法）
bool Rule::willCapture(Board &b, int x, int y, int color) {
    int opponent = -color;

    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};

    b.place(x, y, color);

    for(int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if(inBoard(nx, ny) && b.board[nx][ny] == opponent) {
            if(countLiberties(b, nx, ny) == 0) {
                b.undo(x, y);
                return true;
            }
        }
    }

    b.undo(x, y);
    return false;
}

// 判断是否自杀
bool Rule::isSuicide(Board &b, int x, int y, int color) {
    b.place(x, y, color);

    bool result = (countLiberties(b, x, y) == 0);

    b.undo(x, y);
    return result;
}

// 判断是否合法
bool Rule::isLegal(Board &b, int x, int y, int color) {
    if(!inBoard(x, y) || b.board[x][y] != 0)
        return false;

    if(willCapture(b, x, y, color))
        return false;

    if(isSuicide(b, x, y, color))
        return false;

    return true;
}

// 获取所有合法点
vector<pair<int,int>> Rule::getLegalMoves(Board &b, int color) {
    vector<pair<int,int>> moves;

    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            if(isLegal(b, i, j, color)) {
                moves.push_back({i, j});
            }
        }
    }

    return moves;
}