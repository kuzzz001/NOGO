#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <ctime>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>

using namespace std;

// ====================== 全局变量与配置 ======================
int x, y;                // 最终决策出的落子坐标
int depth;               // MCTS 模拟时的最大深度限制
int chessboard[9][9] = { 0 }; // 棋盘状态：0为空，1为黑子，-1为白子
int black = 1, white = -1;    // 玩家颜色常量
int player = 1, player0;      // 当前轮到谁下（player），以及初始轮到的玩家（player0）
int dx[4] = { 1,0,-1,0 }, dy[4] = { 0,1,0,-1 }; // 方向数组，用于遍历上下左右四个邻居
int vis_tag = 0;
int visited[9][9] = {0};
chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
int timeout_ms = 950;
bool enumerate_timedout = false;

inline int elapsed_ms() {
    return (int)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time).count();
}

// ====================== MCTS (蒙特卡洛树搜索) 结构定义 ======================
#define NODE_MAX 500000
struct MCTSNode {
    int x, y;          // 该节点对应的落子位置
    bool Expanded;     // 是否已完全扩展
    int NowChildren;   // 当前已生成的子节点数量
    int Children[81];  // 子节点索引
    double Value;      // 累积价值
    double Times;      // 访问次数
    int UnexpandedCount;    // 尚未扩展的可选动作数量
    int UnexpandedMoves[81]; // 尚未扩展的可选动作列表

    // 初始化节点，避免全量 memset
    void init(int tx, int ty) {
        x = tx; y = ty;
        Expanded = false;
        NowChildren = 0;
        Value = 0;
        Times = 0;
        UnexpandedCount = -1; // -1 表示尚未初始化可用动作
    }
} Node[NODE_MAX] = {};
int now = 0;           // 搜索时当前所在的节点索引
int now_expanded = 0;  // 已分配的节点总数

// ====================== 可用落子点管理 ======================
// 用于加速 MCTS，记录每个玩家当前的合法落子位置列表
int len[2] = { 0 }, avail[2][81] = { 0 };
int len_store[2] = { 0 }, avail_store[2][81] = { 0 };

// 辅助函数：将玩家颜色 (-1, 1) 映射为索引 (0, 1)
inline int trans(int color) { return (color + 1) / 2; }
// 辅助函数：将索引 (0, 1) 映射回玩家颜色 (-1, 1)
inline int retrans(int number) { return number * 2 - 1; }

struct CandidateMove {
    int move;
    int rightValue;
    int distanceScore;
};

// ====================== 基础博弈逻辑 (不围棋规则) ======================

/**
 * 判断 (x, y) 处的棋子（属于 player）是否还有“气”。
 * 优化：使用 vis_tag 避免频繁重置 visited 数组，不再修改 chessboard。
 */
bool air_judge(int x, int y, int player) {
    if (x < 0 || x > 8 || y < 0 || y > 8) return 0;
    if (chessboard[x][y] == -player) return 0;
    if (chessboard[x][y] == 0) return 1;
    if (visited[x][y] == vis_tag) return 0; // 已访问

    visited[x][y] = vis_tag;
    return air_judge(x + 1, y, player)
        || air_judge(x - 1, y, player)
        || air_judge(x, y + 1, player)
        || air_judge(x, y - 1, player);
}

/**
 * 判断在 (x, y) 处为 player 落子是否合法。
 */
bool put_judge(int x, int y, int player) {
    if (x < 0 || y < 0 || x > 8 || y > 8 || chessboard[x][y] != 0) return 0;
    chessboard[x][y] = player;

    bool legal = true;
    vis_tag++; // 每次判断前增加标记，相当于清空 visited
    if (!air_judge(x, y, player)) legal = false;
    else {
        // 检查四周对手是否被吃子
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx >= 0 && nx <= 8 && ny >= 0 && ny <= 8 && chessboard[nx][ny] == -player) {
                vis_tag++;
                if (!air_judge(nx, ny, -player)) {
                    legal = false;
                    break;
                }
            }
        }
    }

    chessboard[x][y] = 0; // 撤销落子
    return legal;
}

/**
 * 检查 player 是否已经输掉比赛（即没有任何合法落子位置）。
 */
bool end_judge(int player) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (put_judge(i, j, player)) return 0;
    return 1;
}

// ====================== 评估函数与启发式逻辑 ======================

/**
 * 判断 (x, y) 是否为 player 的“眼”（被己方棋子或边界包围）。
 */
bool eye_judge(int x, int y, int player) {
    if (chessboard[x][y]) return 0;
    return ((x + 1 > 8 || chessboard[x + 1][y] == player)
         && (x - 1 < 0 || chessboard[x - 1][y] == player)
         && (y + 1 > 8 || chessboard[x][y + 1] == player)
         && (y - 1 < 0 || chessboard[x][y - 1] == player));
}

/**
 * 计算棋块的气数。
 * 这里用局部 visited 数组精确统计连通块周围不同空点的数量，
 * 供虎口判断使用。
 */
int count_group_liberties(int sx, int sy, int color) {
    bool stoneVisited[9][9] = {};
    bool libertyVisited[9][9] = {};
    int stackX[81], stackY[81];
    int top = 0;
    int liberties = 0;

    stackX[top] = sx;
    stackY[top] = sy;
    stoneVisited[sx][sy] = true;
    top++;

    while (top) {
        top--;
        int x = stackX[top];
        int y = stackY[top];
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx < 0 || nx > 8 || ny < 0 || ny > 8) continue;
            if (chessboard[nx][ny] == 0) {
                if (!libertyVisited[nx][ny]) {
                    libertyVisited[nx][ny] = true;
                    liberties++;
                }
            } else if (chessboard[nx][ny] == color && !stoneVisited[nx][ny]) {
                stoneVisited[nx][ny] = true;
                stackX[top] = nx;
                stackY[top] = ny;
                top++;
            }
        }
    }
    return liberties;
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

int eye_count(int color) {
    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (eye_judge(i, j, color)) cnt++;
    return cnt;
}

int tiger_mouth_count(int color) {
    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (!put_judge(i, j, color)) continue;
            chessboard[i][j] = color;
            if (count_group_liberties(i, j, color) == 1) cnt++;
            chessboard[i][j] = 0;
        }
    return cnt;
}

int move_distance_score(int x, int y) {
    int best = 100;
    bool hasStone = false;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (chessboard[i][j] == 0) continue;
            hasStone = true;
            best = min(best, abs(x - i) + abs(y - j));
        }
    return hasStone ? best : 0;
}

void collect_ordered_moves(int color, int moves[], int& count) {
    CandidateMove cand[81];
    count = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (!put_judge(i, j, color)) continue;
            chessboard[i][j] = color;
            cand[count++] = { i * 9 + j, right_value(color), move_distance_score(i, j) };
            chessboard[i][j] = 0;
        }

    sort(cand, cand + count, [](const CandidateMove& a, const CandidateMove& b) {
        if (a.rightValue != b.rightValue) return a.rightValue < b.rightValue;
        if (a.distanceScore != b.distanceScore) return a.distanceScore < b.distanceScore;
        return a.move < b.move;
    });

    for (int i = 0; i < count; i++) moves[i] = cand[i].move;
}

double feature_score(int color) {
    // 论文中的核心特征：眼、虎口、禁入点。
    // 这里直接用加权和进行估值，避免再引入与 NoGo 规则无关的中心偏置。
    int eyes = eye_count(color);
    int tiger = tiger_mouth_count(color);
    int forbidden = forbidden_count(color);
    return 10.0 * eyes + 3.0 * tiger - 5.0 * forbidden;
}

/**
 * 局面评估函数：轻量评估，只做全盘合法点和眼位统计。
 */
double judge_value(int player) {
    double p = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (chessboard[i][j] != 0) continue;
            if (!put_judge(i, j, player)) {
                p -= 1;
                if (eye_judge(i, j, player)) p -= 0.2;
            } else if (eye_judge(i, j, player)) p += 0.2;
            if (!put_judge(i, j, -player)) p += 1;
        }
    return 1.0 / (1.0 + exp(-p));
}

// ====================== 终局搜索 (Minimax 穷举) ======================

/**
 * 在剩余空位较少时（如 <= 10），通过 Minimax 算法穷举所有可能，寻找必胜策略。
 * k: 当前递归深度（步数）。
 * 返回值：1 表示当前玩家必胜，0 表示必败。
 */
int enumerate(int k) {
    if (enumerate_timedout || elapsed_ms() > timeout_ms * 7 / 10) {
        enumerate_timedout = true;
        return 0;
    }
    int moves[81], moveCount = 0;
    collect_ordered_moves(player, moves, moveCount);
    for (int idx = 0; idx < moveCount; idx++) {
        int i = moves[idx] / 9;
        int j = moves[idx] % 9;
        chessboard[i][j] = player;
        player = -player;
        int result = enumerate(k + 1);
        chessboard[i][j] = 0;
        player = -player;
        if (enumerate_timedout) return 0;
        if (!result) {
            if (k == 1) { x = i; y = j; }
            return 1;
        }
    }
    return 0;
}

// ====================== MCTS 核心步骤：选择与扩展 ======================

/**
 * MCTS 的 Selection（选择）和 Expansion（扩展）阶段。
 * 使用 UCB1 算法在搜索树中向下导航，直到找到一个尚未完全扩展的节点。
 * 返回值：1 表示进行了扩展（生成了新节点），0 表示只是向下选择，-1 表示该分支已无可落子点。
 */
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
            Node[now_expanded].init(selec / 9, selec % 9);
            if (tmplen == 1) Node[now].Expanded = 1;
            now = now_expanded;
            return 1;
        }
    }

    double max_ucb = -1e18;
    int best = -1;
    double logTimes = log(max(1.0, Node[now].Times));
    for (int i = 1; i <= Node[now].NowChildren; i++) {
        int c = Node[now].Children[i];
        double ucb = Node[c].Value / Node[c].Times + 1.414 * sqrt(logTimes / Node[c].Times);
        if (ucb > max_ucb) {
            max_ucb = ucb;
            best = c;
        }
    }

    if (best == -1) return -1;
    now = best;
    return 0;
}

// ====================== MCTS 主算法循环 ======================

/**
 * 蒙特卡洛树搜索主函数。
 * 在规定的时间内不断进行：选择 -> 扩展 -> 模拟 -> 反向传播。
 */
void MCTS() {
    now_expanded = 0;
    Node[0].init(-1, -1);
    int search_path[100];

    len_store[1] = len_store[0] = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, black)) avail_store[1][len_store[1]++] = i * 9 + j;
            if (put_judge(i, j, white)) avail_store[0][len_store[0]++] = i * 9 + j;
        }

    while (elapsed_ms() < timeout_ms) {
        int i = 0;
        now = 0;
        search_path[0] = 0;
        memcpy(avail, avail_store, sizeof(avail));
        memcpy(len, len_store, sizeof(len));

        while (len[trans(player)] > 0 && i <= depth) {
            int res = selection();
            if (res == -1) break;
            search_path[++i] = now;
            chessboard[Node[now].x][Node[now].y] = player;
            player = -player;
        }

        // 2. Simulation (模拟/评估)
        double val = judge_value(-player);

        // 3. Backpropagation (反向传播)
        for (int j = i; j > 0; j--) {
            Node[search_path[j]].Times++;
            Node[search_path[j]].Value += val;
            val *= -1; // 极小极大思想
            chessboard[Node[search_path[j]].x][Node[search_path[j]].y] = 0;
        }
        Node[0].Times++;
        player = player0; // 恢复当前玩家
    }

    // 搜索结束后，在根节点的所有子节点中选择访问次数最多（或胜率最高）的一个作为最终决定
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

/**
 * 根据当前棋局状态选择搜索深度和算法。
 */
void choose() {
    player0 = player;
    // 统计当前棋盘上的总合法落子点数
    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            cnt += put_judge(i, j, player);

    // 根据剩余空位动态调整 MCTS 搜索深度
    if (cnt > 49) depth = 6;
    else if (cnt > 36) depth = 8;
    else if (cnt > 20) depth = 10;
    else depth = 99;

    if (cnt <= 10) {
        enumerate_timedout = false;
        if (enumerate(1)) {
            if (put_judge(x, y, player0)) return;
        }
        if (enumerate_timedout) {
            int moves[81], moveCount = 0;
            collect_ordered_moves(player, moves, moveCount);
            if (moveCount > 0) { x = moves[0] / 9; y = moves[0] % 9; return; }
        }
    }
    
    MCTS();

    if (!put_judge(x, y, player0)) {
        for (int i = 0; i < 9; i++)
            for (int j = 0; j < 9; j++)
                if (put_judge(i, j, player0)) { x = i; y = j; return; }
    }
}

// ====================== 程序入口 ======================
int main() {
    srand((unsigned)time(0));
    int startup_ms = elapsed_ms();
    timeout_ms = max(100, timeout_ms - startup_ms - 30);
    start_time = chrono::steady_clock::now();
    memset(chessboard, 0, sizeof(chessboard));

    string line;
    getline(cin, line); // 从标准输入读取整行 JSON 数据
    
    // 手动解析 JSON (兼容 Botzone 平台的通信协议)
    // 协议格式：{"requests":[{"x":-1,"y":-1}, ...], "responses":[{"x":4,"y":4}, ...]}
    vector<pair<int, int>> requests;
    vector<pair<int, int>> responses;
    
    auto parse_json = [&](string s) {
        // 解析 requests 数组：对方和自己的历史落子请求
        size_t reqPos = s.find("\"requests\"");
        if (reqPos != string::npos) {
            size_t l = s.find('[', reqPos);
            size_t r = s.find(']', l);
            if (l != string::npos && r != string::npos) {
                string arr = s.substr(l + 1, r - l - 1);
                size_t pos = 0;
                while (true) {
                    size_t xPos = arr.find("\"x\"", pos);
                    if (xPos == string::npos) break;
                    size_t colon1 = arr.find(':', xPos);
                    size_t comma = arr.find(',', colon1);
                    int vx = stoi(arr.substr(colon1 + 1, comma - colon1 - 1));
                    size_t yPos = arr.find("\"y\"", comma);
                    size_t colon2 = arr.find(':', yPos);
                    size_t endObj = arr.find('}', colon2);
                    int vy = stoi(arr.substr(colon2 + 1, endObj - colon2 - 1));
                    requests.push_back({vx, vy});
                    pos = endObj + 1;
                }
            }
        }
        // 解析 responses 数组：我方之前的历史响应
        size_t resPos = s.find("\"responses\"");
        if (resPos != string::npos) {
            size_t l = s.find('[', resPos);
            size_t r = s.find(']', l);
            if (l != string::npos && r != string::npos) {
                string arr = s.substr(l + 1, r - l - 1);
                size_t pos = 0;
                while (true) {
                    size_t xPos = arr.find("\"x\"", pos);
                    if (xPos == string::npos) break;
                    size_t colon1 = arr.find(':', xPos);
                    size_t comma = arr.find(',', colon1);
                    int vx = stoi(arr.substr(colon1 + 1, comma - colon1 - 1));
                    size_t yPos = arr.find("\"y\"", comma);
                    size_t colon2 = arr.find(':', yPos);
                    size_t endObj = arr.find('}', colon2);
                    int vy = stoi(arr.substr(colon2 + 1, endObj - colon2 - 1));
                    responses.push_back({vx, vy});
                    pos = endObj + 1;
                }
            }
        }
    };
    
    parse_json(line);

    // 根据历史记录恢复棋盘状态
    int turn = responses.size();
    player = 1; // 默认黑棋先手

    for (int i = 0; i < turn; i++) {
        // 恢复对方的落子
        int ax = requests[i].first;
        int ay = requests[i].second;
        if (ax != -1) { chessboard[ax][ay] = player; player = -player; }

        // 恢复我方的响应
        int rx = responses[i].first;
        int ry = responses[i].second;
        if (rx != -1) { chessboard[rx][ry] = player; player = -player; }
    }

    // 处理当前最新的一个请求
    int fx = requests[turn].first;
    int fy = requests[turn].second;
    if (fx != -1) { chessboard[fx][fy] = player; player = -player; }

    // 开始决策
    choose();

    // 输出决策结果，格式必须符合 Botzone 要求
    cout << "{\"response\":{\"x\":" << x << ",\"y\":" << y << "}}" << endl;

    return 0;
}
