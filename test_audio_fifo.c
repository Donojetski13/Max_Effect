#include "UI_headers/audio_fifo.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

static void test_fifo(void)
{
    audio_fifo q = {0};
    uint16_t raw[AUDIO_FIFO_SIZE];
    int16_t sample;
    for (unsigned i=0; i<AUDIO_FIFO_SIZE; ++i) raw[i] = i;
    assert(!audio_fifo_pop(&q, &sample));
    assert(audio_fifo_push_adc(&q, raw, AUDIO_FIFO_SIZE));
    assert(!audio_fifo_push_adc(&q, raw, 1));
    assert(audio_fifo_level(&q) == AUDIO_FIFO_SIZE);
    for (unsigned i=0; i<AUDIO_FIFO_SIZE; ++i) {
        assert(audio_fifo_pop(&q, &sample));
        assert(sample == ((int)i-2048)*16);
    }
    assert(!audio_fifo_pop(&q, &sample));
    /* Counter wrap and physical ring wrap must preserve order. */
    atomic_store(&q.wr, UINT_MAX-100u);
    atomic_store(&q.rd, UINT_MAX-100u);
    assert(audio_fifo_push_adc(&q, raw, 256));
    for (unsigned i=0; i<256; ++i) {
        assert(audio_fifo_pop(&q, &sample));
        assert(sample == ((int)i-2048)*16);
    }
    /* Reject a whole block without publishing a partial write. */
    assert(audio_fifo_push_adc(&q, raw, AUDIO_FIFO_SIZE-100));
    unsigned wr = atomic_load(&q.wr);
    assert(!audio_fifo_push_adc(&q, raw, 256));
    assert(atomic_load(&q.wr) == wr);
}

/* Event simulation of ADC half-completion and SAI half-consumption using
 * the real queue and timer controller. No block may disappear or become
 * silence for reasonable independent oscillator errors. */
static void simulate(double codec_ppm)
{
    audio_fifo q = {0};
    uint16_t raw[1024];
    for (unsigned i=0;i<1024;++i) raw[i]=2200;
    assert(audio_fifo_push_adc(&q, raw, 1024));
    int16_t s;
    for (unsigned i=0;i<512;++i) assert(audio_fifo_pop(&q,&s));
    double now=0, capture=256.0/46875.0;
    double period_out=256.0/(46875.0*(1.0+codec_ppm/1e6));
    double playback=period_out;
    unsigned min_level=4096,max_level=0;
    while(now<120.0) {
        if(capture<=playback) {
            now=capture;
            assert(audio_fifo_push_adc(&q,raw,256));
            unsigned level=audio_fifo_level(&q);
            if(level>max_level)max_level=level;
            unsigned ticks=audio_capture_period(512,level);
            capture+=256.0*ticks/24000000.0;
        } else {
            now=playback;
            for(unsigned i=0;i<256;++i) {
                assert(audio_fifo_pop(&q,&s));
                assert(s==(2200-2048)*16);
            }
            unsigned level=audio_fifo_level(&q);
            if(level<min_level)min_level=level;
            playback+=period_out;
        }
    }
    printf("120 s, DAC %+.0f ppm: no drops/underruns, FIFO %u..%u\n",
           codec_ppm,min_level,max_level);
}

int main(void)
{
    test_fifo();
    assert(audio_capture_period(512,0)==509);
    assert(audio_capture_period(512,4096)==520);
    assert(audio_capture_period(512,768)==512);
    simulate(0); simulate(100); simulate(-100);
    simulate(1000); simulate(-1000);
    puts("FIFO order, conversion, full/empty, counter wrap and drift checks passed");
}
