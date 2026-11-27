[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-all

Zero-allocation bounded-concurrency batch task planner (npm p-all v5 parity) powered by p-limit in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-all`](https://github.com/sindresorhus/p-all)（`MIT`）（包名 `p-all`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `p-all` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add p-all
```

```cp
import { PAllPlanner, pallFindSlot } from "p-all";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PAllPlanner` | `class` | 字段: `limiter: PLimit, stop_on_error: bool, failed: bool, first_err: i32, total_count: usize, completed_count: usize, +5 项` · 方法: `new, addTask, dispatchNext, reportSuccess, reportError, isDone, +10 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pallFindSlot` | `(ids: [32]u64, count: usize, id: u64): usize?` | 调用 `pallFindSlot` 并返回 `usize?`。 |

## 最小调用示例

```cp
import { pallFindSlot } from "p-all";

pub fn main() {
    let result = pallFindSlot(/* ids: [32]u64 */, 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
