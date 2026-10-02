# Zephyr 应用集合

本仓库集中管理运行在 Zephyr RTOS 上的**自研应用代码**，与上游 Zephyr 源码、
模块（modules）彼此独立。

## 环境

- Zephyr v4.1.0，工作区 `west` 位于 `/home/ubuntu/workspace/zephyrproject`
- Zephyr SDK 0.17.0
- 该仓库仅包含应用代码，不含 zephyr/modules 等上游内容

## 目录结构

```
apps/
├── riscv_asm_learn/          # QEMU RISC-V 32 汇编学习示例
│   ├── CMakeLists.txt
│   ├── prj.conf
│   └── src/
│       ├── main.c
│       └── asm_funcs.S
├── .gitignore
└── README.md
```

## 编译与运行

所有产物统一输出到 `zephyrproject/build/build_<项目名>/`：

```bash
cd /home/ubuntu/workspace/zephyrproject

# QEMU RISC-V 32（需 sudo apt install qemu-system-misc）
west build -b qemu_riscv32 apps/riscv_asm_learn -d build/build_qemu_riscv32_asm
west build -d build/build_qemu_riscv32_asm -t run    # 退出：Ctrl-A 然后 X
```

## 说明

- 上游 Zephyr 与 modules 不纳入本仓库；它们的版本由 west manifest 记录。
- 若需修改某个 modules，应 fork 官方仓库并在 manifest 中指向自己的 fork，
  而不是直接修改工作区内的 modules 目录。