[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# html-escape

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTML entity escaper and unescaper (named entities + decimal/hex numeric character references) for CP (compatible with npm escape-html, he & Rust html-escape)

## 安装与导入

```bash
cap add html-escape
```

```cp
import { appendStr } from "html-escape";
```

## 核心导出 API

- `export fn appendStr(dst: []u8, pos: usize, src: string): usize;`
- `export fn needsEscape(input: string): bool;`
- `export fn escapeHtml(input: string, out: []u8): usize;`
- `export fn encodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn decodeNamedEntity(name: string): u32;`
- `export fn decodeNumericEntity(body: string): u32;`
- `export fn unescapeHtml(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`html-escape@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`html-entities@1.0.0`)，许可证：MIT
