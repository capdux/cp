[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sprintf-js

Zero-allocation C-style sprintf formatting engine supporting %s/%d/%i/%u/%x/%X/%o/%b/%f/%c with flags, width, and precision in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sprintf-js` ([https://github.com/alexei/sprintf.js](https://github.com/alexei/sprintf.js), `BSD-3-Clause`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `sprintf-js` |
| 版本 | `1.0.0` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sprintf-js
```

```cp
import {
    FmtArgKind,
    FmtArg,
    spfPutByte,
    spfPutRepeat,
    spfEmitPadded,
    spfFormatUintRadix,
    spfArgAsSigned,
    spfArgAsUnsigned,
} from "sprintf-js";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `FmtArgKind` | `Int, Uint, Float, Str, Char, Bool` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FmtArg` | `class` | 字段: `kind: FmtArgKind, int_val: i64, uint_val: u64, float_val: f64, str_val: string, char_val: u8, +1 项` · 方法: `int, uint, float, str, ch, boolean` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `spfPutByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `spfPutByte` 并返回 `usize`。 |
| `spfPutRepeat` | `(out: []u8, pos: usize, b: u8, count: usize): usize` | 调用 `spfPutRepeat` 并返回 `usize`。 |
| `spfEmitPadded` | `(out: []u8, pos: usize, sign: u8, body: &[]u8, width: usize, left_align: bool, zero_pad: bool): usize` | 调用 `spfEmitPadded` 并返回 `usize`。 |
| `spfFormatUintRadix` | `(val: u64, base: u64, upper: bool, buf: []u8): usize` | 调用 `spfFormatUintRadix` 并返回 `usize`。 |
| `spfArgAsSigned` | `(arg: FmtArg): i64` | 调用 `spfArgAsSigned` 并返回 `i64`。 |
| `spfArgAsUnsigned` | `(arg: FmtArg): u64` | 调用 `spfArgAsUnsigned` 并返回 `u64`。 |
| `spfArgAsFloat` | `(arg: FmtArg): f64` | 调用 `spfArgAsFloat` 并返回 `f64`。 |
| `spfFormatFloat` | `(val: f64, prec: usize, plus_sign: bool, width: usize, left_align: bool, zero_pad: bool, out: []u8, pos: usize): usize` | 调用 `spfFormatFloat` 并返回 `usize`。 |
| `sprintf` | `(fmt: string, args: &[]FmtArg, out: []u8): usize` | 调用 `sprintf` 并返回 `usize`。 |

## 最小调用示例

```cp
import { sprintf } from "sprintf-js";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sprintf("example", /* args: &[]FmtArg */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
