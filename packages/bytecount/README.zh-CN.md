[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bytecount

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap fast byte frequency counting, SWAR 4-byte parallel counter, and UTF-8 char/line counters (crates.io bytecount equivalent, reuses @cap/memchr)

## 安装与导入

```bash
cap add bytecount
```

```cp
import { bcSumVec } from "bytecount";
```

## 核心导出 API

- `export fn bcSumVec(acc: BcVec16): usize;`
- `export fn count(haystack: &[]u8, needle: u8): usize;`
- `export fn countFast(haystack: &[]u8, needle: u8): usize;`
- `export fn numChars(utf8_bytes: &[]u8): usize;`
- `export fn countLines(buf: &[]u8): usize;`
- `export fn countAsciiWhitespace(buf: &[]u8): usize;`
- `export fn countInRange(buf: &[]u8, lo: u8, hi: u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bytecount@1.0.0`)，许可证：Apache-2.0 OR MIT
