[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sha2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap FIPS 180-4 SHA-256 & SHA-224 streaming hashers with hex output (crates.io sha2 equivalent, reuses cookie-signature)

## 安装与导入

```bash
cap add sha2
```

```cp
import { sh2HexDigit } from "sha2";
```

## 核心导出 API

- `export fn sh2HexDigit(n: u8): u8;`
- `export fn sh2EncodeHex(data: []u8, out: []u8): usize;`
- `export fn sha256Hex(s: string, out64: []u8): usize;`
- `export fn sha224Hex(s: string, out56: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`sha2@1.0.0`)，许可证：MIT OR Apache-2.0
