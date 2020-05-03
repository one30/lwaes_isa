//  saes32_dec.c
//  2020-01-22  Markku-Juhani O. Saarinen <mjos@pqhsield.com>
//  Copyright (c) 2020, PQShield Ltd. All rights reserved.

//  Running pseudocode for full AES-128/192/256 decryption.

#include "crypto_saes32.h"
#include "aes_wrap.h"
#include "rv_endian.h"

//  Decrypt rounds. Implements AES-128/192/256 depending on nr = {10,12,14}

void saes32_dec_rounds(uint8_t pt[16], const uint8_t ct[16],
					   const uint32_t rk[], int nr)
{
	uint32_t t0, t1, t2, t3;				//  even round state registers
	uint32_t u0, u1, u2, u3;				//  odd round state registers
	const uint32_t *kp = &rk[4 * nr];		//  key pointer

	t0 = kp[0];								//  fetch last subkey
	t1 = kp[1];
	t2 = kp[2];
	t3 = kp[3];
	kp -= 8;

	t0 ^= get32u_le(ct);					//  xor with ciphertext block
	t1 ^= get32u_le(ct + 4);
	t2 ^= get32u_le(ct + 8);
	t3 ^= get32u_le(ct + 12);

	while (1) {
		u0 = kp[4];							//  fetch odd subkey
		u1 = kp[5];
		u2 = kp[6];
		u3 = kp[7];

		u0 = saes32_decsm(u0, t0, 0);		//  AES decryption round, 16 instr
		u0 = saes32_decsm(u0, t3, 1);
		u0 = saes32_decsm(u0, t2, 2);
		u0 = saes32_decsm(u0, t1, 3);

		u1 = saes32_decsm(u1, t1, 0);
		u1 = saes32_decsm(u1, t0, 1);
		u1 = saes32_decsm(u1, t3, 2);
		u1 = saes32_decsm(u1, t2, 3);

		u2 = saes32_decsm(u2, t2, 0);
		u2 = saes32_decsm(u2, t1, 1);
		u2 = saes32_decsm(u2, t0, 2);
		u2 = saes32_decsm(u2, t3, 3);

		u3 = saes32_decsm(u3, t3, 0);
		u3 = saes32_decsm(u3, t2, 1);
		u3 = saes32_decsm(u3, t1, 2);
		u3 = saes32_decsm(u3, t0, 3);

		t0 = kp[0];							//  fetch even subkey
		t1 = kp[1];
		t2 = kp[2];
		t3 = kp[3];

		if (kp == rk)						//  final round
			break;
		kp -= 8;

		t0 = saes32_decsm(t0, u0, 0);		//  AES decryption round, 16 instr
		t0 = saes32_decsm(t0, u3, 1);
		t0 = saes32_decsm(t0, u2, 2);
		t0 = saes32_decsm(t0, u1, 3);

		t1 = saes32_decsm(t1, u1, 0);
		t1 = saes32_decsm(t1, u0, 1);
		t1 = saes32_decsm(t1, u3, 2);
		t1 = saes32_decsm(t1, u2, 3);

		t2 = saes32_decsm(t2, u2, 0);
		t2 = saes32_decsm(t2, u1, 1);
		t2 = saes32_decsm(t2, u0, 2);
		t2 = saes32_decsm(t2, u3, 3);

		t3 = saes32_decsm(t3, u3, 0);
		t3 = saes32_decsm(t3, u2, 1);
		t3 = saes32_decsm(t3, u1, 2);
		t3 = saes32_decsm(t3, u0, 3);
	}

	t0 = saes32_decs(t0, u0, 0);			//  final decryption round, 16 ins.
	t0 = saes32_decs(t0, u3, 1);
	t0 = saes32_decs(t0, u2, 2);
	t0 = saes32_decs(t0, u1, 3);

	t1 = saes32_decs(t1, u1, 0);
	t1 = saes32_decs(t1, u0, 1);
	t1 = saes32_decs(t1, u3, 2);
	t1 = saes32_decs(t1, u2, 3);

	t2 = saes32_decs(t2, u2, 0);
	t2 = saes32_decs(t2, u1, 1);
	t2 = saes32_decs(t2, u0, 2);
	t2 = saes32_decs(t2, u3, 3);

	t3 = saes32_decs(t3, u3, 0);
	t3 = saes32_decs(t3, u2, 1);
	t3 = saes32_decs(t3, u1, 2);
	t3 = saes32_decs(t3, u0, 3);

	put32u_le(pt, t0);						//  write plaintext block
	put32u_le(pt + 4, t1);
	put32u_le(pt + 8, t2);
	put32u_le(pt + 12, t3);
}

//  Helper: apply inverse mixcolumns to a vector
//  If decryption keys are computed in the fly (inverse key schedule), there's
//  no need for the encryption instruction (but you need final subkey).

void saes32_dec_invmc(uint32_t * v, size_t len)
{
	size_t i;
	uint32_t x, y;

	for (i = 0; i < len; i++) {
		x = v[i];

		y = saes32_encs(0, x, 0);			//  SubWord()
		y = saes32_encs(y, x, 1);
		y = saes32_encs(y, x, 2);
		y = saes32_encs(y, x, 3);

		x = saes32_decsm(0, y, 0);			//  Just want inv MixCol()
		x = saes32_decsm(x, y, 1);
		x = saes32_decsm(x, y, 2);
		x = saes32_decsm(x, y, 3);

		v[i] = x;
	}
}

//  Key schedule for AES-128 decryption.

void saes32_dec_key128(uint32_t rk[44], const uint8_t key[16])
{
	//  create an encryption key and modify middle rounds
	aes128_enc_key(rk, key);
	saes32_dec_invmc(rk + 4, AES128_RK_WORDS - 8);
}

//  Key schedule for AES-192 decryption.

void saes32_dec_key192(uint32_t rk[52], const uint8_t key[24])
{
	//  create an encryption key and modify middle rounds
	aes192_enc_key(rk, key);
	saes32_dec_invmc(rk + 4, AES192_RK_WORDS - 8);
}

//  Key schedule for AES-256 decryption.

void saes32_dec_key256(uint32_t rk[60], const uint8_t key[32])
{
	//  create an encryption key and modify middle rounds
	aes256_enc_key(rk, key);
	saes32_dec_invmc(rk + 4, AES256_RK_WORDS - 8);
}
