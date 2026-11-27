[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# boyer-moore-magiclen

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Boyer-Moore-Horspool sublinear substring search with reusable 256-byte bad-character table, reverse search, counting, and ASCII case-insensitive search.

## 安装与导入

```bash
cap add boyer-moore-magiclen
```

```cp
import { toLowerAscii } from "boyer-moore-magiclen";
```

## 核心导出 API

- `export fn toLowerAscii(c: u8): u8;`
- `export fn compilePattern(needle: &[]u8): BmhPattern;`
- `export fn findWithCompiled(haystack: &[]u8, needle: &[]u8, compiled: BmhPattern): i64;`
- `export fn findFirst(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn findLast(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn countNonOverlapping(haystack: &[]u8, needle: &[]u8): usize;`
- `export fn findIgnoreCaseAscii(haystack: string, needle: string): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`boyer-moore-magiclen@1.0.0`)，许可证：MIT
