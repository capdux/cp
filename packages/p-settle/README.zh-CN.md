[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-settle

Zero-allocation bounded-concurrency Promise.allSettled task planner (npm p-settle v5 parity) powered by p-limit in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-settle`](https://github.com/sindresorhus/p-settle)（`MIT`）（包名 `p-settle`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `p-settle` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add p-settle
```

```cp
import { SettleStatus, SettledEntry, PSettlePlanner, psetFindSlot } from "p-settle";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SettleStatus` | `Pending, Fulfilled, Rejected` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SettledEntry` | `class` | 字段: `id: u64, status: SettleStatus, value: i64, err_code: i32` |
| `PSettlePlanner` | `class` | 字段: `limiter: PLimit, total_count: usize, fulfilled_count: usize, rejected_count: usize, ids: [32]u64, running: [32]bool, +3 项` · 方法: `new, addTask, dispatchNext, reportFulfilled, reportRejected, isAllSettled, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `psetFindSlot` | `(ids: [32]u64, count: usize, id: u64): usize?` | 调用 `psetFindSlot` 并返回 `usize?`。 |

## 最小调用示例

```cp
import { psetFindSlot } from "p-settle";

pub fn main() {
    let result = psetFindSlot(/* ids: [32]u64 */, 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
