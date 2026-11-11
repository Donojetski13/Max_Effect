#ifndef AUDIO_FIFO_H
#define AUDIO_FIFO_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#define AUDIO_FIFO_SIZE 4096u
#define AUDIO_FIFO_MASK (AUDIO_FIFO_SIZE - 1u)
_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Audio indices must be lock-free");

/* One producer (ADC DMA IRQ), one consumer (SAI TX DMA IRQ). */
typedef struct {
    int16_t samples[AUDIO_FIFO_SIZE];
    atomic_uint wr, rd;
} audio_fifo;

static inline uint32_t audio_fifo_level(audio_fifo *q)
{
    unsigned wr = atomic_load_explicit(&q->wr, memory_order_acquire);
    unsigned rd = atomic_load_explicit(&q->rd, memory_order_acquire);
    return wr - rd;
}

static inline bool audio_fifo_push_adc(audio_fifo *q, const uint16_t *src,
                                       unsigned n)
{
    unsigned wr = atomic_load_explicit(&q->wr, memory_order_relaxed);
    unsigned rd = atomic_load_explicit(&q->rd, memory_order_acquire);
    if (n > AUDIO_FIFO_SIZE || wr - rd > AUDIO_FIFO_SIZE - n) return false;
    for (unsigned i = 0; i < n; ++i)
        q->samples[(wr + i) & AUDIO_FIFO_MASK] =
            (int16_t)(((int32_t)src[i] - 2048) * 16);
    atomic_store_explicit(&q->wr, wr + n, memory_order_release);
    return true;
}

static inline bool audio_fifo_pop(audio_fifo *q, int16_t *sample)
{
    unsigned rd = atomic_load_explicit(&q->rd, memory_order_relaxed);
    unsigned wr = atomic_load_explicit(&q->wr, memory_order_acquire);
    if (rd == wr) return false;
    *sample = q->samples[rd & AUDIO_FIFO_MASK];
    atomic_store_explicit(&q->rd, rd + 1u, memory_order_release);
    return true;
}

/* High fill slows capture; low fill speeds it up. One tick is 41.7 ns at
 * the current 24 MHz timer clock. Bound correction to about +/-1.6%. */
static inline unsigned audio_capture_period(unsigned nominal, unsigned level)
{
    int correction = ((int)level - 768) / 256;
    if (correction > 8) correction = 8;
    if (correction < -8) correction = -8;
    return (unsigned)((int)nominal + correction);
}
#endif
