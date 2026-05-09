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

# 测试1：Botzone格式 - 空棋盘开局
echo "2. 测试1：Botzone格式 - 空棋盘开局（我方先手）"
result=$(echo '{"requests":[{"x":-1,"y":-1}],"responses":[]}' | ./nogo)
echo "   输出: $result"
if [ "$result" != "" ]; then
    echo "   ✅ 测试通过"
else
    echo "   ❌ 测试失败"
fi
echo ""

# 测试2：Botzone格式 - 对方先手
echo "3. 测试2：Botzone格式 - 对方先手（我方后手）"
result=$(echo '{"requests":[{"x":4,"y":4},{"x":-1,"y":-1}],"responses":[]}' | ./nogo)
echo "   输出: $result"
if [ "$result" != "" ]; then
    echo "   ✅ 测试通过"
else
    echo "   ❌ 测试失败"
fi
echo ""

# 测试3：Botzone格式 - 对局中段
echo "4. 测试3：Botzone格式 - 对局中段"
result=$(echo '{"requests":[{"x":4,"y":4},{"x":3,"y":4},{"x":4,"y":3},{"x":-1,"y":-1}],"responses":[]}' | ./nogo)
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
result=$(echo '{"requests":[{"x":-1,"y":-1}],"responses":[]}' | ./nogo)
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
    result=$(echo '{"requests":[{"x":-1,"y":-1}],"responses":[]}' | ./nogo)
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
