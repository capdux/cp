[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hkdf

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 5869 HKDF-SHA256 key derivation function (extract, expand, one-shot) (crates.io hkdf equivalent, reuses hmac)

## 安装与导入

```bash
cap add hkdf
```

```cp
import { hkdfSha256 } from "hkdf";
```

## 核心导出 API

- `export fn hkdfSha256(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`hkdf@1.0.0`)，许可证：MIT OR Apache-2.0
