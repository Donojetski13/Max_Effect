#ifndef ADC_AUDIO_H
#define ADC_AUDIO_H

/* Included once by main.c, matching this repository's header implementation
 * convention. TIM6 and DMA1 channel 3 are owned by this driver. Do not also
 * generate DMA1_Channel3_IRQHandler in stm32l4xx_it.c. */
#include "../stm32l4xx_hal.h"
#include "audio_fifo.h"
#include <string.h>

/* R4=0x004/R8=0x1c7 with a physical 24 MHz MCLK gives 46,875 Hz.
 * Change this only together with the WM8960 clock configuration. */
#define ADC_AUDIO_RATE_HZ 46875u
#define ADC_AUDIO_BLOCK 256u
#define ADC_AUDIO_PREFILL 1024u

extern ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef audio_adc_dma;
static uint16_t audio_adc_buffer[ADC_AUDIO_BLOCK * 2u];
static audio_fifo audio_adc_fifo;
static unsigned audio_adc_nominal_period;
static volatile bool audio_adc_playing;

/* Inspect in the debugger. Read them in main if UART reporting is needed;
 * never print from the sample callbacks. */
static volatile struct {
    uint32_t captured_samples;
    uint32_t dropped_samples;
    uint32_t underrun_samples;
    uint32_t adc_error;
    uint32_t sai_error;
    uint32_t fifo_level;
    uint32_t timer_period;
} audio_timing_stats;

static void adc_audio_stop(void)
{
    TIM6->CR1 &= ~TIM_CR1_CEN;
    audio_adc_playing = false;
    HAL_ADC_Stop_DMA(&hadc1);
}

static bool adc_audio_start(void)
{
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0u) timer_clock *= 2u;
    /* Current APB1 timer clock is 24 MHz: 512 ticks per sample. */
    if (timer_clock % ADC_AUDIO_RATE_HZ != 0u) return false;
    audio_adc_nominal_period = timer_clock / ADC_AUDIO_RATE_HZ;
    if (audio_adc_nominal_period < 16u || audio_adc_nominal_period > 65527u)
        return false;

    atomic_store(&audio_adc_fifo.wr, 0u);
    atomic_store(&audio_adc_fifo.rd, 0u);
    audio_adc_playing = false;
    memset((void *)&audio_timing_stats, 0, sizeof audio_timing_stats);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef pin = {0};
    pin.Pin = GPIO_PIN_0; /* ADC1_IN1 */
    pin.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    pin.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &pin);

    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    audio_adc_dma.Instance = DMA1_Channel3;
    audio_adc_dma.Init.Request = DMA_REQUEST_ADC1;
    audio_adc_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    audio_adc_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    audio_adc_dma.Init.MemInc = DMA_MINC_ENABLE;
    audio_adc_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    audio_adc_dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    audio_adc_dma.Init.Mode = DMA_CIRCULAR;
    audio_adc_dma.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&audio_adc_dma) != HAL_OK) return false;
    __HAL_LINKDMA(&hadc1, DMA_Handle, audio_adc_dma);
    /* Same preemption priority as SAI DMA, bounded half-block callbacks. */
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    /* Override the generated software-trigger settings without requiring a
     * regenerated Cube project. Keep continuous conversion OFF: one TRGO
     * starts exactly one conversion, with DMA requests continuing in a ring. */
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) return false;

    __HAL_RCC_TIM6_CLK_ENABLE();
    TIM6->CR1 = TIM_CR1_ARPE;
    TIM6->PSC = 0;
    TIM6->ARR = audio_adc_nominal_period - 1u;
    TIM6->CR2 = TIM_CR2_MMS_1; /* Update event -> TRGO -> ADC1. */
    TIM6->DIER = 0;
    TIM6->EGR = TIM_EGR_UG; /* Load PSC/ARR before arming ADC. */
    TIM6->SR = 0;
    TIM6->CNT = 0;

    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
        return false;
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)audio_adc_buffer,
                          ADC_AUDIO_BLOCK * 2u) != HAL_OK) return false;
    TIM6->CR1 |= TIM_CR1_CEN;

    /* Only startup waits. Streaming proceeds entirely in DMA callbacks. */
    uint32_t start = HAL_GetTick();
    while (audio_fifo_level(&audio_adc_fifo) < ADC_AUDIO_PREFILL) {
        if (audio_timing_stats.adc_error || HAL_GetTick() - start > 100u) {
            adc_audio_stop();
            return false;
        }
    }
    return true;
}

static void adc_audio_block(const uint16_t *src)
{
    audio_timing_stats.captured_samples += ADC_AUDIO_BLOCK;
    if (!audio_fifo_push_adc(&audio_adc_fifo, src, ADC_AUDIO_BLOCK))
        audio_timing_stats.dropped_samples += ADC_AUDIO_BLOCK;
    unsigned level = audio_fifo_level(&audio_adc_fifo);
    audio_timing_stats.fifo_level = level;
    unsigned period = audio_adc_playing ?
        audio_capture_period(audio_adc_nominal_period, level) :
        audio_adc_nominal_period;
    /* ARR preload applies at the next timer update, never mid-period. */
    TIM6->ARR = period - 1u;
    audio_timing_stats.timer_period = period;
}

void DMA1_Channel3_IRQHandler(void) { HAL_DMA_IRQHandler(&audio_adc_dma); }
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *h)
{
    if (h == &hadc1) adc_audio_block(audio_adc_buffer);
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *h)
{
    if (h == &hadc1) adc_audio_block(&audio_adc_buffer[ADC_AUDIO_BLOCK]);
}
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *h)
{
    if (h == &hadc1) {
        TIM6->CR1 &= ~TIM_CR1_CEN;
        audio_timing_stats.adc_error = HAL_ADC_GetError(h);
    }
}

static int16_t adc_audio_sample(void)
{
    int16_t sample;
    if (!audio_fifo_pop(&audio_adc_fifo, &sample)) {
        ++audio_timing_stats.underrun_samples;
        return 0;
    }
    return sample;
}
#endif
