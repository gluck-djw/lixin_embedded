#include <stdio.h>
#include <stdint.h>

// ① 全局变量
static volatile uint32_t uwTick;

#define FLASH_BASE_ADDRESS 0x8000000
#define BackAppAddress 0x0803D000     // 下载的升级包存储位置
#define ApplicationAddress 0x08010000 // 运行的APP存储位置
