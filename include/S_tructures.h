#pragma once

#ifndef S_TRUCTURES_NOSTD
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef _MSC_VER
#define ST_NORETURN __declspec(noreturn)
#else
#define ST_NORETURN __attribute__((noreturn))
#endif

#define ST_TINY_MAP_CAPACITY (256)
typedef uint64_t StTinyKey;

typedef struct StTinyBucket {
	StTinyKey key;
	void *data, (*cleanup)(void*);
	struct StTinyBucket* next;
	size_t size;
} StTinyBucket;

/// A tiny hashmap-like structure indexed with 8-byte keys.
typedef struct {
	StTinyBucket* buckets[ST_TINY_MAP_CAPACITY];
} StTinyMap;

typedef enum {
	ST_ITERATOR_MAP,
} StIteratorKind;

/// Iterate over `StTinyMap` key-value pairs using `StMapIter()`.
typedef struct {
	const StIteratorKind kind;
	void *const source, *bucket, *data;
	int64_t aux;
} StIterator;

#define ST_MAP_FOREACH(map, iter) for (StIterator iter = StMapIter(map); StIterNext(&(iter));)

#if __STDC_VERSION__ >= 201112L
#define ST_FOREACH(map, iter) for (StIterator iter = _Generic((map), StTinyMap*: StMapIter)(map); StIterNext(&(iter));)
#endif

/// Map up to 8 bytes of a character string to an `StTinyKey`.
StTinyKey StStrKey(const char* s);

/// Hash a string of arbitrary length to use that as an `StTinyMap` key.
StTinyKey StHashStr(const char* s);

/// Create a new `StTinyMap`. Make sure to call `FreeTinyMap` afterwards.
StTinyMap* NewTinyMap();

/// Cleanup a `StTinyMap`.
void FreeTinyMap(StTinyMap* this);

/// Insert data into the tinymap. Allocates a chunk of memory and copies data from input.
///
/// Also returns the resulting bucket in case you need to set the cleanup function.
StTinyBucket* StMapPut(StTinyMap* this, StTinyKey key, const void* data, int size);

/// Find the bucket by input key, or return `NULL` if there is none.
StTinyBucket* StMapFind(const StTinyMap* this, StTinyKey key);

/// Free bucket & data associated with input key.
void StMapNuke(StTinyMap* this, StTinyKey key);

/// Create an iterator of values inside the map.
///
/// Use `.data` to get current entry. Use `StIterNext()` to go to the next value.
///
///
StIterator StTinyMapIter(void* this);

/// Return `true` and set `.at` to the next entry if there is one. Return `false` and set `.at` to `NULL` otherwise.
bool StIterNext(StIterator* iter);

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
#define StLog(...)                                                                                                     \
	do                                                                                                             \
		fprintf(stdout, "[S_tr]: %c" __VA_ARGS__, '\n'), fflush(stdout);                                       \
	while (0)
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

#define StOutOfJuice()                                                                                                 \
	do {                                                                                                           \
		StLog("Out of memory!!!");                                                                             \
		StDie();                                                                                               \
	} while (0)

#define StCheckedAlloc(var, size)                                                                                      \
	do {                                                                                                           \
		(var) = StAlloc((size));                                                                               \
		if (!(var))                                                                                            \
			StOutOfJuice();                                                                                \
	} while (0)

#endif

#ifdef S_TRUCTURES_IMPLEMENTATION
#define ST_MAKE_MAP_GET(suffix, type)                                                                                  \
	type StMapGet##suffix(const StTinyMap* this, StTinyKey key) {                                                  \
		StTinyBucket* bucket = StMapFind(this, key);                                                           \
		return bucket ? *(type*)bucket->data : 0;                                                              \
	}
#else
#define ST_MAKE_MAP_GET(suffix, type) type StMapGet##suffix(const StTinyMap*, StTinyKey)
#endif

void* StMapGet(const StTinyMap* this, StTinyKey key)
#ifdef S_TRUCTURES_IMPLEMENTATION
{
	StTinyBucket* bucket = StMapFind(this, key);
	return bucket ? bucket->data : NULL;
}
#else
	;
#endif

ST_MAKE_MAP_GET(I16, int16_t);
ST_MAKE_MAP_GET(U16, uint16_t);
ST_MAKE_MAP_GET(I32, int32_t);
ST_MAKE_MAP_GET(U32, uint32_t);
ST_MAKE_MAP_GET(I64, int64_t);
ST_MAKE_MAP_GET(U64, uint64_t);

#undef ST_MAKE_MAP_GET

#ifdef S_TRUCTURES_IMPLEMENTATION

#define StKey2Idx(key) ((ST_TINY_MAP_CAPACITY - 1) & StShuffleKey(key))
static const StTinyKey StShuffleKey(const StTinyKey key) {
	return key ^ (key >> (4 * sizeof(key)));
}

static StTinyBucket* StNewTinyBucket(StTinyKey key, const void* data, int size) {
	if (size < 1) { // TODO: bar behind a debug build check?
		StLog("Requested bucket size 0; catching on fire");
		return NULL;
	}

	StTinyBucket* this = NULL;
	StCheckedAlloc(this, sizeof(*this));
	this->cleanup = NULL, this->next = NULL, this->key = key;
	this->size = size, this->data = NULL;
	StCheckedAlloc(this->data, this->size);
	StMemcpy(this->data, data, this->size);
	return this;
}

StTinyKey StStrKey(const char* s) {
	static char buf[sizeof(StTinyKey)] = {0};
	if (!s)
		return 0;
	for (int i = 0; i < sizeof(buf); i++)
		if (!s[i]) {
			StMemcpy(buf, s, i);
			StMemset(buf + i, 0xFF, sizeof(buf) - i);
			return *(StTinyKey*)buf;
		}
	return *(StTinyKey*)s;
}

// Thanks:
// 1. https://github.com/toggins/Klawiatura/blob/bf6d4a12877ee850ea2c52ae5e976fbf5f787aee/src/K_memory.c#L5
// 2. https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function

static const StTinyKey FNV_OFFSET = 0xcbf29ce484222325, FNV_PRIME = 0x00000100000001b3;
StTinyKey StHashStr(const char* s) {
	StTinyKey key = FNV_OFFSET;
	for (const char* c = s; s && *c; c++)
		key ^= (StTinyKey)(uint8_t)*c, key *= FNV_PRIME;
	return key;
}

StTinyMap* NewTinyMap() {
	StTinyMap* this = NULL;
	StCheckedAlloc(this, sizeof(*this));
	StMemset((void*)this->buckets, 0, sizeof(this->buckets));
	return this;
}

static void FreeSingleBucket(StTinyBucket* this) {
	if (this->data) {
		if (this->cleanup)
			this->cleanup(this->data);
		StFree(this->data);
		this->data = NULL;
	}
	this->next = NULL;
	StFree(this);
}

static void FreeBucketChain(StTinyBucket* this) {
	if (this) {
		FreeBucketChain(this->next);
		FreeSingleBucket(this);
	}
}

void FreeTinyMap(StTinyMap* this) {
	if (!this)
		return;
	for (int i = 0; i < ST_TINY_MAP_CAPACITY; i++)
		FreeBucketChain(this->buckets[i]);
	StFree(this);
}

StTinyBucket* StMapPut(StTinyMap* this, StTinyKey key, const void* data, int size) {
	int idx = StKey2Idx(key);
	if (!this->buckets[idx]) {
		this->buckets[idx] = StNewTinyBucket(key, data, size);
		return this->buckets[idx];
	}

	StTinyBucket* bucket = this->buckets[idx];
	if (bucket->key == key)
		goto edit;
	while (bucket->next) {
		if (bucket->key == key)
			goto edit;
		bucket = bucket->next;
	}

	bucket->next = StNewTinyBucket(key, data, size);
	return bucket->next;

edit:
	if (bucket->size == size)
		StMemcpy(bucket->data, data, size);
	else
		StLog("Your bucket doesn't store this much bruv");
	return bucket;
}

StTinyBucket* StMapFind(const StTinyMap* this, StTinyKey key) {
	StTinyBucket* bucket = this->buckets[StKey2Idx(key)];
	while (bucket) {
		if (bucket->key == key)
			return bucket;
		bucket = bucket->next;
	}
	return NULL;
}

void StMapNuke(StTinyMap* this, StTinyKey key) {
	int idx = StKey2Idx(key);
	StTinyBucket* bucket = this->buckets[idx];
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

StIterator StMapIter(void* this) {
	return (StIterator){
		.kind = ST_ITERATOR_MAP,
		.source = this,
		.aux = -1,
	};
}

static bool StMapIterNext(StIterator* iter) {
	if (iter->aux >= ST_TINY_MAP_CAPACITY)
		return false;

	if (iter->bucket)
		iter->bucket = ((StTinyBucket*)iter->bucket)->next;
	while (!iter->bucket) {
		if (++iter->aux >= ST_TINY_MAP_CAPACITY)
			return false;
		iter->bucket = ((StTinyMap*)iter->source)->buckets[iter->aux];
	}

	iter->data = iter->bucket ? ((StTinyBucket*)iter->bucket)->data : NULL;
	return true;
}

static bool StGenericIterNext(StIterator* iter) {
	switch (iter->kind) {
		case ST_ITERATOR_MAP:
			return StMapIterNext(iter);
		default:
			return false;
	}
}

bool StIterNext(StIterator* iter) {
	if (!iter->source) {
		iter->data = iter->bucket = NULL;
		return false;
	}

	bool result = StGenericIterNext(iter);
	if (!result)
		iter->data = iter->bucket = NULL;
	return result;
}

#undef StKey2Idx

#endif
