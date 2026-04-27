#ifndef UTILS_H
#define UTILS_H

#include <utility>
#include <cstdlib>
#include <ctime>
#include <iostream>
using namespace std;

// 方向数组
const int dx[4] = {1, -1, 0, 0};
const int dy[4] = {0, 0, 1, -1};

// 坐标是否合法
inline bool valid(int x, int y) {
    return x >= 0 && x < 9 && y >= 0 && y < 9;
}

// 坐标转 index
inline int posToIndex(int x, int y) {
    return x * 9 + y;
}

// index 转坐标
inline pair<int,int> indexToPos(int idx) {
    return {idx / 9, idx % 9};
}

// 随机数
inline int randInt(int l, int r) {
    return l + rand() % (r - l + 1);
}

// 计时器
class Timer {
public:
    clock_t start;

    Timer() {
        start = clock();
    }

    bool isTimeUp(double limit = 0.95) {
        return (double)(clock() - start) / CLOCKS_PER_SEC > limit;
    }
};

// 打印棋盘（调试）
inline void printBoard(int board[9][9]) {
    for(int i = 0; i < 9; i++) {
        for(int j = 0; j < 9; j++) {
            cout << board[i][j] << " ";
        }
        cout << endl;
    }
    cout << "--------" << endl;
}

#endif