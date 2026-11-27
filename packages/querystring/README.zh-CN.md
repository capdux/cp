[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# querystring

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation URL query string parser, stringifier & parameter extractor (Node.js querystring & npm query-string v9 parity) composing percent-encoding for CP

## 安装与导入

```bash
cap add querystring
```

```cp
import { escapeQuery } from "querystring";
```

## 核心导出 API

- `export fn escapeQuery(raw: string, out: []u8): usize;`
- `export fn unescapeQuery(raw: string, out: []u8): usize;`
- `export fn extractUrlQuery(url: string): string;`
- `export fn parseQueryString(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize;`
- `export fn stringifyQueryString(entries: &[]QueryEntry, sep: u8, eq: u8, out: []u8): usize;`
- `export fn pickQueryParam(url_or_query: string, target_key: string, out: []u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`querystring@1.0.0`)，许可证：MIT
