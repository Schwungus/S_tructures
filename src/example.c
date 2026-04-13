#define S_TRUCTURES_IMPLEMENTATION
#include "S_tructures.h"

int main(int argc, char* argv[]) {
	(void)argc, (void)argv;

	TinyMap map = {0};
	TinyDictPut(&map, "greeting", "hello", strlen("hello!") + 1);
	TinyDictPut(&map, "name", "Bob!", strlen("Bob!") + 1);

	ST_FOREACH (&map, it)
		printf("%s\n", (char*)it.data);

	FreeTinyMap(&map);
	return EXIT_SUCCESS;
}
