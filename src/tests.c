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

#define run_test(fn) run_test_fr(#fn, fn)
static void run_test_fr(const char* name, void (*fn)()) {
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
    do {                                                                                           \
        if ((a) == (b))                                                                            \
            break;                                                                                 \
        fprintf(stdout, "'%s' == '%s'\n", (#a), (#b));                                             \
        fprintf(stdout, "FAIL: line %d\n", __LINE__);                                              \
        fflush(stdout), exit(EXIT_FAILURE);                                                        \
    } while (0)

static void map_simple_put_retrieve() {
    TinyMap map = {0};
    int32_t data = 128;

    TinyMapPut(&map, 1337, &data, sizeof(data));
    assert_eq(TinyMapGetI32(&map, 1337), 128);

    FreeTinyMap(&map);
}

static void map_string_key_and_nuke() {
    TinyMap map = {0};
    int32_t data = 228;

    TinyMapPut(&map, StStrKey("Key1"), &data, sizeof(data));
    assert_eq(TinyMapGetI32(&map, StStrKey("Key1")), data);

    TinyMapErase(&map, StStrKey("Key1"));
    assert_eq(TinyMapGet(&map, StStrKey("Key1")), NULL);

    FreeTinyMap(&map);
}

static void map_string_hash_and_nuke() {
    TinyMap map = {0};
    int32_t data = 67;

    TinyDictPut(&map, "SIX SEVEN", &data, sizeof(data));
    assert_eq(TinyDictGetI32(&map, "SIX SEVEN"), 67);

    TinyDictErase(&map, "SIX SEVEN");
    assert_eq(TinyDictGet(&map, "SIX SEVEN"), NULL);

    FreeTinyMap(&map);
}

static void map_retains_entries() {
    const size_t real_length = 1024;
    TinyMap map = {0};

    const int32_t data = 67;
    for (size_t i = 0; i < real_length; i++)
        TinyMapPut(&map, i, &data, sizeof(data));

    size_t iter_count = 0;
    TINY_MAP_FOREACH (&map, it)
        assert_eq(TinyMapGetI32(&map, iter_count++), data);

    assert_eq(iter_count, real_length);
    assert_eq(TinyMapLength(&map), real_length);

    FreeTinyMap(&map);
}

static void map_counts_length_correctly() {
    const size_t entries_count = 1024;
    TinyMap map = {0};

    for (size_t i = 0; i < entries_count; i++) {
        const int32_t data = 67;
        TinyMapPut(&map, i, &data, sizeof(data));
    }

    for (size_t i = 0; i < entries_count; i++)
        TinyMapErase(&map, i);

    assert_eq(TinyMapLength(&map), 0);

    FreeTinyMap(&map);
}

static void map_overwrites_values_on_put() {
    TinyMap map = {0};

    const int32_t d1 = 67;
    TinyDictPut(&map, "key", &d1, sizeof(d1));
    assert_eq(d1, TinyDictGetI32(&map, "key"));

    const int64_t d2 = 69;
    TinyDictPut(&map, "key", &d2, sizeof(d2));
    assert_eq(d2, TinyDictGetI32(&map, "key"));

    FreeTinyMap(&map);
}

static void reuse_map(TinyMap* map) {
    const int32_t d = 123;

    TinyDictPut(map, "hello", &d, sizeof(d));
    assert_eq(d, TinyDictGetI32(map, "hello"));
    assert_eq(1, TinyMapLength(map));

    FreeTinyMap(map);
    assert_eq(0, TinyMapLength(map));
}

static void map_safe_to_reuse() {
    TinyMap map = {0};
    reuse_map(&map);
    reuse_map(&map);
    reuse_map(&map);
}

static void test_hashmaps() {
    run_test(map_simple_put_retrieve);
    run_test(map_string_key_and_nuke);
    run_test(map_string_hash_and_nuke);
    run_test(map_retains_entries);
    run_test(map_counts_length_correctly);
    run_test(map_overwrites_values_on_put);
    run_test(map_safe_to_reuse);
    // TODO: test nukes...
}

static void d_append_doesnt_crash() {
    uint64_t* da = MakeTinyD(uint64_t);
    const size_t count = ST_TINY_D_INITIAL_CAPACITY * (ST_TINY_D_GROWTH_FACTOR * 8);

    for (uint64_t i = 0; i < count; i++)
        da = TinyDAppend(da, i);

    assert_eq(da[3], 3);
    assert_eq(da[count - 5], count - 5);

    FreeTinyD(da);
}

static void test_tinyDs() {
    run_test(d_append_doesnt_crash);
}

int main(int argc, char* argv[]) {
    test_hashmaps(), test_tinyDs();
    printf("All good!\n"), fflush(stdout);
    return EXIT_SUCCESS;
}
