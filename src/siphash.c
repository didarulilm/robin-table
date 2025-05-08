/*
 * SipHash reference C implementation
 *
 * Copyright (c) 2016 Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * default: SipHash-2-4
 */

#include <stddef.h>
#include <stdint.h>

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define SIPROUND                                                                         \
    do {                                                                                 \
        v0 += v1;                                                                        \
        v1 = ROTL(v1, 13);                                                               \
        v1 ^= v0;                                                                        \
        v0 = ROTL(v0, 32);                                                               \
        v2 += v3;                                                                        \
        v3 = ROTL(v3, 16);                                                               \
        v3 ^= v2;                                                                        \
        v0 += v3;                                                                        \
        v3 = ROTL(v3, 21);                                                               \
        v3 ^= v0;                                                                        \
        v2 += v1;                                                                        \
        v1 = ROTL(v1, 17);                                                               \
        v1 ^= v2;                                                                        \
    } while (0)

static uint64_t siphash(const uint8_t* in, size_t len, uint64_t seed)
{
    uint64_t v0 = 0x736f6d6570736575ULL ^ seed;
    uint64_t v1 = 0x646f72616e646f6dULL ^ (seed >> 32);
    uint64_t v2 = 0x6c7967656e657261ULL ^ seed;
    uint64_t v3 = 0x7465646279746573ULL ^ (seed >> 32);
    uint64_t m;
    const uint8_t* end = in + len - (len % sizeof(uint64_t));
    const uint8_t* curr = in;

    for (; curr != end; curr += 8) {
        m = ((uint64_t)curr[0]) | ((uint64_t)curr[1] << 8) | ((uint64_t)curr[2] << 16) |
            ((uint64_t)curr[3] << 24) | ((uint64_t)curr[4] << 32) |
            ((uint64_t)curr[5] << 40) | ((uint64_t)curr[6] << 48) |
            ((uint64_t)curr[7] << 56);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }

    uint64_t b = len << 56;
    switch (len % 8) {
    case 7:
        b |= ((uint64_t)curr[6]) << 48;
        /* fall through */
    case 6:
        b |= ((uint64_t)curr[5]) << 40;
        /* fall through */
    case 5:
        b |= ((uint64_t)curr[4]) << 32;
        /* fall through */
    case 4:
        b |= ((uint64_t)curr[3]) << 24;
        /* fall through */
    case 3:
        b |= ((uint64_t)curr[2]) << 16;
        /* fall through */
    case 2:
        b |= ((uint64_t)curr[1]) << 8;
        /* fall through */
    case 1:
        b |= ((uint64_t)curr[0]);
    }

    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;

    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t robin_table_siphash(const void* key, size_t klen, uint64_t seed)
{
    return siphash((const uint8_t*)key, klen, seed);
}
