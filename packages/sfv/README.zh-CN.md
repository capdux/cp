[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sfv

Zero-allocation W3C User-Agent Client Hints (Sec-CH-UA*) RFC 8941 structured header parser and formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sfv` ([https://github.com/undef1nd/sfv](https://github.com/undef1nd/sfv)) / `npm:ua-client-hints` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `sfv` → CP 包名 `sfv`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `sfv` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sfv
```

```cp
import {
    BrandVersion,
    uahToLower,
    uahIsOws,
    uahTrimOws,
    uahEqIgnoreCase,
    uahFindIgnoreCaseFrom,
    uahScanQuoted,
    uahSkipToNextItem,
} from "sfv";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BrandVersion` | `class` | 字段: `brand: string, version: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `uahToLower` | `(c: u8): u8` | 调用 `uahToLower` 并返回 `u8`。 |
| `uahIsOws` | `(c: u8): bool` | 调用 `uahIsOws` 并返回 `bool`。 |
| `uahTrimOws` | `(s: &[]u8): &[]u8` | 调用 `uahTrimOws` 并返回 `&[]u8`。 |
| `uahEqIgnoreCase` | `(a: &[]u8, b: &[]u8): bool` | 调用 `uahEqIgnoreCase` 并返回 `bool`。 |
| `uahFindIgnoreCaseFrom` | `(haystack: &[]u8, needle: &[]u8, start_idx: usize): usize` | 调用 `uahFindIgnoreCaseFrom` 并返回 `usize`。 |
| `uahScanQuoted` | `(header: &[]u8, pos: &mut usize, innerStart: &mut usize, innerEnd: &mut usize): bool` | 调用 `uahScanQuoted` 并返回 `bool`。 |
| `uahSkipToNextItem` | `(header: &[]u8, pos: &mut usize)` | 就地执行 `uahSkipToNextItem` 操作。 |
| `uahParseNextBrand` | `(header: &[]u8, pos: &mut usize, item: &mut BrandVersion): bool` | 调用 `uahParseNextBrand` 并返回 `bool`。 |
| `parseBrands` | `(header: string, out: []BrandVersion): usize` | 通过 `parseBrands` 解析输入数据，返回 `usize`。 |
| `isGreaseBrand` | `(brand: string): bool` | 执行 `isGreaseBrand` 条件判定，返回 `bool`。 |
| `findBrandVersion` | `(header: string, brand: string): string` | 查询或读取目标数据，返回 `string`。 |
| `parseMobile` | `(header: string): i32` | 通过 `parseMobile` 解析输入数据，返回 `i32`。 |
| `parsePlatform` | `(header: string): string` | 通过 `parsePlatform` 解析输入数据，返回 `string`。 |
| `uahQuotedEncodedLen` | `(s: &[]u8): usize` | 调用 `uahQuotedEncodedLen` 并返回 `usize`。 |
| `uahWriteQuoted` | `(out: []u8, pos: &mut usize, s: &[]u8)` | 就地执行 `uahWriteQuoted` 操作。 |
| `formatBrands` | `(brands: &[]BrandVersion, out: []u8): usize` | 通过 `formatBrands` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseBrands } from "sfv";

pub fn main() {
    let result = parseBrands("example", /* out: []BrandVersion */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
