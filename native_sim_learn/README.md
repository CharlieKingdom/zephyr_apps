# Zephyr native_sim 外设学习工程

一个**不需要真实开发板**的 Zephyr 入门工程：用 `native_sim` 板把 Zephyr 编译成
一个普通的 Linux 可执行文件（`zephyr.exe`），程序自己充当“主板”，并自带若干
外设模型，让你在 PC 上就能练习 GPIO、按键中断、EEPROM、随机数等常见外设的写法。

- 目标板：`native_sim`（PC 本机仿真，无需 QEMU、无需硬件）
- Zephyr：v4.1.0
- 演示外设：GPIO 输出(LED)、GPIO 输入+中断(按键)、EEPROM、熵源(RNG)

---

## 目录

1. [为什么 native_sim 不需要板子](#1-为什么-native_sim-不需要板子)
2. [前置环境](#2-前置环境)
3. [编译](#3-编译)
4. [运行与预期输出](#4-运行与预期输出)
5. [四个演示逐段讲解](#5-四个演示逐段讲解)
6. [镜像里“外设”从哪来：设备树与 Kconfig](#6-镜像里外设从哪来设备树与-kconfig)
7. [native_sim 命令行选项（会玩才好玩）](#7-native_sim-命令行选项会玩才好玩)
8. [与 QEMU 方案对比](#8-与-qemu-方案对比)
9. [常见问题](#9-常见问题)

---

## 1. 为什么 native_sim 不需要板子

Zephyr 的 `native_sim` 板属于 **POSIX 架构**：它不模拟某一颗具体 CPU，而是把你的
应用代码 + Zephyr 内核 + 驱动，**直接用宿主机编译器（gcc）编译成 x86 程序**。

因此：

- **不需要交叉编译器、不需要 QEMU、不需要硬件**；
- 外设（GPIO、EEPROM、串口、显示……）由 native_sim 自带的**模型/仿真器**提供，
  它们在宿主机内存里“假装”是真实外设；
- 程序就是普通 Linux 进程，可以用 `gdb`、`valgrind` 直接调试，仿真时间默认
  与真实时间解耦（跑得飞快），也支持 `--rt` 锁到真实时间。

> native_sim 是老的 `native_posix` 的演进版，二者用法基本兼容。

本工程自带的板级设备树 `native_sim.dts` 里已经定义好了这些外设节点：

| 设备树节点 | 兼容字符串 | 作用 |
| --- | --- | --- |
| `gpio0` | `zephyr,gpio-emul` | 可被程序读写的 GPIO 仿真控制器 |
| `led0` | `gpio-leds` | 挂在 `gpio0 pin0` 上的 LED |
| `eeprom0` | `zephyr,sim-eeprom` | EEPROM 仿真，数据落盘到 `eeprom.bin` |
| `rng` | `zephyr,native-posix-rng` | 熵源（基于宿主机随机数） |
| `uart0` | `zephyr,native-posix-uart` | 串口控制台 |
| `flash0` | `zephyr,sim-flash` | Flash 仿真，落盘到 `flash.bin` |

---

## 2. 前置环境

假设 Zephyr 工作区位于 `/home/ubuntu/workspace/zephyrproject`。

```bash
# 激活 Zephyr 的 Python 虚拟环境（~/.bashrc 已配置，新终端自动生效）
source /home/ubuntu/workspace/zephyrproject/.venv/bin/activate
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

west --version          # 期望 1.5.0
cd /home/ubuntu/workspace/zephyrproject
west list zephyr         # 确认工作区正常
```

**唯一的宿主机依赖**：`native_sim` 默认是 32 位目标（ILP32，更贴近多数嵌入式
ABI），需要 32 位的 gcc 运行库。若尚未安装：

```bash
sudo apt-get install -y gcc-multilib libc6-dev-i386
```

如果你无法安装 32 位库，也可以改用 64 位目标 `native_sim/native/64`，把下文
所有 `-b native_sim` 换成 `-b native_sim/native/64` 即可。

---

## 3. 编译

一条命令（在 `zephyrproject` 目录下执行）：

```bash
cd /home/ubuntu/workspace/zephyrproject

west build -b native_sim apps/native_sim_learn \
           -d build/build_native_sim_learn
```

参数拆解：

| 参数 | 含义 |
| --- | --- |
| `west build` | Zephyr 构建命令（底层 CMake + Ninja） |
| `-b native_sim` | 目标板 = native_sim（PC 本机仿真） |
| `apps/native_sim_learn` | 应用源码目录 |
| `-d build/build_native_sim_learn` | 输出目录（命名约定：`build_<板子>_<用途>`） |

构建时你会看到一行，说明应用目录下的设备树覆盖被自动加载了：

```
-- Found devicetree overlay: .../apps/native_sim_learn/boards/native_sim.overlay
...
Generating files from .../build_native_sim_learn/zephyr/zephyr.elf for board: native_sim
```

产物：

```
build/build_native_sim_learn/zephyr/zephyr.exe   ← 可执行的 Linux 程序
build/build_native_sim_learn/zephyr/zephyr.elf   ← 同上的 ELF
build/build_native_sim_learn/zephyr/.config      ← 最终 Kconfig 配置
```

---

## 4. 运行与预期输出

`native_sim` 的产物**不是**交给 QEMU，而是**直接当普通程序运行**：

```bash
cd /home/ubuntu/workspace/zephyrproject
./build/build_native_sim_learn/zephyr/zephyr.exe
```

预期输出（节选）：

```
WARNING: Using a test - not safe - entropy source
*** Booting Zephyr OS build v4.1.0 ***

=================================================
 Zephyr native_sim 外设学习示例
 无需真实开发板：程序直接作为 Linux 进程运行
=================================================

=== [1] GPIO 输出：LED (led0 / gpio0 pin0) ===
  第 1 次翻转 -> led0 = 亮(高电平)
  第 2 次翻转 -> led0 = 灭(低电平)
  ...

=== [2] GPIO 输入 + 中断：按键 (sw0 / gpio0 pin2) ===
  等待 3 次按键中断...
  [仿真] 第 1 次按下  (注入低电平)
  [中断] 按键按下！t = 1650 ms
  ...

=== [3] EEPROM：非易失存储 (eeprom-0) ===
  EEPROM 容量：32768 字节
  启动次数 = 1（多次运行 zephyr.exe，该值会持续累加）

=== [4] 熵源 / 随机数 (rng) ===
  取到 16 字节随机数：3b db 31 cc ...
  （native_sim 默认使用固定种子，加 --seed=<n> 可复现同一序列）

全部演示结束。
```

程序末尾调用了 `posix_exit(0)`（仅 POSIX 架构可用），所以它**会自己退出**，
不会像普通 Zephyr 程序那样停在 idle 线程里。

> 关于 `eeprom.bin`：EEPROM 数据保存在**当前工作目录**下的 `eeprom.bin` 里。
> 因此建议 `cd build/build_native_sim_learn` 后再运行，或像上面一样任意指定一个
> 目录；连续运行两次，就能看到“启动次数”从 1 变成 2（掉电保持效果）。
>
> EEPROM / Flash 默认都会在 CWD 生成 `eeprom.bin` / `flash.bin`。

---

## 5. 四个演示逐段讲解

### [1] GPIO 输出：LED

见 [src/app_led.c](file:///home/ubuntu/workspace/zephyrproject/apps/native_sim_learn/src/app_led.c) 的 `demo_led()`。

```c
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);  // 配置为输出
gpio_pin_toggle_dt(&led);                           // 翻转电平
int level = gpio_emul_output_get(led.port, led.pin); // 读回仿真器里的输出电平
```

要点：

- `GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)`：**从设备树 alias 取得设备+引脚**，
  这是 Zephyr 推荐的写法（把“哪个引脚”交给设备树，而不是写死在代码里）。
- 普通 Zephyr 程序用 `gpio_pin_get_dt()` 读的是**输入**；对于纯输出引脚，读回的
  是输入寄存器（通常无意义）。仿真器额外提供了 `gpio_emul_output_get()`，
  可以直接读回**输出电平**，方便在无示波器/无灯珠时验证代码。

### [2] GPIO 输入 + 中断：按键

见 [src/app_button.c](file:///home/ubuntu/workspace/zephyrproject/apps/native_sim_learn/src/app_button.c) 的 `demo_button()` 与 `button_sim_thread()`。按键 `sw0` 由本工程的设备树覆盖
[boards/native_sim.overlay](file:///home/ubuntu/workspace/zephyrproject/apps/native_sim_learn/boards/native_sim.overlay)
定义在 `gpio0 pin2`，**上拉 + 低电平有效**（和真实开发板的用户按键一致）。

```dts
buttons {
    compatible = "gpio-keys";
    button0: button_0 {
        gpios = <&gpio0 2 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
        label = "Simulated button";
    };
};
aliases { sw0 = &button0; };
```

代码步骤与真实板子完全一致：

```c
gpio_pin_configure_dt(&button, GPIO_INPUT);
gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE); // 按下时触发
gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
gpio_add_callback(button.port, &button_cb);
```

**“按键动作”从哪来？** 真实世界是手指去按。仿真环境下，我们让一个线程调用
GPIO 仿真器 API 去改引脚电平，等于“替手指按下按键”：

```c
gpio_emul_input_set(button.port, button.pin, 0); // 注入低电平 = 按下
gpio_emul_input_set(button.port, button.pin, 1); // 注入高电平 = 松开
```

按下瞬间，GPIO 仿真器触发边沿中断 → 回调 `button_isr()` 执行 → 通过信号量
唤醒主线程计数。这就完整跑通了一条 **输入 → 中断 → 业务处理** 的链路。

> 原理补充：`gpio-emul` 驱动还支持“把两个引脚连起来”（在回调里手动把输出
> 引脚的值转写到输入引脚）。Zephyr 官方测试
> `tests/drivers/gpio/gpio_basic_api` 就是这样模拟外部电路的。

### [3] EEPROM：非易失存储

见 [src/app_eeprom.c](file:///home/ubuntu/workspace/zephyrproject/apps/native_sim_learn/src/app_eeprom.c) 的 `demo_eeprom()`。`eeprom-0` 是 native_sim 自带的 `zephyr,sim-eeprom`，
容量 32KB，数据落盘到宿主机当前目录的 `eeprom.bin`。

```c
const struct device *eeprom = DEVICE_DT_GET(DT_ALIAS(eeprom_0));
eeprom_read (eeprom, 0, &rec, sizeof(rec));
eeprom_write(eeprom, 0, &rec, sizeof(rec));
```

这里用一个“启动次数”计数器演示：第一次运行写 1，之后再运行读到 1 并写回 2……
**重启不丢数据**。这正是设备参数保存、日志存储的典型套路。可以用命令行
`--eeprom=<路径>` 指定数据文件位置。

### [4] 熵源 / 随机数

见 [src/app_entropy.c](file:///home/ubuntu/workspace/zephyrproject/apps/native_sim_learn/src/app_entropy.c) 的 `demo_entropy()`。native_sim 提供一个基于宿主机 `random()` 的熵源设备
（设备树 chosen 节点 `zephyr,entropy`）：

```c
const struct device *rng = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
entropy_get_entropy(rng, buf, sizeof(buf));
```

它**不是真随机**（启动时会打印 `WARNING: Using a test - not safe - entropy source`），
但用途很关键：它让 native_sim 具备**确定性**——同一个 `--seed` 必然产生同一串
随机数，非常适合做可复现的自动化测试。

---

## 6. 镜像里“外设”从哪来：设备树与 Kconfig

理解这两点，你就掌握了 Zephyr 外设开发的主线：

1. **设备树（DTS）决定“有哪些设备、接在哪个引脚”**
   - 板子自带的 `native_sim.dts` 定义了 `gpio0 / led0 / eeprom0 / rng ...`；
   - 应用目录下的 `boards/<board>.overlay` 会在其上做**覆盖/追加**，
     本工程用它新增了按键 `sw0`；
   - 代码里通过 `DT_ALIAS(...)` / `DT_CHOSEN(...)` 引用，**不写死引脚号**。

2. **Kconfig（prj.conf）决定“编译进哪些驱动/子系统”**
   - 见本工程 [prj.conf](file:///home/ubuntu/workspace/zephyrproject/apps/native_sim_learn/prj.conf)：
     `CONFIG_GPIO=y`、`CONFIG_GPIO_EMUL=y`、`CONFIG_EEPROM=y`、
     `CONFIG_ENTROPY_GENERATOR=y`；
   - 很多驱动是“**设备树里存在该节点就自动使能**”的（例如 `EEPROM_SIMULATOR`、
     `FAKE_ENTROPY_NATIVE_POSIX` 默认 `y`），所以 prj.conf 里只需打开子系统。

想确认最终配置？直接看构建产物：

```bash
grep -E "CONFIG_(GPIO|EEPROM|ENTROPY)" build/build_native_sim_learn/zephyr/.config
```

---

## 7. native_sim 命令行选项（会玩才好玩）

因为产物就是普通程序，很多行为可用**命令行参数**控制（等价于真实板的“跳线/接线”）：

```bash
./zephyr.exe --help          # 查看当前配置下所有可用选项
```

常用项：

| 选项 | 作用 |
| --- | --- |
| `--seed=<n>` | 设置随机数种子，复现同一随机序列 |
| `--eeprom=<path>` | 指定 EEPROM 落盘文件（默认 CWD 下的 `eeprom.bin`） |
| `--flash=<path>` | 指定 Flash 落盘文件（默认 `flash.bin`） |
| `--rt` / `--no-rt` | 仿真时间是否锁定到真实时间（默认可配置） |
| `--rt-ratio=<r>` | 仿真时间相对真实时间的倍率，如 `--rt-ratio=2` 表示 2 倍速 |
| `--rtc-reset` | RTC 从 0 开始计时（默认取宿主机当前时间） |
| `--color` / `--no-color` | 日志是否带颜色 |

例：用固定种子跑，观察随机数完全一致：

```bash
./zephyr.exe --seed=123
./zephyr.exe --seed=123     # 两次输出的 16 字节随机数相同
```

---

## 8. 与 QEMU 方案对比

本工作区里另一个工程 [riscv_asm_learn](../riscv_asm_learn/README.md) 用的是
`qemu_riscv32`。两者都能“无板开发”，取向不同：

| 维度 | `native_sim` | `qemu_riscv32` |
| --- | --- | --- |
| 是否需要 QEMU | **不需要**（编译成 PC 原生程序） | 需要 `qemu-system-riscv32` |
| 指令集 | 宿主机 x86（非目标 ISA） | 真实 RISC-V，能验证指令/汇编 |
| 外设模型 | 丰富且可读写（GPIO/EEPROM/Flash/显示…） | 较基础（串口等） |
| 调试/测试 | 直接用 gdb/valgrind/ASan，确定性好 | 需配合 QEMU 的 gdb stub |
| 适合 | 应用逻辑、外设 API、子系统、自动化测试 | 指令集/汇编、底层架构学习 |

简单说：**学汇编/架构选 QEMU，学外设与应用开发选 native_sim。**

---

## 9. 常见问题

- **编译报 `bits/libc-header-start.h: No such file`**
  32 位库没装，执行 `sudo apt-get install -y gcc-multilib libc6-dev-i386`；
  或改用 `-b native_sim/native/64`。

- **程序跑完不退出？**
  普通 Zephyr 程序在 `main` 返回后进入 idle 线程，需 `Ctrl+C`。本工程末尾
  调用了 `posix_exit(0)`，因此会自行退出。

- **再次运行 EEPROM 的“启动次数”还是 1？**
  说明每次都在不同目录运行（`eeprom.bin` 存在各自的 CWD），或文件被删了。
  固定用同一目录运行即可看到累加。

- **改代码后要重新编译再跑**
  `west build -d build/build_native_sim_learn`（无需 `--pristine`），然后重新
  运行 `zephyr.exe`。

- **想加点新外设？**
  在 [boards/native_sim.overlay](file:///home/ubuntu/workspace/zephyrproject/apps/native_sim_learn/boards/native_sim.overlay)
  里加设备树节点，在 `prj.conf` 里打开对应 Kconfig，然后照本节套路写代码即可。