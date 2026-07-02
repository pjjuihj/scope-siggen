#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx_hal.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c1;

#ifndef u8
#define u8 uint8_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif
// OLED I2C配置 - 使用 I2C1 (PB6/PB7)
#define OLED_I2C_HANDLE hi2c1
// SSD1306 I2C地址
// 7位地址: 0x3C (I2C扫描器显示)
// 8位地址: 0x78 (HAL库使用)
#define OLED_ADDR 0x78

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// OLED控制命令
#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

// OLED控制用函数
void OLED_WR_Byte(u8 dat, u8 mode);
void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x, u8 y, u8 t);
void OLED_FillRect(u8 x, u8 y, u8 width, u8 height, u8 color);
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode);
void OLED_DrawCircle(u8 x, u8 y, u8 r);
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode);
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1, u8 mode);
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1, u8 mode);
void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1, u8 mode);
void OLED_ScrollDisplay(u8 num, u8 space, u8 mode);
void OLED_ShowPicture(u8 x, u8 y, u8 sizex, u8 sizey, u8 BMP[], u8 mode);
void OLED_Init(void);

#endif


