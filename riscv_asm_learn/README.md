# RISC-V 32 位汇编学习程序技术文档

## 0 文档信息

### 0.1 文档标识

| 项目 | 内容 |
| --- | --- |
| 文档标题 | RISC-V 32 位汇编学习程序技术文档 |
| 文档编号 | RVASM-DOC-001 |
| 版本 | 2.0 |
| 修订日期 | 2026-10-15 |
| 文档状态 | 已发布 |
| 适用软件版本 | Zephyr RTOS v4.1.0 |
| 适用目标板 | qemu_riscv32 |

### 0.2 修订记录

| 版本 | 日期 | 修订说明 |
| --- | --- | --- |
| 1.0 | 2026-10-13 | 初版 |
| 2.0 | 2026-10-15 | 按 ASD-STE100 规范重构文档结构与语言 |

### 0.3 信息等级定义

| 等级 | 标记 | 适用场景 |
| --- | --- | --- |
| 警告 | **警告** | 操作可能导致数据丢失或环境损坏 |
| 注意 | **注意** | 操作可能导致构建失败或调试异常 |
| 注释 | **注释** | 补充说明信息，不影响操作结果 |

本文档为纯软件模拟环境，不涉及人身安全风险。全文未使用"警告"等级。

### 0.4 术语约定

本文档使用受控术语。同一概念全文只使用一个名称。

| 术语 | 定义 |
| --- | --- |
| 构建 | `west build` 命令执行的完整过程，含配置、编译、链接 |
| 编译 | 编译器将源码转换为目标文件的动作 |
| 目标板 | 构建时的硬件目标。本文档中为 `qemu_riscv32` |
| 工作区 | west 管理的源码根目录，即 `zephyrproject/` |
| 虚拟环境 | Zephyr 的 Python 虚拟环境，位于 `.venv/` |
| 反汇编清单 | 链接后生成的 `zephyr.lst` 文件 |
| 段 | ELF 文件的基本组成单位 |
| 输入段 | 源文件或编译器产生的段 |
| 输出段 | 链接脚本收集生成的段 |
| 指示符 | 以 `.` 开头的汇编器命令，不产生机器码 |
| 伪指令 | CPU 中无对应硬件编码的汇编写法。汇编器将其替换为真指令 |
| 真指令 | CPU 直接执行的指令，有硬件编码 |
| 叶子函数 | 不调用其他函数的函数 |
| 调用约定 | 寄存器与栈的使用规则。本文档中为 ilp32 ABI |
| 交叉编译器 | 运行在 x86 主机上、生成 RISC-V 机器码的编译器 |
| QEMU | 在 x86 主机上模拟 RISC-V CPU 的模拟器 |

### 0.5 目录

1. [概述](#1-概述)
2. [适用范围](#2-适用范围)
3. [参考资料](#3-参考资料)
4. [前置条件](#4-前置条件)
5. [系统说明](#5-系统说明)
6. [构建程序](#6-构建程序)
7. [运行程序](#7-运行程序)
8. [反汇编清单](#8-反汇编清单)
9. [调试程序（GDB）](#9-调试程序gdb)
10. [汇编代码技术说明](#10-汇编代码技术说明)
11. [故障排除](#11-故障排除)
12. [文件结构](#12-文件结构)

---

## 1 概述

### 1.1 程序用途

本程序运行于 QEMU 模拟的 RISC-V 32 位目标板。程序由 C 语言与汇编语言组成。C 代码调用 5 个汇编函数。函数计算结果通过串口输出。

本程序用于学习以下内容：

- RISC-V 32 位汇编语言基础
- C 语言与汇编语言的混合编程
- Zephyr RTOS 的构建与运行机制
- 反汇编与 GDB 调试方法

### 1.2 技术特性

| 特性 | 配置 |
| --- | --- |
| 目标板 | `qemu_riscv32`（无需真实硬件） |
| 指令集 | `rv32imac_zicsr_zifencei`（32 位，含 M/A/C 扩展） |
| 调用约定 | `ilp32`（软浮点） |

### 1.3 程序组成

| 文件 | 功能 |
| --- | --- |
| [src/main.c](src/main.c) | C 主函数。调用汇编函数并打印结果 |
| [src/asm_funcs.S](src/asm_funcs.S) | 5 个汇编函数 |
| [prj.conf](prj.conf) | Kconfig 配置文件 |
| [CMakeLists.txt](CMakeLists.txt) | 构建配置文件 |
| `make.sh` | 一键构建与运行脚本 |

5 个汇编函数如下：

| 函数 | 功能 |
| --- | --- |
| `asm_add(a, b)` | 加法 |
| `asm_sub(a, b)` | 减法 |
| `asm_mul(a, b)` | 乘法 |
| `asm_sum_to(n)` | 计算 1+2+…+n |
| `asm_factorial(n)` | 计算阶乘 n! |

---

## 2 适用范围

### 2.1 适用读者

本文档适用于以下读者：

- 具备 C 语言基础的嵌入式开发初学者
- 不要求读者具备 RISC-V 或汇编语言基础

### 2.2 适用环境

| 项目 | 要求 |
| --- | --- |
| 操作系统 | Ubuntu（x86_64） |
| 工作区路径 | `/home/ubuntu/workspace/zephyrproject` |
| Zephyr 工具链 | Zephyr SDK 0.17.0 |
| 构建工具 | west 1.5.0、CMake、Ninja |
| 模拟器 | qemu-system-riscv32 |

### 2.3 不适用范围

本文档不适用于以下场景：

- 真实 RISC-V 硬件的烧录与调试
- 64 位 RISC-V（rv64）目标板的构建

---

## 3 参考资料

| 编号 | 资料 | 说明 |
| --- | --- | --- |
| R1 | [RISC-V 指令集官方手册](https://github.com/riscv/riscv-isa-manual) | 全部指令的定义。RV32I 指令一页可查。伪指令清单见卷一附录 A |
| R2 | [Zephyr 上游 arch/riscv](https://github.com/zephyrproject-rtos/zephyr/tree/v4.1.0/arch/riscv) | 内核 RISC-V 启动与上下文切换源码（[reset.S](../../zephyr/arch/riscv/core/reset.S)、[switch.S](../../zephyr/arch/riscv/core/switch.S)） |
| R3 | [linker.ld](../../zephyr/include/zephyr/arch/riscv/common/linker.ld) | RISC-V 链接脚本（本地工作区） |
| R4 | [sections.h](../../zephyr/include/zephyr/linker/sections.h) | 输出段名宏定义（本地工作区） |
| R5 | [board.cmake](../../zephyr/boards/qemu/riscv32/board.cmake) | 目标板 QEMU 参数（本地工作区，上游 v4.1.0 同路径） |
| R6 | [asm_funcs.S](src/asm_funcs.S) | 5 个汇编函数源码 |
| R7 | [main.c](src/main.c) | C 侧调用源码 |

---

## 4 前置条件

### 4.1 软件清单

| 软件 | 来源 | 用途 |
| --- | --- | --- |
| `west` 1.5.0 | Zephyr 虚拟环境 | 构建命令入口 |
| `riscv64-zephyr-elf-*` 工具链 | Zephyr SDK | 将源码编译为 RISC-V 机器码 |
| `qemu-system-riscv32` | Ubuntu apt | 在 x86 主机上模拟 RISC-V CPU |
| Python 3 虚拟环境 | `zephyrproject/.venv` | 提供 west 运行环境 |

### 4.2 环境准备程序

按以下步骤准备环境。

1. 激活虚拟环境：

```bash
source /home/ubuntu/workspace/zephyrproject/.venv/bin/activate
```

> **注释**：`~/.bashrc` 已配置自动激活。新终端通常无需手动执行此步。

2. 设置工具链变量：

```bash
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
```

3. 确认 west 版本：

```bash
west --version
```

期望输出：`1.5.0`。

4. 确认工作区状态：

```bash
cd /home/ubuntu/workspace/zephyrproject
west list zephyr
```

命令必须正常返回，无报错。

5. 确认 QEMU 安装：

```bash
qemu-system-riscv32 --version
```

若系统提示找不到命令，执行以下命令安装：

```bash
sudo apt-get install -y qemu-system-misc
```

### 4.3 两个 RISC-V 工具的区别

> **注意**：不要混淆以下两个工具。两者名称相似，来源与功能不同。

| 工具 | 来源 | 功能 |
| --- | --- | --- |
| `riscv64-zephyr-elf-*` | Zephyr SDK | 交叉编译器。将源码编译为 RISC-V 机器码 |
| `qemu-system-riscv32` | Ubuntu apt | 模拟器。在 x86 主机上模拟一颗 RISC-V CPU |

---

## 5 系统说明

本章说明 Zephyr 选择 RISC-V 架构与 QEMU 模拟器的机制。理解本章内容后，读者可以将本程序移植到其他目标板。

### 5.1 板级定义文件

构建命令中的 `-b qemu_riscv32` 使 Zephyr 读取 `zephyr/boards/qemu/riscv32/` 目录下的 4 个文件。

| 文件 | 作用 |
| --- | --- |
| `qemu_riscv32.yaml` | 目标板元信息：`arch: riscv`、RAM 与 Flash 大小、`type: qemu` |
| `Kconfig.defconfig` | 目标板默认配置（如启用 TLS） |
| `qemu_riscv32.dts` | 设备树：SoC 选择、串口地址、内存映射 |
| `board.cmake` | QEMU 启动参数（见 5.5 节） |

### 5.2 架构选择

流程如下：

1. `qemu_riscv32.yaml` 声明 `arch: riscv`；
2. Zephyr 载入 [arch/riscv](https://github.com/zephyrproject-rtos/zephyr/tree/v4.1.0/arch/riscv)（R2）；
3. Zephyr 使用 RISC-V 架构的启动代码。启动代码包括 `reset.S`、trap 处理、`switch.S` 上下文切换。

### 5.3 内存与设备配置

`qemu_riscv32.dts` 引入 `virt-riscv32.dtsi`。该文件定义以下内容：

```dts
ram0: memory@80000000 { reg = <0x80000000 0x10000000>; };  /* 256MB RAM */
uart0: uart@10000000  { compatible = "ns16550"; ... };     /* 串口 */
```

内存布局如下：

| 设备 | 地址 | 说明 |
| --- | --- | --- |
| RAM | `0x80000000` 起，共 256 MB | 程序加载与运行区 |
| 串口 | `0x10000000` | ns16550 兼容串口 |

此布局为 QEMU `virt` 机器的标准布局。CPU 节点的 `riscv,isa` 属性声明指令集扩展。

### 5.4 工具链选择

目标板配置选中 M、A、C 扩展。最终编译参数为：

```
-march=rv32imac_zicsr_zifencei     ← 生成 RV32 指令
-mabi=ilp32                        ← 32 位软浮点调用约定
```

编译器为 SDK 中的 `riscv64-zephyr-elf-gcc`。该编译器前缀为 `riscv64-`，可同时生成 rv32 与 rv64 两种目标代码。

验证方法：构建完成后执行以下命令。

```bash
grep -o 'march=[^ ]*' build/build_qemu_riscv32_asm/compile_commands.json | head -1
grep -o 'mabi=[^ ]*'  build/build_qemu_riscv32_asm/compile_commands.json | head -1
```

### 5.5 QEMU 启动参数

[board.cmake](https://github.com/zephyrproject-rtos/zephyr/blob/v4.1.0/boards/qemu/riscv32/board.cmake)（R5）定义以下参数：

```cmake
set(QEMU_binary_suffix riscv32)          # → 使用 qemu-system-riscv32
set(QEMU_CPU_TYPE_${ARCH} riscv32)
set(QEMU_FLAGS_${ARCH}
  -nographic      # 不使用图形界面，串口直连当前终端
  -machine virt   # QEMU 的 "virt" 通用虚拟平台
  -bios none      # 不加载 BIOS，直接运行编译出的 ELF
  -m 256          # 256MB 内存
)
```

执行 `west build -t run` 时，Zephyr 拼装出完整命令（可在构建日志中查看）：

```bash
qemu-system-riscv32 -nographic -machine virt -bios none -m 256 \
  -kernel build/build_qemu_riscv32_asm/zephyr/zephyr.elf
```

参数选择机制：

| 参数 | 作用 | 决定因素 |
| --- | --- | --- |
| 使用 `qemu-system-riscv32` 程序 | 选择模拟器 | `QEMU_binary_suffix=riscv32` |
| `-machine virt` | 选择虚拟平台 | `QEMU_FLAGS` |
| `-kernel <ELF>` | 指定运行的程序 | 构建产物路径 |

---

## 6 构建程序

### 6.1 方式 A：脚本构建（推荐）

按以下步骤执行。

1. 进入应用目录：

```bash
cd /home/ubuntu/workspace/zephyrproject/apps/riscv_asm_learn
```

2. 执行构建脚本：

```bash
./make.sh
```

3. 确认构建结果。输出必须包含内存用量与 5 个函数的计算结果（见 6.5 节与 7.1 节）。

### 6.2 构建脚本说明

`make.sh` 将"切换目录、激活环境、构建、运行"合并为一条命令。脚本全文如下：

```bash
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
```

各语句作用如下：

| 语句 | 作用 |
| --- | --- |
| `set -euo pipefail` | 出错即停。任一命令失败、引用未定义变量、管道中任一段失败时，脚本立即退出 |
| `cd "$(dirname "$(readlink -f "$0")")/../.."` | 定位脚本自身路径，再切换到工作区根目录。因此从任何目录调用脚本，相对路径均有效 |
| `source .venv/bin/activate` | 激活虚拟环境。保证非交互执行（如 IDE 任务）也能找到 `west` 与 SDK |
| `export ZEPHYR_TOOLCHAIN_VARIANT=zephyr` | 指定使用 Zephyr SDK 工具链 |
| `west build ...` | 执行构建。参数见 6.3 节 |

### 6.3 构建命令参数

脚本核心命令如下：

```bash
west build -p always -b qemu_riscv32 apps/riscv_asm_learn \
           -d build/build_qemu_riscv32_asm -t run
```

| 参数 | 含义 |
| --- | --- |
| `west build` | Zephyr 构建命令。底层使用 CMake 与 Ninja |
| `-p always` | 构建前清空构建目录，再全量重建。保证结果干净、可复现 |
| `-b qemu_riscv32` | 指定目标板为 `qemu_riscv32`。此参数决定 arch、SoC、DTS、工具链的选择 |
| `apps/riscv_asm_learn` | 应用源码目录。目录含 `CMakeLists.txt`、`prj.conf`、`src/` |
| `-d build/build_qemu_riscv32_asm` | 构建输出目录。命名规则：`build_<目标板>_<用途>` |
| `-t run` | 构建完成后立即在 QEMU 中运行（见第 7 章） |

`-p` 参数有三种取值：

| 取值 | 行为 |
| --- | --- |
| `always` | 每次构建前清空构建目录，全量重建 |
| `auto` | 仅在更换目标板或应用时清空构建目录 |
| `never` | 纯增量构建，默认值 |

> **注释**：日常快速迭代时，可将脚本中 `-p always` 改为 `-p auto`，缩短构建时间。

### 6.4 方式 B：手动构建

不使用脚本时，按以下步骤执行。

1. 进入工作区根目录：

```bash
cd /home/ubuntu/workspace/zephyrproject
```

2. 执行构建命令：

```bash
west build -b qemu_riscv32 apps/riscv_asm_learn \
           -d build/build_qemu_riscv32_asm
```

### 6.5 构建结果确认

构建成功时显示以下输出：

```
Memory region         Used Size  Region Size  %age Used
             RAM:       24688 B       256 MB      0.01%
Generating files from .../build_qemu_riscv32_asm/zephyr/zephyr.elf for board: qemu_riscv32
```

确认产物为 RISC-V 32 位程序：

```bash
file build/build_qemu_riscv32_asm/zephyr/zephyr.elf
```

期望输出：

```
ELF 32-bit LSB executable, UCB RISC-V, RVC, soft-float ABI, ... not stripped
```

### 6.6 输出文件

| 文件 | 说明 |
| --- | --- |
| `build/build_qemu_riscv32_asm/zephyr/zephyr.elf` | 可执行的 RISC-V ELF 文件 |
| `build/build_qemu_riscv32_asm/zephyr/zephyr.map` | 内存布局（符号级） |
| `build/build_qemu_riscv32_asm/zephyr/zephyr.lst` | 反汇编清单（段表 + 源码混排，见第 8 章） |
| `build/build_qemu_riscv32_asm/zephyr/zephyr.stat` | ELF 头信息（readelf -e 输出） |
| `build/build_qemu_riscv32_asm/zephyr/.config` | 最终生效的 Kconfig 配置 |

---

## 7 运行程序

> **注释**：`./make.sh` 已内置 `-t run`，构建后自动运行。本章适用于"只运行、不重新构建"的场景。

### 7.1 方式 A：west 运行（推荐）

1. 进入工作区根目录：

```bash
cd /home/ubuntu/workspace/zephyrproject
```

2. 执行运行命令：

```bash
west build -d build/build_qemu_riscv32_asm -t run
```

3. 确认输出。期望输出如下：

```
*** Booting Zephyr OS build v4.1.0 ***
=== RISC-V RV32 汇编学习示例 ===
asm_add(12, 5)       = 17
asm_sub(12, 5)       = 7
asm_mul(12, 5)       = 60
asm_sum_to(10)        = 55
asm_factorial(6)     = 720
=== 演示结束 ===
```

### 7.2 方式 B：手动运行

手动执行 QEMU 命令，用于理解运行原理。

1. 定义命令变量：

```bash
cd /home/ubuntu/workspace/zephyrproject
QEMU=$HOME/zephyr-sdk-0.17.0/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-riscv32
ELF=build/build_qemu_riscv32_asm/zephyr/zephyr.elf
```

2. 执行 QEMU：

```bash
$QEMU -nographic -machine virt -bios none -m 256 -kernel $ELF
```

> **注释**：方式 A 与方式 B 完全等价。方式 B 可直观看到 QEMU 接收的全部参数。参数来源见 5.5 节。

### 7.3 退出方法

QEMU 退出方法：

1. 按下 `Ctrl-A`；
2. 松开 `Ctrl-A`；
3. 按下 `X`。

> **注释**：程序打印结束后 QEMU 不自动退出。Zephyr 程序执行完 `main` 后进入空闲线程，QEMU 持续运行。此为正常现象。

---

## 8 反汇编清单

### 8.1 自动生成机制

[prj.conf](prj.conf) 中的 `CONFIG_OUTPUT_DISASSEMBLY=y` 使构建系统在每次链接后自动运行 objdump，生成反汇编清单 `build/build_qemu_riscv32_asm/zephyr/zephyr.lst`。文件约 250 KB。无需手动执行命令。

[CMakeLists.txt](CMakeLists.txt) 为反汇编命令追加了 `-h` 参数。因此该文件由两部分组成：

| 部分 | 内容 | 来源 |
| --- | --- | --- |
| ① 段表 | 每个段的名称、大小、地址、对齐、标志 | objdump `-h` |
| ② 反汇编正文 | 按段按函数排列，源码与机器码混排 | objdump `-d -S` |

### 8.2 段表说明

段表节选如下：

```
Idx Name          Size      VMA       LMA       File off  Algn
  0 rom_start     00000018  80000000  80000000  000000c0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  3 text          0000269e  80000174  80000174  00000234  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
 12 bss           00000234  80005000  80005000  00004d28  2**3
                  ALLOC
 13 noinit        00000e00  80005240  80005240  00004d28  2**4
                  ALLOC
 14 datas         0000002c  80006040  80006040  00004d28  2**2
                  CONTENTS, ALLOC, LOAD, DATA
```

各列含义：

| 列 | 含义 |
| --- | --- |
| VMA | 程序运行时段所在的内存地址 |
| LMA | 加载器将该段写入内存的地址 |
| File off | 段内容在 zephyr.elf 文件内的偏移 |
| Size | 段大小 |
| Algn | 对齐要求。`2**3` 表示 8 字节对齐 |

补充说明：

- `text` 段起始地址为 0x80000174。该地址 = RAM 基址 0x80000000（见 5.3 节）+ 启动段大小 0x174 字节（rom_start + reset + exceptions）。启动链的先后顺序体现在地址中。
- qemu_riscv32 不使用 XIP。ROM 与 RAM 为同一块内存。因此 VMA 等于 LMA。真实硬件上常见 rodata 段的 LMA 位于 Flash、VMA 位于 RAM。

标志行含义：

| 标志 | 含义 |
| --- | --- |
| `CONTENTS` | 段内容实际存储在 ELF 文件中 |
| `ALLOC` | 段在运行时占用内存 |
| `LOAD` | 加载器负责搬运该段 |

对照分析：`bss` 与 `noinit` 段只有 `ALLOC` 标志，没有 `CONTENTS` 与 `LOAD` 标志。这两个段在运行时占用 RAM，但不占用固件体积。启动代码将 bss 段逐字节清零，对 noinit 段不做初始化。main.c 中未定义大型全局数组，但 bss 与 noinit 合计约 4 KB。该大小来自 Zephyr 静态工作区与全部线程栈。

地址为 0 的 `.debug_*`、`.comment`、`.riscv.attributes` 段没有 `ALLOC` 标志。这些段为元数据，供 GDB 与工具链使用，不进入内存。

> **注释**：段表与 zephyr.map 描述同一对象的不同粒度。map 文件精确到符号（每个函数与变量的位置和大小）。段表精确到段（整块的起始地址与总大小）。建议先读段表建立整体认识，再查 map 文件确认具体符号。

### 8.3 段名来源

段表 Name 列显示的名称为 ELF 输出段名。输出段名定义于链接脚本 [linker.ld](../../zephyr/include/zephyr/arch/riscv/common/linker.ld)（R3）。构建时链接脚本展开为 `build/.../zephyr/linker.cmd`。

段名的形成链路如下：

```
源文件生产"输入段"          linker.ld 收集进"输出段"        .lst 段表显示
─────────────────────      ───────────────────────      ─────────────
.section .text      (asm_funcs.S)   *(.text)          ┐
编译器默认 .text    (main.c)        *(".text.*")      ├─→  text
                                               ┘
.section .vectors.__start (vector.S 的      KEEP(*(.vectors.*)) ──→ rom_start
  SECTION_FUNC(vectors,__start) 宏展开)
编译器默认 .data / .sdata    *(.data) *(.sdata*)  ──→  datas
```

输出段名的三种定义方式：

- 直接写在 linker.ld 中的字面量，如 `rom_start`；
- 名字宏。[sections.h](../../zephyr/include/zephyr/linker/sections.h)（R4）定义 `_TEXT_SECTION_NAME=text`、`_DATA_SECTION_NAME=datas`、`_BSS_SECTION_NAME=bss`、`_NOINIT_SECTION_NAME=noinit`；
- 构建系统生成的链接片段，如 `sw_isr_table`、`initlevel`、`device_area`。这些段由 `zephyr/linker/common-rom.ld` 与中断表生成脚本引入。

输入段名的三种生产者：

| 生产者 | 示例 |
| --- | --- |
| 编译器默认段 | `.text`、`.rodata`、`.data`、`.bss`（带点前缀） |
| 汇编手写 `.section` | [asm_funcs.S](src/asm_funcs.S) 的 `.section .text` |
| Zephyr 宏 | `SECTION_FUNC(sect, sym)` 展开为 `.section .sect.sym`；`STRUCT_SECTION_ITERABLE` 生成 `._init.*`、`._device.static.*` 等 |

> **注意**：汇编手写段名必须与链接脚本中的收集规则匹配。段名不匹配时该段成为孤儿段。[prj.conf](prj.conf) 已开启 `--orphan-handling=error`，孤儿段在链接期直接报错。

> **注释**：数据输出段名为 `datas` 而非 `data`。原因记录在 sections.h 注释中：XIP 场景下输出段名 `data` 与链接器内建用法冲突。名称 `datas` 为历史约定，沿用至今。

### 8.4 反汇编正文说明

反汇编正文将汇编指令与实际机器码对照显示。示例如下：

```
80000174 <asm_add>:
80000174:  952e      add  a0,a0,a1
80000176:  8082      ret
```

左侧为地址与机器码，右侧为助记符。

两项观察结果：

- `add` 指令只占 2 字节（0x952e），不占 4 字节。原因：`-march` 包含 C 扩展，实际存储为压缩指令 `c.add`。两种编码语义相同（见 10.1 节与 5.4 节）。
- `ret` 的机器码 0x8082 为压缩指令 `c.jr ra`，语义等于 `jalr x0, 0(ra)`。CPU 中不存在名为 `ret` 的硬件指令。此为伪指令的直接证据。

参数说明：

- `-S` 参数使源码与机器码逐行混排。对应配置项 `CONFIG_OUTPUT_DISASSEMBLY_WITH_SOURCE`，默认开启；
- `-d` 参数只反汇编含指令的段（rom_start、reset、exceptions、text）。数据段不进入正文。

### 8.5 手动反汇编命令

只查看段表或只查看单个函数时，手动命令更快捷。

```bash
OBJDUMP=$HOME/zephyr-sdk-0.17.0/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-objdump
ELF=$HOME/workspace/zephyrproject/build/build_qemu_riscv32_asm/zephyr/zephyr.elf

$OBJDUMP -h $ELF                              # 只查看段表
$OBJDUMP -d $ELF                              # 只查看反汇编
$OBJDUMP -d --disassemble=asm_factorial $ELF  # 只查看指定函数（配合 10.6 节推演）
```

### 8.6 扩展选项

| 选项 | 作用 |
| --- | --- |
| `CONFIG_OUTPUT_DISASSEMBLE_ALL` | 连数据段一起反汇编（`-SDz`）。用于排查"数据被当作代码执行"类故障 |
| `zephyr.stat` | `readelf -e` 输出。提供 ELF 头、程序头、段头的另一视角，与反汇编清单互相印证 |

---

## 9 调试程序（GDB）

### 9.1 调试限制

> **注意**：不要使用 `west build -t debug`。QEMU 目标板不支持该命令。

原因说明：

1. `west debug` 与 `west flash` 依赖构建生成的 `runners.yaml` 文件；
2. `qemu_riscv32` 的 `board.cmake` 不注册可用 runner。Zephyr 的 `qemu` runner 为占位空壳，不支持任何命令；
3. 执行 `west build -t debug` 报错：`FATAL ERROR: no runners.yaml found`。

> **注释**：`west build -t run` 可以正常使用。该命令由 CMake 直接启动 QEMU，不经过 runner 机制。QEMU 目标板调试使用本章的双终端手动方法。

### 9.2 调试步骤

调试前先完成构建（见第 6 章）。

**终端 A：启动 QEMU**

1. 定义变量并启动 QEMU：

```bash
QEMU=$HOME/zephyr-sdk-0.17.0/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-riscv32
ELF=$HOME/workspace/zephyrproject/build/build_qemu_riscv32_asm/zephyr/zephyr.elf
$QEMU -nographic -machine virt -bios none -m 256 -s -S -kernel $ELF
```

参数说明：`-s` 开启 gdbstub，监听端口 1234。`-S` 使 QEMU 启动后立即暂停，等待调试器连接。

**终端 B：连接 GDB**

> **注意**：shell 变量只在定义它的终端内有效。终端 A 定义的 `QEMU` 与 `ELF` 在终端 B 中不存在。终端 B 必须重新定义 `ELF`。GDB 依靠该变量加载符号表。省略该变量时，GDB 启动警告 `No executable has been specified`，`break` 命令报错 `No symbol table is loaded`。

1. 启动 GDB 并加载符号表：

```bash
GDB=$HOME/zephyr-sdk-0.17.0/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-gdb
ELF=$HOME/workspace/zephyrproject/build/build_qemu_riscv32_asm/zephyr/zephyr.elf
$GDB $ELF
```

2. 连接 QEMU 并执行调试命令：

```
(gdb) target remote :1234
(gdb) break asm_add          # 在汇编函数上设断点
(gdb) continue
(gdb) info registers a0 a1   # 查看参数寄存器
(gdb) stepi                  # 单步执行一条指令
(gdb) info registers a0      # 查看结果
(gdb) x/4i $pc               # 反汇编接下来的指令
```

调试方法：将断点设置在目标函数上。用 `stepi` 逐条执行。用 `info registers` 观察寄存器变化。

### 9.3 常见调试现象

| 现象 | 说明与处理 |
| --- | --- |
| 连接后停在 `0x00001000 in ?? ()` | 正常现象。该地址为 QEMU `virt` 机器的复位入口。QEMU 执行内置 ROM 后跳转至 ELF 入口 `0x80000000`。执行 `continue` 即可 |
| `break` 时提示 `Make breakpoint pending on future shared library load?` | 回答 **n**。裸机静态程序没有共享库。回答 y 时断点永不生效 |

---

## 10 汇编代码技术说明

本章说明 5 个汇编函数的技术细节。汇编源码见 [asm_funcs.S](src/asm_funcs.S)（R6），C 侧调用见 [main.c](src/main.c)（R7）。

本章约定：每条指令回答两个问题——它执行什么操作；为什么需要该操作。全部结论可通过第 8 章反汇编清单与第 9 章 GDB 调试验证。

### 10.1 基本概念

**概念一：汇编是机器码的可读文本形式。**

CPU 只能识别二进制编码。汇编语言将每条机器码替换为一个助记符。对应关系以加法为例：

| 层次 | 编写者 | 形式 |
| --- | --- | --- |
| C 语言 | 人 | `return a + b;` |
| 汇编 | 人编写，汇编器翻译 | `add a0, a0, a1` |
| 机器码 | CPU 执行 | 一串二进制数 |

一行汇编对应一条 CPU 指令。C 语言的一行通常被编译器转换为一行或多行汇编。

`add a0, a0, a1` 的 32 位编码按 RISC-V R 型格式划分：

```
 funct7     rs2      rs1    funct3    rd      opcode
┌────────┬────────┬────────┬───────┬────────┬─────────┐
│0000000 │  01011 │  01010 │  000  │  01010 │ 0110011 │ → add a0, a0, a1
└────────┴────────┴────────┴───────┴────────┴─────────┘
  "是加法"   x11      x10      加     x10     "寄存器-
             =a1      =a0             =a0     寄存器运算"
```

说明：

- 每段 5 位，可编码 32 个寄存器之一（2^5 = 32）；
- 汇编器的功能是"助记符到二进制编码"的翻译；
- 本程序 ELF 中实际存储的是 16 位压缩编码 `c.add`。原因：`-march` 包含 C 扩展（见 5.4 节）。两种编码 CPU 均可执行，效果相同。验证方法见第 8 章。

**概念二：寄存器是 CPU 内部的高速存储单元。**

内存容量大，但与运算器的距离远。CPU 执行加减与比较前，必须先将数据装入内部存储单元。该存储单元即寄存器。RV32 有 32 个寄存器。每个寄存器存储一个 32 位整数。

本程序的全部运算在寄存器之间完成。仅 asm_factorial 使用内存（栈）。

寄存器有两套名称：编号名 `x0`–`x31`，别名 `zero/ra/sp/a0/t0/s0...`。编写代码使用别名。别名直接指示寄存器用途（见 10.2 节）。

**概念三：一行汇编的读取顺序——目标寄存器写在最前面。**

```
add     a0, a0, a1
┬┘     ┬┘   ┬┘  ┬┘
操作码  目标  源1  源2
```

读取规则：`a0 = a0 + a1`。结果始终写入第一个操作数。后续操作数为输入。此规则适用于 RISC-V 几乎全部运算指令。

> **注意**：x86 的 AT&T 语法目标操作数写在最后。查阅外部资料时注意区分。

汇编文件中存在两种"行"不是指令：

| 形式 | 示例 | 性质 |
| --- | --- | --- |
| `名字:` | `asm_add:` | 标签。为当前位置命名。不产生代码。跳转与调用依靠标签定位 |
| `.` 开头 | `.globl asm_add` | 汇编指示符。写给汇编器的命令。不转换为机器码 |

本程序使用的全部指示符：

| 指示符 | 作用 | 说明 |
| --- | --- | --- |
| `.section .text` | 将后续代码归入 .text 段 | 链接时排入 RAM 代码区 |
| `.globl asm_add` | 导出符号 | C 侧据此链接。缺失时链接报 undefined reference |
| `.type asm_add, @function` | 标记符号类型 | 供调试器与反汇编器使用的元信息 |
| `.size asm_add, .-asm_add` | 记录函数长度 | `.` 表示当前位置。当前位置减去起点即函数大小 |

**伪指令**：部分汇编写法在 CPU 中无对应硬件编码。汇编器在汇编阶段将其替换为真指令。`ret`、`li`、`mv`、`j`、`bgt`、`call` 均为伪指令。对照表见 10.5 节。

### 10.2 调用约定 ilp32

调用约定是 C 编译器与汇编函数之间的寄存器使用规则。规则约定：参数存放位置、返回值存放位置、寄存器的保存责任。双方遵守规则后，C 与汇编可以互相调用。

RV32 全部 32 个寄存器的分工：

| 寄存器 | 别名 | 名称含义 | 保存责任 | 本程序中的角色 |
| --- | --- | --- | --- | --- |
| x0 | zero | 恒零 | 读取恒为 0，写入等于丢弃 | `li` 借该寄存器生成常数 |
| x1 | ra | return address | 调用别人的函数自己保存（call 指令会覆盖它） | 10.6 节的主角 |
| x2 | sp | stack pointer | 函数进出必须保持栈平衡 | 开辟与回收栈帧 |
| x3 / x4 | gp / tp | 全局指针 / 线程指针 | 内核与链接器管理 | 本程序不使用 |
| x5–x7, x28–x31 | t0–t6 | temporary（临时） | 调用者保存。call 后视为已失效 | acc、i、临时值 |
| x8–x9, x18–x27 | s0–s11 | saved（长期保存） | 被调用者保存。使用后必须恢复原值 | 本程序未使用 |
| x10–x17 | a0–a7 | argument（参数） | 调用者保存 | a0 = 第 1 参数与返回值，a1 = 第 2 参数 |

保存责任规则：

- 调用者保存寄存器（t 系、a 系）：可在函数内任意使用。call 返回后不保证原值；
- 被调用者保存寄存器（s 系）：函数可以使用。退出前必须恢复原值；
- 使用原则：需要跨函数调用保留的值，存入 s 系寄存器或栈。用后即弃的中间值，存入 t 系寄存器。

`asm_add(12, 5)` 的完整调用过程：

```
 C 编译器（调用方）                        汇编函数（被调用方）
──────────────────────────              ──────────────────────────
1. 把 12 放进 a0，把 5 放进 a1
2. 执行 call asm_add
   （call 同时把"返回地址"写进 ra）  ──→   3. 按约定：第 1 个参数在 a0，
                                              第 2 个参数在 a1
                                         4. 执行运算：a0 = a0 + a1
                                         5. 按约定：结果留在 a0
                                         6. ret：跳转到 ra 记录的地址
←──────────────────────────
7. C 从 a0 取回返回值
```

记忆规则：参数进 a 系，返回值出 a0，临时值用 t 系，长期值用 s 系，返回地址存 ra，栈操作靠 sp。

规则的来源：`-mabi=ilp32`。字母含义：i = 32 位 int，l = 32 位 long，p = 32 位指针。即 32 位软浮点 ABI（见 5.4 节）。

### 10.3 函数 asm_add 与 asm_sub

asm_add 完整源码：

```asm
	.globl asm_add          /* 指示符：导出函数名，C 才能链接到 */
	.type  asm_add, @function
asm_add:                    /* 标签：函数入口地址的名字 */
	add     a0, a0, a1      /* 真指令：a0 = a0 + a1 */
	ret                     /* 伪指令：返回调用者 */
	.size  asm_add, .-asm_add
```

逐行说明：

| 行 | 说明 |
| --- | --- |
| `.globl` `.type` | 指示符，纯元信息（见 10.1 节） |
| `asm_add:` | 标签，标记函数第一行代码的位置。C 中 `asm_add(12,5)` 的函数名被链接器解析为地址 0x80000174（见 8.4 节） |
| `add a0, a0, a1` | 目标寄存器在前：a0 = a0 + a1 |
| `ret` | 伪指令。真身为 `jalr x0, 0(ra)`：跳转到 ra 保存的地址。目标写 x0 表示丢弃新返回地址 |

寄存器级执行追踪（`asm_add(12, 5)`）：

| 时刻 | a0 | a1 | 说明 |
| --- | --- | --- | --- |
| 调用前（C 已按约定放置参数） | 12 | 5 | 参数就位 |
| `add` 执行后 | **17** | 5 | 结果写回 a0 |
| `ret` 执行后 | 17 | 5 | a0 保持不变。该值即返回值 |

该函数不需要栈。原因：

1. 函数不调用其他函数（叶子函数），ra 天然安全；
2. 函数不使用 s 系寄存器；
3. 函数没有需要存储的局部变量。

asm_sub 与 asm_add 结构相同。运算指令为 `sub a0, a0, a1`。结果：a0 = 12 − 5 = 7。

### 10.4 函数 asm_mul 与 M 扩展

源码：

```asm
	.globl asm_mul
	.type  asm_mul, @function
asm_mul:
	mul     a0, a0, a1      /* a0 = a0 * a1（低 32 位）*/
	ret
	.size  asm_mul, .-asm_mul
```

指令用法与 add 相同。重点为 M 扩展的存在原因：

- RISC-V 基础指令集 RV32I 只包含 40 余条指令：加减、逻辑、访存、分支。**不包含乘除法**。早期硬件为节省面积省略乘除器。多数乘法可用移位与加法替代；
- `mul/div/rem` 指令需要硬件实现 **M 扩展**；
- 本程序 `-march=rv32imac_zicsr_zifencei` 中的 `m` 即 M 扩展（来源见 5.4 节）。QEMU 按 ISA 字符串模拟，支持该扩展；
- 验证实验：将 `-march` 改为 `rv32i` 重新构建。`mul` 指令汇编报错。指令是否存在由扩展集决定；
- "低 32 位"说明：12 × 5 = 60 未超出 32 位。100000 × 100000 = 10^10 超出 2^32。RV32 的 `mul` 只保留低 32 位。需要高位时使用 `mulh` 指令（M 扩展进阶指令）。

### 10.5 函数 asm_sum_to：循环与分支

功能：计算 1+2+…+n。该函数首次引入循环结构。

```asm
	.globl asm_sum_to
	.type  asm_sum_to, @function
asm_sum_to:
	li      t0, 0           /* acc = 0（累加和）*/
	li      t1, 1           /* i   = 1（计数器）*/
.Lsum_loop:
	bgt     t1, a0, .Lsum_done   /* if (i > n) 跳出循环 */
	add     t0, t0, t1      /* acc += i */
	addi    t1, t1, 1       /* i++      */
	j       .Lsum_loop      /* 无条件跳回循环开头 */
.Lsum_done:
	mv      a0, t0          /* 返回值 = acc */
	ret
	.size  asm_sum_to, .-asm_sum_to
```

指令对照表。左列为编写的伪指令，右列为汇编器替换后 CPU 实际执行的真指令：

| 编写形式 | 真身（CPU 执行） | 说明 |
| --- | --- | --- |
| `li t0, 0` | `addi t0, zero, 0` | load immediate：装入常数。小常数用一条 `addi`（借 zero 寄存器加出）；大常数（超出 12 位范围）用 `lui`+`addi` 两条。开启 C 扩展时常为 2 字节的 `c.li` |
| `bgt t1, a0, 标签` | `blt a0, t1, 标签` | branch if greater than：i > n 时跳转。CPU 硬件比较分支只有 `beq/bne/blt/bge/bltu/bgeu` 六种。`bgt` 由汇编器对调两个操作数实现 |
| `add t0, t0, t1` | 真指令（原样） | acc += i，寄存器加法 |
| `addi t1, t1, 1` | 真指令 | add immediate：i++。常数直接编入指令，无需先装入寄存器。立即数为 12 位有符号数（−2048 至 2047） |
| `j .Lsum_loop` | `jal x0, 偏移` | 无条件跳转。目标写标签，汇编器计算距离。目标寄存器写 x0 表示不留返回地址 |
| `mv a0, t0` | `addi a0, t0, 0` | move：a0 = t0。本质为加 0 运算 |

逐轮执行追踪（`asm_sum_to(10)`，进入时 a0=10）：

| 轮次 | 进入循环时 t1 (i) | 本轮 bgt 判断 (i>10?) | 本轮 t0 变化 | 本轮结束时 t0 |
| --- | --- | --- | --- | --- |
| 1 | 1 | 否，继续 | 0 + 1 | 1 |
| 2 | 2 | 否 | 1 + 2 | 3 |
| 3 | 3 | 否 | 3 + 3 | 6 |
| … | … | … | … | … |
| 10 | 10 | 否 | 45 + 10 | 55 |
| 11 | 11 | **是 → 跳转到 .Lsum_done** | 不执行 | 55 |

随后 `mv a0, t0`（a0=55），执行 `ret`。C 侧取得返回值 55。

结论：C 语言的 `for` 循环在 CPU 层面由"比较、跳转"两类指令反复执行实现。

两项细节说明：

- `.L` 开头的标签为局部标签。GNU as 约定：局部标签不进入符号表、不占用全局命名空间、允许重名。循环与分支的目标标签均使用此约定；
- t0/t1 用完不保存的原因：调用约定规定 t 系寄存器为调用者保存。所有函数默认 t 系寄存器的值随时可能失效。本函数未调用其他函数，t0/t1 实际安全。但需要跨 call 保留的值不应存入 t 系寄存器。

### 10.6 函数 asm_factorial：递归与栈帧

本函数用汇编实现递归调用。先说明两项背景知识。

**背景一：栈的定义。**

- 栈是 RAM 中划出的暂存区。特性为后进先出；
- `sp` 永远指向栈顶（当前空位）；
- RISC-V 约定栈向低地址生长。存入数据前先减小 sp（分配空间），取回数据后增大 sp（归还空间）；
- 每个线程拥有独立的栈。Zephyr 切换线程时切换 sp。本程序中 main 的栈由内核启动时分配。

**背景二：保存 ra 的必要性。**

- `call` 每次执行都将返回地址写入 ra。ra 只有一个；
- `factorial(3)` 递归调用 `factorial(2)` 时，第二次 call 覆盖第一次的 ra；
- 不保存 ra 的后果：最深一层返回后，外层全部跳转到错误地址。症状为程序跑飞或无声挂死，排查困难；
- 结论：函数若调用其他函数（非叶子函数），进入函数后第一件事是将 ra 存入栈。

栈帧布局（一次分配 8 字节，存储两项数据）：

```
        地址增大 ↑
        ┌──────────────────────┐
        │      更早的栈内容       │
        ├──────────────────────┤ ← 开帧前的 sp（= 回收后的 sp）
        │ ra（4 字节）返回地址    │ ← sw ra, 4(sp)
        │ n （4 字节）当前参数    │ ← sw a0, 0(sp)
        └──────────────────────┘ ← 开帧后的 sp（addi sp, sp, -8 之后）
        地址减小 ↓（栈向下生长）
```

完整源码与逐行注释：

```asm
	.globl asm_factorial
	.type  asm_factorial, @function
asm_factorial:
	li      t0, 1
	ble     a0, t0, .Lfac_base   /* if (n <= 1) 直接返回 1 —— 递归出口 */

	addi    sp, sp, -8      /* 开 8 字节栈帧：ra 和 n 各占 4 字节 */
	sw      ra, 4(sp)       /* store word：把 ra 存到内存地址 sp+4 */
	sw      a0, 0(sp)       /* 把当前 n 存到内存地址 sp+0 */

	addi    a0, a0, -1      /* a0 = n-1：为下一层准备参数 */
	call    asm_factorial   /* 递归！返回时 a0 = (n-1)!（同时 ra 被覆盖）*/

	lw      t1, 0(sp)       /* load word：从 sp+0 取回本层的 n */
	lw      ra, 4(sp)       /* 从 sp+4 取回本层的返回地址 */
	addi    sp, sp, 8       /* 回收栈帧：sp 必须还回原位（栈平衡） */

	mul     a0, a0, t1      /* a0 = (n-1)! × n */
	ret                     /* 用恢复过的 ra 返回 */
.Lfac_base:
	li      a0, 1           /* 0! = 1! = 1（出口分支：不开帧）*/
	ret
	.size  asm_factorial, .-asm_factorial
```

新指令为两条访存指令。语法为 `偏移(基址寄存器)`：

| 指令 | 方向 | 作用 |
| --- | --- | --- |
| `sw rs, 偏移(sp)` | 寄存器 → 内存 | store word：把 rs 的 32 位存入地址 `sp+偏移`。偏移为 12 位有符号数（±2048）。栈帧较大时必须先调整 sp 本身 |
| `lw rd, 偏移(sp)` | 内存 → 寄存器 | load word：从 `sp+偏移` 读取 32 位装入 rd。`sw` 与 `lw` 为 RISC-V 读写内存的基本指令 |

`factorial(3)` 全过程推演。建议用第 9 章 GDB 单步执行验证。

**下潜阶段**（每层开辟栈帧，ra 逐层存入栈）：

| 时刻 | sp 相对位置 | 本帧栈上存储的内容 | a0 | ra 当前指向 |
| --- | --- | --- | --- | --- |
| main 执行 `call fac(3)` | — | — | 3 | → main 中 call 的下一行 |
| fac(3) 进入，开辟栈帧 | sp₁ | `[n=3][ra→main]` | 3 | 仍指向 main（正被 sw 存入栈） |
| fac(3) 执行 `call fac(2)` | sp₁ | 同上 | 2 | → fac(3) 内 call 的下一行（ra 被覆盖） |
| fac(2) 进入，开辟栈帧 | sp₂（=sp₁−8） | `[n=2][ra→fac(3)内]` | 2 | 仍指向 fac(3) 内（正被存入栈） |
| fac(2) 执行 `call fac(1)` | sp₂ | 同上 | 1 | → fac(2) 内 call 的下一行（ra 再次被覆盖） |
| fac(1) 进入，命中出口 | **不开帧** | 栈保持 sp₂ | 1 | fac(2) 内 call 的下一行（未被覆盖，直接可用） |

最深时刻的栈全景：

```
        ┌───────────────────────────┐
 sp₁ →  │ fac(3) 的帧： n=3 │ ra→main │   ← 仍在栈上
        ├───────────────────────────┤
 sp₂ →  │ fac(2) 的帧： n=2 │ ra→fac(3)│  ← 仍在栈上
        ├───────────────────────────┤
        │   fac(1) 是出口，没开帧      │   ← 正在执行 li a0,1; ret
        └───────────────────────────┘
```

**回升阶段**（逐层取回、相乘、归还栈帧）：

1. `fac(1)` 执行 ret。返回到 `fac(2)` 中 call 的下一行。a0=1（即 1!）；
2. `fac(2)`：`lw t1` 取回 n=2，`lw ra` 取回自己存入的返回地址，`mul` 计算 a0 = 1×2 = 2，`sp += 8` 归还栈帧，ret 返回 `fac(3)`；
3. `fac(3)` 同法：a0 = 2×3 = 6，ret 返回 main。

计算结果：3! = 6。main.c 实际调用 `asm_factorial(6)`，结果 720。执行流程相同，递归深度为 6 层。

**常见错误自查表**（修改本函数练习时对照）：

| 错误 | 后果 |
| --- | --- |
| 缺少 `sw ra` | 递归加深后 ra 被覆盖。返回时跳转到错误地址。程序跑飞（新手最常见错误） |
| `sw ra` 与 `lw ra` 偏移不一致 | 取回的数据错误 |
| `-8` 与 `+8` 不配对 | 栈不平衡。上层栈帧全部错位 |
| `mul` 先于 `lw` 执行（顺序颠倒） | 使用了错误的数据参与运算 |

### 10.7 练习任务

每项练习完成后，执行 `./make.sh` 查看结果。

| 编号 | 任务 | 提示 |
| --- | --- | --- |
| 1 | 编写 `asm_max(int a, int b)`，返回较大值 | 使用 `blt a0, a1, 标签` 加 `mv`。在 main.c 添加 `extern int asm_max(int, int);` 与一行 printk 验证 |
| 2 | 修改 `asm_sum_to`，只累加偶数 | 修改 `addi t1, t1, 2`。需同步调整初始值 |
| 3 | 用 GDB 在 `asm_factorial` 设断点，单步执行 10.6 节的推演 | 见第 9 章。用 `info registers sp ra a0` 观察栈的下潜与回升 |

### 10.8 C 与汇编对照表

编写新函数时按本表查询。

| C 语言写法 | 汇编写法 | 本程序示例 |
| --- | --- | --- |
| `return a + b;` | `add` | asm_add |
| `return a - b;` | `sub` | asm_sub |
| `return a * b;` | `mul`（需 M 扩展） | asm_mul |
| `int x = 常数;` | `li x, 常数`（伪指令） | `li t0, 0` |
| `x++` | `addi x, x, 1` | `addi t1, t1, 1` |
| `if (a > b) …` | `bgt`/`bge`/`beq`/`bne`… | `bgt t1, a0, .Lsum_done` |
| `if (n <= 1) …` | `ble`（伪指令，即 bge 反向） | `ble a0, t0, .Lfac_base` |
| `goto 标签` / 循环回边 | `j 标签`（伪指令） | `j .Lsum_loop` |
| 调用函数 | `call 名称`（即 `jal ra, …`） | `call asm_factorial` |
| 函数返回 | `ret`（即 `jalr x0, 0(ra)`） | 各函数末尾 |
| `return x;` | `mv a0, x` 加 `ret` | `mv a0, t0` |
| 局部变量 | 优先存入寄存器（t 系或 s 系）。需跨 call 或寄存器不足时存入栈 | t0/t1 与栈上的 n |
| 保存/恢复现场 | `sw`/`lw` 配合 `sp` 开辟与回收栈帧 | asm_factorial 全套 |

---

## 11 故障排除

| 现象 | 原因 | 纠正措施 |
| --- | --- | --- |
| `qemu-system-riscv32: command not found` | 宿主机缺少 QEMU | 执行 `sudo apt-get install -y qemu-system-misc` |
| `cannot create PID file: Cannot lock pid file` | 上次 QEMU 未正常退出 | 执行 `pkill -f qemu-system-riscv32`，然后删除输出目录中的 `qemu.pid` |
| `west build -t run` 一直不返回 | 程序进入 Zephyr 空闲线程，属正常现象 | 按 `Ctrl-A` 再按 `X` 退出 |
| `west build -t debug` 报 `FATAL ERROR: no runners.yaml found` | QEMU 目标板不支持 west debug/flash（见 9.1 节） | 使用第 9 章双终端手动调试方法 |
| 提示找不到 `ZEPHYR_BASE` 或目标板 | 虚拟环境未激活，或不在 `zephyrproject` 目录下执行 | 激活虚拟环境，进入工作区根目录 |
| 修改 `.S` 文件后不重新编译 | 构建系统未检测到变更 | 构建命令加 `-p always` 强制重建 |
| 需要改用 64 位 RISC-V | — | 将 `-b qemu_riscv32` 换成 `-b qemu_riscv64`。寄存器宽度变为 64 位（`a0` 等别名不变） |

---

## 12 文件结构

### 12.1 目录结构

```
riscv_asm_learn/
├── CMakeLists.txt      # 声明把 main.c 与 asm_funcs.S 一起编译
├── prj.conf            # Kconfig 配置（串口控制台，目标板默认已开启）
├── make.sh             # 一键构建与运行脚本（见第 6 章）
├── .clangd             # 编辑器跳转配置，指向本项目的 compile_commands.json
├── README.md           # 本文档
└── src/
    ├── main.c          # C 调用汇编并打印
    └── asm_funcs.S     # 5 个汇编函数
```

### 12.2 建议学习顺序

| 顺序 | 主题 | 章节 |
| --- | --- | --- |
| 1 | 寄存器与调用约定 | [10.2 调用约定](#102-调用约定-ilp32)、[10.3 asm_add 与 asm_sub](#103-函数-asm_add-与-asm_sub)。理解 `a0/a1` 与返回值 |
| 2 | 算术指令 | [10.3 asm_sub](#103-函数-asm_add-与-asm_sub)（`sub`）、[10.4 asm_mul 与 M 扩展](#104-函数-asm_mul-与-m-扩展)（`mul` 加 M 扩展） |
| 3 | 控制流 | [10.5 asm_sum_to](#105-函数-asm_sum_to循环与分支)（`li/add/bgt/j` 循环） |
| 4 | 函数调用与栈 | [10.6 asm_factorial](#106-函数-asm_factorial递归与栈帧)（`call/ret`、`sw/lw`、栈帧、保存 `ra`） |
| 5 | 验证理解 | 自行添加 `asm_max(int a, int b)` 并构建运行（练习见 [10.7](#107-练习任务)） |
