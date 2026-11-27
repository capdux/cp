[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# parse-link-header

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 8288 HTTP Link header parser and builder with GitHub/REST pagination helpers for CP

## 安装与导入

```bash
cap add parse-link-header
```

```cp
import { relMatches } from "parse-link-header";
```

## 核心导出 API

- `export fn relMatches(rel_attr: string, target: string): bool;`
- `export fn skipToNextEntry(s: string, start: usize): usize;`
- `export fn parseLinkHeader(header: string): LinkHeader;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendParam(out: []u8, pos: usize, name: string, val: string): usize;`
- `export fn formatLinkHeader(entries: []LinkEntry, out: []u8): string;`
- `export fn appendPageLink(out: []u8, start_pos: usize, base_url: string, sep: u8, target_page: usize, rel: string, emitted: usize): usize;`
- `export fn formatPaginationLinks(base_url: string, page: usize, total_pages: usize, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`parse-link-header@1.0.0`)，许可证：MIT
