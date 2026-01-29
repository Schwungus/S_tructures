# S_tructures

Useful dynamic data structures for plain C. Currently only implements hashmaps.

## Basic usage

### Installation

Include the library in your CMake project by modifying your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    S_tructures
    GIT_REPOSITORY https://github.com/Schwungus/S_tructures.git
    GIT_TAG master) # or pin to a specific version by commit SHA
FetchContent_MakeAvailable(S_tructures)

# Don't forget to add S_tructures to your program:
target_link_libraries(myProgram S_tructures)
```

Then add a `S_tructures.c` file with the library's implementation details:

```c
#define S_TRUCTURES_IMPLEMENTATION
#include "S_tructures.h" // IWYU pragma: keep
```

And you're ready to go! Just `#include "S_tructures.h"` in your code, and have fun using these crutches.

### Tiny-map hashmaps

For now, we only have "tiny" hashmaps to offer. Here's a quick appetizer to get you started:

```c
StTinyMap* map = NewTinyMap();
StMapPut(map, StHashStr("greeting"), "hello", strlen("hello!") + 1);
StMapPut(map, StHashStr("name"), "Bob!", strlen("Bob!") + 1);

ST_FOREACH (map, it)
    printf("%s\n", (char*)it.data);

FreeTinyMap(map), map = NULL;
```

The tiny hashmaps have the following properties:

1. They don't store the whole key you put in them, only an 8 byte hash. This is also why you need to use `StHashStr` to get a tiny-map-compatible key from a string key.
2. They don't handle hash collisions, at all, due to the point above. If this issue breaks your program, you should buy a lottery ticket!
3. They are orderless, i.e. they iterate in a "whatever" order. Don't rely on the order of insertion when doing a for-each over key-value pairs.

Take a look into [our testbed](src/tests.c) for an overview of what other things our tiny-map hashmaps implementation can do.

## Advanced use-cases

### Custom allocator

In your `S_tructures.c`, you can customize the memory allocator by defining `StAlloc`, `StFree`, `StMemset`, and `StMemcpy`. For example, to use memory utilities provided by SDL3, you can define:

```c
#include <SDL3/SDL_stdinc.h>
#define StAlloc SDL_malloc
#define StFree SDL_free
#define StMemset SDL_memset
#define StMemcpy SDL_memcpy

#define S_TRUCTURES_IMPLEMENTATION
#include "S_tructures.h" // IWYU pragma: keep
```

### Custom logger

`StLog` is called when spitting useful error messages, and can be customized as well. Here's how it's defined by default:

```c
#include <stdio.h>
#define StLog(...)                               \
    do {                                         \
        fprintf(stdout, "[S_tr]: " __VA_ARGS__); \
        fprintf(stdout, "\n");                   \
        fflush(stdout);                          \
    } while (0)
```

### `StTinyBucket` cleanup function

You can set a custom cleanup function to call before deallocating data from a bucket. For example:

```c
static void CleanupTexture(void* ptr) {
    // Pointer-cast & dereference since the function takes a value of type `Texture`:
    UnloadTexture(*(Texture*)ptr);
}

StTinyMap* map = NewTinyMap();

Texture tx = LoadTexture("...");
StTinyBucket* bucket = StMapPut(map, StStrKey("MyTex"), tx, sizeof(tx));
bucket->cleanup = CleanupTexture;

FreeTinyMap(map); // `CleanupTexture` will be called with a pointer to `tx` as the argument
```
