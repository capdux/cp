# @cap/read-pkg

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:read-pkg` ([https://github.com/sindresorhus/read-pkg](https://github.com/sindresorhus/read-pkg))

Zero-allocation, Clean-room MIT implementation of `read-pkg` v9 core semantics for CP, reusing `@cap/normalize-package-data`.

## Features

- **`ReadPkgResult`**: `{ meta: PackageMetadata, normalized: NormalizedPackageResult, parsed: bool }`.
- **`parsePackageJson(json_text: string, normalize: bool): ReadPkgResult`**: Scans top-level `"name"`, `"version"`, `"license"`, `"repository"` (string or `{ "url": "..." }`), `"main"`, and `"private"` fields without heap allocations; optionally normalizes and validates via `@cap/normalize-package-data`.
- **`extractJsonStringField(json_text: string, key: string): string?`**: Zero-allocation top-level JSON string field extractor (skips nested objects/arrays).
- **`extractJsonBoolField(json_text: string, key: string): bool?`**: Zero-allocation top-level JSON boolean field extractor.

## License

MIT
