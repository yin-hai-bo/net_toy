# net_toy
Network Utilities for C/C++.

## Build With CMake

This repository now supports CMake-based builds.

### Prerequisites

- CMake 3.16 or newer
- A C++11 compiler
- GoogleTest installed locally when `BUILD_TESTING` is enabled

If CMake cannot find GoogleTest automatically, pass either `CMAKE_PREFIX_PATH` or `GTest_DIR` when configuring.

### Configure

From the repository root:

```powershell
cmake -S . -B build
```

If GoogleTest is installed in a custom location:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=D:\deps\googletest\install
```

To configure without tests:

```powershell
cmake -S . -B build -DBUILD_TESTING=OFF
```

### Build

```powershell
cmake --build build
```

For multi-config generators such as Visual Studio, specify a configuration:

```powershell
cmake --build build --config Release
```

### Run Tests

After a successful build:

```powershell
ctest --test-dir build --output-on-failure
```

With Visual Studio generators:

```powershell
ctest --test-dir build -C Release --output-on-failure
```
