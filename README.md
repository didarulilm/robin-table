# #️⃣ robin-table 
[![CI](https://github.com/didarulilm/robin-table/actions/workflows/ci.yml/badge.svg)](https://github.com/didarulilm/robin-table/actions/workflows/ci.yml)

The robin-table library is a fast hash table/hash map implementation in C using open addressing with linear probing and Robin Hood hashing for collision resolution.

## Key Features

- Pretty fast hash table, check the [benchmarks](https://github.com/didarulilm/robin-table?tab=readme-ov-file#benchmarks) for details.
- Support any arbitrary keys/values.
- Built-in support for both cryptographic and non-cryptographic hash functions with performance analysis utilities to help you assess the best hash function for your key domain.
- Blazingly fast iterator that allows safe traversal of all entries while encapsulating the internal implementation details of the hash table.
- The library can be used with all internal assertions disabled by defining the `RT_NO_ASSERT` macro.

## API Examples

### Create a hash table 

To construct a hash table you need to specify the initial number of entries, a hash function, and a seed value for the provided hash function:

```C
/* Create a hash table for 64 initial entries, using the SipHash algorithm */
robin_table_t* rt = robin_table_create(64, robin_table_siphash, 0);
```

Note that it is important to use a good hash function; otherwise, it can lead to degraded performance or potential vulnerabilities to Denial-of-Service (DoS) attacks; read the [built-in hash functions](https://github.com/didarulilm/robin-table?tab=readme-ov-file#built-in-hash-functions) section below.

### Insertion, access, and removal of entries

The hash table is type-agnostic and multiple entries of differing types can exist in the same hash table. When inserting, accessing, or removing an entry from the hash table you must provide the length of the key:

```C
void* res;

/* Insert an entry (returns the existing value if the key already exists) */
res = robin_table_put(rt, "foo", sizeof("foo") - 1, "bar");

/* Access the value associated with a given key */
res = robin_table_get(rt, "foo", sizeof("foo") - 1);

/* Remove the entry with the given key */
res = robin_table_del(rt, "foo", sizeof("foo") - 1);
```

However, I recommend defining a convenience macro based on your key domain to pass fewer arguments and simplify the usage of keys:

```C
/* A convenience macro for string literal keys */
#define KEY_STR_LIT(k)    (k), sizeof(k) - 1  

void* res;

/* Expands to `robin_table_put(rt, "foo", 3, "bar");`*/
res = robin_table_put(rt, KEY_STR_LIT("foo"), "bar");

/* Expands to `robin_table_get(rt, "foo", 3);` */
res = robin_table_get(rt, KEY_STR_LIT("foo"));

/* Expands to `robin_table_del(rt, "foo", 3);` */
res = robin_table_del(rt, KEY_STR_LIT("foo"));
```

To quickly clear the hash table by removing all existing entries use the `robin_table_clear` function:

```C
/* Clear the hash table */
if (robin_table_clear(rt, false) != true) {
    printf("failed\n");
}

/* Clear the hash table and shrink to its initial number of buckets */
if (robin_table_clear(rt, true) != true) {
    printf("failed\n");
}
```

### Iteration

The iterator interface enables safe, read-only key-value access in an arbitrary order:

```C
/* Create an iterator */
robin_table_iter_t* iter = robin_table_iter_create(rt);

/* Iterate over all hash table entries */
while (robin_table_iter_next(iter)) {
    /* Access each entry */
}

/* Free the iterator */
robin_table_iter_destroy(iter);
```

### Built-in hash functions 

This library is internally configured to use [rapidhash](https://github.com/Nicoshev/rapidhash) (an improved wyhash) by default, which is the fastest recommended hash function by [SMHasher](https://github.com/rurban/smhasher?tab=readme-ov-file#summary). In addition, it comes with built-in support for [SipHash-2-4](https://github.com/veorq/SipHash) and [xxh64](https://github.com/Cyan4973/xxHash), eliminating the need for custom implementations in most cases:

```C
robin_table_rapidhash()   /* Returns 64-bit hash value of the key using rapidhash */
robin_table_siphash()     /* Returns 64-bit hash value of the key using SipHash-2-4 */
robin_table_xxh64()       /* Returns 64-bit hash value of the key using xxh64 */
```

I recommend using one of the supplied hash functions unless you have very specific requirements that necessitate alternative algorithms. You can easily compare the performance of different hash functions and choose the most suitable one for your specific key domain using the built-in suite of performance analysis functions:

```C
robin_table_psl_max()        /* Returns maximum probe sequence length in the hash table */  
robin_table_psl_mean()       /* Returns average probe sequence length across all entries */ 
robin_table_psl_variance()   /* Returns statistical variance of probe sequence lengths */   
```

### Cleanup

To free the memory associated with the hash table use the `robin_table_destroy` function:

```C
robin_table_destroy(rt);
```

Note that the robin-table does not manage the memory associated with keys or values, so ensure you free these resources before calling `robin_table_destroy`.

## Testing and Usage 

This library uses the [Meson Build System](https://mesonbuild.com/Quick-guide.html) to orchestrate the build and installation process. After installing Meson, follow these simple steps to build and install on your host machine:

1. `git clone https://github.com/didarulilm/robin-table.git` - clone the repository
2. `meson setup build` - setup the build directory
3. `meson compile -C build` - compile the code
4. `meson test -C build` - run tests with benchmarks
5. `meson install -C build` - *OPTIONAL*  install the library 

To use the robin-table library, simply clone this repository in your [subprojects](https://mesonbuild.com/Subprojects.html) directory and just `#include "robin_table.h"` in your code. 

You can also specify a [Wrap](https://mesonbuild.com/Wrap-dependency-system-manual.html) file that tells Meson how to download it for you and it will automatically download and extract it during build. An example wrap-git named `robin_table.wrap` will look like this:

```ini
[wrap-git]
url = https://github.com/didarulilm/robin-table.git
revision = v1.0.0  
depth = 1
```

## Benchmarks

I used a custom-built single-header testing framework to run unit tests alongside benchmarks 
in a single test file. The benchmarks were run on the following configuration:

- Machine: Apple M1 Pro 2020  
- OS: macOS Sequoia 15.4.1 
- Compiler: clang version 17.0.0

In all cases, the hash tables were created for an initial number of 8,000,000 entries. The provided hash function was `robin_table_rapidhash` with `RT_RAPID_SEED` as the default seed. When the key types were randomly generated 32-byte string literals:

| Operation Type | Total Operations | Duration (μs) | Duration (sec) | Throughput (ops/sec) |
| -------------- | ---------------- | ------------- | -------------- | -------------------- |
| put            | 8,000,000        | 1,622,562     | 1.622          | 4,932,183            |
| get            | 8,000,000        | 1,597,656     | 1.597          | 5,009,393            |
| del            | 8,000,000        | 1,656,132     | 1.656          | 4,830,918            |
| iter           | 8,000,000        | 140,016       | 0.140          | 57,142,857           |
 
 When the key types were randomly generated 8-byte unsigned ints:

| Operation Type | Total Operations | Duration (μs) | Duration (sec) | Throughput (ops/sec) |
| -------------- | ---------------- | ------------- | -------------- | -------------------- |
| put            | 8,000,000        | 1,367,725     | 1.367          | 5,852,231            |
| get            | 8,000,000        | 1,268,225     | 1.268          | 6,309,148            |
| del            | 8,000,000        | 1,439,134     | 1.439          | 5,559,416            |
| iter           | 8,000,000        | 139,592       | 0.139          | 57,553,957           |

## License

The robin-table source code is available under the MIT License.
