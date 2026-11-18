# `include/` — Public and Third-Party Headers

This directory contains public C interface header files (`.h`) for your project, as well as headers for drop-in third-party C/C++ libraries.

---

## How It Works

1. **Recursive Include Path Discovery**:
   - The build system (`Makefile`) recursively discovers `include/` and **all nested subdirectories** (e.g., `include/`, `include/core/`, `include/SDL3/`, `include/my_lib/`).
   - Every detected directory is automatically prepended to the compiler command with `-I<path>`.
   - You can include headers placed directly in `include/` or nested folders without needing complex relative paths:
     ```c
     #include "app.h"
     #include <SDL3/SDL.h>  /* Provided by include/SDL3/ */
     ```

2. **Language Server & Tooling Sync**:
   - The automatically generated `compile_commands.json` receives the full list of `-I` search paths on every build.
   - **clangd** and **VS Code IntelliSense** immediately recognize all headers, providing full autocompletion, hover docs, and diagnostics without manual JSON configuration.

3. **Public API Contract**:
   - Files in `include/` represent the public interface of your project.
   - Both your application sources (`src/`) and unit tests (`tests/`) have full access to all headers in `include/`.
   - If your project is configured as a static library (`PROJECT_TYPE = static`) or shared library (`PROJECT_TYPE = shared`), the contents of `include/` are what consumers of your library will include in their code.

---

## Best Practices for Header Files

### 1. Robust Header Guards
Always protect headers against multiple inclusions using standard preprocessor guards:
```c
#ifndef MY_MODULE_H
#define MY_MODULE_H

/* Declarations */

#endif /* MY_MODULE_H */
```
Alternatively, `#pragma once` is supported by all modern compilers (GCC, Clang, MSVC).

### 2. Self-Contained Headers
Ensure each header includes everything it needs for its own types, constants, and function prototypes:
```c
#include <stddef.h>  /* size_t */
#include <stdint.h>  /* uint32_t, int64_t */
#include <stdbool.h> /* bool */
```
A consumer should never have to manually `#include <stddef.h>` before including your header.

### 3. C++ Compatibility
If your library might be used in C++ projects, wrap declarations in `extern "C"`:
```c
#ifdef __cplusplus
extern "C" {
#endif

int my_function(int a);

#ifdef __cplusplus
}
#endif
```

### 4. Doxygen / Docstrings
Document function contracts, parameter requirements, nullability, and return values directly in the header (see `include/app.h` for an example using `@brief`, `@param`, and `@return`).

---

## Third-Party Header-Only Libraries

You can drop single-header C libraries (such as `stb_image.h`, `cJSON.h`, `miniaudio.h`, or `sokol_gfx.h`) directly into `include/` or a subfolder like `include/third_party/`:
- Headers are immediately available across `src/` and `tests/`.
- For header-only libraries that require an implementation macro (e.g., `#define STB_IMAGE_IMPLEMENTATION`), place the `#define` and `#include` in a dedicated `.c` file inside `src/` (e.g., `src/stb_impl.c`).

---

## Starter Files Included

- **`app.h`**:
  Public application configuration (`AppConfig`), runtime state (`AppState`), and lifecycle interface (`app_default_config`, `app_init`, `app_event`, `app_iterate`, `app_quit`) wrapping SDL3 video/screen, audio, and camera.
- **`SDL3/`**:
  Pre-integrated SDL 3.4.20 headers (with a `.clang-format` file disabling reformatting of upstream headers).
