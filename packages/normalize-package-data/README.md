[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# normalize-package-data

Zero-allocation package metadata normalizer and validator for CP, reusing validate-npm-package-name and validate-npm-package-license.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:normalize-package-data` ([https://github.com/npm/normalize-package-data](https://github.com/npm/normalize-package-data))

| Metadata | Value |
| --- | --- |
| Package | `normalize-package-data` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add normalize-package-data
```

```cp
import {
    PackageMetadata,
    NormalizedPackageResult,
    npdIsOwnerRepoShorthand,
    npdWriteHostedUrl,
    expandRepoShorthand,
    normalizeEntryPath,
    cleanVersionString,
    normalizePackageData,
} from "normalize-package-data";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PackageMetadata` | `class` | Fields: `name: string, version: string, license: string, repository: string, main_entry: string, private_pkg: bool` |
| `NormalizedPackageResult` | `class` | Fields: `valid: bool, name_valid: bool, license_valid: bool, has_warnings: bool, warning_count: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `npdIsOwnerRepoShorthand` | `(s: string): bool` | Invokes `npdIsOwnerRepoShorthand` and returns `bool`. |
| `npdWriteHostedUrl` | `(base_url: string, slug: string, out: []u8): usize` | Invokes `npdWriteHostedUrl` and returns `usize`. |
| `expandRepoShorthand` | `(repo: string, out: []u8): usize` | Invokes `expandRepoShorthand` and returns `usize`. |
| `normalizeEntryPath` | `(entry: string): string` | Invokes `normalizeEntryPath` and returns `string`. |
| `cleanVersionString` | `(ver: string): string` | Invokes `cleanVersionString` and returns `string`. |
| `normalizePackageData` | `(meta: PackageMetadata): NormalizedPackageResult` | Invokes `normalizePackageData` and returns `NormalizedPackageResult`. |

## Minimal Example

```cp
import { npdIsOwnerRepoShorthand } from "normalize-package-data";

pub fn main() {
    let result = npdIsOwnerRepoShorthand("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
