[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# dlv-list

Zero-heap Vec-backed doubly linked list with stable u32 handles and O(1) slot reuse

| 元信息 | 值 |
| --- | --- |
| 包名 | `dlv-list` |
| 版本 | `0.5.2` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add dlv-list
```

```cp
import {
    DlvListI64,
    dlvlZeroI64,
    dlvlNoneU32,
    dlvlFalseBool,
    dlvlAllocSlot,
} from "dlv-list";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DlvListI64` | `class` | 字段: `values: [DLV_CAP]i64, prev: [DLV_CAP]u32, next: [DLV_CAP]u32, occupied: [DLV_CAP]bool, head: u32, tail: u32, +3 项` · 方法: `new, pushFront, pushBack, get, remove, popFront, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `dlvlZeroI64` | `(): [DLV_CAP]i64` | 调用 `dlvlZeroI64` 并返回 `[DLV_CAP]i64`。 |
| `dlvlNoneU32` | `(): [DLV_CAP]u32` | 调用 `dlvlNoneU32` 并返回 `[DLV_CAP]u32`。 |
| `dlvlFalseBool` | `(): [DLV_CAP]bool` | 调用 `dlvlFalseBool` 并返回 `[DLV_CAP]bool`。 |
| `dlvlAllocSlot` | `(list: &mut DlvListI64): u32` | 调用 `dlvlAllocSlot` 并返回 `u32`。 |

## 最小调用示例

```cp
import { dlvlZeroI64 } from "dlv-list";

pub fn main() {
    let result = dlvlZeroI64();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
