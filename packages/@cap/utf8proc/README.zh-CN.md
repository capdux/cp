[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/utf8proc

> 版本：`2.10.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Clean Unicode 16.0 normalization (NFC/NFD/NFKC/NFKD), case-folding & charwidth library for CP

## 安装与导入

```bash
cap add @cap/utf8proc
```

```cp
import { utf8proc_version } from "@cap/utf8proc";
```

## 核心导出 API

- `export fn utf8proc_version(): [*:0]&u8!;`
- `export fn utf8proc_unicode_version(): [*:0]&u8!;`
- `export fn utf8proc_charwidth(codepoint: i32): i32;`
- `export fn utf8proc_tolower(c: i32): i32;`
- `export fn utf8proc_toupper(c: i32): i32;`
- `export fn utf8proc_NFC(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFD(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFKC(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFKD(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFKC_Casefold(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn free(ptr: *auto!?);`
- `export fn versionString(): string;`
- `export fn unicodeVersion(): string;`
- `export fn charWidth(cp: i32): i32;`
- `export fn toLower(cp: i32): i32;`
- `export fn toUpper(cp: i32): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`utf8proc@2.10.0`)，许可证：MIT
