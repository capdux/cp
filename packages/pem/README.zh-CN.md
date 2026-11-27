[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pem

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7468 PEM armor multi-block parser, validator, and 64-column wrapped encoder for certificates and keys

## 安装与导入

```bash
cap add pem
```

```cp
import { isValidLabel } from "pem";
```

## 核心导出 API

- `export fn isValidLabel(label: string): bool;`
- `export fn parsePemFrom(input: string, from_offset: usize): PemBlock;`
- `export fn parsePem(input: string): PemBlock;`
- `export fn parsePemBundle(input: string): PemBundle;`
- `export fn extractBase64(blk: PemBlock, out: []u8): string;`
- `export fn encodePem(label: string, base64_payload: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rustls-pemfile@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`pem-rfc7468@1.0.0`)，许可证：Apache-2.0
