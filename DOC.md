# 不围棋AI设计与实现文档

## 一、项目概述

### 1.1 项目背景
不围棋（NoGo）是一种由围棋衍生的棋类游戏，与围棋规则相反。本项目实现了一个基于C++的不围棋AI程序，用于在北京大学Botzone平台与其他AI对战。

### 1.2 核心特点
- **单文件实现**：便于上传和调试
- **高性能**：符合1秒时间限制要求
- **多策略融合**：结合贪心、极大极小搜索、MCTS等多种算法
- **智能决策**：包含开局库、时间管理、高级评估函数

## 二、游戏规则详解

### 2.1 基本规则
1. **棋盘**：9×9标准围棋棋盘
2. **落子**：双方轮流在空位落子，黑子先手
3. **禁止吃子**：如果落子后吃掉对方棋子，落子方判负
4. **禁止自杀**：如果落子后自身无气，落子方判负
5. **禁止空手**：若无合法落子点，当前方判负
6. **时间限制**：每步限时1秒，超时判负
7. **内存限制**：256MB

### 2.2 核心概念

#### 连（Connection）
棋子和棋子接在一块，成为一个整体。横是连，竖是连，斜线不算连。

#### 气（Liberty）
一个棋子在棋盘上，与它直线紧邻的空点是这个棋子的"气"。棋子直线紧邻的点上：
- 如果有同色棋子存在，则它们相互连接成一个不可分割的整体
- 如果有异色棋子存在，这口气就不复存在
- 如果所有的气均为对方所占据，便呈无气状态

#### 吃子（Capture）
落子后，使对方的棋子变成无气状态就是吃子。**不围棋禁止吃子**。

### 2.3 胜负判定
- 对手无法找到合法落子点 → 胜利
- 对手吃子 → 胜利
- 对手自杀 → 胜利
- 对手超时 → 胜利

## 三、Botzone平台使用指南

### 3.1 平台注册与登录
1. 访问北京大学Botzone平台：https://www.botzone.org.cn/
2. 注册账号并登录

### 3.2 创建Bot
1. 进入个人中心，点击"创建Bot"
2. 配置Bot信息：
   - Bot名称
   - 游戏类型：不围棋（NoGo）
   - 语言选择：C++17
   - 编译选项：`-std=c++17 -O2`

### 3.3 上传代码
1. 将`main.cpp`文件内容粘贴到代码编辑区
2. 点击"保存"并编译测试

### 3.4 创建对局
1. 在平台首页创建游戏桌
2. 指定对手进行对战
3. 支持在线调试和对局回放

### 3.5 调试模式
- 可查看完整对局Log
- 支持切换调试模式
- 可回放对局步骤

## 四、程序编写要点

### 4.1 本地与平台差异

#### 潜在问题
1. **内存问题**：平台上可能暴露出数组越界、地址越界、除零、未初始化等问题
2. **随机数不一致**：本地与平台生成的随机数可能不同

#### 建议
- 使用TScanCode工具检查代码
- 避免依赖特定随机数序列

### 4.2 超时与卡时（关键）

比赛时Bot会在评测机集群运行，平台高负载可能导致程序变慢，引发超时。

**推荐卡时代码**：

```cpp
int threshold = 0.95 * (double)CLOCKS_PER_SEC;
int start_time, current_time;
start_time = current_time = clock();
while (current_time - start_time < threshold) {
    // Monte Carlo Tree Search...
    current_time = clock();
}
```

**要点**：
- 不要固定循环次数
- 每次循环通过`clock()`判断已运行时间
- 接近1秒时退出循环
- 预留5%的时间余量（0.95倍阈值）

## 五、技术架构

### 5.1 数据结构

#### 棋盘表示
```cpp
int board[9][9];  // 0=空白，1=对方，-1=我方
```

#### 坐标系统
- 左上角为原点(0, 0)
- x轴：向下递增（0-8）
- y轴：向右递增（0-8）

#### 方向数组
```cpp
const int cx[] = {-1, 1, 0, 0};  // 上下左右
const int cy[] = {0, 0, -1, 1};
```

### 3.2 核心算法

#### 算法演进历程

| 版本 | 算法 | 特点 |
|------|------|------|
| v0.1.0 | 贪心策略 | 位置权重 + 气数评估 |
| v0.2.0 | 极大极小搜索 | Alpha-Beta剪枝 |
| v0.3.0 | MCTS | 蒙特卡洛树搜索 |
| v0.4.0 | 优化版本 | 置换表 + 历史启发 |
| v0.5.0 | 高级优化 | MCTS扩展优化 |
| v0.6.0 | 智能决策 | 时间管理 + 开局库 |
| v0.6.1 | 性能优化 | 符合1秒时间限制 |

## 四、算法详解

### 4.1 贪心策略（基础评估）

#### 评估函数
```cpp
int evaluateBoard(int b[9][9], int player) {
    int score = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (b[i][j] == player) {
                score += countLiberties(i, j, b);
                score += positionWeight[i][j];
            } else if (b[i][j] == -player) {
                score -= countLiberties(i, j, b);
                score -= positionWeight[i][j];
            }
        }
    }
    return score;
}
```

#### 位置权重矩阵
```cpp
const int positionWeight[9][9] = {
    {0,  0,  1,  2,  3,  2,  1,  0,  0},
    {0,  1,  3,  4,  5,  4,  3,  1,  0},
    {1,  3,  5,  6,  7,  6,  5,  3,  1},
    {2,  4,  6,  8,  9,  8,  6,  4,  2},
    {3,  5,  7,  9, 10,  9,  7,  5,  3},
    {2,  4,  6,  8,  9,  8,  6,  4,  2},
    {1,  3,  5,  6,  7,  6,  5,  3,  1},
    {0,  1,  3,  4,  5,  4,  3,  1,  0},
    {0,  0,  1,  2,  3,  2,  1,  0,  0}
};
```

### 4.2 极大极小搜索 + Alpha-Beta剪枝

#### 算法原理
```
当前局面
  ├── 我方落子 → 对方落子 → 我方落子 → ...
  └── 评估各分支得分，选择最优
```

#### 伪代码
```cpp
int minimax(int b[9][9], int depth, int alpha, int beta, 
             int player, int originalPlayer) {
    if (depth == 0) return evaluateBoard(b, originalPlayer);
    
    vector<pair<int, int>> moves = getValidMoves(b, player);
    if (moves.empty()) return player == originalPlayer ? -10000 : 10000;
    
    if (player == originalPlayer) {
        int maxEval = -10000;
        for (auto& move : moves) {
            int x = move.first, y = move.second;
            b[x][y] = player;
            int eval = minimax(b, depth - 1, alpha, beta, -player, originalPlayer);
            b[x][y] = 0;
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha) break;  // Alpha-Beta剪枝
        }
        return maxEval;
    } else {
        int minEval = 10000;
        for (auto& move : moves) {
            int x = move.first, y = move.second;
            b[x][y] = player;
            int eval = minimax(b, depth - 1, alpha, beta, -player, originalPlayer);
            b[x][y] = 0;
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}
```

#### 优化技术
- **置换表**：缓存已评估的局面
- **历史启发**：记录历史走法，优先搜索
- **迭代加深**：从浅层开始逐步深入

### 4.3 蒙特卡洛树搜索（MCTS）

#### 核心思想
1. **选择**：从根节点按UCB1公式选择子节点
2. **扩展**：添加新节点
3. **模拟**：随机模拟到终局
4. **回溯**：更新节点统计信息

#### UCB1公式
```cpp
double ucb1(MCTSNode* node, double explorationParam = 1.414) {
    if (node->visits == 0) return 1000000.0;
    return (double)node->wins / node->visits 
         + explorationParam * sqrt(log(node->parent->visits) / node->visits);
}
```

#### MCTS流程
```cpp
pair<int, int> mctsSearch(int b[9][9], int player, int simulations) {
    MCTSNode* root = new MCTSNode(-1, -1, nullptr);
    expand(root, b, player);
    
    for (int i = 0; i < simulations; i++) {
        MCTSNode* node = select(root);
        expand(node, b, player);
        int result = simulate(b, player, player);
        backpropagate(node, result);
    }
    
    MCTSNode* bestChild = selectBestChild(root);
    updateHistoryTable(bestChild->x, bestChild->y, 3);
    
    pair<int, int> result = make_pair(bestChild->x, bestChild->y);
    delete root;
    return result;
}
```

## 五、高级功能

### 5.1 时间管理

#### 动态调整策略
```cpp
int calculateSimulations(double timeLeft, int moveCount) {
    if (timeLeft > 0.5) return 500;      // 充足时间
    else if (timeLeft > 0.3) return 300;  // 正常时间
    else if (timeLeft > 0.1) return 150;  // 时间紧张
    else return 80;                       // 非常紧张
}
```

#### 性能指标
- 运行时间：0.217秒（远低于1秒限制）
- 模拟次数：500次（平衡性能与质量）

### 5.2 开局库

#### 实现原理
```cpp
map<int, pair<int, int>> openingBook;

void initOpeningBook() {
    openingBook[0] = {4, 4};  // 空棋盘 → 中心
    openingBook[1] = {3, 4};  // 中心被占 → 附近
    openingBook[2] = {4, 3};
    openingBook[3] = {5, 4};
    openingBook[4] = {4, 5};
}
```

#### 优势
- 开局速度提升90%+
- 减少不必要的计算
- 提供开局最优解

### 5.3 高级评估函数

#### 多维度评估
```cpp
int evaluateBoardAdvanced(int b[9][9], int player) {
    int score = 0;
    
    score += evaluateBoard(b, player);           // 基础评估
    score += evaluateConnectivity(b, player) * 5; // 连通性
    score += evaluateThreats(b, player) * 3;      // 威胁度
    score += evaluateFlexibility(b, player) * 2;  // 灵活性
    
    return score;
}
```

#### 评估维度

| 维度 | 权重 | 说明 |
|------|------|------|
| 基础评估 | ×1 | 气数 + 位置权重 |
| 连通性 | ×5 | 大连通块更有优势 |
| 威胁度 | ×3 | 气少的棋子威胁更大 |
| 灵活性 | ×2 | 可选位置越多越好 |

## 六、优化技术

### 6.1 置换表（Transposition Table）
```cpp
unordered_map<size_t, pair<int, int>> transpositionTable;

size_t getBoardHash(int b[9][9]) {
    size_t hash = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            hash ^= (size_t)b[i][j] << (i * 9 + j);
        }
    }
    return hash;
}
```

### 6.2 历史启发（History Heuristic）
```cpp
int historyTable[9][9] = {0};

void updateHistoryTable(int x, int y, int bonus) {
    historyTable[x][y] += bonus;
}

int getHistoryScore(int x, int y) {
    return historyTable[x][y];
}
```

### 6.3 编译优化
```bash
g++ -std=c++17 -O2 main.cpp -o nogo
```

## 九、Botzone平台对接

### 7.1 输入格式
```cpp
cin >> n;  // 对局步数
for (int i = 0; i < n - 1; i++) {
    cin >> x >> y; if (x != -1) board[x][y] = 1;  // 对方
    cin >> x >> y; if (x != -1) board[x][y] = -1; // 我方
}
cin >> x >> y; if (x != -1) board[x][y] = 1;  // 对方最新一步
```

### 7.2 输出格式
```cpp
cout << new_x << ' ' << new_y << endl;
```

### 7.3 平台要求
- ✅ 单文件实现
- ✅ C++17标准
- ✅ 1秒时间限制
- ✅ 256MB内存限制
- ✅ 标准输入/输出

## 十、性能测试

### 8.1 时间性能
| 测试场景 | 运行时间 | 状态 |
|----------|----------|------|
| 空棋盘开局 | 0.217秒 | ✅ |
| 中局复杂局面 | 0.3-0.5秒 | ✅ |
| 残局简单局面 | 0.1-0.2秒 | ✅ |

### 8.2 内存占用
| 项目 | 大小 |
|------|------|
| 棋盘数组 | 324字节 |
| 置换表 | ~10MB |
| MCTS树 | ~5MB |
| 其他数据结构 | ~1MB |
| **总计** | **~16MB** |

## 十一、版本历史

### v0.6.1 - 性能优化版本（当前）
- 修复时间限制问题（10秒 → 1秒）
- 优化模拟次数（800次 → 500次）
- 添加编译优化（-O2）
- 运行时间：0.217秒

### v0.6.0 - 智能决策版本
- 添加时间管理功能
- 实现开局库
- 改进评估函数（连通性、威胁度、灵活性）

### v0.5.0 - 高级优化版本
- 添加置换表缓存
- 实现历史启发优化
- MCTS扩展阶段应用历史启发

### v0.4.0 - MCTS版本
- 实现蒙特卡洛树搜索
- UCB1选择策略
- 随机模拟与回溯

### v0.3.0 - 搜索版本
- 实现极大极小搜索
- Alpha-Beta剪枝优化
- 置换表缓存

### v0.2.0 - 贪心版本
- 实现贪心策略
- 位置权重评估
- 气数计算

### v0.1.0 - 基础版本
- 基础棋盘管理
- 合法落子判断
- 气数计算

## 十二、未来规划

### v0.7.0 - 性能优化版本
- [ ] 并行化模拟（多线程MCTS）
- [ ] 动态调整探索参数
- [ ] 进一步优化评估函数

### v0.8.0 - 高级功能版本
- [ ] 置换表优化（更大容量）
- [ ] 迭代加深搜索
- [ ] 残局数据库

## 十三、参考资料

- Botzone不围棋规则：https://www.botzone.org.cn/games#NoGo
- 极大极小算法原理
- Alpha-Beta剪枝优化
- 蒙特卡洛树搜索（MCTS）
- UCB1公式与探索-利用平衡

## 十四、编译与运行

### 编译命令
```bash
g++ -std=c++17 -O2 main.cpp -o nogo
```

### 本地测试
```bash
echo -e "1\n-1 -1" | ./nogo
```

### 上传Botzone
1. 登录Botzone平台
2. 创建不围棋Bot
3. 上传main.cpp
4. 开始对战

---

**文档版本**：v2.0  
**最后更新**：2026-05-09  
**作者**：kuzzz
