[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# plist

Zero-allocation Apple XML Property List (.plist) dictionary parser and serializer in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:plist` ([https://github.com/ebarnard/rust-plist](https://github.com/ebarnard/rust-plist))

| 元信息 | 值 |
| --- | --- |
| 包名 | `plist` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add plist
```

```cp
import {
    PlistValKind,
    PlistValue,
    plstInvalid,
    plstEq,
    plstStartsAt,
    plstFindSub,
    plstSkipWs,
    plstParseI64,
} from "plist";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `PlistValKind` | `String, Integer, Real, Bool, Data, Date, Invalid` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PlistValue` | `class` | 字段: `kind: PlistValKind, str_val: string, int_val: i64, bool_val: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `plstInvalid` | `(): PlistValue` | 调用 `plstInvalid` 并返回 `PlistValue`。 |
| `plstEq` | `(a: string, b: string): bool` | 调用 `plstEq` 并返回 `bool`。 |
| `plstStartsAt` | `(hay: string, pos: usize, needle: string): bool` | 调用 `plstStartsAt` 并返回 `bool`。 |
| `plstFindSub` | `(hay: string, start: usize, needle: string): usize?` | 调用 `plstFindSub` 并返回 `usize?`。 |
| `plstSkipWs` | `(s: string, start: usize): usize` | 调用 `plstSkipWs` 并返回 `usize`。 |
| `plstParseI64` | `(s: string): i64` | 调用 `plstParseI64` 并返回 `i64`。 |
| `plstSkipBalanced` | `(xml: string, pos: usize, open_tag: string, close_tag: string): usize` | 调用 `plstSkipBalanced` 并返回 `usize`。 |
| `plstSkipElement` | `(xml: string, pos: usize): usize` | 调用 `plstSkipElement` 并返回 `usize`。 |
| `plstParseValueAt` | `(xml: string, pos: usize): PlistValue` | 调用 `plstParseValueAt` 并返回 `PlistValue`。 |
| `plistGetDictValue` | `(xml: string, key_name: string): PlistValue` | 调用 `plistGetDictValue` 并返回 `PlistValue`。 |
| `plstWriteBytes` | `(dst: []u8, pos: usize, src: string): usize` | 调用 `plstWriteBytes` 并返回 `usize`。 |
| `plstFormatI64` | `(val: i64, buf: []u8): usize` | 调用 `plstFormatI64` 并返回 `usize`。 |
| `plistWriteDictEntry` | `(key: string, val: PlistValue, out: []u8): usize` | 调用 `plistWriteDictEntry` 并返回 `usize`。 |

## 最小调用示例

```cp
import { plstEq } from "plist";

pub fn main() {
    let result = plstEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
