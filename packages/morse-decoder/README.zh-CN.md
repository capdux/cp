[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# morse-decoder

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ITU-R M.1677 International Morse Code encoder and decoder supporting A-Z, 0-9, punctuation, and '/' word separators.

## 安装与导入

```bash
cap add morse-decoder
```

```cp
import { toUpperAscii } from "morse-decoder";
```

## 核心导出 API

- `export fn toUpperAscii(c: u8): u8;`
- `export fn isWs(c: u8): bool;`
- `export fn bytesEq(a: string, b: string): bool;`
- `export fn encodeChar(c: u8): string;`
- `export fn decodeSymbol(sym: string): u8;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn encodeText(input: string, out: []u8): usize;`
- `export fn decodeText(morse: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`morse-decoder@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`morse@1.0.0`)，许可证：MIT
