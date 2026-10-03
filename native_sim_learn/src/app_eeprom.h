/*
 * [3] EEPROM：非易失存储
 */
#ifndef APP_EEPROM_H
#define APP_EEPROM_H

/* 读取 eeprom-0 的启动计数并累加写回（数据落盘到 eeprom.bin） */
void demo_eeprom(void);

#endif /* APP_EEPROM_H */