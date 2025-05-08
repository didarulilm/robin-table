/*
 * MIT License
 *
 * Copyright (c) 2025 Didarul Islam <didarulislam85@hotmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ROBIN_TABLE_H
#define ROBIN_TABLE_H

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Define RT_NO_ASSERT to disable all internal assertions.
 */
/* #define RT_NO_ASSERT */

/*
 * Default 64-bit seed value for the rapidhash hash function.
 */
#define RT_RAPID_SEED    0xbdd89aa982704029ULL

typedef struct robin_table_t robin_table_t;

robin_table_t* robin_table_create(size_t count,
                                  uint64_t (*hash_func)(const void*, size_t, uint64_t),
                                  uint64_t seed);
void robin_table_destroy(robin_table_t* rt);

void* robin_table_put(robin_table_t* rt, const void* key, size_t klen, void* val);
void* robin_table_get(robin_table_t* rt, const void* key, size_t klen);
void* robin_table_del(robin_table_t* rt, const void* key, size_t klen);

bool robin_table_clear(robin_table_t* rt, bool update_buckets);
size_t robin_table_count(const robin_table_t* rt);
double robin_table_load_factor(const robin_table_t* rt);

typedef struct {
    const void* key;
    void* val;
} robin_table_iter_t;

robin_table_iter_t* robin_table_iter_create(const robin_table_t* rt);
bool robin_table_iter_next(robin_table_iter_t* iter);
void robin_table_iter_destroy(robin_table_iter_t* iter);

uint64_t robin_table_rapidhash(const void* key, size_t klen, uint64_t seed);
uint64_t robin_table_siphash(const void* key, size_t klen, uint64_t seed);
uint64_t robin_table_xxh64(const void* key, size_t klen, uint64_t seed);

size_t robin_table_psl_max(const robin_table_t* rt);
double robin_table_psl_mean(const robin_table_t* rt);
double robin_table_psl_variance(const robin_table_t* rt);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* ROBIN_TABLE_H */
