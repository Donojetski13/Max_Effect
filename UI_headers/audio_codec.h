/*
audio_codec.h


High-level interface for the audio codec.
The rest of the system should use these functions instead
of trying to directly access codec registers
*/


#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

#include "../FFT-implementation-in-C-reduced/include/fft_common.h"
#include "../FFT-implementation-in-C-reduced/include/fft_algorithms.h"
#include <stdint.h>
#include <stdbool.h>
#include "../stm32l4xx_hal.h"
#include "filters.h"
#include "wm8960.h"

#define WM8960
//#define TLV320AIC3204
//#define _BYPASS


extern I2C_HandleTypeDef hi2c2;
HAL_StatusTypeDef ret;

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

#ifdef WM8960
/*Initialize codec and configure ADC/DAC, sample rate, interface using I2c*/
/*Returns TRUE if successful*/
bool codec_init_WM8960() {
    printf("\nInitializing codec...\n");
    uint8_t data_buf[2];
    uint8_t dBs = 0; // decebels to add on top of defualt
    bool mute_on = true;
    bool speakers = true;

    // reset old values
	data_buf[0] = (WM8960_RESET << 1) + 0; // reset values bit 8 doesn't matter
	data_buf[1] = 0x00; // bits7-0 values don't matter
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;
    printf("\nInput Path Settings...\n");
    // Initialize the WM8960 codec
    // Enable mics, ADC, & other power settings
    data_buf[0] = (WM8960_POWER1 << 1) + 1; // reg address + bit 8(of Vmid fast start-up)
    data_buf[1] = 0xD6; // Vmid fast start-up, Vref, Input PGA right, ADC right, mic bias, & M clk enabled
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    // delay 3 ms
    HAL_Delay(10);

    // Enable mics, ADC, & other power settings
	data_buf[0] = (WM8960_POWER1 << 1) + 0; // reg address + bit 8(of Vmid play back)
	data_buf[1] = 0xD6; // Vmid play back, Vref, Input PGA right, ADC right, mic bias, & M clk enabled
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;

    data_buf[0] = (WM8960_POWER3 << 1) + 0; // reg address + bit 8
    data_buf[1] = 0x1C; // RMIC, LOMIX, ROMIX
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    // set MCLK for SYSCLK data
	data_buf[0] = (WM8960_CLOCK1 << 1) + 0; // reg address + bit 8
	data_buf[1] = WM8960_SYSCLK_DIV_2; // divide Mclk by 2 (MCLK is 24MHz)
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;


    // set I2S data size package
	data_buf[0] = (WM8960_IFACE2 << 1) + 0; // reg address + bit 8
	data_buf[1] = 0x40; // ALRCGPIO = 1 for ADC clk 					+ (ADC DAC loop off)
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;

    // set I2S data size package
	data_buf[0] = (WM8960_IFACE1 << 1) + 0; // reg address + bit 8(of Vmid play back)
	data_buf[1] = 0x42; // master mode (bit 6) normal polarity, 16 bit word length, 12S format
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;

	// set master clock settings
	data_buf[0] = (WM8960_CLOCK2 << 1) + 1; // reg address + bit 8
	data_buf[1] = 0xC7; // DCLKDIV stays 111, BCLKDIV=0111 -> BCLK = 12 MHz / 8 = 1.5 MHz = 32 x 46.875 kHz
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;

    // set mic read to single-ended
    data_buf[0] = (WM8960_RINPATH << 1) + 1; // reg address + bit 8(connect RIN1 to -PGA)
    data_buf[1] = 0x08; // connect  RMIC2B only
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    // Input volume(left and right)
    data_buf[0] = (WM8960_RINVOL << 1) + 1; // reg address + bit 8(update input volume)
    data_buf[1] = (mute_on ? 0x80 : 0x00) + clampi(WM8960_INVOL + (int)lroundf(dBs / 0.75f), 0, 0x3F);  // 0.75 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_ADDCTL1 << 1) + 1; // reg address + bit 8(thermal shutdown enable)
    data_buf[1] = 0xDB; // bias current for 3.3V, mono, input both right DAC output, faster slow clk select, enable slow clk
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_RADC << 1) + 1; // reg address + bit 8(update ADC volume/gain)
    data_buf[1] = clampi(WM8960_ADCVOL + dBs * 2, 0, 0xFF);                   // 0.5 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    // noise gate settings
//    data_buf[0] = (WM8960_ALC1 << 1) + 0; // reg address + bit 8
//	data_buf[1] = 0x8B; // ALC for Right channel + default
//	/* send data via HAL I2C at addr (in 8bit format)*/
//	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
//	if (ret != HAL_OK) return false;
//    data_buf[0] = (WM8960_NOISEG << 1) + 0; // reg address + bit 8
//    data_buf[1] = ((WM8960_NGATETHRESHOLD + dB2NGATEVAL(NG_dBs)) << 3) + 1; // Threshold(-76.5dB + dBs), noise gate enabled
//    /* send data via HAL I2C at addr (in 8bit format)*/
//    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
//    if (ret != HAL_OK) return false;

    printf("Output Path Settings...\n"); // speakers off
    // Output path settings
    // DAC enable and volume
    data_buf[0] = (WM8960_POWER2 << 1) + 1; // reg address + bit 8(left DAC on) //********//
    data_buf[1] = speakers? 0x98: 0xE0; // right DAC on, both right & left output buffers on,(SPKR and SPKL on)(Out3 & PLL off)
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_RDAC << 1) + 1; // reg address + bit 8(update DAC volume/gain)
    data_buf[1] = clampi(WM8960_DACVOL - dBs * 2, 0, 0xFF);               // 0.5 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    // DAC softmute & sample rate settings
    data_buf[0] = (WM8960_DACCTL1 << 1) + 0; // reg address + bit 8
    data_buf[1] = 0x00; // no 6dB attenuation, no pol invertion, softmute off, no de-emphasis, HP filter on
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_DACCTL2 << 1) + 0; // reg address + bit 8
    data_buf[1] = 0x08; // no pol invertion, softmute on, fast softmute ramp rate, normal filter
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    // Output mix & volume(left and right)
    data_buf[0] = (WM8960_LOUTMIX << 1) + 1; // reg address + bit 8(use DAC for right output mix)
    data_buf[1] = 0x00; // no LIN3 or its dBs in left output mix
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_ROUTMIX << 1) + 1; // reg address + bit 8(use DAC for right output mix)
    data_buf[1] = 0x00; // no RIN3 or its dBs in right output mix
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    data_buf[0] = (WM8960_LOUT1 << 1) + 0; // reg address + bit 8(wait for right to update)
	data_buf[1] = clampi(WM8960_OUTVOL - dBs, 0, 0x7F);               // 1 dB steps
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;
	data_buf[0] = (WM8960_LOUT2 << 1) + 0; // reg address + bit 8(wait for right to update)
	data_buf[1] = clampi(WM8960_OUTVOL - dBs, 0, 0x7F);               // 1 dB steps
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;

    data_buf[0] = (WM8960_ROUT1 << 1) + 1; // reg address + bit 8(update output volume)
    data_buf[1] = clampi(WM8960_OUTVOL - dBs, 0, 0x7F);               // 1 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_ROUT2 << 1) + 1; // reg address + bit 8(update output volume)
    data_buf[1] = clampi(WM8960_OUTVOL - dBs, 0, 0x7F);               // 1 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;

    // headphones and speaker output enable
    data_buf[0] = (WM8960_CLASSD1 << 1) + 0; // reg address + bit 8
    data_buf[1] = speakers? 0xF7: 0x37; // both speakers off        //********//
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_ADDCTL2 << 1) + 0; // reg address + bit 8
    data_buf[1] = 0x00; // jack detect on: headphone and speaker both active default rest (pol, tristates, & ADCLRC)
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
#ifdef _BYPASS
    // Bypass output test
    data_buf[0] = (WM8960_BYPASS2 << 1) + 0; // reg address + bit 8
	data_buf[1] = 0x80 + 0x00; // Right bypass with 0 + (00h - 70h, 3dBs steps)dBs attenuation
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;

#endif
    printf("\nInitializing done\n");
    return true;
}
#endif
#ifdef TLV320AIC3204
/*Initialize codec and configure ADC/DAC, sample rate, interface using I2c*/
/*Returns TRUE if successful*/
bool codec_init_TLV320AIC3204() {
    printf("\nInitializing codec...\n");
    uint8_t data_buf[2];
    uint8_t dBs = 0; // decebels to add on top of defualt
    // Initialize the TLV320AIC3204 codec
    // Enable mics

    // set mic read to diferential

    // Input volume(left and right)

    printf("\nInitializing done\n");
    return true;
}
#endif

// /*stop audio i/o through codec (mute)*/
bool codec_mute(bool mute_on) {
	uint8_t data_buf[2];
	data_buf[0] = (WM8960_DACCTL1 << 1) + 0; // reg address + bit 8
	if (mute_on) {
		data_buf[1] = 0x08; // no 6dB attenuation, no pol invertion, softmute off, no de-emphasis, HP filter on
		/* send data via HAL I2C at addr (in 8bit format)*/
		ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
		if (ret != HAL_OK) return false;
		printf("Mute\n");
    } else{
		data_buf[1] = 0x00; // no 6dB attenuation, no pol invertion, softmute off, no de-emphasis, HP filter on
		/* send data via HAL I2C at addr (in 8bit format)*/
		ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
		if (ret != HAL_OK) return false;
		printf("Un-Mute\n");
    }

    return true;
}

/*Set analog input gain */
bool codec_set_intput_gain(int gain) {
    uint8_t data_buf[2];
    data_buf[0] = (WM8960_RINVOL << 1) + 1; // reg address + bit 8(update input volume)
    data_buf[1] = 0x40 + clampi(WM8960_INVOL + (int)lroundf(gain / 0.75f), 0, 0x3F);  // 0.75 dB steps + zero cross
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
//    printf("%d dBs (input)\n", gain);
    return true;
}

/*Set input ADC gain */
bool codec_set_ADC_gain(int gain) {
    uint8_t data_buf[2];
    data_buf[0] = (WM8960_RADC << 1) + 1; // reg address + bit 8(update ADC volume/gain)
    data_buf[1] = clampi(WM8960_ADCVOL + gain * 2, 0, 0xFF);                   // 0.5 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
//    printf("%d dBs (ADC)\n", gain);
    return true;
}
/*Set  output DAC gain */
bool codec_set_DAC_gain(int gain) {
    uint8_t data_buf[2];
    data_buf[0] = (WM8960_RDAC << 1) + 1; // reg address + bit 8(update DAC volume/gain)
    data_buf[1] = clampi(WM8960_DACVOL - gain * 2, 0, 0xFF);               // 0.5 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
//    printf("%d dBs (DAC)\n", gain);
    return true;
}

/*Set analog output  gain */
bool codec_set_output_gain(int gain) {
    uint8_t data_buf[2];
    data_buf[0] = (WM8960_LOUT1 << 1) + 0; // reg address + bit 8(wait for right to update))
	data_buf[1] = 0x80 + clampi(WM8960_OUTVOL - gain, 0, 0x7F);         // gain change on zero cross 1 dB steps
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;
	data_buf[0] = (WM8960_LOUT2 << 1) + 0; // reg address + bit 8(wait for right to update))
	data_buf[1] = 0x80 + clampi(WM8960_OUTVOL - gain, 0, 0x7F);         // gain change on zero cross 1 dB steps
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;

    data_buf[0] = (WM8960_ROUT1 << 1) + 1; // reg address + bit 8(update right output volume)
	data_buf[1] = 0x80 + clampi(WM8960_OUTVOL - gain, 0, 0x7F);         // gain change on zero cross 1 dB steps
	/* send data via HAL I2C at addr (in 8bit format)*/
	ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK) return false;
    data_buf[0] = (WM8960_ROUT2 << 1) + 1; // reg address + bit 8(update right output volume)
    data_buf[1] = 0x80 + clampi(WM8960_OUTVOL - gain, 0, 0x7F);         // gain change on zero cross 1 dB steps
    /* send data via HAL I2C at addr (in 8bit format)*/
    ret = HAL_I2C_Master_Transmit(&hi2c2, (WM8960_I2C_ADDR << 1), data_buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return false;
//    printf("%d dBs (output)\n", (gain));
    return true;
}
/*************************** I2S Stuff ********************/

#define FRAMES   256                       // frames per half buffer
#define RING_SIZE 4096                       // power of two
#define RING_MASK (RING_SIZE - 1)
//#define SIGNAL_SCALE 32768.0 			// for 16 bits

extern SAI_HandleTypeDef hsai_BlockA1, hsai_BlockB1;

static int16_t tx_buf[FRAMES * 2 * 2];     // 2 halves x 2 slots (L,R)
static int16_t rx_buf[FRAMES * 2 * 2];     //
static int16_t tx_ring[RING_SIZE], rx_ring[RING_SIZE];
static volatile uint32_t tx_wr, tx_rd, rx_wr, rx_rd;

static inline int16_t sat16(double v) {
    return (v > 32767.0) ? 32767 : (v < -32768.0) ? -32768 : (int16_t)v;
}

static void tx_fill(int16_t *dst);

bool audio_start(void) {
    memset(tx_buf, 0, sizeof tx_buf);
    memset(rx_buf, 0, sizeof rx_buf);
#ifdef MCU_ADC_DMA
    /* Prefill both DMA halves from captured data before starting playback.
     * Codec RX is unnecessary when the source is the STM32 ADC. */
    tx_fill(&tx_buf[0]);
    tx_fill(&tx_buf[FRAMES * 2]);
    audio_adc_playing = true;
#else
    if (HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t*)rx_buf, FRAMES * 2 * 2) != HAL_OK) return false;
#endif
    if (HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t*)tx_buf, FRAMES * 2 * 2) != HAL_OK) {
#ifdef MCU_ADC_DMA
        audio_adc_playing = false;
#endif
        return false;
    }
    return true;
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *h) {
#ifdef MCU_ADC_DMA
    /* No blocking UART in an IRQ. Main stops capture on the next wakeup. */
    audio_timing_stats.sai_error = HAL_SAI_GetError(h);
#else
    (void)h;
#endif
}

// TX: ring -> DMA buffer
static void tx_fill(int16_t *dst) {
    for (int i = 0; i < FRAMES; i++) {
#ifdef MCU_ADC_DMA
        int16_t s = adc_audio_sample();
#else
        int16_t s = (tx_rd != tx_wr) ? tx_ring[tx_rd++ & RING_MASK] : 0;
#endif
		dst[2*i] = dst[2*i + 1] = s;                      // mono -> L and R
    }
}
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *h) { if (h == &hsai_BlockA1) tx_fill(&tx_buf[0]); }
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *h)     { if (h == &hsai_BlockA1) tx_fill(&tx_buf[FRAMES * 2]); }

// RX: DMA buffer -> ring (one slot per frame)
static void rx_drain(const int16_t *src) {
    for (int i = 0; i < FRAMES; i++) {
        if ((uint32_t)(rx_wr - rx_rd) < RING_SIZE)                           // drop if consumer is behind
            rx_ring[rx_wr++ & RING_MASK] = src[2*i];
    }
}
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *h) { if (h == &hsai_BlockB1) rx_drain(&rx_buf[0]); }
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *h)     { if (h == &hsai_BlockB1) rx_drain(&rx_buf[FRAMES * 2]); }

// returns true if n samples were read (waits up to 10 ms)
bool input_from_codec(int16_t *signal, int n) {
    uint32_t t0 = HAL_GetTick();
    for (int i = 0; i < n; i++) {
        while (rx_rd == rx_wr) if (HAL_GetTick() - t0 > 10) return false;
        signal[i] = rx_ring[rx_rd++ & RING_MASK];
    }
    return true;
}

// returns true if n samples were read (waits up to 10 ms)
bool tune_from_codec(complex_t *signal, int n) {
    uint32_t t0 = HAL_GetTick();
    for (int i = 0; i < n; i++) {
        while (rx_rd == rx_wr) if (HAL_GetTick() - t0 > 10) return false;
        signal[i] = rx_ring[rx_rd++ & RING_MASK];
    }
    return true;
}

void _2complex(int16_t *rSig, complex_t * cSig, int n) {
    for (int i = 0; i < n; i++) {
    	cSig[i] = rSig[i];
    }
}


void _2real16(int16_t *rSig, complex_t * cSig, int n) {
    for (int i = 0; i < n; i++) {
    	rSig[i] = sat16(creal(cSig[i]));
    }

}

bool output2codec(int16_t *signal, int n) {
    for (int i = 0; i < n; i++) {
        if ((uint32_t)(tx_wr - tx_rd) >= RING_SIZE) return false;            // full: drop
        tx_ring[tx_wr++ & RING_MASK] = signal[i];
    }
    return true;
}

bool tune2codec(complex_t *signal, int n) {
    for (int i = 0; i < n; i++) {
        if ((uint32_t)(tx_wr - tx_rd) >= RING_SIZE) return false;            // full: drop
        tx_ring[tx_wr++ & RING_MASK] = sat16(creal(signal[i]));
    }
    return true;
}

/*********************************** MCU stuff ****************/
extern ADC_HandleTypeDef hadc1;

// Funtion to get input signal from ADC or other source (for testing)
void get_input_MCU_signal(int16_t* signal, int n) {
    // Read ADC input or generate test signal
    for (int i = 0; i < n; i++) {
         HAL_ADC_Start(&hadc1);
         HAL_ADC_PollForConversion(&hadc1, 1);
         signal[i] = (int16_t)(((int32_t)HAL_ADC_GetValue(&hadc1) - 2048) * 16); // Convert 12 unsigned to signed 16
    }
}
#endif
