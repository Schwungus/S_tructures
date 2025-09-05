#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ST_TINY_MAP_CAPACITY (256)
typedef uint64_t StTinyKey;

typedef struct StTinyBucket {
	StTinyKey key;
	void *data, (*cleanup)(void*);
	struct StTinyBucket* next;
	size_t size;
} StTinyBucket;

/// A tiny hashmap-like structure indexed with 8-byte keys.
typedef struct StTinyMap {
	StTinyBucket* buckets[ST_TINY_MAP_CAPACITY];
} StTinyMap;

/// Iterate over `StTinyMap` key-value pairs using `StMapIter()`.
typedef struct StTinyMapIter {
	StTinyMap* source;
	StTinyBucket* at;
	int64_t index;
} StTinyMapIter;

/// Map a string literal to a `StTinyKey`.
StTinyKey StStrKey(const char* s);

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

/// Create an iterator of key-value pairs inside the map.
///
/// Use `.at` for current entry. Use `StMapNext()` to go to the next.
StTinyMapIter StMapIter(StTinyMap* this);

/// Return `true` and set `.at` to the next entry if there is one. Return `false` and set `.at` to `NULL` otherwise.
bool StMapNext(StTinyMapIter* iter);

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
	do {                                                                                                           \
		fprintf(stdout, "[S_tr]: " __VA_ARGS__);                                                               \
		fflush(stdout);                                                                                        \
	} while (0)
#endif

#define StOutOfJuice() StLog("Out of memory!!!\n")
#define StCheckedAlloc(var, size)                                                                                      \
	do {                                                                                                           \
		(var) = StAlloc((size));                                                                               \
		if ((var) == NULL) {                                                                                   \
			StOutOfJuice();                                                                                \
			return NULL;                                                                                   \
		}                                                                                                      \
	} while (0)

#endif

#ifdef S_TRUCTURES_IMPLEMENTATION
#define ST_MAKE_MAP_GET(suffix, type)                                                                                  \
	type StMapGet##suffix(const StTinyMap* this, StTinyKey key) {                                                  \
		StTinyBucket* bucket = StMapFind(this, key);                                                           \
		return bucket == NULL ? 0 : *(type*)bucket->data;                                                      \
	}
#else
#define ST_MAKE_MAP_GET(suffix, type) type StMapGet##suffix(const StTinyMap*, StTinyKey)
#endif

void* StMapGet(const StTinyMap* this, StTinyKey key)
#ifdef S_TRUCTURES_IMPLEMENTATION
{
	StTinyBucket* bucket = StMapFind(this, key);
	return bucket == NULL ? NULL : bucket->data;
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
		StLog("Requested bucket size 0; catching on fire\n");
		return NULL;
	}

	StTinyBucket* this = NULL;
	StCheckedAlloc(this, sizeof(*this));
	this->cleanup = NULL;
	this->next = NULL;
	this->key = key;

	this->size = size;
	this->data = NULL;
	StCheckedAlloc(this->data, this->size);
	StMemcpy(this->data, data, this->size);
	return this;
}

StTinyKey StStrKey(const char* s) {
	static char buf[sizeof(StTinyKey)] = {0};
	for (int i = 0; i < sizeof(buf); i++)
		if (s[i] == '\0') {
			StMemcpy(buf, s, i);
			StMemset(buf + i, 0xFF, sizeof(buf) - i);
			return *(StTinyKey*)buf;
		}
	return *(StTinyKey*)s;
}

StTinyMap* NewTinyMap() {
	StTinyMap* this = NULL;
	StCheckedAlloc(this, sizeof(*this));
	StMemset((void*)this->buckets, 0, sizeof(this->buckets));
	return this;
}

static void StNukeBucket(StTinyBucket* this) {
	if (this == NULL)
		return;
	if (this->data != NULL) {
		if (this->cleanup != NULL)
			this->cleanup(this->data);
		StFree(this->data);
		this->data = NULL;
	}
	this->next = NULL;
	StFree(this);
}

static void StFreeBucketForGood(StTinyBucket* this) {
	if (this != NULL) {
		StFreeBucketForGood(this->next);
		StNukeBucket(this);
	}
}

void FreeTinyMap(StTinyMap* this) {
	if (this == NULL)
		return;
	for (int i = 0; i < ST_TINY_MAP_CAPACITY; i++)
		StFreeBucketForGood(this->buckets[i]);
	StFree(this);
}

StTinyBucket* StMapPut(StTinyMap* this, StTinyKey key, const void* data, int size) {
	int idx = StKey2Idx(key);
	if (this->buckets[idx] == NULL) {
		this->buckets[idx] = StNewTinyBucket(key, data, size);
		return this->buckets[idx];
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
	return bucket->next;

edit:
	if (bucket->size == size)
		StMemcpy(bucket->data, data, size);
	else
		StLog("Your bucket doesn't store this much bruv\n");
	return bucket;
}

StTinyBucket* StMapFind(const StTinyMap* this, StTinyKey key) {
	StTinyBucket* bucket = this->buckets[StKey2Idx(key)];
	while (bucket != NULL) {
		if (bucket->key == key)
			return bucket;
		bucket = bucket->next;
	}
	return NULL;
}

void StMapNuke(StTinyMap* this, StTinyKey key) {
	int idx = StKey2Idx(key);
	StTinyBucket* bucket = this->buckets[idx];
	if (bucket == NULL)
		return;
	if (bucket->key == key) {
		this->buckets[idx] = bucket->next;
		StNukeBucket(bucket);
		return;
	}
	while (bucket->next != NULL) {
		if (bucket->next->key == key) {
			bucket->next = bucket->next->next;
			StNukeBucket(bucket->next);
			return;
		}
		bucket = bucket->next;
	}
}

StTinyMapIter StMapIter(StTinyMap* this) {
	StTinyMapIter iter;
	iter.source = this;
	iter.index = -1;
	iter.at = NULL;
	return iter;
}

bool StMapNext(StTinyMapIter* iter) {
	if (iter->source == NULL)
		return false;
	if (iter->index >= ST_TINY_MAP_CAPACITY)
		return false;

	if (iter->at != NULL)
		iter->at = iter->at->next;
	while (iter->at == NULL) {
		if (++iter->index >= ST_TINY_MAP_CAPACITY)
			return false;
		iter->at = iter->source->buckets[iter->index];
	}
	return true;
}

#undef StKey2Idx

#endif
