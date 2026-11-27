[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# futoin-hkdf

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation RFC 5869 HKDF-SHA256 key derivation function (RustCrypto hkdf parity)

## 安装与导入

```bash
cap add futoin-hkdf
```

```cp
import { hkdfExtract } from "futoin-hkdf";
```

## 核心导出 API

- `export fn hkdfExtract(salt: []u8, ikm: []u8, out_prk32: []u8): bool;`
- `export fn hkdfExpand(prk32: []u8, info: []u8, out_okm: []u8): bool;`
- `export fn hkdfDerive(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`futoin-hkdf@1.0.0`)，许可证：MIT
