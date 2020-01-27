//  main.c
//  2020-01-23  Markku-Juhani O. Saarinen <mjos@pqshield.com>
//  Copyright (c) 2020, PQShield Ltd. All rights reserved.

//  Minimal unit tests for AES-128/192/256 (FIPS 197) and SM4 (GM/T 0002-2012).

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//  prototypes for high-level primitives

#include "aes_enc.h"
#include "aes_dec.h"
#include "sm4_encdec.h"

//  test helper: load a hex string

size_t sethex(uint8_t * v, size_t maxlen, const char *str)
{
    size_t i;
    unsigned x;

    for (i = 0; i < maxlen; i++) {
        if (str[2 * i] == 0 || sscanf(&str[2 * i], "%02X", &x) == 0)
            break;
        v[i] = x;
    }

    return i;
}

//  test helper: check against a test vector

int chkhex(const char *lab, const void *data, size_t len, const char *ref)
{
    size_t i;
    char x, buf[2 * len + 1];
    const char hex[] = "0123456789ABCDEF";

    for (i = 0; i < len; i++) {
        x = ((const uint8_t *) data)[i];
        buf[2 * i] = hex[(x >> 4) & 0xF];
        buf[2 * i + 1] = hex[x & 0xF];
    }
    buf[2 * len] = 0;

    if (ref != NULL) {
        if (strcasecmp(ref, buf) == 0) {
            printf("[PASS] %s %s\n", lab, buf);
            return 0;
        } else {
            printf("[FAIL] %s %s (%s)\n", lab, buf, ref);
            return 1;
        }
    }

    printf("[TEST] %s %s\n", lab, buf);

    return 0;
}

//  Test AES

int test_aes()
{
    uint8_t pt[16] = { 0 }, ct[16] = { 0 }, xt[16] = { 0 }, key[32] = { 0};
    uint32_t rk[AES256_RK_WORDS];
    int fail = 0;

    sethex(pt, sizeof(pt), "00112233445566778899AABBCCDDEEFF");
    sethex(key, sizeof(key),
        "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F");

    //  FIPS 197 test vectors
    aes128_enc_key(rk, key);
    aes128_enc_ecb(ct, pt, rk);
    fail += chkhex("AES-128 Enc", ct, 16, "69C4E0D86A7B0430D8CDB78070B4C55A");
    aes128_dec_key(rk, key);
    aes128_dec_ecb(xt, ct, rk);
    fail += chkhex("AES-128 Dec", xt, 16, "00112233445566778899AABBCCDDEEFF");

    aes192_enc_key(rk, key);
    aes192_enc_ecb(ct, pt, rk);
    fail += chkhex("AES-192 Enc", ct, 16, "DDA97CA4864CDFE06EAF70A0EC0D7191");
    aes192_dec_key(rk, key);
    aes192_dec_ecb(xt, ct, rk);
    fail += chkhex("AES-192 Dec", xt, 16, "00112233445566778899AABBCCDDEEFF");

    aes256_enc_key(rk, key);
    aes256_enc_ecb(ct, pt, rk);
    fail += chkhex("AES-256 Enc", ct, 16, "8EA2B7CA516745BFEAFC49904B496089");
    aes256_dec_key(rk, key);
    aes256_dec_ecb(xt, ct, rk);
    fail += chkhex("AES-256 Dec", xt, 16, "00112233445566778899AABBCCDDEEFF");

    //  another test vector set (picked from SP800-38A)
    sethex(key, sizeof(key), "2B7E151628AED2A6ABF7158809CF4F3C");
    aes128_enc_key(rk, key);
    sethex(pt, sizeof(pt), "6BC1BEE22E409F96E93D7E117393172A");
    aes128_enc_ecb(ct, pt, rk);
    fail += chkhex("AES-128 Enc", ct, 16, "3AD77BB40D7A3660A89ECAF32466EF97");
    aes128_dec_key(rk, key);
    aes128_dec_ecb(xt, ct, rk);
    fail += chkhex("AES-128 Dec", xt, 16, "6BC1BEE22E409F96E93D7E117393172A");

    sethex(key, sizeof(key),
        "8E73B0F7DA0E6452C810F32B809079E562F8EAD2522C6B7B");
    aes192_enc_key(rk, key);
    sethex(pt, sizeof(pt), "AE2D8A571E03AC9C9EB76FAC45AF8E51");
    aes192_enc_ecb(ct, pt, rk);
    fail += chkhex("AES-192 Enc", ct, 16, "974104846D0AD3AD7734ECB3ECEE4EEF");
    aes192_dec_key(rk, key);
    aes192_dec_ecb(xt, ct, rk);
    fail += chkhex("AES-192 Dec", xt, 16, "AE2D8A571E03AC9C9EB76FAC45AF8E51");

    sethex(key, sizeof(key),
        "603DEB1015CA71BE2B73AEF0857D77811F352C073B6108D72D9810A30914DFF4");
    aes256_enc_key(rk, key);
    sethex(pt, sizeof(pt), "30C81C46A35CE411E5FBC1191A0A52EF");
    aes256_enc_ecb(ct, pt, rk);
    fail += chkhex("AES-256 Enc", ct, 16, "B6ED21B99CA6F4F9F153E7B1BEAFED1D");
    aes256_dec_key(rk, key);
    aes256_dec_ecb(xt, ct, rk);
    fail += chkhex("AES-256 Dec", xt, 16, "30C81C46A35CE411E5FBC1191A0A52EF");

    return fail;
}

//  Test SM4

int test_sm4()
{
    uint8_t pt[16], ct[16], xt[16], key[16];
    uint32_t rk[SM4_RK_WORDS];
    int fail = 0;

    //  The standard test vector.
    sethex(key, sizeof(key), "0123456789ABCDEFFEDCBA9876543210");
    sm4_enc_key(rk, key);
    sethex(pt, sizeof(pt), "0123456789ABCDEFFEDCBA9876543210");
    sm4_enc_ecb(ct, pt, rk);
    fail += chkhex("SM4 Encrypt", ct, 16, "681EDF34D206965E86B3E94F536E4246");
    sm4_dec_key(rk, key);
    sm4_enc_ecb(xt, ct, rk);
    fail += chkhex("SM4 Decrypt", xt, 16, "0123456789ABCDEFFEDCBA9876543210");

    //  From various sources.
    sethex(key, sizeof(key), "FEDCBA98765432100123456789ABCDEF");
    sm4_enc_key(rk, key);
    sethex(pt, sizeof(pt), "000102030405060708090A0B0C0D0E0F");
    sm4_enc_ecb(ct, pt, rk);
    fail += chkhex("SM4 Encrypt", ct, 16, "F766678F13F01ADEAC1B3EA955ADB594");
    sm4_dec_key(rk, key);
    sm4_dec_ecb(xt, ct, rk);
    fail += chkhex("SM4 Decrypt", xt, 16, "000102030405060708090A0B0C0D0E0F");

    sethex(key, sizeof(key), "EB23ADD6454757555747395B76661C9A");
    sm4_enc_key(rk, key);
    sethex(pt, sizeof(pt),  "D294D879A1F02C7C5906D6C2D0C54D9F");
    sm4_enc_ecb(ct, pt, rk);
    fail += chkhex("SM4 Encrypt", ct, 16, "865DE90D6B6E99273E2D44859D9C16DF");
    sm4_dec_key(rk, key);
    sm4_dec_ecb(xt, ct, rk);
    fail += chkhex("SM4 Decrypt", xt, 16, "D294D879A1F02C7C5906D6C2D0C54D9F");

    sethex(key, sizeof(key), "F11235535318FA844A3CBE643169F59E");
    sm4_enc_key(rk, key);
    sethex(pt, sizeof(pt), "A27EE076E48E6F389710EC7B5E8A3BE5");
    sm4_enc_ecb(ct, pt, rk);
    fail += chkhex("SM4 Encrypt", ct, 16, "94CFE3F59E8507FEC41DBE738CCD53E1");
    sm4_dec_key(rk, key);
    sm4_dec_ecb(xt, ct, rk);
    fail += chkhex("SM4 Decrypt", xt, 16, "A27EE076E48E6F389710EC7B5E8A3BE5");

    return fail;
}

//  stub main: run unit tests

int main(int argc, char **argv)
{
    int fail = 0;

    fail += test_aes();
    fail += test_sm4();

    if (fail == 0) {
        printf("[PASS] all tests passed.\n");
    } else {
        printf("[FAIL] %d test(s) failed.\n", fail);
    }

    return fail;
}

