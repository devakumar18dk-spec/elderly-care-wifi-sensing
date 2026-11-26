/* judge_core.c - see judge_core.h.  Line-by-line port of the Python judge (engine.py, bocpd.py, cause.py, activity.py). */
#include "judge_core.h"
#include <math.h>

#ifdef JUDGE_REAL_DOUBLE
#  define R_LOG(x)   log(x)
#  define R_EXP(x)   exp(x)
#  define R_LOG1P(x) log1p(x)
#  define R_LOG2(x)  log2(x)
#  define R_FABS(x)  fabs(x)
#else
#  define R_LOG(x)   logf(x)
#  define R_EXP(x)   expf(x)
#  define R_LOG1P(x) log1pf(x)
#  define R_LOG2(x)  log2f(x)
#  define R_FABS(x)  fabsf(x)
#endif
#define PI_R ((real)3.14159265358979323846)

/* ======================================================= T3: BOCPD ======================================================= */
#define H_HAZARD  ((real)(1.0 / 120.0))
#define LOG_H     ((real)-4.787491742782046)     /* log(1/120)   */
#define LOG_1MH   ((real)-0.00836824967051658)       /* log(1-1/120) */
#define K0 ((real)1)
#define A0 ((real)1)
#define B0 ((real)1)
#define MU0 ((real)0)

static real g_lg[BOCPD_RMAX + 2];                              /* scratch */

void bocpd_init(bocpd_t *b)
{
    b->n = 1; b->logR[0] = 0; b->mu[0] = (real)MU0; b->beta[0] = (real)B0;
}

static void normalise(real *lr, int n)                          /* subtract log-sum-exp so the probabilities sum to 1 */
{
    real m = lr[0];
    for (int i = 1; i < n; i++) if (lr[i] > m) m = lr[i];
    real s = 0;
    for (int i = 0; i < n; i++) s += R_EXP(lr[i] - m);
    real z = m + R_LOG(s);
    for (int i = 0; i < n; i++) lr[i] -= z;
}

void bocpd_update(bocpd_t *b, real x, bocpd_out_t *out)
{
    int n = b->n;
    real m = -INFINITY;
    for (int i = 0; i < n; i++) {                               /* log predictive (Student-t) x prior run-length prob */
        real kap = K0 + (real)i, al = A0 + (real)i * (real)0.5;
        real sc2 = b->beta[i] * (kap + 1) / (al * kap);
        real df  = 2 * al;
        real dx  = x - b->mu[i];
        real lp  = (T3_LG_A1[i] - T3_LG_A[i]) - (real)0.5 * R_LOG(df * PI_R * sc2)
                   - (df + 1) / 2 * R_LOG1P(dx * dx / (df * sc2));
        g_lg[i] = b->logR[i] + lp;
        if (g_lg[i] > m) m = g_lg[i];
    }
    real s = 0;
    for (int i = 0; i < n; i++) s += R_EXP(g_lg[i] - m);
    real log_cp = m + R_LOG(s) + LOG_H;
    real log_gr = LOG_1MH;

    for (int i = n - 1; i >= 0; i--) {                          /* shift up by one (descending = in place) */
        real kap = K0 + (real)i;
        real dx  = x - b->mu[i];
        b->mu[i + 1]   = (kap * b->mu[i] + x) / (kap + 1);
        b->beta[i + 1] = b->beta[i] + kap * dx * dx / (2 * (kap + 1));
        b->logR[i + 1] = g_lg[i] + log_gr;
    }
    b->mu[0] = (real)MU0; b->beta[0] = (real)B0; b->logR[0] = log_cp;
    n += 1;
    normalise(b->logR, n);
    if (n > BOCPD_RMAX + 1) { n = BOCPD_RMAX + 1; normalise(b->logR, n); }
    b->n = n;

    real cp = 0, ex = 0, pmax = -1; int arg = 0;
    for (int i = 0; i < n; i++) {
        real p = R_EXP(b->logR[i]);
        if (i < 4) cp += p;
        ex += p * i;
        if (p > pmax) { pmax = p; arg = i; }
    }
    out->cp_prob = cp; out->map_run = arg; out->expected_run = ex;
}

/* ======================================================= T2 + glue ======================================================= */
void judge_init(judge_t *j)
{
    for (int n = 0; n < JC_NNODES; n++) { j->trust[n] = 100; bocpd_init(&j->bocpd[n]); }
    j->ticks = 0;
}

void judge_tick_begin(judge_t *j) { j->ticks++; }

static real trust_from_r(real r)                                /* r<=1 -> 100, r=3 -> 60, r>=16 -> 0 */
{
    real rr = r < 1 ? 1 : r;
    real t = (real)1 - R_LOG2(rr) / (real)4;
    if (t > 1) t = 1;
    if (t < 0) t = 0;
    return (real)100 * t;
}

void judge_node_update(judge_t *j, int node, const real v[JC_NFEAT], int has, judge_node_out_t *o)
{
    real raw_t, z_b;
    if (!has) {
        o->d2 = 0; o->r = 0; raw_t = 0; z_b = 12;
    } else {
        real z[JC_NFEAT], d2 = 0;
        for (int k = 0; k < JC_NFEAT; k++)
            z[k] = (v[k] - T2_MED[node][k]) / T2_SCALE[node][k] - T2_MU[node][k];
        for (int a = 0; a < JC_NFEAT; a++) {                    /* d2 = z' P z */
            real pz = 0;
            for (int c = 0; c < JC_NFEAT; c++) pz += T2_PREC[node][a * JC_NFEAT + c] * z[c];
            d2 += z[a] * pz;
        }
        o->d2 = d2; o->r = d2 / T2_Q99[node];
        raw_t = trust_from_r(o->r);
        z_b = (R_LOG(d2 + 1) - T2_LOGD_MED[node]) / T2_LOGD_SCALE[node];
        if (z_b < -5) z_b = -5;
        if (z_b > 20) z_b = 20;
    }
    real prev = j->trust[node];
    j->trust[node] = (raw_t < prev) ? (real)0.6 * raw_t + (real)0.4 * prev : (real)0.3 * raw_t + (real)0.7 * prev;

    bocpd_out_t b;
    bocpd_update(&j->bocpd[node], z_b, &b);
    o->raw_trust = raw_t; o->trust = j->trust[node]; o->z_b = z_b;
    o->cp_raw = b.cp_prob; o->cp = (j->ticks > 10) ? b.cp_prob : 0; o->map_run = b.map_run;
}

/* ======================================================= T6: fault cause ======================================================= */
int cause_posterior(uint32_t mask, real post[JC_NCAUSE])
{
    real lp[JC_NCAUSE], mx = -INFINITY; int top = 0;
    for (int c = 0; c < JC_NCAUSE; c++) {
        real l = T6_LOGPRIOR[c];
        for (int k = 0; k < JC_NIND; k++) l += ((mask >> k) & 1u) ? T6_LOGP1[c][k] : T6_LOGP0[c][k];
        lp[c] = l;
        if (l > mx) { mx = l; top = c; }
    }
    real s = 0;
    for (int c = 0; c < JC_NCAUSE; c++) { post[c] = R_EXP(lp[c] - mx); s += post[c]; }
    for (int c = 0; c < JC_NCAUSE; c++) post[c] /= s;
    return top;
}

/* ======================================================= T10: activity HMM ======================================================= */
static real logsumexp(const real *a, int n)
{
    real m = a[0];
    for (int i = 1; i < n; i++) if (a[i] > m) m = a[i];
    real s = 0;
    for (int i = 0; i < n; i++) s += R_EXP(a[i] - m);
    return m + R_LOG(s);
}

void hmm_init(hmm_t *h) { h->started = 0; for (int k = 0; k < JC_NSTATE; k++) h->alpha[k] = 0; }

int hmm_filter(hmm_t *h, const real *x, real post[JC_NSTATE])
{
    real la[JC_NSTATE], tmp[JC_NSTATE];
    for (int k = 0; k < JC_NSTATE; k++) {
        real e = 0;
        if (x) {                                                /* Gaussian emission, diagonal */
            for (int d = 0; d < 2; d++) {
                real dx = x[d] - T10_MU[k][d];
                e += T10_LOGNORM[k][d] + dx * dx / T10_VAR[k][d];
            }
            e *= (real)-0.5;
        }
        if (!h->started) la[k] = T10_LOGPI[k] + e;
        else {
            for (int i = 0; i < JC_NSTATE; i++) tmp[i] = h->alpha[i] + T10_LOGA[i][k];
            la[k] = logsumexp(tmp, JC_NSTATE) + e;
        }
    }
    real z = logsumexp(la, JC_NSTATE);
    int top = 0;
    for (int k = 0; k < JC_NSTATE; k++) {
        h->alpha[k] = la[k] - z; post[k] = R_EXP(h->alpha[k]);
        if (post[k] > post[top]) top = k;
    }
    h->started = 1;
    return top;
}
