[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lodash.debounce

Zero-allocation deterministic clock-driven Debouncer and Throttler state machines with leading/trailing/maxWait semantics in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lodash.debounce` ([https://github.com/lodash/lodash](https://github.com/lodash/lodash))（原库名 `lodash.debounce` → CP 包名 `lodash.debounce`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `lodash.debounce` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add lodash.debounce
```

```cp
import { Debouncer, Throttler, elapsedSince } from "lodash.debounce";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Debouncer` | `class` | 字段: `wait_ms: u64, leading: bool, trailing: bool, max_wait_ms: u64, active: bool, pending: bool, +3 项` · 方法: `init, trigger, poll, flush, cancel, isPending, +2 项` |
| `Throttler` | `class` | 字段: `interval_ms: u64, leading: bool, trailing: bool, has_window: bool, window_start_ms: u64, pending: bool, +1 项` · 方法: `init, trigger, poll, cancel, isPending, execCount` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `elapsedSince` | `(now_ms: u64, past_ms: u64): u64` | 调用 `elapsedSince` 并返回 `u64`。 |

## 最小调用示例

```cp
import { elapsedSince } from "lodash.debounce";

pub fn main() {
    let result = elapsedSince(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
