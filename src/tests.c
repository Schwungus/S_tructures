#include <stdio.h>
#include <stdlib.h>

#define S_TRUCTURES_IMPLEMENTATION
#include "S_tructures.h"

static int counter = 1;
#define AssertEq(a, b)                                                                                                 \
	do {                                                                                                           \
		fprintf(stdout, "test %2d '%s' == '%s'\n", counter, (#a), (#b)), fflush(stdout);                       \
		if ((a) != (b))                                                                                        \
			fprintf(stdout, "failed (line %d)!!!\n", __LINE__), fflush(stdout), exit(EXIT_FAILURE);        \
		counter++;                                                                                             \
	} while (0)

static const int test_entry_count = 8;
static void cleanup_test_entry(void* ptr) {
	int32_t data = *(int32_t*)ptr;
	printf("%d ", data ^ test_entry_count);
}

void test_hashmaps() {
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
	for (int i = 0; i < test_entry_count; i++) {
		int32_t data = test_entry_count ^ i;
		StTinyBucket* b = StMapPut(map, i ^ test_entry_count, &data, sizeof(data));
		b->cleanup = cleanup_test_entry;
	}
	for (int i = 0; i < test_entry_count; i++)
		AssertEq(StMapGetI32(map, i ^ test_entry_count), test_entry_count ^ i);
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

	int iter_count = 0;
	ST_FOREACH (map, it)
		iter_count++;
	AssertEq(iter_count, test_entry_count);

	FreeTinyMap(map);
	printf("\n");
}

int main(int argc, char* argv[]) {
	test_hashmaps();
	printf("All good!\n"), fflush(stdout);
	return EXIT_SUCCESS;
}
