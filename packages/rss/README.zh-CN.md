[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rss

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RSS 2.0 XML feed generator and CDATA-aware tag extractor in pure CP

## 安装与导入

```bash
cap add rss
```

```cp
import { pushSlice } from "rss";
```

## 核心导出 API

- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushXmlEscaped(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushTag(dst: []u8, pos: usize, tag: &[]u8, val: &[]u8): i64;`
- `export fn isSpace(b: u8): bool;`
- `export fn startsWithAt(haystack: &[]u8, pos: usize, needle: &[]u8): bool;`
- `export fn extractTagValue(xml: &[]u8, tag: &[]u8, start_pos: usize): &[]u8;`
- `export fn countItems(xml: &[]u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rss@1.0.0`)，许可证：MIT
