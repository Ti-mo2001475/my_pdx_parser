#!/usr/bin/env bash
# build.sh - build.bat 的 macOS 版本
# 用法:
#   ./build.sh            # 等价于 build.bat 无参数:配置并构建默认目标 console
#   ./build.sh print_modifier   # 配置并构建指定目标 (console / print_modifier / print_trigger)
#   ./build.sh clean      # 清理构建产物 (不复制可执行文件)
set -euo pipefail

# 切换到脚本所在的项目根目录,保证从任意目录执行都正确
cd "$(dirname "$0")"

# macOS 上没有 .exe 后缀
mkdir -p build
cd build

TARGET="${1:-console}"

# 每次都重新配置(与 build.bat 行为一致);默认使用 Unix Makefiles + clang++
cmake ..

if [ "$TARGET" = "clean" ]; then
    make clean
    exit 0
fi

make "$TARGET"
cp "$TARGET" "../$TARGET"
echo "构建完成,可执行文件已复制到 ../$TARGET"
