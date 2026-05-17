// ============================================================
// NoGo (不围棋) 博弈算法 —— MCTS + Minimax 混合策略
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

// x, y: 最终决策出的落子坐标（输出给判题系统）
int x, y;

// depth: MCTS 模拟深度上限，根据棋盘剩余空位动态调整
int depth;

// chessboard: 9x9 棋盘
//   0  = 空位
//   1  = 黑棋
//   -1 = 白棋
int chessboard[9][9] = { 0 };

// black / white: 颜色常量，方便阅读
int black = 1, white = -1;

// player:   当前轮到谁落子（1=黑, -1=白）
// player0:  choose() 时刻的玩家，MCTS 内部切换 player 后用于恢复
int player = 1, player0;

// dx, dy: 方向数组，分别表示 右/下/左/上 四个方向
int dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };

// start:   程序启动时钟（用于计时）
// timeout: 超时限制 = 0.95 秒（CLOCKS_PER_SEC 是每秒的时钟滴答数）
int start, timeout = (int)(0.95 * (double)CLOCKS_PER_SEC);

// ====================== MCTS 节点结构 ======================

// MCTS 树的最大节点数（150 万），静态数组分配，避免动态内存开销
#define NODE_MAX 1500000

struct MCTSNode {
    int x, y;                // 该节点对应的落子坐标
    bool Expanded;           // 该节点是否已完全扩展（所有子节点已生成）
    int NowChildren;         // 当前已生成的子节点数量
    int Children[81];        // 子节点在 Node 数组中的索引（最多 81 个合法落子）
    double Value;            // 累积价值（Wins）
    double Times;            // 访问次数（Visits）
} Node[NODE_MAX] = {};

int now = 0;           // 搜索过程中当前所在的节点索引
int now_expanded = 0;  // 当前已分配的节点总数（从 0 开始递增）

// ====================== 可用落子点管理 ======================

// 不围棋每回合的合法落子点不会立刻变化，预先计算后可以复用来加速 MCTS。
// len[color_idx], avail[color_idx][]: 当前模拟中该颜色玩家的可用落子点
// len_store, avail_store: choose() 时预先算好的"基准"可用落子点，每轮模拟从它复制
int len[2] = { 0 }, avail[2][81] = { 0 };
int len_store[2] = { 0 }, avail_store[2][81] = { 0 };

// trans(color):   将颜色值 (-1 或 1) 映射为数组索引 (0 或 1)
//                 计算: (-1+1)/2=0, (1+1)/2=1
inline int trans(int color) { return (color + 1) / 2; }

// retrans(num):   将数组索引 (0 或 1) 映射回颜色值 (-1 或 1)
inline int retrans(int number) { return number * 2 - 1; }

// ====================== 不围棋规则引擎 ======================

/**
 * 判断 (x, y) 处属于 player 的棋子是否还有"气"。
 *
 * 算法：DFS 递归遍历连通块。
 *   1. 越界 → 无气
 *   2. 遇到对方棋子 → 此方向无气
 *   3. 遇到空位 → 有气（找到了气的出口）
 *   4. 遇到己方棋子 → 继续递归搜索
 *
 * 注意：递归过程中会把已访问的己方棋子标记为 -player（对方颜色），
 *       这样既避免了重复访问，又能正确区分连通块边界。
 *       递归返回前会把改动的棋盘恢复。
 */
bool air_judge(int x, int y, int player) {
    if (x < 0 || x > 8 || y < 0 || y > 8) return 0;   // 棋盘外
    if (chessboard[x][y] == -player) return 0;          // 撞到对方棋子
    if (chessboard[x][y] == 0) return 1;                // 找到空位 = 有气

    chessboard[x][y] = -player;   // 标记已访问（改为对方颜色，和己方颜色区别开）
    bool res = air_judge(x + 1, y, player)
            || air_judge(x - 1, y, player)
            || air_judge(x, y + 1, player)
            || air_judge(x, y - 1, player);
    chessboard[x][y] = player;    // 恢复原样
    return res;
}

/**
 * 判断 player 能否在 (x, y) 落子。
 *
 * 不围棋规则下，落子合法需要同时满足：
 *   1. 目标位置为空
 *   2. 落子后己方棋子有气（不能自杀）
 *   3. 落子后不能使任何相邻对方棋子无气（禁止吃子！这是不围棋和围棋的关键区别）
 */
bool put_judge(int x, int y, int player) {
    if (x < 0 || y < 0 || x > 8 || y > 8 || chessboard[x][y] != 0) return 0;

    chessboard[x][y] = player;   // 模拟落子

    bool legal = true;

    // 条件 2: 己方必须有气（不能下自杀棋）
    if (!air_judge(x, y, player)) legal = false;
    // 条件 3: 不能让相邻对方棋子无气（不围棋禁止吃子）
    else if ((x + 1 <= 8 && chessboard[x + 1][y] == -player && !air_judge(x + 1, y, -player))
          || (x - 1 >= 0 && chessboard[x - 1][y] == -player && !air_judge(x - 1, y, -player))
          || (y + 1 <= 8 && chessboard[x][y + 1] == -player && !air_judge(x, y + 1, -player))
          || (y - 1 >= 0 && chessboard[x][y - 1] == -player && !air_judge(x, y - 1, -player)))
        legal = false;

    chessboard[x][y] = 0;        // 撤销模拟落子
    return legal;
}

/**
 * 判断 player 是否已经输掉比赛。
 * 输棋条件：该玩家不存在任何合法落子点。
 */
bool end_judge(int player) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (put_judge(i, j, player)) return 0;  // 只要找到一个合法点就还没输
    return 1;  // 找不到合法点 → 输了
}

// ====================== 评估函数 ======================

/**
 * 判断 (x, y) 是否为 player 的"眼"。
 *
 * "眼"的定义：一个空位，其上下左右全都是己方棋子或棋盘边界。
 * 眼是围棋中的重要概念，通常意味着该位置对方无法侵入。
 */
bool eye_judge(int x, int y, int player) {
    if (chessboard[x][y]) return 0;  // 不是空位
    return ((x + 1 > 8 || chessboard[x + 1][y] == player)
         && (x - 1 < 0 || chessboard[x - 1][y] == player)
         && (y + 1 > 8 || chessboard[x][y + 1] == player)
         && (y - 1 < 0 || chessboard[x][y - 1] == player));
}

/**
 * 局面评估函数（轻量级，用于 MCTS 的模拟阶段）。
 *
 * 评估逻辑：
 *   - 对于我方 (player)：不能落子的空位扣分，是眼的空位额外扣分或加分
 *   - 对于对方 (-player)：不能落子的空位给我方加分
 *
 * 最后用 sigmoid 函数将评分映射到 (0, 1) 区间，作为获胜概率的近似。
 *
 * 公式：value = 1 / (1 + e^(-p))
 *   当 p→+∞ 时 value→1（我方大优）
 *   当 p→-∞ 时 value→0（我方大劣）
 */
double judge_value(int player) {
    double p = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (chessboard[i][j] != 0) continue;  // 跳过非空位

            // 我方视角：该空位
            if (!put_judge(i, j, player)) {
                p -= 1;      // 我方不能下 → 不利
                if (eye_judge(i, j, player)) p -= 0.2;  // 是眼 → 更不利
            } else if (eye_judge(i, j, player)) {
                p += 0.2;    // 我方可以下且是眼 → 有利
            }

            // 对方视角：该空位
            if (!put_judge(i, j, -player)) p += 1;  // 对方不能下 → 我方有利
        }
    return 1.0 / (1.0 + exp(-p));   // sigmoid 归一化
}

// ====================== 终局穷举（Minimax） ======================

/**
 * 极小化极大搜索（Minimax），用于残局阶段穷举所有可能。
 *
 * 当剩余合法落子点很少（≤ 10 个）时，MCTS 的概率估计不如穷举精确。
 * enumerate() 会深度优先搜索所有走法，找到必胜策略。
 *
 * 参数 k: 当前递归深度（1 = 第一步，也就是根节点的直接走法）
 * 返回值: 1 = 当前玩家必胜, 0 = 必败
 *
 * 关键逻辑：
 *   只要找到一个走法能让对方必败 → 我方必胜，立即返回
 *   所有走法都导致对方必胜 → 我方必败
 */
int enumerate(int k) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, player)) {
                // 模拟落子
                chessboard[i][j] = player;
                player = -player;         // 轮到对方

                // 递归：检查在这种走法下对方是否必败
                bool win = enumerate(k + 1);

                if (!win) {   // 对方必败 → 我方必胜！
                    if (k == 1) { x = i; y = j; }  // 记录第一步必胜走法
                    chessboard[i][j] = 0;
                    player = -player;
                    return 1;   // 找到一个必胜走法就返回
                }

                // 撤销落子，尝试下一个走法
                chessboard[i][j] = 0;
                player = -player;         // 恢复当前玩家
            }
        }
    return 0;  // 所有走法都无法使对方必败 → 我方必败（对方必胜）
}

// ====================== MCTS 选择与扩展 ======================

/**
 * MCTS 的 Selection（选择）+ Expansion（扩展）阶段。
 *
 * 返回值:
 *   1  = 进行了扩展（从当前节点生成了新的子节点）
 *   0  = 没有扩展，只是向下选择到已有子节点
 *   -1 = 当前分支已无合法落子点，搜索终止
 *
 * 流程：
 *   Step 1: 如果当前节点还没完全扩展 → 随机选一个未尝试的合法落子，扩展为新节点。
 *   Step 2: 如果当前节点已完全扩展 → 用 UCB1 公式选择最优子节点深入。
 *
 * UCB1 公式: UCB = Value/Times + sqrt(2 * ln(N_parent) / N_child)
 *   前半部分 Value/Times = 胜率（exploitation 利用）
 *   后半部分 sqrt(...)     = 探索奖励（exploration 探索）
 *   平衡已知好走法和未充分探索的走法。
 */
int selection() {
    int selec = 0;
    int number = trans(player);  // 当前玩家的颜色索引

    // --- 节点尚未完全扩展 → 生成新子节点 ---
    if (!Node[now].Expanded) {
        // 复制一份当前可用的落子点
        int tmplen = len[number];
        int tmpavail[81];
        memcpy(tmpavail, avail[number], sizeof(tmpavail));

        // 去掉已经生成过子节点的落子点
        int* sign;
        for (int i = 1; i <= Node[now].NowChildren; i++) {
            int c = Node[Node[now].Children[i]].x * 9 + Node[Node[now].Children[i]].y;
            sign = find(tmpavail, tmpavail + tmplen, c);
            if (sign != tmpavail + tmplen) *sign = tmpavail[--tmplen];
        }

        // 从剩余未扩展的落子点中随机选一个
        while (true) {
            if (tmplen == 0) {   // 没有未扩展的了
                if (Node[now].NowChildren == 0) return -1;  // 整个节点都没有合法落子
                Node[now].Expanded = 1;  // 标记为完全扩展
                selec = -1;
                break;
            }
            int r = rand() % tmplen;
            int xx = tmpavail[r] / 9, yy = tmpavail[r] % 9;
            if (put_judge(xx, yy, player)) {
                selec = tmpavail[r];
                // 从全局可用列表中移除（这个落子已被占用）
                sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number]) *sign = avail[number][--len[number]];
                break;
            } else {
                // 意外情况：此位置已不可用，从两个列表中移除
                sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number]) *sign = avail[number][--len[number]];
                tmpavail[r] = tmpavail[--tmplen];
            }
        }

        // 如果选出了合法落子，创建新节点
        if (selec != -1) {
            Node[now].NowChildren++;
            Node[now].Children[Node[now].NowChildren] = ++now_expanded;
            Node[now_expanded].x = selec / 9;
            Node[now_expanded].y = selec % 9;
            if (tmplen == 1) Node[now].Expanded = 1;  // 只剩一个可选，相当于完全扩展
            now = now_expanded;
            return 1;  // 扩展完成
        }
    }

    // --- 节点已完全扩展 → UCB1 选择最优子节点 ---
    double max_ucb = -1;
    int best = 0;
    for (int i = 1; i <= Node[now].NowChildren; i++) {
        int c = Node[now].Children[i];
        // UCB1 公式 = 胜率 + 探索项
        // 胜率部分 = Value / Times
        // 探索部分 = sqrt(2) * sqrt(ln(父节点访问次数) / 子节点访问次数)
        double ucb = Node[c].Value / Node[c].Times
                   + sqrt(2) * sqrt(log(Node[now].Times) / Node[c].Times);
        if (ucb > max_ucb) {
            max_ucb = ucb;
            best = c;
        }
    }
    now = best;   // 进入最优子节点
    return 0;     // 只是选择，没有创建新节点
}

// ====================== MCTS 主循环 ======================

/**
 * 蒙特卡洛树搜索（MCTS）主函数。
 *
 * 每轮迭代包含 4 个步骤：
 *   1. Selection（选择）:    从根节点出发，按 UCB1 公式选最优路径直到叶子节点
 *   2. Expansion（扩展）:    在叶子节点上随机扩展一个新子节点
 *   3. Simulation（模拟）:   用轻量评估函数 judge_value() 估计当前局面
 *   4. Backpropagation（回传）: 将评估结果沿路径回传到根节点
 *
 * 时间限制内不断迭代，最后选择根节点下胜率/访问次数最高的走法。
 */
void MCTS() {
    memset(Node, 0, sizeof(Node));   // 清空 MCTS 树
    now_expanded = 0;
    Node[0].x = -1;
    Node[0].y = -1;                 // 根节点（无落子，坐标为 -1）

    int search_path[100];            // 记录当前迭代的搜索路径，用于回传

    // 预先计算黑白双方在当前局面下的所有合法落子点
    len_store[1] = len_store[0] = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, black)) avail_store[1][len_store[1]++] = i * 9 + j;
            if (put_judge(i, j, white)) avail_store[0][len_store[0]++] = i * 9 + j;
        }

    // 在时间限制内不断迭代
    while (clock() - start < timeout) {
        int i = 0;
        now = 0;
        search_path[0] = 0;

        // 每轮模拟前，从 store 复制可用落子列表
        memcpy(avail, avail_store, sizeof(avail));
        memcpy(len, len_store, sizeof(len));

        // Step 1 + 2: Selection + Expansion
        // 不断向下选择/扩展，直到走到终点或达到深度上限
        while (!end_judge(player) && i <= depth) {
            int unexpand = selection();
            if (unexpand == -1) break;          // 无合法落子
            search_path[++i] = now;              // 记录路径
            chessboard[Node[now].x][Node[now].y] = player;  // 执行落子
            player = -player;
        }

        // Step 3: Simulation（用评估函数快速估值，而非随机模拟到终局）
        double val = judge_value(-player);  // 注意参数是 -player，因为 player 已切换

        // Step 4: Backpropagation（沿路径回传价值）
        for (int j = i; j > 0; j--) {
            Node[search_path[j]].Times++;                // 增加访问次数
            Node[search_path[j]].Value += val;            // 累加价值
            val *= -1;  // 切换视角：父节点的价值 = -子节点的价值（零和博弈）
            chessboard[Node[search_path[j]].x][Node[search_path[j]].y] = 0;  // 撤销落子
        }
        Node[0].Times++;       // 根节点访问次数 +1
        player = player0;      // 恢复当前玩家
    }

    // 从根节点的所有子节点中选出最佳走法
    double max_val = -1;
    int best = 0;
    for (int i = 1; i <= Node[0].NowChildren; i++) {
        int c = Node[0].Children[i];
        double v = Node[c].Value / Node[c].Times;  // 计算平均胜率
        if (v > max_val) {
            max_val = v;
            best = c;
        }
    }
    x = Node[best].x;   // 最终落子坐标
    y = Node[best].y;
}

// ====================== 决策调度 ======================

/**
 * 根据当前棋局状态，选择搜索策略。
 *
 * 策略分两档：
 *   1. 残局（剩余合法落子 ≤ 10）: 使用 Minimax 穷举必胜策略
 *   2. 中前盘（剩余合法落子 > 10）: 使用 MCTS 概率搜索
 *
 * MCTS 深度随空位减少而增大：
 *   > 49 个合法点 → depth = 6
 *   > 36 个合法点 → depth = 8
 *   > 20 个合法点 → depth = 10
 *   ≤ 20 个合法点 → depth = 99（几乎无限深）
 */
void choose() {
    player0 = player;   // 记住当前玩家

    // 统计当前玩家有多少合法落子点
    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            cnt += put_judge(i, j, player);

    // 动态设置 MCTS 深度
    if (cnt > 49) depth = 6;
    else if (cnt > 36) depth = 8;
    else if (cnt > 20) depth = 10;
    else depth = 99;

    // 残局穷举
    if (cnt <= 10 && enumerate(1)) return;

    // 否则使用 MCTS
    MCTS();
}

// ====================== 程序入口 ======================

/**
 * 程序入口函数。
 *
 * 通信协议（Botzone JSON 格式）：
 *   输入（stdin）: 一行 JSON
 *     {
 *       "requests":  [{"x": -1, "y": -1}, ...],  // 历史请求（包括本方和对方）
 *       "responses": [{"x": 4, "y": 4}, ...]      // 我方历史响应
 *     }
 *    注: x=-1, y=-1 表示该回合没有落子（首回合或游戏结束）
 *
 *   输出（stdout）: 一行 JSON
 *     {"response": {"x": 落子x, "y": 落子y}}
 */
int main() {
    srand((unsigned)time(0));      // 初始化随机数种子（MCTS 扩展时需要随机选点）
    start = clock();               // 记录起始时间
    memset(chessboard, 0, sizeof(chessboard));

    // 读取 Botzone 平台发来的 JSON 输入
    string line;
    getline(cin, line);
    Json::Reader reader;
    Json::Value input;
    reader.parse(line, input);

    // 根据历史记录重建棋盘状态
    int turn = input["responses"].size();  // 已完成的回合数
    player = 1;   // 黑棋先手

    // 逐回合恢复：每回合先是对方的 request，然后是我方的 response
    for (int i = 0; i < turn; i++) {
        int ax = input["requests"][i]["x"].asInt();
        int ay = input["requests"][i]["y"].asInt();
        if (ax != -1) { chessboard[ax][ay] = player; player = -player; }

        int rx = input["responses"][i]["x"].asInt();
        int ry = input["responses"][i]["y"].asInt();
        if (rx != -1) { chessboard[rx][ry] = player; player = -player; }
    }

    // 处理当前最新的 request（对方刚下的这一步）
    int fx = input["requests"][turn]["x"].asInt();
    int fy = input["requests"][turn]["y"].asInt();
    if (fx != -1) { chessboard[fx][fy] = player; player = -player; }

    // 执行决策
    choose();

    // 用 jsoncpp 构造输出 JSON 并打印
    Json::Value ret, action;
    action["x"] = x;
    action["y"] = y;
    ret["response"] = action;
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;

    return 0;
}
