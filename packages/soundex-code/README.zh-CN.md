[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# soundex-code

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap American Soundex phonetic algorithm encoder and phonetic match engine for CP (compatible with npm soundex-code & Rust rphonetic)

## 安装与导入

```bash
cap add soundex-code
```

```cp
import { toUpper } from "soundex-code";
```

## 核心导出 API

- `export fn toUpper(c: u8): u8;`
- `export fn isAlpha(c: u8): bool;`
- `export fn soundexDigit(u: u8): u8;`
- `export fn soundex(word: string, out: []u8): usize;`
- `export fn soundsLike(a: string, b: string): bool;`
- `export fn soundexDifference(a: string, b: string): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`soundex-code@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`rphonetic@1.0.0`)，许可证：MIT
