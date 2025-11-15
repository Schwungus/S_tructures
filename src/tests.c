#include <stdio.h>
#include <stdlib.h>

#include "S_tructures.h"

static int counter = 1;
#define AssertEq(a, b)                                                                                                 \
	do {                                                                                                           \
		fprintf(stdout, "test %2d '%s' == '%s'\n", counter, (#a), (#b)), fflush(stdout);                       \
		if ((a) != (b))                                                                                        \
			fprintf(stdout, "failed (line %d)!!!\n", __LINE__), fflush(stdout), exit(EXIT_FAILURE);        \
		counter++;                                                                                             \
	} while (0)

static const int testCount = 8;
static void testCleanup(void* ptr) {
	int32_t data = *(int32_t*)ptr;
	printf("%d ", data ^ testCount);
}

void testMaps() {
	// Simple map put.
	StTinyMap* map = NewTinyMap();
	int32_t data = 128;
	StMapPut(map, 0, &data, sizeof(data));
	AssertEq(StMapGetI32(map, 0), 128);
	FreeTinyMap(map);

	// Simple map put and retrieve.
	map = NewTinyMap();
	data = 128;
	StMapPut(map, 1337, &data, sizeof(data));
	AssertEq(StMapGetI32(map, 1337), 128);

	// Fill with junk data and test cleanup. `testCleanup` just prints out the values we put in.
	for (int i = 0; i < testCount; i++) {
		int32_t data = testCount ^ i;
		StTinyBucket* b = StMapPut(map, i ^ testCount, &data, sizeof(data));
		b->cleanup = testCleanup;
	}
	for (int i = 0; i < testCount; i++)
		AssertEq(StMapGetI32(map, i ^ testCount), testCount ^ i);
	// Still using the map with a { 1337: 128 } entry...

	// String key (direct mapping).
	data = 228;
	StMapPut(map, StStrKey("Key1"), &data, sizeof(data));
	AssertEq(StMapGetI32(map, StStrKey("Key1")), 228);

	// Nuke correctness check.
	StMapNuke(map, StStrKey("Key1"));
	AssertEq(StMapGet(map, StStrKey("Key1")), NULL);

	// Same with a hashed string.
	data = 67;
	StMapPut(map, StHashStr("SIX SEVEN"), &data, sizeof(data));
	AssertEq(StMapGetI32(map, StHashStr("SIX SEVEN")), 67);
	StMapNuke(map, StHashStr("SIX SEVEN"));
	AssertEq(StMapGet(map, StHashStr("SIX SEVEN")), NULL);

	// Just checking if the 1337 key is still there.
	AssertEq(StMapGetI32(map, 1337), 128);
	StMapNuke(map, 1337);
	AssertEq(StMapGet(map, 1337), NULL);

	StTinyMapIter iter = StMapIter(map);
	int iterCount = 0;
	while (StMapNext(&iter))
		iterCount++;
	AssertEq(iterCount, testCount);

	FreeTinyMap(map);
	printf("\n");
}

int main(int argc, char* argv[]) {
	testMaps();
	printf("All good!\n");
	fflush(stdout);
	return EXIT_SUCCESS;
}
