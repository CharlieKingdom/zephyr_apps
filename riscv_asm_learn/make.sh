#!/usr/bin/env bash
# riscv_asm_learn 编译脚本
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

# 编译本 app，输出到 build/build_qemu_riscv32_asm
# -p always：每次先清空 build 目录再全量重编（无视文件是否改动）
west build -p always -b qemu_riscv32 apps/riscv_asm_learn -d build/build_qemu_riscv32_asm -t run