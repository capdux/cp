[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# amazon-s3-uri

Zero-heap AWS S3 / R2 / MinIO URI parser (s3://, virtual-hosted-style, path-style) and bucket name validator for CP (compatible with npm amazon-s3-uri & Rust s3-uri)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:amazon-s3-uri` ([https://github.com/jvdl/amazon-s3-uri](https://github.com/jvdl/amazon-s3-uri)) / `crates.io:aws-sdk-s3` ([https://github.com/awslabs/aws-sdk-rust](https://github.com/awslabs/aws-sdk-rust))（原库名 `amazon-s3-uri` → CP 包名 `amazon-s3-uri`）

| Metadata | Value |
| --- | --- |
| Package | `amazon-s3-uri` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add amazon-s3-uri
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `S3Uri` | `class` | Fields: `style: u8, bucket: string, key: string, region: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `indexOfByte` | `(s: string, ch: u8): i64` | Invokes `indexOfByte` and returns `i64`. |
| `isValidBucketName` | `(name: string): bool` | Checks predicate `isValidBucketName` and returns `bool`. |
| `stripQueryAndFrag` | `(s: string): string` | Invokes `stripQueryAndFrag` and returns `string`. |
| `parseS3Uri` | `(input: string): S3Uri` | Parses input via `parseS3Uri` and returns `S3Uri`. |
| `formatS3Uri` | `(bucket: string, key: string, out: []u8): usize` | Serializes/formats data via `formatS3Uri` returning `usize`. |

## Minimal Example

```cp
import { isValidBucketName } from "amazon-s3-uri";

pub fn main() {
    let result = isValidBucketName("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
