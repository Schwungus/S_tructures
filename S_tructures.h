#pragma once

#include <stdint.h>

#define ST_TINY_MAP_CAPACITY (256)
typedef uint64_t StTinyKey;

typedef struct StTinyBucket {
	StTinyKey key;
	void* data;
	struct StTinyBucket* next;
	size_t size;
} StTinyBucket;

/// A tiny hashmap-like structure indexed with 8-byte keys.
typedef struct StTinyMap {
	StTinyBucket* buckets[ST_TINY_MAP_CAPACITY];
} StTinyMap;

StTinyMap* StNewTinyMap();

void StFreeTinyMap(StTinyMap* this);

/// Insert data into the tinymap. Allocates a chunk of memory and copies data from input.
void StMapPut(StTinyMap* this, StTinyKey key, const void* data, int size);

/// Returns the bucket with specified key, or `NULL` if there is none.
StTinyBucket* StMapLookup(const StTinyMap* this, StTinyKey key);

#ifdef S_TRUCTURES_IMPLEMENTATION

#ifndef StAlloc
#include <stdlib.h>
#define StAlloc malloc
#endif

#ifndef StFree
#include <stdlib.h>
#define StFree free
#endif

#ifndef StLog
#include <stdio.h>
#define StLog(...)                                                                                                     \
	do {                                                                                                           \
		fprintf(stdout, __VA_ARGS__);                                                                          \
		fflush(stdout);                                                                                        \
	} while (0)
#endif

#include <string.h>

#endif

#ifdef S_TRUCTURES_IMPLEMENTATION
#define ST_MAKE_MAP_GET(suffix, type)                                                                                  \
	type StMapGet##suffix(const StTinyMap* this, StTinyKey key) {                                                  \
		StTinyBucket* bucket = StMapLookup(this, key);                                                         \
		return bucket == NULL ? 0 : *(type*)bucket->data;                                                      \
	}
#else
#define ST_MAKE_MAP_GET(suffix, type) type StMapGet##suffix(const StTinyMap* this, StTinyKey key)
#endif

ST_MAKE_MAP_GET(I32, int32_t);
ST_MAKE_MAP_GET(U32, uint32_t);
ST_MAKE_MAP_GET(I64, int64_t);
ST_MAKE_MAP_GET(U64, uint64_t);

#undef ST_MAKE_MAP_GET

#ifdef S_TRUCTURES_IMPLEMENTATION

static const StTinyKey StShuffleKey(const StTinyKey key) {
	return key ^ (key >> 32);
}

static StTinyBucket* StNewTinyBucket(StTinyKey key, const void* data, int size) {
	StTinyBucket* this = StAlloc(sizeof(*this));
	this->next = NULL;
	this->key = key;

	if (size < 1) { // TODO: bar behind a debug build check?
		StLog("Requested bucket size 0; catching on fire\n");
		this->size = 0;
		this->data = NULL;
		return this;
	}

	this->size = size;
	this->data = StAlloc(this->size);
	memcpy(this->data, data, this->size);
	return this;
}

StTinyMap* StNewTinyMap() {
	StTinyMap* this = StAlloc(sizeof(*this));
	memset((void*)this->buckets, 0, sizeof(this->buckets));
	return this;
}

static void StFreeBucket(StTinyBucket* this) {
	if (this == NULL)
		return;
	if (this->data != NULL)
		StFree(this->data);
	StFreeBucket(this->next);
	StFree(this);
}

void StFreeTinyMap(StTinyMap* this) {
	for (int i = 0; i < ST_TINY_MAP_CAPACITY; i++)
		StFreeBucket(this->buckets[i]);
	StFree(this);
}

void StMapPut(StTinyMap* this, StTinyKey key, const void* data, int size) {
	int idx = (ST_TINY_MAP_CAPACITY - 1) & StShuffleKey(key);
	if (this->buckets[idx] == NULL) {
		this->buckets[idx] = StNewTinyBucket(key, data, size);
		return;
	}

	StTinyBucket* bucket = this->buckets[idx];
	if (bucket->key == key)
		goto edit;
	while (bucket->next != NULL) {
		if (bucket->key == key)
			goto edit;
		bucket = bucket->next;
	}

	bucket->next = StNewTinyBucket(key, data, size);
	return;

edit:
	if (bucket->size == size)
		memcpy(bucket->data, data, size);
	else
		StLog("Your bucket doesn't store this much bruv\n");
}

StTinyBucket* StMapLookup(const StTinyMap* this, StTinyKey key) {
	const size_t idx = (ST_TINY_MAP_CAPACITY - 1) & StShuffleKey(key);
	StTinyBucket* bucket = this->buckets[idx];
	while (bucket != NULL) {
		if (bucket->key == key)
			return bucket;
		bucket = bucket->next;
	}
	return NULL;
}

#endif
