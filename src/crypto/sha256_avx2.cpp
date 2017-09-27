#ifdef ENABLE_AVX2

#include <stdint.h>
#if defined(_MSC_VER)
#include <immintrin.h>
#elif defined(__GNUC__)
#include <x86intrin.h>
#endif

#include "crypto/common.h"

namespace sha256_avx {
namespace {

constexpr uint32_t sha256_consts[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, /*  0 */
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, /*  8 */
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, /* 16 */
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, /* 24 */
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, /* 32 */
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, /* 40 */
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, /* 48 */
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, /* 56 */
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline __attribute__((always_inline)) __m256i Ch(const __m256i b, const __m256i c, const __m256i d) {
    return _mm256_xor_si256(_mm256_and_si256(b,c), _mm256_andnot_si256(b,d));
}

inline __attribute__((always_inline)) __m256i Maj(const __m256i b, const __m256i c, const __m256i d) {
    return _mm256_xor_si256(_mm256_xor_si256(_mm256_and_si256(b,c), _mm256_and_si256(b,d)), _mm256_and_si256(c,d));
}

inline __attribute__((always_inline)) __m256i  ROTR(__m256i x, const int n) {
    return _mm256_or_si256(_mm256_srli_epi32(x, n), _mm256_slli_epi32(x, 32 - n));
}

inline  __attribute__((always_inline)) __m256i SHR(__m256i x, const int n) {
    return _mm256_srli_epi32(x, n);
}

inline __attribute__((always_inline)) __m256i Read8(const unsigned char** chunk, int offset) {
    return _mm256_set_epi32(ReadBE32(chunk[0] + offset), ReadBE32(chunk[1] + offset), ReadBE32(chunk[2] + offset), ReadBE32(chunk[3] + offset), ReadBE32(chunk[4] + offset), ReadBE32(chunk[5] + offset), ReadBE32(chunk[6] + offset), ReadBE32(chunk[7] + offset));
}

inline __attribute__((always_inline)) __m256i Load8(const uint32_t* s) {
    return _mm256_set_epi32(s[0], s[8], s[16], s[24], s[32], s[40], s[48], s[56]);
}

void inline __attribute__((always_inline)) Write8(unsigned char** out, int offset, __m256i v) {
    WriteBE32(out[0] + offset, _mm256_extract_epi32(v, 0));
    WriteBE32(out[1] + offset, _mm256_extract_epi32(v, 1));
    WriteBE32(out[2] + offset, _mm256_extract_epi32(v, 2));
    WriteBE32(out[3] + offset, _mm256_extract_epi32(v, 3));
    WriteBE32(out[4] + offset, _mm256_extract_epi32(v, 4));
    WriteBE32(out[5] + offset, _mm256_extract_epi32(v, 5));
    WriteBE32(out[6] + offset, _mm256_extract_epi32(v, 6));
    WriteBE32(out[7] + offset, _mm256_extract_epi32(v, 7));
}

void inline __attribute__((always_inline)) Store8(uint32_t* s, __m256i v) {
    s[0] = _mm256_extract_epi32(v, 0);
    s[8] = _mm256_extract_epi32(v, 1);
    s[16] = _mm256_extract_epi32(v, 2);
    s[24] = _mm256_extract_epi32(v, 3);
    s[32] = _mm256_extract_epi32(v, 4);
    s[40] = _mm256_extract_epi32(v, 5);
    s[48] = _mm256_extract_epi32(v, 6);
    s[56] = _mm256_extract_epi32(v, 7);
}

}
/* SHA256 Functions */
#define BIGSIGMA0_256(x)    (_mm256_xor_si256(_mm256_xor_si256(ROTR((x), 2),ROTR((x), 13)),ROTR((x), 22)))
#define BIGSIGMA1_256(x)    (_mm256_xor_si256(_mm256_xor_si256(ROTR((x), 6),ROTR((x), 11)),ROTR((x), 25)))


#define SIGMA0_256(x)       (_mm256_xor_si256(_mm256_xor_si256(ROTR((x), 7),ROTR((x), 18)), SHR((x), 3 )))
#define SIGMA1_256(x)       (_mm256_xor_si256(_mm256_xor_si256(ROTR((x),17),ROTR((x), 19)), SHR((x), 10)))

#define add4(x0, x1, x2, x3) _mm256_add_epi32(_mm256_add_epi32(x0, x1),_mm256_add_epi32( x2,x3))
#define add5(x0, x1, x2, x3, x4) _mm256_add_epi32(add4(x0, x1, x2, x3), x4)

#define SHA256ROUND(a, b, c, d, e, f, g, h, i, w)                       \
    T1 = add5(h, BIGSIGMA1_256(e), Ch(e, f, g), _mm256_set1_epi32(sha256_consts[i]), w);   \
    d = _mm256_add_epi32(d, T1);                                           \
    h = _mm256_add_epi32(T1, _mm256_add_epi32(BIGSIGMA0_256(a), Maj(a, b, c)));



void Transform_8way(uint32_t* s, const unsigned char** in) {
    __m256i a = Load8(s), b = Load8(s + 1), c = Load8(s + 2), d = Load8(s + 3), e = Load8(s + 4), f = Load8(s + 5), g = Load8(s + 6), h = Load8(s + 7);
    __m256i t0 = a, t1 = b, t2 = c, t3 = d, t4 = e, t5 = f, t6 = g, t7 = h;
    __m256i T1;

    __m256i w0 = Read8(in, 0);
    SHA256ROUND(a, b, c, d, e, f, g, h, 0, w0);
    __m256i w1 = Read8(in, 4);
    SHA256ROUND(h, a, b, c, d, e, f, g, 1, w1);
    __m256i w2 = Read8(in, 8);
    SHA256ROUND(g, h, a, b, c, d, e, f, 2, w2);
    __m256i w3 = Read8(in, 12);
    SHA256ROUND(f, g, h, a, b, c, d, e, 3, w3);
    __m256i w4 = Read8(in, 16);
    SHA256ROUND(e, f, g, h, a, b, c, d, 4, w4);
    __m256i w5 = Read8(in, 20);
    SHA256ROUND(d, e, f, g, h, a, b, c, 5, w5);
    __m256i w6 = Read8(in, 24);
    SHA256ROUND(c, d, e, f, g, h, a, b, 6, w6);
    __m256i w7 = Read8(in, 28);
    SHA256ROUND(b, c, d, e, f, g, h, a, 7, w7);
    __m256i w8 = Read8(in, 32);
    SHA256ROUND(a, b, c, d, e, f, g, h, 8, w8);
    __m256i w9 = Read8(in, 36);
    SHA256ROUND(h, a, b, c, d, e, f, g, 9, w9);
    __m256i w10 = Read8(in, 40);
    SHA256ROUND(g, h, a, b, c, d, e, f, 10, w10);
    __m256i w11 = Read8(in, 44);
    SHA256ROUND(f, g, h, a, b, c, d, e, 11, w11);
    __m256i w12 = Read8(in, 48);
    SHA256ROUND(e, f, g, h, a, b, c, d, 12, w12);
    __m256i w13 = Read8(in, 52);
    SHA256ROUND(d, e, f, g, h, a, b, c, 13, w13);
    __m256i w14 = Read8(in, 56);
    SHA256ROUND(c, d, e, f, g, h, a, b, 14, w14);
    __m256i w15 = Read8(in, 60);
    SHA256ROUND(b, c, d, e, f, g, h, a, 15, w15);

    w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
    SHA256ROUND(a, b, c, d, e, f, g, h, 16, w0);
    w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
    SHA256ROUND(h, a, b, c, d, e, f, g, 17, w1);
    w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
    SHA256ROUND(g, h, a, b, c, d, e, f, 18, w2);
    w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
    SHA256ROUND(f, g, h, a, b, c, d, e, 19, w3);
    w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
    SHA256ROUND(e, f, g, h, a, b, c, d, 20, w4);
    w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
    SHA256ROUND(d, e, f, g, h, a, b, c, 21, w5);
    w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
    SHA256ROUND(c, d, e, f, g, h, a, b, 22, w6);
    w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
    SHA256ROUND(b, c, d, e, f, g, h, a, 23, w7);
    w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
    SHA256ROUND(a, b, c, d, e, f, g, h, 24, w8);
    w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
    SHA256ROUND(h, a, b, c, d, e, f, g, 25, w9);
    w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
    SHA256ROUND(g, h, a, b, c, d, e, f, 26, w10);
    w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
    SHA256ROUND(f, g, h, a, b, c, d, e, 27, w11);
    w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
    SHA256ROUND(e, f, g, h, a, b, c, d, 28, w12);
    w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
    SHA256ROUND(d, e, f, g, h, a, b, c, 29, w13);
    w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
    SHA256ROUND(c, d, e, f, g, h, a, b, 30, w14);
    w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
    SHA256ROUND(b, c, d, e, f, g, h, a, 31, w15);

    w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
    SHA256ROUND(a, b, c, d, e, f, g, h, 32, w0);
    w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
    SHA256ROUND(h, a, b, c, d, e, f, g, 33, w1);
    w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
    SHA256ROUND(g, h, a, b, c, d, e, f, 34, w2);
    w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
    SHA256ROUND(f, g, h, a, b, c, d, e, 35, w3);
    w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
    SHA256ROUND(e, f, g, h, a, b, c, d, 36, w4);
    w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
    SHA256ROUND(d, e, f, g, h, a, b, c, 37, w5);
    w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
    SHA256ROUND(c, d, e, f, g, h, a, b, 38, w6);
    w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
    SHA256ROUND(b, c, d, e, f, g, h, a, 39, w7);
    w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
    SHA256ROUND(a, b, c, d, e, f, g, h, 40, w8);
    w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
    SHA256ROUND(h, a, b, c, d, e, f, g, 41, w9);
    w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
    SHA256ROUND(g, h, a, b, c, d, e, f, 42, w10);
    w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
    SHA256ROUND(f, g, h, a, b, c, d, e, 43, w11);
    w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
    SHA256ROUND(e, f, g, h, a, b, c, d, 44, w12);
    w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
    SHA256ROUND(d, e, f, g, h, a, b, c, 45, w13);
    w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
    SHA256ROUND(c, d, e, f, g, h, a, b, 46, w14);
    w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
    SHA256ROUND(b, c, d, e, f, g, h, a, 47, w15);

    w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
    SHA256ROUND(a, b, c, d, e, f, g, h, 48, w0);
    w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
    SHA256ROUND(h, a, b, c, d, e, f, g, 49, w1);
    w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
    SHA256ROUND(g, h, a, b, c, d, e, f, 50, w2);
    w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
    SHA256ROUND(f, g, h, a, b, c, d, e, 51, w3);
    w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
    SHA256ROUND(e, f, g, h, a, b, c, d, 52, w4);
    w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
    SHA256ROUND(d, e, f, g, h, a, b, c, 53, w5);
    w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
    SHA256ROUND(c, d, e, f, g, h, a, b, 54, w6);
    w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
    SHA256ROUND(b, c, d, e, f, g, h, a, 55, w7);
    w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
    SHA256ROUND(a, b, c, d, e, f, g, h, 56, w8);
    w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
    SHA256ROUND(h, a, b, c, d, e, f, g, 57, w9);
    w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
    SHA256ROUND(g, h, a, b, c, d, e, f, 58, w10);
    w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
    SHA256ROUND(f, g, h, a, b, c, d, e, 59, w11);
    w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
    SHA256ROUND(e, f, g, h, a, b, c, d, 60, w12);
    w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
    SHA256ROUND(d, e, f, g, h, a, b, c, 61, w13);
    w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
    SHA256ROUND(c, d, e, f, g, h, a, b, 62, w14);
    w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
    SHA256ROUND(b, c, d, e, f, g, h, a, 63, w15);

    Store8(s + 0, _mm256_add_epi32(a, t0));
    Store8(s + 1, _mm256_add_epi32(b, t1));
    Store8(s + 2, _mm256_add_epi32(c, t2));
    Store8(s + 3, _mm256_add_epi32(d, t3));
    Store8(s + 4, _mm256_add_epi32(e, t4));
    Store8(s + 5, _mm256_add_epi32(f, t5));
    Store8(s + 6, _mm256_add_epi32(g, t6));
    Store8(s + 7, _mm256_add_epi32(h, t7));
}

}

#endif
