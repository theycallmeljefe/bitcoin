#ifndef BITCOIN_CRYPTO_SHA256_SHANI
#define BITCOIN_CRYPTO_SHA256_SHANI 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void sha256_shani(const uint8_t *data2, uint32_t *state, size_t length);
#ifdef __cplusplus
}
#endif

#endif
