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

**文档版本**：v1.0  
**最后更新**：2026-05-09
