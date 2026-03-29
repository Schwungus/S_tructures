#include <stdio.h>
#include <stdlib.h>

static int malloc_counter = 0;

static void* counted_malloc(int size) {
	malloc_counter++;
	return malloc(size);
}

static void counted_free(void* ptr) {
	malloc_counter--;
	free(ptr);
}

#define S_TRUCTURES_IMPLEMENTATION
#define StAlloc counted_malloc
#define StFree counted_free
#include "S_tructures.h"

#define run_test(fn) _run_test(#fn, fn)
static void _run_test(const char* name, void (*fn)()) {
	static int test_counter = 0;
	printf("TEST #%d '%s':\n\n", ++test_counter, name);
	fflush(stdout);

	malloc_counter = 0;
	fn();
	if (malloc_counter) {
		printf("FAIL: leaked %d allocations\n", malloc_counter);
		fflush(stdout), exit(EXIT_FAILURE);
	}

	printf("PASS\n\n");
	fflush(stdout);
}

#define assert_eq(a, b)                                                                            \
	do {                                                                                       \
		if ((a) == (b))                                                                    \
			break;                                                                     \
		fprintf(stdout, "'%s' == '%s'\n", (#a), (#b));                                     \
		fprintf(stdout, "FAIL: line %d\n", __LINE__);                                      \
		fflush(stdout), exit(EXIT_FAILURE);                                                \
	} while (0)

static void map_simple_put_retrieve() {
	TinyMap* map = MakeTinyMap();
	int32_t data = 128;

	TinyMapPut(map, 1337, &data, sizeof(data));
	assert_eq(TinyMapGetI32(map, 1337), 128);

	FreeTinyMap(map);
}

static void map_string_key() {
	TinyMap* map = MakeTinyMap();
	int32_t data = 228;

	TinyMapPut(map, StStrKey("Key1"), &data, sizeof(data));
	assert_eq(TinyMapGetI32(map, StStrKey("Key1")), 228);

	FreeTinyMap(map);
}

static void map_string_key_and_nuke() {
	TinyMap* map = MakeTinyMap();
	int32_t data = 228;

	TinyMapErase(map, StStrKey("Key1"));
	assert_eq(TinyMapGet(map, StStrKey("Key1")), NULL);

	FreeTinyMap(map);
}

static void map_string_hash_and_nuke() {
	TinyMap* map = MakeTinyMap();
	int32_t data = 67;

	TinyMapPut(map, StHashStr("SIX SEVEN"), &data, sizeof(data));
	assert_eq(TinyMapGetI32(map, StHashStr("SIX SEVEN")), 67);

	TinyMapErase(map, StHashStr("SIX SEVEN"));
	assert_eq(TinyMapGet(map, StHashStr("SIX SEVEN")), NULL);

	FreeTinyMap(map);
}

static void map_retains_entries() {
	const int entry_count = 1024;
	TinyMap* map = MakeTinyMap();

	const int32_t data = 67;
	for (int i = 0; i < entry_count; i++)
		TinyMapPut(map, i, &data, sizeof(data));

	int iter_count = 0;
	ST_FOREACH (map, it)
		assert_eq(TinyMapGetI32(map, iter_count++), data);
	assert_eq(iter_count, entry_count);

	FreeTinyMap(map);
}

static void map_overwrites_values_on_put() {
	TinyMap* map = MakeTinyMap();

	const int32_t d1 = 67;
	TinyDictPut(map, "key", &d1, sizeof(d1));
	assert_eq(d1, TinyMapGetI32(map, StHashStr("key")));

	const int64_t d2 = 69;
	TinyDictPut(map, "key", &d2, sizeof(d2));
	assert_eq(d2, TinyMapGetI32(map, StHashStr("key")));

	FreeTinyMap(map);
}

static void test_hashmaps() {
	run_test(map_simple_put_retrieve);
	run_test(map_string_key);
	run_test(map_string_key_and_nuke);
	run_test(map_string_hash_and_nuke);
	run_test(map_retains_entries);
	run_test(map_overwrites_values_on_put);
	// TODO: test nukes...
}

int main(int argc, char* argv[]) {
	test_hashmaps();
	printf("All good!\n"), fflush(stdout);
	return EXIT_SUCCESS;
}
