[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# he

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTML5 named entity & numeric character reference UTF-8 encoder/decoder and XSS escaper (he / escape-html / html-entities / Rust html-escape parity) for CP

## 安装与导入

```bash
cap add he
```

```cp
import { pushByte } from "he";
```

## 核心导出 API

- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn writeUtf8(out: []u8, pos: usize, raw_cp: u32): usize;`
- `export fn pushHexEntity(out: []u8, pos: usize, cp: u32): usize;`
- `export fn needsHtmlEscape(s: string): bool;`
- `export fn escapeHtml(input: string, out: []u8): string;`
- `export fn encodeNonAscii(input: string, out: []u8): string;`
- `export fn lookupNamedEntity(name: string): u32;`
- `export fn decodeHtmlEntities(input: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`he@1.0.0`)，许可证：MIT
