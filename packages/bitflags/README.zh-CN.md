[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bitflags

Zero-heap 64-bit bitflags set algebra (contains, intersects, insert, remove, toggle, difference, popcount, trailing/leading zeros) for CP (compatible with Rust bitflags & npm bitfield)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bitfield` ([https://github.com/fb55/bitfield](https://github.com/fb55/bitfield))（原库名 `bitfield` → CP 包名 `bitflags`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `bitflags` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bitflags
```

```cp
import {
    Flags64,
    empty,
    fromBits,
    fromBitIndex,
    isEmpty,
    contains,
    intersects,
    unionFlags,
} from "bitflags";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Flags64` | `class` | 字段: `bits: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `empty` | `(): Flags64` | 调用 `empty` 并返回 `Flags64`。 |
| `fromBits` | `(raw: u64): Flags64` | 调用 `fromBits` 并返回 `Flags64`。 |
| `fromBitIndex` | `(idx: u8): Flags64` | 调用 `fromBitIndex` 并返回 `Flags64`。 |
| `isEmpty` | `(f: Flags64): bool` | 执行 `isEmpty` 条件判定，返回 `bool`。 |
| `contains` | `(f: Flags64, other: Flags64): bool` | 调用 `contains` 并返回 `bool`。 |
| `intersects` | `(f: Flags64, other: Flags64): bool` | 调用 `intersects` 并返回 `bool`。 |
| `unionFlags` | `(a: Flags64, b: Flags64): Flags64` | 调用 `unionFlags` 并返回 `Flags64`。 |
| `intersection` | `(a: Flags64, b: Flags64): Flags64` | 调用 `intersection` 并返回 `Flags64`。 |
| `difference` | `(a: Flags64, b: Flags64): Flags64` | 调用 `difference` 并返回 `Flags64`。 |
| `symmetricDifference` | `(a: Flags64, b: Flags64): Flags64` | 调用 `symmetricDifference` 并返回 `Flags64`。 |
| `insert` | `(f: &mut Flags64, other: Flags64)` | 就地执行 `insert` 操作。 |
| `remove` | `(f: &mut Flags64, other: Flags64)` | 就地执行 `remove` 操作。 |
| `toggle` | `(f: &mut Flags64, other: Flags64)` | 就地执行 `toggle` 操作。 |
| `setCond` | `(f: &mut Flags64, other: Flags64, enabled: bool)` | 写入或更新状态数据。 |
| `truncate` | `(raw: u64, valid_mask: u64): Flags64` | 调用 `truncate` 并返回 `Flags64`。 |
| `countBits` | `(f: Flags64): u32` | 调用 `countBits` 并返回 `u32`。 |
| `lowestBitIndex` | `(f: Flags64): i32` | 调用 `lowestBitIndex` 并返回 `i32`。 |
| `formatHex` | `(f: Flags64, out: []u8): usize` | 通过 `formatHex` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isEmpty } from "bitflags";

pub fn main() {
    let result = isEmpty(/* f: Flags64 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
