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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "robin_table.h"

#ifndef RT_NO_ASSERT
#include <assert.h>
#define RT_ASSERT(expr)           assert(expr)
#else
#define RT_ASSERT(expr)
#endif /* RT_NO_ASSERT */

#define RT_HASH_FUNC_DEFAULT      robin_table_rapidhash

/* Bucket count MUST be a power of two */
#define RT_BUCKET_COUNT_MIN       32U

/* Maximum and minimum load factor thresholds */
#define RT_LOAD_FACTOR_PCT_MAX    75U
#define RT_LOAD_FACTOR_PCT_MIN    25U

typedef struct {
    void* key;
    void* val;
    size_t psl;
    size_t klen;
    uint64_t hash;
} robin_bucket_t;

struct robin_table_t {
    robin_bucket_t* buckets;
    size_t count;
    size_t bucket_count;
    size_t init_buckets;
    size_t mask;
    size_t expand_at;
    size_t shrink_at;
    uint64_t seed;
    uint64_t (*hash_func)(const void*, size_t, uint64_t);
};

typedef struct {
    robin_table_iter_t iter;
    const robin_table_t* rt;
    size_t idx;
} robin_table_iter_impl_t;

/*
 * Round n to the next highest power of two.
 */
static inline size_t robin_table_next_pow2(size_t n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    ++n;
    return n;
}

/*
 * Compute the optimal number of buckets given the specified number of entries.
 */
static inline size_t robin_table_calc_bucket_count(size_t count)
{
    size_t bucket_count;

    /* Apply the maximum load factor */
    bucket_count = (count * 100) / RT_LOAD_FACTOR_PCT_MAX;

    /* Apply the minimum number of buckets */
    if (bucket_count < RT_BUCKET_COUNT_MIN) {
        bucket_count = RT_BUCKET_COUNT_MIN;
    } else {
        /* Round up to the nearest power of two */
        bucket_count = robin_table_next_pow2(bucket_count);
    }
    return bucket_count;
}

/*
 * Construct a new hash table with the given number of entries and hash function.
 */
robin_table_t* robin_table_create(size_t count,
                                  uint64_t (*hash_func)(const void*, size_t, uint64_t),
                                  uint64_t seed)
{
    robin_table_t* rt;

    rt = malloc(sizeof(robin_table_t));
    if (!rt) {
        return NULL;
    }
    rt->bucket_count = robin_table_calc_bucket_count(count);
    rt->buckets = calloc(rt->bucket_count, sizeof(robin_bucket_t));
    if (!rt->buckets) {
        free(rt);
        return NULL;
    }
    rt->count = 0;
    rt->init_buckets = rt->bucket_count;
    rt->mask = rt->bucket_count - 1;
    rt->expand_at = (rt->bucket_count * RT_LOAD_FACTOR_PCT_MAX) / 100;
    rt->shrink_at = (rt->bucket_count * RT_LOAD_FACTOR_PCT_MIN) / 100;
    rt->hash_func = hash_func ? hash_func : RT_HASH_FUNC_DEFAULT;
    rt->seed = seed;
    return rt;
}

/*
 * Internal function to add an entry without resizing the hash table.
 */
static void* robin_table_put0(robin_table_t* rt, const void* key, size_t klen, void* val)
{
    const uint64_t hash = rt->hash_func(key, klen, rt->seed);
    size_t idx = hash & rt->mask;
    robin_bucket_t entry;

    /* Set the entry */
    entry.key = (void*)key;
    entry.val = val;
    entry.klen = klen;
    entry.hash = hash;
    entry.psl = 0;

    while (1) {
        robin_bucket_t* bucket = rt->buckets + idx;

        /* Empty bucket: insert the entry */
        if (!bucket->key) {
            *bucket = entry;
            ++rt->count;
            return val;
        }

        /* Duplicate key: do not overwrite existing value */
        if (bucket->hash == hash && bucket->klen == klen &&
            memcmp(bucket->key, key, klen) == 0) {
            return bucket->val;
        }

        /*
         * If we hit a "richer" bucket (lower PSL), steal its
         * spot and swap the rich bucket's contents with the
         * current bucket.
         */
        if (bucket->psl < entry.psl) {
            robin_bucket_t temp;
            temp = *bucket;
            *bucket = entry;
            entry = temp;
        }

        /* Advance to the next bucket */
        idx = (idx + 1) & rt->mask;
        ++entry.psl;
    }
}

/*
 * Expand or shrink the hash table and rehash all existing entries.
 */
static bool robin_table_resize(robin_table_t* rt, size_t bucket_count)
{
    const size_t old_bucket_count = rt->bucket_count;
    robin_bucket_t* old_buckets = rt->buckets;
    robin_bucket_t* new_buckets;

    RT_ASSERT((bucket_count & (bucket_count - 1)) == 0);
    RT_ASSERT(bucket_count > rt->count);

    new_buckets = calloc(bucket_count, sizeof(robin_bucket_t));
    if (!new_buckets) {
        return false;
    }
    rt->buckets = new_buckets;
    rt->count = 0;
    rt->bucket_count = bucket_count;
    rt->mask = rt->bucket_count - 1;
    rt->expand_at = (rt->bucket_count * RT_LOAD_FACTOR_PCT_MAX) / 100;
    rt->shrink_at = (rt->bucket_count * RT_LOAD_FACTOR_PCT_MIN) / 100;

    for (size_t i = 0; i < old_bucket_count; ++i) {
        const robin_bucket_t* bucket = old_buckets + i;

        if (bucket->key) {
            robin_table_put0(rt, bucket->key, bucket->klen, bucket->val);
        }
    }
    free(old_buckets);
    return true;
}

/*
 * Add a new entry in the hash table using Robin Hood hashing.
 *
 * => If an entry with a matching key already exists, return the existing value.
 * => Otherwise, return newly assigned value on successful insertion.
 */
void* robin_table_put(robin_table_t* rt, const void* key, size_t klen, void* val)
{
    RT_ASSERT(rt != NULL);
    RT_ASSERT(key != NULL && klen != 0);

    if (rt->count >= rt->expand_at) {
        if (!robin_table_resize(rt, rt->bucket_count << 1)) {
            return NULL;
        }
    }

    return robin_table_put0(rt, key, klen, val);
}

/*
 * Search the bucket containing the given key using the Robin Hood invariant.
 *
 * => If the key exists, return its associated bucket; otherwise, NULL.
 */
static robin_bucket_t* robin_table_get_bucket(robin_table_t* rt, const void* key,
                                              size_t klen)
{
    const uint64_t hash = rt->hash_func(key, klen, rt->seed);
    size_t idx = hash & rt->mask;
    size_t psl = 0;

    while (1) {
        robin_bucket_t* bucket = rt->buckets + idx;

        /* Matching key: return the bucket */
        if (bucket->hash == hash && bucket->klen == klen &&
            memcmp(bucket->key, key, klen) == 0) {
            return bucket;
        }

        /*
         * If we hit an empty bucket or a "rich"
         * bucket, stop probing and return NULL.
         */
        if (!bucket->key || bucket->psl < psl) {
            return NULL;
        }

        /* Advance to the next bucket */
        idx = (idx + 1) & rt->mask;
        ++psl;
    }
}

/*
 * Retrieve the value associated with a given key, or NULL if no entry exists.
 */
void* robin_table_get(robin_table_t* rt, const void* key, size_t klen)
{
    robin_bucket_t* bucket;

    RT_ASSERT(rt != NULL);
    RT_ASSERT(key != NULL && klen != 0);

    bucket = robin_table_get_bucket(rt, key, klen);
    return bucket ? bucket->val : NULL;
}

/*
 * Remove an entry with the specified key from the hash table.
 *
 * => Return the associated value, or NULL, if the entry is not found.
 */
void* robin_table_del(robin_table_t* rt, const void* key, size_t klen)
{
    robin_bucket_t* bucket;
    size_t idx;
    void* val;

    RT_ASSERT(rt != NULL);
    RT_ASSERT(key != NULL && klen != 0);

    bucket = robin_table_get_bucket(rt, key, klen);
    if (!bucket) {
        return NULL;  /* Key not found */
    }

    /* Store the value */
    val = bucket->val;

    /* Get the bucket index */
    idx = bucket - rt->buckets;

    /* Apply the backward shift method */
    while (1) {
        robin_bucket_t* next_bucket;

        idx = (idx + 1) & rt->mask;
        next_bucket = rt->buckets + idx;

        /*
         * Stop shifting if the next bucket is empty or
         * has a key that is in its original position.
         */
        if (!next_bucket->key || next_bucket->psl == 0) {
            /* Clear the bucket */
            memset(bucket, 0, sizeof(*bucket));
            --rt->count;
            break;
        }

        --next_bucket->psl;
        *bucket = *next_bucket;
        bucket = next_bucket;
    }

    if (rt->bucket_count > rt->init_buckets && rt->count <= rt->shrink_at) {
        /*
         * Safe to ignore shrink failures: no structural impact
         * on the hash table.
         */
        (void)robin_table_resize(rt, rt->bucket_count >> 1);
    }
    return val;
}

/*
 * Clear the hash table and optionally shrink to its initial number of buckets.
 */
bool robin_table_clear(robin_table_t* rt, bool update_buckets)
{
    RT_ASSERT(rt != NULL);

    if (update_buckets) {
        robin_bucket_t* new_buckets;

        new_buckets = malloc(rt->init_buckets * sizeof(robin_bucket_t));
        if (!new_buckets) {
            return false;
        }
        free(rt->buckets);
        rt->buckets = new_buckets;
        rt->bucket_count = rt->init_buckets;
        rt->mask = rt->bucket_count - 1;
        rt->expand_at = (rt->bucket_count * RT_LOAD_FACTOR_PCT_MAX) / 100;
        rt->shrink_at = (rt->bucket_count * RT_LOAD_FACTOR_PCT_MIN) / 100;
    }
    rt->count = 0;
    memset(rt->buckets, 0, rt->bucket_count * sizeof(*rt->buckets));
    return true;
}

/*
 * Free the memory associated with the hash table.
 */
void robin_table_destroy(robin_table_t* rt)
{
    if (!rt) {
        return;
    }

    free(rt->buckets);
    memset(rt, 0, sizeof(*rt));
    free(rt);
}

/*
 * Return the number of entries in the hash table.
 */
size_t robin_table_count(const robin_table_t* rt)
{
    RT_ASSERT(rt != NULL);

    return rt->count;
}

/*
 * Return the load factor of the hash table.
 */
double robin_table_load_factor(const robin_table_t* rt)
{
    RT_ASSERT(rt != NULL);
    RT_ASSERT(rt->count != 0);

    return (double)rt->count / rt->bucket_count;
}

/*
 * Create a new iterator for traversing the hash table.
 */
robin_table_iter_t* robin_table_iter_create(const robin_table_t* rt)
{
    RT_ASSERT(rt != NULL);

    robin_table_iter_impl_t* iter_impl = malloc(sizeof(robin_table_iter_impl_t));
    if (!iter_impl) {
        return NULL;
    }
    iter_impl->iter.key = NULL;
    iter_impl->iter.val = NULL;
    iter_impl->rt = rt;
    iter_impl->idx = -1;

    return &iter_impl->iter;
}

/*
 * Advance the iterator to the next entry in the hash table.
 *
 * => Return false if no more valid entries are left in the hash table.
 */
bool robin_table_iter_next(robin_table_iter_t* iter)
{
    RT_ASSERT(iter != NULL);

    robin_table_iter_impl_t* iter_impl = (robin_table_iter_impl_t*)iter;

    while (++iter_impl->idx < iter_impl->rt->bucket_count) {
        const robin_bucket_t* bucket = iter_impl->rt->buckets + iter_impl->idx;

        if (bucket->key) {
            iter_impl->iter.key = bucket->key;
            iter_impl->iter.val = bucket->val;
            return true;
        }
    }

    /* Clear the iterator */
    memset(&iter_impl->iter, 0, sizeof(*iter));
    return false;
}

/*
 * Free the memory associated with the iterator.
 */
void robin_table_iter_destroy(robin_table_iter_t* iter)
{
    if (!iter) {
        return;
    }

    robin_table_iter_impl_t* iter_impl = (robin_table_iter_impl_t*)iter;
    free(iter_impl);
}

/*
 * Return the maximum probe sequence length in the hash table.
 *
 * => The maximum probe sequence length directly correlates with 
 *    the worst-case lookup cost of the hash table.
 */
size_t robin_table_psl_max(const robin_table_t* rt)
{
    RT_ASSERT(rt != NULL);
    RT_ASSERT(rt->count != 0);

    size_t max_psl = 0;

    for (size_t i = 0; i < rt->bucket_count; ++i) {
        const robin_bucket_t* bucket = rt->buckets + i;

        if (bucket->key && bucket->psl > max_psl) {
            max_psl = bucket->psl;
        }
    }
    return max_psl;
}

/*
 * Compute the average displacement of entries from their original buckets.
 *
 * => The higher the mean psl, the more probes are required on average in
 *    core operations (e.g., put, get, del).
 */
double robin_table_psl_mean(const robin_table_t* rt)
{
    RT_ASSERT(rt != NULL);
    RT_ASSERT(rt->count != 0);

    double psl_sum = 0;

    for (size_t i = 0; i < rt->bucket_count; ++i) {
        const robin_bucket_t* bucket = rt->buckets + i;

        if (bucket->key) {
            psl_sum += (double)bucket->psl;
        }
    }
    return psl_sum / rt->count;
}

/*
 * Compute the statistical variance of the probe sequence lengths.
 *
 * => The higher the variance, the more likely the hash function is
 *    invalidating the simple uniform hashing assumption and leading
 *    to clustering in specific areas of the hash table.
 */
double robin_table_psl_variance(const robin_table_t* rt)
{
    RT_ASSERT(rt != NULL);
    RT_ASSERT(rt->count != 0);

    double mean = robin_table_psl_mean(rt);
    double var_sum = 0;

    for (size_t i = 0; i < rt->bucket_count; ++i) {
        const robin_bucket_t* bucket = rt->buckets + i;

        if (bucket->key) {
            double diff = (double)bucket->psl - mean;
            var_sum += diff * diff;
        }
    }
    return var_sum / rt->count;
}
