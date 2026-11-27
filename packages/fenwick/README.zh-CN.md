[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fenwick

Zero-allocation Binary Indexed Tree (Fenwick Tree) with O(log N) prefix/range sums and binary-lifting lowerBound for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fenwick` ([https://github.com/pwoolcoc/fenwick](https://github.com/pwoolcoc/fenwick))

| 元信息 | 值 |
| --- | --- |
| 包名 | `fenwick` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fenwick
```

```cp
import { FenwickTree, lowbit } from "fenwick";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FenwickTree` | `class` | 字段: `tree: [TREE_LEN]i64, length: usize` · 方法: `init, fromSlice, len, add, prefixSum, rangeSum, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lowbit` | `(x: usize): usize` | 调用 `lowbit` 并返回 `usize`。 |

## 最小调用示例

```cp
import { lowbit } from "fenwick";

pub fn main() {
    let result = lowbit(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
