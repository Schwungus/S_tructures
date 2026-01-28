#define S_TRUCTURES_IMPLEMENTATION
#include "S_tructures.h"

int main(int argc, char* argv[]) {
	(void)argc, (void)argv;

	StTinyMap* map = NewTinyMap();
	StMapPut(map, StHashStr("greeting"), "hello", strlen("hello!") + 1);
	StMapPut(map, StHashStr("name"), "Bob!", strlen("Bob!") + 1);

	ST_MAP_FOREACH (map, iter)
		printf("%s\n", (char*)iter.at->data);

	FreeTinyMap(map);
}
