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
- **Version-specific testing**: Always test against both `reductstore/store:main` (dev version with new features) and `reductstore/store:latest` (stable version) before pushing changes. Tag tests for new features (e.g., `[1_18]` for v1.18+ features) to exclude them when testing against stable versions.

## Commit & PR Guidelines
- Commits: concise, imperative summaries (`Fix parsing server url`), optionally append issue refs like `(#102)`.
- PRs: describe intent and behavior changes, link issues, call out API impacts, include test commands/output, and note any server or env requirements.
- Ensure builds/tests pass before review; include screenshots only when modifying docs or examples.

## Non-blocking Deletions (v1.18+)

Starting from ReductStore v1.18, bucket and entry deletions are non-blocking. When you delete a bucket or entry:

- Deletion happens in the background
- The `status` field in `BucketInfo` and `EntryInfo` indicates the state: `IBucket::Status::kReady` or `IBucket::Status::kDeleting`
- Operations on deleting buckets/entries return HTTP 409 (Conflict)
- Poll the status to wait for deletion to complete

### Example Usage

```cpp
// Check bucket status
auto [info, err] = bucket->GetInfo();
if (info.status == IBucket::Status::kDeleting) {
    std::cout << "Bucket is being deleted" << std::endl;
}

// List entries with status
auto [entries, list_err] = bucket->GetEntryList();
for (const auto& entry : entries) {
    if (entry.status == IBucket::Status::kDeleting) {
        std::cout << "Entry " << entry.name << " is being deleted" << std::endl;
    }
}

// Wait for deletion to complete
bool deleted = false;
while (!deleted) {
    auto [buckets, list_err] = client->GetBucketList();
    if (list_err) break;
    
    deleted = true;
    for (const auto& bucket_info : buckets) {
        if (bucket_info.name == "my-bucket" && bucket_info.status == IBucket::Status::kDeleting) {
            deleted = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            break;
        }
    }
}
```

### Handling HTTP 409 Conflicts

Operations on deleting buckets/entries will return HTTP 409:

```cpp
auto write_err = bucket->Write("entry", std::nullopt, [](auto rec) {
    rec->WriteAll("data");
});

if (write_err.code == 409) {
    std::cerr << "Bucket is being deleted, cannot write" << std::endl;
}
```
