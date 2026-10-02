/*
 * RISC-V (RV32) 汇编学习示例 —— C 侧调用汇编函数
 *
 * 函数原型见下方 extern 声明，实现全部在 src/asm_funcs.S 中。
 * 教学点：RISC-V 调用约定（ilp32 ABI）
 *   - 参数：a0, a1, a2, ... a7
 *   - 返回：a0
 *   - 临时寄存器：t0-t6（调用者保存）
 *   - 被调用者保存：s0-s11、sp、ra
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

extern int asm_add(int a, int b);        /* add 指令         */
extern int asm_sub(int a, int b);        /* sub 指令         */
extern int asm_mul(int a, int b);        /* mul 指令（M 扩展）*/
extern int asm_sum_to(int n);            /* 循环 + 分支      */
extern int asm_factorial(int n);         /* 递归 + 栈帧      */

int main(void)
{
	int a = 12, b = 5;

	printk("=== RISC-V RV32 汇编学习示例 ===\n");
	printk("asm_add(%d, %d)       = %d\n", a, b, asm_add(a, b));
	printk("asm_sub(%d, %d)       = %d\n", a, b, asm_sub(a, b));
	printk("asm_mul(%d, %d)       = %d\n", a, b, asm_mul(a, b));
	printk("asm_sum_to(%d)        = %d\n", 10, asm_sum_to(10));
	printk("asm_factorial(%d)     = %d\n", 6, asm_factorial(6));
	printk("=== 演示结束 ===\n");

	return 0;
}