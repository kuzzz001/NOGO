#ifndef RULE_H
#define RULE_H

#include "board.h"
#include <vector>
#include <utility>
using namespace std;

class Rule {
public:
    // 判断一个位置是否在棋盘内
    static bool inBoard(int x, int y);

    // 计算某个连通块的气
    static int countLiberties(Board &b, int x, int y);

    // 判断是否自杀
    static bool isSuicide(Board &b, int x, int y, int color);

    // 判断是否会吃子（NoGo中非法）
    static bool willCapture(Board &b, int x, int y, int color);

    // 判断是否合法落子
    static bool isLegal(Board &b, int x, int y, int color);

    // 获取所有合法落子
    static vector<pair<int,int>> getLegalMoves(Board &b, int color);

private:
    static bool visited[BOARD_SIZE][BOARD_SIZE];

    static void resetVisited();
    static void dfs(Board &b, int x, int y, int color, int &liberties);
};

#endif