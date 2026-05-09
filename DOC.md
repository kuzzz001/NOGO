# 不围棋AI设计与实现思路

## 一、项目概述

不围棋（NoGo）是一种与围棋规则相反的棋类游戏，玩家不能吃掉对方的棋子，最终无法合法落子的一方输掉比赛。

## 二、核心规则理解

1. **棋盘**：9×9标准围棋棋盘
2. **落子**：双方交替在空位落子
3. **禁止**：不能使己方棋子在落子后立即被吃掉（无气）
4. **胜利条件**：对手无法找到合法落子点

## 三、现有代码分析

### 已实现功能
- `judgeAvailable()`：判断某位置是否可落子
- `dfs_air()`：通过DFS检测棋子是否有气
- 随机策略示例代码

### 关键数据结构
```cpp
int board[9][9];        // 棋盘状态：0=空，1=对方，-1=我方
bool dfs_air_visit[][]; // DFS访问标记
```

## 四、AI策略实现方案

### 方案一：贪心策略（入门级）

**思路**：每步选择使己方获得最大收益的位置

```cpp
int evaluateMove(int x, int y, int col) {
    // 评分标准：
    // 1. 落子后己方新增气数
    // 2. 落子后对方减少的气数
    // 3. 位置权重（金角银边）
}
```

**优点**：实现简单、计算快速
**缺点**：只考虑当前一步，无法应对复杂局面

---

### 方案二：极大极小搜索 + Alpha-Beta剪枝（进阶级）

**搜索框架**：
```
当前局面
  ├── 我方落子 → 对方落子 → 我方落子 → ...
  └── 评估各分支得分，选择最优
```

**评估函数设计**：
```cpp
int evaluateBoard(int board[9][9]) {
    int score = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (board[i][j] == myColor)
                score += getLiberties(i, j) + positionWeight[i][j];
            else if (board[i][j] == opponentColor)
                score -= getLiberties(i, j) + positionWeight[i][j];
        }
    }
    return score;
}
```

**优化技巧**：
- **置换表**：缓存已评估的局面
- **迭代加深**：从浅层开始逐步深入
- **走法排序**：优先搜索最有可能的走法

---

### 方案三：蒙特卡洛树搜索（高级）

**核心思想**：
1. 选择：从根节点按UCB1公式选择子节点
2. 扩展：添加新节点
3. 模拟：随机模拟到终局
4. 回溯：更新节点统计信息

```cpp
double ucb1(Node* node) {
    if (node->visits == 0) return INF;
    return node->wins / node->visits 
         + sqrt(2 * log(node->parent->visits) / node->visits);
}
```

---

## 五、关键算法实现

### 5.1 气数计算
```cpp
int countLiberties(int x, int y) {
    memset(dfs_air_visit, 0, sizeof(dfs_air_visit));
    dfs_air(x, y);
    // 统计visited中true的数量即为气数
}
```

### 5.2 位置权重矩阵
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

### 5.3 极大极小搜索伪代码
```cpp
int minimax(int depth, int alpha, int beta, bool maximizing) {
    if (depth == 0) return evaluateBoard();

    if (maximizing) {
        int maxEval = -INF;
        for (每个合法落子位置) {
            int eval = minimax(depth-1, alpha, beta, false);
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha) break; // Alpha-Beta剪枝
        }
        return maxEval;
    } else {
        int minEval = INF;
        for (每个合法落子位置) {
            int eval = minimax(depth-1, alpha, beta, true);
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}
```

## 六、优化建议

1. **减少搜索范围**：利用对称性（旋转、镜像）
2. **历史启发**：记录历史走法避免重复搜索
3. **杀手启发**：优先搜索导致剪枝的走法
4. **评估函数优化**：结合CNN特征或手工特征

## 七、推荐学习路径

| 阶段 | 实现方案 | 难度 |
|------|----------|------|
| 入门 | 贪心策略 + 位置权重 | ★☆☆☆☆ |
| 进阶 | 极大极小 + Alpha-Beta | ★★☆☆☆ |
| 高级 | Monte Carlo树搜索 | ★★★★☆ |

## 八、参考资料

- Botzone不围棋规则：http://www.botzone.org/games#NoGo
- 极大极小算法原理
- Alpha-Beta剪枝优化
- 蒙特卡洛树搜索（MCTS）
