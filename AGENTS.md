# Repository Guidelines

## Project Structure & Module Organization
`src/` contains library implementations such as `checksum.cpp`, `route_table.cpp`, and `tb_rate_limiter.cpp`. Public headers live in `include/` and should remain the single source of exported interfaces. Tests are in `test/` with `test_*.cpp` naming. Build outputs go to `bin/`. Windows-specific project files are under `win/` (`net_toy.sln`, `slib.vcxproj`).

## Build, Test, and Development Commands
Prefer CMake for new development. Use `cmake -S . -B build` to configure, `cmake --build build` to compile, and `ctest --test-dir build --output-on-failure` to run tests. If GoogleTest is installed in a custom location, pass `-DCMAKE_PREFIX_PATH=<gtest_install_dir>` during configure. Use `cmake -S . -B build -DBUILD_TESTING=OFF` when you only need the library. The existing `Makefile` remains available as a legacy path: `make`, `make test`, and `make clean`. On Windows, `win/net_toy.sln` is still useful for IDE debugging or MSVC validation.

Example:
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Coding Style & Naming Conventions
Target `C++11`; keep code portable across GCC and MSVC. Follow the existing style: 4-space indentation, braces on the same line, and small focused functions. Use `snake_case` for files, functions, and variables (`route_table.cpp`, `str_to_ip`), and `PascalCase` for types (`RouteTable`, `Checksum`). Put public declarations in `include/` and keep helper functions `static` or in unnamed namespaces when possible. Avoid duplicate logic between modules and tests.

## Testing Guidelines
Tests use `GoogleTest` and are built through CMake when `BUILD_TESTING` is enabled. Add or extend `test/test_*.cpp` for each behavior change. Prefer helper functions or parameterized coverage over copy-pasted test blocks. Name tests by feature and scenario, for example `TEST(ChecksumTest, Tcp)` or `TEST(CIDR, Convert)`. Every code change should include a matching test or a clear reason why no test is needed.

## Commit & Pull Request Guidelines
Recent history uses Conventional Commit style prefixes such as `feat:` and `chore:`. Keep commit subjects short, imperative, and descriptive, for example `feat: add route table merge case`. Pull requests should explain the behavior change, list test coverage (`make test`), and note any platform-specific impact. Include screenshots only when the change affects Visual Studio project setup or other UI-visible tooling.

## Platform & Configuration Notes
The CMake build expects a C++11 toolchain and a locally installed `GoogleTest` package when tests are enabled. The legacy `Makefile` assumes `g++`, `ar`, and `GoogleTest` are already available. Keep source and Markdown files in UTF-8. Do not commit generated files from `bin/` or `build/`.
