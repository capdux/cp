[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# number_prefix

Zero-heap SI decimal (k/M/G/T/P/E) and IEC binary (Ki/Mi/Gi/Ti/Pi/Ei) numeric prefix formatter

| 元信息 | 值 |
| --- | --- |
| 包名 | `number_prefix` |
| 版本 | `0.4.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add number_prefix
```

```cp
import {
    PrefixKind,
    PrefixedNumber,
    decimalPrefix,
    binaryPrefix,
    npfWriteStr,
    npfWriteU64,
    formatPrefixed,
} from "number_prefix";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `PrefixKind` | `Standalone, Kilo, Mega, Giga, Tera, Peta, Exa, Kibi ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PrefixedNumber` | `class` | 字段: `value: f64, prefix: PrefixKind, symbol: string, is_standalone: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decimalPrefix` | `(amount: f64): PrefixedNumber` | 调用 `decimalPrefix` 并返回 `PrefixedNumber`。 |
| `binaryPrefix` | `(amount: f64): PrefixedNumber` | 调用 `binaryPrefix` 并返回 `PrefixedNumber`。 |
| `npfWriteStr` | `(s: string, out: []u8, pos: usize): usize` | 调用 `npfWriteStr` 并返回 `usize`。 |
| `npfWriteU64` | `(val: u64, out: []u8, pos: usize): usize` | 调用 `npfWriteU64` 并返回 `usize`。 |
| `formatPrefixed` | `(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize` | 通过 `formatPrefixed` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatPrefixed } from "number_prefix";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatPrefixed(/* pn: PrefixedNumber */, 0, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
