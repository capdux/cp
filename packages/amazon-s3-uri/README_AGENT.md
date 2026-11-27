# amazon-s3-uri@1.0.0 (Agent Quickref)

> Zero-heap AWS S3 / R2 / MinIO URI parser (s3://, virtual-hosted-style, path-style) and bucket name validator for CP (compatible with npm amazon-s3-uri & Rust s3-uri) | License: MIT
> Import: `import { ... } from "amazon-s3-uri";` | Full types: `src/index.d.cp`

## Functions
- `export fn eqBytes(a: string, b: string): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn endsWith(s: string, suffix: string): bool;`
- `export fn indexOfByte(s: string, ch: u8): i64;`
- `export fn isValidBucketName(name: string): bool;`
- `export fn stripQueryAndFrag(s: string): string;`
- `export fn parseS3Uri(input: string): S3Uri;`
- `export fn formatS3Uri(bucket: string, key: string, out: []u8): usize;`
