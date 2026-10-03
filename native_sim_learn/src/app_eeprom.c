/*
 * [3] EEPROM：非易失存储
 *
 * eeprom-0 = eeprom0（zephyr,sim-eeprom），数据落盘到宿主机的 eeprom.bin，
 * 因此多次运行 zephyr.exe 时启动计数会持续累加。
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/sys/printk.h>

#include "app_eeprom.h"

#define EEPROM_MAGIC 0xEE9703U

struct eeprom_record {
	uint32_t magic;
	uint32_t boot_count;
};

void demo_eeprom(void)
{
	/* eeprom-0 = eeprom0（zephyr,sim-eeprom），数据落盘到 eeprom.bin */
	const struct device *eeprom = DEVICE_DT_GET(DT_ALIAS(eeprom_0));
	struct eeprom_record rec;
	int rc;

	printk("\n=== [3] EEPROM：非易失存储 (eeprom-0) ===\n");

	if (!device_is_ready(eeprom)) {
		printk("  错误：EEPROM 设备未就绪\n");
		return;
	}

	printk("  EEPROM 容量：%zu 字节\n", eeprom_get_size(eeprom));

	rc = eeprom_read(eeprom, 0, &rec, sizeof(rec));
	if (rc < 0) {
		printk("  读取失败（%d），按首次上电处理\n", rc);
		rec.magic = 0;
	}

	if (rec.magic != EEPROM_MAGIC) {
		rec.magic = EEPROM_MAGIC;
		rec.boot_count = 0;
	}
	rec.boot_count++;

	printk("  启动次数 = %u（多次运行 zephyr.exe，该值会持续累加）\n",
	       rec.boot_count);

	rc = eeprom_write(eeprom, 0, &rec, sizeof(rec));
	if (rc < 0) {
		printk("  写入失败：%d\n", rc);
	}
}