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

/// A unique identifier for a tiny-map entry. If you need more than 8 bytes per
/// key, try squeezing it into a hash using `StHashStr()`.
typedef uint64_t TinyKey;

/// An internal storage cell for `TinyMap`s. Essentially a singly linked list.
typedef struct TinyBucket {
	TinyKey key;
	void *data, (*cleanup)(void*);
	struct TinyBucket* next;
	size_t size;
} TinyBucket;

/// A tiny hashmap-like structure indexed with 8-byte keys.
typedef struct {
	TinyBucket* buckets[ST_TINY_MAP_CAPACITY];
} TinyMap;

/// A generic iterator over S_tructures.
typedef struct StIter {
	bool (*next)(struct StIter*);
	void *source, *bucket, *data;
	int64_t aux;
} StIter;

#define ST_MAP_FOREACH(map, iter) for (StIter iter = TinyMapIter(map); StIterNext(&(iter));)

#if __STDC_VERSION__ >= 201112L

#define ST_ITER(container) (_Generic(*(container), TinyMap: TinyMapIter)(container))
#define ST_FOREACH(container, iter) for (StIter iter = ST_ITER(container); StIterNext(&(iter));)

#endif

/// Copy up to 8 bytes from a string and return them as an `StTinyKey`.
TinyKey StStrKey(const char* s);

/// Hash a string of arbitrary length into an `StTinyKey`.
TinyKey StHashStr(const char* s);

/// Create a new `TinyMap`. Make sure to call `FreeTinyMap` after you're done
/// with it.
TinyMap* MakeTinyMap();
#define NewTinyMap MakeTinyMap

/// Cleanup a `TinyMap`.
void FreeTinyMap(TinyMap* this);

/// Returns the amount of key-value pairs inside this tiny-map.
size_t TinyMapLength(TinyMap* this);

/// Insert data into the tinymap. Allocates a chunk of memory and copies data
/// from input.
///
/// This also returns the resulting bucket in case you need to set a cleanup
/// function.
TinyBucket* TinyMapPut(TinyMap* this, TinyKey key, const void* data, int size);

/// An alias for `TinyMapPut` which accepts string keys and hashes them for you.
#define TinyDictPut(this, key, data, size) TinyMapPut((this), StHashStr((key)), (data), (size))

/// Find the bucket by input key, or return `NULL` if there is none.
TinyBucket* TinyMapFind(const TinyMap* this, TinyKey key);

/// Returns a pointer to an entry's data, if any. Spits out a `NULL` otherwise.
///
/// If you need to check the entry's actual size, use the full-form `TinyMapFind`.
void* TinyMapGet(const TinyMap* this, TinyKey key);

/// An alias for `TinyMapGet` which accepts string keys and hashes them for you.
#define TinyDictGet(this, key) TinyMapGet((this), StHashStr((key)))

/// Free the bucket and the data associated with a key.
void TinyMapErase(TinyMap* this, TinyKey key);

/// An alias for `TinyMapErase` which accepts string keys and hashes them for you.
#define TinyDictErase(this, key) TinyMapErase((this), StHashStr((key)))

/// Create an iterator over the values of a tiny-map.
///
/// Pointer-cast and dereference `.data` to get the value of the current entry.
/// Cast `.bucket` to `TinyBucket` to set/unset a cleanup function.
StIter TinyMapIter(void* this);

/// Return true and advance if there is an entry available. Return false and
/// null `.data` otherwise.
bool StIterNext(StIter* iter);

#ifdef S_TRUCTURES_IMPLEMENTATION

#ifndef StAlloc
#include <stdlib.h>
#define StAlloc malloc
#endif

#ifndef StFree
#include <stdlib.h>
#define StFree free
#endif

#ifndef StMemset
#include <string.h>
#define StMemset memset
#endif

#ifndef StMemcpy
#include <string.h>
#define StMemcpy memcpy
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
	type TinyMapGet##suffix(const TinyMap* this, TinyKey key) {                                \
		const void* data = TinyMapGet(this, key);                                          \
		return data ? *(type*)data : 0;                                                    \
	}                                                                                          \
                                                                                                   \
	type TinyDictGet##suffix(const TinyMap* this, const char* key) {                           \
		const void* data = TinyMapGet(this, StHashStr(key));                               \
		return data ? *(type*)data : 0;                                                    \
	}
#else
#define ST_MAKE_MAP_GET(suffix, type)                                                              \
	type TinyMapGet##suffix(const TinyMap*, TinyKey);                                          \
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
static const TinyKey StShuffleKey(const TinyKey key) {
	return key ^ (key >> (4 * sizeof(key)));
}

static TinyBucket* StNewTinyBucket(TinyKey key, const void* data, int size) {
	if (size < 1) { // TODO: bar behind a debug build check?
		StLog("Requested bucket size 0; catching on fire");
		return NULL;
	}

	TinyBucket* this = NULL;
	StCheckedAlloc(this, sizeof(*this));
	this->cleanup = NULL, this->next = NULL, this->key = key;
	this->size = size, this->data = NULL;
	StCheckedAlloc(this->data, this->size);
	StMemcpy(this->data, data, this->size);
	return this;
}

TinyKey StStrKey(const char* s) {
	static char buf[sizeof(TinyKey)] = {0};
	if (!s)
		return 0;
	for (int i = 0; i < sizeof(buf); i++)
		if (!s[i]) {
			StMemcpy(buf, s, i);
			StMemset(buf + i, 0xFF, sizeof(buf) - i);
			return *(TinyKey*)buf;
		}
	return *(TinyKey*)s;
}

// Thanks:
// 1. <https://github.com/toggins/Klawiatura/blob/bf6d4a12877ee850ea2c52ae5e976fbf5f787aee/src/K_memory.c#L5>
// 2. <https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function>

TinyKey StHashStr(const char* s) {
	TinyKey key = 0xcbf29ce484222325;
	for (; s && *s; s++)
		key ^= *(uint8_t*)s, key *= 0x00000100000001b3;
	return key;
}

TinyMap* NewTinyMap() {
	TinyMap* this = NULL;
	StCheckedAlloc(this, sizeof(*this));
	StMemset((void*)this->buckets, 0, sizeof(this->buckets));
	return this;
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
	StFree(this);
}

size_t TinyMapLength(TinyMap* this) {
	size_t length = 0;
	ST_FOREACH (this, iter)
		length++;
	return length;
}

TinyBucket* TinyMapPut(TinyMap* this, TinyKey key, const void* data, int size) {
	size_t idx = TinyKey2Idx(key);
	if (!this->buckets[idx]) {
		this->buckets[idx] = StNewTinyBucket(key, data, size);
		return this->buckets[idx];
	}

	TinyBucket* bucket = this->buckets[idx];
	while (bucket->key != key) {
		if (bucket->next) {
			bucket = bucket->next;
		} else {
			bucket->next = StNewTinyBucket(key, data, size);
			return bucket->next;
		}
	}

	StCleanupBucket(bucket);
	if (bucket->size != size) {
		if (bucket->data)
			StFree(bucket->data);
		StCheckedAlloc(bucket->data, size);
		bucket->size = size;
	}
	StMemcpy(bucket->data, data, size);

	return bucket;
}

TinyBucket* TinyMapFind(const TinyMap* this, TinyKey key) {
	TinyBucket* bucket = this->buckets[TinyKey2Idx(key)];
	while (bucket) {
		if (bucket->key == key)
			return bucket;
		bucket = bucket->next;
	}
	return NULL;
}

void* TinyMapGet(const TinyMap* this, TinyKey key) {
	TinyBucket* bucket = TinyMapFind(this, key);
	return bucket ? bucket->data : NULL;
}

void TinyMapErase(TinyMap* this, TinyKey key) {
	size_t idx = TinyKey2Idx(key);
	TinyBucket* bucket = this->buckets[idx];
	if (!bucket)
		return;
	if (bucket->key == key) {
		this->buckets[idx] = bucket->next;
		FreeSingleBucket(bucket);
		return;
	}
	while (bucket->next) {
		if (bucket->next->key == key) {
			bucket->next = bucket->next->next;
			FreeSingleBucket(bucket->next);
			return;
		}
		bucket = bucket->next;
	}
}

static bool TinyMapIterNext(StIter* iter) {
	if (iter->aux >= ST_TINY_MAP_CAPACITY)
		return false;

	if (iter->bucket)
		iter->bucket = ((TinyBucket*)iter->bucket)->next;
	while (!iter->bucket) {
		if (++iter->aux >= ST_TINY_MAP_CAPACITY)
			return false;
		iter->bucket = ((TinyMap*)iter->source)->buckets[iter->aux];
	}

	iter->data = iter->bucket ? ((TinyBucket*)iter->bucket)->data : NULL;
	return true;
}

StIter TinyMapIter(void* this) {
	return (StIter){
		.next = TinyMapIterNext,
		.source = this,
		.aux = -1,
	};
}

bool StIterNext(StIter* iter) {
	if (iter->source && iter->next && iter->next(iter))
		return true;
	iter->data = iter->bucket = NULL;
	return false;
}

#undef TinyKey2Idx

#endif
