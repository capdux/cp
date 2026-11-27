[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ascii85

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Adobe Ascii85 (PostScript / PDF base-85 with 'z' zero compression and <~...~> delimiters) encoder and decoder for CP.

## 安装与导入

```bash
cap add ascii85
```

```cp
import { maxEncodedLen } from "ascii85";
```

## 核心导出 API

- `export fn maxEncodedLen(byte_len: usize, with_delimiters: bool): usize;`
- `export fn encode(data: &[]u8, with_delimiters: bool, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ascii85@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`ascii85@1.0.0`)，许可证：MIT
