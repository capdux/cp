[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# oci-distribution

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation OCI / Docker image reference parser, validator, and normalizer for CP.

## 安装与导入

```bash
cap add oci-distribution
```

```cp
import { isValidTag } from "oci-distribution";
```

## 核心导出 API

- `export fn isValidTag(tag: string): bool;`
- `export fn isValidDigest(digest: string): bool;`
- `export fn parseImageRef(input: string): ImageRef;`
- `export fn formatNormalized(ref_in: ImageRef, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`oci-distribution@1.0.0`)，许可证：Apache-2.0
- **上游生态**：`npm` (`docker-image-parse@1.0.0`)，许可证：MIT
