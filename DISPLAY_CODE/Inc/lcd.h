/*
 * lcd.h
 *
 *  Created on: Oct 12, 2026
 *      Author: ewan
 */

#ifndef LCD_H
#define LCD_H

#include "main.h"
#include "lvgl.h"

#define TFT_WIDTH 240
#define TFT_HEIGHT 280
#define TFT_Y_OFFSET 20

void TFT_Init(void);
void TFT_WriteCommand(uint8_t cmd);
void TFT_WriteData(uint8_t *data, uint16_t len);
void TFT_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void TFT_FillScreen(uint16_t color);

void LCD_LVGL_Init(void);



#endif /* INC_LCD_H_ */
