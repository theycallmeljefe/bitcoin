/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_SETHASH_MAIN_H
#define SECP256K1_MODULE_SETHASH_MAIN_H

#include "include/secp256k1_sethash.h"

#define ROTL32(x,n) ((x) << (n) | (x) >> (32-(n)))
#define QUARTERROUND(a,b,c,d) \
  a += b; d = ROTL32(d ^ a, 16); \
  c += d; b = ROTL32(b ^ c, 12); \
  a += b; d = ROTL32(d ^ a, 8); \
  c += d; b = ROTL32(b ^ c, 7);

#ifdef WORDS_BIGENDIAN
#define LE32(x) ((((p) & 0xFF) << 24) | (((p) & 0xFF00) << 8) | (((p) & 0xFF0000) >> 8) | (((p) & 0xFF000000) >> 24))
#else
#define LE32(p) (x)
#endif

static void chacha_256(unsigned char* out32, const unsigned char* seed32, uint32_t n) {
    uint32_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    int i;

    x0 = 0x61707865;
    x1 = 0x3320646e;
    x2 = 0x79622d32;
    x3 = 0x6b206574;
    x4 = *(const uint32_t*)(seed32);
    x5 = *(const uint32_t*)(seed32 + 4);
    x6 = *(const uint32_t*)(seed32 + 8);
    x7 = *(const uint32_t*)(seed32 + 12);
    x8 = *(const uint32_t*)(seed32 + 16);
    x9 = *(const uint32_t*)(seed32 + 20);
    x10 = *(const uint32_t*)(seed32 + 24);
    x11 = *(const uint32_t*)(seed32 + 28);
    x12 = n;
    x13 = 0;
    x14 = 0;
    x15 = 0;

    QUARTERROUND( x0, x4, x8,x12)
    QUARTERROUND( x1, x5, x9,x13)
    QUARTERROUND( x2, x6,x10,x14)
    QUARTERROUND( x3, x7,x11,x15)
    QUARTERROUND( x0, x5,x10,x15)
    QUARTERROUND( x1, x6,x11,x12)
    QUARTERROUND( x2, x7, x8,x13)
    QUARTERROUND( x3, x4, x9,x14)

    for (i = 8; i > 0; --i) {
        QUARTERROUND( x0, x4, x8,x12)
        QUARTERROUND( x1, x5, x9,x13)
        QUARTERROUND( x2, x6,x10,x14)
        QUARTERROUND( x3, x7,x11,x15)
        QUARTERROUND( x0, x5,x10,x15)
        QUARTERROUND( x1, x6,x11,x12)
        QUARTERROUND( x2, x7, x8,x13)
        QUARTERROUND( x3, x4, x9,x14)
    }

    QUARTERROUND( x0, x4, x8,x12)
    QUARTERROUND( x1, x5, x9,x13)
    QUARTERROUND( x2, x6,x10,x14)
    QUARTERROUND( x3, x7,x11,x15)
    QUARTERROUND( x0, x5,x10,x15)
    QUARTERROUND( x1, x6,x11,x12)
    QUARTERROUND( x2, x7, x8,x13)
    QUARTERROUND( x3, x4, x9,x14)

    *(uint32_t*)(out32) = x0 + 0x61707865;
    *(uint32_t*)(out32 + 4) = x1 + 0x3320646e;
    *(uint32_t*)(out32 + 8) = x2 + 0x79622d32;
    *(uint32_t*)(out32 + 12) = x3 + 0x6b206574;
    *(uint32_t*)(out32 + 16) = x12 + n;
    *(uint32_t*)(out32 + 20) = x13;
    *(uint32_t*)(out32 + 24) = x14;
    *(uint32_t*)(out32 + 28) = x15;
}

static int secp256k1_sethash_load(const secp256k1_context* ctx, secp256k1_gej* gej, const secp256k1_sethash* sethash) {
    if (sizeof(secp256k1_fe_storage) == 32) {
        /* When the secp256k1_fe_storage type is exactly 32 byte, use its
         * representation inside secp256k1_pubkey, as conversion is very fast.
         * Note that secp256k1_pubkey_save must use the same representation. */
        secp256k1_fe_storage s;
        memcpy(&s, &sethash->data[0], 32);
        secp256k1_fe_from_storage(&gej->x, &s);
        memcpy(&s, &sethash->data[32], 32);
        secp256k1_fe_from_storage(&gej->y, &s);
        memcpy(&s, &sethash->data[64], 32);
        secp256k1_fe_from_storage(&gej->z, &s);
        gej->infinity = 0;
    } else {
        /* Otherwise, fall back to 32-byte big endian for X and Y. */
        secp256k1_fe_set_b32(&gej->x, sethash->data);
        secp256k1_fe_set_b32(&gej->y, sethash->data + 32);
        secp256k1_fe_set_b32(&gej->z, sethash->data + 64);
        gej->infinity = 0;
    }
    ARG_CHECK(!secp256k1_fe_is_zero(&gej->x));
    return 1;
}

static void secp256k1_sethash_save(secp256k1_sethash* sethash, secp256k1_gej* gej) {
    if (sizeof(secp256k1_fe_storage) == 32) {
        secp256k1_fe_storage s;
        secp256k1_fe_to_storage(&s, &gej->x);
        memcpy(&sethash->data[0], &s, 32);
        secp256k1_fe_to_storage(&s, &gej->y);
        memcpy(&sethash->data[32], &s, 32);
        secp256k1_fe_to_storage(&s, &gej->z);
        memcpy(&sethash->data[64], &s, 32);
    } else {
        VERIFY_CHECK(!secp256k1_gej_is_infinity(gej));
        secp256k1_fe_normalize_var(&gej->x);
        secp256k1_fe_get_b32(sethash->data, &gej->x);
        secp256k1_fe_normalize_var(&gej->y);
        secp256k1_fe_get_b32(sethash->data + 32, &gej->y);
        secp256k1_fe_normalize_var(&gej->z);
        secp256k1_fe_get_b32(sethash->data + 64, &gej->z);
    }
}

static const secp256k1_fe INIT_Y = SECP256K1_FE_CONST(0x4218f20a, 0xe6c646b3, 0x63db6860, 0x5822fb14, 0x264ca8d2, 0x587fdd6f, 0xbc750d58, 0x7e76a7ee);

void secp256k1_sethash_init(const secp256k1_context* ctx, secp256k1_sethash* sethash) {
    secp256k1_gej init;
    secp256k1_fe_set_int(&init.x, 1);
    secp256k1_fe_set_int(&init.z, 1);
    (void)ctx;
    init.y = INIT_Y;
    secp256k1_sethash_save(sethash, &init);
}

void secp256k1_sethash_update(const secp256k1_context* ctx, secp256k1_sethash* sethash, const unsigned char* data32, int remove) {
    secp256k1_gej state;
    secp256k1_ge ge;
    secp256k1_fe x;
    uint32_t i = 0;

    (void)ctx;
    secp256k1_sethash_load(ctx, &state, sethash);

    secp256k1_fe_set_b32(&x, data32);

    do {
        unsigned char r[32];

        if (secp256k1_ge_set_xquad_var(&ge, &x)) break;
        chacha_256(r, data32, i++);
        secp256k1_fe_set_b32(&x, r);
    } while(1);

    if (remove) secp256k1_ge_neg(&ge, &ge);
    secp256k1_gej_add_ge_var(&state, &state, &ge, NULL);

    secp256k1_sethash_save(sethash, &state);
}

#endif /* SECP256K1_MODULE_SETHASH_MAIN_H */
