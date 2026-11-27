[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mediatype

Zero-allocation RFC 9110 / RFC 6838 HTTP Media Type (MIME) parser, structured suffix extractor, and parameter matcher for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`mediatype`](https://github.com/picoHz/mediatype)（`MIT`）（原库名 `mediatype`，CP 包名 `mediatype`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `mediatype` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mediatype
```

```cp
import {
    MediaType,
    MediaParam,
    ParsedMediaType,
    mtToLower,
    mtIsOws,
    mtTrimOws,
    mtEqIgnoreCase,
    mtIsTchar,
} from "mediatype";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MediaType` | `class` | 字段: `top: string, subtype: string, suffix: string, params: string` |
| `MediaParam` | `class` | 字段: `name: string, value: string` |
| `ParsedMediaType` | `class` | 字段: `valid: bool, top_level: string, sub_level: string, suffix: string, params: [8]MediaParam, param_count: usize` · 方法: `invalid, parse, getParam, hasSuffix, formatEssence, formatFull, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mtToLower` | `(c: u8): u8` | 调用 `mtToLower` 并返回 `u8`。 |
| `mtIsOws` | `(c: u8): bool` | 调用 `mtIsOws` 并返回 `bool`。 |
| `mtTrimOws` | `(s: &[]u8): &[]u8` | 调用 `mtTrimOws` 并返回 `&[]u8`。 |
| `mtEqIgnoreCase` | `(a: &[]u8, b: &[]u8): bool` | 调用 `mtEqIgnoreCase` 并返回 `bool`。 |
| `mtIsTchar` | `(c: u8): bool` | 调用 `mtIsTchar` 并返回 `bool`。 |
| `isValidToken` | `(s: string): bool` | 执行 `isValidToken` 条件判定，返回 `bool`。 |
| `parseMediaType` | `(input: string, out: &mut MediaType): bool` | 通过 `parseMediaType` 解析输入数据，返回 `bool`。 |
| `getParam` | `(mt: MediaType, name: string, out: []u8): usize` | 查询或读取目标数据，返回 `usize`。 |
| `formatMediaType` | `(mt: MediaType, out: []u8): usize` | 通过 `formatMediaType` 编码或格式化输出，返回 `usize`。 |
| `isJsonLike` | `(mt: MediaType): bool` | 执行 `isJsonLike` 条件判定，返回 `bool`。 |
| `isTextLike` | `(mt: MediaType): bool` | 执行 `isTextLike` 条件判定，返回 `bool`。 |
| `essenceEq` | `(a: string, b: string): bool` | 调用 `essenceEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { isValidToken } from "mediatype";

pub fn main() {
    let result = isValidToken("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
