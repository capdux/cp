[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# icu_locid

Zero-allocation BCP-47 / UTS #35 Unicode Language Identifier parser, canonicalizer, and likely-subtags maximizer reusing tinystr (ICU4X icu_locid parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:icu_locid` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))（原库名 `icu_locid` → CP 包名 `icu_locid`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `icu_locid` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add icu_locid
```

```cp
import {
    LanguageIdentifier,
    LocaleParseResult,
    locIsSep,
    locIsDigitByte,
    locEmptyVariants,
    locLangEqStr,
    locRegionEqStr,
    locValidLanguageSubtag,
} from "icu_locid";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LanguageIdentifier` | `class` | 字段: `language: TinyAsciiStr8, has_script: bool, script: TinyAsciiStr4, has_region: bool, region: TinyAsciiStr4, variant_count: usize, +1 项` · 方法: `und, parse, toStr, isUnd, eq, matches, +1 项` |
| `LocaleParseResult` | `class` | 字段: `ok: bool, locale: LanguageIdentifier, err: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `locIsSep` | `(ch: u8): bool` | 调用 `locIsSep` 并返回 `bool`。 |
| `locIsDigitByte` | `(ch: u8): bool` | 调用 `locIsDigitByte` 并返回 `bool`。 |
| `locEmptyVariants` | `(): [4]TinyAsciiStr8` | 调用 `locEmptyVariants` 并返回 `[4]TinyAsciiStr8`。 |
| `locLangEqStr` | `(lang: TinyAsciiStr8, lit: string): bool` | 调用 `locLangEqStr` 并返回 `bool`。 |
| `locRegionEqStr` | `(reg: TinyAsciiStr4, lit: string): bool` | 调用 `locRegionEqStr` 并返回 `bool`。 |
| `locValidLanguageSubtag` | `(sub: TinyAsciiStr8): bool` | 调用 `locValidLanguageSubtag` 并返回 `bool`。 |
| `locValidVariantSubtag` | `(sub: TinyAsciiStr8): bool` | 调用 `locValidVariantSubtag` 并返回 `bool`。 |

## 最小调用示例

```cp
import { locLangEqStr } from "icu_locid";

pub fn main() {
    let result = locLangEqStr(/* lang: TinyAsciiStr8 */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
