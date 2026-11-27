[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-limit

Zero-allocation concurrency limiter and FIFO task slot scheduler (p-limit parity) powered by yocto-queue in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-limit`](https://github.com/sindresorhus/p-limit)（`MIT`）（包名 `p-limit`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `p-limit` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add p-limit
```

```cp
import { SubmitResult, CompleteResult, PLimit, plClampConcurrency } from "p-limit";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SubmitResult` | `class` | 字段: `started_immediately: bool, queued: bool, rejected: bool` |
| `CompleteResult` | `class` | 字段: `has_next: bool, next_task_id: u64` |
| `PLimit` | `class` | 字段: `limit: usize, active: usize, queue: YoctoQueueU64` · 方法: `new, activeCount, pendingCount, queueCapacity, concurrency, setConcurrency, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `plClampConcurrency` | `(c: usize): usize` | 调用 `plClampConcurrency` 并返回 `usize`。 |

## 最小调用示例

```cp
import { plClampConcurrency } from "p-limit";

pub fn main() {
    let result = plClampConcurrency(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
