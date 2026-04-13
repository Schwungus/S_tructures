# S_tructures

Useful dynamic data structures for plain C. Currently only implements tiny-hashmaps.

## Installation

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

## Tiny-hashmaps

For now, we only have "tiny" hashmaps to offer. Here's a quick appetizer to get you started:

```c
TinyMap map = {0};
TinyMapPut(&map, StHashStr("greeting"), "hello", strlen("hello!") + 1);
TinyMapPut(&map, StHashStr("name"), "Bob!", strlen("Bob!") + 1);

ST_FOREACH (&map, it)
    printf("%s\n", (char*)it.data);

FreeTinyMap(&map);
```

The tiny hashmaps have the following properties:

1. Tiny-maps don't store the whole key you put in them, only an 8 byte hash at most. This is also why you need to use `StHashStr` to get a tiny-map-compatible key from a string key.
2. Tiny-maps don't handle hash collisions, at all, due to the point above. If this issue breaks your program, you should probably buy a lottery ticket!
3. Values inside tiny-map buckets are dynamically typed. As long as you're handling your data in a sane way, you should be able to pointer-cast `StTinyBucket.data` to anything, without your program hardcrashing. E.g. when `it` is the bucket:

   ```c
   typedef struct { int x, y; } Enemy;

   void DrawEnemy(Enemy this) { /* ... */ }

   TinyMap enemies = ...;
   ST_FOREACH (enemies, it)
     DrawEnemy(*(Enemy*)it.data);
   ```

4. Iterating over key-value pairs isn't guaranteed to result in the pairs coming in the same order they were inserted.

Take a look into [our testbed](src/tests.c) for an overview of what other things our tiny-map hashmaps implementation can do.

## Tiny D's

Tiny D's work very similarly to Golang slices and provide an equivalent of the `append` idiom[^append]. The only functional difference from Go is the fact you have to free them manually as you always do with dynamically allocated memory:

```c
uint64_t* da = MakeTinyD(uint64_t);

da = TinyDAppend(da, 42);
da = TinyDAppend(da, 67);
printf("the sum of %d plus %d is: %d", da[0], da[1], da[0] + da[1]);

FreeTinyD(da);
```

As a sidenote, [tsoding](https://github.com/tsoding)'s latest videos inspired me to add tiny D's because he, too, realized the convenience of using dynamic arrays in your programs, as opposed to coding up custom linked-list datastructures for each dynamically growing container type. I just wanted to share my view on how these should be implemented: transparent for the end-user (the programmer) _and_ modelled after an existing, established idiom, which happens to be Go's slices and the `append` construct.

[^append]: See its intended usage in [the Go tour](https://go.dev/tour/moretypes/15).

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

Make sure to define `StAlloc` & `StFree` and `StMemset` & `StMemcpy` in pairs. Doing otherwise will not compile as it's a logic error; i.e. your custom `malloc` implementation should almost always come with its own custom `free` if you get the gist.

### Custom logger

`StLog` is called when spitting useful error messages, and can be customized as well. Here's how it's defined by default:

```c
#include <stdio.h>
#define StLog(...) \
  do { \
    fprintf(stdout, "[S_tr]: %c" __VA_ARGS__, '\n'); \
    fflush(stdout); \
  } while (0)
```

### `TinyBucket` cleanup function

You can set a custom cleanup function to call before deallocating data from a bucket. For example:

```c
static void CleanupTexture(void* ptr) {
    // pointer-cast & dereference since the function takes a value of type `Texture`:
    UnloadTexture(*(Texture*)ptr);
}

TinyMap map = {0};

Texture tx = LoadTexture("...");
TinyBucket* bucket = TinyDictPut(&map, "MyTex", &tx, sizeof(tx)); // copies `tx` into `map`
bucket->cleanup = CleanupTexture;

FreeTinyMap(&map); // `CleanupTexture` will be called with a pointer to texture data as the argument
```
