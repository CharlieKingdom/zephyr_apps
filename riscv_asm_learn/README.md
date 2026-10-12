# RISC-V 32 汇编学习 Demo

一个跑在 **QEMU 模拟的 RISC-V 32 位机器**上的 Zephyr 程序。
用 C 调用 5 个**纯汇编**函数，通过串口打印结果，带你从 0 认识 RISC-V 架构。

- 目标板：`qemu_riscv32`（不需要真实硬件）
- 指令集：`rv32imac_zicsr_zifencei`（32 位，含 M/A/C 扩展）
- 调用约定：`ilp32`（软浮点）

---

## 目录

1. [前置环境](#1-前置环境)
2. [从 0 开始编译](#2-从-0-开始编译)
3. [Zephyr 如何选中 RISC-V 与 QEMU](#3-zephyr-如何选中-risc-v-与-qemu)
4. [运行仿真](#4-运行仿真)
5. [逐行读懂汇编代码](#5-逐行读懂汇编代码)
6. [进阶反汇编与 GDB 调试](#6-进阶反汇编与-gdb-调试)
7. [常见问题](#7-常见问题)

---

## 1. 前置环境

假设 Zephyr 工作区位于 `/home/ubuntu/workspace/zephyrproject`。

```bash
# 激活 Zephyr 的 Python 虚拟环境（~/.bashrc 已配置，新终端自动生效）
source /home/ubuntu/workspace/zephyrproject/.venv/bin/activate
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

# 确认版本
west --version          # 期望 1.5.0
cd /home/ubuntu/workspace/zephyrproject
west list zephyr         # 确认工作区正常
```

另外 QEMU 需要**宿主机**提供 `qemu-system-riscv32`（它不属于 Zephyr SDK）：

```bash
qemu-system-riscv32 --version     # 若报找不到，执行：
sudo apt-get install -y qemu-system-misc
```

> 注意区分两个 "riscv"：
> - **交叉编译器**：`riscv64-zephyr-elf-*`，来自 Zephyr SDK，负责把源码编译成 RISC-V 机器码。
> - **QEMU**：`qemu-system-riscv32`，来自 Ubuntu apt，负责在 x86 主机上模拟一颗 RISC-V CPU。

---

## 2. 从 0 开始编译

### 2.1 一键编译 + 运行：`./make.sh`

本工程自带脚本 `make.sh`，把「切换目录 → 激活环境 → 编译 → 运行」打包成一条命令：

```bash
cd /home/ubuntu/workspace/zephyrproject/apps/riscv_asm_learn
./make.sh
```

脚本全文如下：

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

逐段说明：

| 片段 | 作用 |
| --- | --- |
| `set -euo pipefail` | "出错即停"：任一命令失败、引用未定义变量、管道中任一段失败，脚本都立刻退出，不会带病继续 |
| `cd "$(dirname "$(readlink -f "$0")")/../.."` | 先定位脚本自身、再切到工作区根 `zephyrproject/`，因此**无论从哪个目录调用**，后面的相对路径都成立 |
| `source .venv/bin/activate` | 激活 Zephyr 的 Python 虚拟环境，非交互执行（如 IDE 任务）也能找到 `west` 与 SDK |
| `export ZEPHYR_TOOLCHAIN_VARIANT=zephyr` | 指定使用 Zephyr SDK 工具链 |
| `west build ...` | 真正的编译命令，逐参数见 [2.2](#22-编译命令逐参数拆解) |

### 2.2 编译命令逐参数拆解

脚本的核心是这一行：

```bash
west build -p always -b qemu_riscv32 apps/riscv_asm_learn \
           -d build/build_qemu_riscv32_asm -t run
```

| 参数 | 含义 |
| --- | --- |
| `west build` | Zephyr 的构建命令（底层是 CMake + Ninja） |
| `-p always` | 构建前**清空 build 目录**再全量重编，无视文件是否改动；保证结果干净、可复现 |
| `-b qemu_riscv32` | **目标板 = qemu_riscv32**，决定用哪套 arch/SoC/DTS/工具链 |
| `apps/riscv_asm_learn` | 应用源码目录（含 `CMakeLists.txt`、`prj.conf`、`src/`） |
| `-d build/build_qemu_riscv32_asm` | 输出目录（本项目约定的命名：`build_<板子>_<用途>`） |
| `-t run` | 构建完成后立即在 QEMU 中运行（详见[第 4 节](#4-运行仿真)） |

> `-p` 的三种取值：`always`（每次都全量重编）、`auto`（仅在换板/换 app 时清空）、`never`（纯增量，默认）。
> 想加快日常迭代速度，可把脚本里的 `-p always` 改为 `-p auto`。

### 2.3 手工编译（不用脚本时）

在 `zephyrproject` 目录下手工执行：

```bash
cd /home/ubuntu/workspace/zephyrproject

west build -b qemu_riscv32 apps/riscv_asm_learn \
           -d build/build_qemu_riscv32_asm
```

编译成功后会看到：

```
Memory region         Used Size  Region Size  %age Used
             RAM:       24688 B       256 MB      0.01%
Generating files from .../build_qemu_riscv32_asm/zephyr/zephyr.elf for board: qemu_riscv32
```

产物：

```
build/build_qemu_riscv32_asm/zephyr/zephyr.elf   ← 可执行的 RISC-V ELF
build/build_qemu_riscv32_asm/zephyr/zephyr.map   ← 内存布局
build/build_qemu_riscv32_asm/zephyr/.config      ← 最终 Kconfig 配置
```

确认它确实是 RISC-V 32 位程序：

```bash
file build/build_qemu_riscv32_asm/zephyr/zephyr.elf
# ELF 32-bit LSB executable, UCB RISC-V, RVC, soft-float ABI, ... not stripped
```

---

## 3. Zephyr 如何选中 RISC-V 与 QEMU

这部分讲原理，理解了以后你能自己移植到别的板子。

### 3.1 板子（board）是一切的入口

`-b qemu_riscv32` 会让 Zephyr 去 `zephyr/boards/qemu/riscv32/` 读 4 个关键文件：

| 文件 | 作用 |
| --- | --- |
| `qemu_riscv32.yaml` | 板子元信息：`arch: riscv`、`ram/flash` 大小、`type: qemu` |
| `Kconfig.defconfig` | 板级默认配置（如启用 TLS） |
| `qemu_riscv32.dts` | 设备树：SoC 选谁、串口地址、内存映射 |
| `board.cmake` | **QEMU 启动参数**（灵魂所在，见下） |

### 3.2 arch 怎么定为 riscv

`arch: riscv` → Zephyr 载入
[arch/riscv](https://github.com/zephyrproject-rtos/zephyr/tree/v4.1.0/arch/riscv)（上游 v4.1.0，本地工作区同路径），
用 RISC-V 的启动代码（`reset.S`、trap 处理、`switch.S` 上下文切换等）。

### 3.3 SoC 与 DTS CPU 和内存长什么样

`qemu_riscv32.dts` 引入 `virt-riscv32.dtsi`，里面有：

```dts
ram0: memory@80000000 { reg = <0x80000000 0x10000000>; };  /* 256MB RAM */
uart0: uart@10000000  { compatible = "ns16550"; ... };       /* 串口 */
```

即 QEMU `virt` 机器的经典布局：**RAM 从 `0x80000000` 开始，串口在 `0x10000000`**。
CPU 的 `riscv,isa` 描述支持哪些扩展。

### 3.4 工具链怎么选成 RISC-V 交叉编译器

配置里选中了 M/A/C 扩展，最终编译参数为：

```
-march=rv32imac_zicsr_zifencei     ← 生成 RV32 指令
-mabi=ilp32                        ← 32 位软浮点调用约定
```

编译器是 SDK 里的 `riscv64-zephyr-elf-gcc`（用 `riscv64-` 前缀的编译器可以同时产出 rv32 / rv64 目标）。
可自行验证：

```bash
grep -o 'march=[^ ]*' build/build_qemu_riscv32_asm/compile_commands.json | head -1
grep -o 'mabi=[^ ]*'  build/build_qemu_riscv32_asm/compile_commands.json | head -1
```

### 3.5 QEMU 参数从哪来

看
[board.cmake](https://github.com/zephyrproject-rtos/zephyr/blob/v4.1.0/boards/qemu/riscv32/board.cmake)（上游 v4.1.0，本地工作区同路径）：

```cmake
set(QEMU_binary_suffix riscv32)          # → 用 qemu-system-riscv32
set(QEMU_CPU_TYPE_${ARCH} riscv32)
set(QEMU_FLAGS_${ARCH}
  -nographic      # 不用图形界面，串口直连当前终端
  -machine virt   # QEMU 的 "virt" 通用虚拟平台
  -bios none      # 不要 BIOS，直接跑我们编译的 ELF
  -m 256          # 256MB 内存
)
```

执行 `west build -t run` 时，Zephyr 会拼出完整命令（可在构建日志里看到）：

```bash
qemu-system-riscv32 -nographic -machine virt -bios none -m 256 \
  -kernel build/build_qemu_riscv32_asm/zephyr/zephyr.elf
```

**这就是"QEMU 怎么选择 riscv"的答案**：`QEMU_binary_suffix=riscv32` 选程序，
`_machine virt` 选虚拟平台，`-kernel` 告诉它跑哪个 ELF。

---

## 4. 运行仿真

> `./make.sh` 已经在编译后自动执行运行（内置 `-t run`）。本节用于**只想重新运行、不再编译**，或想了解 QEMU 底层调用的场景。

### 方式一：用 west（推荐）

```bash
cd /home/ubuntu/workspace/zephyrproject
west build -d build/build_qemu_riscv32_asm -t run
```

输出：

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

**退出 QEMU**：按 `Ctrl-A` 松开，再按 `X`。
（若程序结束但没退出，QEMU 会停在空转——Zephyr 程序通常执行完 `main` 后进入空闲线程。）

### 方式二：手动敲 qemu（理解原理）

```bash
cd /home/ubuntu/workspace/zephyrproject

QEMU=$HOME/zephyr-sdk-0.17.0/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-riscv32
ELF=build/build_qemu_riscv32_asm/zephyr/zephyr.elf

$QEMU -nographic -machine virt -bios none -m 256 -kernel $ELF
```

两者完全等价，方式二能让你看清 QEMU 到底接收了哪些参数，
参数的来源逐条见 [3.5 QEMU 参数从哪来](#35-qemu-参数从哪来)。

---

## 5. 逐行读懂汇编代码

代码在 [src/asm_funcs.S](src/asm_funcs.S)（5 个汇编函数），C 侧调用在 [src/main.c](src/main.c)。

本章假设你对 RISC-V 和汇编**一窍不通**：先在 5.1/5.2 建立两块基石（一行汇编怎么读、寄存器怎么分工），
然后 5 个函数按"由易到难"逐个精讲——每条指令都回答两个问题：**它做了什么？为什么需要它？**
最后 5.7 给出练习和"C ↔ 汇编"翻译词典。所有结论都能用
[6.1 反汇编](#61-查看反汇编) 和 [6.2 GDB 单步](#62-用-gdb-单步调试) 亲手验证。

本章小节：

- [5.1 开始之前 三个最基本的概念](#51-开始之前-三个最基本的概念)
- [5.2 RISC-V 的调用约定 ilp32](#52-risc-v-的调用约定-ilp32)
- [5.3 asm_add 与 asm_sub 第一段汇编](#53-asm_add-与-asm_sub-第一段汇编)
- [5.4 asm_mul 与 M 扩展](#54-asm_mul-与-m-扩展)
- [5.5 asm_sum_to 循环与分支](#55-asm_sum_to-循环与分支)
- [5.6 asm_factorial 递归与栈帧](#56-asm_factorial-递归与栈帧)
- [5.7 动手练习与翻译词典](#57-动手练习与翻译词典)

### 5.1 开始之前 三个最基本的概念

**概念一：汇编 = 机器码的"人类可读写法"。**

CPU 是纯二进制动物，它唯一看得懂的是 0/1。把每条机器码换成一个好记的名字（助记符），
就是汇编。对应关系（以加法为例）：

| 层次 | 谁写 | 长什么样 |
| --- | --- | --- |
| C 语言 | 人 | `return a + b;` |
| 汇编 | 人写、汇编器翻译 | `add a0, a0, a1` |
| 机器码 | CPU 执行 | 一串 0/1（见下） |

一行汇编 ≈ 一条 CPU 指令 ≈ CPU 的一个基本动作；C 的一行通常被编译器翻成一行或多行汇编。

`add a0, a0, a1` 的 32 位"真身"，按 RISC-V 的 R 型格式切开（看懂这一格，指令编码的骨架就有了）：

```
 funct7     rs2      rs1    funct3    rd      opcode
┌────────┬────────┬────────┬───────┬────────┬─────────┐
│0000000 │  01011 │  01010 │  000  │  01010 │ 0110011 │ → add a0, a0, a1
└────────┴────────┴────────┴───────┴────────┴─────────┘
  "是加法"   x11      x10      加     x10     "寄存器-
             =a1      =a0             =a0     寄存器运算"
```

- 每段 5 位正好能编码 32 个寄存器之一（2^5 = 32）；
- 汇编器干的活，本质就是"助记符 → 这串 0/1"的查表翻译；
- 小知识：本工程 ELF 里实际存的是它的 **16 位压缩版 `c.add`**（因为 `-march` 带 `c` 扩展，
  见 [3.4](#34-工具链怎么选成-risc-v-交叉编译器)），两种编码 CPU 都认识，效果相同——
  亲眼所见请看 [6.1 反汇编](#61-查看反汇编)。

**概念二：寄存器 = CPU 自带的高速"小抽屉"。**

内存（RAM）容量大但离运算器远；CPU 做任何加减比较，都要先把数据搬进自己内部的
超高速存储格——寄存器。RV32 有 **32 个**寄存器，每个恰好放一个 32 位整数。
本 demo 的所有运算都发生在寄存器之间（只有 factorial 用到了内存——栈）。

寄存器有两套名字：编号名 `x0`–`x31`，和别名 `zero/ra/sp/a0/t0/s0...`。
日常写代码用**别名**，因为别名自带"用途说明书"（见 [5.2](#52-risc-v-的调用约定-ilp32)）。

**概念三：一行汇编怎么读——目标寄存器写在最前面。**

```
add     a0, a0, a1
┬┘     ┬┘   ┬┘  ┬┘
操作码  目标  源1  源2
```

读作：`a0 = a0 + a1`。**结果永远写进第一个操作数**，后面的是输入。
这条规则覆盖 RISC-V 几乎所有运算指令，记住它你就能读懂 80% 的汇编。
（查资料时注意：x86 的 AT&T 语法恰好相反，目标在最后，别串了。）

另外有两种"行"**不是指令**，要区分开：

| 长相 | 例子 | 是什么 |
| --- | --- | --- |
| `名字:` | `asm_add:` | 标签：给"当前位置"起名，本身**不产生任何代码**，跳转/调用靠它定位 |
| `.` 开头 | `.globl asm_add` | 汇编指示符：写给汇编器看的排版命令，**不会变成机器码** |

本工程用到的全部指示符一览：

| 指示符 | 作用 | 通俗解释 |
| --- | --- | --- |
| `.section .text` | 之后的代码归入 .text 段 | "把我放进代码区"，链接时会被排进 RAM 的代码区 |
| `.globl asm_add` | 导出符号 | "我的名字要公开"，C 那边才能链接到我；缺了它链接报 undefined reference |
| `.type asm_add, @function` | 标记符号类型 | 给调试器/反汇编器看的元信息 |
| `.size asm_add, .-asm_add` | 记录函数长度 | `.` = 当前位置，减去起点即大小 |

最后一个术语：**伪指令**。有些写法看起来是指令，但 CPU 里**根本不存在**，
是汇编器送你的便利写法，汇编时自动替换成真指令——`ret`、`li`、`mv`、`j`、`bgt`、`call` 全是。
它们的"真身"对照表见 [5.5](#55-asm_sum_to-循环与分支)，别急。

### 5.2 RISC-V 的调用约定 ilp32

**调用约定（Calling Convention，本工程用 ilp32 ABI）= C 编译器与汇编函数之间的"合同"**，
写明：参数放哪、返回值放哪、寄存器坏了算谁的。两边都按合同办事，C 和汇编才能互调。

RV32 全部 32 个寄存器的分工（**本节是查表区，后面忘了随时翻回来**）：

| 寄存器 | 别名 | 名字含义 | 保存责任 | 本 demo 中的角色 |
| --- | --- | --- | --- | --- |
| x0 | zero | 恒零 | 读永远得 0，写等于丢弃 | `li` 借它"加出"常数 |
| x1 | ra | return address | 要 call 别人的函数自己保（call 会覆盖它） | [5.6](#56-asm_factorial-递归与栈帧) 的主角 |
| x2 | sp | stack pointer | 函数进出必须"栈平衡" | 开帧/回收栈 |
| x3 / x4 | gp / tp | 全局/线程指针 | 内核与链接器管理 | demo 不碰 |
| x5–x7, x28–x31 | t0–t6 | temporary 临时 | **调用者**：call 后视为已作废 | acc、i、临时值 |
| x8–x9, x18–x27 | s0–s11 | saved 长期 | **被调用者**：用了必须原样奉还 | demo 未用 |
| x10–x17 | a0–a7 | argument 参数 | 调用者 | a0=第1参/返回值，a1=第2参 |

- **调用者保存**（t 系、a 系）：你可以随便用，但别指望 call 完它还在；
- **被调用者保存**（s 系）：函数可以先存后用，但退出前必须恢复原值；
- 所以"跨函数调用还想要的值"放 s 系或栈上，"用完就丢的中间值"放 t 系。

`asm_add(12, 5)` 的完整"合同履行"过程：

```
 C 编译器（甲方）                          汇编函数（乙方）
──────────────────────────              ──────────────────────────
1. 把 12 放进 a0，把 5 放进 a1
2. 执行 call asm_add
   （call 顺手把"返回地址"写进 ra）  ──→   3. 按合同：第1个参数在 a0，
                                              第2个参数在 a1
                                         4. 干活：a0 = a0 + a1
                                         5. 按合同：结果留在 a0
                                         6. ret：跳回 ra 记录的地址
←──────────────────────────
7. C 从 a0 取回返回值，继续生活
```

口诀：**参数进 a，返回出 a，临时用 t，长期用 s，返回地址存 ra，栈的进出靠 sp。**

这些规定从哪来？`-mabi=ilp32` 一个字母组正好对应（i=32 位 int，l=32 位 long，p=32 位指针），
即 32 位软浮点 ABI，见 [3.4](#34-工具链怎么选成-risc-v-交叉编译器)。

### 5.3 asm_add 与 asm_sub 第一段汇编

两个函数的完整源码（含所有"排版"行），先看 asm_add：

```asm
	.globl asm_add          /* 指示符：导出函数名，C 才能链接到 */
	.type  asm_add, @function
asm_add:                    /* 标签：函数入口地址的名字 */
	add     a0, a0, a1      /* 真指令：a0 = a0 + a1 */
	ret                     /* 伪指令：返回调用者 */
	.size  asm_add, .-asm_add
```

逐行拆解：

| 行 | 读法 / 作用 |
| --- | --- |
| `.globl` `.type` | 见 [5.1 指示符表](#51-开始之前-三个最基本的概念)，纯元信息 |
| `asm_add:` | 标签 = 函数第一行代码的位置。C 里写 `asm_add(12,5)`，链接器把这个名字解析成地址（0x80000174，见 [6.1](#61-查看反汇编)） |
| `add a0, a0, a1` | 目标在前：a0 = a0 + a1 |
| `ret` | 伪指令，真身是 `jalr x0, 0(ra)`：**跳到 ra 保存的地址**。目标写 x0 = 把返回地址丢弃（普通返回不嵌套） |

寄存器级追踪（`asm_add(12, 5)`）：

| 时刻 | a0 | a1 | 说明 |
| --- | --- | --- | --- |
| 调用前（C 已按合同放好） | 12 | 5 | 参数就位 |
| `add` 执行后 | **17** | 5 | 结果写回 a0 |
| `ret` 执行后 | 17 | 5 | a0 原样带回，这就是返回值 |

为什么这个函数**不需要栈**？它没调用任何别的函数（行话叫"叶子函数"，leaf function），
ra 天然安全；没用 s 系寄存器；也没有放不下的局部变量。一条运算 + 一个返回，干净利落。

`asm_sub` 完全同理，把加换成减：`sub a0, a0, a1` → a0 = 12 − 5 = 7。

### 5.4 asm_mul 与 M 扩展

```asm
	.globl asm_mul
	.type  asm_mul, @function
asm_mul:
	mul     a0, a0, a1      /* a0 = a0 * a1（低 32 位）*/
	ret
	.size  asm_mul, .-asm_mul
```

重点不在指令（用法与 add 一模一样），而在它背后的**指令集经济学**：

- RISC-V 的基础指令集 **RV32I 只有 40 多条**：加减、逻辑、访存、分支……**没有乘除法**
  （早期硬件省面积；许多乘法可用移位+加法凑）。想要 `mul/div/rem`，硬件必须实现 **M 扩展**；
- 本工程 `-march=rv32imac_zicsr_zifencei` 里那个 `m` 就是它（这个字符串怎么来的见
  [3.4](#34-工具链怎么选成-risc-v-交叉编译器)）。QEMU 按 ISA 字符串模拟，自然支持；
- 动手试：把 march 换成 `rv32i` 重新编译，这条 `mul` 会直接汇编报错——指令"有没有"由扩展集决定；
- "低 32 位"：12 × 5 = 60 没超；但 100000 × 100000 = 10^10 超过 2^32，RV32 的 `mul` 只保留低 32 位
  （要高位得用 `mulh`，M 扩展的进阶货）。

### 5.5 asm_sum_to 循环与分支

目标：计算 1+2+…+n。这是第一次让 CPU"转圈"——循环。

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

新面孔逐条精讲。左列是"你写的"，右列是**汇编器替换后 CPU 实际执行的真身**——
这张表同时是 [5.1](#51-开始之前-三个最基本的概念) 里说的伪指令对照表：

| 你写的 | 真身（CPU 执行的） | 一句话解释 |
| --- | --- | --- |
| `li t0, 0` | `addi t0, zero, 0` | load immediate：装常数。小常数一条 `addi`（借 zero"加"出来）；大常数（装不进 12 位）则 `lui`+`addi` 两条。开了 C 扩展时常是 2 字节的 `c.li` |
| `bgt t1, a0, 标签` | `blt a0, t1, 标签` | branch if greater than：i > n 就跳。CPU 硬件只有 `beq/bne/blt/bge/bltu/bgeu` 六种比较方向，`bgt` 是汇编器把两个操作数**对调**实现的 |
| `add t0, t0, t1` | 真指令（就是它自己） | acc += i，寄存器加法 |
| `addi t1, t1, 1` | 真指令 | add immediate：i++。常数直接编进指令里，不用先装寄存器。立即数是 **12 位有符号**（−2048~2047），够 i++ 用 |
| `j .Lsum_loop` | `jal x0, 偏移` | 无条件跳转。目的地写标签，汇编器自己算距离；目标写 x0 = 不留返回地址 |
| `mv a0, t0` | `addi a0, t0, 0` | move：a0 = t0，本质是"加 0" |

逐轮执行追踪（`asm_sum_to(10)`，进入时 a0=10）——循环是汇编最值得"人肉跑一遍"的地方：

| 轮次 | 进入循环时 t1 (i) | 本轮 bgt 判断 (i>10?) | 本轮 t0 变化 | 本轮结束时 t0 |
| --- | --- | --- | --- | --- |
| 1 | 1 | 否，继续 | 0 + 1 | 1 |
| 2 | 2 | 否 | 1 + 2 | 3 |
| 3 | 3 | 否 | 3 + 3 | 6 |
| … | … | … | … | … |
| 10 | 10 | 否 | 45 + 10 | 55 |
| 11 | 11 | **是 → 跳到 .Lsum_done** | 不执行 | 55 |

随后 `mv a0, t0`（a0=55）→ `ret`，C 拿到 55。C 里最普通的 `for` 循环，
在 CPU 眼里就是"**比较 → 跳转**"这两板斧的往复。

两个细节：

- `.L` 开头的标签是**局部标签**（GNU as 约定）：不进符号表、不污染全局命名空间、可重名。
  循环/分支的目标几乎都写它；
- 为什么 t0/t1 用完不保存？合同：t 系=调用者保存=大家默认"它随时会坏"。
  本函数没 call 别人，所以其实安全；但**跨 call 还想要的值，别放 t 系**——这是习惯问题。

### 5.6 asm_factorial 递归与栈帧

最难也最精彩的一课：用汇编实现递归（调用自己）。先补两块背景知识。

**背景一：栈是什么？**

- 栈 = RAM 里划出的"暂存区"，特点是**后进先出**（像叠盘子）；
- `sp` 永远指向栈顶（当前空位）；
- RISC-V 约定栈向**低地址**生长：存东西前先把 sp 减小（腾地方），取完再加回去（还地方）；
- 每个线程有自己的栈（Zephyr 切换线程时换 sp），本 demo 里 main 的栈由内核启动时安排好。

**背景二：为什么必须保存 ra？**

- `call` 每次执行都会把"返回地址"写进 ra——而 **ra 只有一个**；
- `factorial(3)` 递归调用 `factorial(2)` 时，第二次 call 会**覆盖**第一次的 ra；
- 如果不保存，最深一层返回后，外层全部"失忆"，跳去错误地址——症状是程序跑飞/无声挂死，极难排查；
- 结论：**只要函数还会 call 别人（非叶子函数），进门第一件事就是把 ra 押到栈上**。

本函数的"账本"——栈帧布局（一次开 8 字节，装两样东西）：

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
	addi    sp, sp, 8       /* 回收栈帧：sp 必须还回原位（栈平衡！）*/

	mul     a0, a0, t1      /* a0 = (n-1)! × n */
	ret                     /* 用恢复过的 ra 回家 */
.Lfac_base:
	li      a0, 1           /* 0! = 1! = 1（叶子分支：不开帧）*/
	ret
	.size  asm_factorial, .-asm_factorial
```

新指令只有两条访存指令，语法是 `偏移(基址寄存器)`：

| 指令 | 方向 | 作用 |
| --- | --- | --- |
| `sw rs, 偏移(sp)` | 寄存器 → 内存 | store word：把 rs 的 32 位存进地址 `sp+偏移`。偏移是 12 位有符号（±2048），所以栈帧一大就得先动 sp 本身 |
| `lw rd, 偏移(sp)` | 内存 → 寄存器 | load word：从 `sp+偏移` 读 32 位装进 rd。`sw`/`lw` 是 RISC-V 读写内存的基本手段 |

`factorial(3)` 全过程推演（建议之后用 [6.2 GDB](#62-用-gdb-单步调试) 单步亲自走一遍）：

**下潜阶段**（每层开帧、ra 越押越多）：

| 时刻 | sp 相对位置 | 本帧栈上押着的东西 | a0 | ra 当前指向 |
| --- | --- | --- | --- | --- |
| main 执行 `call fac(3)` | — | — | 3 | → main 中 call 的下一行 |
| fac(3) 进门，开帧 | sp₁ | `[n=3][ra→main]` | 3 | 仍指向 main（**正被 sw 押进栈**） |
| fac(3) 执行 `call fac(2)` | sp₁ | 同上 | 2 | → fac(3) 内 call 的下一行（**ra 被覆盖！**） |
| fac(2) 进门，开帧 | sp₂（=sp₁−8） | `[n=2][ra→fac(3)内]` | 2 | 仍指向 fac(3) 内（正被押进栈） |
| fac(2) 执行 `call fac(1)` | sp₂ | 同上 | 1 | → fac(2) 内 call 的下一行（ra 再次被覆盖） |
| fac(1) 进门，命中出口 | **不开帧** | 栈保持 sp₂ | 1 | fac(2) 内 call 的下一行（未被覆盖，直接可用） |

最深时刻的栈全景：

```
        ┌───────────────────────────┐
 sp₁ →  │ fac(3) 的帧： n=3 │ ra→main │   ← 还押着
        ├───────────────────────────┤
 sp₂ →  │ fac(2) 的帧： n=2 │ ra→fac(3)│  ← 还押着
        ├───────────────────────────┤
        │   fac(1) 是出口，没开帧      │   ← 正在执行 li a0,1; ret
        └───────────────────────────┘
```

**回升阶段**（逐层取回、相乘、还栈）：

1. `fac(1)` ret → 回到 `fac(2)` 中 call 的下一行，a0=1（即 1!）；
2. `fac(2)`：`lw t1` 取回 n=2，`lw ra` 取回自己押的返回地址，`mul` 得 a0 = 1×2 = 2，
   `sp += 8` 还帧，ret → 回到 `fac(3)`；
3. `fac(3)` 同法：a0 = 2×3 = 6，ret 回 main。

答案 3! = 6；`main.c` 实际调用 `asm_factorial(6)` = 720——同一套流程走 6 层而已。

自查清单（改这个函数练习时对照）：

- 忘了 `sw ra` → 递归一深 ra 就被覆盖，返回时跑飞（**新手第一大坑**）；
- `sw ra` 和 `lw ra` 的偏移不一致 → 取回的是垃圾；
- `-8` 和 `+8` 不配对 → 栈不平衡，上层全部错位；
- 先 `mul` 后 `lw`（顺序反了）→ 用的是下一层的数据。

### 5.7 动手练习与翻译词典

**练习（由浅入深，改完直接 `./make.sh` 看结果）：**

1. 写 `asm_max(int a, int b)` 返回较大值。提示：`blt a0, a1, 标签` + `mv`；
   在 main.c 加 `extern int asm_max(int, int);` 和一行 printk 验证；
2. 把 `asm_sum_to` 改成只加偶数（`addi t1, t1, 2`？初始值该是多少？）；
3. 用 [6.2 GDB](#62-用-gdb-单步调试) 断在 `asm_factorial`，`stepi` 单步走一遍
   [5.6](#56-asm_factorial-递归与栈帧) 的推演，用 `info registers sp ra a0` 亲眼看栈的下潜与回升。

**C ↔ 汇编"翻译词典"**（写新函数时按需查）：

| 你想表达的 C | 汇编写法 | 本 demo 的例子 |
| --- | --- | --- |
| `return a + b;` | `add` | asm_add |
| `return a - b;` | `sub` | asm_sub |
| `return a * b;` | `mul`（要 M 扩展） | asm_mul |
| `int x = 常数;` | `li x, 常数`（伪指令） | `li t0, 0` |
| `x++` | `addi x, x, 1` | `addi t1, t1, 1` |
| `if (a > b) …` | `bgt`/`bge`/`beq`/`bne`… | `bgt t1, a0, .Lsum_done` |
| `if (n <= 1) …` | `ble`（伪指令，= bge 反向） | `ble a0, t0, .Lfac_base` |
| `goto 标签` / 循环回边 | `j 标签`（伪指令） | `j .Lsum_loop` |
| 调用函数 | `call 名字`（= `jal ra, …`） | `call asm_factorial` |
| 函数返回 | `ret`（= `jalr x0, 0(ra)`） | 各函数末尾 |
| `return x;` | `mv a0, x` + `ret` | `mv a0, t0` |
| 局部变量 | 优先放寄存器（t/s 系）；要跨 call 或放不下 → 栈 | t0/t1 与栈上的 n |
| 保存/恢复现场 | `sw`/`lw` 配 `sp` 开帧回收 | factorial 全套 |

**延伸阅读**（出圈后的两步）：

- [RISC-V 指令集官方手册](https://github.com/riscv/riscv-isa-manual)（免费）：
  所有指令的"字典"，RV32I 全部指令一页可查，伪指令清单在卷一附录 A；
- [Zephyr 上游 arch/riscv](https://github.com/zephyrproject-rtos/zephyr/tree/v4.1.0/arch/riscv)：
  看内核怎么用同样的指令写启动与上下文切换（`reset.S`、`switch.S`）。

---

## 6. 进阶反汇编与 GDB 调试

### 6.1 查看反汇编

把[第 5 章](#5-逐行读懂汇编代码)的每条指令与真实机器码对上——亲眼验证"伪指令"的存在：

```bash
OBJDUMP=$HOME/zephyr-sdk-0.17.0/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-objdump
ELF=$HOME/workspace/zephyrproject/build/build_qemu_riscv32_asm/zephyr/zephyr.elf

$OBJDUMP -d $ELF                       # 全部
$OBJDUMP -d --disassemble=asm_factorial $ELF   # 只看某个函数
```

会看到类似：

```
80000174 <asm_add>:
80000174:  952e      add  a0,a0,a1
80000176:  8082      ret
```

左边是地址和机器码，右边是助记符。两个观察：

- `add` 只占 **2 字节**（0x952e）而非 4 字节——因为 `-march` 带 C 扩展，
  存的是压缩指令 `c.add`（编码怎么读见 [5.1](#51-开始之前-三个最基本的概念)，扩展集见
  [3.4](#34-工具链怎么选成-risc-v-交叉编译器)）；RAM 基址 0x80000000 的由来见
  [3.3](#33-soc-与-dts-cpu-和内存长什么样)（asm_add 的地址 = 基址 + 前面启动段的大小偏移）；
- `ret` 的机器码 `0x8082` 是压缩指令 `c.jr ra`，语义等于 `jalr x0, 0(ra)`——
  CPU 里根本没有"ret"，印证了它是伪指令。

### 6.2 用 GDB 单步调试

单步走一遍 [5.6 factorial 的推演](#56-asm_factorial-递归与栈帧)，是理解栈帧最快的路。

终端 A：启动 QEMU 并**暂停等待调试器**（`-s` 开 gdbstub 端口 1234，`-S` 启动即暂停）：

```bash
QEMU=$HOME/zephyr-sdk-0.17.0/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-riscv32
ELF=$HOME/workspace/zephyrproject/build/build_qemu_riscv32_asm/zephyr/zephyr.elf
$QEMU -nographic -machine virt -bios none -m 256 -s -S -kernel $ELF
```

终端 B：连上去调试：

> **注意**：shell 变量**只在定义它的那个终端里有效**——终端 A 里定义的
> `QEMU`/`ELF` 在终端 B 里不存在，所以这里必须**重新定义** `ELF`
> （GDB 依靠它加载符号表；漏掉它启动时会警告
> `No executable has been specified`，`break` 会报
> `No symbol table is loaded`）。

```bash
GDB=$HOME/zephyr-sdk-0.17.0/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-gdb
ELF=$HOME/workspace/zephyrproject/build/build_qemu_riscv32_asm/zephyr/zephyr.elf
$GDB $ELF                    # 启动时加载符号表（少了 $ELF 就是"裸连"）
(gdb) target remote :1234
(gdb) break asm_add          # 在汇编函数上设断点
(gdb) continue
(gdb) info registers a0 a1   # 看参数寄存器
(gdb) stepi                  # 单步执行一条指令
(gdb) info registers a0      # 看结果
(gdb) x/4i $pc               # 反汇编接下来的指令
```

> 连接后若停在 `0x00001000 in ?? ()`：这是 QEMU `virt` 机器的复位入口
> （QEMU 内置 ROM，跳一段后才到 ELF 入口 `0x80000000`），属正常现象，
> `continue` 即可。若 `break` 时被问
> `Make breakpoint pending on future shared library load?`，答 **n**
> （裸机静态程序没有共享库可等，答 y 断点永远不会生效）。

想练习什么，就把断点打在哪个函数上，用 `stepi` 一条条走，配合 `info registers` 观察变化。

> **注意**：不要用 `west build -t debug`——QEMU 板不支持它。
> `west debug/flash` 依赖构建生成的 `runners.yaml`，而 `qemu_riscv32` 的
> `board.cmake` 从不注册可用 runner（Zephyr 的 `qemu` runner 只是占位空壳，
> 不支持任何命令），执行会报 `FATAL ERROR: no runners.yaml found`。
> QEMU 板调试就用上面的手动两终端法；`west build -t run` 能用是因为
> 它由 CMake 直接起 QEMU，不经过 runner 机制。

---

## 7. 常见问题

| 现象 | 原因 / 解决 |
| --- | --- |
| `qemu-system-riscv32: command not found` | 装 `sudo apt-get install qemu-system-misc` |
| `cannot create PID file: Cannot lock pid file` | 上次 QEMU 没退干净。`pkill -f qemu-system-riscv32` 后删掉输出目录里的 `qemu.pid` |
| `west build -t run` 一直不返回 | 程序进入 Zephyr 空闲线程，属正常；按 `Ctrl-A` 再 `X` 退出 |
| `west build -t debug` 报 `FATAL ERROR: no runners.yaml found` | 正常现象：QEMU 板不支持 west flash/debug（`qemu` runner 是占位空壳）。调试用 [6.2 节](#62-用-gdb-单步调试)的手动两终端法 |
| 提示找不到 `ZEPHYR_BASE` / 板子 | 未激活 venv，或不在 `zephyrproject` 目录下执行 |
| 改了 `.S` 不重新编译 | 加 `-p always` 强制重建：`west build ... -p always` |
| 想换成 64 位 RISC-V | 把 `-b qemu_riscv32` 换成 `-b qemu_riscv64`，并相应使用 64 位寄存器（`a0` 等仍是 64 位宽） |

---

## 附：本项目文件结构与教学顺序

```
riscv_asm_learn/
├── CMakeLists.txt      # 声明把 main.c 与 asm_funcs.S 一起编译
├── prj.conf            # Kconfig 配置（串口控制台，板子默认已开）
├── make.sh             # 一键编译 + 运行脚本（见第 2 节）
├── .clangd             # 编辑器跳转配置，指向本项目的 compile_commands.json
├── README.md           # 本文档
└── src/
    ├── main.c          # C 调用汇编并打印
    └── asm_funcs.S     # 5 个汇编函数
```

建议学习顺序（由浅入深，均链接到[第 5 章](#5-逐行读懂汇编代码)对应小节）：

1. **寄存器与 ABI** → 先看 [5.2 调用约定](#52-risc-v-的调用约定-ilp32)，
   再看 [asm_add](#53-asm_add-与-asm_sub-第一段汇编)，理解 `a0/a1` 和返回值
2. **算术指令** → [asm_sub](#53-asm_add-与-asm_sub-第一段汇编)（`sub`）、
   [asm_mul](#54-asm_mul-与-m-扩展)（`mul` + M 扩展）
3. **控制流** → [asm_sum_to](#55-asm_sum_to-循环与分支)（`li/add/bgt/j` 循环）
4. **函数调用与栈** → [asm_factorial](#56-asm_factorial-递归与栈帧)
   （`call/ret`、`sw/lw`、栈帧、保存 `ra`）
5. **验证理解** → 自己加一个 `asm_max(int a, int b)` 返回较大值，编译并在 QEMU 里运行
   （练习见 [5.7](#57-动手练习与翻译词典)）

