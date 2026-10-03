# RISC-V 32 汇编学习 Demo

一个跑在 **QEMU 模拟的 RISC-V 32 位机器**上的 Zephyr 程序。
用 C 调用 5 个**纯汇编**函数，通过串口打印结果，带你从 0 认识 RISC-V 架构。

- 目标板：`qemu_riscv32`（不需要真实硬件）
- 指令集：`rv32imac_zicsr_zifencei`（32 位，含 M/A/C 扩展）
- 调用约定：`ilp32`（软浮点）

---

## 目录

1. [前置环境](#1-前置环境)
2. [从 0 开始：编译](#2-从-0-开始编译)
3. [Zephyr 是怎么"选中"RISC-V 和 QEMU 的](#3-zephyr-是怎么选中-riscv-和-qemu-的)
4. [运行仿真](#4-运行仿真)
5. [逐行读懂汇编代码](#5-逐行读懂汇编代码)
6. [进阶：反汇编与 GDB 调试](#6-进阶反汇编与-gdb-调试)
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

## 2. 从 0 开始：编译

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
| `west build ...` | 真正的编译命令，逐参数见 2.2 |

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
| `-t run` | 构建完成后立即在 QEMU 中运行（详见第 4 节） |

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

## 3. Zephyr 是怎么"选中"RISC-V 和 QEMU 的

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

`arch: riscv` → Zephyr 载入 [zephyr/arch/riscv](file:///home/ubuntu/workspace/zephyrproject/zephyr/arch/riscv)，
用 RISC-V 的启动代码（`reset.S`、trap 处理、`switch.S` 上下文切换等）。

### 3.3 SoC / DTS：CPU 与内存长什么样

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

看 [board.cmake](file:///home/ubuntu/workspace/zephyrproject/zephyr/boards/qemu/riscv32/board.cmake)：

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

两者完全等价，方式二能让你看清 QEMU 到底接收了哪些参数。

---

## 5. 逐行读懂汇编代码

代码在 [src/asm_funcs.S](file:///home/ubuntu/workspace/zephyrproject/apps/riscv_asm_learn/src/asm_funcs.S)，
C 侧调用在 [src/main.c](file:///home/ubuntu/workspace/zephyrproject/apps/riscv_asm_learn/src/main.c)。

### 5.1 先记住 RISC-V 的调用约定（ilp32 ABI）

| 寄存器 | 别名 | 用途 |
| --- | --- | --- |
| `x10`–`x17` | `a0`–`a7` | 函数**参数**；`a0`/`a1` 兼作**返回值** |
| `x1` | `ra` | 返回地址（`ret` 靠它跳回） |
| `x2` | `sp` | 栈指针 |
| `x5`–`x7`,`x28`–`x31` | `t0`–`t6` | 临时寄存器（**调用者保存**，可随便用） |
| `x8`,`x9`,`x18`–`x27` | `s0`–`s11` | 被调用者保存（用了必须自己恢复） |
| `x0` | `zero` | 恒为 0（写它等于丢弃） |

口诀：**参数进 a，返回出 a，临时用 t，长期用 s，返回地址存 ra。**

### 5.2 最简单：`asm_add`（算术指令）

```asm
asm_add:
    add a0, a0, a1   # a0 = a0 + a1
    ret              # 返回
```

C 调用 `asm_add(12, 5)` 时：`a0=12, a1=5` → 执行后 `a0=17` → `ret` 把 17 带回 C。

### 5.3 `asm_mul`：需要 M 扩展

```asm
asm_mul:
    mul a0, a0, a1   # a0 = a0 * a1
    ret
```

`mul` 属于 **M 扩展**（乘除法）。基础 RV32I 是没有乘法的——这正是 `-march` 里那个 `m` 的意义。
如果把 `-march` 换成 `rv32i`，这条 `mul` 会编译失败。

### 5.4 `asm_sum_to`：循环与分支

```asm
asm_sum_to:                 # 计算 1+2+...+n
    li   t0, 0              # acc = 0
    li   t1, 1              # i   = 1
.Lsum_loop:
    bgt  t1, a0, .Lsum_done # if (i > n) goto done
    add  t0, t0, t1         # acc += i
    addi t1, t1, 1          # i++
    j    .Lsum_loop         # goto loop
.Lsum_done:
    mv   a0, t0             # 返回值 = acc
    ret
```

要点：
- `li` 是**伪指令**（load immediate），汇编器会按立即数大小展开成 `addi` 或 `lui/addi`。
- `.L` 开头的标签是**局部标签**，不进入符号表。
- 分支指令 `bgt`（greater than）、`beq`、`bne`、`blt`、`ble`、`bge` 等，是 RISC-V 唯一的条件跳转。

### 5.5 `asm_factorial`：递归与栈帧

```asm
asm_factorial:               # n!
    li   t0, 1
    ble  a0, t0, .Lfac_base  # if (n <= 1) return 1

    addi sp, sp, -8          # 开栈帧（8 字节）
    sw   ra, 4(sp)           # 保存返回地址（递归会覆盖 ra！）
    sw   a0, 0(sp)           # 保存当前 n

    addi a0, a0, -1          # a0 = n-1
    call asm_factorial       # 递归；返回时 a0 = (n-1)!

    lw   t1, 0(sp)           # 恢复 n
    lw   ra, 4(sp)           # 恢复 ra
    addi sp, sp, 8           # 回收栈帧

    mul  a0, a0, t1          # a0 = (n-1)! * n
    ret
.Lfac_base:
    li   a0, 1
    ret
```

**为什么必须保存 `ra`**：`call` 会把返回地址写进 `ra`。递归调用会覆盖它，
所以每一层必须先把 `ra` 压栈。这是理解"函数调用如何工作"最典型的一课。

---

## 6. 进阶：反汇编与 GDB 调试

### 6.1 看反汇编（把指令和机器码对上）

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

左边是地址和机器码，右边是助记符。`ret` 的机器码 `0x8082` 其实就是
`jalr x0, 0(ra)` 的压缩编码——印证了 ret 是伪指令。

### 6.2 用 GDB 单步调试（观察寄存器变化）

终端 A：启动 QEMU 并**暂停等待调试器**（`-s` 开 gdbstub 端口 1234，`-S` 启动即暂停）：

```bash
QEMU=$HOME/zephyr-sdk-0.17.0/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-riscv32
ELF=$HOME/workspace/zephyrproject/build/build_qemu_riscv32_asm/zephyr/zephyr.elf
$QEMU -nographic -machine virt -bios none -m 256 -s -S -kernel $ELF
```

终端 B：连上去调试：

```bash
GDB=$HOME/zephyr-sdk-0.17.0/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-gdb
$GDB $ELF
(gdb) target remote :1234
(gdb) break asm_add          # 在汇编函数上设断点
(gdb) continue
(gdb) info registers a0 a1   # 看参数寄存器
(gdb) stepi                  # 单步执行一条指令
(gdb) info registers a0      # 看结果
(gdb) x/4i $pc               # 反汇编接下来的指令
```

想练习什么，就把断点打在哪个函数上，用 `stepi` 一条条走，配合 `info registers` 观察变化。

> 也可以用 `west build -d build/build_qemu_riscv32_asm -t debug`，
> Zephyr 会按板子的 `board.cmake` 自动起 QEMU + gdb。

---

## 7. 常见问题

| 现象 | 原因 / 解决 |
| --- | --- |
| `qemu-system-riscv32: command not found` | 装 `sudo apt-get install qemu-system-misc` |
| `cannot create PID file: Cannot lock pid file` | 上次 QEMU 没退干净。`pkill -f qemu-system-riscv32` 后删掉输出目录里的 `qemu.pid` |
| `west build -t run` 一直不返回 | 程序进入 Zephyr 空闲线程，属正常；按 `Ctrl-A` 再 `X` 退出 |
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

建议学习顺序（由浅入深）：

1. **寄存器与 ABI** → 看 `asm_add`，理解 `a0/a1` 和返回值
2. **算术指令** → `asm_sub`（`sub`）、`asm_mul`（`mul` + M 扩展）
3. **控制流** → `asm_sum_to`（`li/add/bgt/j` 循环）
4. **函数调用与栈** → `asm_factorial`（`call/ret`、`sw/lw`、栈帧、保存 `ra`）
5. **验证理解** → 自己加一个 `asm_max(int a, int b)` 返回较大值，编译并在 QEMU 里运行