#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "indcpa.h"
#include "polyvec.h"
#include "poly.h"
#include "ntt.h"
#include "symmetric.h"
#include "randombytes.h"
#include "matacc.h"

static void pack_pk(uint8_t r[KYBER_INDCPA_PUBLICKEYBYTES],
                    polyvec *pk,
                    const uint8_t seed[KYBER_SYMBYTES])
{
  polyvec_tobytes(r, pk);
  memcpy(r + KYBER_POLYVECBYTES, seed, KYBER_SYMBYTES);
}

static void unpack_pk(polyvec *pk,
                      uint8_t seed[KYBER_SYMBYTES],
                      const uint8_t packedpk[KYBER_INDCPA_PUBLICKEYBYTES])
{
  polyvec_frombytes(pk, packedpk);
  memcpy(seed, packedpk + KYBER_POLYVECBYTES, KYBER_SYMBYTES);
}

static void pack_sk(uint8_t r[KYBER_INDCPA_SECRETKEYBYTES], polyvec *sk)
{
  polyvec_tobytes(r, sk);
}

static void unpack_sk(polyvec *sk,
                      const uint8_t packedsk[KYBER_INDCPA_SECRETKEYBYTES])
{
  polyvec_frombytes(sk, packedsk);
}

static void pack_ciphertext(uint8_t r[KYBER_INDCPA_BYTES],
                             polyvec *b, poly *v)
{
  polyvec_compress(r, b);
  poly_compress(r + KYBER_POLYVECCOMPRESSEDBYTES, v);
}

static void unpack_ciphertext(polyvec *b, poly *v,
                               const uint8_t c[KYBER_INDCPA_BYTES])
{
  polyvec_decompress(b, c);
  poly_decompress(v, c + KYBER_POLYVECCOMPRESSEDBYTES);
}

/*************************************************
* Name:        indcpa_keypair
*
* Description: Generates public and private key for the CPA-secure
*              public-key encryption scheme underlying Kyber.
*              Uses matacc for on-the-fly matrix-vector multiplication,
*              avoiding storing the full matrix A in memory.
*
* Arguments:   - uint8_t *pk: pointer to output public key
*              - uint8_t *sk: pointer to output private key
**************************************************/
void indcpa_keypair(uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                    uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES])
{
  unsigned int i;
  uint8_t buf[2*KYBER_SYMBYTES];
  const uint8_t *publicseed = buf;
  const uint8_t *noiseseed  = buf + KYBER_SYMBYTES;
  uint8_t nonce = 0;
  polyvec skpv, skpv_prime, pkpv;
  poly e;

  randombytes(buf, KYBER_SYMBYTES);
  hash_g(buf, buf, KYBER_SYMBYTES);

  for (i = 0; i < KYBER_K; i++)
    poly_getnoise_eta1(&skpv.vec[i], noiseseed, nonce++);

  polyvec_ntt(&skpv);

  /* Matrix-vector multiplication A·sk using on-the-fly matacc */
  matacc_cache32(&pkpv.vec[0], &skpv, &skpv_prime, 0, publicseed, 0);
  for (i = 1; i < KYBER_K; i++)
    matacc_opt32(&pkpv.vec[i], &skpv, &skpv_prime, i, publicseed, 0);

  /* INTT, add noise in polynomial domain, forward NTT */
  for (i = 0; i < KYBER_K; i++) {
    poly_invntt_tomont(&pkpv.vec[i]);
    poly_getnoise_eta1(&e, noiseseed, nonce++);
    poly_add(&pkpv.vec[i], &pkpv.vec[i], &e);
    poly_ntt(&pkpv.vec[i]);
  }

  pack_sk(sk, &skpv);
  pack_pk(pk, &pkpv, publicseed);
}

/*************************************************
* Name:        indcpa_enc
*
* Description: Encryption function of the CPA-secure
*              public-key encryption scheme underlying Kyber.
*              Uses matacc for on-the-fly A^T·r computation.
*
* Arguments:   - uint8_t *c:      pointer to output ciphertext
*              - const uint8_t *m: pointer to input message
*              - const uint8_t *pk: pointer to input public key
*              - const uint8_t *coins: pointer to input randomness
**************************************************/
void indcpa_enc(uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[KYBER_SYMBYTES])
{
  unsigned int i;
  uint8_t seed[KYBER_SYMBYTES];
  uint8_t nonce = 0;
  polyvec sp, sp_prime, pkpv, b;
  poly v, k, ep, epp;

  unpack_pk(&pkpv, seed, pk);
  poly_frommsg(&k, m);

  for (i = 0; i < KYBER_K; i++)
    poly_getnoise_eta1(sp.vec + i, coins, nonce++);

  polyvec_ntt(&sp);

  /* Compute b = A^T·sp using on-the-fly matacc (transposed=1) */
  matacc_cache32(&b.vec[0], &sp, &sp_prime, 0, seed, 1);
  for (i = 1; i < KYBER_K; i++)
    matacc_opt32(&b.vec[i], &sp, &sp_prime, i, seed, 1);

  /* INTT, add noise in polynomial domain */
  for (i = 0; i < KYBER_K; i++) {
    poly_invntt_tomont(&b.vec[i]);
    poly_getnoise_eta2(&ep, coins, nonce++);
    poly_add(&b.vec[i], &b.vec[i], &ep);
    poly_reduce(&b.vec[i]);
  }

  /* Compute v = pk·sp + epp + k */
  polyvec_basemul_acc_montgomery(&v, &pkpv, &sp);
  poly_invntt_tomont(&v);

  poly_getnoise_eta2(&epp, coins, nonce++);
  poly_add(&v, &v, &epp);
  poly_add(&v, &v, &k);
  poly_reduce(&v);

  pack_ciphertext(c, &b, &v);
}

/*************************************************
* Name:        indcpa_dec
*
* Description: Decryption function of the CPA-secure scheme.
*              Identical to the pqcrystals reference implementation.
*
* Arguments:   - uint8_t *m:      pointer to output decrypted message
*              - const uint8_t *c: pointer to input ciphertext
*              - const uint8_t *sk: pointer to input secret key
**************************************************/
void indcpa_dec(uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES])
{
  polyvec b, skpv;
  poly v, mp;

  unpack_ciphertext(&b, &v, c);
  unpack_sk(&skpv, sk);

  polyvec_ntt(&b);
  polyvec_basemul_acc_montgomery(&mp, &skpv, &b);
  poly_invntt_tomont(&mp);

  poly_sub(&mp, &v, &mp);
  poly_reduce(&mp);

  poly_tomsg(m, &mp);
}
