[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cssesc

Zero-allocation CSSOM-compliant CSS identifier and string escaper in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cssesc` ([https://github.com/mathiasbynens/cssesc](https://github.com/mathiasbynens/cssesc))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cssesc` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cssesc
```

```cp
import {
    CssQuoteStyle,
    CssEscOptions,
    CescWriter,
    CescUtf8Res,
    defaultCssEscOptions,
    cescDecodeUtf8,
    cescIsAsciiAlnumOrDash,
    cssescBuf,
} from "cssesc";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CssQuoteStyle` | `Single, Double` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CssEscOptions` | `class` | 字段: `is_identifier: bool, quotes: CssQuoteStyle, wrap: bool, escape_everything: bool` |
| `CescWriter` | `class` | 字段: `buf: []u8, pos: usize, overflow: bool` · 方法: `writeByte, writeHexEscape` |
| `CescUtf8Res` | `class` | 字段: `cp: u32, next: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `defaultCssEscOptions` | `(): CssEscOptions` | 调用 `defaultCssEscOptions` 并返回 `CssEscOptions`。 |
| `cescDecodeUtf8` | `(s: string, start: usize): CescUtf8Res` | 调用 `cescDecodeUtf8` 并返回 `CescUtf8Res`。 |
| `cescIsAsciiAlnumOrDash` | `(cp: u32): bool` | 调用 `cescIsAsciiAlnumOrDash` 并返回 `bool`。 |
| `cssescBuf` | `(input: string, opts: CssEscOptions, out: []u8): usize` | 调用 `cssescBuf` 并返回 `usize`。 |
| `cssesc` | `(input: string, opts: CssEscOptions): string` | 调用 `cssesc` 并返回 `string`。 |

## 最小调用示例

```cp
import { cescDecodeUtf8 } from "cssesc";

pub fn main() {
    let result = cescDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
