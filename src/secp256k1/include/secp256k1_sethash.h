#ifndef SECP256K1_SETHASH_H
#define SECP256K1_SETHASH_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char data[96];
} secp256k1_sethash;

SECP256K1_API void secp256k1_sethash_init(const secp256k1_context* ctx, secp256k1_sethash* sethash);
SECP256K1_API void secp256k1_sethash_update(const secp256k1_context* ctx, secp256k1_sethash* sethash, const unsigned char* data32, int remove);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SETHASH_H */
