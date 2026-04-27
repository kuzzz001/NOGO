#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <utility>
using namespace std;

const int BOARD_SIZE = 9;

class Board {
public:
    int board[BOARD_SIZE][BOARD_SIZE];

    Board();

    // 基本操作
    bool inBoard(int x, int y);
    void clear();
    void place(int x, int y, int color);   // 落子（不做合法性检查）
    void undo(int x, int y);               // 撤销

    // 核心规则
    int countLiberties(int x, int y);
    bool isSuicide(int x, int y, int color);
    bool willCapture(int x, int y, int color);
    bool isLegal(int x, int y, int color);

    // 获取所有合法落子
    vector<pair<int,int>> getLegalMoves(int color);

private:
    bool visited[BOARD_SIZE][BOARD_SIZE];

    void resetVisited();
    void dfs(int x, int y, int color, int &liberties);
};

#endif