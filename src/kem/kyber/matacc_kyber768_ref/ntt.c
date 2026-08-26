#include <stdint.h>
#include "params.h"
#include "ntt.h"
#include "reduce.h"

#if defined(NEON)
#include "arm_neon.h"
#endif

/* Code to generate zetas and zetas_inv used in the number-theoretic transform:

#define KYBER_ROOT_OF_UNITY 17

static const uint8_t tree[128] = {
  0, 64, 32, 96, 16, 80, 48, 112, 8, 72, 40, 104, 24, 88, 56, 120,
  4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
  2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122,
  6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
  1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121,
  5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
  3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
  7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127
};

void init_ntt() {
  unsigned int i;
  int16_t tmp[128];

  tmp[0] = MONT;
  for(i=1;i<128;i++)
    tmp[i] = fqmul(tmp[i-1],MONT*KYBER_ROOT_OF_UNITY % KYBER_Q);

  for(i=0;i<128;i++) {
    zetas[i] = tmp[tree[i]];
    if(zetas[i] > KYBER_Q/2)
      zetas[i] -= KYBER_Q;
    if(zetas[i] < -KYBER_Q/2)
      zetas[i] += KYBER_Q;
  }
}
*/

#if defined(NEON)
const int16_t __attribute__((aligned(16))) zetas[128] = {
#else
const int16_t zetas[128] = {
#endif
  -1044,  -758,  -359, -1517,  1493,  1422,   287,   202,
   -171,   622,  1577,   182,   962, -1202, -1474,  1468,
    573, -1325,   264,   383,  -829,  1458, -1602,  -130,
   -681,  1017,   732,   608, -1542,   411,  -205, -1571,
   1223,   652,  -552,  1015, -1293,  1491,  -282, -1544,
    516,    -8,  -320,  -666, -1618, -1162,   126,  1469,
   -853,   -90,  -271,   830,   107, -1421,  -247,  -951,
   -398,   961, -1508,  -725,   448, -1065,   677, -1275,
  -1103,   430,   555,   843, -1251,   871,  1550,   105,
    422,   587,   177,  -235,  -291,  -460,  1574,  1653,
   -246,   778,  1159,  -147,  -777,  1483,  -602,  1119,
  -1590,   644,  -872,   349,   418,   329,  -156,   -75,
    817,  1097,   603,   610,  1322, -1285, -1465,   384,
  -1215,  -136,  1218, -1335,  -874,   220, -1187, -1659,
  -1185, -1530, -1278,   794, -1510,  -854,  -870,   478,
   -108,  -308,   996,   991,   958, -1460,  1522,  1628
};

#if defined(NEON)
const int16_t __attribute__((aligned(16))) inv_zetas[128] = {
    1628,  1522, -1460,   958,   991,   996,  -308,  -108,
     478,  -870,  -854, -1510,   794, -1278, -1530, -1185,
   -1659, -1187,   220,  -874, -1335,  1218,  -136, -1215,
     384, -1465, -1285,  1322,   610,   603,  1097,   817,
     -75,  -156,   329,   418,   349,  -872,   644, -1590,
    1119,  -602,  1483,  -777,  -147,  1159,   778,  -246,
    1653,  1574,  -460,  -291,  -235,   177,   587,   422,
     105,  1550,   871, -1251,   843,   555,   430, -1103,
   -1275,   677, -1065,   448,  -725, -1508,   961,  -398,
    -951,  -247, -1421,   107,   830,  -271,   -90,  -853,
    1469,   126, -1162, -1618,  -666,  -320,    -8,   516,
   -1544,  -282,  1491, -1293,  1015,  -552,   652,  1223,
   -1571,  -205,   411, -1542,   608,   732,  1017,  -681,
    -130, -1602,  1458,  -829,   383,   264, -1325,   573,
    1468, -1474, -1202,   962,   182,  1577,   622,  -171,
     202,   287,  1422,  1493, -1517,  -359,  -758, -1044
};
#endif

#if defined(NEON)

/*************************************************
* Name:        v_fqmul_neon
*
* Description: NEON vectorized multiplication followed by Montgomery reduction
*
* Arguments:   - int16x8_t v_a: first factor (8 x int16)
*              - int16x8_t v_zeta: second factor (8 x int16)
*              - int16x8_t v_q: modulus q broadcast (8 x int16)
*              - int16x8_t v_qinv: q inverse broadcast (8 x int16)
*
* Returns 8 x 16-bit integers congruent to a*zeta*R^{-1} mod q
**************************************************/
static inline int16x8_t v_fqmul_neon(int16x8_t v_a, int16x8_t v_zeta,
    int16x8_t v_q, int16x8_t v_qinv) {
    int32x4_t v_prod_l = vmull_s16(vget_low_s16(v_a), vget_low_s16(v_zeta));
    int32x4_t v_prod_h = vmull_high_s16(v_a, v_zeta);

    int16x4_t v_m_l = vmul_s16(vmovn_s32(v_prod_l), vget_low_s16(v_qinv));
    int16x4_t v_m_h = vmul_s16(vmovn_s32(v_prod_h), vget_low_s16(v_qinv));

    int32x4_t v_res_l = vmlsl_s16(v_prod_l, v_m_l, vget_low_s16(v_q));
    int32x4_t v_res_h = vmlsl_s16(v_prod_h, v_m_h, vget_low_s16(v_q));

    return vcombine_s16(vshrn_n_s32(v_res_l, 16), vshrn_n_s32(v_res_h, 16));
}

/*************************************************
* Name:        ntt
*
* Description: NEON-optimized inplace number-theoretic transform (NTT) in Rq.
*              input is in standard order, output is in bitreversed order
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void ntt(int16_t r[256]) {
    const int16x8_t v_q = vdupq_n_s16(3329);
    const int16x8_t v_qinv = vdupq_n_s16(-3327);
    unsigned int len, start, j, k = 1;

    int16x8_t v_rj, v_rj_len, v_t, v_sum, v_diff, v_zeta;
    int16x8_t v_rj_A, v_rj_B, v_rj_len_A, v_rj_len_B;
    int16x8_t v_t_A, v_t_B, v_sum_A, v_sum_B, v_diff_A, v_diff_B;

    for (len = 128; len >= 2; len >>= 1) {
        if (len == 4) {
            for (j = 0; j < 256; j += 16) {
                int16_t z1 = zetas[k++];
                int16_t z2 = zetas[k++];
                v_zeta = vcombine_s16(vdup_n_s16(z1), vdup_n_s16(z2));

                int16x8_t V1 = vld1q_s16(&r[j]);
                int16x8_t V2 = vld1q_s16(&r[j + 8]);

                int16x8_t V_A = vcombine_s16(vget_low_s16(V1), vget_low_s16(V2));
                int16x8_t V_B = vcombine_s16(vget_high_s16(V1), vget_high_s16(V2));

                int16x8_t V_t = v_fqmul_neon(V_B, v_zeta, v_q, v_qinv);

                int16x8_t V_sum = vaddq_s16(V_A, V_t);
                int16x8_t V_diff = vsubq_s16(V_A, V_t);

                int16x8_t V1_out = vcombine_s16(vget_low_s16(V_sum), vget_low_s16(V_diff));
                int16x8_t V2_out = vcombine_s16(vget_high_s16(V_sum), vget_high_s16(V_diff));

                vst1q_s16(&r[j], V1_out);
                vst1q_s16(&r[j + 8], V2_out);
            }
        }
        else if (len == 2) {
            for (j = 0; j < 256; j += 16) {
                int16_t z1 = zetas[k++]; int16_t z2 = zetas[k++];
                int16_t z3 = zetas[k++]; int16_t z4 = zetas[k++];
                int16_t z_arr[8] = { z1, z1, z2, z2, z3, z3, z4, z4 };
                v_zeta = vld1q_s16(z_arr);

                int16x8_t V1 = vld1q_s16(&r[j]);
                int16x8_t V2 = vld1q_s16(&r[j + 8]);

                int32x4_t v1_32 = vreinterpretq_s32_s16(V1);
                int32x4_t v2_32 = vreinterpretq_s32_s16(V2);

                int16x8_t V_A = vreinterpretq_s16_s32(vuzp1q_s32(v1_32, v2_32));
                int16x8_t V_B = vreinterpretq_s16_s32(vuzp2q_s32(v1_32, v2_32));

                int16x8_t V_t = v_fqmul_neon(V_B, v_zeta, v_q, v_qinv);

                int16x8_t V_sum = vaddq_s16(V_A, V_t);
                int16x8_t V_diff = vsubq_s16(V_A, V_t);

                int32x4_t vsum_32 = vreinterpretq_s32_s16(V_sum);
                int32x4_t vdiff_32 = vreinterpretq_s32_s16(V_diff);

                int16x8_t V1_out = vreinterpretq_s16_s32(vzip1q_s32(vsum_32, vdiff_32));
                int16x8_t V2_out = vreinterpretq_s16_s32(vzip2q_s32(vsum_32, vdiff_32));

                vst1q_s16(&r[j], V1_out);
                vst1q_s16(&r[j + 8], V2_out);
            }
        }
        else {
            for (start = 0; start < 256; start += 2 * len) {
                int16_t zeta = zetas[k++];
                v_zeta = vdupq_n_s16(zeta);

                if (len >= 16) {
                    for (j = start; j < start + len; j += 16) {
                        v_rj_A = vld1q_s16(&r[j]);
                        v_rj_B = vld1q_s16(&r[j + 8]);
                        v_rj_len_A = vld1q_s16(&r[j + len]);
                        v_rj_len_B = vld1q_s16(&r[j + 8 + len]);

                        v_t_A = v_fqmul_neon(v_rj_len_A, v_zeta, v_q, v_qinv);
                        v_t_B = v_fqmul_neon(v_rj_len_B, v_zeta, v_q, v_qinv);

                        v_sum_A = vaddq_s16(v_rj_A, v_t_A);
                        v_diff_A = vsubq_s16(v_rj_A, v_t_A);
                        v_sum_B = vaddq_s16(v_rj_B, v_t_B);
                        v_diff_B = vsubq_s16(v_rj_B, v_t_B);

                        vst1q_s16(&r[j], v_sum_A);
                        vst1q_s16(&r[j + 8], v_sum_B);
                        vst1q_s16(&r[j + len], v_diff_A);
                        vst1q_s16(&r[j + 8 + len], v_diff_B);
                    }
                }
                else {
                    for (j = start; j < start + len; j += 8) {
                        v_rj = vld1q_s16(&r[j]);
                        v_rj_len = vld1q_s16(&r[j + len]);

                        v_t = v_fqmul_neon(v_rj_len, v_zeta, v_q, v_qinv);

                        v_sum = vaddq_s16(v_rj, v_t);
                        v_diff = vsubq_s16(v_rj, v_t);

                        vst1q_s16(&r[j], v_sum);
                        vst1q_s16(&r[j + len], v_diff);
                    }
                }
            }
        }
    }
}

/*************************************************
* Name:        invntt
*
* Description: NEON-optimized inplace inverse number-theoretic transform in Rq
*              and multiplication by Montgomery factor 2^16.
*              Input is in bitreversed order, output is in standard order
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void invntt(int16_t r[256]) {
    unsigned int len, start, j, k = 0;
    int16x8_t v_rj, v_rj_len, v_sum, v_diff, v_zeta, v_res_b;
    const int16_t f = 1441;

    const int16x8_t v_q = vdupq_n_s16(3329);
    const int16x8_t v_qinv = vdupq_n_s16(-3327);

    for (len = 2; len <= 128; len <<= 1) {

        if (len == 2) {
            for (j = 0; j < 256; j += 16) {
                int16_t z1 = inv_zetas[k++]; int16_t z2 = inv_zetas[k++];
                int16_t z3 = inv_zetas[k++]; int16_t z4 = inv_zetas[k++];
                int16_t z_arr[8] = { z1, z1, z2, z2, z3, z3, z4, z4 };
                v_zeta = vld1q_s16(z_arr);

                int16x8_t V1 = vld1q_s16(&r[j]);
                int16x8_t V2 = vld1q_s16(&r[j + 8]);

                int32x4_t v1_32 = vreinterpretq_s32_s16(V1);
                int32x4_t v2_32 = vreinterpretq_s32_s16(V2);
                int16x8_t V_A = vreinterpretq_s16_s32(vuzp1q_s32(v1_32, v2_32));
                int16x8_t V_B = vreinterpretq_s16_s32(vuzp2q_s32(v1_32, v2_32));

                v_sum = vaddq_s16(V_A, V_B);
                v_diff = vsubq_s16(V_B, V_A);
                v_res_b = v_fqmul_neon(v_diff, v_zeta, v_q, v_qinv);

                int32x4_t vsum_32 = vreinterpretq_s32_s16(v_sum);
                int32x4_t vres_32 = vreinterpretq_s32_s16(v_res_b);
                int16x8_t V1_out = vreinterpretq_s16_s32(vzip1q_s32(vsum_32, vres_32));
                int16x8_t V2_out = vreinterpretq_s16_s32(vzip2q_s32(vsum_32, vres_32));

                vst1q_s16(&r[j], V1_out);
                vst1q_s16(&r[j + 8], V2_out);
            }
        }
        else if (len == 4) {
            for (j = 0; j < 256; j += 16) {
                int16_t z1 = inv_zetas[k++]; int16_t z2 = inv_zetas[k++];
                v_zeta = vcombine_s16(vdup_n_s16(z1), vdup_n_s16(z2));

                int16x8_t V1 = vld1q_s16(&r[j]);
                int16x8_t V2 = vld1q_s16(&r[j + 8]);

                int16x8_t V_A = vcombine_s16(vget_low_s16(V1), vget_low_s16(V2));
                int16x8_t V_B = vcombine_s16(vget_high_s16(V1), vget_high_s16(V2));

                v_sum = vaddq_s16(V_A, V_B);
                v_diff = vsubq_s16(V_B, V_A);
                v_res_b = v_fqmul_neon(v_diff, v_zeta, v_q, v_qinv);

                int16x8_t V1_out = vcombine_s16(vget_low_s16(v_sum), vget_low_s16(v_res_b));
                int16x8_t V2_out = vcombine_s16(vget_high_s16(v_sum), vget_high_s16(v_res_b));

                vst1q_s16(&r[j], V1_out);
                vst1q_s16(&r[j + 8], V2_out);
            }
        }
        else {
            for (start = 0; start < 256; start += 2 * len) {
                int16_t zeta = inv_zetas[k++];
                v_zeta = vdupq_n_s16(zeta);
                for (j = start; j < start + len; j += 8) {
                    v_rj = vld1q_s16(&r[j]);
                    v_rj_len = vld1q_s16(&r[j + len]);

                    v_sum = vaddq_s16(v_rj, v_rj_len);
                    v_diff = vsubq_s16(v_rj_len, v_rj);
                    v_res_b = v_fqmul_neon(v_diff, v_zeta, v_q, v_qinv);

                    vst1q_s16(&r[j], v_sum);
                    vst1q_s16(&r[j + len], v_res_b);
                }
            }
        }
    }

    int16x8_t v_f = vdupq_n_s16(f);
    for (j = 0; j < 256; j += 8) {
        v_rj = vld1q_s16(&r[j]);
        vst1q_s16(&r[j], v_fqmul_neon(v_rj, v_f, v_q, v_qinv));
    }
}

/*************************************************
* Name:        poly_basemul_neon
*
* Description: NEON-optimized multiplication of two polynomials in NTT domain
*
* Arguments:   - int16_t r[256]: output polynomial
*              - const int16_t a[256]: first input polynomial
*              - const int16_t b[256]: second input polynomial
**************************************************/
void poly_basemul_neon(int16_t r[256], const int16_t a[256], const int16_t b[256]) {
    const int16x8_t v_q = vdupq_n_s16(3329);
    const int16x8_t v_qinv = vdupq_n_s16(-3327);
    int j;
    int k = 64;

    for (j = 0; j < 256; j += 16) {
        int16x8_t v_zeta = vld1q_s16(&zetas[k]);
        k += 8;

        int16x8x2_t va = vld2q_s16(&a[j]);
        int16x8x2_t vb = vld2q_s16(&b[j]);

        int16x8_t a1b1 = v_fqmul_neon(va.val[1], vb.val[1], v_q, v_qinv);
        int16x8_t a1b1zeta = v_fqmul_neon(a1b1, v_zeta, v_q, v_qinv);
        int16x8_t a0b0 = v_fqmul_neon(va.val[0], vb.val[0], v_q, v_qinv);
        int16x8_t r0 = vaddq_s16(a1b1zeta, a0b0);

        int16x8_t a0b1 = v_fqmul_neon(va.val[0], vb.val[1], v_q, v_qinv);
        int16x8_t a1b0 = v_fqmul_neon(va.val[1], vb.val[0], v_q, v_qinv);
        int16x8_t r1 = vaddq_s16(a0b1, a1b0);

        int16x8x2_t vr;
        vr.val[0] = r0;
        vr.val[1] = r1;
        vst2q_s16(&r[j], vr);
    }
}

#else /* default: original scalar implementation */

/*************************************************
* Name:        fqmul
*
* Description: Multiplication followed by Montgomery reduction
*
* Arguments:   - int16_t a: first factor
*              - int16_t b: second factor
*
* Returns 16-bit integer congruent to a*b*R^{-1} mod q
**************************************************/
static int16_t fqmul(int16_t a, int16_t b) {
  return montgomery_reduce((int32_t)a*b);
}

/*************************************************
* Name:        ntt
*
* Description: Inplace number-theoretic transform (NTT) in Rq.
*              input is in standard order, output is in bitreversed order
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void ntt(int16_t r[256]) {
  unsigned int len, start, j, k;
  int16_t t, zeta;

  k = 1;
  for(len = 128; len >= 2; len >>= 1) {
    for(start = 0; start < 256; start = j + len) {
      zeta = zetas[k++];
      for(j = start; j < start + len; j++) {
        t = fqmul(zeta, r[j + len]);
        r[j + len] = r[j] - t;
        r[j] = r[j] + t;
      }
    }
  }
}

/*************************************************
* Name:        invntt_tomont
*
* Description: Inplace inverse number-theoretic transform in Rq and
*              multiplication by Montgomery factor 2^16.
*              Input is in bitreversed order, output is in standard order
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void invntt(int16_t r[256]) {
  unsigned int start, len, j, k;
  int16_t t, zeta;
  const int16_t f = 1441; // mont^2/128

  k = 127;
  for(len = 2; len <= 128; len <<= 1) {
    for(start = 0; start < 256; start = j + len) {
      zeta = zetas[k--];
      for(j = start; j < start + len; j++) {
        t = r[j];
        r[j] = barrett_reduce(t + r[j + len]);
        r[j + len] = r[j + len] - t;
        r[j + len] = fqmul(zeta, r[j + len]);
      }
    }
  }

  for(j = 0; j < 256; j++)
    r[j] = fqmul(r[j], f);
}

/*************************************************
* Name:        basemul
*
* Description: Multiplication of polynomials in Zq[X]/(X^2-zeta)
*              used for multiplication of elements in Rq in NTT domain
*
* Arguments:   - int16_t r[2]: pointer to the output polynomial
*              - const int16_t a[2]: pointer to the first factor
*              - const int16_t b[2]: pointer to the second factor
*              - int16_t zeta: integer defining the reduction polynomial
**************************************************/
void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta)
{
  r[0]  = fqmul(a[1], b[1]);
  r[0]  = fqmul(r[0], zeta);
  r[0] += fqmul(a[0], b[0]);
  r[1]  = fqmul(a[0], b[1]);
  r[1] += fqmul(a[1], b[0]);
}

#endif /* NEON */
