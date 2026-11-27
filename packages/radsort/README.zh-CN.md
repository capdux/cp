[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# radsort

Zero-allocation O(N) 8-bit LSD radix sort for u32, i32, u64, i64, and IEEE-754 f64 with pass skipping for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:radsort` ([https://github.com/JakubValtar/radsort](https://github.com/JakubValtar/radsort))（原库名 `radsort` → CP 包名 `radsort`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `radsort` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add radsort
```

```cp
import {
    i32ToSortable,
    i64ToSortable,
    f64ToSortable,
    sortU32,
    sortI32,
    sortU64,
    sortI64,
    sortF64,
} from "radsort";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `i32ToSortable` | `(v: i32): u32` | 调用 `i32ToSortable` 并返回 `u32`。 |
| `i64ToSortable` | `(v: i64): u64` | 调用 `i64ToSortable` 并返回 `u64`。 |
| `f64ToSortable` | `(v: f64): u64` | 调用 `f64ToSortable` 并返回 `u64`。 |
| `sortU32` | `(data: []u32, scratch: []u32): bool` | 调用 `sortU32` 并返回 `bool`。 |
| `sortI32` | `(data: []i32, scratch: []i32): bool` | 调用 `sortI32` 并返回 `bool`。 |
| `sortU64` | `(data: []u64, scratch: []u64): bool` | 调用 `sortU64` 并返回 `bool`。 |
| `sortI64` | `(data: []i64, scratch: []i64): bool` | 调用 `sortI64` 并返回 `bool`。 |
| `sortF64` | `(data: []f64, scratch: []f64): bool` | 调用 `sortF64` 并返回 `bool`。 |
| `isSortedI64` | `(data: &[]i64): bool` | 执行 `isSortedI64` 条件判定，返回 `bool`。 |
| `isSortedU64` | `(data: &[]u64): bool` | 执行 `isSortedU64` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isSortedI64 } from "radsort";

pub fn main() {
    let result = isSortedI64(/* data: &[]i64 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
