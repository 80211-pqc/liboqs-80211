/*
 * matacc.c – Portable C99 implementation of the matacc matrix-vector
 * accumulation for Kyber.
 *
 * These functions perform NTT-domain matrix-vector accumulation while
 * generating matrix rows on-the-fly from a SHAKE128 XOF, avoiding the
 * need to store the full matrix A in memory.
 *
 * ── Naming convention ──────────────────────────────────────────────────────
 *  cache : generate and WRITE b_prime cache (aprimeptr) during this call.
 *  opt   : READ the pre-built b_prime cache; skip recomputing mont(a·ζ).
 *
 *  16_32 : first polynomial  – INITIALISE r_tmp (assign, not accumulate).
 *  32_32 : middle polynomial – ACCUMULATE  r_tmp (+=).
 *  32_16 : last polynomial   – ACCUMULATE + Montgomery-reduce → int16_t r.
 * ───────────────────────────────────────────────────────────────────────────
 *
 * Montgomery notes
 * ────────────────
 *  mont_reduce(a) = a · R⁻¹ mod q,  R = 2^16,  q = KYBER_Q = 3329.
 *  Result is in (-q, q).
 *
 * aprimeptr layout (per group of 4):
 *   aprimeptr[ctr*4+0] = b[ctr*4+0]              (plain)
 *   aprimeptr[ctr*4+1] = mont(b[ctr*4+1] ·  ζ)
 *   aprimeptr[ctr*4+2] = b[ctr*4+2]              (plain)
 *   aprimeptr[ctr*4+3] = mont(b[ctr*4+3] · -ζ)
 */

#include <stdint.h>
#include "params.h"
#include "symmetric.h"
#include "ntt.h"
#include "matacc.h"

/* ── Internal helpers ──────────────────────────────────────────────────── */

static inline int16_t mont_reduce(int32_t a)
{
    int16_t t = (int16_t)((int16_t)a * (int16_t)(-3327));
    return (int16_t)((a - (int32_t)t * (int32_t)KYBER_Q) >> 16);
}

static inline void load_12bit(const unsigned char *p,
                               int16_t *v0, int16_t *v1)
{
    uint32_t t = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    *v0 = (int16_t)(t & 0x0FFF);
    *v1 = (int16_t)((t >> 12) & 0x0FFF);
}

/* ======================================================================= */
/* CACHE VARIANTS – write aprimeptr on every group                          */
/* ======================================================================= */

void matacc_asm_cache_16_32(int32_t *r_tmp, const int16_t *b,
                              int16_t c[4],
                              unsigned char buf[XOF_BLOCKBYTES + 2],
                              xof_state *state, int16_t *aprimeptr)
{
    unsigned char *pos = buf;
    unsigned char *end = buf + XOF_BLOCKBYTES;
    int k = 0, ctr = 0;

    while (ctr < KYBER_N / 4) {
        int16_t v0, v1;
        load_12bit(pos, &v0, &v1);
        pos += 3;

        if (v0 < KYBER_Q) {
            c[k++] = v0;
            if (k == 4) {
                k = 0;
                int16_t a0 = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2 = b[ctr*4+2], a3 = b[ctr*4+3];
                int16_t zt = zetas[64 + ctr];
                int16_t a1z = mont_reduce((int32_t)a1 *  (int32_t)zt);
                int16_t a3z = mont_reduce((int32_t)a3 * -(int32_t)zt);
                aprimeptr[ctr*4+0] = a0;
                aprimeptr[ctr*4+1] = a1z;
                aprimeptr[ctr*4+2] = a2;
                aprimeptr[ctr*4+3] = a3z;
                r_tmp[ctr*4+0]  = (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1]  = (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2]  = (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3]  = (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (v1 < KYBER_Q && ctr < KYBER_N / 4) {
            c[k++] = v1;
            if (k == 4) {
                k = 0;
                int16_t a0 = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2 = b[ctr*4+2], a3 = b[ctr*4+3];
                int16_t zt = zetas[64 + ctr];
                int16_t a1z = mont_reduce((int32_t)a1 *  (int32_t)zt);
                int16_t a3z = mont_reduce((int32_t)a3 * -(int32_t)zt);
                aprimeptr[ctr*4+0] = a0;
                aprimeptr[ctr*4+1] = a1z;
                aprimeptr[ctr*4+2] = a2;
                aprimeptr[ctr*4+3] = a3z;
                r_tmp[ctr*4+0]  = (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1]  = (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2]  = (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3]  = (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (pos + 3 > end && ctr < KYBER_N / 4) {
            xof_squeezeblocks(buf, 1, state);
            pos = buf;
            end = buf + XOF_BLOCKBYTES;
        }
    }
}

void matacc_asm_cache_32_32(int32_t *r_tmp, const int16_t *b,
                              int16_t c[4],
                              unsigned char buf[XOF_BLOCKBYTES + 2],
                              xof_state *state, int16_t *aprimeptr)
{
    unsigned char *pos = buf;
    unsigned char *end = buf + XOF_BLOCKBYTES;
    int k = 0, ctr = 0;

    while (ctr < KYBER_N / 4) {
        int16_t v0, v1;
        load_12bit(pos, &v0, &v1);
        pos += 3;

        if (v0 < KYBER_Q) {
            c[k++] = v0;
            if (k == 4) {
                k = 0;
                int16_t a0 = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2 = b[ctr*4+2], a3 = b[ctr*4+3];
                int16_t zt = zetas[64 + ctr];
                int16_t a1z = mont_reduce((int32_t)a1 *  (int32_t)zt);
                int16_t a3z = mont_reduce((int32_t)a3 * -(int32_t)zt);
                aprimeptr[ctr*4+0] = a0;
                aprimeptr[ctr*4+1] = a1z;
                aprimeptr[ctr*4+2] = a2;
                aprimeptr[ctr*4+3] = a3z;
                r_tmp[ctr*4+0] += (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1] += (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2] += (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3] += (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (v1 < KYBER_Q && ctr < KYBER_N / 4) {
            c[k++] = v1;
            if (k == 4) {
                k = 0;
                int16_t a0 = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2 = b[ctr*4+2], a3 = b[ctr*4+3];
                int16_t zt = zetas[64 + ctr];
                int16_t a1z = mont_reduce((int32_t)a1 *  (int32_t)zt);
                int16_t a3z = mont_reduce((int32_t)a3 * -(int32_t)zt);
                aprimeptr[ctr*4+0] = a0;
                aprimeptr[ctr*4+1] = a1z;
                aprimeptr[ctr*4+2] = a2;
                aprimeptr[ctr*4+3] = a3z;
                r_tmp[ctr*4+0] += (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1] += (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2] += (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3] += (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (pos + 3 > end && ctr < KYBER_N / 4) {
            xof_squeezeblocks(buf, 1, state);
            pos = buf;
            end = buf + XOF_BLOCKBYTES;
        }
    }
}

void matacc_asm_cache_32_16(int16_t *r, const int16_t *b,
                              int16_t c[4],
                              unsigned char buf[XOF_BLOCKBYTES + 2],
                              xof_state *state, int16_t *aprimeptr,
                              const int32_t *r_tmp)
{
    unsigned char *pos = buf;
    unsigned char *end = buf + XOF_BLOCKBYTES;
    int k = 0, ctr = 0;

    while (ctr < KYBER_N / 4) {
        int16_t v0, v1;
        load_12bit(pos, &v0, &v1);
        pos += 3;

        if (v0 < KYBER_Q) {
            c[k++] = v0;
            if (k == 4) {
                k = 0;
                int16_t a0 = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2 = b[ctr*4+2], a3 = b[ctr*4+3];
                int16_t zt = zetas[64 + ctr];
                int16_t a1z = mont_reduce((int32_t)a1 *  (int32_t)zt);
                int16_t a3z = mont_reduce((int32_t)a3 * -(int32_t)zt);
                aprimeptr[ctr*4+0] = a0;
                aprimeptr[ctr*4+1] = a1z;
                aprimeptr[ctr*4+2] = a2;
                aprimeptr[ctr*4+3] = a3z;
                r[ctr*4+0] = mont_reduce(r_tmp[ctr*4+0]
                             + (int32_t)a1z * c[1] + (int32_t)a0 * c[0]);
                r[ctr*4+1] = mont_reduce(r_tmp[ctr*4+1]
                             + (int32_t)a1  * c[0] + (int32_t)a0 * c[1]);
                r[ctr*4+2] = mont_reduce(r_tmp[ctr*4+2]
                             + (int32_t)a3z * c[3] + (int32_t)a2 * c[2]);
                r[ctr*4+3] = mont_reduce(r_tmp[ctr*4+3]
                             + (int32_t)a3  * c[2] + (int32_t)a2 * c[3]);
                ctr++;
            }
        }

        if (v1 < KYBER_Q && ctr < KYBER_N / 4) {
            c[k++] = v1;
            if (k == 4) {
                k = 0;
                int16_t a0 = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2 = b[ctr*4+2], a3 = b[ctr*4+3];
                int16_t zt = zetas[64 + ctr];
                int16_t a1z = mont_reduce((int32_t)a1 *  (int32_t)zt);
                int16_t a3z = mont_reduce((int32_t)a3 * -(int32_t)zt);
                aprimeptr[ctr*4+0] = a0;
                aprimeptr[ctr*4+1] = a1z;
                aprimeptr[ctr*4+2] = a2;
                aprimeptr[ctr*4+3] = a3z;
                r[ctr*4+0] = mont_reduce(r_tmp[ctr*4+0]
                             + (int32_t)a1z * c[1] + (int32_t)a0 * c[0]);
                r[ctr*4+1] = mont_reduce(r_tmp[ctr*4+1]
                             + (int32_t)a1  * c[0] + (int32_t)a0 * c[1]);
                r[ctr*4+2] = mont_reduce(r_tmp[ctr*4+2]
                             + (int32_t)a3z * c[3] + (int32_t)a2 * c[2]);
                r[ctr*4+3] = mont_reduce(r_tmp[ctr*4+3]
                             + (int32_t)a3  * c[2] + (int32_t)a2 * c[3]);
                ctr++;
            }
        }

        if (pos + 3 > end && ctr < KYBER_N / 4) {
            xof_squeezeblocks(buf, 1, state);
            pos = buf;
            end = buf + XOF_BLOCKBYTES;
        }
    }
}

/* ======================================================================= */
/* OPT VARIANTS – read pre-built aprimeptr cache                            */
/* ======================================================================= */

void matacc_asm_opt_16_32(int32_t *r_tmp, const int16_t *b,
                            int16_t c[4],
                            unsigned char buf[XOF_BLOCKBYTES + 2],
                            xof_state *state, const int16_t *aprimeptr)
{
    unsigned char *pos = buf;
    unsigned char *end = buf + XOF_BLOCKBYTES;
    int k = 0, ctr = 0;

    while (ctr < KYBER_N / 4) {
        int16_t v0, v1;
        load_12bit(pos, &v0, &v1);
        pos += 3;

        if (v0 < KYBER_Q) {
            c[k++] = v0;
            if (k == 4) {
                k = 0;
                int16_t a1z = aprimeptr[ctr*4+1];
                int16_t a3z = aprimeptr[ctr*4+3];
                int16_t a0  = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2  = b[ctr*4+2], a3 = b[ctr*4+3];
                r_tmp[ctr*4+0]  = (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1]  = (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2]  = (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3]  = (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (v1 < KYBER_Q && ctr < KYBER_N / 4) {
            c[k++] = v1;
            if (k == 4) {
                k = 0;
                int16_t a1z = aprimeptr[ctr*4+1];
                int16_t a3z = aprimeptr[ctr*4+3];
                int16_t a0  = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2  = b[ctr*4+2], a3 = b[ctr*4+3];
                r_tmp[ctr*4+0]  = (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1]  = (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2]  = (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3]  = (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (pos + 3 > end && ctr < KYBER_N / 4) {
            xof_squeezeblocks(buf, 1, state);
            pos = buf;
            end = buf + XOF_BLOCKBYTES;
        }
    }
}

void matacc_asm_opt_32_32(int32_t *r_tmp, const int16_t *b,
                            int16_t c[4],
                            unsigned char buf[XOF_BLOCKBYTES + 2],
                            xof_state *state, const int16_t *aprimeptr)
{
    unsigned char *pos = buf;
    unsigned char *end = buf + XOF_BLOCKBYTES;
    int k = 0, ctr = 0;

    while (ctr < KYBER_N / 4) {
        int16_t v0, v1;
        load_12bit(pos, &v0, &v1);
        pos += 3;

        if (v0 < KYBER_Q) {
            c[k++] = v0;
            if (k == 4) {
                k = 0;
                int16_t a1z = aprimeptr[ctr*4+1];
                int16_t a3z = aprimeptr[ctr*4+3];
                int16_t a0  = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2  = b[ctr*4+2], a3 = b[ctr*4+3];
                r_tmp[ctr*4+0] += (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1] += (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2] += (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3] += (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (v1 < KYBER_Q && ctr < KYBER_N / 4) {
            c[k++] = v1;
            if (k == 4) {
                k = 0;
                int16_t a1z = aprimeptr[ctr*4+1];
                int16_t a3z = aprimeptr[ctr*4+3];
                int16_t a0  = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2  = b[ctr*4+2], a3 = b[ctr*4+3];
                r_tmp[ctr*4+0] += (int32_t)a1z * c[1] + (int32_t)a0 * c[0];
                r_tmp[ctr*4+1] += (int32_t)a1  * c[0] + (int32_t)a0 * c[1];
                r_tmp[ctr*4+2] += (int32_t)a3z * c[3] + (int32_t)a2 * c[2];
                r_tmp[ctr*4+3] += (int32_t)a3  * c[2] + (int32_t)a2 * c[3];
                ctr++;
            }
        }

        if (pos + 3 > end && ctr < KYBER_N / 4) {
            xof_squeezeblocks(buf, 1, state);
            pos = buf;
            end = buf + XOF_BLOCKBYTES;
        }
    }
}

void matacc_asm_opt_32_16(int16_t *r, const int16_t *b,
                            int16_t c[4],
                            unsigned char buf[XOF_BLOCKBYTES + 2],
                            xof_state *state, const int16_t *aprimeptr,
                            const int32_t *r_tmp)
{
    unsigned char *pos = buf;
    unsigned char *end = buf + XOF_BLOCKBYTES;
    int k = 0, ctr = 0;

    while (ctr < KYBER_N / 4) {
        int16_t v0, v1;
        load_12bit(pos, &v0, &v1);
        pos += 3;

        if (v0 < KYBER_Q) {
            c[k++] = v0;
            if (k == 4) {
                k = 0;
                int16_t a1z = aprimeptr[ctr*4+1];
                int16_t a3z = aprimeptr[ctr*4+3];
                int16_t a0  = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2  = b[ctr*4+2], a3 = b[ctr*4+3];
                r[ctr*4+0] = mont_reduce(r_tmp[ctr*4+0]
                             + (int32_t)a1z * c[1] + (int32_t)a0 * c[0]);
                r[ctr*4+1] = mont_reduce(r_tmp[ctr*4+1]
                             + (int32_t)a1  * c[0] + (int32_t)a0 * c[1]);
                r[ctr*4+2] = mont_reduce(r_tmp[ctr*4+2]
                             + (int32_t)a3z * c[3] + (int32_t)a2 * c[2]);
                r[ctr*4+3] = mont_reduce(r_tmp[ctr*4+3]
                             + (int32_t)a3  * c[2] + (int32_t)a2 * c[3]);
                ctr++;
            }
        }

        if (v1 < KYBER_Q && ctr < KYBER_N / 4) {
            c[k++] = v1;
            if (k == 4) {
                k = 0;
                int16_t a1z = aprimeptr[ctr*4+1];
                int16_t a3z = aprimeptr[ctr*4+3];
                int16_t a0  = b[ctr*4+0], a1 = b[ctr*4+1];
                int16_t a2  = b[ctr*4+2], a3 = b[ctr*4+3];
                r[ctr*4+0] = mont_reduce(r_tmp[ctr*4+0]
                             + (int32_t)a1z * c[1] + (int32_t)a0 * c[0]);
                r[ctr*4+1] = mont_reduce(r_tmp[ctr*4+1]
                             + (int32_t)a1  * c[0] + (int32_t)a0 * c[1]);
                r[ctr*4+2] = mont_reduce(r_tmp[ctr*4+2]
                             + (int32_t)a3z * c[3] + (int32_t)a2 * c[2]);
                r[ctr*4+3] = mont_reduce(r_tmp[ctr*4+3]
                             + (int32_t)a3  * c[2] + (int32_t)a2 * c[3]);
                ctr++;
            }
        }

        if (pos + 3 > end && ctr < KYBER_N / 4) {
            xof_squeezeblocks(buf, 1, state);
            pos = buf;
            end = buf + XOF_BLOCKBYTES;
        }
    }
}

/* ======================================================================= */
/* High-level wrappers called from indcpa.c                                 */
/* ======================================================================= */

/*
 * matacc_cache32 – Multiplies one row of A (or A^T), generated on-the-fly,
 * with the vector b and accumulates into r.  Also populates b_prime with
 * b[i] pre-multiplied by zeta values (cache for subsequent matacc_opt32
 * calls on the same b vector).
 *
 * Call this for the FIRST row (i == 0).
 */
void matacc_cache32(poly *r, const polyvec *b, polyvec *b_prime,
                    unsigned char i, const unsigned char *seed, int transposed)
{
    unsigned char buf[XOF_BLOCKBYTES + 2];
    xof_state state;
    int16_t c[4];
    int32_t r_tmp[KYBER_N];
    int j = 0;

    xof_init(&state, seed);

    /* First polynomial: 16-bit init */
    if (transposed)
        xof_absorb(&state, seed, i, j);
    else
        xof_absorb(&state, seed, j, i);
    xof_squeezeblocks(buf, 1, &state);
    matacc_asm_cache_16_32(r_tmp, b->vec[j].coeffs, c, buf, &state,
                           b_prime->vec[j].coeffs);

    /* Middle polynomials: 32-bit accumulate */
    for (j = 1; j < KYBER_K - 1; j++) {
        if (transposed)
            xof_absorb(&state, seed, i, j);
        else
            xof_absorb(&state, seed, j, i);
        xof_squeezeblocks(buf, 1, &state);
        matacc_asm_cache_32_32(r_tmp, b->vec[j].coeffs, c, buf, &state,
                               b_prime->vec[j].coeffs);
    }

    /* Last polynomial: 32-bit accumulate + Montgomery reduce → int16_t */
    if (transposed)
        xof_absorb(&state, seed, i, j);
    else
        xof_absorb(&state, seed, j, i);
    xof_squeezeblocks(buf, 1, &state);
    matacc_asm_cache_32_16(r->coeffs, b->vec[j].coeffs, c, buf, &state,
                           b_prime->vec[j].coeffs, r_tmp);

    xof_release(&state);
}

/*
 * matacc_opt32 – Multiplies one row of A (or A^T), generated on-the-fly,
 * with the vector b and accumulates into r.  Reads the pre-built b_prime
 * cache instead of recomputing it.
 *
 * Call this for rows i >= 1, after matacc_cache32 has been called for row 0.
 */
void matacc_opt32(poly *r, const polyvec *b, const polyvec *b_prime,
                  unsigned char i, const unsigned char *seed, int transposed)
{
    unsigned char buf[XOF_BLOCKBYTES + 2];
    xof_state state;
    int16_t c[4];
    int32_t r_tmp[KYBER_N];
    int j = 0;

    xof_init(&state, seed);

    /* First polynomial: 16-bit init, read cache */
    if (transposed)
        xof_absorb(&state, seed, i, j);
    else
        xof_absorb(&state, seed, j, i);
    xof_squeezeblocks(buf, 1, &state);
    matacc_asm_opt_16_32(r_tmp, b->vec[j].coeffs, c, buf, &state,
                         b_prime->vec[j].coeffs);

    /* Middle polynomials: 32-bit accumulate, read cache */
    for (j = 1; j < KYBER_K - 1; j++) {
        if (transposed)
            xof_absorb(&state, seed, i, j);
        else
            xof_absorb(&state, seed, j, i);
        xof_squeezeblocks(buf, 1, &state);
        matacc_asm_opt_32_32(r_tmp, b->vec[j].coeffs, c, buf, &state,
                             b_prime->vec[j].coeffs);
    }

    /* Last polynomial: accumulate + Montgomery reduce → int16_t */
    if (transposed)
        xof_absorb(&state, seed, i, j);
    else
        xof_absorb(&state, seed, j, i);
    xof_squeezeblocks(buf, 1, &state);
    matacc_asm_opt_32_16(r->coeffs, b->vec[j].coeffs, c, buf, &state,
                         b_prime->vec[j].coeffs, r_tmp);

    xof_release(&state);
}
