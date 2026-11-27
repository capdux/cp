[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# icu_locid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation BCP-47 / UTS #35 Unicode Language Identifier parser, canonicalizer, and likely-subtags maximizer reusing tinystr (ICU4X icu_locid parity) in pure CP.

## 安装与导入

```bash
cap add icu_locid
```

```cp
import { locIsSep } from "icu_locid";
```

## 核心导出 API

- `export fn locIsSep(ch: u8): bool;`
- `export fn locIsDigitByte(ch: u8): bool;`
- `export fn locEmptyVariants(): [4]TinyAsciiStr8;`
- `export fn locLangEqStr(lang: TinyAsciiStr8, lit: string): bool;`
- `export fn locRegionEqStr(reg: TinyAsciiStr4, lit: string): bool;`
- `export fn locValidLanguageSubtag(sub: TinyAsciiStr8): bool;`
- `export fn locValidVariantSubtag(sub: TinyAsciiStr8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`icu_locid@1.0.0`)，许可证：MIT
