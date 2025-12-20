# Repository Guidelines

## Project Structure & Modules
- `src/reduct`: public SDK surface (`client`, `bucket`, `error`, `result`, `http_options`); `src/reduct/internal`: HTTP transport, serialization, and time parsing helpers.
- `tests`: Catch2 runner in `tests/test.cc` with API scenarios in `tests/reduct/*_test.cc`; `tests/fixture.h` handles cleanup/fixtures against a live server.
- `examples`: minimal client usage samples; use them as templates for docs or quick checks.
- `cmake/`: dependency setup and install helpers; `conanfile.py`, `vcpkg*.json`: package manager configs.
- CMake builds default to `build/`; keep generated artifacts out of `src/` and `tests/`.

## Build, Test & Dev Commands
- Configure: `cmake -S . -B build [-DREDUCT_CPP_USE_FETCHCONTENT=ON] [-DREDUCT_CPP_ENABLE_TESTS=ON]`.
- Build: `cmake --build build [--config Release]`.
- Tests: after enabling tests, `cmake --build build --target reduct-tests` then `ctest --test-dir build` or run `./build/bin/reduct-tests`.
- Conan flow: `conan install . --build=missing && cmake --preset conan-release && cmake --build --preset conan-release --config Release`.
- Install SDK system-wide when needed: `sudo cmake --install build`.

## Coding Style & Naming
- C++20, 2-space indentation, keep lines ≤120 chars (`CPPLINT.cfg`); prefer initializer lists and `auto` only when it aids readability.
- Classes/types/methods use `CamelCase` (e.g., `IClient::GetBucket`); locals/parameters stay `snake_case`.
- Keep headers self-contained; minimize headers in `.cc` files to what is required; avoid raw `new`/`delete` in favor of RAII utilities.

## Testing Guidelines
- Framework: Catch2 v2 via `FetchContent`.
- Tests live in `tests/reduct/*_test.cc`; name cases to mirror API behaviors being verified.
- Integration tests expect a reachable ReductStore at `http://127.0.0.1:8383`; set `REDUCT_CPP_TOKEN_API` for auth. Fixture purges `test_*` buckets/tokens/replications before each run.
- Add or extend tests with new APIs, bug fixes, and boundary conditions that touch storage state.

## Commit & PR Guidelines
- Commits: concise, imperative summaries (`Fix parsing server url`), optionally append issue refs like `(#102)`.
- PRs: describe intent and behavior changes, link issues, call out API impacts, include test commands/output, and note any server or env requirements.
- Ensure builds/tests pass before review; include screenshots only when modifying docs or examples.

## Development Workflow
Before pushing changes, always complete these steps in order:

1. **Run cpplint** - Ensure code follows style guidelines:
   ```bash
   pip install "cpplint<2"
   find src/ \( -name "*.cc" -o -name "*.h" \) -print0 | xargs -0 cpplint
   find tests/ \( -name "*.cc" -o -name "*.h" \) -print0 | xargs cpplint
   ```

2. **Build the code** - Verify compilation succeeds:
   ```bash
   cmake -S . -B build -DREDUCT_CPP_USE_FETCHCONTENT=ON -DREDUCT_CPP_ENABLE_TESTS=ON
   cmake --build build
   ```

3. **Test against both server versions** - Ensure backward compatibility:
   ```bash
   # Test with development version (main - new features)
   docker run -d -p 8383:8383 --name reductstore-test reduct/store:main
   ./build/bin/reduct-tests
   docker stop reductstore-test && docker rm reductstore-test
   
   # Test with stable version (latest)
   docker run -d -p 8383:8383 --name reductstore-test reduct/store:latest
   ./build/bin/reduct-tests "~[1_18]"  # Exclude v1.18+ specific tests
   docker stop reductstore-test && docker rm reductstore-test
   ```

4. **Reference GitHub Actions** - See `.github/workflows/ci.yml` for the complete CI pipeline that runs cpplint, builds on multiple platforms, and tests against both `reduct/store:main` and `reduct/store:latest`.

## Documentation Guidelines
- **Do not update README.md or create examples for new features unless explicitly requested in the issue description.**
- Keep documentation changes minimal and focused on the specific requirements outlined in the issue.
- AGENTS.md is the appropriate place for development guidelines and internal documentation, not feature documentation.
