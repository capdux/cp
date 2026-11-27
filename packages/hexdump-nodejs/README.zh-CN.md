[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hexdump-nodejs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation canonical hexdump -C 16-byte-per-line binary debugger, spaced hex encoder, and first-diff locator in pure CP

## 安装与导入

```bash
cap add hexdump-nodejs
```

```cp
import { buildPrintableLut } from "hexdump-nodejs";
```

## 核心导出 API

- `export fn buildPrintableLut(): [256]u8;`
- `export fn formatLine(data: &[]u8, base_offset: usize, out: []u8): usize;`
- `export fn formatDump(data: &[]u8, out: []u8): usize;`
- `export fn encodeHexSpaced(data: &[]u8, uppercase: bool, out: []u8): usize;`
- `export fn firstDiffOffset(a: &[]u8, b: &[]u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`hexdump-nodejs@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`pretty-hex@1.0.0`)，许可证：MIT
