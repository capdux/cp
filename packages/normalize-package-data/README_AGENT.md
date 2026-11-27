# normalize-package-data — AI Agent Quick Ref

Zero-allocation package metadata normalizer and validator for CP, reusing validate-npm-package-name and validate-npm-package-license. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class PackageMetadata { name: string, version: string, license: string, repository: string, /* +2 fields */ }
class NormalizedPackageResult { valid: bool, name_valid: bool, license_valid: bool, has_warnings: bool, /* +1 fields */ }
```

## API Signatures

```cp
fn npdIsOwnerRepoShorthand(s: string): bool;
fn npdWriteHostedUrl(base_url: string, slug: string, out: []u8): usize;
fn expandRepoShorthand(repo: string, out: []u8): usize;
fn normalizeEntryPath(entry: string): string;
fn cleanVersionString(ver: string): string;
fn normalizePackageData(meta: PackageMetadata): NormalizedPackageResult;
```

## Minimal Usage

```cp
import { npdIsOwnerRepoShorthand } from "normalize-package-data";

pub fn main() {
    let result = npdIsOwnerRepoShorthand("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"normalize-package-data"`; install via `cap add normalize-package-data`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
