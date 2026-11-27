[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ed25519

> 版本：`1.0.0` | 许可证：`Apache-2.0 OR MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 8032 Ed25519 signature & key containers, canonical scalar S < L validation, RFC 8410 SPKI/PKCS#8 DER and hex codecs in pure CP

## 安装与导入

```bash
cap add ed25519
```

```cp
import { ed25Zero32 } from "ed25519";
```

## 核心导出 API

- `export fn ed25Zero32(): [32]u8;`
- `export fn ed25Zero64(): [64]u8;`
- `export fn ed25GroupOrderL(): [32]u8;`
- `export fn ed25SpkiPrefix(): [12]u8;`
- `export fn ed25Pkcs8Prefix(): [16]u8;`
- `export fn ed25HexNibble(ch: u8): i32;`
- `export fn ed25HexDigit(n: u8): u8;`
- `export fn ed25EncodeHex(src: &[]u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ed25519@1.0.0`)，许可证：Apache-2.0 OR MIT
