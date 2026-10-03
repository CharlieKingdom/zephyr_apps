/*
 * [2] GPIO 输入 + 中断：按键
 */
#ifndef APP_BUTTON_H
#define APP_BUTTON_H

/* 用后台线程模拟按键动作，等待并统计 sw0 的边沿中断 */
void demo_button(void);

#endif /* APP_BUTTON_H */