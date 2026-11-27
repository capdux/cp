[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bitvec

Zero-heap arbitrary-length bit vector with popcount, find-first-one, bitwise ops and shifts (clean-room CP implementation of crates.io bitvec)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bitvec` ([https://github.com/ferrilab/bitvec](https://github.com/ferrilab/bitvec))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bitvec` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bitvec
```

```cp
import { BitVec512, bvPopcountU64 } from "bitvec";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BitVec512` | `class` | 字段: `words: [8]u64, bit_len: usize` · 方法: `new, withLen, len, capacity, isEmpty, clear, +16 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bvPopcountU64` | `(x: u64): usize` | 调用 `bvPopcountU64` 并返回 `usize`。 |

## 最小调用示例

```cp
import { bvPopcountU64 } from "bitvec";

pub fn main() {
    let result = bvPopcountU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
