/*
 * lcd.c
 *
 *  Created on: Oct 12, 2026, migrated from single file
 *      Author: ewan
 */
#include "lcd.h"
#define DRAW_BUF_LINES 60

//extern SPI bus from IOC
extern SPI_HandleTypeDef hspi1;

#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))

static uint8_t buf1[TFT_WIDTH * DRAW_BUF_LINES * BYTES_PER_PIXEL];

#define TFT_SELECT() \
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET)

#define TFT_DESELECT() \
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET)

#define TFT_DC_CMD() \
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET)

#define TFT_DC_DATA() \
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET)


/* Send command */
void TFT_WriteCommand(uint8_t cmd)
{
    TFT_SELECT();
    TFT_DC_CMD();

    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);

    TFT_DESELECT();
}

/* Send data */
void TFT_WriteData(uint8_t *data, uint16_t len)
{
    TFT_SELECT();
    TFT_DC_DATA();

    HAL_SPI_Transmit(&hspi1, data, len, HAL_MAX_DELAY);

    TFT_DESELECT();
}

/* Initialize ST7789 */
void TFT_Init(void)
{
    // Hardware reset
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);

    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(120);

    // Software reset
    TFT_WriteCommand(0x01);
    HAL_Delay(150);

    // Sleep out
    TFT_WriteCommand(0x11);
    HAL_Delay(120);

    // 16-bit RGB565 color mode
    TFT_WriteCommand(0x3A);
    uint8_t color_mode = 0x55;
    TFT_WriteData(&color_mode, 1);

    // Memory access control (RGB color order)
    TFT_WriteCommand(0x36);
    uint8_t madctl = 0x00;
    TFT_WriteData(&madctl, 1);

    // Display inversion ON
    TFT_WriteCommand(0x21);

    // Normal display mode
    TFT_WriteCommand(0x13);

    // Display ON
    TFT_WriteCommand(0x29);
    HAL_Delay(100);
}

/* Set drawing area */
void TFT_SetWindow(uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    // Column addresses
    TFT_WriteCommand(0x2A);

    data[0] = x0 >> 8;
    data[1] = x0 & 0xFF;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xFF;

    TFT_WriteData(data, 4);

    // Row addresses (account for panel offset)
    y0 += TFT_Y_OFFSET;
    y1 += TFT_Y_OFFSET;

    TFT_WriteCommand(0x2B);

    data[0] = y0 >> 8;
    data[1] = y0 & 0xFF;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xFF;

    TFT_WriteData(data, 4);

    // Memory write command
    TFT_WriteCommand(0x2C);
}

/* Fill entire display with one RGB565 color */
void TFT_FillScreen(uint16_t color)
{
    uint8_t buffer[512];

    // Prepare 256 pixels of the same color
    for (int i = 0; i < 512; i += 2)
    {
        buffer[i] = color >> 8;
        buffer[i + 1] = color & 0xFF;
    }

    TFT_SetWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);

    TFT_SELECT();
    TFT_DC_DATA();

    // Send 240 * 280 pixels in chunks
    uint32_t pixels = TFT_WIDTH * TFT_HEIGHT;

    while (pixels > 0)
    {
        uint16_t count = (pixels > 256) ? 256 : pixels;

        HAL_SPI_Transmit(&hspi1, buffer,
                         count * 2, HAL_MAX_DELAY);

        pixels -= count;
    }

    TFT_DESELECT();
}


static void my_flush_cb(lv_display_t *display,
                 const lv_area_t *area,
                 uint8_t *px_map)
{
    // Calculate number of pixels in the region
    uint32_t width = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;
    uint32_t pixels = width * height;

    // Set ST7789 drawing window
    TFT_SetWindow(area->x1, area->y1,
                  area->x2, area->y2);

    // Select display and enter data mode
    TFT_SELECT();
    TFT_DC_DATA();

    // Convert little-endian RGB565 to SPI byte order
    uint8_t buffer[512];
    uint32_t index = 0;

    while (index < pixels)
    {
        uint32_t count = pixels - index;
        if (count > 256) count = 256;

        for (uint32_t i = 0; i < count; i++)
        {
            uint32_t src = (index + i) * 2;

            buffer[2 * i] = px_map[src + 1];
            buffer[2 * i + 1] = px_map[src];
        }

        HAL_SPI_Transmit(&hspi1, buffer,
                         count * 2, HAL_MAX_DELAY);

        index += count;
    }

    TFT_DESELECT();

    // Inform LVGL the transfer is complete
    lv_display_flush_ready(display);
}

void LCD_LVGL_Init(void){
	TFT_Init();
	lv_init();
	lv_tick_set_cb(HAL_GetTick);
	lv_display_t *display1 = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
	lv_display_set_buffers(display1, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display1, my_flush_cb);
}

