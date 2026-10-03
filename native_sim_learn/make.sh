#!/usr/bin/env bash
# native_sim_learn 编译脚本
# 用法：./make.sh
set -euo pipefail

# 切到 west 工作区根目录（本脚本的上上级目录，即 zephyrproject/）
cd "$(dirname "$(readlink -f "$0")")/../.."

# 激活 Zephyr 的 Python 虚拟环境（保证非交互式执行也能找到 west）
if [ -f .venv/bin/activate ]; then
  # shellcheck disable=SC1091
  source .venv/bin/activate
fi
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

# 编译本 app，输出到 build/build_native_sim_learn
# -p always：每次先清空 build 目录再全量重编（无视文件是否改动）
west build -p always -b native_sim apps/native_sim_learn -d build/build_native_sim_learn