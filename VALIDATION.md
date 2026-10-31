## Windows verification — 2026-10-30

- Visual Studio 2022 Build Tools (MSVC 19.44) with CMake; x64 Release compiled successfully on Windows.
- `ctest --test-dir build -C Release --output-on-failure`: 1/1 CTest target passed, executing five assertion-based test groups (FIFO/partial fills, replacement priority, validation, crossing amendments, randomized invariants).
- Previously tested with GCC and CMake in a local Linux container.
- No live broker/exchange connection or exchange certification was tested. All validation was local; no GitHub Actions.
