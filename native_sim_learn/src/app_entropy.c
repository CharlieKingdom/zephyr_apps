/*
 * [4] 熵源 / 随机数
 *
 * native_sim 提供一个基于宿主机随机数的熵源设备（zephyr,native-posix-rng）。
 * 默认使用固定种子，加 --seed=<n> 可复现同一序列。
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/sys/printk.h>

#include "app_entropy.h"

void demo_entropy(void)
{
	const struct device *rng = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
	uint8_t buf[16];
	int rc;

	printk("\n=== [4] 熵源 / 随机数 (rng) ===\n");

	if (!device_is_ready(rng)) {
		printk("  错误：随机数设备未就绪\n");
		return;
	}

	rc = entropy_get_entropy(rng, buf, sizeof(buf));
	if (rc < 0) {
		printk("  读取失败：%d\n", rc);
		return;
	}

	printk("  取到 %zu 字节随机数：", sizeof(buf));
	for (size_t i = 0; i < sizeof(buf); i++) {
		printk("%02x ", buf[i]);
	}
	printk("\n  （native_sim 默认使用固定种子，加 --seed=<n> 可复现同一序列）\n");
}