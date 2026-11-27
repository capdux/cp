[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/pressure

Official overload protection, body limit (413), load shedding (503), and K8s /healthz /readyz probes for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/pressure` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/pressure
```

```cp
import {
    PressureConfig,
    pressureSetReady,
    pressureSetInflightForTest,
    pressureShedCount,
    pressureResetStats,
    pressStrEql,
    pressureOnRequest,
    pressureOnAfter,
} from "@web/pressure";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PressureConfig` | `class` | 字段: `max_body_bytes: usize, max_inflight: u32, retry_after_secs: string, health_path: string, ready_path: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pressureSetReady` | `(ready: bool)` | 就地执行 `pressureSetReady` 操作。 |
| `pressureSetInflightForTest` | `(n: u32)` | 就地执行 `pressureSetInflightForTest` 操作。 |
| `pressureShedCount` | `(): u64` | 调用 `pressureShedCount` 并返回 `u64`。 |
| `pressureResetStats` | `()` | 就地执行 `pressureResetStats` 操作。 |
| `pressStrEql` | `(a: string, b: string): bool` | 调用 `pressStrEql` 并返回 `bool`。 |
| `pressureOnRequest` | `(ctx: &mut Context): bool` | 调用 `pressureOnRequest` 并返回 `bool`。 |
| `pressureOnAfter` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `pressureOnAfter` 操作。 |
| `handleHealthz` | `(ctx: &mut Context): Response` | 调用 `handleHealthz` 并返回 `Response`。 |
| `handleReadyz` | `(ctx: &mut Context): Response` | 调用 `handleReadyz` 并返回 `Response`。 |
| `createPressurePlugin` | `(cfg: PressureConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerPressure` | `(app: &mut App, cfg: PressureConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { pressStrEql } from "@web/pressure";

pub fn main() {
    let result = pressStrEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
