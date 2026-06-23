#ifndef S_TRUCTURES_H
#define S_TRUCTURES_H

#ifndef S_TRUCTURES_NOSTD
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef _MSC_VER
#define ST_NORETURN __declspec(noreturn)
#else
#define ST_NORETURN __attribute__((noreturn))
#endif

#define ST_TINY_MAP_CAPACITY (256)

/// A unique identifier for a tiny-map entry.
///
/// Use `StHashStr()` or `TinyDict*()` functions for indexing using string keys of arbitrary length.
/// Otherwise, if 8 bytes is enough, you can use `StStrKey`.
typedef uint64_t TinyHash;

/// An internal storage cell for `TinyMap`s.
typedef struct TinyBucket {
    TinyHash hash;
    void *data, (*cleanup)(void*);
    size_t data_size;
} TinyBucket;

/// A tiny hashmap-like structure indexed with 8-byte keys.
typedef struct {
    TinyBucket** buckets;
    size_t length;
} TinyMap;

/// An iterator over tiny-maps.
typedef struct {
    TinyMap* source;
    size_t hash_idx, bucket_idx;
    TinyBucket* bucket;
    void* data;
} TinyMapIterator;

#define ST_TINY_D_INITIAL_CAPACITY ((size_t)64)
#define ST_TINY_D_GROWTH_FACTOR ((size_t)2)

/// The header of a tiny dynamic array. You never interact with it directly.
typedef struct {
    size_t length, capacity, elt_size;
} TinyDHead;

#define TINY_MAP_FOREACH(map, it) for (TinyMapIterator it = TinyMapIter((map)); TinyMapNext(&(it));)

/// Copy up to 8 bytes from a string and return them as an `StTinyKey`.
TinyHash StStrKey(const char* s);

/// Hash a string of arbitrary length into an `StTinyKey`.
TinyHash StHashStr(const char* s);

/// Cleanup a `TinyMap`.
void FreeTinyMap(TinyMap* that);

/// Returns the amount of key-value pairs inside this tiny-map.
size_t TinyMapLength(const TinyMap* that);

/// Insert data into the tinymap. Allocates a chunk of memory and copies data
/// from input.
///
/// This also returns the resulting bucket in case you need to set a cleanup
/// function.
TinyBucket* TinyMapPut(TinyMap* that, TinyHash hash, const void* data, int size);

/// An shorthand for `TinyMapPut` which accepts string keys and hashes them for you.
#define TinyDictPut(that, hash, data, size) TinyMapPut((that), StHashStr((hash)), (data), (size))

/// Find the bucket by input key, or return `NULL` if there is none.
TinyBucket* TinyMapFind(const TinyMap* that, TinyHash hash);

/// An shorthand for `TinyMapFind` which accepts string keys and hashes them for you.
#define TinyDictFind(that, hash) TinyMapFind((that), StHashStr((hash)))

/// Returns a pointer to an entry's data, if any. Spits out a `NULL` otherwise.
///
/// If you need to check the entry's actual size, use the full-form `TinyMapFind`.
char* TinyMapGet(const TinyMap* that, TinyHash hash);

/// An shorthand for `TinyMapGet` which accepts string keys and hashes them for you.
#define TinyDictGet(that, hash) TinyMapGet((that), StHashStr((hash)))

/// Free the bucket and the data associated with a key.
void TinyMapErase(TinyMap* that, TinyHash hash);

/// An shorthand for `TinyMapErase` which accepts string keys and hashes them for you.
#define TinyDictErase(that, hash) TinyMapErase((that), StHashStr((hash)))

/// Creates an iterator over the values of a tiny-map.
///
/// Pointer-cast and dereference `.data` to get the value of the current entry. Cast `.bucket` to
/// `TinyBucket` to set/unset a cleanup function.
TinyMapIterator TinyMapIter(TinyMap* that);

/// Returns true and advances the iterator if there is an entry available inside the iterable.
/// Otherwise returns false.
bool TinyMapNext(TinyMapIterator* iter);

/// Creates a dynamic-array with the specified capacity and element-size.
void* MakeTinyDPro(size_t capacity, size_t elt_size);

/// A shorthand for `MakeTinyDPro` that creates a dynamic-array with a default capacity and the
/// element-size equal to the size requirement of the passed type.
#define MakeTinyD(T) ((T*)MakeTinyDPro(ST_TINY_D_INITIAL_CAPACITY, sizeof(T)))

/// Properly cleans up a tiny dynamic-array and its header.
void FreeTinyD(void* that);

/// Returns a specific property of a tiny-D.
size_t TinyDLength(const void* that), TinyDCapacity(const void* that),
    TinyDElementSize(const void* that);

/// Appends an element to the dynamic-array, growing it if necessary. DO NOT FORGET to assign the
/// result of this to the array you passed in.
void* TinyDAppendPro(void* that, const void* ref);

/// A shorthand for `TinyDPush` that accepts any value, not just pointers. DO NOT FORGET to assign
/// the result of this to the array you passed in.
///
/// (Ab)uses the GCC compound statement extension; may not work with non-mainstream compilers.
#define TinyDAppend(that, value)                                                                   \
    ({                                                                                             \
        __typeof__(value) tmp = (value);                                                           \
        TinyDAppendPro((that), &tmp);                                                              \
    })

/// Shrinks a tiny-D's internal length counter to the specified value.
void* TinyDShrink(void* that, size_t newlen);

/// Shaves the last appended value off the tiny-D.
void* TinyDPop(void* that);

/// Alias for `TinyDPop` to match with `TinyDPopFront`.
#define TinyDPopBack TinyDPop

/// Removes the first element.
void* TinyDPopFront(void* that);

/// Pops the element at index `idx` and shifts the rest accordingly.
void* TinyDErase(void* that, size_t idx);

#ifdef S_TRUCTURES_IMPLEMENTATION

#if !defined(StAlloc) && !defined(StFree)

#include <stdlib.h>
#define StAlloc malloc
#define StFree free

#elif !defined(StAlloc) || !defined(StFree)

#error Define StAlloc and StFree together!

#endif

#if !defined(StMemset) && !defined(StMemcpy)

#include <string.h>
#define StMemset memset
#define StMemcpy memcpy

#elif !defined(StMemset) || !defined(StMemcpy)

#error Define StMemset and StMemcpy together!

#endif

#ifndef StLog
#include <stdio.h>
#define StLog(msg, ...)                                                                            \
    do {                                                                                           \
        fprintf(stdout, "[S_tr]: " msg "\n", ##__VA_ARGS__);                                       \
        fflush(stdout);                                                                            \
    } while (0)
#endif

#ifndef StDie

#include <stdlib.h> // `EXIT_FAILURE` & `exit`

ST_NORETURN void StDie()
#ifdef S_TRUCTURES_IMPLEMENTATION
{
    exit(EXIT_FAILURE);
}
#else
    ;
#endif

#endif

#define StOutOfJuice()                                                                             \
    do {                                                                                           \
        StLog("Out of memory!!!");                                                                 \
        StDie();                                                                                   \
    } while (0)

#define StCheckedAlloc(var, size)                                                                  \
    do {                                                                                           \
        *(void**)&(var) = StAlloc((size));                                                         \
        if (!(var))                                                                                \
            StOutOfJuice();                                                                        \
    } while (0)

#endif

#ifdef S_TRUCTURES_IMPLEMENTATION
#define ST_MAKE_MAP_GET(suffix, type)                                                              \
    type TinyMapGet##suffix(const TinyMap* that, TinyHash hash) {                                  \
        const void* data = TinyMapGet(that, hash);                                                 \
        return data ? *(type*)data : 0;                                                            \
    }                                                                                              \
                                                                                                   \
    type TinyDictGet##suffix(const TinyMap* that, const char* key) {                               \
        const void* data = TinyMapGet(that, StHashStr(key));                                       \
        return data ? *(type*)data : 0;                                                            \
    }
#else
#define ST_MAKE_MAP_GET(suffix, type)                                                              \
    type TinyMapGet##suffix(const TinyMap*, TinyHash);                                             \
    type TinyDictGet##suffix(const TinyMap*, const char*);
#endif

ST_MAKE_MAP_GET(I16, int16_t);
ST_MAKE_MAP_GET(U16, uint16_t);
ST_MAKE_MAP_GET(I32, int32_t);
ST_MAKE_MAP_GET(U32, uint32_t);
ST_MAKE_MAP_GET(I64, int64_t);
ST_MAKE_MAP_GET(U64, uint64_t);

#undef ST_MAKE_MAP_GET

#ifdef S_TRUCTURES_IMPLEMENTATION

#define TinyKey2Idx(key) ((size_t)((ST_TINY_MAP_CAPACITY - 1) & StShuffleKey(key)))
#define TinyDGetHead(ptr) ((ptr) ? ((TinyDHead*)((char*)(ptr) - sizeof(TinyDHead))) : NULL)

static const TinyHash StShuffleKey(const TinyHash hash) {
    return hash ^ (hash >> (4 * sizeof(hash)));
}

TinyHash StStrKey(const char* s) {
    static char buf[sizeof(TinyHash)] = {0};
    if (!s)
        return 0;
    for (int i = 0; i < sizeof(buf); i++)
        if (!s[i]) {
            StMemcpy(buf, s, i);
            StMemset(buf + i, 0xFF, sizeof(buf) - i);
            return *(TinyHash*)buf;
        }
    return *(TinyHash*)s;
}

// Thanks:
// 1. <https://github.com/toggins/Klawiatura/blob/bf6d4a12877ee850ea2c52ae5e976fbf5f787aee/src/K_memory.c#L5>
// 2. <https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function>

TinyHash StHashStr(const char* s) {
    TinyHash hash = 0xcbf29ce484222325;
    for (; s && *s; s++)
        hash ^= *(uint8_t*)s, hash *= 0x00000100000001b3;
    return hash;
}

static void StCleanupBucket(const TinyBucket* that) {
    if (that->cleanup && that->data)
        that->cleanup(that->data);
}

static void FreeTinyBucket(const TinyBucket* that) {
    StCleanupBucket(that);
    if (that->data)
        StFree(that->data);
}

void FreeTinyMap(TinyMap* that) {
    if (!that)
        return;

    if (that->buckets) {
        for (int i = 0; i < ST_TINY_MAP_CAPACITY; i++) {
            for (int j = 0; j < TinyDLength(that->buckets[i]); j++)
                FreeTinyBucket(&that->buckets[i][j]);
            FreeTinyD(that->buckets[i]);
        }

        StFree((void*)that->buckets), that->buckets = NULL;
    }

    that->length = 0;
}

size_t TinyMapLength(const TinyMap* that) {
    return that->length;
}

TinyBucket* TinyMapPut(TinyMap* that, TinyHash hash, const void* data, int size) {
    if (size < 1) { // TODO: bar behind a debug build check?
        StLog("Requested bucket size 0; catching on fire");
        return NULL;
    }

    if (!that->buckets) {
        StCheckedAlloc(that->buckets, sizeof(TinyBucket*) * ST_TINY_MAP_CAPACITY);
        StMemset((void*)that->buckets, 0, sizeof(TinyBucket*) * ST_TINY_MAP_CAPACITY);
    }

    const size_t idx = TinyKey2Idx(hash);

    if (!that->buckets[idx])
        that->buckets[idx] = MakeTinyD(TinyBucket);

    const size_t len = TinyDLength(that->buckets[idx]);

    for (size_t i = 0; i < len; i++) {
        TinyBucket* bucket = &that->buckets[idx][i];

        if (bucket->hash == hash) {
            StCleanupBucket(bucket);

            if (bucket->data_size != (size_t)size) {
                if (bucket->data) {
                    StFree(bucket->data);
                    bucket->data = NULL;
                }

                StCheckedAlloc(bucket->data, size);
                bucket->data_size = size;
            }

            StMemcpy(bucket->data, data, size);

            return bucket;
        }
    }

    TinyBucket bucket = {0};
    bucket.hash = hash, bucket.data_size = size;
    StCheckedAlloc(bucket.data, bucket.data_size);
    StMemcpy(bucket.data, data, bucket.data_size);

    that->buckets[idx] = (TinyBucket*)TinyDAppendPro(that->buckets[idx], &bucket);
    that->length++;

    return &that->buckets[idx][len];
}

TinyBucket* TinyMapFind(const TinyMap* that, TinyHash hash) {
    if (!that || !that->buckets)
        return NULL;

    TinyBucket* buckets = that->buckets[TinyKey2Idx(hash)];

    for (size_t i = 0; i < TinyDLength(buckets); i++)
        if (buckets[i].hash == hash)
            return &buckets[i];

    return NULL;
}

char* TinyMapGet(const TinyMap* that, TinyHash hash) {
    TinyBucket* const bucket = TinyMapFind(that, hash);
    return bucket ? (char*)bucket->data : NULL;
}

void TinyMapErase(TinyMap* that, TinyHash hash) {
    if (!that || !that->buckets)
        return;

    const size_t idx = TinyKey2Idx(hash);
    TinyBucket* const buckets = that->buckets[idx];
    const size_t length = TinyDLength(buckets);

    for (size_t i = 0; i < length; i++) {
        if (buckets[i].hash == hash) {
            FreeTinyBucket(&buckets[i]);
            buckets[i] = buckets[length - 1];
            that->buckets[idx] = (TinyBucket*)TinyDPop(buckets);
            that->length--;

            break;
        }
    }
}

bool TinyMapNext(TinyMapIterator* iter) {
    if (!iter->source || !iter->source->buckets)
        return false;

    while (iter->hash_idx < ST_TINY_MAP_CAPACITY
           && iter->bucket_idx >= TinyDLength(iter->source->buckets[iter->hash_idx]))
        iter->hash_idx++, iter->bucket_idx = 0;

    if (iter->hash_idx >= ST_TINY_MAP_CAPACITY)
        return false;

    iter->bucket = &iter->source->buckets[iter->hash_idx][iter->bucket_idx++];
    iter->data = iter->bucket ? iter->bucket->data : NULL;

    return true;
}

TinyMapIterator TinyMapIter(TinyMap* that) {
    return (TinyMapIterator){.source = that};
}

size_t TinyDLength(const void* that) {
    return TinyDGetHead(that) ? TinyDGetHead(that)->length : 0;
}

size_t TinyDCapacity(const void* that) {
    return TinyDGetHead(that) ? TinyDGetHead(that)->capacity : 0;
}

size_t TinyDElementSize(const void* that) {
    return TinyDGetHead(that) ? TinyDGetHead(that)->elt_size : 0;
}

void* MakeTinyDPro(size_t capacity, size_t elt_size) {
    char* ptr = NULL;
    StCheckedAlloc(ptr, sizeof(TinyDHead) + elt_size * capacity);
    ptr += sizeof(TinyDHead);

    TinyDGetHead(ptr)->length = 0, TinyDGetHead(ptr)->capacity = capacity;
    TinyDGetHead(ptr)->elt_size = elt_size;

    return ptr;
}

void FreeTinyD(void* that) {
    if (that)
        StFree(TinyDGetHead(that));
}

void* TinyDShrink(void* that, size_t newlen) {
    if (that && newlen < TinyDLength(that))
        TinyDGetHead(that)->length = newlen;
    return that;
}

void* TinyDPop(void* that) {
    if (TinyDLength(that) > 0)
        return TinyDShrink(that, TinyDLength(that) - 1);
    return that;
}

void* TinyDPopFront(void* that) {
    if (TinyDLength(that) > 0)
        return TinyDErase(that, 0);
    return that;
}

void* TinyDErase(void* _this, size_t idx) {
    char* that = (char*)_this;

    if (idx >= TinyDLength(that))
        return that;

    const size_t size = TinyDElementSize(that);

    for (size_t i = idx; i < TinyDLength(that); i++)
        StMemcpy(that + i * size, that + (i + 1) * size, size);

    return TinyDPop(that);
}

void* TinyDAppendPro(void* _this, const void* ref) {
    char* that = (char*)_this;

    if (!TinyDGetHead(that))
        return NULL;

    const size_t length = TinyDGetHead(that)->length;
    const size_t elt_size = TinyDGetHead(that)->elt_size;
    const size_t no_cap = TinyDGetHead(that)->capacity;

    if (length == no_cap) {
        const size_t newcap
            = no_cap ? no_cap * ST_TINY_D_GROWTH_FACTOR : ST_TINY_D_INITIAL_CAPACITY;

        char* tmp = NULL;
        StCheckedAlloc(tmp, elt_size * newcap + sizeof(TinyDHead));
        tmp += sizeof(TinyDHead);

        TinyDGetHead(tmp)->capacity = newcap;
        TinyDGetHead(tmp)->length = length;
        TinyDGetHead(tmp)->elt_size = elt_size;

        StMemcpy(tmp, that, no_cap * elt_size);
        FreeTinyD(that), that = tmp;
    }

    StMemcpy(that + length * elt_size, ref, elt_size);
    TinyDGetHead(that)->length += 1;

    return that;
}

#undef TinyDGetHead
#undef TinyKey2Idx

#endif // S_TRUCTURES_IMPLEMENTATION

#endif // S_TRUCTURES_H
