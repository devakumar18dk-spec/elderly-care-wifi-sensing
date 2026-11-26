/* main_demo.c - judge demo for STM32F407VET6 (Cortex-M4F, 168 MHz).
 *
 * What it does: replays recorded feature vectors (stored in flash, demo_vectors.h) through the C judge, prints one
 * line per interesting node-tick over UART, checks every answer against the Python-computed expected value, and
 * measures how many CPU cycles one judge tick takes (DWT cycle counter).
 *
 * How to use: create a CubeIDE project for STM32F407VET6 with USART (e.g. USART1 or USART2, 115200 8N1), add
 *   judge_core.c / judge_core.h / model_params.h / demo_vectors.h / this file,
 * set the FPU to "FPv4-SP-D16, hard ABI" and call judge_demo() from main() after the HAL/clock/UART init.
 * Open a serial terminal at 115200 and press reset.
 * Only edit uart_write() below: make it send bytes on your UART (HAL_UART_Transmit).
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stm32f4xx.h"          /* CMSIS device header (comes with CubeIDE) */
#include "judge_core.h"
#include "demo_vectors.h"

/* ---- 1. hook for your UART (edit this one function) ---- */
extern UART_HandleTypeDef huart2;                     /* the handle CubeMX generated; rename if yours differs */
#include "stm32f4xx_hal.h"
static void uart_write(const char *s, int n) { HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)n, 1000); }
static void say(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#include <stdarg.h>
static void say(const char *fmt, ...)
{
    char b[160]; va_list ap; va_start(ap, fmt); int n = vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    uart_write(b, n < 0 ? 0 : (n > (int)sizeof b - 1 ? (int)sizeof b - 1 : n));
}

/* ---- 2. cycle counter ---- */
static void cyc_init(void) { CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; }
static inline uint32_t cyc(void) { return DWT->CYCCNT; }

static judge_t J;                                      /* about 12 KB: 4 nodes x (T3 state) */

void judge_demo(void)
{
    cyc_init();
    say("\r\n=== elderly-care judge on STM32F407 (T2 trust, T3 change-point, T6 cause, T10 activity) ===\r\n");
    say("core clock %lu MHz, sizeof(judge_t)=%u bytes\r\n", (unsigned long)(SystemCoreClock / 1000000UL), (unsigned)sizeof(judge_t));

    judge_init(&J);
    uint32_t worst = 0, total = 0, n_upd = 0; unsigned bad = 0, checked = 0;

    for (int t = 0; t < DEMO_TICKS; t++) {
        judge_tick_begin(&J);
        for (int n = 0; n < JC_NNODES; n++) {
            int i = t * JC_NNODES + n; real v[JC_NFEAT];
            for (int k = 0; k < JC_NFEAT; k++) v[k] = (real)DEMO_V[i][k];
            judge_node_out_t o;
            uint32_t c0 = cyc(); judge_node_update(&J, n, v, DEMO_HAS[i], &o); uint32_t dc = cyc() - c0;
            total += dc; n_upd++; if (dc > worst) worst = dc;

            int ok = fabsf((float)o.trust - DEMO_EXP_TRUST[i]) < 0.05f && fabsf((float)o.cp - DEMO_EXP_CP[i]) < 0.02f;
            checked++; if (!ok) bad++;
            if (o.trust < 60.0f || o.cp > 0.5f || t % 30 == 0) {
                say("t=%3d node %d  trust %5.1f  change %.2f  run %3d  expected trust %5.1f  %s\r\n",
                    t, n + 1, (double)o.trust, (double)o.cp, o.map_run, (double)DEMO_EXP_TRUST[i], ok ? "OK" : "MISMATCH");
                if (o.trust < 60.0f) {                           /* T6: why is this node low-trust? (demo: simple evidence) */
                    uint32_t mask = 0;
                    if (!DEMO_HAS[i]) mask |= 1u << 0;           /* offline */
                    if (o.cp > 0.5f)  mask |= 1u << 7;           /* amplitude/level shift evidence */
                    real post[JC_NCAUSE]; int top = cause_posterior(mask, post);
                    static const char *const NAME[JC_NCAUSE] = {"healthy", "signal_dropout", "node_stuck", "level_shift",
                                                                 "noise_or_interference", "node_moved", "room_wide_change"};
                    say("        likely cause: %s (%.0f%%)\r\n", NAME[top], (double)post[top] * 100.0);
                }
            }
        }
    }
    say("\r\nchecked %u node-ticks against the Python judge: %u mismatches  ->  %s\r\n", checked, bad, bad ? "FAIL" : "PASS");
    say("time per node update (T2+T3): average %lu cycles = %lu us, worst %lu cycles = %lu us\r\n",
        (unsigned long)(total / n_upd), (unsigned long)(total / n_upd / (SystemCoreClock / 1000000UL)),
        (unsigned long)worst, (unsigned long)(worst / (SystemCoreClock / 1000000UL)));

    /* T10: feed a short made-up motion sequence and show the activity state */
    hmm_t H; hmm_init(&H); real post[JC_NSTATE];
    static const char *const SN[JC_NSTATE] = {"absent", "still", "light", "active"};
    real x[2] = {-4.0f, -6.0f};
    uint32_t c0 = cyc(); int s = hmm_filter(&H, x, post); uint32_t dc = cyc() - c0;
    say("T10 activity step: %lu cycles, state=%s (%.0f%%)\r\n", (unsigned long)dc, SN[s], (double)post[s] * 100.0);
    say("done.\r\n");
}
