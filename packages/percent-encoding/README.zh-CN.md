[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# percent-encoding

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation WHATWG URL Standard percent-encoding and decoding (Rust percent-encoding parity)

## 安装与导入

```bash
cap add percent-encoding
```

```cp
import { controlsSet } from "percent-encoding";
```

## 核心导出 API

- `export fn controlsSet(): AsciiSet;`
- `export fn fragmentSet(): AsciiSet;`
- `export fn querySet(): AsciiSet;`
- `export fn pathSet(): AsciiSet;`
- `export fn userinfoSet(): AsciiSet;`
- `export fn componentSet(): AsciiSet;`
- `export fn nonAlphanumericSet(): AsciiSet;`
- `export fn percentEncodeLen(input: &[]u8, set: AsciiSet): usize;`
- `export fn percentEncode(input: &[]u8, set: AsciiSet, out: []u8): usize;`
- `export fn percentDecode(input: string, out: []u8): usize;`
- `export fn percentDecodeStrict(input: string, out: []u8): i64;`
- `export fn percentDecodePlus(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`percent-encoding@1.0.0`)，许可证：MIT
