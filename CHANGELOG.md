# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- RS-418: `IBucket::RemoveRecord`, `IBucket::RemoveRecordBatch`, `IBucket::RemoveQuery` methods for removing records, [PR-74](https://github.com/reductstore/reduct-cpp/pull/74)
- RS-389: Support `QuotaType::kHard`, [PR-75](https://github.com/reductstore/reduct-cpp/pull/75)
- RS-388: `IBucket::RenameEntry` to rename entry in bucket, [PR-76](https://github.com/reductstore/reduct-cpp/pull/76)
- RS-419: Add IBucket::Rename method to rename bucket, [PR-77](https://github.com/reductstore/reduct-cpp/pull/77)

## [1.11.0] - 2022-08-19

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

- `Bucket.list`,  [PR-42](https://github.com/reduct-storage/reduct-cpp/pull/42)

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

[Unreleased]: https://github.com/reduct-storage/reduct-cpp/compare/v1.11.0...HEAD

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
