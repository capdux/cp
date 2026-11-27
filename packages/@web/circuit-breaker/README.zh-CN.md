[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/circuit-breaker

Official Envoy / Tower three-state circuit breaker (Closed / Open / HalfOpen) plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/circuit-breaker` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/circuit-breaker
```

```cp
import {
    CircuitState,
    CircuitBreakerConfig,
    cbNowSec,
    cbState,
    cbFailureCount,
    cbSetTestClock,
    cbReset,
    cbBeforeHandle,
} from "@web/circuit-breaker";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CircuitState` | `Closed, Open, HalfOpen` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CircuitBreakerConfig` | `class` | 字段: `failure_threshold: u32, reset_timeout_secs: u64, retry_after_secs: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cbNowSec` | `(): u64` | 调用 `cbNowSec` 并返回 `u64`。 |
| `cbState` | `(): CircuitState` | 调用 `cbState` 并返回 `CircuitState`。 |
| `cbFailureCount` | `(): u32` | 调用 `cbFailureCount` 并返回 `u32`。 |
| `cbSetTestClock` | `(now_sec: u64)` | 就地执行 `cbSetTestClock` 操作。 |
| `cbReset` | `()` | 就地执行 `cbReset` 操作。 |
| `cbBeforeHandle` | `(ctx: &mut Context): bool` | 调用 `cbBeforeHandle` 并返回 `bool`。 |
| `cbAfterHandle` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `cbAfterHandle` 操作。 |
| `cbCreatePlugin` | `(cfg: CircuitBreakerConfig): Plugin` | 调用 `cbCreatePlugin` 并返回 `Plugin`。 |
| `registerCircuitBreaker` | `(app: &mut App, cfg: CircuitBreakerConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { cbNowSec } from "@web/circuit-breaker";

pub fn main() {
    let result = cbNowSec();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
