// ============================================================
// NoGo (不围棋) 博弈算法 —— MCTS + Minimax 混合策略 v2
// 基于 second.cpp 优化：
//   优化1: judge_value() 加入 right_value 权值评估
//   优化2: Minimax 穷举按权利值排序（找到必胜解即退出 → 等效剪枝）
// ============================================================
// 不围棋规则：与围棋类似，但禁止吃子（不能下在使对方无气的位置）。
// 先手黑棋（1），后手白棋（-1）。无合法落子点的一方判负。
// ============================================================

#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <ctime>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>
#include "jsoncpp/json.h"

using namespace std;

// ====================== 全局变量 ======================

int x, y;
int depth;
int chessboard[9][9] = { 0 };
int black = 1, white = -1;
int player = 1, player0;
int dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };
int start, timeout = (int)(0.95 * (double)CLOCKS_PER_SEC);

// ====================== MCTS 节点结构 ======================

#define NODE_MAX 1500000

struct MCTSNode {
    int x, y;
    bool Expanded;
    int NowChildren;
    int Children[81];
    double Value;
    double Times;
} Node[NODE_MAX] = {};

int now = 0;
int now_expanded = 0;

// ====================== 可用落子点管理 ======================

int len[2] = { 0 }, avail[2][81] = { 0 };
int len_store[2] = { 0 }, avail_store[2][81] = { 0 };

inline int trans(int color) { return (color + 1) / 2; }
inline int retrans(int number) { return number * 2 - 1; }

// ====================== 不围棋规则引擎 ======================

bool air_judge(int x, int y, int player) {
    if (x < 0 || x > 8 || y < 0 || y > 8) return 0;
    if (chessboard[x][y] == -player) return 0;
    if (chessboard[x][y] == 0) return 1;

    chessboard[x][y] = -player;
    bool res = air_judge(x + 1, y, player)
            || air_judge(x - 1, y, player)
            || air_judge(x, y + 1, player)
            || air_judge(x, y - 1, player);
    chessboard[x][y] = player;
    return res;
}

bool put_judge(int x, int y, int player) {
    if (x < 0 || y < 0 || x > 8 || y > 8 || chessboard[x][y] != 0) return 0;

    chessboard[x][y] = player;

    bool legal = true;

    if (!air_judge(x, y, player)) legal = false;
    else if ((x + 1 <= 8 && chessboard[x + 1][y] == -player && !air_judge(x + 1, y, -player))
          || (x - 1 >= 0 && chessboard[x - 1][y] == -player && !air_judge(x - 1, y, -player))
          || (y + 1 <= 8 && chessboard[x][y + 1] == -player && !air_judge(x, y + 1, -player))
          || (y - 1 >= 0 && chessboard[x][y - 1] == -player && !air_judge(x, y - 1, -player)))
        legal = false;

    chessboard[x][y] = 0;
    return legal;
}

bool end_judge(int player) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (put_judge(i, j, player)) return 0;
    return 1;
}

// ====================== 评估函数（优化1: 权值评估） ======================

bool eye_judge(int x, int y, int player) {
    if (chessboard[x][y]) return 0;
    return ((x + 1 > 8 || chessboard[x + 1][y] == player)
         && (x - 1 < 0 || chessboard[x - 1][y] == player)
         && (y + 1 > 8 || chessboard[x][y + 1] == player)
         && (y - 1 < 0 || chessboard[x][y - 1] == player));
}

int forbidden_count(int color) {
    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (chessboard[i][j] == 0 && !put_judge(i, j, color)) cnt++;
    return cnt;
}

int right_value(int color) {
    return forbidden_count(-color);
}

double judge_value(int player) {
    double p = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (chessboard[i][j] != 0) continue;

            if (!put_judge(i, j, player)) {
                p -= 1;
                if (eye_judge(i, j, player)) p -= 0.2;
            } else if (eye_judge(i, j, player)) {
                p += 0.2;
            }

            if (!put_judge(i, j, -player)) p += 1;
        }
    return 1.0 / (1.0 + exp(-p));
}

// ====================== 终局穷举（优化2: 权利值排序） ======================

int enumerate(int k) {
    pair<int,int> scored_moves[81];
    int moveCount = 0;

    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (!put_judge(i, j, player)) continue;

            chessboard[i][j] = player;
            int rv = right_value(player);
            chessboard[i][j] = 0;

            scored_moves[moveCount++] = { -rv, i * 9 + j };
        }

    sort(scored_moves, scored_moves + moveCount);

    for (int idx = 0; idx < moveCount; idx++) {
        int move = scored_moves[idx].second;
        int i = move / 9;
        int j = move % 9;

        chessboard[i][j] = player;
        player = -player;

        int win = enumerate(k + 1);

        chessboard[i][j] = 0;
        player = -player;

        if (!win) {
            if (k == 1) { x = i; y = j; }
            return 1;
        }
    }
    return 0;
}

// ====================== MCTS 选择与扩展 ======================

int selection() {
    int selec = 0;
    int number = trans(player);

    if (!Node[now].Expanded) {
        int tmplen = len[number];
        int tmpavail[81];
        memcpy(tmpavail, avail[number], sizeof(tmpavail));

        int* sign;
        for (int i = 1; i <= Node[now].NowChildren; i++) {
            int c = Node[Node[now].Children[i]].x * 9 + Node[Node[now].Children[i]].y;
            sign = find(tmpavail, tmpavail + tmplen, c);
            if (sign != tmpavail + tmplen) *sign = tmpavail[--tmplen];
        }

        while (true) {
            if (tmplen == 0) {
                if (Node[now].NowChildren == 0) return -1;
                Node[now].Expanded = 1;
                selec = -1;
                break;
            }
            int r = rand() % tmplen;
            int xx = tmpavail[r] / 9, yy = tmpavail[r] % 9;
            if (put_judge(xx, yy, player)) {
                selec = tmpavail[r];
                sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number]) *sign = avail[number][--len[number]];
                break;
            } else {
                sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number]) *sign = avail[number][--len[number]];
                tmpavail[r] = tmpavail[--tmplen];
            }
        }

        if (selec != -1) {
            Node[now].NowChildren++;
            Node[now].Children[Node[now].NowChildren] = ++now_expanded;
            Node[now_expanded].x = selec / 9;
            Node[now_expanded].y = selec % 9;
            if (tmplen == 1) Node[now].Expanded = 1;
            now = now_expanded;
            return 1;
        }
    }

    double max_ucb = -1;
    int best = 0;
    for (int i = 1; i <= Node[now].NowChildren; i++) {
        int c = Node[now].Children[i];
        double ucb = Node[c].Value / Node[c].Times
                   + sqrt(2) * sqrt(log(Node[now].Times) / Node[c].Times);
        if (ucb > max_ucb) {
            max_ucb = ucb;
            best = c;
        }
    }
    now = best;
    return 0;
}

// ====================== MCTS 主循环 ======================

void MCTS() {
    memset(Node, 0, sizeof(Node));
    now_expanded = 0;
    Node[0].x = -1;
    Node[0].y = -1;

    int search_path[100];

    len_store[1] = len_store[0] = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, black)) avail_store[1][len_store[1]++] = i * 9 + j;
            if (put_judge(i, j, white)) avail_store[0][len_store[0]++] = i * 9 + j;
        }

    while (clock() - start < timeout) {
        int i = 0;
        now = 0;
        search_path[0] = 0;

        memcpy(avail, avail_store, sizeof(avail));
        memcpy(len, len_store, sizeof(len));

        while (!end_judge(player) && i <= depth) {
            int unexpand = selection();
            if (unexpand == -1) break;
            search_path[++i] = now;
            chessboard[Node[now].x][Node[now].y] = player;
            player = -player;
        }

        double val = judge_value(-player);

        for (int j = i; j > 0; j--) {
            Node[search_path[j]].Times++;
            Node[search_path[j]].Value += val;
            val *= -1;
            chessboard[Node[search_path[j]].x][Node[search_path[j]].y] = 0;
        }
        Node[0].Times++;
        player = player0;
    }

    double max_val = -1;
    int best = 0;
    for (int i = 1; i <= Node[0].NowChildren; i++) {
        int c = Node[0].Children[i];
        double v = Node[c].Value / Node[c].Times;
        if (v > max_val) {
            max_val = v;
            best = c;
        }
    }
    x = Node[best].x;
    y = Node[best].y;
}

// ====================== 决策调度 ======================

void choose() {
    player0 = player;

    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            cnt += put_judge(i, j, player);

    if (cnt > 49) depth = 6;
    else if (cnt > 36) depth = 8;
    else if (cnt > 20) depth = 10;
    else depth = 99;

    if (cnt <= 10 && enumerate(1)) return;

    MCTS();
}

// ====================== 程序入口 ======================

int main() {
    srand((unsigned)time(0));
    start = clock();
    memset(chessboard, 0, sizeof(chessboard));

    string line;
    getline(cin, line);
    Json::Reader reader;
    Json::Value input;
    reader.parse(line, input);

    int turn = input["responses"].size();
    player = 1;

    for (int i = 0; i < turn; i++) {
        int ax = input["requests"][i]["x"].asInt();
        int ay = input["requests"][i]["y"].asInt();
        if (ax != -1) { chessboard[ax][ay] = player; player = -player; }

        int rx = input["responses"][i]["x"].asInt();
        int ry = input["responses"][i]["y"].asInt();
        if (rx != -1) { chessboard[rx][ry] = player; player = -player; }
    }

    int fx = input["requests"][turn]["x"].asInt();
    int fy = input["requests"][turn]["y"].asInt();
    if (fx != -1) { chessboard[fx][fy] = player; player = -player; }

    choose();

    Json::Value ret, action;
    action["x"] = x;
    action["y"] = y;
    ret["response"] = action;
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;

    return 0;
}
