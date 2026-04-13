#pragma once

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

/// An internal storage cell for `TinyMap`s. Essentially a singly linked list.
typedef struct TinyBucket {
	TinyHash hash;
	void *data, (*cleanup)(void*);
	size_t data_size;
	struct TinyBucket* next;
} TinyBucket;

/// A tiny hashmap-like structure indexed with 8-byte keys.
typedef struct {
	TinyBucket* buckets[ST_TINY_MAP_CAPACITY];
	size_t length;
} TinyMap;

/// An iterator over tiny-maps.
typedef struct {
	TinyMap* source;
	TinyBucket* bucket;
	size_t bucket_idx;
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
void FreeTinyMap(TinyMap* this);

/// Returns the amount of key-value pairs inside this tiny-map.
size_t TinyMapLength(const TinyMap* this);

/// Insert data into the tinymap. Allocates a chunk of memory and copies data
/// from input.
///
/// This also returns the resulting bucket in case you need to set a cleanup
/// function.
TinyBucket* TinyMapPut(TinyMap* this, TinyHash hash, const void* data, int size);

/// An shorthand for `TinyMapPut` which accepts string keys and hashes them for you.
#define TinyDictPut(this, hash, data, size) TinyMapPut((this), StHashStr((hash)), (data), (size))

/// Find the bucket by input key, or return `NULL` if there is none.
TinyBucket* TinyMapFind(const TinyMap* this, TinyHash hash);

/// An shorthand for `TinyMapFind` which accepts string keys and hashes them for you.
#define TinyDictFind(this, hash) TinyMapFind((this), StHashStr((hash)))

/// Returns a pointer to an entry's data, if any. Spits out a `NULL` otherwise.
///
/// If you need to check the entry's actual size, use the full-form `TinyMapFind`.
char* TinyMapGet(const TinyMap* this, TinyHash hash);

/// An shorthand for `TinyMapGet` which accepts string keys and hashes them for you.
#define TinyDictGet(this, hash) TinyMapGet((this), StHashStr((hash)))

/// Free the bucket and the data associated with a key.
void TinyMapErase(TinyMap* this, TinyHash hash);

/// An shorthand for `TinyMapErase` which accepts string keys and hashes them for you.
#define TinyDictErase(this, hash) TinyMapErase((this), StHashStr((hash)))

/// Creates an iterator over the values of a tiny-map.
///
/// Pointer-cast and dereference `.data` to get the value of the current entry. Cast `.bucket` to
/// `TinyBucket` to set/unset a cleanup function.
TinyMapIterator TinyMapIter(TinyMap* this);

/// Returns true and advances the iterator if there is an entry available inside the iterable.
/// Otherwise returns false.
bool TinyMapNext(TinyMapIterator* iter);

/// Creates a dynamic-array with the specified capacity and element-size.
void* MakeTinyDPro(size_t capacity, size_t elt_size);

/// A shorthand for `MakeTinyDPro` that creates a dynamic-array with a default capacity and the
/// element-size equal to the size requirement of the passed type.
#define MakeTinyD(T) ((T*)MakeTinyDPro(ST_TINY_D_INITIAL_CAPACITY, sizeof(T)))

/// Properly cleans up a tiny dynamic-array and its header.
void FreeTinyD(void* this);

#define TinyDLength(this) (TinyDGetHead((this))->length)
#define TinyDCapacity(this) (TinyDGetHead((this))->capacity)
#define TinyDElementSize(this) (TinyDGetHead((this))->elt_size)

/// Appends an element to the dynamic-array, growing it if necessary. DO NOT FORGET to assign the
/// result of this to the array you passed in.
void* TinyDAppendPro(void* this, const void* ref);

/// A shorthand for `TinyDPush` that accepts any value, not just pointers. DO NOT FORGET to assign
/// the result of this to the array you passed in.
///
/// (Ab)uses the GCC compound statement extension; may not work with non-mainstream compilers.
#define TinyDAppend(this, value)                                                                   \
	({                                                                                         \
		__typeof__(value) tmp = (value);                                                   \
		TinyDAppendPro((this), &tmp);                                                      \
	})

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
#define StLog(...)                                                                                 \
	do {                                                                                       \
		fprintf(stdout, "[S_tr]: %c" __VA_ARGS__, '\n');                                   \
		fflush(stdout);                                                                    \
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
	do {                                                                                       \
		StLog("Out of memory!!!");                                                         \
		StDie();                                                                           \
	} while (0)

#define StCheckedAlloc(var, size)                                                                  \
	do {                                                                                       \
		(var) = StAlloc((size));                                                           \
		if (!(var))                                                                        \
			StOutOfJuice();                                                            \
	} while (0)

#endif

#ifdef S_TRUCTURES_IMPLEMENTATION
#define ST_MAKE_MAP_GET(suffix, type)                                                              \
	type TinyMapGet##suffix(const TinyMap* this, TinyHash hash) {                              \
		const void* data = TinyMapGet(this, hash);                                         \
		return data ? *(type*)data : 0;                                                    \
	}                                                                                          \
                                                                                                   \
	type TinyDictGet##suffix(const TinyMap* this, const char* key) {                           \
		const void* data = TinyMapGet(this, StHashStr(key));                               \
		return data ? *(type*)data : 0;                                                    \
	}
#else
#define ST_MAKE_MAP_GET(suffix, type)                                                              \
	type TinyMapGet##suffix(const TinyMap*, TinyHash);                                         \
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

#define TinyDGetHead(ptr) ((TinyDHead*)((char*)(ptr) - sizeof(TinyDHead)))

static const TinyHash StShuffleKey(const TinyHash hash) {
	return hash ^ (hash >> (4 * sizeof(hash)));
}

static TinyBucket* StNewTinyBucket(TinyHash hash, const void* data, int size) {
	if (size < 1) { // TODO: bar behind a debug build check?
		StLog("Requested bucket size 0; catching on fire");
		return NULL;
	}

	TinyBucket* this = NULL;
	StCheckedAlloc(this, sizeof(*this));
	this->cleanup = NULL, this->next = NULL, this->hash = hash;
	this->data_size = size, this->data = NULL;
	StCheckedAlloc(this->data, this->data_size);
	StMemcpy(this->data, data, this->data_size);
	return this;
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

static void StCleanupBucket(TinyBucket* this) {
	if (this->cleanup && this->data)
		this->cleanup(this->data);
}

static void FreeSingleBucket(TinyBucket* this) {
	StCleanupBucket(this);
	if (this->data)
		StFree(this->data);
	StFree(this);
}

static void FreeBucketChain(TinyBucket* this) {
	if (this) {
		FreeBucketChain(this->next);
		FreeSingleBucket(this);
	}
}

void FreeTinyMap(TinyMap* this) {
	if (!this)
		return;
	for (int i = 0; i < ST_TINY_MAP_CAPACITY; i++)
		FreeBucketChain(this->buckets[i]);
	StMemset(this, 0, sizeof(*this));
}

size_t TinyMapLength(const TinyMap* this) {
	return this->length;
}

TinyBucket* TinyMapPut(TinyMap* this, TinyHash hash, const void* data, int size) {
	size_t idx = TinyKey2Idx(hash);
	if (!this->buckets[idx]) {
		this->buckets[idx] = StNewTinyBucket(hash, data, size);
		this->length++;
		return this->buckets[idx];
	}

	TinyBucket* bucket = this->buckets[idx];
	while (bucket->hash != hash) {
		if (bucket->next) {
			bucket = bucket->next;
		} else {
			bucket->next = StNewTinyBucket(hash, data, size);
			this->length++;
			return bucket->next;
		}
	}

	StCleanupBucket(bucket);
	if (bucket->data_size != (size_t)size) {
		if (bucket->data)
			StFree(bucket->data);
		StCheckedAlloc(bucket->data, size);
		bucket->data_size = size;
	}
	StMemcpy(bucket->data, data, size);

	return bucket;
}

TinyBucket* TinyMapFind(const TinyMap* this, TinyHash hash) {
	TinyBucket* bucket = this->buckets[TinyKey2Idx(hash)];
	while (bucket) {
		if (bucket->hash == hash)
			return bucket;
		bucket = bucket->next;
	}
	return NULL;
}

char* TinyMapGet(const TinyMap* this, TinyHash hash) {
	TinyBucket* bucket = TinyMapFind(this, hash);
	return bucket ? bucket->data : NULL;
}

void TinyMapErase(TinyMap* this, TinyHash hash) {
	size_t idx = TinyKey2Idx(hash);
	TinyBucket* bucket = this->buckets[idx];
	if (!bucket)
		return;
	if (bucket->hash == hash) {
		this->buckets[idx] = bucket->next;
		FreeSingleBucket(bucket);
		this->length--;
		return;
	}
	while (bucket->next) {
		if (bucket->next->hash == hash) {
			bucket->next = bucket->next->next;
			FreeSingleBucket(bucket->next);
			this->length--;
			return;
		}
		bucket = bucket->next;
	}
}

bool TinyMapNext(TinyMapIterator* iter) {
	if (iter->bucket_idx > ST_TINY_MAP_CAPACITY)
		return false;

	if (iter->bucket)
		iter->bucket = iter->bucket->next;

	while (!iter->bucket) {
		iter->bucket = iter->source->buckets[iter->bucket_idx];
		if (++iter->bucket_idx > ST_TINY_MAP_CAPACITY)
			return false;
	}

	iter->data = iter->bucket ? iter->bucket->data : NULL;

	return true;
}

TinyMapIterator TinyMapIter(TinyMap* this) {
	return (TinyMapIterator){.source = this};
}

void* MakeTinyDPro(size_t capacity, size_t elt_size) {
	char* ptr = NULL;
	StCheckedAlloc(ptr, sizeof(TinyDHead) + elt_size * capacity);
	ptr += sizeof(TinyDHead);

	TinyDLength(ptr) = 0, TinyDCapacity(ptr) = capacity;
	TinyDElementSize(ptr) = elt_size;

	return ptr;
}

void FreeTinyD(void* this) {
	if (this)
		StFree(TinyDGetHead(this));
}

void* TinyDAppendPro(void* this, const void* ref) {
	char* buf = this;

	const size_t length = TinyDLength(buf), elt_size = TinyDElementSize(buf),
		     no_cap = TinyDCapacity(buf);

	if (length == no_cap) {
		const size_t newcap
			= no_cap ? no_cap * ST_TINY_D_GROWTH_FACTOR : ST_TINY_D_INITIAL_CAPACITY;

		char* tmp = NULL;
		StCheckedAlloc(tmp, elt_size * newcap + sizeof(TinyDHead));
		tmp += sizeof(TinyDHead);

		TinyDCapacity(tmp) = newcap, TinyDLength(tmp) = length;
		TinyDElementSize(tmp) = elt_size;
		StMemcpy(tmp, buf, no_cap * elt_size);

		FreeTinyD(buf), buf = tmp;
	}

	StMemcpy(buf + length * elt_size, ref, elt_size);
	TinyDLength(buf)++;

	return buf;
}

#undef TinyKey2Idx

#endif
