Code for display. ui.h is main menu, ui_effects.h is effects page, ui_tuner.h for the tuner menu
Drop the lvgl-v9.6.0-lite folder into src/. It can be found here: https://drive.google.com/drive/folders/1Fcxl3y1AszxCB3g-uuLggd1KzUmn0cB4?usp=sharing

## Display and Rotary Encoder Pinout

**Microcontroller:** STM32L4R5ZI (Nucleo-L4R5ZI-P)

### ST7789 TFT Display (SPI1)

The 1.69" 240×280 TFT display uses SPI1 for communication with the STM32. LVGL handles the graphical user interface, while the ST7789 driver sends commands and pixel data over SPI.

| STM32 Pin | Display Signal | Purpose |
|---|---|---|
| PA5 | SCK / SCL | SPI1 clock |
| PA7 | MOSI / SDA | SPI1 data transmission |
| PD14 | CS | Chip select (active low) |
| PD15 | DC | Data/command selection |
| PF12 | RST | Display hardware reset |

**Additional notes:**
- SPI1 operates in master mode.
- Display data is transmitted using `HAL_SPI_Transmit()`.
- Chip select is controlled manually through PD14.
- The DC pin distinguishes commands from pixel data.
- Display reset is controlled through PF12.
- PA4 (SPI1_NSS) and PA6 (SPI1_MISO) are configured but are not used by the current LCD driver.
- Display power, ground, and backlight connections should be verified against the physical wiring.

### Rotary Encoder with Pushbutton

The rotary encoder uses TIM1 in hardware quadrature encoder mode for detecting rotation. Its integrated pushbutton uses a GPIO external interrupt.

| STM32 Pin | Encoder Signal | Purpose |
|---|---|---|
| PE9 | A / CLK | TIM1 Channel 1 (quadrature input A) |
| PE11 | B / DT | TIM1 Channel 2 (quadrature input B) |
| PF13 | SW | Encoder pushbutton (EXTI13) |
| GND | GND | Common ground reference |

**Encoder configuration:**
- Timer: TIM1
- Encoder mode: `TIM_ENCODERMODE_TI12`
- Resolution: 4 counts per detent
- Pushbutton interrupt: Falling-edge EXTI
- Interrupt handler: `EXTI15_10_IRQn`
- Internal pull-up enabled for the pushbutton
- Software debounce: 15 ms

**Encoder functionality:**
- Clockwise rotation: Navigate down / increase parameter value
- Counterclockwise rotation: Navigate up / decrease parameter value
- Pushbutton press: Select menu item / enter or exit parameter editing

### Pin Summary

| Component | STM32 Pins |
|---|---|
| TFT Display | PA5, PA7, PD14, PD15, PF12 |
| Rotary Encoder | PE9, PE11, PF13 |
| **Total** | **8 GPIO signal pins** |

### Relevant Firmware Files

- `Core/Inc/main.h` — GPIO pin definitions
- `Core/Src/main.c` — SPI1, TIM1, and GPIO initialization
- `Core/Src/stm32l4xx_hal_msp.c` — Peripheral pin configuration
- `Core/Src/lcd.c` — ST7789 display driver and LVGL interface
- `Core/Src/encoder.c` — Rotary encoder and button handling
- `Core/Src/ui.c` — Main UI navigation
- `Core/Src/ui_effects.c` — Effect screens and parameter controls

**Note:** Pin identifiers refer to STM32 GPIO pins, not physical Nucleo connector pin numbers.
