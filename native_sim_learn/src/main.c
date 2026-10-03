/*
 * native_sim 外设学习示例 —— 程序入口
 *
 * 目标板：native_sim
 *   —— 在 PC 上把 Zephyr 编译成一个普通的 Linux 可执行文件（zephyr.exe），
 *      程序本身充当“主板”，并自带若干外设模型，无需任何真实开发板。
 *
 * 本示例依次演示 4 类常见外设，各部分已拆分为独立模块：
 *   [1] GPIO 输出      ：app_led.c      —— 翻转 led0，并通过仿真器读回引脚电平
 *   [2] GPIO 输入+中断 ：app_button.c   —— sw0 按键，按下时触发中断
 *   [3] EEPROM         ：app_eeprom.c   —— 非易失存储，重启后数据仍在
 *   [4] 熵源 / 随机数  ：app_entropy.c  —— 从 rng 设备读取随机字节
 *
 * 运行：west build -b native_sim apps/native_sim_learn -d build/build_native_sim_learn
 *       build/build_native_sim_learn/zephyr/zephyr.exe
 */

#include <zephyr/sys/printk.h>

#include "app_led.h"
#include "app_button.h"
#include "app_eeprom.h"
#include "app_entropy.h"

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"	/* posix_exit()：让 native_sim 程序干净退出 */
#endif

int main(void)
{
	printk("\n");
	printk("=================================================\n");
	printk(" Zephyr native_sim 外设学习示例\n");
	printk(" 无需真实开发板：程序直接作为 Linux 进程运行\n");
	printk("=================================================\n");

	demo_led();
	demo_button();
	demo_eeprom();
	demo_entropy();

	printk("\n全部演示结束。\n");

#ifdef CONFIG_ARCH_POSIX
	posix_exit(0);
#endif
	return 0;
}