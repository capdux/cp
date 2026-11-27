[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-timeout

Zero-allocation deadline tracker and timeout state machine guard (npm p-timeout v6 equivalent) for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-timeout` ([https://github.com/sindresorhus/p-timeout](https://github.com/sindresorhus/p-timeout))（原库名 `p-timeout` → CP 包名 `p-timeout`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `p-timeout` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add p-timeout
```

```cp
import { TimeoutOptions, TimeoutGuard, ptoMaxU64, ptoSaturatingAdd } from "p-timeout";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TimeoutOptions` | `class` | 字段: `milliseconds: u64, has_fallback: bool, fallback_code: i32, error_message: string` · 方法: `withMs` |
| `TimeoutGuard` | `class` | 字段: `start_ms: u64, deadline_ms: u64, opts: TimeoutOptions, state: u8, result_code: i32, used_fallback: bool` · 方法: `new, deadlineMs, remainingMs, isExpired, poll, resolve, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ptoMaxU64` | `(): u64` | 调用 `ptoMaxU64` 并返回 `u64`。 |
| `ptoSaturatingAdd` | `(a: u64, b: u64): u64` | 调用 `ptoSaturatingAdd` 并返回 `u64`。 |

## 最小调用示例

```cp
import { ptoMaxU64 } from "p-timeout";

pub fn main() {
    let result = ptoMaxU64();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
