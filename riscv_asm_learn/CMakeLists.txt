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

# CONFIG_OUTPUT_DISASSEMBLY=y（见 prj.conf）让每次链接后自动生成 zephyr.lst，
# 默认命令是 objdump -d -S。此处给 bintools 的反汇编旗标追加 -h：
# .lst 顶部先打印 ELF 段表（rom_start/text/datas/bss 等段的 Size/VMA/LMA/标志），
# 往下才是反汇编正文，一份文件同时看清"段级内存分布"与"指令编码"。
set_property(TARGET bintools PROPERTY disassembly_flag -d -h)