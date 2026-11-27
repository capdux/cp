# amazon-s3-uri — AI Agent Quick Ref

Zero-heap AWS S3 / R2 / MinIO URI parser (s3://, virtual-hosted-style, path-style) and bucket name validator for CP (compatible with npm amazon-s3-uri & Rust s3-uri) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    S3Uri,
    indexOfByte,
    isValidBucketName,
    stripQueryAndFrag,
    parseS3Uri,
    formatS3Uri,
} from "amazon-s3-uri";
```

## Core Types & Constants

```cp
class S3Uri { style: u8, bucket: string, key: string, region: string, /* +1 fields */ }
```

## API Signatures

```cp
fn indexOfByte(s: string, ch: u8): i64;
fn isValidBucketName(name: string): bool;
fn stripQueryAndFrag(s: string): string;
fn parseS3Uri(input: string): S3Uri;
fn formatS3Uri(bucket: string, key: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidBucketName } from "amazon-s3-uri";

pub fn main() {
    let result = isValidBucketName("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"amazon-s3-uri"`; install via `cap add amazon-s3-uri`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
