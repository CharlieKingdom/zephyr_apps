/*
 * [2] GPIO 输入 + 中断：按键
 *
 * sw0 由 boards/native_sim.overlay 定义（上拉、低电平有效）。
 * 用另一个线程向 gpio0 注入高低电平来模拟“按下/松开”，
 * 中断处理函数被触发后通过信号量通知主流程。
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "app_button.h"

/* sw0 = gpio0 pin2，由 boards/native_sim.overlay 定义（上拉、低电平有效） */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

#define BUTTON_PRESSES 3

static struct gpio_callback button_cb;
static struct k_sem button_pressed_sem;

static void button_isr(const struct device *port, struct gpio_callback *cb,
		       uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	printk("  [中断] 按键按下！t = %u ms\n", k_uptime_get_32());
	k_sem_give(&button_pressed_sem);
}

/* 用“另一个线程”模拟真实世界的按键动作：向 gpio0 pin2 注入高低电平 */
static void button_sim_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int i = 1; i <= BUTTON_PRESSES; i++) {
		k_msleep(400);

		printk("  [仿真] 第 %d 次按下  (注入低电平)\n", i);
		gpio_emul_input_set(button.port, button.pin, 0);
		k_msleep(150);

		printk("  [仿真] 第 %d 次松开  (注入高电平)\n", i);
		gpio_emul_input_set(button.port, button.pin, 1);
	}
}

static K_THREAD_STACK_DEFINE(button_sim_stack, 1024);
static struct k_thread button_sim_tcb;

void demo_button(void)
{
	printk("\n=== [2] GPIO 输入 + 中断：按键 (sw0 / gpio0 pin%d) ===\n",
	       button.pin);

	if (!gpio_is_ready_dt(&button)) {
		printk("  错误：按键设备未就绪\n");
		return;
	}

	if (gpio_pin_configure_dt(&button, GPIO_INPUT) < 0) {
		printk("  错误：按键配置失败\n");
		return;
	}

	/* 低电平有效：等到“有效电平”（按下）时触发边沿中断 */
	if (gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE) < 0) {
		printk("  错误：中断配置失败\n");
		return;
	}

	gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);

	k_sem_init(&button_pressed_sem, 0, BUTTON_PRESSES);

	/* 启动“仿真按键”线程，然后阻塞等待中断被触发 */
	k_thread_create(&button_sim_tcb, button_sim_stack,
			K_THREAD_STACK_SIZEOF(button_sim_stack),
			button_sim_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	printk("  等待 %d 次按键中断...\n", BUTTON_PRESSES);
	for (int i = 0; i < BUTTON_PRESSES; i++) {
		k_sem_take(&button_pressed_sem, K_FOREVER);
	}

	k_thread_join(&button_sim_tcb, K_FOREVER);
	gpio_remove_callback(button.port, &button_cb);
	printk("  共捕获 %d 次按键中断，GPIO 输入通路验证通过\n", BUTTON_PRESSES);
}