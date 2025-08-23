# S_tructures

Useful premade datastructures for us plain-C rawdoggers.

## Usage

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
#include "S_tructures.h"
```

And you're ready to go! Just `#include "S_tructures.h"` in your code, and have fun using these crutches. Look into [our testbed](tests.c) to see them in action.

## Customization

In your `S_tructures.c`, you can customize the memory allocator by defining `StAlloc`, `StFree`, `StMemset`, and `StMemcpy`. For example, to use memory utilities provided by SDL3, you can define:

```c
#include <SDL3/SDL_stdinc.h>
#define StAlloc SDL_malloc
#define StFree SDL_free
#define StMemset SDL_memset
#define StMemcpy SDL_memcpy

#define S_TRUCTURES_IMPLEMENTATION
#include "S_tructures.h"
```

`StLog` is called when spitting useful error messages, and can be customized as well. Here's how it's defined by default:

```c
#include <stdio.h>
#define StLog(...)                                                                                                     \
        do {                                                                                                           \
                fprintf(stdout, __VA_ARGS__);                                                                          \
                fflush(stdout);                                                                                        \
        } while (0)
```
