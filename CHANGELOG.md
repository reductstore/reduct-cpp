# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## Added

- **CMake**: Added `LANGUAGES CXX` to `project()` for explicit C++ language declaration, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **Build options**:
  - Added `REDUCT_CPP_USE_FETCHCONTENT` option for fetching dependencies via FetchContent, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Added detection of vcpkg usage via `CMAKE_TOOLCHAIN_FILE`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Added `RCPP_INSTALL` option to control installation, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Added `RCPP_TARGET_NAME` and target alias (`reductcpp::reductcpp`), [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **Dependencies**:
  - Added vcpkg configuration files: `vcpkg.json` and `vcpkg-configuration.json`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Added pkg-config fallback for `cpp-httplib`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **CMake package config**:
  - Added modern `reductcpp-config.cmake.in` with `find_dependency` support, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **CI**:
  - Added separate build-package actions for system, vcpkg, and conan, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Added `package_manager` matrix for unit tests (`vcpkg` and `conan`), [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Added `check-example` job for testing examples with different dependency sources, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Added `actionlint` hook to `.pre-commit-config.yaml`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **Examples**:
  - Added option to build examples with FetchContent (`REDUCT_CPP_EXAMPLE_USE_FETCHCONTENT`), [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).

- Check server API version and warn if it's too old, [PR-89](https://github.com/reductstore/reduct-cpp/pull/89)

## Changed

- **Dependencies**:
  - Updated minimal required versions for dependencies (`fmt` 9.1.0, `cpp-httplib` 0.14.3, `OpenSSL` 3.0.13), [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Unified dependency handling via `RCPP_DEPENDENCIES` list, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Moved `concurrentqueue` include path handling into CMake definitions, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **Build system**:
  - Refactored `InstallDependencies.cmake` to split FetchContent and system dependency modes, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Removed legacy Conan-specific handling from core build logic, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Improved include directories handling using `BUILD_INTERFACE` / `INSTALL_INTERFACE`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Removed `STATIC` library type requirement for `reductcpp` target, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Disabled `CXX_EXTENSIONS`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **Installation**:
  - Updated install rules to use modern `configure_package_config_file` and `write_basic_package_version_file`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **CI**:
  - Migrated to `ubuntu-24.04` runners, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Improved `cpplint` step to use `-print0`/`xargs -0` for robust file name handling, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Refactored CI workflows to separate dependency modes (system, fetchcontent, vcpkg, conan), [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **Examples**:
  - Switched example targets to link against `reductcpp::reductcpp`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
  - Updated CMake minimum version to 3.23 in examples, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- **README**:
  - Reorganized integration section: added system-wide build/install, FetchContent integration, and updated dependency list, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).


## Deprecated

- `each_n`, `each_s`, `limit` in `IBucket::QueryOptions` and `IBucket::ReplicationSettings` are deprecated, [PR-89](https://github.com/reductstore/reduct-cpp/pull/89)

## Removed

- Removed old `ReductCppConfig.cmake.in` and `ReductCppConfigVersion.cmake.in`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Removed `build-package-cmake` GitHub Action, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Removed redundant Conan dependency fetching logic from `InstallDependencies.cmake`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Removed `zlib` FetchContent declaration (now expected from system or package manager), [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Removed hardcoded installation of package in Conan CI action, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).

- Deprecated `include` and `exclude` parameters in `QueryOptions` and `ReplicationSettings`, [PR-92](https://github.com/reductstore/reduct-cpp/pull/92)

## Fixed

- Fixed `concurrentqueue` include path handling for different build environments (FetchContent, vcpkg), [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Fixed various CI workflow typos and paths, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Fixed example CMake scripts to support both system-installed and FetchContent modes, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Fixed `subscription.cc` example to comment out `.include` filter in `QueryOptions`, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).
- Fixed output directories for test binaries, [PR-90](https://github.com/reductstore/reduct-cpp/pull/90).

## [1.15.0] - 2025-05-05

### Added

- RS-628: Support `ext` parameter in `IBucket::QueryOptions`, [PR-87](https://github.com/reductstore/reduct-cpp/pull/87)

### Fixed

- Build on Windows with system OpenSSL, [PR-86](https://github.com/reductstore/reduct-cpp/pull/86)

### Changed

- `REDUCT_CPP_USE_CONAN` must be used to build with conan, [PR-86](https://github.com/reductstore/reduct-cpp/pull/86)

## [1.14.0] - 2025-02-25

### Added

- RS-550: Add `when` condition to replication settings, [PR-80](https://github.com/reductstore/reduct-cpp/pull/80)

### Changed

- RS-563: Support for Conan v2, [PR-83](https://github.com/reductstore/reduct-cpp/pull/83)

## [1.13.0] - 2024-12-04

### Added

- RS-543: Support conditional query, [PR-79](https://github.com/reductstore/reduct-cpp/pull/79)

## [1.12.0] = 2024-10-04

### Added

- RS-418: `IBucket::RemoveRecord`, `IBucket::RemoveRecordBatch`, `IBucket::RemoveQuery` methods for removing records, [PR-74](https://github.com/reductstore/reduct-cpp/pull/74)
- RS-389: Support `QuotaType::kHard`, [PR-75](https://github.com/reductstore/reduct-cpp/pull/75)
- RS-388: `IBucket::RenameEntry` to rename entry in bucket, [PR-76](https://github.com/reductstore/reduct-cpp/pull/76)
- RS-419: Add IBucket::Rename method to rename bucket, [PR-77](https://github.com/reductstore/reduct-cpp/pull/77)
- RS-462: Improve batching, [PR-78](https://github.com/reductstore/reduct-cpp/pull/78)

## [1.11.0] - 2024-08-19

### Added

- RS-31: `IBucket::Update` and `IBucket::UpdateBatch` methods for changing labels, [PR-72](https://github.com/reductstore/reduct-cpp/pull/72)

### Infrastructure

- RS-273: Refactor CI actions and add build for Windows, [PR-73](https://github.com/reductstore/reduct-cpp/pull/73)

## [1.10.0] - 2022-06-11

### Added

- RS-261: Support for `each_n` and `each_s` query parameters, [PR-68](https://github.com/reductstore/reduct-cpp/pull/68)
- `is_provisioned` flag to Token, [PR-69](https://github.com/reductstore/reduct-cpp/pull/69)
- `REDUCT_CPP_USE_STD_CHRONO` option to use `std::chrono::parse` instead of `date::parse`, [PR-67](https://github.com/reductstore/reduct-cpp/pull/67)
- RS-311: add `each_n` and `each_s` replication settings, [PR-70](https://github.com/reductstore/reduct-cpp/pull/70)

### Fixed

- Windows compilation, [PR-66](https://github.com/reductstore/reduct-cpp/pull/66)

### Changed

- RS-269: move documentation to main website, [PR-71](https://github.com/reductstore/reduct-cpp/pull/71)

## [1.9.0] - 2024-03-09

### Added

- RS-179: add license information to ServerInfo, [PR-65](https://github.com/reductstore/reduct-cpp/pull/65)

## [1.8.0] - 2023-12-02

### Added

- RS-44: Replication API, [PR-64](https://github.com/reductstore/reduct-cpp/pull/64)

### Changed:

- docs: update link to new website, [PR-63](https://github.com/reductstore/reduct-cpp/pull/63)

## [1.7.1] - 2023-11-04

### Fixed:

- RS-68: Fix writing blobs bigger than 512Kb, [PR-62](https://github.com/reductstore/reduct-cpp/pull/62)

## [1.7.0] - 2023-10-06

### Added

- support for ReductStore API v1.7 with `IBucket::WriteBatch` and `is_provisioned` field for `BucketInfo` and `Token`
  structs, [PR-60](https://github.com/reductstore/reduct-cpp/pull/60)

### Fixed

- RTHD build, [PR-61](https://github.com/reductstore/reduct-cpp/pull/61)

## [1.6.0] - 2023-08-15

### Added

- `IBucket.RemoveEntry` to remove entry from bucket, [PR-57](https://github.com/reductstore/reduct-cpp/pull/57)
- `QueryOption.limit` to limit the number of records returned by a
  query, [PR-58](https://github.com/reductstore/reduct-cpp/pull/58)

## [1.5.0] - 2023-06-30

### Added

- Support for batched records, [PR-54](https://github.com/reductstore/reduct-cpp/pull/54)
- Option to read only metadata of a record, [PR-55](https://github.com/reductstore/reduct-cpp/pull/55)

## [1.4.0] - 2023-06-04

### Added

- Implement continuous querying, [PR-53](https://github.com/reductstore/reduct-cpp/pull/53)

### Update

- Adapt to the new API v1.4, [PR-52](https://github.com/reductstore/reduct-cpp/pull/52)

## [1.3.0] - 2023-02-22

### Added

- Support for labels and content type, [PR-50](https://github.com/reductstore/reduct-cpp/pull/50)
- Use `x-reduct-error` header for error responses, [PR-51](https://github.com/reductstore/reduct-cpp/pull/51)

## [1.2.0] - 2022-12-20

### Added

- Support HTTP API v1.2 with IClient::Me method to get current token name and
  permission, [PR-48](https://github.com/reductstore/reduct-cpp/pull/48)

### Updated

- Improve mkdocs documentation [PR-46](https://github.com/reduct-storage/reduct-cpp/pull/46)

## [1.1.0] - 2022-12-02

### Added

- Support HTTP API v1.1 with token API, [PR-45](https://github.com/reduct-storage/reduct-cpp/pull/45)

## [1.0.1] - 2022-10-09

### Fixed

- Sending data in chunks, [PR-44](https://github.com/reduct-storage/reduct-cpp/pull/44)

## [1.0.0] - 2022-10-09

### Added

- Support HTTP API v1.0, [PR-42](https://github.com/reduct-storage/reduct-cpp/pull/42)

### Removed

- `Bucket.list`, [PR-42](https://github.com/reduct-storage/reduct-cpp/pull/42)

### Changed

- Refactor Bucket.Write and Bucket.Read methods, [PR-43](https://github.com/reduct-storage/reduct-cpp/pull/43)

## [0.8.0] - 2022-08-27

### Added

- Support HTTP API v0.8, [PR-39](https://github.com/reduct-storage/reduct-cpp/pull/39)

## [0.7.0] - 2022-08-07

### Added

- Support HTTP API v0.7, [PR-36](https://github.com/reduct-storage/reduct-cpp/pull/36)

### Fixed

- Cmake Fetch_Content integration, [PR-33](https://github.com/reduct-storage/reduct-cpp/pull/33)

### Changed

- `IBucket::List` is deprecated, [PR-36](https://github.com/reduct-storage/reduct-cpp/pull/36)
- Cmake doesn't build test and examples by default, [PR-37](https://github.com/reduct-storage/reduct-cpp/pull/37)

## [0.6.0] - 2022-06-29

### Added

- IBucket::GetOrCreateBucket, [PR-31](https://github.com/reduct-storage/reduct-cpp/pull/31/)
- Support HTTP API v0.6, [PR-30](https://github.com/reduct-storage/reduct-cpp/pull/30/)

### Changed

- Overload IBucket::Write and IBucket::Read methods to read/write by
  chunks, [PR-29](https://github.com/reduct-storage/reduct-cpp/pull/29/)

## [0.5.0] - 2022-05-23

### Added

- Support Reduct Storage API v0.5.0, [PR-25](https://github.com/reduct-storage/reduct-cpp/pull/25)

### Changed:

- Use README for index page of docs, [PR-22](https://github.com/reduct-storage/reduct-cpp/pull/22)
- Refactor tests, [PR-23](https://github.com/reduct-storage/reduct-cpp/pull/23)
- Install and test package in CI, [PR-24](https://github.com/reduct-storage/reduct-cpp/pull/24)

## [0.4.0] - 2022-04-01

### Added

- Support Reduct Storage API v0.4.0, [PR-16](https://github.com/reduct-storage/reduct-cpp/pull/16)

## [0.3.0] - 2022-03-19

### Added

- Support Reduct Storage API v0.3.0, [PR-13](https://github.com/reduct-storage/reduct-cpp/pull/13)

## [0.2.0] - 2022-03-09

### Added

- Support Reduct Storage API v0.2.0, [PR-9](https://github.com/reduct-storage/reduct-cpp/pull/9)
- Conan recipe, [PR-12](https://github.com/reduct-storage/reduct-cpp/pull/12)

### Changed

- Move documentation on mkdocs + RDT, [PR-11](https://github.com/reduct-storage/reduct-cpp/pull/11)

## [0.1.0] - 2022-02-11

### Added

- Initial implementation of Reduct Storage API v0.1.0

[Unreleased]: https://github.com/reduct-storage/reduct-cpp/compare/v1.15.0...HEAD
[1.15.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.14.0...1.15.0
[1.14.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.13.0...1.14.0
[1.13.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.12.0...1.13.0
[1.12.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.11.0...1.12.0
[1.11.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.10.0...1.11.0
[1.10.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.9.0...1.10.0
[1.9.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.8.0...1.9.0
[1.8.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.7.1...1.8.0
[1.7.1]: https://github.com/reduct-storage/reduct-cpp/compare/v1.7.0...1.7.1
[1.7.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.6.0...1.7.0
[1.6.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.5.0...1.6.0
[1.5.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.4.0...1.5.0
[1.4.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.3.0...1.4.0
[1.3.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.2.0...1.3.0
[1.2.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.1.0...1.2.0
[1.1.0]: https://github.com/reduct-storage/reduct-cpp/compare/v1.0.1...1.1.0
[1.0.1]: https://github.com/reduct-storage/reduct-cpp/compare/v1.0.0...1.0.1
[1.0.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.8.0...1.0.0
[0.8.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.7.0...v0.8.0
[0.7.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.6.0...v0.7.0
[0.6.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.5.0...v0.6.0
[0.5.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.4.0...v0.5.0
[0.4.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/reduct-storage/reduct-cpp/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/reduct-storage/reduct-cpp/releases/tag/v0.1.0
