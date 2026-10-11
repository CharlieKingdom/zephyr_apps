# RISC-V 32 位汇编学习工程（Zephyr + QEMU）
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(riscv_asm_learn)

target_sources(app PRIVATE
  src/main.c
  src/asm_funcs.S
)

# CONFIG_LINKER_ORPHAN_SECTION_ERROR=y（见 prj.conf）本应注入 --orphan-handling=error，
# 但 Zephyr 的工具链能力检查会编译一个探测程序并用【默认】链接脚本试链：
# 探测程序自带的 .riscv.attributes 段在默认脚本下是孤儿段，error 模式使试链失败，
# 于是该旗标被误判为"工具链不支持"而静默丢弃（缓存在 ~/.cache/zephyr/ToolchainCapabilityDatabase）。
# Zephyr 自己的 linker.ld 显式放置了 .riscv.attributes，实际链接不会踩这个坑。
# 故在此直接注入裸旗标，绕过能力检查，让孤儿段在链接期就报 error。
zephyr_link_libraries(-Wl,--orphan-handling=error)