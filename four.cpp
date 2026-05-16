#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <ctime>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>
using namespace std;

// ==================== 全局变量 ====================
int x, y;                       // 最终输出
int depth;                      // 模拟深度上限
int chessboard[9][9] = {0};
const int black = 1, white = -1;
int player;                     // 当前落子方
int player0;                    // choose() 时的原始玩家

const int dx[4] = {1, 0, -1, 0};
const int dy[4] = {0, 1, 0, -1};

int visited[9][9] = {0};
int vis_tag = 0;

// 计时
chrono::steady_clock::time_point start_time;
const int timeout_ms = 950;
inline int elapsed_ms() {
    return (int)chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - start_time).count();
}

inline int trans(int color) { return (color + 1) / 2; } // black→1, white→0

// 快速随机数 (xorshift)
static unsigned int rand_state = 123456789;
inline void fast_srand(unsigned seed) { rand_state = seed; }
inline int fast_rand() {
    rand_state ^= rand_state << 13;
    rand_state ^= rand_state >> 17;
    rand_state ^= rand_state << 5;
    return (int)(rand_state & 0x7FFFFFFF);
}
inline int rand_int(int n) { return fast_rand() % n; }

// ==================== 气的判断（迭代版，用显式栈） ====================
bool air_judge(int sx, int sy, int color) {
    // 手动栈，最多 81 个点
    int stk_x[82], stk_y[82];
    int top = 1;
    stk_x[0] = sx; stk_y[0] = sy;
    visited[sx][sy] = vis_tag;
    while (top > 0) {
        top--;
        int cx = stk_x[top], cy = stk_y[top];
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx > 8 || ny < 0 || ny > 8) continue;
            if (visited[nx][ny] == vis_tag) continue;
            if (chessboard[nx][ny] == -color) continue;
            if (chessboard[nx][ny] == 0) return true; // 有气
            visited[nx][ny] = vis_tag;
            stk_x[top] = nx; stk_y[top] = ny;
            top++;
        }
    }
    return false; // 无气
}

// ==================== 落子合法性判断 ====================
bool put_judge(int px, int py, int color) {
    if (px < 0 || px > 8 || py < 0 || py > 8) return false;
    if (chessboard[px][py] != 0) return false;

    chessboard[px][py] = color;
    bool legal = true;

    vis_tag++;
    if (!air_judge(px, py, color)) {
        legal = false;          // 自己无气 → 非法
    }
    if (legal) {
        for (int d = 0; d < 4; d++) {
            int nx = px + dx[d], ny = py + dy[d];
            if (nx < 0 || nx > 8 || ny < 0 || ny > 8) continue;
            if (chessboard[nx][ny] == -color) {
                vis_tag++;
                if (!air_judge(nx, ny, -color)) {
                    legal = false; // 邻敌无气 → 非法（原代码逻辑）
                    break;
                }
            }
        }
    }
    chessboard[px][py] = 0;
    return legal;
}

// ==================== 终点判断（只用于 choose() 判断是否终局） ====================
bool end_judge(int color) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (put_judge(i, j, color))
                return false;
    return true;
}

// ==================== 眼判断 ====================
bool eye_judge(int i, int j, int color) {
    if (chessboard[i][j]) return false;
    return (i+1>8 || chessboard[i+1][j]==color) &&
           (i-1<0 || chessboard[i-1][j]==color) &&
           (j+1>8 || chessboard[i][j+1]==color) &&
           (j-1<0 || chessboard[i][j-1]==color);
}

// ==================== 估值函数 ====================
double judge_value(int color) {
    double p = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (chessboard[i][j] != 0) continue;
            if (!put_judge(i, j, color)) {
                p -= 1;
                if (eye_judge(i, j, color)) p -= 0.2;
            } else if (eye_judge(i, j, color)) {
                p += 0.2;
            }
            if (!put_judge(i, j, -color))
                p += 1;
        }
    }
    return 1.0 / (1.0 + exp(-p));
}

// ==================== 穷举搜索（仅当合法动作 ≤ 10 时使用） ====================
bool enumerate(int k) {
    if (elapsed_ms() > timeout_ms * 8 / 10) return false;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, player)) {
                chessboard[i][j] = player;
                player = -player;
                bool win = enumerate(k + 1);
                if (!win) {
                    if (k == 1) { x = i; y = j; }
                    chessboard[i][j] = 0;
                    player = -player;
                    return true;
                }
                chessboard[i][j] = 0;
                player = -player;
            }
        }
    }
    return false;
}

// ==================== MCTS 节点（修复 Children 大小为 82） ====================
#define NODE_MAX 1200000           // 适度减小以保内存安全
struct MCTSNode {
    int x, y;
    bool Expanded;
    int NowChildren;
    int Children[82];              // 修复：81 个合法点 + 1-indexed = 82
    double Value;
    double Times;
    void init(int xx, int yy) {
        x = xx; y = yy;
        Expanded = false;
        NowChildren = 0;
        Value = 0;
        Times = 0;
    }
} Node[NODE_MAX];
int now = 0;
int now_expanded = 0;

// 合法动作列表（每方各一套）
int len[2] = {0};
int avail[2][81] = {0};
int len_store[2] = {0};
int avail_store[2][81] = {0};

// ==================== MCTS 选择 + 扩展（修复版） ====================
int selection() {
    int selec = 0;
    int number = trans(player);             // black→1, white→0

    if (!Node[now].Expanded) {
        // 复制当前合法动作列表
        int tmplen = len[number];
        int tmpavail[81];
        memcpy(tmpavail, avail[number], sizeof(tmpavail));

        // 移除已经尝试过的子节点
        for (int i = 1; i <= Node[now].NowChildren; i++) {
            int c = Node[Node[now].Children[i]].x * 9 + Node[Node[now].Children[i]].y;
            int* sign = find(tmpavail, tmpavail + tmplen, c);
            if (sign != tmpavail + tmplen)
                *sign = tmpavail[--tmplen];
        }

        // 尝试随机选一个合法动作
        while (true) {
            if (tmplen == 0) {
                if (Node[now].NowChildren == 0)
                    return -1;              // 完全无子节点 → 终局
                Node[now].Expanded = true;  // 标记完全展开，回退到 UCB
                break;
            }
            int r = rand_int(tmplen);
            int xx = tmpavail[r] / 9;
            int yy = tmpavail[r] % 9;
            if (put_judge(xx, yy, player)) {
                selec = tmpavail[r];
                // 从全局 avail 中移除（避免重复）
                int* sign = find(avail[number], avail[number] + len[number], selec);
                if (sign != avail[number] + len[number])
                    *sign = avail[number][--len[number]];
                break;
            } else {
                // 该动作已不合法，从全局和局部都移除
                int* sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number])
                    *sign = avail[number][--len[number]];
                tmpavail[r] = tmpavail[--tmplen];
            }
        }

        if (selec != 0) {
            if (now_expanded + 1 >= NODE_MAX) return -1;
            Node[now].NowChildren++;
            Node[now].Children[Node[now].NowChildren] = ++now_expanded;
            Node[now_expanded].init(selec / 9, selec % 9);
            now = now_expanded;
            return 1;                       // 扩展了新节点
        }
    }

    // 节点已完全展开 → UCB 选择
    double best_ucb = -1e18;
    int best = -1;
    double logt = log(max(1.0, Node[now].Times));
    for (int i = 1; i <= Node[now].NowChildren; i++) {
        int c = Node[now].Children[i];
        double ucb = Node[c].Value / Node[c].Times +
                     1.414 * sqrt(logt / Node[c].Times);
        if (ucb > best_ucb) {
            best_ucb = ucb;
            best = c;
        }
    }
    if (best == -1) return -1;
    now = best;
    return 0;                               // 选择已有子节点
}

// ==================== MCTS 主循环（优化版：去掉模拟中的 end_judge） ====================
void MCTS() {
    now_expanded = 0;
    Node[0].init(-1, -1);
    int search_path[100];

    // 预计算双方合法动作（初始棋盘）
    len_store[0] = len_store[1] = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, black))
                avail_store[1][len_store[1]++] = i * 9 + j;
            if (put_judge(i, j, white))
                avail_store[0][len_store[0]++] = i * 9 + j;
        }
    }

    while (elapsed_ms() < timeout_ms) {
        int dep = 0;
        now = 0;
        search_path[0] = 0;

        // 每轮模拟重置 avail
        memcpy(avail, avail_store, sizeof(avail));
        memcpy(len, len_store, sizeof(len));

        // 模拟落子，直到深度上限或无法落子
        while (dep < depth) {
            int res = selection();
            if (res == -1) break;           // 无法继续（终局或错误）
            search_path[++dep] = now;
            chessboard[Node[now].x][Node[now].y] = player;
            player = -player;

            // 【关键修正】更新 avail：落子后，为当前玩家重新扫描合法点
            // 因为可能提子导致新空位出现
            int cur = trans(player);
            len[cur] = 0;
            for (int i = 0; i < 9; i++)
                for (int j = 0; j < 9; j++)
                    if (put_judge(i, j, player))
                        avail[cur][len[cur]++] = i * 9 + j;
        }

        // 估值
        double val = judge_value(-player);

        // 反向传播
        for (int i = dep; i > 0; i--) {
            Node[search_path[i]].Times++;
            Node[search_path[i]].Value += val;
            val = -val;
            chessboard[Node[search_path[i]].x][Node[search_path[i]].y] = 0;
        }
        Node[0].Times++;
        player = player0;
    }

    // 选择胜率最高的子节点
    double bestv = -1e18;
    int best = 0;
    for (int i = 1; i <= Node[0].NowChildren; i++) {
        int c = Node[0].Children[i];
        double v = Node[c].Value / Node[c].Times;
        if (v > bestv) {
            bestv = v;
            best = c;
        }
    }
    if (best != 0) {
        x = Node[best].x;
        y = Node[best].y;
    }
}

// ==================== 顶层决策 ====================
void choose() {
    player0 = player;

    // 统计合法落子数
    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            cnt += put_judge(i, j, player);

    // 根据合法点数设定深度
    if (cnt > 49)      depth = 6;
    else if (cnt > 36) depth = 8;
    else if (cnt > 20) depth = 10;
    else               depth = 99;

    // 合法点 ≤ 10：穷举搜索
    if (cnt <= 10) {
        if (enumerate(1)) return;
    }

    MCTS();

    // 兜底：若 MCTS 结果不合法，选第一个合法点
    if (!put_judge(x, y, player0)) {
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                if (put_judge(i, j, player0)) {
                    x = i; y = j;
                    return;
                }
            }
        }
    }
}

// ==================== JSON 解析（不变） ====================
void parse_input(string &line, vector<pair<int,int>>& requests,
                 vector<pair<int,int>>& responses) {
    size_t reqPos = line.find("\"requests\"");
    if (reqPos != string::npos) {
        size_t l = line.find('[', reqPos);
        size_t r = line.find(']', l);
        string arr = line.substr(l+1, r-l-1);
        size_t pos = 0;
        while (true) {
            size_t xp = arr.find("\"x\"", pos);
            if (xp == string::npos) break;
            size_t c1 = arr.find(':', xp);
            size_t cm = arr.find(',', c1);
            int vx = stoi(arr.substr(c1+1, cm-c1-1));
            size_t yp = arr.find("\"y\"", cm);
            size_t c2 = arr.find(':', yp);
            size_t ed = arr.find('}', c2);
            int vy = stoi(arr.substr(c2+1, ed-c2-1));
            requests.push_back({vx, vy});
            pos = ed + 1;
        }
    }
    size_t resPos = line.find("\"responses\"");
    if (resPos != string::npos) {
        size_t l = line.find('[', resPos);
        size_t r = line.find(']', l);
        string arr = line.substr(l+1, r-l-1);
        size_t pos = 0;
        while (true) {
            size_t xp = arr.find("\"x\"", pos);
            if (xp == string::npos) break;
            size_t c1 = arr.find(':', xp);
            size_t cm = arr.find(',', c1);
            int vx = stoi(arr.substr(c1+1, cm-c1-1));
            size_t yp = arr.find("\"y\"", cm);
            size_t c2 = arr.find(':', yp);
            size_t ed = arr.find('}', c2);
            int vy = stoi(arr.substr(c2+1, ed-c2-1));
            responses.push_back({vx, vy});
            pos = ed + 1;
        }
    }
}

// ==================== 主函数 ====================
int main() {
    fast_srand((unsigned)time(0));
    start_time = chrono::steady_clock::now();

    string line;
    getline(cin, line);
    vector<pair<int,int>> requests, responses;
    parse_input(line, requests, responses);

    int turn = responses.size();
    player = 1;
    for (int i = 0; i < turn; i++) {
        int ax = requests[i].first, ay = requests[i].second;
        if (ax != -1) { chessboard[ax][ay] = player; player = -player; }
        int rx = responses[i].first, ry = responses[i].second;
        if (rx != -1) { chessboard[rx][ry] = player; player = -player; }
    }
    int fx = requests[turn].first, fy = requests[turn].second;
    if (fx != -1) { chessboard[fx][fy] = player; player = -player; }

    choose();

    cout << "{\"response\":{\"x\":" << x << ",\"y\":" << y << "}}" << endl;
    return 0;
}
