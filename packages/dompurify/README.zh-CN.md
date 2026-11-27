[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dompurify

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTML sanitizer stripping blocked tags, event attributes, and unsafe URI schemes (npm DOMPurify parity) in pure CP

## 安装与导入

```bash
cap add dompurify
```

```cp
import { defaultSanitizeOptions } from "dompurify";
```

## 核心导出 API

- `export fn defaultSanitizeOptions(): SanitizeHtmlOptions;`
- `export fn dmpIsNameChar(c: u8): bool;`
- `export fn dmpHasPrefixCI(s: string, prefix: string): bool;`
- `export fn dmpWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn dmpIsBlockedContentTag(tag: string): bool;`
- `export fn dmpIsBlockedVoidTag(tag: string): bool;`
- `export fn dmpIsAllowedTag(tag: string, opts: SanitizeHtmlOptions): bool;`
- `export fn dmpIsEventAttr(attr: string): bool;`
- `export fn dmpIsSafeUrl(url: string): bool;`
- `export fn dmpIsSafeAttr(tag: string, attr: string, val: string, opts: SanitizeHtmlOptions): bool;`
- `export fn dmpFindTagClose(html: string, start: usize): usize;`
- `export fn dmpFindClosingTagEnd(html: string, start: usize, tag: string): usize;`
- `export fn sanitizeHtml(html: string, opts: SanitizeHtmlOptions, out: []u8): usize;`
- `export fn isSafeHtml(html: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`dompurify@1.0.0`)，许可证：Apache-2.0
