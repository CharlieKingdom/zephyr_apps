# native_sim 外设学习工程（Zephyr + native_sim）
# 无需真实开发板：程序直接在宿主机（PC）上编译并运行。
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(native_sim_learn)

target_sources(app PRIVATE
  src/main.c
  src/app_led.c
  src/app_button.c
  src/app_eeprom.c
  src/app_entropy.c
)