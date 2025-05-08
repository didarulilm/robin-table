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
#include <stdint.h>

#include "robin_table.h"
#include "t_framework.h"

#define TEST_NUM_ENTRIES    1000000UL  /* 1M */
#define TEST_STR_LEN        32U

#define KEY_INT(k)          (k), sizeof(*(k))
#define KEY_STR(k)          (k), TEST_STR_LEN + 1

typedef struct {
    size_t count;
    uint64_t seed;
    uint64_t (*hash_func)(const void*, size_t, uint64_t);
} test_rt_options_t;

static char* temp_val = "lorem";  /* Placeholder value */

static char* test_alloc_random_str(void)
{
    char* key;

    key = malloc(TEST_STR_LEN + 1);
    if (!key) {
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < TEST_STR_LEN; ++i) {
        unsigned char n = random();
        key[i] = (n % 95) + 32;  /* ASCII printable only (32 to 126) */
    }
    key[TEST_STR_LEN] = '\0';
    return key;
}

static uint64_t* test_alloc_random_int(void)
{
    uint64_t* key;

    key = malloc(sizeof(*key));
    if (!key) {
        exit(EXIT_FAILURE);
    }

    *key = 0;
    for (int i = 0; i < 4; ++i) {
        *key = (*key << 16) | (random() & 0xFFFF);
    }
    return key;
}

static char** test_alloc_keys_str(void)
{
    char** keys_str;

    keys_str = malloc(TEST_NUM_ENTRIES * sizeof(*keys_str));
    if (!keys_str) {
        exit(EXIT_FAILURE);
    }
    return keys_str;
}

static uint64_t** test_alloc_keys_int(void)
{
    uint64_t** keys_int;

    keys_int = malloc(TEST_NUM_ENTRIES * sizeof(*keys_int));
    if (!keys_int) {
        exit(EXIT_FAILURE);
    }
    return keys_int;
}

static void test_initialize_keys(char** keys_str, uint64_t** keys_int)
{
    for (size_t i = 0; i < TEST_NUM_ENTRIES; ++i) {
        keys_str[i] = test_alloc_random_str();
        keys_int[i] = test_alloc_random_int();
    }
}

static void test_free_keys(char** keys_str, uint64_t** keys_int)
{
    for (size_t i = 0; i < TEST_NUM_ENTRIES; ++i) {
        free(keys_str[i]);
        free(keys_int[i]);
    }
    free(keys_str);
    free(keys_int);
}

TEST_ADD(test_put_str, char** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_TIMER_START();
    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_STR(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);
    TEST_TIMER_END();

    ASSERT(robin_table_count(rt) == TEST_NUM_ENTRIES);
    robin_table_destroy(rt);
}

TEST_ADD(test_put_int, uint64_t** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_TIMER_START();
    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_INT(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);
    TEST_TIMER_END();

    ASSERT(robin_table_count(rt) == TEST_NUM_ENTRIES);
    robin_table_destroy(rt);
}

TEST_ADD(test_get_str, char** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_STR(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    TEST_TIMER_START();
    TEST_LOOP_START(2);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_get(rt, KEY_STR(keys[i]));
        ASSERT_LOOP(res == temp_val, 2);
    }
    TEST_LOOP_END(2);
    TEST_TIMER_END();

    robin_table_destroy(rt);
}

TEST_ADD(test_get_int, uint64_t** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_INT(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    TEST_TIMER_START();
    TEST_LOOP_START(2);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_get(rt, KEY_INT(keys[i]));
        ASSERT_LOOP(res == temp_val, 2);
    }
    TEST_LOOP_END(2);
    TEST_TIMER_END();

    robin_table_destroy(rt);
}

TEST_ADD(test_del_str, char** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_STR(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    ASSERT(robin_table_count(rt) == TEST_NUM_ENTRIES);

    TEST_TIMER_START();
    TEST_LOOP_START(2);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_del(rt, KEY_STR(keys[i]));
        ASSERT_LOOP(res == temp_val, 2);
    }
    TEST_LOOP_END(2);
    TEST_TIMER_END();

    ASSERT(robin_table_count(rt) == 0);
    robin_table_destroy(rt);
}

TEST_ADD(test_del_int, uint64_t** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_INT(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    ASSERT(robin_table_count(rt) == TEST_NUM_ENTRIES);

    TEST_TIMER_START();
    TEST_LOOP_START(2);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_del(rt, KEY_INT(keys[i]));
        ASSERT_LOOP(res == temp_val, 2);
    }
    TEST_LOOP_END(2);
    TEST_TIMER_END();

    ASSERT(robin_table_count(rt) == 0);
    robin_table_destroy(rt);
}

TEST_ADD(test_iterate_str, char** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    robin_table_iter_t* iter;
    size_t iter_count;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_STR(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    iter = robin_table_iter_create(rt);
    ASSERT(iter != NULL);

    iter_count = 0;

    TEST_TIMER_START();
    TEST_LOOP_START(2);
    while (robin_table_iter_next(iter)) {
        const char* key = iter->key;
        char* val = iter->val;
        ASSERT_LOOP(key != NULL && val != NULL, 2);
        ++iter_count;
    }
    TEST_LOOP_END(2);
    TEST_TIMER_END();

    ASSERT(iter_count == TEST_NUM_ENTRIES);
    robin_table_iter_destroy(iter);
    robin_table_destroy(rt);
}

TEST_ADD(test_iterate_int, uint64_t** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    robin_table_iter_t* iter;
    size_t iter_count;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_INT(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    iter = robin_table_iter_create(rt);
    ASSERT(iter != NULL);

    iter_count = 0;

    TEST_TIMER_START();
    TEST_LOOP_START(2);
    while (robin_table_iter_next(iter)) {
        const uint64_t* key = iter->key;
        char* val = iter->val;
        ASSERT_LOOP(key != NULL && val != NULL, 2);
        ++iter_count;
    }
    TEST_LOOP_END(2);
    TEST_TIMER_END();

    ASSERT(iter_count == TEST_NUM_ENTRIES);
    robin_table_iter_destroy(iter);
    robin_table_destroy(rt);
}

TEST_ADD(test_consistency, uint64_t** keys, test_rt_options_t rt_opt)
{
    char* new_val = "ipsum";
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_TIMER_START();
    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_INT(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);

        res = robin_table_get(rt, KEY_INT(keys[i]));
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    TEST_LOOP_START(2);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        /* Remove odd entries */
        if (i % 2 != 0) {
            res = robin_table_del(rt, KEY_INT(keys[i]));
            ASSERT_LOOP(res == temp_val, 2);

            res = robin_table_get(rt, KEY_INT(keys[i]));
            ASSERT_LOOP(res == NULL, 2);
        } else {
            /* Try overwriting existing entries */
            res = robin_table_put(rt, KEY_INT(keys[i]), new_val);
            ASSERT_LOOP(res == temp_val, 2);

            res = robin_table_get(rt, KEY_INT(keys[i]));
            ASSERT_LOOP(res == temp_val, 2);
        }
    }
    TEST_LOOP_END(2);

    ASSERT(robin_table_count(rt) == TEST_NUM_ENTRIES / 2);

    TEST_LOOP_START(3);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        /* Re-insert into deleted slots */
        if (i % 2 != 0) {
            res = robin_table_get(rt, KEY_INT(keys[i]));
            ASSERT_LOOP(res == NULL, 3);

            res = robin_table_put(rt, KEY_INT(keys[i]), new_val);
            ASSERT_LOOP(res == new_val, 3);

            res = robin_table_get(rt, KEY_INT(keys[i]));
            ASSERT_LOOP(res == new_val, 3);
        } else {
            res = robin_table_get(rt, KEY_INT(keys[i]));
            ASSERT_LOOP(res == temp_val, 3);
        }
    }
    TEST_LOOP_END(3);
    TEST_TIMER_END();

    ASSERT(robin_table_count(rt) == TEST_NUM_ENTRIES);
    robin_table_destroy(rt);
}

TEST_ADD(test_clear, uint64_t** keys, test_rt_options_t rt_opt)
{
    robin_table_t* rt;
    void* res;

    rt = robin_table_create(rt_opt.count, rt_opt.hash_func, rt_opt.seed);
    ASSERT(rt != NULL);

    TEST_LOOP_START(1);
    for (size_t i = 0; i < rt_opt.count; ++i) {
        res = robin_table_put(rt, KEY_INT(keys[i]), temp_val);
        ASSERT_LOOP(res == temp_val, 1);
    }
    TEST_LOOP_END(1);

    ASSERT(robin_table_count(rt) == TEST_NUM_ENTRIES);

    TEST_TIMER_START();
    ASSERT(robin_table_clear(rt, false) == true);
    TEST_TIMER_END();

    ASSERT(robin_table_count(rt) == 0);
    robin_table_destroy(rt);
}

TEST_MAIN(
    test_rt_options_t rt_opt; 
    char** keys_str; 
    uint64_t** keys_int;

    rt_opt.count = TEST_NUM_ENTRIES; 
    rt_opt.hash_func = robin_table_rapidhash;
    rt_opt.seed = RT_RAPID_SEED;

    keys_str = test_alloc_keys_str(); 
    keys_int = test_alloc_keys_int();

    srandom(42); 
    test_initialize_keys(keys_str, keys_int);

    TEST_RUN(test_put_str, keys_str, rt_opt); 
    TEST_RUN(test_put_int, keys_int, rt_opt);
    TEST_RUN(test_get_str, keys_str, rt_opt); 
    TEST_RUN(test_get_int, keys_int, rt_opt);
    TEST_RUN(test_del_str, keys_str, rt_opt); 
    TEST_RUN(test_del_int, keys_int, rt_opt);
    TEST_RUN(test_iterate_str, keys_str, rt_opt); 
    TEST_RUN(test_iterate_int, keys_int, rt_opt); 
    TEST_RUN(test_consistency, keys_int, rt_opt);
    TEST_RUN(test_clear, keys_int, rt_opt);

    test_free_keys(keys_str, keys_int);
)
