/* judge_core.h - the elderly-care "judge" models T2, T3, T6, T10 in plain C.
 *
 * Pure C99, no malloc, no globals that need an OS, no STM32 headers: it compiles on a PC (gcc) and on the
 * Cortex-M4 (arm-none-eabi-gcc).  Numbers are `real`: float on the board (the F407 has a single-precision FPU),
 * double when you build the PC test with -DJUDGE_REAL_DOUBLE (to prove the maths is identical to Python).
 *
 *   T2  trust score        robust Mahalanobis distance to the node's healthy baseline -> trust 0..100
 *   T3  change-point       Bayesian online change-point detection (Adams & MacKay 2007), Normal-Gamma model
 *   T6  fault cause        naive Bayes over 13 yes/no evidence flags -> 7 possible causes
 *   T10 activity           4-state HMM (absent / still / light / active), online forward filter
 */
#ifndef JUDGE_CORE_H
#define JUDGE_CORE_H

#include <stdint.h>

#ifdef JUDGE_REAL_DOUBLE
typedef double real;
#else
typedef float real;
#endif

#include "model_params.h"      /* generated; uses `real`, so the board build stores float constants */

#define BOCPD_RMAX 240                 /* longest run-length tracked (240 ticks = 20 min) */

/* ---------------- T3 state (per node, about 3 KB in float) ---------------- */
typedef struct {
    int  n;                            /* current number of run-length hypotheses */
    real logR[BOCPD_RMAX + 2];         /* log P(run length = r) */
    real mu[BOCPD_RMAX + 2];           /* Normal-Gamma mean per run length */
    real beta[BOCPD_RMAX + 2];         /* Normal-Gamma beta per run length */
} bocpd_t;

typedef struct { real cp_prob; int map_run; real expected_run; } bocpd_out_t;

void bocpd_init(bocpd_t *b);
void bocpd_update(bocpd_t *b, real x, bocpd_out_t *out);

/* ---------------- T2 + T3 together = what the judge does per node, per 5-second tick ---------------- */
typedef struct {
    real d2;          /* squared Mahalanobis distance to the healthy baseline */
    real r;           /* d2 / q99  (1 = edge of normal) */
    real raw_trust;   /* 0..100, this tick only */
    real trust;       /* 0..100, smoothed (falls fast, recovers slowly) */
    real z_b;         /* standardised log-distance fed to the change-point detector */
    real cp_raw;      /* P(a change happened in the last 3 ticks) */
    real cp;          /* same, but 0 for the first 10 ticks (warm-up) */
    int  map_run;     /* most likely run length */
} judge_node_out_t;

typedef struct {
    real     trust[JC_NNODES];
    bocpd_t  bocpd[JC_NNODES];
    unsigned ticks;
} judge_t;

void judge_init(judge_t *j);
void judge_tick_begin(judge_t *j);                     /* call once per 5-second tick, before the node updates */
/* v = the 9 features of this node for this tick (see FEATS in the Python judge); has=0 if the node sent nothing */
void judge_node_update(judge_t *j, int node, const real v[JC_NFEAT], int has, judge_node_out_t *out);

/* ---------------- T6 ---------------- */
/* mask bit k = indicator k is true (order in model_params.h).  Fills post[JC_NCAUSE], returns the index of the top cause. */
int cause_posterior(uint32_t mask, real post[JC_NCAUSE]);

/* ---------------- T10 ---------------- */
typedef struct { real alpha[JC_NSTATE]; int started; } hmm_t;
void hmm_init(hmm_t *h);                               /* also call to reset */
/* x = {log motion, log amplitude-variance} or NULL when there was no data.  Returns the most likely state. */
int  hmm_filter(hmm_t *h, const real *x, real post[JC_NSTATE]);

#endif
