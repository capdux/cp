[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# weezl

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation GIF/TIFF/PDF LZW dictionary lossless compressor & decompressor with 9..12-bit variable code widths, Clear/EOI control codes, and KwKwK handling (Rust weezl parity) for CP

## 安装与导入

```bash
cap add weezl
```

```cp
import { hashPair } from "weezl";
```

## 核心导出 API

- `export fn hashPair(key: u32): usize;`
- `export fn encode(src: &[]u8, dst: []u8): i64;`
- `export fn decode(src: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`weezl@1.0.0`)，许可证：MIT
