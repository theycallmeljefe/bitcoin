#include "sha256_shani.h"

#include <x86intrin.h>
#include <emmintrin.h>

// Based on http://software.intel.com/en-us/articles/intel-sha-extensions and code by Sean Gulley.
void sha256_shani(const uint8_t *data2, uint32_t *state, size_t length)
{
    __m128i STATE0, STATE1;
    __m128i MSG, TMP, MASK;
    __m128i TMSG0, TMSG1, TMSG2, TMSG3;
    __m128i ABEF_SAVE, CDGH_SAVE;
    uint32_t *data;
    data = (uint32_t*)data2;
    // Load initial hash values
    TMP = _mm_loadu_si128((__m128i*) &state[0]);
    STATE1 = _mm_loadu_si128((__m128i*) &state[4]);
    MASK = _mm_set_epi64x(0x0c0d0e0f08090a0bull, 0x0405060700010203ull);

    TMP = _mm_shuffle_epi32(TMP, 0xB1); // CDAB
    STATE1 = _mm_shuffle_epi32(STATE1, 0x1B); // EFGH
    STATE0 = _mm_alignr_epi8(TMP, STATE1, 8); // ABEF
    STATE1 = _mm_blend_epi16(STATE1, TMP, 0xF0); // CDGH
    length *= 64;
    while (length)
    {
        // Save hash values for addition after rounds
        ABEF_SAVE = STATE0;
        CDGH_SAVE = STATE1;

        // Rounds 0-3
        MSG = _mm_loadu_si128((__m128i*) data+0);
        TMSG0 = _mm_shuffle_epi8(MSG, MASK);
        MSG = _mm_add_epi32(TMSG0, _mm_set_epi64x(0xE9B5DBA5B5C0FBCFull, 0x71374491428A2F98ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Rounds 4-7
        TMSG1 = _mm_loadu_si128((__m128i*) (data+4));
        TMSG1 = _mm_shuffle_epi8(TMSG1, MASK);
        MSG = _mm_add_epi32(TMSG1, _mm_set_epi64x(0xAB1C5ED5923F82A4ull, 0x59F111F13956C25Bull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG0 = _mm_sha256msg1_epu32(TMSG0, TMSG1);

        // Rounds 8-11
        TMSG2 = _mm_loadu_si128((__m128i*) (data+8));
        TMSG2 = _mm_shuffle_epi8(TMSG2, MASK);
        MSG = _mm_add_epi32(TMSG2, _mm_set_epi64x(0x550C7DC3243185BEull, 0x12835B01D807AA98ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG1 = _mm_sha256msg1_epu32(TMSG1, TMSG2);

        // Rounds 12-15
        TMSG3 = _mm_loadu_si128((__m128i*) (data+12));
        TMSG3 = _mm_shuffle_epi8(TMSG3, MASK);
        MSG = _mm_add_epi32(TMSG3, _mm_set_epi64x(0xC19BF1749BDC06A7ull, 0x80DEB1FE72BE5D74ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG3, TMSG2, 4);
        TMSG0 = _mm_add_epi32(TMSG0, TMP);
        TMSG0 = _mm_sha256msg2_epu32(TMSG0, TMSG3);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG2 = _mm_sha256msg1_epu32(TMSG2, TMSG3);

        // Rounds 16-19
        MSG = _mm_add_epi32(TMSG0, _mm_set_epi64x(0x240CA1CC0FC19DC6ull, 0xEFBE4786E49B69C1ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG0, TMSG3, 4);
        TMSG1 = _mm_add_epi32(TMSG1, TMP);
        TMSG1 = _mm_sha256msg2_epu32(TMSG1, TMSG0);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG3 = _mm_sha256msg1_epu32(TMSG3, TMSG0);

        // Rounds 20-23
        MSG = _mm_add_epi32(TMSG1, _mm_set_epi64x(0x76F988DA5CB0A9DCull, 0x4A7484AA2DE92C6Full));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG1, TMSG0, 4);
        TMSG2 = _mm_add_epi32(TMSG2, TMP);
        TMSG2 = _mm_sha256msg2_epu32(TMSG2, TMSG1);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG0 = _mm_sha256msg1_epu32(TMSG0, TMSG1);

        // Rounds 24-27
        MSG = _mm_add_epi32(TMSG2, _mm_set_epi64x(0xBF597FC7B00327C8ull, 0xA831C66D983E5152ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG2, TMSG1, 4);
        TMSG3 = _mm_add_epi32(TMSG3, TMP);
        TMSG3 = _mm_sha256msg2_epu32(TMSG3, TMSG2);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG1 = _mm_sha256msg1_epu32(TMSG1, TMSG2);

        // Rounds 28-31
        MSG = _mm_add_epi32(TMSG3, _mm_set_epi64x(0x1429296706CA6351ull, 0xD5A79147C6E00BF3ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG3, TMSG2, 4);
        TMSG0 = _mm_add_epi32(TMSG0, TMP);
        TMSG0 = _mm_sha256msg2_epu32(TMSG0, TMSG3);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG2 = _mm_sha256msg1_epu32(TMSG2, TMSG3);

        // Rounds 32-35
        MSG = _mm_add_epi32(TMSG0, _mm_set_epi64x(0x53380D134D2C6DFCull, 0x2E1B213827B70A85ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG0, TMSG3, 4);
        TMSG1 = _mm_add_epi32(TMSG1, TMP);
        TMSG1 = _mm_sha256msg2_epu32(TMSG1, TMSG0);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG3 = _mm_sha256msg1_epu32(TMSG3, TMSG0);

        // Rounds 36-39
        MSG = _mm_add_epi32(TMSG1, _mm_set_epi64x(0x92722C8581C2C92Eull, 0x766A0ABB650A7354ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG1, TMSG0, 4);
        TMSG2 = _mm_add_epi32(TMSG2, TMP);
        TMSG2 = _mm_sha256msg2_epu32(TMSG2, TMSG1);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG0 = _mm_sha256msg1_epu32(TMSG0, TMSG1);

        // Rounds 40-43
        MSG = _mm_add_epi32(TMSG2, _mm_set_epi64x(0xC76C51A3C24B8B70ull, 0xA81A664BA2BFE8A1ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG2, TMSG1, 4);
        TMSG3 = _mm_add_epi32(TMSG3, TMP);
        TMSG3 = _mm_sha256msg2_epu32(TMSG3, TMSG2);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG1 = _mm_sha256msg1_epu32(TMSG1, TMSG2);

        // Rounds 44-47
        MSG = _mm_add_epi32(TMSG3, _mm_set_epi64x(0x106AA070F40E3585ull, 0xD6990624D192E819ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG3, TMSG2, 4);
        TMSG0 = _mm_add_epi32(TMSG0, TMP);
        TMSG0 = _mm_sha256msg2_epu32(TMSG0, TMSG3);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG2 = _mm_sha256msg1_epu32(TMSG2, TMSG3);

        // Rounds 48-51
        MSG = _mm_add_epi32(TMSG0, _mm_set_epi64x(0x34B0BCB52748774Cull, 0x1E376C0819A4C116ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG0, TMSG3, 4);
        TMSG1 = _mm_add_epi32(TMSG1, TMP);
        TMSG1 = _mm_sha256msg2_epu32(TMSG1, TMSG0);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        TMSG3 = _mm_sha256msg1_epu32(TMSG3, TMSG0);

        // Rounds 52-55
        MSG = _mm_add_epi32(TMSG1, _mm_set_epi64x(0x682E6FF35B9CCA4Full, 0x4ED8AA4A391C0CB3ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG1, TMSG0, 4);
        TMSG2 = _mm_add_epi32(TMSG2, TMP);
        TMSG2 = _mm_sha256msg2_epu32(TMSG2, TMSG1);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Rounds 56-59
        MSG = _mm_add_epi32(TMSG2, _mm_set_epi64x(0x8CC7020884C87814ull, 0x78A5636F748F82EEull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(TMSG2, TMSG1, 4);
        TMSG3 = _mm_add_epi32(TMSG3, TMP);
        TMSG3 = _mm_sha256msg2_epu32(TMSG3, TMSG2);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Rounds 60-63
        MSG = _mm_add_epi32(TMSG3, _mm_set_epi64x(0xC67178F2BEF9A3F7ull, 0xA4506CEB90BEFFFAull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Add current hash values with previously saved
        STATE0 = _mm_add_epi32(STATE0, ABEF_SAVE);
        STATE1 = _mm_add_epi32(STATE1, CDGH_SAVE);

        data += 16;
        length -= 64; 
    }

    // Write hash values back in the correct order
    TMP = _mm_shuffle_epi32(STATE0, 0x1B); // FEBA
    STATE1 = _mm_shuffle_epi32(STATE1, 0xB1); // DCHG
    STATE0 = _mm_blend_epi16(TMP, STATE1, 0xF0); // DCBA
    STATE1 = _mm_alignr_epi8(STATE1, TMP, 8); // ABEF

    _mm_storeu_si128((__m128i*) &state[0], STATE0);
    _mm_storeu_si128((__m128i*) &state[4], STATE1);
}
