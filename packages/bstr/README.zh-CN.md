[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bstr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation byte string utilities (Rust bstr v1 parity) powered by @cap/memchr SWAR search, line splitting, lossy UTF-8 decoding, and ASCII transforms

## 安装与导入

```bash
cap add bstr
```

```cp
import { bstrFind } from "bstr";
```

## 核心导出 API

- `export fn bstrFind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn bstrRfind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn bstrFindByte(haystack: &[]u8, b: u8): i64;`
- `export fn bstrRfindByte(haystack: &[]u8, b: u8): i64;`
- `export fn bstrContains(haystack: &[]u8, needle: &[]u8): bool;`
- `export fn bstrStartsWith(s: &[]u8, prefix: &[]u8): bool;`
- `export fn bstrEndsWith(s: &[]u8, suffix: &[]u8): bool;`
- `export fn bstrTrimStart(s: &[]u8): SliceRange;`
- `export fn bstrTrimEnd(s: &[]u8): SliceRange;`
- `export fn bstrTrim(s: &[]u8): SliceRange;`
- `export fn bstrLineCount(s: &[]u8): usize;`
- `export fn bstrLineAt(s: &[]u8, idx: usize): SliceRange;`
- `export fn bstrReplaceAll(src: &[]u8, pattern: &[]u8, replacement: &[]u8, out: []u8): usize;`
- `export fn bstrIsUtf8(src: &[]u8): bool;`
- `export fn bstrToUtf8Lossy(src: &[]u8, out: []u8): usize;`
- `export fn bstrIsAscii(s: &[]u8): bool;`
- `export fn bstrToLowerAscii(src: &[]u8, out: []u8): usize;`
- `export fn bstrToUpperAscii(src: &[]u8, out: []u8): usize;`
- `export fn bstrEqIgnoreAsciiCase(a: &[]u8, b: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bstr@1.0.0`)，许可证：MIT
