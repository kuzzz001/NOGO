# 不围棋AI测试指南

## 一、快速开始

### 1.1 一键测试
```bash
./test.sh
```

### 1.2 手动测试
```bash
# 编译
g++ -std=c++17 -O2 main.cpp -o nogo

# 运行测试
echo -e "1\n-1 -1" | ./nogo
```

## 二、测试用例详解

### 2.1 基础测试用例

#### 测试1：空棋盘开局（我方先手）
**输入**：
```
1
-1 -1
```

**说明**：
- `1`：对局步数
- `-1 -1`：表示我方先手（对方未落子）

**预期输出**：
- 中心位置附近，如 `4 4`、`3 4`、`4 3` 等

**测试命令**：
```bash
echo -e "1\n-1 -1" | ./nogo
```

#### 测试2：对方先手（我方后手）
**输入**：
```
1
4 4
-1 -1
```

**说明**：
- `1`：对局步数
- `4 4`：对方第一步下在中心
- `-1 -1`：我方未落子（需要决策）

**预期输出**：
- 中心附近位置，如 `3 4`、`4 3`、`5 4`、`4 5`

**测试命令**：
```bash
echo -e "1\n4 4\n-1 -1" | ./nogo
```

#### 测试3：对局中段
**输入**：
```
3
4 4
3 4
4 3
-1 -1
```

**说明**：
- `3`：对局步数
- `4 4`：对方第一步
- `3 4`：我方第一步
- `4 3`：对方第二步
- `-1 -1`：我方第二步（需要决策）

**预期输出**：
- 合法的落子位置

**测试命令**：
```bash
echo -e "3\n4 4\n3 4\n4 3\n-1 -1" | ./nogo
```

### 2.2 边界测试用例

#### 测试4：角部开局
**输入**：
```
1
0 0
-1 -1
```

**预期输出**：
- 避免角部，选择更有利位置

**测试命令**：
```bash
echo -e "1\n0 0\n-1 -1" | ./nogo
```

#### 测试5：边部开局
**输入**：
```
1
0 4
-1 -1
```

**预期输出**：
- 避免边部，选择更有利位置

**测试命令**：
```bash
echo -e "1\n0 4\n-1 -1" | ./nogo
```

### 2.3 性能测试用例

#### 测试6：时间限制测试
```bash
echo -e "1\n-1 -1" | time ./nogo
```

**预期结果**：
- 运行时间 < 1秒
- 输出合法位置

#### 测试7：复杂局面性能测试
```bash
# 创建复杂对局
cat > complex_input.txt << EOF
10
4 4
3 4
4 3
5 4
4 5
3 3
5 5
2 4
6 4
-1 -1
EOF

# 测试
time ./nogo < complex_input.txt
```

### 2.4 稳定性测试用例

#### 测试8：多次运行测试
```bash
for i in {1..100}; do
    echo -e "1\n-1 -1" | ./nogo > /dev/null
done
echo "稳定性测试完成"
```

#### 测试9：随机局面测试
```bash
for i in {1..10}; do
    n=$((RANDOM % 20 + 1))
    echo -e "$n\n-1 -1" | ./nogo
done
```

## 三、验证方法

### 3.1 合法性验证

#### 检查输出格式
```bash
output=$(echo -e "1\n-1 -1" | ./nogo)
if [[ $output =~ ^[0-8]\ [0-8]$ ]]; then
    echo "✅ 输出格式正确"
else
    echo "❌ 输出格式错误"
fi
```

#### 检查坐标范围
```bash
output=$(echo -e "1\n-1 -1" | ./nogo)
x=$(echo $output | cut -d' ' -f1)
y=$(echo $output | cut -d' ' -f2)

if [ $x -ge 0 ] && [ $x -le 8 ] && [ $y -ge 0 ] && [ $y -le 8 ]; then
    echo "✅ 坐标范围正确"
else
    echo "❌ 坐标范围错误"
fi
```

### 3.2 性能验证

#### 测量运行时间
```bash
start=$(date +%s.%N)
echo -e "1\n-1 -1" | ./nogo > /dev/null
end=$(date +%s.%N)
elapsed=$(echo "$end - $start" | bc)

echo "运行时间: $elapsed 秒"

if (( $(echo "$elapsed < 1.0" | bc -l) )); then
    echo "✅ 性能符合要求"
else
    echo "❌ 性能不符合要求"
fi
```

#### 测量内存占用
```bash
/usr/bin/time -v ./nogo < input.txt 2>&1 | grep "Maximum resident set size"
```

## 四、调试技巧

### 4.1 添加调试输出

在 `main.cpp` 中添加调试代码：
```cpp
// 在决策后添加
cerr << "决策位置: " << new_x << " " << new_y << endl;
cerr << "模拟次数: " << simulations << endl;
```

### 4.2 使用GDB调试
```bash
g++ -std=c++17 -g main.cpp -o nogo_debug
gdb ./nogo_debug
```

常用GDB命令：
- `break main`：设置断点
- `run`：运行程序
- `print new_x`：打印变量
- `step`：单步执行
- `continue`：继续执行

### 4.3 使用Valgrind检测内存泄漏
```bash
valgrind --leak-check=full ./nogo < input.txt
```

## 五、Botzone平台测试

### 5.1 准备上传

#### 1. 检查文件
```bash
ls -lh main.cpp
```

#### 2. 验证编译
```bash
g++ -std=c++17 -O2 main.cpp -o nogo
```

#### 3. 运行测试
```bash
./test.sh
```

### 5.2 上传步骤

1. **登录Botzone平台**
   - 访问：https://www.botzone.org.cn
   - 登录账号

2. **创建Bot**
   - 选择"不围棋"游戏
   - 创建新Bot

3. **上传代码**
   - 上传 `main.cpp`
   - 等待编译

4. **开始对战**
   - 创建对战房间
   - 邀请对手或匹配AI

### 5.3 平台测试

#### 本地对战测试
```bash
# 模拟平台输入
cat > botzone_input.txt << EOF
1
-1 -1
EOF

./nogo < botzone_input.txt
```

#### 查看对局日志
- 在Botzone平台查看对局回放
- 分析决策是否合理
- 检查是否有超时或错误

## 六、常见问题

### 6.1 编译错误

#### 问题：找不到头文件
```bash
# 解决：检查编译器版本
g++ --version
```

#### 问题：链接错误
```bash
# 解决：添加必要的链接选项
g++ -std=c++17 -O2 main.cpp -o nogo -pthread
```

### 6.2 运行错误

#### 问题：段错误
```bash
# 解决：使用GDB调试
gdb ./nogo
(gdb) run < input.txt
```

#### 问题：超时
```bash
# 解决：减少模拟次数
# 修改 calculateSimulations 函数
```

### 6.3 性能问题

#### 问题：运行时间过长
```bash
# 解决1：添加编译优化
g++ -std=c++17 -O3 main.cpp -o nogo

# 解决2：减少模拟次数
# 修改 calculateSimulations 函数

# 解决3：使用性能分析工具
perf record ./nogo < input.txt
perf report
```

## 七、测试报告模板

### 测试报告

```
测试日期：2026-05-09
测试人员：XXX
测试环境：macOS, g++ 11.x

测试结果：
1. 编译测试：✅ 通过
2. 功能测试：✅ 通过
3. 性能测试：✅ 通过（0.217秒）
4. 稳定性测试：✅ 通过（10/10）
5. 边界测试：✅ 通过

发现问题：
- 无

改进建议：
- 无

结论：
✅ 项目符合Botzone平台要求，可以上传测试
```

## 八、自动化测试脚本

### 完整测试脚本（test.sh）

```bash
#!/bin/bash

echo "========================================="
echo "  不围棋AI测试脚本"
echo "========================================="
echo ""

# 编译
echo "1. 编译程序..."
g++ -std=c++17 -O2 main.cpp -o nogo
if [ $? -eq 0 ]; then
    echo "   ✅ 编译成功"
else
    echo "   ❌ 编译失败"
    exit 1
fi
echo ""

# 测试1：空棋盘开局
echo "2. 测试1：空棋盘开局（我方先手）"
result=$(echo -e "1\n-1 -1" | ./nogo)
echo "   输出: $result"
if [ "$result" != "" ]; then
    echo "   ✅ 测试通过"
else
    echo "   ❌ 测试失败"
fi
echo ""

# 测试2：对方先手
echo "3. 测试2：对方先手（我方后手）"
result=$(echo -e "1\n4 4\n-1 -1" | ./nogo)
echo "   输出: $result"
if [ "$result" != "" ]; then
    echo "   ✅ 测试通过"
else
    echo "   ❌ 测试失败"
fi
echo ""

# 测试3：对局中段
echo "4. 测试3：对局中段"
result=$(echo -e "3\n4 4\n3 4\n4 3\n-1 -1" | ./nogo)
echo "   输出: $result"
if [ "$result" != "" ]; then
    echo "   ✅ 测试通过"
else
    echo "   ❌ 测试失败"
fi
echo ""

# 测试4：性能测试
echo "5. 测试4：性能测试（时间限制）"
start=$(date +%s.%N)
result=$(echo -e "1\n-1 -1" | ./nogo)
end=$(date +%s.%N)
elapsed=$(echo "$end - $start" | bc)
echo "   运行时间: $elapsed 秒"
if (( $(echo "$elapsed < 1.0" | bc -l) )); then
    echo "   ✅ 性能测试通过（< 1秒）"
else
    echo "   ❌ 性能测试失败（>= 1秒）"
fi
echo ""

# 测试5：多次运行稳定性
echo "6. 测试5：多次运行稳定性"
success_count=0
for i in {1..10}; do
    result=$(echo -e "1\n-1 -1" | ./nogo)
    if [ "$result" != "" ]; then
        ((success_count++))
    fi
done
echo "   成功次数: $success_count/10"
if [ $success_count -eq 10 ]; then
    echo "   ✅ 稳定性测试通过"
else
    echo "   ❌ 稳定性测试失败"
fi
echo ""

echo "========================================="
echo "  测试完成"
echo "========================================="
```

## 九、总结

### 测试清单

- [ ] 编译测试
- [ ] 空棋盘开局测试
- [ ] 对方先手测试
- [ ] 对局中段测试
- [ ] 边界测试
- [ ] 性能测试（< 1秒）
- [ ] 稳定性测试
- [ ] 内存占用测试（< 256MB）
- [ ] Botzone平台测试

### 快速命令

```bash
# 一键测试
./test.sh

# 单独测试
echo -e "1\n-1 -1" | ./nogo

# 性能测试
echo -e "1\n-1 -1" | time ./nogo

# 编译
g++ -std=c++17 -O2 main.cpp -o nogo
```

---

## 十、duel_nogo.py 对战脚本

### 10.1 脚本简介

`duel_nogo.py` 是一个全自动的对战测试脚本，用于在不围棋中对比两个 Bot 的强度。它支持：

- 自动编译 `.cpp` 源文件或直接运行可执行文件
- 通过子进程 stdin/stdout 模拟 Botzone 通信协议
- 独立判定合法性和终局条件（不信任 Bot 内部规则）
- 记录完整着法序列、终局棋盘、胜负原因
- 自动交替先手，消除先后手偏差

### 10.2 基本用法

```bash
python3 duel_nogo.py <A.cpp> <B.cpp> [选项]
```

#### 必填参数

| 参数 | 说明 |
|------|------|
| `bot_a` | Bot A 的 `.cpp` 源文件或已编译的可执行文件路径 |
| `bot_b` | Bot B 的 `.cpp` 源文件或已编译的可执行文件路径 |

#### 可选参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--games N` | 20 | 对局数。奇数局 A 执黑先手，偶数局 B 执黑先手 |
| `--timeout T` | 1.2 | 每步超时限制（秒），超时判负 |
| `--save-log <文件>` | 无 | 将完整对战日志保存为 JSON 文件 |
| `--flags-a "..."` | 空 | Bot A 的额外编译参数，如 `-ljsoncpp` |
| `--flags-b "..."` | 空 | Bot B 的额外编译参数 |
| `--verbose` | 关 | 逐步打印着法坐标 |
| `--build-dir <目录>` | `.duel_build` | 编译产物存放目录 |

### 10.3 使用示例

```bash
# 基础：first.cpp vs new1.cpp，对局 30 局
python3 duel_nogo.py first.cpp new1.cpp --games 30

# 自对弈：验证 Bot 无 bug（应接近 50%）
python3 duel_nogo.py new1.cpp new1.cpp --games 4

# 新版 vs 旧版回归测试 + 保存日志
python3 duel_nogo.py new1_v2.cpp new1.cpp --games 50 --save-log v2_vs_now.json

# Bot 依赖 jsoncpp 库
python3 duel_nogo.py bot.cpp new1.cpp --games 10 --flags-a "-ljsoncpp"

# 调试模式：逐步查看着法
python3 duel_nogo.py first.cpp new1.cpp --games 2 --verbose

# 自定义编译产物目录
python3 duel_nogo.py first.cpp new1.cpp --build-dir build_tmp

# 使用预编译的二进制
python3 duel_nogo.py ./build/bot_a.out ./build/bot_b.out --games 20
```

### 10.4 输出解读

#### 终端输出

```
Game 001 | A=black | winner=   B | moves= 0 | black_timeout
Game 002 | A=white | winner=   A | moves=59 | white_illegal_move
Game 003 | A=black | winner=   A | moves=72 | black_no_legal_move

Summary
A wins        : 2
B wins        : 1
A as black    : 1
A as white    : 1
A win rate    : 66.67%
B win rate    : 33.33%
```

- **A=black**：A 本局执黑
- **winner=A/B**：胜方
- **moves**：总着法数（一个回合算两步）
- **失败原因**：
  - `timeout`：超时
  - `illegal_move:X,Y`：在 (X,Y) 落了不合法位置
  - `no_legal_move`：无可走之处，规则判负

#### 日志格式（`--save-log`）

```json
{
  "bot_a": "/path/to/bot_a.cpp",
  "bot_b": "/path/to/bot_b.cpp",
  "games": 50,
  "timeout": 1.0,
  "summary": {
    "A_wins": 45,
    "B_wins": 5,
    "A_black_wins": 22,
    "A_white_wins": 23,
    "B_black_wins": 3,
    "B_white_wins": 2
  },
  "games_detail": [
    {
      "game": 1,
      "A_color": "black",
      "winner": "A",
      "reason": "white_illegal_move",
      "moves": 31,
      "history": [[7,8], [8,0], [6,7], [6,0], ...],
      "final_board": [[1,-1,0,...], ...]
    }
  ]
}
```

### 10.5 迭代测试工作流

建议的迭代流程，每次优化后做新旧对比：

```bash
# 1. 保存当前版本为快照
cp new1.cpp new1_v3.cpp

# 2. 修改 new1.cpp ...

# 3. 新版 vs 旧版
python3 duel_nogo.py new1_v3.cpp new1.cpp --games 50 --save-log v3_vs_v4.json

# 4. 确认无退化后再提交
git add new1.cpp
git commit -m "optimize: xxx"
```

### 10.6 工作原理

**通信协议**：模拟 Botzone 平台的 JSON 通信

- **输入**（Bot 收到）：`{"requests":[{"x":-1,"y":-1},...],"responses":[{"x":4,"y":4},...]}`
- **输出**（Bot 返回）：`{"response":{"x":X,"y":Y}}`

**裁判逻辑**：
1. 脚本独立维护一份棋盘
2. 每次收到 Bot 的着法后，用脚本内置的规则判定器校验合法性
3. 着法不合法 → 对方胜，原因 `illegal_move`
4. 超时 → 对方胜，原因 `timeout`
5. 无可走之处 → 当前方负，原因 `no_legal_move`

**注意事项**：
- 脚本不信任 Bot 内部的规则判断，所有合法性由脚本外挂判定
- 先后手自动交替：第 1 局 A 先手，第 2 局 B 先手，依此类推
- 编译时自动检测 `jsoncpp/json.h` 引用并添加编译参数（但建议迁移为手动解析以避免依赖问题）

---

**文档版本**：v1.1  
**最后更新**：2026-05-13
