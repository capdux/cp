[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# powerfmt

Zero-heap high-performance integer formatting (alignment, padding, radix 2/8/10/16, digit grouping) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:powerfmt` ([https://github.com/jhpratt/powerfmt](https://github.com/jhpratt/powerfmt)) / `npm:format-number` ([https://github.com/comfuture/format-number](https://github.com/comfuture/format-number))

| 元信息 | 值 |
| --- | --- |
| 包名 | `powerfmt` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add powerfmt
```

```cp
import {
    IntFmtSpec,
    digitByte,
    writeByte,
    writeRepeat,
    formatMagnitude,
    formatU64,
    formatI64,
    formatDecPadded,
} from "powerfmt";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `IntFmtSpec` | `class` | 字段: `min_width: usize, pad_byte: u8, alignment: u8, radix: u8, uppercase: bool, force_sign: bool, +2 项` · 方法: `defaultSpec` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `digitByte` | `(d: u64, uppercase: bool): u8` | 调用 `digitByte` 并返回 `u8`。 |
| `writeByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `writeByte` 并返回 `usize`。 |
| `writeRepeat` | `(out: []u8, pos: usize, b: u8, count: usize): usize` | 调用 `writeRepeat` 并返回 `usize`。 |
| `formatMagnitude` | `(mag: u64, sign_byte: u8, spec: IntFmtSpec, out: []u8): usize` | 通过 `formatMagnitude` 编码或格式化输出，返回 `usize`。 |
| `formatU64` | `(val: u64, spec: IntFmtSpec, out: []u8): usize` | 通过 `formatU64` 编码或格式化输出，返回 `usize`。 |
| `formatI64` | `(val: i64, spec: IntFmtSpec, out: []u8): usize` | 通过 `formatI64` 编码或格式化输出，返回 `usize`。 |
| `formatDecPadded` | `(val: i64, width: usize, out: []u8): usize` | 通过 `formatDecPadded` 编码或格式化输出，返回 `usize`。 |
| `formatHex` | `(val: u64, uppercase: bool, out: []u8): usize` | 通过 `formatHex` 编码或格式化输出，返回 `usize`。 |
| `formatWithCommas` | `(val: i64, out: []u8): usize` | 通过 `formatWithCommas` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatU64 } from "powerfmt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatU64(0, /* spec: IntFmtSpec */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
