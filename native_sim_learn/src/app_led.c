/*
 * [1] GPIO 输出：LED
 *
 * 翻转 led0，并通过 native_sim 的 GPIO 仿真器读回引脚电平，
 * 验证输出通路是否正确。
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/sys/printk.h>

#include "app_led.h"

/* led0 = gpio0 pin0，来自板级设备树 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

void demo_led(void)
{
	printk("\n=== [1] GPIO 输出：LED (led0 / gpio0 pin%d) ===\n", led.pin);

	if (!gpio_is_ready_dt(&led)) {
		printk("  错误：LED 设备未就绪\n");
		return;
	}

	/* 配置为推挽输出，初始为灭 */
	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) < 0) {
		printk("  错误：LED 配置失败\n");
		return;
	}

	for (int i = 1; i <= 4; i++) {
		gpio_pin_toggle_dt(&led);

		/*
		 * gpio_emul_output_get() 读取的是“仿真器内部记录的输出电平”，
		 * 相当于用万用表去量一根还没接灯珠的引脚——便于验证代码是否正确。
		 */
		int level = gpio_emul_output_get(led.port, led.pin);

		printk("  第 %d 次翻转 -> led0 = %s\n", i,
		       (level > 0) ? "亮(高电平)" : "灭(低电平)");
		k_msleep(300);
	}
}