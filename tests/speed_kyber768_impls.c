// SPDX-License-Identifier: MIT
// Custom benchmark: compare Kyber768 implementation variants directly.
// Currently available in liboqs.a on this x86 build:
//   - pqcrystals_kyber768_ref
//   - matacc_kyber768_ref
// Not available on x86: oldpqclean_kyber768 (aarch64 only)
// To add libjade: rebuild with -DOQS_ENABLE_LIBJADE_KEM_kyber_768=ON

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_benchmark.h"
#include "system_info.c"

// Kyber768 key/ciphertext sizes (same across all implementations)
#define KYBER768_PK_BYTES  1184
#define KYBER768_SK_BYTES  2400
#define KYBER768_CT_BYTES  1088
#define KYBER768_SS_BYTES  32

// pqcrystals reference implementation
extern int pqcrystals_kyber768_ref_keypair(uint8_t *pk, uint8_t *sk);
extern int pqcrystals_kyber768_ref_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
extern int pqcrystals_kyber768_ref_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

// matacc reference implementation
extern int matacc_kyber768_ref_keypair(uint8_t *pk, uint8_t *sk);
extern int matacc_kyber768_ref_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
extern int matacc_kyber768_ref_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

// libjade reference implementation (only if compiled in)
#if defined(OQS_ENABLE_LIBJADE_KEM_kyber_768)
extern int libjade_kyber768_ref_keypair(uint8_t *pk, uint8_t *sk);
extern int libjade_kyber768_ref_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
extern int libjade_kyber768_ref_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
#endif

#if defined(OQS_ENABLE_LIBJADE_KEM_kyber_768_avx2)
extern int libjade_kyber768_avx2_keypair(uint8_t *pk, uint8_t *sk);
extern int libjade_kyber768_avx2_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
extern int libjade_kyber768_avx2_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
#endif

typedef struct {
	const char *name;
	int (*keypair)(uint8_t *pk, uint8_t *sk);
	int (*enc)(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
	int (*dec)(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
} impl_t;

static impl_t impls[] = {
	{
		"pqcrystals_kyber768_ref",
		pqcrystals_kyber768_ref_keypair,
		pqcrystals_kyber768_ref_enc,
		pqcrystals_kyber768_ref_dec,
	},
	{
		"matacc_kyber768_ref",
		matacc_kyber768_ref_keypair,
		matacc_kyber768_ref_enc,
		matacc_kyber768_ref_dec,
	},
#if defined(OQS_ENABLE_LIBJADE_KEM_kyber_768)
	{
		"libjade_kyber768_ref",
		libjade_kyber768_ref_keypair,
		libjade_kyber768_ref_enc,
		libjade_kyber768_ref_dec,
	},
#endif
#if defined(OQS_ENABLE_LIBJADE_KEM_kyber_768_avx2)
	{
		"libjade_kyber768_avx2",
		libjade_kyber768_avx2_keypair,
		libjade_kyber768_avx2_enc,
		libjade_kyber768_avx2_dec,
	},
#endif
};

static void bench_impl(const impl_t *impl, uint64_t duration) {
	uint8_t pk[KYBER768_PK_BYTES];
	uint8_t sk[KYBER768_SK_BYTES];
	uint8_t ct[KYBER768_CT_BYTES];
	uint8_t ss_enc[KYBER768_SS_BYTES];
	uint8_t ss_dec[KYBER768_SS_BYTES];

	// Warm-up: generate keys once so enc/dec have valid data
	if (impl->keypair(pk, sk) != 0) {
		fprintf(stderr, "keypair failed for %s\n", impl->name);
		return;
	}
	if (impl->enc(ct, ss_enc, pk) != 0) {
		fprintf(stderr, "enc failed for %s\n", impl->name);
		return;
	}

	printf("%-36s | %10s | %14s | %15s | %10s | %25s | %10s\n",
	       impl->name, "", "", "", "", "", "");

	TIME_OPERATION_SECONDS(impl->keypair(pk, sk),          "keygen", duration)
	TIME_OPERATION_SECONDS(impl->enc(ct, ss_enc, pk),      "encaps", duration)
	TIME_OPERATION_SECONDS(impl->dec(ss_dec, ct, sk),      "decaps", duration)

	// Correctness check
	if (memcmp(ss_enc, ss_dec, KYBER768_SS_BYTES) != 0) {
		fprintf(stderr, "WARNING: shared secret mismatch in %s!\n", impl->name);
	}
}

int main(int argc, char **argv) {
	uint64_t duration = 3;

	if (argc >= 3 && (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--duration") == 0)) {
		duration = (uint64_t)strtol(argv[2], NULL, 10);
		if (duration == 0) {
			duration = 3;
		}
	}

	print_system_info();
	printf("Kyber768 implementation comparison\n");
	printf("===================================\n");
	printf("Duration per operation: %llu seconds\n\n", (unsigned long long)duration);

	PRINT_TIMER_HEADER
	size_t n = sizeof(impls) / sizeof(impls[0]);
	for (size_t i = 0; i < n; i++) {
		bench_impl(&impls[i], duration);
	}
	PRINT_TIMER_FOOTER

	return EXIT_SUCCESS;
}
