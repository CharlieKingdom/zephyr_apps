# RISC-V 32 位汇编学习工程（Zephyr + QEMU）
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(riscv_asm_learn)

target_sources(app PRIVATE
  src/main.c
  src/asm_funcs.S
)