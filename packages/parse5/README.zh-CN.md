[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# parse5

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap WHATWG HTML5 streaming tokenizer, attribute extractor, and text stripper (clean-room port of npm parse5)

## 安装与导入

```bash
cap add parse5
```

```cp
import { p5IsWs } from "parse5";
```

## 核心导出 API

- `export fn p5IsWs(b: u8): bool;`
- `export fn p5IsTagNameChar(b: u8): bool;`
- `export fn p5Trim(s: string): string;`
- `export fn p5FindSubstr(s: string, start_pos: usize, needle: string): usize;`
- `export fn isVoidElement(tag: string): bool;`
- `export fn parseAttributes(raw_attrs: string, out_attrs: []HtmlAttr): usize;`
- `export fn extractTextContent(html: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`parse5@1.0.0`)，许可证：MIT
