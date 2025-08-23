#include <stdio.h>
#include <stdlib.h>

#include "S_tructures.h"

static int counter = 1;

#define AssertEq(a, b)                                                                                                 \
	do {                                                                                                           \
		fprintf(stdout, "Test %d: ", counter);                                                                 \
		if ((a) != (b)) {                                                                                      \
			fprintf(stdout, "failed: '%s' != '%s'\n", (#a), (#b));                                         \
			fflush(stdout);                                                                                \
			exit(EXIT_FAILURE);                                                                            \
		}                                                                                                      \
		fprintf(stdout, "success!\n");                                                                         \
		fflush(stdout);                                                                                        \
		counter++;                                                                                             \
	} while (0)

void testMaps() {
	StTinyMap* map = NewTinyMap();

	int32_t data = 128;
	StMapPut(map, 1337, &data, sizeof(data));
	AssertEq(StMapGetI32(map, 1337), 128);

	const int count = 8;
	for (int i = 0; i < count; i++) {
		int32_t data = count ^ i;
		StMapPut(map, i ^ count, &data, sizeof(data));
	}
	for (int i = 0; i < count; i++)
		AssertEq(StMapGetI32(map, i ^ count), count ^ i);

	data = 228;
	StMapPut(map, StStrKey("Key1"), &data, sizeof(data));
	AssertEq(StMapGetI32(map, StStrKey("Key1")), 228);

	StMapNuke(map, StStrKey("Key1"));
	AssertEq(StMapGet(map, StStrKey("Key1")), NULL);

	AssertEq(StMapGetI32(map, 1337), 128);
	StMapNuke(map, 1337);
	AssertEq(StMapGet(map, 1337), NULL);

	StTinyMapIter iter = StMapIter(map);
	int iterCount = 0;
	while (StMapNext(&iter))
		iterCount++;
	AssertEq(iterCount, count);

	FreeTinyMap(map);
}

int main(int argc, char* argv[]) {
	testMaps();

	printf("All good!\n");
	fflush(stdout);
	return EXIT_SUCCESS;
}
