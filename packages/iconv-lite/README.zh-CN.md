[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# iconv-lite

Zero-allocation character encoding converter supporting UTF-8, ASCII, Latin1, Windows-1252, UTF-16LE/BE with BOM and surrogate pairs for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:iconv-lite` ([https://github.com/ashtuchkin/iconv-lite](https://github.com/ashtuchkin/iconv-lite))

| 元信息 | 值 |
| --- | --- |
| 包名 | `iconv-lite` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add iconv-lite
```

```cp
import {
    Charset,
    IcvUtf8Step,
    icvStrEq,
    parseCharset,
    encodingExists,
    icvWin1252ToUnicode,
    icvUnicodeToWin1252,
    icvWriteUtf8,
} from "iconv-lite";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Charset` | `Utf8, Ascii, Latin1, Win1252, Utf16Le, Utf16Be, Unknown` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `IcvUtf8Step` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `icvStrEq` | `(a: string, b: string): bool` | 调用 `icvStrEq` 并返回 `bool`。 |
| `parseCharset` | `(name: string): Charset` | 通过 `parseCharset` 解析输入数据，返回 `Charset`。 |
| `encodingExists` | `(name: string): bool` | 调用 `encodingExists` 并返回 `bool`。 |
| `icvWin1252ToUnicode` | `(b: u8): u32` | 调用 `icvWin1252ToUnicode` 并返回 `u32`。 |
| `icvUnicodeToWin1252` | `(cp: u32): u8?` | 调用 `icvUnicodeToWin1252` 并返回 `u8?`。 |
| `icvWriteUtf8` | `(out: []u8, pos: usize, cp: u32): usize` | 调用 `icvWriteUtf8` 并返回 `usize`。 |
| `icvReadUtf8` | `(s: string, pos: usize): IcvUtf8Step` | 调用 `icvReadUtf8` 并返回 `IcvUtf8Step`。 |
| `icvReadU16` | `(data: &[]u8, pos: usize, is_le: bool): u16` | 调用 `icvReadU16` 并返回 `u16`。 |
| `icvWriteU16` | `(out: []u8, pos: usize, val: u16, is_le: bool): usize` | 调用 `icvWriteU16` 并返回 `usize`。 |
| `decodeToUtf8` | `(data: &[]u8, charset: Charset, strip_bom: bool, out: []u8): usize` | 通过 `decodeToUtf8` 解析输入数据，返回 `usize`。 |
| `encodeFromUtf8` | `(utf8: string, charset: Charset, add_bom: bool, out: []u8): usize` | 通过 `encodeFromUtf8` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseCharset } from "iconv-lite";

pub fn main() {
    let result = parseCharset("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
