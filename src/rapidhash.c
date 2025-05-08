/*
 * rapidhash - Very fast, high quality, platform-independent hashing algorithm.
 * Copyright (C) 2024 Nicolas De Carli
 *
 * Based on 'wyhash', by Wang Yi <godspeed_china@yeah.net>
 *
 * BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * You can contact the author at:
 * - rapidhash source repository: https://github.com/Nicoshev/rapidhash
 */

#include <stdint.h>
#include <string.h>

#define _likely_(x) __builtin_expect(!!(x), 1)
#define _unlikely_(x) __builtin_expect(!!(x), 0)

static const uint64_t rapid_secret[3] = {0x2d358dccaa6c78a5ULL, 0x8bb84b93962eacc9ULL,
                                         0x4b33a62ed433d4a3ULL};

typedef struct {
    uint64_t low;
    uint64_t high;
} uint128_result;

static inline uint128_result rapid_mul128(uint64_t A, uint64_t B)
{
    uint128_result result;

#if defined(__SIZEOF_INT128__)
    __uint128_t r = (__uint128_t)A * B;
    result.low = (uint64_t)r;
    result.high = (uint64_t)(r >> 64);
#else
    uint64_t a_high = A >> 32;
    uint64_t b_high = B >> 32;
    uint64_t a_low = (uint32_t)A;
    uint64_t b_low = (uint32_t)B;

    uint64_t result_high = a_high * b_high;
    uint64_t result_m0 = a_high * b_low;
    uint64_t result_m1 = b_high * a_low;
    uint64_t result_low = a_low * b_low;

    uint64_t high = result_high + (result_m0 >> 32) + (result_m1 >> 32);
    uint64_t t = result_low + (result_m0 << 32);
    high += (t < result_low);
    uint64_t low = t + (result_m1 << 32);
    high += (low < t);

    result.low = low;
    result.high = high;
#endif
    return result;
}

static inline uint64_t rapid_mix(uint64_t A, uint64_t B)
{
    uint128_result result = rapid_mul128(A, B);
    return result.low ^ result.high;
}

static inline uint64_t rapid_read64(const uint8_t* p)
{
    uint64_t v;
    memcpy(&v, p, sizeof(uint64_t));
    return v;
}

static inline uint64_t rapid_read32(const uint8_t* p)
{
    uint32_t v;
    memcpy(&v, p, sizeof(uint32_t));
    return v;
}

static inline uint64_t rapid_readSmall(const uint8_t* p, size_t k)
{
    return (((uint64_t)p[0]) << 56) | (((uint64_t)p[k >> 1]) << 32) | p[k - 1];
}

static inline uint64_t rapidhash_internal(const uint8_t* p, const size_t len,
                                          uint64_t seed, const uint64_t secret[3])
{
    seed ^= rapid_mix(seed ^ secret[0], secret[1]) ^ len;
    uint64_t a, b;
    if (_likely_(len <= 16)) {
        if (_likely_(len >= 4)) {
            const uint8_t* plast = p + len - 4;
            a = (rapid_read32(p) << 32) | rapid_read32(plast);

            const uint64_t delta = ((len & 24) >> (len >> 3));
            b = ((rapid_read32(p + delta) << 32) | rapid_read32(plast - delta));
        } else if (_likely_(len > 0)) {
            a = rapid_readSmall(p, len);
            b = 0;
        } else {
            a = b = 0;
        }
    } else {
        size_t i = len;
        if (_unlikely_(i > 48)) {
            uint64_t see1 = seed, see2 = seed;
            do {
                seed = rapid_mix(rapid_read64(p) ^ secret[0], rapid_read64(p + 8) ^ seed);
                see1 = rapid_mix(rapid_read64(p + 16) ^ secret[1],
                                 rapid_read64(p + 24) ^ see1);
                see2 = rapid_mix(rapid_read64(p + 32) ^ secret[2],
                                 rapid_read64(p + 40) ^ see2);
                p += 48;
                i -= 48;
            } while (_likely_(i >= 48));
            seed ^= see1 ^ see2;
        }
        if (i > 16) {
            seed = rapid_mix(rapid_read64(p) ^ secret[2],
                             rapid_read64(p + 8) ^ seed ^ secret[1]);
            if (i > 32) {
                seed = rapid_mix(rapid_read64(p + 16) ^ secret[2],
                                 rapid_read64(p + 24) ^ seed);
            }
        }
        a = rapid_read64(p + i - 16);
        b = rapid_read64(p + i - 8);
    }
    a ^= secret[1];
    b ^= seed;
    uint128_result mul_result = rapid_mul128(a, b);
    return rapid_mix(mul_result.low ^ secret[0] ^ len, mul_result.high ^ secret[1]);
}

static inline uint64_t rapidhash(const uint8_t* key, size_t len, uint64_t seed)
{
    return rapidhash_internal(key, len, seed, rapid_secret);
}

uint64_t robin_table_rapidhash(const void* key, size_t klen, uint64_t seed)
{
    return rapidhash((const uint8_t*)key, klen, seed);
}
