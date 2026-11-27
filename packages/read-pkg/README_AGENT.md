# read-pkg — AI Agent Quick Ref

Zero-allocation package.json / cap.json parser and normalizer for CP, reusing normalize-package-data. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ReadPkgResult,
    rpkgIsSpace,
    rpkgStrEq,
    rpkgStartsWithAt,
    rpkgIsValueBoundary,
    rpkgSkipWs,
    rpkgFindStringEnd,
    rpkgSkipValue,
} from "read-pkg";
```

## Core Types & Constants

```cp
class ReadPkgResult { meta: PackageMetadata, normalized: NormalizedPackageResult, parsed: bool }
```

## API Signatures

```cp
fn rpkgIsSpace(ch: u8): bool;
fn rpkgStrEq(a: string, b: string): bool;
fn rpkgStartsWithAt(s: string, pos: usize, kw: string): bool;
fn rpkgIsValueBoundary(s: string, pos: usize): bool;
fn rpkgSkipWs(s: string, pos: usize): usize;
fn rpkgFindStringEnd(s: string, open_quote: usize): usize?;
fn rpkgSkipValue(s: string, pos: usize): usize?;
fn rpkgValidateTopObject(json_text: string): bool;
fn rpkgFindTopLevelValue(json_text: string, key: string): usize?;
fn extractJsonStringField(json_text: string, key: string): string?;
fn extractJsonBoolField(json_text: string, key: string): bool?;
fn rpkgExtractRepository(json_text: string): string;
fn parsePackageJson(json_text: string, normalize: bool): ReadPkgResult;
```

## Minimal Usage

```cp
import { parsePackageJson } from "read-pkg";

pub fn main() {
    let result = parsePackageJson("example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"read-pkg"`; install via `cap add read-pkg`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
