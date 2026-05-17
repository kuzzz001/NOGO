// ============================================================
// NoGo (不围棋) 强化博弈算法 v2
// MCTS + Minimax 混合策略
// ============================================================
// 不围棋制胜核心：
//   赢 = 对方无合法落子。输 = 吃子 / 自杀 / 无合法落子。
//   权利值（我方可落子 - 对方可落子）是优势唯一可靠指标。
//   策略：持续压缩对方可选空间，不越过「吃子」红线。
//
// v1 -> v2 修复（v1 被 second.cpp 碾压的根因）：
//   BUG1: sigmoid 冷却因子 0.08 → 估值全在 0.5±0.3 → UCB 无区分力
//   BUG2: 扩展改为确定性排序 → 摧毁 MCTS 探索 → 搜索窄而偏
//   BUG3: VirtLoss 跨迭代污染 + 开销大
//   BUG4: min_opp_liberty BFS 太慢 → 迭代数锐减
//
// v2 改进（在 second.cpp 正确架构上增量强化）：
//   1. 权利值核心评估（±3 权重，大信号 → UCB 区分度好）
//   2. 自适应 UCB（权利值引导探索/利用平衡）
//   3. 有序 Minimax 终局穷举（启发式排序→必胜路径快速发现）
//   4. 更深搜索 depth + 更多迭代（轻量 eval = 更快）
//   5. 终局穷举阈值提升到 14（vs second 的 10）
// ============================================================

#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <cmath>
#include <algorithm>
#include "jsoncpp/json.h"

using namespace std;

int x, y;
int depth;
int chessboard[9][9] = { 0 };
int black = 1, white = -1;
int player = 1, player0;
int start_clock, timeout;

#define NODE_MAX 1500000

struct MCTSNode {
    int x, y;
    bool Expanded;
    int NowChildren;
    int Children[81];
    double Value;
    double Times;
} Node[NODE_MAX] = {};

int now = 0, now_expanded = 0;

int len[2] = { 0 }, avail[2][81] = { 0 };
int len_store[2] = { 0 }, avail_store[2][81] = { 0 };

inline int trans(int c) { return (c + 1) >> 1; }

// ====================== 规则引擎 ======================

bool air_judge(int x, int y, int p) {
    if (x < 0 || x > 8 || y < 0 || y > 8) return 0;
    if (chessboard[x][y] == -p) return 0;
    if (chessboard[x][y] == 0) return 1;
    chessboard[x][y] = -p;
    bool res = air_judge(x + 1, y, p)
            || air_judge(x - 1, y, p)
            || air_judge(x, y + 1, p)
            || air_judge(x, y - 1, p);
    chessboard[x][y] = p;
    return res;
}

bool put_judge(int x, int y, int p) {
    if (x < 0 || y < 0 || x > 8 || y > 8 || chessboard[x][y] != 0) return 0;
    chessboard[x][y] = p;
    bool legal = true;
    if (!air_judge(x, y, p)) legal = false;
    else if ((x + 1 <= 8 && chessboard[x + 1][y] == -p && !air_judge(x + 1, y, -p))
          || (x - 1 >= 0 && chessboard[x - 1][y] == -p && !air_judge(x - 1, y, -p))
          || (y + 1 <= 8 && chessboard[x][y + 1] == -p && !air_judge(x, y + 1, -p))
          || (y - 1 >= 0 && chessboard[x][y - 1] == -p && !air_judge(x, y - 1, -p)))
        legal = false;
    chessboard[x][y] = 0;
    return legal;
}

bool end_judge(int p) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (put_judge(i, j, p)) return 0;
    return 1;
}

// ====================== 评估函数 ======================

// 仿真评估（权利值核心，权重 ±3，大信号保证 UCB 区分度）
double judge_value(int p) {
    double score = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (chessboard[i][j] != 0) {
                // 占有棋子微弱正面
                if (chessboard[i][j] == p)  score += 0.1;
                else                        score -= 0.1;
                continue;
            }
            if (!put_judge(i, j, -p)) score += 3.0;
            if (!put_judge(i, j, p))  score -= 3.0;
        }
    return 1.0 / (1.0 + exp(-score));
}

// ====================== 有序 Minimax（终局穷举） ======================

struct Scored { int pos; int score; };
bool operator<(const Scored& a, const Scored& b) { return a.score < b.score; }

int enumerate(int k) {
    Scored tmp[81];
    int n = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (!put_judge(i, j, player)) continue;
            chessboard[i][j] = player;
            int opp = 0;
            for (int a = 0; a < 9; a++)
                for (int b = 0; b < 9; b++)
                    if (put_judge(a, b, -player)) opp++;
            chessboard[i][j] = 0;
            tmp[n].pos = i * 9 + j;
            tmp[n].score = opp;
            n++;
        }
    sort(tmp, tmp + n);

    for (int m = 0; m < n; m++) {
        int i = tmp[m].pos / 9, j = tmp[m].pos % 9;
        chessboard[i][j] = player;
        player = -player;
        bool opp_lose = !enumerate(k + 1);
        chessboard[i][j] = 0;
        player = -player;
        if (opp_lose) {
            if (k == 0) { x = i; y = j; }
            return 1;
        }
    }
    return 0;
}

// ====================== MCTS ======================

double ucb_c = 1.414;

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

    // UCB1 选择
    double max_ucb = -1e9;
    int best = 0;
    for (int i = 1; i <= Node[now].NowChildren; i++) {
        int c = Node[now].Children[i];
        double ucb = Node[c].Value / Node[c].Times
                   + ucb_c * sqrt(log(Node[now].Times + 1.0) / Node[c].Times);
        if (ucb > max_ucb) {
            max_ucb = ucb;
            best = c;
        }
    }
    now = best;
    return 0;
}

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

    int my_avail  = len_store[trans(player0)];
    int opp_avail = len_store[trans(-player0)];
    int rv0 = my_avail - opp_avail;
    if      (rv0 > 10) ucb_c = 0.5;
    else if (rv0 > 5)  ucb_c = 1.0;
    else if (rv0 < -8) ucb_c = 2.5;
    else if (rv0 < -3) ucb_c = 2.0;
    else               ucb_c = 1.414;

    while (clock() - start_clock < timeout) {
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
            val = -val;
            chessboard[Node[search_path[j]].x][Node[search_path[j]].y] = 0;
        }
        Node[0].Times++;
        player = player0;
    }

    double max_val = -1e9;
    int best = 0;
    for (int i = 1; i <= Node[0].NowChildren; i++) {
        int c = Node[0].Children[i];
        double v = Node[c].Value / Node[c].Times;
        if (Node[c].Times < 3) v *= Node[c].Times / 3.0;
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

    if (cnt == 0) return;

    if (cnt > 49)      depth = 10;
    else if (cnt > 36) depth = 14;
    else if (cnt > 20) depth = 22;
    else               depth = 70;

    if (cnt <= 14 && enumerate(0)) return;

    MCTS();
}

// ====================== 入口 ======================

int main() {
    srand((unsigned)time(0));
    start_clock = clock();
    timeout = (int)(0.95 * (double)CLOCKS_PER_SEC);
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
