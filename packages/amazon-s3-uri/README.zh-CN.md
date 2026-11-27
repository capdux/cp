[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# amazon-s3-uri

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap AWS S3 / R2 / MinIO URI parser (s3://, virtual-hosted-style, path-style) and bucket name validator for CP (compatible with npm amazon-s3-uri & Rust s3-uri)

## 安装与导入

```bash
cap add amazon-s3-uri
```

```cp
import { eqBytes } from "amazon-s3-uri";
```

## 核心导出 API

- `export fn eqBytes(a: string, b: string): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn endsWith(s: string, suffix: string): bool;`
- `export fn indexOfByte(s: string, ch: u8): i64;`
- `export fn isValidBucketName(name: string): bool;`
- `export fn stripQueryAndFrag(s: string): string;`
- `export fn parseS3Uri(input: string): S3Uri;`
- `export fn formatS3Uri(bucket: string, key: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`amazon-s3-uri@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`aws-sdk-s3@1.0.0`)，许可证：Apache-2.0
