/*
 * RISC-V (RV32) 汇编实现 —— 供 C 调用
 *
 * 汇编语法：GNU as，按 ilp32 ABI 传参。
 *   第一个参数 a0，第二个 a1，返回值放 a0。
 *   被调用函数若使用 s0-s11 必须自行保存/恢复。
 */

	.section .text

/* ---------------------------------------------------------------
 * int asm_add(int a, int b)   =>  a0 = a0 + a1
 * 对应 C：return a + b;
 * --------------------------------------------------------------- */
	.globl asm_add
	.type  asm_add, @function
asm_add:
	add     a0, a0, a1      /* a0 = a0 + a1 */
	ret                     /* 返回，等价于 jalr x0, 0(ra) */
	.size  asm_add, .-asm_add

/* ---------------------------------------------------------------
 * int asm_sub(int a, int b)   =>  a0 = a0 - a1
 * --------------------------------------------------------------- */
	.globl asm_sub
	.type  asm_sub, @function
asm_sub:
	sub     a0, a0, a1      /* a0 = a0 - a1 */
	ret
	.size  asm_sub, .-asm_sub

/* ---------------------------------------------------------------
 * int asm_mul(int a, int b)   =>  a0 = a0 * a1
 * 需要 M 扩展（qemu_riscv32 的 ISA 为 rv32imac，已包含）
 * --------------------------------------------------------------- */
	.globl asm_mul
	.type  asm_mul, @function
asm_mul:
	mul     a0, a0, a1      /* a0 = a0 * a1（低 32 位）*/
	ret
	.size  asm_mul, .-asm_mul

/* ---------------------------------------------------------------
 * int asm_sum_to(int n)   =>  1 + 2 + ... + n
 * 教学：循环、条件分支、局部标签（数字标签可重复）
 *   t0 = 累加和(acc)   t1 = 计数器(i)
 * --------------------------------------------------------------- */
	.globl asm_sum_to
	.type  asm_sum_to, @function
asm_sum_to:
	li      t0, 0           /* acc = 0 */
	li      t1, 1           /* i = 1   */
.Lsum_loop:
	bgt     t1, a0, .Lsum_done   /* if (i > n) 跳出循环 */
	add     t0, t0, t1      /* acc += i */
	addi    t1, t1, 1       /* i++      */
	j       .Lsum_loop      /* 无条件跳回循环开头 */
.Lsum_done:
	mv      a0, t0          /* 返回值 = acc */
	ret
	.size  asm_sum_to, .-asm_sum_to

/* ---------------------------------------------------------------
 * int asm_factorial(int n)   =>  n!（递归）
 * 教学：栈帧、保存 ra 与中间值、递归调用
 *   栈布局： 0(sp)=n  4(sp)=ra
 * --------------------------------------------------------------- */
	.globl asm_factorial
	.type  asm_factorial, @function
asm_factorial:
	li      t0, 1
	ble     a0, t0, .Lfac_base   /* if (n <= 1) 返回 1 */

	addi    sp, sp, -8      /* 开栈帧 */
	sw      ra, 4(sp)       /* 保存返回地址 */
	sw      a0, 0(sp)       /* 保存当前 n  */

	addi    a0, a0, -1      /* a0 = n - 1 */
	call    asm_factorial   /* 递归调用，返回值在 a0 */

	lw      t1, 0(sp)       /* 取回原 n */
	lw      ra, 4(sp)       /* 恢复返回地址 */
	addi    sp, sp, 8       /* 回收栈帧 */

	mul     a0, a0, t1      /* a0 = (n-1)! * n */
	ret

.Lfac_base:
	li      a0, 1           /* 0! = 1! = 1 */
	ret
	.size  asm_factorial, .-asm_factorial