/*
 * encoder.c
 *
 *  Created on: Oct 12, 2026
 *      Author: ewan
 */


#include "encoder.h"
#include "main.h"

//extern timer setup
extern TIM_HandleTypeDef htim1;

#define COUNTS_PER_DETENT 4

static uint16_t previous_count = 0;
static int32_t accumulated_counts = 0;

static volatile uint8_t button_irq = 0;

static uint8_t button_pending = 0;
static uint32_t button_irq_time = 0;
static uint8_t button_held = 0;

void Encoder_Init(void)
{
    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);

    previous_count =
        (uint16_t)__HAL_TIM_GET_COUNTER(&htim1);

    accumulated_counts = 0;
    button_irq = 0;
    button_pending = 0;
    button_held = 0;
}

void Encoder_ButtonIRQ(void)
{
    button_irq = 1;
}

EncoderEvent_t Encoder_GetEvent(void)
{
    uint32_t now = HAL_GetTick();

    // Detect push-switch interrupt
    if(button_irq)
    {
        button_irq = 0;

        if(!button_held)
        {
            button_irq_time = now;
            button_pending = 1;
        }
    }

    // Confirm button has stayed pressed for 15 ms
    if(button_pending && (now - button_irq_time >= 15))
    {
        button_pending = 0;

        if(HAL_GPIO_ReadPin(ENC_SW_GPIO_Port,
                            ENC_SW_Pin) == GPIO_PIN_RESET)
        {
            button_held = 1;
            return ENCODER_PRESSED;
        }
    }

    // Re-arm once the button is released
    if(button_held &&
       HAL_GPIO_ReadPin(ENC_SW_GPIO_Port,
                       ENC_SW_Pin) == GPIO_PIN_SET)
    {
        button_held = 0;
    }

    // Read TIM1 hardware encoder counter
    uint16_t current_count =
        (uint16_t)__HAL_TIM_GET_COUNTER(&htim1);

    int16_t delta =
        (int16_t)(current_count - previous_count);

    previous_count = current_count;

    accumulated_counts += delta;

    if(accumulated_counts >= COUNTS_PER_DETENT)
    {
        accumulated_counts -= COUNTS_PER_DETENT;
        return ENCODER_CW;
    }

    if(accumulated_counts <= -COUNTS_PER_DETENT)
    {
        accumulated_counts += COUNTS_PER_DETENT;
        return ENCODER_CCW;
    }

    return ENCODER_NONE;
}
