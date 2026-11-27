[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fixedbitset

Zero-allocation fixed-size bitsets BitSet256/BitSet1024 with SWAR popcount, fast scanning, and set algebra (Rust fixedbitset / bit-set parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`fixedbitset`](https://github.com/petgraph/fixedbitset)（`MIT OR Apache-2.0`）（原库名 `fixedbitset`，CP 包名 `fixedbitset`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `fixedbitset` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fixedbitset
```

```cp
import {
    BitSet256,
    BitSet1024,
    allOnes64,
    bitMask64,
    popcount64,
    ctz64,
} from "fixedbitset";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BitSet256` | `class` | 字段: `words: [4]u64` · 方法: `init, full, len, clear, contains, insert, +14 项` |
| `BitSet1024` | `class` | 字段: `words: [16]u64` · 方法: `init, full, len, clear, contains, insert, +14 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `allOnes64` | `(): u64` | 调用 `allOnes64` 并返回 `u64`。 |
| `bitMask64` | `(bit: usize): u64` | 调用 `bitMask64` 并返回 `u64`。 |
| `popcount64` | `(x_in: u64): usize` | 调用 `popcount64` 并返回 `usize`。 |
| `ctz64` | `(w_in: u64): usize` | 调用 `ctz64` 并返回 `usize`。 |

## 最小调用示例

```cp
import { allOnes64 } from "fixedbitset";

pub fn main() {
    let result = allOnes64();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
