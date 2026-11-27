[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/cmark

> 版本：`0.31.1` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CommonMark 0.31.1 official C reference Markdown to HTML/XML/LaTeX renderer for CP

## 安装与导入

```bash
cap add @cap/cmark
```

```cp
import { cmark_markdown_to_html } from "@cap/cmark";
```

## 核心导出 API

- `export fn cmark_markdown_to_html(text: [*]&u8!, len: usize, options: i32): [*:0]u8!?;`
- `export fn cmark_version(): i32;`
- `export fn cmark_version_string(): [*:0]&u8!?;`
- `export fn free(ptr: *auto!?);`
- `export fn versionString(): string;`
- `export fn versionNumber(): i32;`
- `export fn markdownToHtml(md: string, opts: i32): !HtmlString;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`cmark@0.31.1`)，许可证：BSD-2-Clause
