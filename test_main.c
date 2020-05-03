//  test_main.c
//  2020-01-23  Markku-Juhani O. Saarinen <mjos@pqshield.com>
//  Copyright (c) 2020, PQShield Ltd. All rights reserved.

//  Minimal unit tests for AES-128/192/256 (FIPS 197) and SM4 (GM/T 0002-2012).

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "aes_wrap.h"
#include "crypto_rv32.h"
#include "gcm_wrap.h"
#include "gcm_gfmul.h"

//  unit tests

int test_aes();								//  aes_test.c
int test_sm4();								//  sm4_test.c
int test_gcm();								//  gcm_test.c

//  generate "reference" hw testbench data for the instruction
//  output should match with hdl/saes32_tb.v

int test_hwtb()
{
	uint32_t rd, rs1, rs2, fn;

	rs1 = 0x00000000;
	rs2 = 0x00000000;

	for (fn = 0; fn < 24; fn++) {

		rd = saes32(rs1, rs2, fn);

		printf("[TB] rd=%08x rs1=%08x rs2=%08x fn=%02x\n", rd, rs1, rs2, fn);

		rs2 += 0x01234567;
	}

	return 0;
}

//  stub main: run unit tests

int main(int argc, char **argv)
{
	int fail = 0;

	//  generate hardware testbench data ?
	if (argc > 1 && strcmp(argv[1], "tb") == 0) {
		return test_hwtb();
	}
	//  algorithm tests
	printf("[INFO] === AES using SAES64 ===\n");
	aes_enc_rounds = saes64_enc_rounds;

	aes_dec_rounds = saes64_dec_rounds;
	aes128_dec_key = saes64_dec_key128;
	aes192_dec_key = saes64_dec_key192;
	aes256_dec_key = saes64_dec_key256;
	fail += test_aes();

	return fail;

	printf("[INFO] === AES using SAES32 ===\n");
	aes_enc_rounds = saes32_enc_rounds;
	aes128_enc_key = saes32_enc_key128;
	aes192_enc_key = saes32_enc_key192;
	aes256_enc_key = saes32_enc_key256;

	aes_dec_rounds = saes32_dec_rounds;
	aes128_dec_key = saes32_dec_key128;
	aes192_dec_key = saes32_dec_key192;
	aes256_dec_key = saes32_dec_key256;

	fail += test_aes();

	printf("[INFO] === GCM using rv64_ghash_mul() ===\n");
	ghash_rev = rv64_ghash_rev;
	ghash_mul = rv64_ghash_mul;
	fail += test_gcm();

	printf("[INFO] === GCM using rv32_ghash_mul() ===\n");
	ghash_rev = rv32_ghash_rev;
	ghash_mul = rv32_ghash_mul;
	fail += test_gcm();

	printf("[INFO] === GCM using rv32_ghash_mul_kar() ===\n");
	ghash_rev = rv32_ghash_rev;
	ghash_mul = rv32_ghash_mul_kar;
	fail += test_gcm();

	printf("[INFO] === SM4 test ===\n");
	fail += test_sm4();

	if (fail == 0) {
		printf("[PASS] all tests passed.\n");
	} else {
		printf("[FAIL] %d test(s) failed.\n", fail);
	}

	return fail;
}
