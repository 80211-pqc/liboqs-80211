#ifndef API_H
#define API_H

#include <stdint.h>

#define matacc_kyber512_SECRETKEYBYTES  1632
#define matacc_kyber512_PUBLICKEYBYTES  800
#define matacc_kyber512_CIPHERTEXTBYTES 768
#define matacc_kyber512_BYTES           32

#define matacc_kyber512_ref_SECRETKEYBYTES  matacc_kyber512_SECRETKEYBYTES
#define matacc_kyber512_ref_PUBLICKEYBYTES  matacc_kyber512_PUBLICKEYBYTES
#define matacc_kyber512_ref_CIPHERTEXTBYTES matacc_kyber512_CIPHERTEXTBYTES
#define matacc_kyber512_ref_BYTES           matacc_kyber512_BYTES

int matacc_kyber512_ref_keypair(uint8_t *pk, uint8_t *sk);
int matacc_kyber512_ref_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int matacc_kyber512_ref_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#define matacc_kyber768_SECRETKEYBYTES  2400
#define matacc_kyber768_PUBLICKEYBYTES  1184
#define matacc_kyber768_CIPHERTEXTBYTES 1088
#define matacc_kyber768_BYTES           32

#define matacc_kyber768_ref_SECRETKEYBYTES  matacc_kyber768_SECRETKEYBYTES
#define matacc_kyber768_ref_PUBLICKEYBYTES  matacc_kyber768_PUBLICKEYBYTES
#define matacc_kyber768_ref_CIPHERTEXTBYTES matacc_kyber768_CIPHERTEXTBYTES
#define matacc_kyber768_ref_BYTES           matacc_kyber768_BYTES

int matacc_kyber768_ref_keypair(uint8_t *pk, uint8_t *sk);
int matacc_kyber768_ref_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int matacc_kyber768_ref_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#define matacc_kyber1024_SECRETKEYBYTES  3168
#define matacc_kyber1024_PUBLICKEYBYTES  1568
#define matacc_kyber1024_CIPHERTEXTBYTES 1568
#define matacc_kyber1024_BYTES           32

#define matacc_kyber1024_ref_SECRETKEYBYTES  matacc_kyber1024_SECRETKEYBYTES
#define matacc_kyber1024_ref_PUBLICKEYBYTES  matacc_kyber1024_PUBLICKEYBYTES
#define matacc_kyber1024_ref_CIPHERTEXTBYTES matacc_kyber1024_CIPHERTEXTBYTES
#define matacc_kyber1024_ref_BYTES           matacc_kyber1024_BYTES

int matacc_kyber1024_ref_keypair(uint8_t *pk, uint8_t *sk);
int matacc_kyber1024_ref_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int matacc_kyber1024_ref_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#endif
