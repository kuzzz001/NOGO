# 不围棋（NoGo）博弈算法技术综述

## 摘要

不围棋（NoGo）作为一种源于围棋但规则完全相反的新兴博弈游戏，近年来在人工智能领域受到越来越多的关注。本文系统综述了 NoGo 博弈算法的最新研究进展，重点聚焦技术实现层面的深入分析。研究表明，NoGo 具有与围棋相似的棋盘结构（9×9），但胜负判定规则完全相反：玩家落子后若吃掉对方棋子或导致自杀则判负，且禁止弃权。当前主流的 NoGo 算法主要包括**优化 UCT 算法**、**基于价值评估的递归算法**、**AlphaZero 架构**以及**基于规则的基础算法**。其中，基于 UCT 的算法在实际应用中表现出色，与 OASE-NoGo 对弈可达到 90%-94% 的胜率。最新的研究成果显示，通过引入深度学习技术和并行优化策略，算法性能得到显著提升，如 NoGoZero + 相比原始 AlphaZero 训练速度提升 6 倍。本文还分析了各种算法的技术特点、性能对比以及在实际项目开发中的应用建议，为 NoGo 博弈系统的设计与实现提供了全面的技术参考。

## 引言

不围棋（NoGo）是一种在 2005 年左右诞生的新兴博弈游戏，其规则设计与传统围棋形成鲜明对比。与围棋追求围空占地不同，NoGo 的核心目标是避免吃掉对方棋子或自杀，一旦出现这些情况即判负。这种规则的颠覆性变化使得 NoGo 成为测试人工智能算法的理想平台，因为它既保持了围棋的复杂性，又需要全新的策略思维。

从博弈论的角度来看，NoGo 属于两人零和确定性博弈，具有完美信息特征。棋盘采用 9×9 规格，黑子先行，双方轮流落子，棋子一旦落下不可移动。游戏的状态空间复杂度约为 10^76，与 9×9 围棋相当，但由于规则的特殊性，传统的围棋算法无法直接应用。这为人工智能研究者提供了一个独特的挑战：如何设计专门针对 NoGo 规则的高效算法。

近年来，随着深度学习和强化学习技术的快速发展，NoGo 博弈算法取得了突破性进展。从最初基于蒙特卡洛树搜索（MCTS）的传统算法，到融入神经网络的 AlphaZero 架构，再到最新的混合算法和并行优化策略，算法性能不断提升。特别是在 2020 年中国计算机博弈锦标赛（CCGC）中，基于 NoGoZero + 的参赛程序获得亚军，击败了众多基于 AlphaZero 的参赛队伍，充分证明了算法创新的价值。

本文旨在全面综述 NoGo 博弈算法的技术实现与研究进展，为相关领域的研究人员和开发人员提供系统性的技术参考。文章将从算法原理、技术实现、性能对比、优化策略以及实际应用等多个维度进行深入分析，重点关注算法的可操作性和工程实践价值。

## 一、NoGo 基础理论与规则体系

### 1.1 标准规则定义

不围棋的规则体系虽然简洁，但与传统棋类游戏存在根本性差异。根据全国计算机博弈大赛的标准规则，NoGo 具有以下核心特征：

**棋盘规格与落子规则**：NoGo 使用 9×9 的标准围棋棋盘，采用国际标准坐标系统，行号为数字 1-9，列号为大写英文字母 A-I[(35)](http://jyt.ah.gov.cn/tsdw/gdjyc/dxsxkhjnjs/40723621.html)。游戏由黑子先行，双方轮流在空交叉点落子，落子后棋子不可移动。这种落子方式与围棋相同，但后续规则却截然不同。

**胜负判定机制**：NoGo 的胜负判定规则是其最独特的设计，与围棋完全相反。根据规则，以下情况将导致落子方判负：



* 如果一方落子后吃掉了对方的棋子，则落子一方判负

* 如果一方落子后导致己方自杀（棋子无气），则落子一方判负

* 如果一方选择弃权（Pass），则判负

* 每方用时 15 分钟，超时判负

值得注意的是，NoGo 游戏没有和棋，必须分出胜负。这种规则设计使得游戏过程充满戏剧性，每一步都可能成为决定胜负的关键。

**气与棋块的概念**：与围棋类似，NoGo 也使用 "气" 和 "棋块" 的概念。气是指棋子或连通棋块周围的空白点，棋块是指通过直线相邻连接的同色棋子集合。在 NoGo 中，每个棋块必须保持至少一个气，否则将导致自杀判负。这种设计使得玩家必须时刻关注自己棋块的生存状态，同时避免形成可能导致对方被吃的局面。

### 1.2 游戏特征与复杂度分析

NoGo 作为一个博弈游戏，具有独特的数学特征和计算复杂度。从博弈论的角度分析，NoGo 是一个典型的**PSPACE 完全问题**，这意味着其计算复杂度与国际象棋、围棋等经典博弈游戏处于同一级别。

**状态空间复杂度**：NoGo 的状态空间复杂度约为 10^76，与 9×9 围棋相当。这是因为每个交叉点有三种可能的状态（空、黑、白），总共有 9×9=81 个交叉点，理论上的状态总数为 3^81≈10^38。但由于规则限制，许多状态是不可能出现的（如导致自杀或吃子的状态），实际可到达的状态数约为 10^76。

**游戏深度**：NoGo 的最大游戏深度为 81 步（当棋盘完全填满时），但由于规则限制，实际游戏通常在 60-70 步内结束。研究表明，即使在 7×7 的较小棋盘上，游戏也比预期的更深，远非任何一方可以立即获胜的简单游戏。

**策略复杂度**：与围棋不同，NoGo 没有成熟的人类策略体系。由于其规则的新颖性和极其有限的背景研究，NoGo 缺乏像围棋那样经过数千年发展的策略理论。这既是挑战也是机遇，因为计算机程序可以通过自我对弈学习来探索最优策略，而不受人类经验的限制。

### 1.3 关键概念与术语

为了更好地理解后续算法，需要明确 NoGo 中的几个关键概念：

**禁入点（Forbidden Points）**：禁入点是指落子后会导致对方棋块无气（被吃）的点。在 NoGo 中，落子到禁入点将直接判负。禁入点的识别是算法设计中的核心问题之一，需要通过气的计算来判断。

**眼（Eye）**：眼是被同色棋子完全包围的单个空白点。在围棋中，两个真眼可以保证棋块的生存，但在 NoGo 中，眼的概念变得复杂。一方面，眼可以为己方棋块提供气；另一方面，如果眼被对方占据，可能导致己方棋块自杀。

**虎口（Tiger's Mouth）**：虎口是指落子后气数为 1 的点。在 NoGo 中，虎口是一个危险区域，因为任何一方落子于此都可能导致自杀。因此，识别和利用虎口是重要的策略之一。

**权利值（Right Value）**：权利值是指对方不可落子点的总数，是评估当前局面优劣的重要指标[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)。权利值越大，说明对方的选择越少，己方的优势越大。这一概念在基于价值评估的算法中起着关键作用。

## 二、主流算法技术实现

### 2.1 优化 UCT 算法

UCT（Upper Confidence bounds applied to Trees）算法是蒙特卡洛树搜索（MCTS）的核心变体，在 NoGo 博弈中得到了广泛应用和优化。

#### 2.1.1 算法原理与改进

UCT 算法最初用于计算机围棋，通过将 UCB（Upper Confidence Bound）算法应用于树结构来实现。在 NoGo 中，UCT 算法的基本结构保持不变，但在具体实现上进行了多项改进。

UCT 算法包含四个循环步骤：



1. **选择（Selection）**：根据节点的上置信界值，持续选择具有最大上置信界的节点，直到到达叶子节点

2. **扩展（Expansion）**：生成所选节点的所有子节点并添加到树中

3. **模拟（Simulation）**：从所选叶子节点开始进行蒙特卡洛模拟

4. **反馈（Backpropagation）**：将模拟结果反向传播给父节点

在 NoGo 的应用中，研究人员提出了 \*\* 动态移动队列（Dynamic Move Queue）\*\* 的创新设计。这一改进使得程序执行的 UCB 下降次数增加了近 4 倍，从原来的 5 万次增加到 20 万次，显著提升了搜索效率。

#### 2.1.2 盘面评估函数

UCT 算法的性能很大程度上依赖于盘面评估函数的设计。在 NoGo 中，研究人员提出了一个综合考虑多种因素的评估函数：

**盘面评估函数**：

$S(N) = n_1 s_1 + n_2 s_2 + n_3 s_3$

其中，$n_1$为眼数，$n_2$为虎口数，$n_3$为禁入点数，$s_1, s_2 > 0$，$s_3 < 0$。

这一评估函数的设计理念是：



* 眼数（$n_1$）：眼可以为己方棋块提供气，是有利因素，权重$s_1 = 10.0$

* 虎口数（$n_2$）：虎口虽然危险，但也是控制对手的重要手段，权重$s_2 = 3.0$

* 禁入点数（$n_3$）：禁入点会限制己方的选择，是不利因素，权重$s_3 = -5.0$

通过这种加权求和的方式，算法能够快速评估当前盘面的优劣，指导搜索方向。

#### 2.1.3 知识数据库的构建

为了进一步提升性能，研究人员还提出了构建 NoGo 知识数据库的方法。知识数据库的核心是**移动置信度（Move Confidence Degree, MCD）**：

$MCD = \alpha \cdot v_i / V + winrate$

其中，$\alpha$是常数，$v_i$是当前节点的访问次数，$V$是总访问次数，$winrate$是当前节点的胜率。只有当移动的 MCD 值超过给定标准时，才将其写入知识库。

这种设计的优势在于：



* 避免了存储整个游戏树带来的巨大空间开销

* 只保存高置信度的移动，提高了数据质量

* 可以在不同对局中复用，加速学习过程

#### 2.1.4 实验结果与性能分析

优化 UCT 算法在实际应用中取得了优异的成绩。根据实验数据，该算法与 OASE-NoGo 对弈的结果为：



* 初级难度：100 局胜 90 局，胜率 90%

* 高级难度：50 局胜 47 局，胜率 94%

在资源使用方面，该算法的表现也很出色。使用 Intel Core i7-3770 CPU（3.4GHz），内存 8GB 的配置，算法能够在合理时间内完成搜索。通过动态移动队列的优化，算法的执行效率提升了 4 倍，使得在相同时间内能够进行更多的模拟和搜索。

### 2.2 基于价值评估的递归算法

基于价值评估的递归算法是一种专门针对 NoGo 设计的高效算法，其核心思想是通过计算 "权利值" 来评估盘面并进行递归搜索。

#### 2.2.1 权利值计算方法

权利值（Right Value）是该算法的核心概念，定义为对方不可落子点的总数[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)。具体计算方法如下：

**权利值计算公式**：

$V(p, q) = \sum N_w + \sum N_b$

其中，$N_w$为白方不可落子点，$N_b$为黑方不可落子点。

这一概念的创新之处在于，它不仅考虑了己方的优势，还同时考虑了对方的劣势。权利值越大，说明对方的选择越少，己方的优势越明显。

在实际计算中，权利值的计算需要考虑多种因素：



* 禁入点：落子后会导致对方被吃的点

* 自杀点：落子后会导致己方自杀的点

* 被阻挡的点：由于周围棋子布局而无法落子的点

#### 2.2.2 递归搜索策略

递归算法采用深度优先搜索策略，其基本流程如下[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)：



1. **计算所有合法点的价值**：对每个可能的落子点，计算其权利值

2. **选择价值最大的候选点**：在所有合法点中，选择权利值最大的点作为候选

3. **递归多层评估**：对候选点进行多层递归评估，确保选择的是真正的最优解

4. **价值相同时的处理**：当多个点价值相同时，使用曼哈顿距离打散规则选择

递归深度的选择是算法性能的关键。研究表明，在 9×9 的 NoGo 棋盘上，递归深度设置为 3-5 层时，算法在计算速度和决策质量之间达到了良好的平衡。

#### 2.2.3 曼哈顿距离打散规则

当多个候选点的权利值相同时，算法采用曼哈顿距离打散规则进行选择[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)。该规则的具体步骤如下：



1. 计算候选点与所有已有棋子的曼哈顿距离

2. 找出每个候选点的最小曼哈顿距离

3. 选择最小曼哈顿距离最大的候选点

4. 如果仍有多个选择，则随机选择

这种设计的逻辑是：选择与已有棋子距离较远的点，可以为后续的布局提供更大的灵活性，避免过早地将自己限制在局部区域。

#### 2.2.4 性能表现与优势分析

基于价值评估的递归算法在实际应用中展现出了显著优势：

**性能优势**：



* 低配置设备响应时间小于 2 秒[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)

* 时间和空间复杂度远低于 MCTS 算法

* 与 OASE-NoGo 高级难度对弈 200 局，胜率达到 92.5%[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)

**技术优势**：



* 算法逻辑相对简单，易于实现和调试

* 不需要大量的计算资源，适合嵌入式设备

* 决策速度快，适合实时对弈场景

* 可解释性强，每个决策都有明确的价值依据

### 2.3 AlphaZero 架构及其变体

AlphaZero 架构在 NoGo 领域的应用代表了当前技术的最高水平。通过将深度神经网络与蒙特卡洛树搜索相结合，AlphaZero 能够通过自我对弈学习掌握最优策略。

#### 2.3.1 基础 AlphaZero 架构

AlphaZero 在 NoGo 中的应用保持了其核心架构：



* **神经网络**：用于预测策略和评估价值

* **蒙特卡洛树搜索（MCTS）**：用于引导搜索过程

* **自我对弈**：通过与自己对弈生成训练数据

在 NoGo 的具体实现中，神经网络的输入设计考虑了游戏的特殊性。输入特征包括：



* 当前棋局状态（9×9 网格）

* 黑子位置（9×9 网格）

* 白子位置（9×9 网格）

* 当前行棋方标识（1×1）

这种设计使得神经网络能够充分捕捉棋局的空间特征和当前状态。

#### 2.3.2 NoGoZero + 的创新改进

在基础 AlphaZero 的基础上，研究人员提出了 NoGoZero+，通过多项创新技术显著提升了性能：

**1. 全局注意力残差块（Global Attention Residual Block）**

传统的卷积神经网络在捕捉全局信息方面存在局限。NoGoZero + 引入了全局注意力机制，使得模型能够更好地理解棋盘的整体布局。这一改进特别针对 NoGo 中经常出现的长距离相互作用场景。

**2. 网络课程学习（Network Curriculum Learning）**

NoGoZero + 采用了渐进式的网络训练策略：



* 第一阶段：使用 5 个残差块，32 个通道

* 第二阶段：使用 10 个残差块，64 个通道

* 第三阶段：使用 20 个残差块，128 个通道

这种设计的优势在于：



* 避免了训练初期的梯度消失问题

* 逐步增加模型复杂度，提高收敛速度

* 每个阶段都能产生高质量的训练数据

**3. 高级特定特征（Advanced Specific Features）**

NoGoZero + 还引入了针对 NoGo 规则设计的特定优化：



* 专门的禁入点识别模块

* 自杀风险评估机制

* 长距离威胁检测算法

这些特征充分利用了 NoGo 规则的特殊性，提升了模型的决策质量。

#### 2.3.3 实验结果与性能对比

NoGoZero + 在多个维度上都取得了突破性进展：

**训练效率对比**：



* 原始 AlphaZero：需要 12 万局自我对弈才能达到 2500 Elo

* NoGoZero+：仅需 2 万局自我对弈即可达到 2750 Elo

* 训练速度提升约 6 倍

**对弈性能对比**：

NoGoZero + 与原始 AlphaZero 进行 100 局对弈，结果为 81 胜 19 负，胜率达到 81%。这一结果充分证明了改进策略的有效性。

**计算资源需求**：

实验使用 NVIDIA Tesla V100 GPU（32GB 显存），采用随机梯度下降（SGD）优化器，学习率 0.001，动量 0.8[(68)](https://mdpi-res.com/d_attachment/electronics/electronics-10-01533/article_deploy/electronics-10-01533-v2.pdf?version=1624855346)。相比原始 AlphaZero 需要数千个 TPU 的配置，NoGoZero + 在资源需求上有了显著降低。

#### 2.3.4 表格化 AlphaZero 的探索

除了深度神经网络版本，研究人员还探索了表格化 AlphaZero 在 NoGo 中的应用。这种方法使用查找表代替深度神经网络，具有以下特点：

**技术特点**：



* 每个位置都有唯一的表项，包含策略和价值

* 策略使用 softmax 函数计算概率分布

* 价值使用 sigmoid 函数缩放到 \[0,1] 区间

**实验发现**：

研究表明，表格化 AlphaZero 在许多超参数设置下都能学习到理论值和最优策略。特别是在小规模棋盘（如 2×6、1×14）上，该方法能够快速收敛到最优解。

### 2.4 基于规则的基础算法

基于规则的算法是 NoGo 博弈中最基础也是最直接的方法，通过实现游戏规则和基本策略来进行决策。

#### 2.4.1 核心算法设计

基础算法的核心是实现以下功能：



* **同色邻接入栈算法**：使用深度优先搜索（DFS）遍历连通块

* **气计算算法**：计算棋块的气数

* **着法可行性判定算法**：判断落子是否合法

**DFS 遍历算法**：

该算法用于查找包含给定点的棋块，其实现步骤如下：



1. 初始化访问集合和栈

2. 将起始点压入栈并标记为已访问

3. 循环处理栈顶元素，检查其四个方向的邻居

4. 如果邻居是同色且未访问过，将其压入栈

5. 统计所有访问过的点，形成棋块

**气计算算法**：

气的计算是判断合法落子的关键。算法步骤：



1. 获取棋块的所有坐标

2. 遍历每个坐标的四个方向邻居

3. 统计空白点的数量

4. 考虑边界情况的特殊处理

#### 2.4.2 着法合法性判定

着法合法性判定是基础算法的核心，必须严格按照规则实现：

**判定步骤**：



1. 检查落子点是否为空

2. 模拟落子，检查是否导致对方被吃

3. 检查是否导致己方自杀

4. 确保没有违反任何规则

这种判定方法的优势是逻辑清晰、易于实现，但缺点是计算复杂度较高，特别是在棋盘较满时需要检查大量情况。

#### 2.4.3 行棋策略设计

基础算法还包含了一些基本的行棋策略：



* 先占角，再占边，最后占中腹

* 优先做眼、破坏对方眼位

* 多眼连通保证自身可落子

这些策略虽然简单，但在实际应用中能够产生不错的效果，特别是对于初学者或资源受限的场景。

#### 2.4.4 性能与应用场景

基于规则的算法在不同场景下表现出不同的特点：

**性能特点**：



* 算法简单，易于实现和调试

* 计算资源需求低，适合各种设备

* 决策速度快，适合实时交互

* 可解释性强，每个决策都有明确依据

**应用场景**：



* 教学演示：帮助初学者理解游戏规则

* 轻量级应用：在移动设备或嵌入式系统上运行

* 算法验证：作为基准算法测试其他方法的性能

* 人机对战：为人类玩家提供基础级别的 AI 对手

## 三、算法性能对比与优化策略

### 3.1 主要算法性能对比

通过对各种 NoGo 算法的深入分析，可以从多个维度进行性能对比：

**胜率对比**：

不同算法与 OASE-NoGo 对弈的胜率表现如下：



* 优化 UCT 算法：初级 90%，高级 94%

* 价值递归算法：高级难度 92.5%[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)

* AlphaZero 架构：通过自我对弈学习，最终可达 2750 Elo，远超传统算法

**训练效率对比**：

在训练速度方面，NoGoZero + 相比原始 AlphaZero 取得了显著改进：



* 原始 AlphaZero：12 万局达到 2500 Elo

* NoGoZero+：2 万局达到 2750 Elo

* 速度提升：约 6 倍

**计算资源需求**：

不同算法的资源需求差异巨大：



* 基础规则算法：仅需普通 CPU，内存需求极低

* UCT 算法：需要多核 CPU，内存随搜索深度增长

* AlphaZero：需要 GPU 支持，原始版本需要数千 TPU

* NoGoZero+：仅需单个 V100 GPU 即可完成训练[(68)](https://mdpi-res.com/d_attachment/electronics/electronics-10-01533/article_deploy/electronics-10-01533-v2.pdf?version=1624855346)

**决策速度**：



* 价值递归算法：低配置设备响应时间 < 2 秒[(66)](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)

* UCT 算法：根据模拟次数不同，通常在 1-10 秒

* AlphaZero：单次推理约 10-100 毫秒，但需要多次搜索

### 3.2 计算资源需求分析

算法的资源需求直接影响其实际应用。以下是主要算法的资源需求详细分析：

**UCT 算法资源需求**：

UCT 算法的资源消耗主要来自两个方面：



1. **内存占用**：每个节点需要存储访问次数、胜率、子节点指针等信息。根据实验数据，使用 "延迟节点创建" 策略可以显著减少内存使用。例如，在 10000 次模拟 / 步的情况下，内存使用从 61.9MB 减少到 15.5MB。

2. **CPU 时间**：主要消耗在树搜索和模拟上。通过动态移动队列优化，UCB 下降次数增加 4 倍，但总时间增加不到 1 倍，说明优化是有效的。

**AlphaZero 架构资源需求**：

AlphaZero 的资源需求包括：



1. **GPU 显存**：神经网络的参数存储和前向传播需要大量显存。NoGoZero + 使用的 ResNet-20-128 模型大约需要 10-15GB 显存。

2. **计算时间**：每局自我对弈需要进行数千次 MCTS 搜索，每次搜索需要多次神经网络推理。使用 GPU 加速可以将单次推理时间从 CPU 的 100 毫秒降低到 1 毫秒以下。

3. **存储需求**：训练过程产生的自我对弈数据需要大量存储空间。NoGoZero + 提供的数据集包含约 16 万局游戏，压缩后约 50GB。

### 3.3 优化策略深度分析

为了提升算法性能，研究人员提出了多种优化策略：

**1. 并行化策略**

传统的 UCT 算法基于蒙特卡洛树搜索，只能在单线程上运行，无法充分利用多核 CPU 性能。针对这一问题，研究人员提出了多种并行化方案：



* **根并行化（Root Parallelization）**：并行构建独立的游戏树，基于所有树的根级分支进行移动决策。这种方法简单有效，但可能导致资源浪费。

* **深度并行化**：在搜索树的不同深度并行进行搜索。这种方法需要更复杂的同步机制，但可以更充分地利用计算资源。

* **GPU 并行化**：将部分计算密集型任务（如神经网络推理、模拟评估）移植到 GPU 上。现代 GPU 具有数千个处理核心，能够显著加速这些任务。

**2. 剪枝策略**

剪枝是减少搜索空间的有效方法：



* **基于价值的剪枝**：当某个分支的价值明显低于当前最优值时，可以提前剪枝

* **基于概率的剪枝**：当某个移动的选择概率低于阈值时，可以忽略

* **基于规则的剪枝**：利用 NoGo 的特殊规则，快速识别不可能的移动

**3. 缓存优化**

缓存是提升算法效率的重要手段：



* **棋局状态缓存**：使用置换表（Transposition Table）存储已计算过的棋局状态

* **评估函数结果缓存**：缓存常用位置的评估结果

* **移动序列缓存**：缓存常见开局和中局的最优移动序列

**4. 自适应策略**

自适应策略能够根据游戏进程动态调整算法参数：



* **动态模拟次数**：在简单局面减少模拟次数，在复杂局面增加模拟次数

* **自适应探索参数**：根据搜索状态动态调整 UCT 中的探索参数 C

* **学习率调度**：在训练过程中动态调整学习率

### 3.4 算法可扩展性分析

算法的可扩展性对于实际应用至关重要：

**棋盘大小扩展**：

不同算法对棋盘大小的适应性不同：



* UCT 算法：随着棋盘增大，搜索空间呈指数级增长，扩展性较差

* AlphaZero：通过神经网络的泛化能力，能够较好地适应不同棋盘大小

* 价值递归算法：计算复杂度与棋盘大小呈多项式关系，扩展性较好

**规则变体适应**：

NoGo 存在多种变体，如线性 NoGo（1×n）、三维 NoGo 等。算法的适应性：



* UCT 算法：需要针对新规则重新设计评估函数

* AlphaZero：通过自我对弈可以学习新规则

* 价值递归算法：需要重新设计价值计算方法

**硬件平台适应**：

算法在不同硬件平台上的表现：



* CPU 平台：所有算法都能运行，但性能差异大

* GPU 平台：AlphaZero 类算法受益最大

* 嵌入式平台：价值递归和基础算法更适合

## 四、实际应用与项目开发建议

### 4.1 竞赛表现与案例分析

NoGo 在中国计算机博弈大赛中已成为正式比赛项目，各参赛队伍的表现为算法应用提供了宝贵经验。

**获奖案例分析**：

在 2020 年中国计算机博弈锦标赛（CCGC）中，基于 NoGoZero + 的参赛程序获得亚军，击败了众多基于 AlphaZero 的参赛队伍。这一成绩充分证明了算法创新的价值。

中南大学的参赛作品表现突出，其 "宇梦不围棋" 获得三等奖[(104)](https://news.csu.edu.cn/info/1002/123923.htm)。作为新参加的软件，该作品研发时间最短却在比赛中显示出对棋种的深刻理解，获得了不错的成绩。

安徽财经大学在 2025 年的比赛中表现优异，不围棋项目获得二等奖[(101)](https://smse.aufe.edu.cn/2025/1009/c1791a239719/page.htm)。该校从省赛获奖队伍中遴选出 11 支队伍参赛，在 8 个棋类项目中展现了出色的技术实力。

**技术路线对比**：

通过分析获奖作品，可以总结出以下技术特点：



1. **深度学习路线**：基于 AlphaZero 或其变体，需要大量计算资源和训练时间

2. **传统算法路线**：基于 UCT 或其他搜索算法，更注重工程优化

3. **混合路线**：结合深度学习和传统算法，试图平衡性能和效率

### 4.2 项目开发技术路线建议

基于对各种算法的分析，针对不同需求提出以下技术路线建议：

**入门级项目（教学演示）**：

推荐使用基于规则的基础算法。理由如下：



* 算法简单，易于实现和理解

* 资源需求低，普通 PC 即可运行

* 可解释性强，适合教学用途

* 开发周期短，可快速完成原型

实现建议：



1. 优先实现核心规则（气计算、合法性判定）

2. 逐步添加基础策略（占角、做眼等）

3. 开发图形界面，提供良好的用户体验

4. 实现人机对战和人人对战模式

**专业级项目（竞赛参赛）**：

推荐使用优化 UCT 算法或 NoGoZero+。选择建议：



* 如果计算资源充足，优先选择 NoGoZero+，其性能优势明显

* 如果追求稳定性和可预测性，选择优化 UCT 算法

* 时间紧迫时，可基于开源代码进行改进

开发建议：



1. 建立完善的测试体系，与基准程序进行大量对弈

2. 重点优化开局库和终局策略

3. 实现高效的内存管理和缓存机制

4. 考虑并行化和 GPU 加速

**商业级应用（产品发布）**：

推荐使用混合架构，结合多种算法优势：



* 界面和基础功能：使用轻量级算法

* AI 核心：根据用户水平选择不同强度的算法

* 学习功能：实现简单的自学习机制

实现建议：



1. 设计模块化架构，便于算法替换和升级

2. 实现多种难度级别，满足不同用户需求

3. 开发在线对战功能，支持人机和人人对战

4. 集成分析功能，提供走法建议和复盘功能

### 4.3 部署与集成方案

算法的实际部署需要考虑多个因素：

**单机部署方案**：

适用于桌面应用和移动应用：



1. **Windows 平台**：

* 使用 C++ 实现核心算法，确保性能

* 使用 Qt 或 Unity 开发界面

* 打包成独立可执行文件

1. **移动平台**：

* 使用 C++/SDL 开发跨平台应用

* 针对移动设备优化内存使用

* 实现触控操作和 AI 难度调节

1. **Web 应用**：

* 使用 JavaScript/wasm 实现

* 将核心算法编译为 WebAssembly

* 支持浏览器内直接运行

**服务器部署方案**：

适用于在线服务和云平台：



1. **API 设计**：

* 设计 RESTful API 接口

* 支持 JSON 格式的请求和响应

* 实现身份验证和权限管理

1. **负载均衡**：

* 使用 Docker 容器化部署

* 实现自动扩缩容

* 设计缓存层减少计算压力

1. **实时对战**：

* 使用 WebSocket 实现实时通信

* 设计游戏状态同步机制

* 实现断线重连和状态保存

### 4.4 测试与验证方法

完善的测试体系是保证算法质量的关键：

**功能测试**：



1. **规则验证**：

* 测试所有规则的正确实现

* 边界条件测试（角、边、天元）

* 特殊情况测试（禁入点、自杀等）

1. **算法正确性**：

* 与已知正确的程序进行对比测试

* 使用小型棋盘验证算法逻辑

* 测试不同深度和参数设置的影响

**性能测试**：



1. **运行时间测试**：

* 测试不同局面下的决策时间

* 测试不同算法参数对性能的影响

* 测试大规模计算时的性能表现

1. **内存使用测试**：

* 测试长时间运行的内存泄漏

* 测试不同棋盘大小的内存需求

* 测试多线程和并行计算的内存使用

**对弈测试**：



1. **自对弈测试**：

* 让算法与自己对弈，统计胜率

* 分析常见错误模式

* 测试算法的稳定性

1. **基准测试**：

* 与标准程序（如 OASE-NoGo）进行对弈

* 统计不同条件下的胜率

* 分析算法的优势和劣势

1. **压力测试**：

* 在极限条件下测试算法表现

* 测试长时间连续对弈的稳定性

* 测试资源受限情况下的降级策略

## 结论

本文系统综述了不围棋（NoGo）博弈算法的技术实现与研究进展，从基础理论到前沿技术进行了全面分析。通过对多种算法的深入研究，可以得出以下主要结论：

**算法技术发展趋势**：NoGo 博弈算法经历了从传统搜索算法到深度学习的演进过程。早期基于规则和 UCT 的算法奠定了基础，随后 AlphaZero 架构的引入带来了质的飞跃，最新的 NoGoZero + 通过创新技术将训练效率提升了 6 倍。这一发展轨迹表明，融合领域知识的深度学习方法是当前的主流方向。

**性能对比分析**：各种算法在不同维度上展现出各自优势。优化 UCT 算法在传统方法中表现最佳，胜率可达 94%；价值递归算法具有最佳的资源效率，适合嵌入式应用；AlphaZero 架构代表了最高水平，通过自我对弈可达 2750 Elo。算法选择应根据具体应用场景和资源条件进行权衡。

**技术创新亮点**：



1. **评估函数创新**：从简单的计数方法发展到基于深度学习的评估网络

2. **搜索策略优化**：从静态参数到自适应参数，从单线程到并行计算

3. **学习机制突破**：从依赖人类知识到完全自主学习

4. **工程实现改进**：从理论算法到实际部署，注重性能与可扩展性平衡

**未来研究方向**：



1. **算法融合**：结合不同算法的优势，开发混合架构

2. **泛化能力提升**：研究算法对不同棋盘大小和规则变体的适应能力

3. **可解释性增强**：在保持高性能的同时，提高算法决策的可理解性

4. **边缘计算优化**：开发适合移动设备和嵌入式系统的轻量级算法

**实践应用建议**：



1. **项目开发**：建议采用模块化架构，便于算法替换和升级

2. **技术选择**：根据资源条件和性能要求选择合适的算法路线

3. **部署策略**：考虑不同平台的特性，进行针对性优化

4. **测试验证**：建立完善的测试体系，确保算法质量

不围棋作为一个新兴的博弈领域，为人工智能研究提供了独特的挑战和机遇。随着算法技术的不断进步和计算资源的日益丰富，我们有理由相信，NoGo 博弈算法将在理论研究和实际应用两个方面都取得更大突破。特别是在当前人工智能快速发展的背景下，NoGo 算法的研究成果有望为其他博弈游戏乃至更广泛的人工智能应用提供有价值的借鉴。

**参考资料&#x20;**

\[1] Solving Linear NoGo with Combinatorial Game Theory[ https://webdocs.cs.ualberta.ca/\~mmueller/ps/2024/2024\_Nogo\_CGT.pdf](https://webdocs.cs.ualberta.ca/~mmueller/ps/2024/2024_Nogo_CGT.pdf)

\[2] Revisiting Monte-Carlo Tree Search on a Normal Form Game: NoGo[ https://www.researchgate.net/profile/Shi-Jim-Yen-2/publication/220867825\_Revisiting\_Monte-Carlo\_Tree\_Search\_on\_a\_Normal\_Form\_Game\_NoGo/links/00b4953927279d2dd2000000/Revisiting-Monte-Carlo-Tree-Search-on-a-Normal-Form-Game-NoGo.pdf](https://www.researchgate.net/profile/Shi-Jim-Yen-2/publication/220867825_Revisiting_Monte-Carlo_Tree_Search_on_a_Normal_Form_Game_NoGo/links/00b4953927279d2dd2000000/Revisiting-Monte-Carlo-Tree-Search-on-a-Normal-Form-Game-NoGo.pdf)

\[3] 踩地雷程式TeddySweeper的設計與實作[ https://web.csie.ndhu.edu.tw/sjyen/Papers/TCGA2013.pdf](https://web.csie.ndhu.edu.tw/sjyen/Papers/TCGA2013.pdf)

\[4] STUDIES IN ALTERNATING AND SIMULTANEOUS COMBINATORIAL GAME THEORY[ https://dalspaceb.library.dal.ca/server/api/core/bitstreams/15c40c7f-9457-4678-ae90-d81f0e0fbe1c/content](https://dalspaceb.library.dal.ca/server/api/core/bitstreams/15c40c7f-9457-4678-ae90-d81f0e0fbe1c/content)

\[5] 正态模糊格点的 Feynman 规则和轴矢流反常[ http://zgwlc.xml-journal.net/fileZGWLC/journal/article/zgwlc/1989/3/PDF/890307.pdf](http://zgwlc.xml-journal.net/fileZGWLC/journal/article/zgwlc/1989/3/PDF/890307.pdf)

\[6] SIMPLICIAL COMPLEXES ARE GAME COMPLEXES[ https://arxiv.org/pdf/1608.05629](https://arxiv.org/pdf/1608.05629)

\[7] The Research of UCT and Rapid Action Value Estimation in NoGo Game[ https://m.zhangqiaokeyan.com/academic-conference-foreign\_meeting-199519\_thesis/0705015993920.html](https://m.zhangqiaokeyan.com/academic-conference-foreign_meeting-199519_thesis/0705015993920.html)

\[8] Depth, balancing, and limits of the Elo model[ https://arxiv.org/pdf/1511.02006](https://arxiv.org/pdf/1511.02006)

\[9] Pattern matching and Monte-Carlo simulation mechanism for the game of NoGo[ https://m.zhangqiaokeyan.com/academic-conference-foreign\_meeting-197767\_thesis/0705015278275.html](https://m.zhangqiaokeyan.com/academic-conference-foreign_meeting-197767_thesis/0705015278275.html)

\[10] Nested Monte Carlo Search for Two-Player Games[ https://object.cloud.sdsc.edu/v1/AUTH\_da4962d3368042ac8337e2dfdd3e7bf3/ml-papers/AAAI/2016/nested\_monte\_carlo\_search\_for\_twoplayer\_games\_\_1c5601e3.pdf](https://object.cloud.sdsc.edu/v1/AUTH_da4962d3368042ac8337e2dfdd3e7bf3/ml-papers/AAAI/2016/nested_monte_carlo_search_for_twoplayer_games__1c5601e3.pdf)

\[11] Does the Superior Colliculus Control Perceptual Sensitivity or Choice Bias during Attention? Evidence from a Multialternative Decision Framework[ https://cns.iisc.ac.in/sridhar/assets/publications/Devarajan\_JNeurosci\_2017.pdf](https://cns.iisc.ac.in/sridhar/assets/publications/Devarajan_JNeurosci_2017.pdf)

\[12] On Hidden Variables: Value and Expectation No-Go Theorems[ https://www.semanticscholar.org/paper/On-Hidden-Variables:-Value-and-Expectation-No-Go-Blass-Michigan/ce95ccd9f33932a138ab94e23245f33e4155fe92](https://www.semanticscholar.org/paper/On-Hidden-Variables:-Value-and-Expectation-No-Go-Blass-Michigan/ce95ccd9f33932a138ab94e23245f33e4155fe92)

\[13] No-go theorem for spatially regular boson stars made of static nonminimally coupled massive scalar fields[ https://physics.paperswithcode.com/paper/no-go-theorem-for-spatially-regular-boson](https://physics.paperswithcode.com/paper/no-go-theorem-for-spatially-regular-boson)

\[14] Zebrafish capable of generating future state prediction error show improved active avoidance behavior in virtual reality[ https://pmc.ncbi.nlm.nih.gov/articles/PMC8481257/pdf/41467\_2021\_Article\_26010.pdf](https://pmc.ncbi.nlm.nih.gov/articles/PMC8481257/pdf/41467_2021_Article_26010.pdf)

\[15] Genetic fuzzy markup language for game of NoGo[ https://m.zhangqiaokeyan.com/academic-journal-foreign\_knowledge-based-systems\_thesis/0204111363961.html](https://m.zhangqiaokeyan.com/academic-journal-foreign_knowledge-based-systems_thesis/0204111363961.html)

\[16] Unilaterally Competitive Multi-Player Stopping Games[ https://arxiv.org/pdf/1211.4113](https://arxiv.org/pdf/1211.4113)

\[17] 教师作为形成的E-RUBRICS设计师：一个案例研究GO / NO-GO标准的介绍和验证[ https://m.zhangqiaokeyan.com/journal-foreign-detail/0704024695885.html](https://m.zhangqiaokeyan.com/journal-foreign-detail/0704024695885.html)

\[18] COMBINATORIAL GAME COMPLEXITY: AN INTRODUCTION WITH POSET GAMES[ https://arxiv.org/pdf/1505.07416](https://arxiv.org/pdf/1505.07416)

\[19] A carry-over task rule in task switching: An ERP investigation using a Go/Nogo paradigm[ https://m.zhangqiaokeyan.com/journal-foreign-detail/07040124196.html](https://m.zhangqiaokeyan.com/journal-foreign-detail/07040124196.html)

\[20] GEOMETRIC QUANTIZATION AND NO-GO THEOREMS[ https://www.semanticscholar.org/paper/GEOMETRIC-QUANTIZATION-AND-NO-GO-THEOREMS-Ginzburg-Montgomery/16ca3e1b966becf3dea6ff3cc508f85042ab2971](https://www.semanticscholar.org/paper/GEOMETRIC-QUANTIZATION-AND-NO-GO-THEOREMS-Ginzburg-Montgomery/16ca3e1b966becf3dea6ff3cc508f85042ab2971)

\[21] A study of Monte Carlo Methods for Phantom Go[ https://140.113.37.243/bitstream/11536/73579/1/614001.pdf](https://140.113.37.243/bitstream/11536/73579/1/614001.pdf)

\[22] A Fuzzy Logic and Default Reasoning Model of Social Norm and Equilibrium Selection in Games under Unforeseen Contingencies[ http://core.ac.uk/download/pdf/6262894.pdf](http://core.ac.uk/download/pdf/6262894.pdf)

\[23] 不围棋（No Go）竞赛规则[ http://computergames.caai.cn/jsgz05.html](http://computergames.caai.cn/jsgz05.html)

\[24] PATTERN MATCHING AND MONTE-CARLO SIMULATION MECHANISM FOR THE GAME OF NOGO(pdf)[ https://www.sci-hub.ru/download/2024/5043/0e5e386169d0092a19adaabcd69059b5/sun2012.pdf](https://www.sci-hub.ru/download/2024/5043/0e5e386169d0092a19adaabcd69059b5/sun2012.pdf)

\[25] • Board Game Arena[ https://cs.boardgamearena.com/doc/Gamehelpclassicgo](https://cs.boardgamearena.com/doc/Gamehelpclassicgo)

\[26] Play Go game 9x9[ https://data.bangtech.com/game/go/index.htm](https://data.bangtech.com/game/go/index.htm)

\[27] The Research on Patterns and UCT Algorithm in NoGo Game[ https://dacemirror.sci-hub.se/proceedings-article/006e296b9065799ee9f2a5a3dbb6dfc2/sun2013.pdf#navpanes=0\&view=FitH](https://dacemirror.sci-hub.se/proceedings-article/006e296b9065799ee9f2a5a3dbb6dfc2/sun2013.pdf#navpanes=0\&view=FitH)

\[28] Go (9\*9)[ https://atelierboisjoueur.fr/jeux-en-bois-massif/go/](https://atelierboisjoueur.fr/jeux-en-bois-massif/go/)

\[29] Rules (in traslation):[ http://www.tortellinogoclub.org/english/regole.htm](http://www.tortellinogoclub.org/english/regole.htm)

\[30] CaptainChen/NoGo[ https://gitee.com/CaptainChen/no-go](https://gitee.com/CaptainChen/no-go)

\[31] 计算机博弈围棋,计算机博弈:“不围棋”入门教程-CSDN博客[ https://blog.csdn.net/weixin\_27012007/article/details/119121748](https://blog.csdn.net/weixin_27012007/article/details/119121748)

\[32] 围棋基本规则与术语详解[ https://www.iesdouyin.com/share/video/7410643882435824896/?region=\&mid=7410643031734455080\&u\_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with\_sec\_did=1\&video\_share\_track\_ver=\&titleType=title\&share\_sign=9yJw6o3hx5N\_lrnXPe7jW4oxanPbl34w6TrHFa4JyI4-\&share\_version=280700\&ts=1778673322\&from\_aid=1128\&from\_ssr=1\&share\_track\_info=%7B%22link\_description\_type%22%3A%22%22%7D](https://www.iesdouyin.com/share/video/7410643882435824896/?region=\&mid=7410643031734455080\&u_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with_sec_did=1\&video_share_track_ver=\&titleType=title\&share_sign=9yJw6o3hx5N_lrnXPe7jW4oxanPbl34w6TrHFa4JyI4-\&share_version=280700\&ts=1778673322\&from_aid=1128\&from_ssr=1\&share_track_info=%7B%22link_description_type%22%3A%22%22%7D)

\[33] 基于C++的不围棋NOGO代码-PKU计算概论A大作业-MCTS算法&& Minimax算法-CSDN博客[ https://blog.csdn.net/TonyChen1234/article/details/116465948](https://blog.csdn.net/TonyChen1234/article/details/116465948)

\[34] GitHub - epcm/QtNoGo: 基于Qt的不围棋(nogo)单机对战平台，包含基于MCTS的AI对战Bot[ https://github.com/epcm/QtNoGo/](https://github.com/epcm/QtNoGo/)

\[35] 安徽省大学生创新创业教育办公室关于发布2024年安徽省大学生计算机博弈大赛赛项规程的通知\_安徽省教育厅[ http://jyt.ah.gov.cn/tsdw/gdjyc/dxsxkhjnjs/40723621.html](http://jyt.ah.gov.cn/tsdw/gdjyc/dxsxkhjnjs/40723621.html)

\[36] Efficiently Mastering the Game of NoGo with Deep Reinforcement Learning Supported by Domain Knowledge[ https://pdfs.semanticscholar.org/2aa5/02cc4b3a5a2e036a0da2485f5ae7a195bee9.pdf](https://pdfs.semanticscholar.org/2aa5/02cc4b3a5a2e036a0da2485f5ae7a195bee9.pdf)

\[37] Technique Analysis and Designing of Program with UCT Algorithm for NoGo[ http://computergames.caai.cn/download/papers/2013/SatB11-3.pdf](http://computergames.caai.cn/download/papers/2013/SatB11-3.pdf)

\[38] Analyses of Tabular AlphaZero on NoGo[ https://dspace.jaist.ac.jp/dspace/bitstream/10119/18244/1/I-IKEDA-K0405-21.pdf](https://dspace.jaist.ac.jp/dspace/bitstream/10119/18244/1/I-IKEDA-K0405-21.pdf)

\[39] The Research on Patterns and UCT Algorithm in NoGo Game[ https://dacemirror.sci-hub.se/proceedings-article/006e296b9065799ee9f2a5a3dbb6dfc2/sun2013.pdf#navpanes=0\&view=FitH](https://dacemirror.sci-hub.se/proceedings-article/006e296b9065799ee9f2a5a3dbb6dfc2/sun2013.pdf#navpanes=0\&view=FitH)

\[40] Application of Reparameterization and Terminal Batch Processing Operators in NoGo[ https://dl.acm.org/doi/abs/10.1145/3703187.3703264](https://dl.acm.org/doi/abs/10.1145/3703187.3703264)

\[41] A Survey of Monte Carlo Tree Search Methods[ https://www.researchgate.net/profile/Diego-Perez-Liebana/publication/235985858\_A\_Survey\_of\_Monte\_Carlo\_Tree\_Search\_Methods/links/0046352723c772ecd9000000/A-Survey-of-Monte-Carlo-Tree-Search-Methods.pdf](https://www.researchgate.net/profile/Diego-Perez-Liebana/publication/235985858_A_Survey_of_Monte_Carlo_Tree_Search_Methods/links/0046352723c772ecd9000000/A-Survey-of-Monte-Carlo-Tree-Search-Methods.pdf)

\[42] Multiple Policy Value Monte Carlo Tree Search[ https://i777777o696a636169o6f7267z.oszar.com/proceedings/2019/0653.pdf](https://i777777o696a636169o6f7267z.oszar.com/proceedings/2019/0653.pdf)

\[43] Enhancing AI Agent Reliability under Out-of-Distribution and Near-Distribution Conditions[ https://escholarship.org/content/qt63x062gt/qt63x062gt.pdf](https://escholarship.org/content/qt63x062gt/qt63x062gt.pdf)

\[44] Modification of improved upper confidence bounds for regulating exploration in Monte-Carlo tree search[ https://daneshyari.com/article/preview/4952488.pdf](https://daneshyari.com/article/preview/4952488.pdf)

\[45] Efficiently Mastering the Game of NoGo with Deep Reinforcement Learning Supported by Domain Knowledge[ https://pdfs.semanticscholar.org/2aa5/02cc4b3a5a2e036a0da2485f5ae7a195bee9.pdf](https://pdfs.semanticscholar.org/2aa5/02cc4b3a5a2e036a0da2485f5ae7a195bee9.pdf)

\[46] Analyses of Tabular AlphaZero on NoGo[ https://sci-hub.se/downloads/2021-05-19/f2/hsueh2020.pdf#navpanes=0\&view=FitH](https://sci-hub.se/downloads/2021-05-19/f2/hsueh2020.pdf#navpanes=0\&view=FitH)

\[47] Learning to Stop: Dynamic Simulation Monte Carlo Tree Search[ https://cdn.aaai.org/ojs/16100/16100-13-19594-1-2-20210518.pdf](https://cdn.aaai.org/ojs/16100/16100-13-19594-1-2-20210518.pdf)

\[48] NAT: Neural Architecture Transformer for Accurate and Compact Architectures[ https://papers.neurips.cc/paper\_files/paper/2019/file/beed13602b9b0e6ecb5b568ff5058f07-Paper.pdf](https://papers.neurips.cc/paper_files/paper/2019/file/beed13602b9b0e6ecb5b568ff5058f07-Paper.pdf)

\[49] Fusing Global and Local: Transformer-CNN Synergy for Next-Gen Current Estimation[ https://arxiv.org/pdf/2504.07996](https://arxiv.org/pdf/2504.07996)

\[50] Mastering Atari, Go, Chess and Shogi by Planning with a Learned Model[ https://arxiv.org/pdf/1911.08265](https://arxiv.org/pdf/1911.08265)

\[51] Forecasting Demand and Controlling Inventory Using Transformer-Based Neural Network Optimized by Whale Optimization Algorithm[ https://dl.acm.org/doi/abs/10.1145/3675417.3675569](https://dl.acm.org/doi/abs/10.1145/3675417.3675569)

\[52] Image caption detection with help of transformer networks algorithm in comparison with convolutional neural networks[ https://pubs.aip.org/aip/acp/article/3252/1/020052/3337937/Image-caption-detection-with-help-of-transformer](https://pubs.aip.org/aip/acp/article/3252/1/020052/3337937/Image-caption-detection-with-help-of-transformer)

\[53] Mastering the Game of Go with Deep Neural Networks and Tree Search[ https://www.researchgate.net/profile/Timothy\_Lillicrap/publication/292074166\_Mastering\_the\_game\_of\_Go\_with\_deep\_neural\_networks\_and\_tree\_search/links/5b9659a74585153a531a6601/Mastering-the-game-of-Go-with-deep-neural-networks-and-tree-search.pdf](https://www.researchgate.net/profile/Timothy_Lillicrap/publication/292074166_Mastering_the_game_of_Go_with_deep_neural_networks_and_tree_search/links/5b9659a74585153a531a6601/Mastering-the-game-of-Go-with-deep-neural-networks-and-tree-search.pdf)

\[54] Enhancement of Underwater Images through Parallel Fusion of Transformer and CNN[ https://ouci.dntb.gov.ua/en/works/4kzMGzG7/](https://ouci.dntb.gov.ua/en/works/4kzMGzG7/)

\[55] Mastering the Game of Go without Human Knowledge[ https://top.tyaskartini89.workers.dev/ickypeach-https-discovery.ucl.ac.uk/id/eprint/10045895/1/agz\_unformatted\_nature.pdf](https://top.tyaskartini89.workers.dev/ickypeach-https-discovery.ucl.ac.uk/id/eprint/10045895/1/agz_unformatted_nature.pdf)

\[56] Fault Diagnosis of Integrated Core Processor in Avionics System Based on CNN-Transformer[ https://www.semanticscholar.org/paper/Fault-Diagnosis-of-Integrated-Core-Processor-in-on-Zhao-Jiao/91afc0738e6d507f2684ed4eb3ef10f20abad381](https://www.semanticscholar.org/paper/Fault-Diagnosis-of-Integrated-Core-Processor-in-on-Zhao-Jiao/91afc0738e6d507f2684ed4eb3ef10f20abad381)

\[57] Investigating the use of Machine Learning Methods in Go[ https://www.researchgate.net/profile/Daniel-Sleator/publication/265228271\_Investigating\_the\_use\_of\_Machine\_Learning\_Methods\_in\_Go/links/55081cff0cf2d7a2812733a6/Investigating-the-use-of-Machine-Learning-Methods-in-Go.pdf](https://www.researchgate.net/profile/Daniel-Sleator/publication/265228271_Investigating_the_use_of_Machine_Learning_Methods_in_Go/links/55081cff0cf2d7a2812733a6/Investigating-the-use-of-Machine-Learning-Methods-in-Go.pdf)

\[58] Adaptive chunking improves effective working memory capacity in a prefrontal cortex and basal ganglia circuit[ https://pmc.ncbi.nlm.nih.gov/articles/PMC11870651/pdf/elife-97894.pdf](https://pmc.ncbi.nlm.nih.gov/articles/PMC11870651/pdf/elife-97894.pdf)

\[59] GitHub - epcm/QtNoGo: 基于Qt的不围棋(nogo)单机对战平台，包含基于MCTS的AI对战Bot[ https://github.com/epcm/QtNoGo](https://github.com/epcm/QtNoGo)

\[60] Application of Reparameterization and Terminal Batch Processing Operators in NoGo[ https://dl.acm.org/doi/pdf/10.1145/3703187.3703264](https://dl.acm.org/doi/pdf/10.1145/3703187.3703264)

\[61] Efficiently Mastering the Game of NoGo with Deep Reinforcement Learning Supported by Domain Knowledge[ https://mdpi-res.com/d\_attachment/electronics/electronics-10-01533/article\_deploy/electronics-10-01533-v2.pdf?version=1624855346](https://mdpi-res.com/d_attachment/electronics/electronics-10-01533/article_deploy/electronics-10-01533-v2.pdf?version=1624855346)

\[62] To Go or Not To Go? A Near Unsupervised Learning Approach For Robot Navigation(pdf)[ https://arxiv.org/pdf/1709.05439v1](https://arxiv.org/pdf/1709.05439v1)

\[63] GenAI: Fast Data Synthetization with Distribution-free Hierarchical Bayesian Models[ https://mltechniques.com/2023/09/22/tabular-data-generation-with-fast-hierarchical-deep-resampling/](https://mltechniques.com/2023/09/22/tabular-data-generation-with-fast-hierarchical-deep-resampling/)

\[64] 基于alphazero的不围棋博弈系统研究(pdf)[ https://cs.hit.edu.cn/\_upload/article/files/71/96/56b7336148f3b3875091f6ff766f/34c16c08-a04e-4a59-abed-498636d61a3e.pdf](https://cs.hit.edu.cn/_upload/article/files/71/96/56b7336148f3b3875091f6ff766f/34c16c08-a04e-4a59-abed-498636d61a3e.pdf)

\[65] 基于Qt5.9.9实现的不围棋(Nogo)游戏:支持PVP/PVE、Minimax+AlphaBeta与MCTS双AI算法及局势可视化 - CSDN文库[ https://wenku.csdn.net/doc/64bwtwe52t](https://wenku.csdn.net/doc/64bwtwe52t)

\[66] 基于价值评估的不围棋递归算法[ https://xblk.ecnu.edu.cn/CN/html/20190107.htm](https://xblk.ecnu.edu.cn/CN/html/20190107.htm)

\[67] 基于C++的不围棋NOGO代码-PKU计算概论A大作业-MCTS算法&& Minimax算法-CSDN博客[ https://blog.csdn.net/TonyChen1234/article/details/116465948](https://blog.csdn.net/TonyChen1234/article/details/116465948)

\[68] Efficiently Mastering the Game of NoGo with Deep Reinforcement Learning Supported by Domain Knowledge(pdf)[ https://mdpi-res.com/d\_attachment/electronics/electronics-10-01533/article\_deploy/electronics-10-01533-v2.pdf?version=1624855346](https://mdpi-res.com/d_attachment/electronics/electronics-10-01533/article_deploy/electronics-10-01533-v2.pdf?version=1624855346)

\[69] Solving Linear NoGo with Combinatorial Game Theory(pdf)[ https://webdocs.cs.ualberta.ca/\~mmueller/ps/2024/2024\_Nogo\_CGT.pdf](https://webdocs.cs.ualberta.ca/~mmueller/ps/2024/2024_Nogo_CGT.pdf)

\[70] Nogo/ at master · stylke/Nogo · GitHub[ https://github.com/stylke/Nogo?search=1](https://github.com/stylke/Nogo?search=1)

\[71] BANG : Billion-Scale Approximate Nearest Neighbor Search using a Single GPU[ https://arxiv.org/html/2401.11324v1](https://arxiv.org/html/2401.11324v1)

\[72] Fantasy: Efficient Large-scale Vector Search on GPU Clusters with GPUDirect Async[ https://arxiv.org/html/2512.02278v1/](https://arxiv.org/html/2512.02278v1/)

\[73] DeepSeek开源DualPipe等算法突破美国算力霸权[ https://www.iesdouyin.com/share/video/7476096677577821491/?region=\&mid=7476096773428071219\&u\_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with\_sec\_did=1\&video\_share\_track\_ver=\&titleType=title\&share\_sign=ec4\_1D1OcYR6F4ECA5tc9j1NKnv4WHNzFI62V4HwOO0-\&share\_version=280700\&ts=1778673437\&from\_aid=1128\&from\_ssr=1\&share\_track\_info=%7B%22link\_description\_type%22%3A%22%22%7D](https://www.iesdouyin.com/share/video/7476096677577821491/?region=\&mid=7476096773428071219\&u_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with_sec_did=1\&video_share_track_ver=\&titleType=title\&share_sign=ec4_1D1OcYR6F4ECA5tc9j1NKnv4WHNzFI62V4HwOO0-\&share_version=280700\&ts=1778673437\&from_aid=1128\&from_ssr=1\&share_track_info=%7B%22link_description_type%22%3A%22%22%7D)

\[74] PathWeaver: A High-Throughput Multi-GPU System for Graph-Based Approximate Nearest Neighbor Search(pdf)[ https://www.usenix.org/system/files/atc25-kim.pdf](https://www.usenix.org/system/files/atc25-kim.pdf)

\[75] Portable PGAS-Based GPU-Accelerated Branch-And-Bound Algorithms at Scale[ https://onlinelibrary.wiley.com/doi/full/10.1002/cpe.70321](https://onlinelibrary.wiley.com/doi/full/10.1002/cpe.70321)

\[76] Bring Massive-Scale Vector Search to the GPU with Apache Lucene[ https://www.nvidia.com/en-us/on-demand/session/gtc25-S71286/](https://www.nvidia.com/en-us/on-demand/session/gtc25-S71286/)

\[77] Efficiently Mastering the Game of NoGo with Deep Reinforcement Learning Supported by Domain Knowledge[ https://mdpi-res.com/d\_attachment/electronics/electronics-10-01533/article\_deploy/electronics-10-01533-with-cover.pdf?version=1669247301](https://mdpi-res.com/d_attachment/electronics/electronics-10-01533/article_deploy/electronics-10-01533-with-cover.pdf?version=1669247301)

\[78] AlphaZero | AI Wiki[ https://aiwiki.ai/wiki/alphazero](https://aiwiki.ai/wiki/alphazero)

\[79] AlphaGo Zero[ https://grokipedia.com/page/AlphaGo\_Zero](https://grokipedia.com/page/AlphaGo_Zero)

\[80] Pretraining AlphaZero using proximal policy optimization[ https://kth.diva-portal.org/smash/get/diva2:1987622/FULLTEXT01.pdf](https://kth.diva-portal.org/smash/get/diva2:1987622/FULLTEXT01.pdf)

\[81] cchess-zero/Mastering\_Chess\_and\_Shogi\_by\_Self-Play\_with\_a\_General\_Reinforcement\_Learning\_Algorithm.ipynb at master · Skyseezhang321/cchess-zero · GitHub[ https://web17036.github.shopify.com/Skyseezhang321/cchess-zero/blob/master/Mastering\_Chess\_and\_Shogi\_by\_Self-Play\_with\_a\_General\_Reinforcement\_Learning\_Algorithm.ipynb](https://web17036.github.shopify.com/Skyseezhang321/cchess-zero/blob/master/Mastering_Chess_and_Shogi_by_Self-Play_with_a_General_Reinforcement_Learning_Algorithm.ipynb)

\[82] Nogo/ at master · stylke/Nogo · GitHub[ https://github.com/stylke/Nogo?search=1](https://github.com/stylke/Nogo?search=1)

\[83] 四种主流语言N皇后算法性能对比分析[ https://www.iesdouyin.com/share/video/7582899630199835904/?region=\&mid=7582899674669583158\&u\_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with\_sec\_did=1\&video\_share\_track\_ver=\&titleType=title\&share\_sign=.PlpcCat7gOG.23240Y45xcc4a2epdfH7xg7p0.Jicc-\&share\_version=280700\&ts=1778673447\&from\_aid=1128\&from\_ssr=1\&share\_track\_info=%7B%22link\_description\_type%22%3A%22%22%7D](https://www.iesdouyin.com/share/video/7582899630199835904/?region=\&mid=7582899674669583158\&u_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with_sec_did=1\&video_share_track_ver=\&titleType=title\&share_sign=.PlpcCat7gOG.23240Y45xcc4a2epdfH7xg7p0.Jicc-\&share_version=280700\&ts=1778673447\&from_aid=1128\&from_ssr=1\&share_track_info=%7B%22link_description_type%22%3A%22%22%7D)

\[84] Analyses of Tabular AlphaZero on NoGo[ https://scholar.nycu.edu.tw/zh/publications/analyses-of-tabular-alphazero-on-nogo](https://scholar.nycu.edu.tw/zh/publications/analyses-of-tabular-alphazero-on-nogo)

\[85] 성능[ https://rexai.top/agno-Go/ko/advanced/performance.html](https://rexai.top/agno-Go/ko/advanced/performance.html)

\[86] ScratchGo: An Integrated Approach to Playing Computer Go[ https://woodyfolsom.net/blog/wp-content/uploads/2012/12/scratchgo-wfolsom-2013.pdf](https://woodyfolsom.net/blog/wp-content/uploads/2012/12/scratchgo-wfolsom-2013.pdf)

\[87] 蒙特卡洛树搜索算法解析与应用[ https://www.iesdouyin.com/share/video/7557311212770381098/?region=\&mid=7557311153472195328\&u\_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with\_sec\_did=1\&video\_share\_track\_ver=\&titleType=title\&share\_sign=m9Nmv.w6o1ZdeTqpfJ6CrQYYyUL3Kr2MFmZC0n1rdEw-\&share\_version=280700\&ts=1778673454\&from\_aid=1128\&from\_ssr=1\&share\_track\_info=%7B%22link\_description\_type%22%3A%22%22%7D](https://www.iesdouyin.com/share/video/7557311212770381098/?region=\&mid=7557311153472195328\&u_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with_sec_did=1\&video_share_track_ver=\&titleType=title\&share_sign=m9Nmv.w6o1ZdeTqpfJ6CrQYYyUL3Kr2MFmZC0n1rdEw-\&share_version=280700\&ts=1778673454\&from_aid=1128\&from_ssr=1\&share_track_info=%7B%22link_description_type%22%3A%22%22%7D)

\[88] Zen4:探究日本围棋AI技术的巅峰-CSDN博客[ https://blog.csdn.net/weixin\_35835018/article/details/148036358](https://blog.csdn.net/weixin_35835018/article/details/148036358)

\[89] Understanding Methods for Scalable MCTS[ https://iclr-blogposts.github.io/2025/blog/scalable-mcts/](https://iclr-blogposts.github.io/2025/blog/scalable-mcts/)

\[90] DarkForest MCTS实现原理:蒙特卡洛树搜索的高效优化策略-CSDN博客[ https://blog.csdn.net/gitblog\_00576/article/details/155797499](https://blog.csdn.net/gitblog_00576/article/details/155797499)

\[91] 北方苍鹰优化算法(NGO)及其在智能优化中的应用\_智能优化算法比较NGO - CSDN文库[ https://wenku.csdn.net/doc/3sy70dxnf0](https://wenku.csdn.net/doc/3sy70dxnf0)

\[92] 群体智能优化算法-北方苍鹰优化算法(NGO，含Matlab源代码)\_northern goshawk optimization-CSDN博客[ https://blog.csdn.net/qq\_53665413/article/details/146974074](https://blog.csdn.net/qq_53665413/article/details/146974074)

\[93] 基于领导者自适应步长改进的北方苍鹰优化算法(INGO)-CSDN博客[ https://blog.csdn.net/2301\_82017165/article/details/135514436](https://blog.csdn.net/2301_82017165/article/details/135514436)

\[94] 完全 端 到 端 闭环 导航 ！ 仅 需 相机 ， LoGo Planner 实现 感知 定位 规划 一体化&#x20;

&#x20;

&#x20;LoGo Planner 是 一项 完全 端 到 端 的 闭环 导航 研究 ， 核心 在于 解决 传统 方案 “ 定位 悄然 失效 ” 的 部署 难题 。 它 摒弃 独立 的 SLAM 模块 ， 通过 长 序列 视觉 观测 进行 隐式 状态 估计 ， 并 将 真实 尺度 感知 融入 策略 内部 ， 使 机器人 获得 内生 的 “ 空间 位置 感 ” 。 该 方法 在 轮式 、 四 足 和 人形 等 多种 平台 上 实现 了 跨 场景 稳健 导航 ， 推动 了 端 到 端 导航 迈向 真正 的 “ 真 闭环 ” 。&#x20;

&#x20;

&#x20;论文 题目 ： LoGo Planner : Localization Grounded Navigation Policy with Metric - aware Visual Geometry&#x20;

&#x20;

&#x20;\# 端 到 端 # 自主 导航 # 具 身 智能 # 人工 智能 # 机器人 # 环境 感知 # 清华 大学[ https://www.iesdouyin.com/share/video/7592928238926564614/?region=\&mid=7592928296711670534\&u\_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with\_sec\_did=1\&video\_share\_track\_ver=\&titleType=title\&share\_sign=JCnVmjYwkzhMgEleHmFr9auUHZsi1v9LPW9hbMowjgs-\&share\_version=280700\&ts=1778673454\&from\_aid=1128\&from\_ssr=1\&share\_track\_info=%7B%22link\_description\_type%22%3A%22%22%7D](https://www.iesdouyin.com/share/video/7592928238926564614/?region=\&mid=7592928296711670534\&u_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with_sec_did=1\&video_share_track_ver=\&titleType=title\&share_sign=JCnVmjYwkzhMgEleHmFr9auUHZsi1v9LPW9hbMowjgs-\&share_version=280700\&ts=1778673454\&from_aid=1128\&from_ssr=1\&share_track_info=%7B%22link_description_type%22%3A%22%22%7D)

\[95] 【无人机设计与控制】 五种算法(SO、POA、DBO、NGO、PSO)求解无人机三维路径规划\_pso和so算法-CSDN博客[ https://blog.csdn.net/Matlab\_jiqi/article/details/145970364](https://blog.csdn.net/Matlab_jiqi/article/details/145970364)

\[96] rules\_go/go/nogo.rst at master · bazel-contrib/rules\_go · GitHub[ https://github.com/bazel-contrib/rules\_go/blob/master/go/nogo.rst](https://github.com/bazel-contrib/rules_go/blob/master/go/nogo.rst)

\[97] MATLAB实现NGO-SCN北方苍鹰算法优化随机配置网络的数据回归预测\_随机配置网络scn-CSDN博客[ https://blog.csdn.net/xiaoxingkongyuxi/article/details/144681690](https://blog.csdn.net/xiaoxingkongyuxi/article/details/144681690)

\[98] 计算机博弈与中国大学生计算机博弈大赛\_计算机对抗赛规则-CSDN博客[ https://blog.csdn.net/zeyeqianli/article/details/149193049](https://blog.csdn.net/zeyeqianli/article/details/149193049)

\[99] 优秀竞赛获奖作品:计算机博弈竞赛项目-点格棋-计算机科学与技术学院[ https://xxxy.sie.edu.cn/info/2013/11776.htm](https://xxxy.sie.edu.cn/info/2013/11776.htm)

\[100] 全国计算机博弈大赛[ http://computergames.caai.cn/info/news180809.html](http://computergames.caai.cn/info/news180809.html)

\[101] 我院学子在全国大学生计算机博弈大赛中斩获多项佳绩[ https://smse.aufe.edu.cn/2025/1009/c1791a239719/page.htm](https://smse.aufe.edu.cn/2025/1009/c1791a239719/page.htm)

\[102] 中国象棋项目总结 📋[ https://github.com/chenchihwen/chinese-chess-ai/blob/main/PROJECT\_SUMMARY.md](https://github.com/chenchihwen/chinese-chess-ai/blob/main/PROJECT_SUMMARY.md)

\[103] 不围棋对弈算法及计算机博弈可解释性研究-学位-万方数据知识服务平台[ http://d.wanfangdata.com.cn/thesis/D02667205](http://d.wanfangdata.com.cn/thesis/D02667205)

\[104] 中南大学自主研发软件“MYGO”卫冕全国计算机博弈大赛-中南大学新闻网门户网站[ https://news.csu.edu.cn/info/1002/123923.htm](https://news.csu.edu.cn/info/1002/123923.htm)

\[105] Nogo/ at master · stylke/Nogo · GitHub[ https://github.com/stylke/Nogo?search=1](https://github.com/stylke/Nogo?search=1)

\[106] Efficiently Mastering the Game of NoGo with Deep Reinforcement Learning Supported by Domain Knowledge[ https://www.mdpi.com/2079-9292/10/13/1533/html](https://www.mdpi.com/2079-9292/10/13/1533/html)

\[107] 神经网络系统设计方法论：从问题定义到生产部署[ https://www.iesdouyin.com/share/video/7571837736282485690/?region=\&mid=7571837801688222490\&u\_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with\_sec\_did=1\&video\_share\_track\_ver=\&titleType=title\&share\_sign=Nh1Yg2dsYVVKCIMLalROm\_4ZQjt9d8h9dTE53NSk4Ow-\&share\_version=280700\&ts=1778673464\&from\_aid=1128\&from\_ssr=1\&share\_track\_info=%7B%22link\_description\_type%22%3A%22%22%7D](https://www.iesdouyin.com/share/video/7571837736282485690/?region=\&mid=7571837801688222490\&u_code=0\&did=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&iid=MS4wLjABAAAANwkJuWIRFOzg5uCpDRpMj4OX-QryoDgn-yYlXQnRwQQ\&with_sec_did=1\&video_share_track_ver=\&titleType=title\&share_sign=Nh1Yg2dsYVVKCIMLalROm_4ZQjt9d8h9dTE53NSk4Ow-\&share_version=280700\&ts=1778673464\&from_aid=1128\&from_ssr=1\&share_track_info=%7B%22link_description_type%22%3A%22%22%7D)

\[108] NoGoGameAI[ https://github.com/kodmasta/NoGoGameAI](https://github.com/kodmasta/NoGoGameAI)

> （注：文档部分内容可能由 AI 生成）