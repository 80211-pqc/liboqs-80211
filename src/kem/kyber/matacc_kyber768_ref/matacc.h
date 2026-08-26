#ifndef MATACC_H
#define MATACC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "symmetric.h"

/*
 * Six inner-loop helpers: portable C99 replacements for the ARM Cortex-M4
 * assembly routines in the original kyber-main-stack.
 *
 * Naming convention:
 *   cache : generate and WRITE the b_prime cache (aprimeptr) during this call.
 *   opt   : READ the pre-built b_prime cache; skip recomputing mont(a·ζ).
 *
 *   16_32 : first polynomial  – INITIALISE r_tmp.
 *   32_32 : middle polynomial – ACCUMULATE  r_tmp.
 *   32_16 : last polynomial   – ACCUMULATE + Montgomery-reduce → int16_t r.
 */
void matacc_asm_cache_16_32(int32_t *r_tmp, const int16_t *b, int16_t c[4],
                             unsigned char buf[XOF_BLOCKBYTES+2],
                             xof_state *state, int16_t *aprimeptr);

void matacc_asm_cache_32_32(int32_t *r_tmp, const int16_t *b, int16_t c[4],
                             unsigned char buf[XOF_BLOCKBYTES+2],
                             xof_state *state, int16_t *aprimeptr);

void matacc_asm_cache_32_16(int16_t *r, const int16_t *b, int16_t c[4],
                             unsigned char buf[XOF_BLOCKBYTES+2],
                             xof_state *state, int16_t *aprimeptr,
                             const int32_t *r_tmp);

void matacc_asm_opt_16_32(int32_t *r_tmp, const int16_t *b, int16_t c[4],
                           unsigned char buf[XOF_BLOCKBYTES+2],
                           xof_state *state, const int16_t *aprimeptr);

void matacc_asm_opt_32_32(int32_t *r_tmp, const int16_t *b, int16_t c[4],
                           unsigned char buf[XOF_BLOCKBYTES+2],
                           xof_state *state, const int16_t *aprimeptr);

void matacc_asm_opt_32_16(int16_t *r, const int16_t *b, int16_t c[4],
                           unsigned char buf[XOF_BLOCKBYTES+2],
                           xof_state *state, const int16_t *aprimeptr,
                           const int32_t *r_tmp);

/*
 * High-level API used by indcpa.c
 *
 * matacc_cache32 – first row of A (or A^T): generates the b_prime cache.
 * matacc_opt32   – subsequent rows: reads the pre-built cache.
 */
#define matacc_cache32 KYBER_NAMESPACE(matacc_cache32)
void matacc_cache32(poly *r, const polyvec *b, polyvec *b_prime,
                    unsigned char i, const unsigned char *seed, int transposed);

#define matacc_opt32 KYBER_NAMESPACE(matacc_opt32)
void matacc_opt32(poly *r, const polyvec *b, const polyvec *b_prime,
                  unsigned char i, const unsigned char *seed, int transposed);

#endif /* MATACC_H */
