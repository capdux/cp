[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/logger

Official production observability plugin for cap:web (X-Request-Id, W3C traceparent, JSON access logs, OpenMetrics /metrics)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/logger` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/logger
```

```cp
import {
    LoggerConfig,
    MetricsSnapshot,
    logWriteByte,
    logWriteRaw,
    logHexDigitLower,
    logWriteU64Decimal,
    logWriteU64HexFixed16,
    logWriteJsonEscaped,
} from "@web/logger";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LoggerConfig` | `class` | 字段: `request_id_header: string, metrics_path: string, enable_trace: bool` |
| `MetricsSnapshot` | `class` | 字段: `requests_total: u64, responses_2xx_total: u64, responses_4xx_total: u64, responses_5xx_total: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `logWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `logWriteByte` 并返回 `bool`。 |
| `logWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `logWriteRaw` 并返回 `bool`。 |
| `logHexDigitLower` | `(n: u8): u8` | 调用 `logHexDigitLower` 并返回 `u8`。 |
| `logWriteU64Decimal` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `logWriteU64Decimal` 并返回 `bool`。 |
| `logWriteU64HexFixed16` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `logWriteU64HexFixed16` 并返回 `bool`。 |
| `logWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `logWriteJsonEscaped` 并返回 `bool`。 |
| `formatAccessLogJson` | `(method: string, path: string, status: u16, req_id: string, out_buf: []u8): usize?` | 通过 `formatAccessLogJson` 编码或格式化输出，返回 `usize?`。 |
| `formatTraceparent` | `(seq: u64, out_buf: []u8): usize?` | 通过 `formatTraceparent` 编码或格式化输出，返回 `usize?`。 |
| `formatOpenMetrics` | `(snap: MetricsSnapshot, out_buf: []u8): usize?` | 通过 `formatOpenMetrics` 编码或格式化输出，返回 `usize?`。 |
| `resetState` | `()` | 就地执行 `resetState` 操作。 |
| `getMetrics` | `(): MetricsSnapshot` | 查询或读取目标数据，返回 `MetricsSnapshot`。 |
| `lastAccessLog` | `(): string` | 调用 `lastAccessLog` 并返回 `string`。 |
| `onRequestHook` | `(ctx: &mut Context): bool` | 调用 `onRequestHook` 并返回 `bool`。 |
| `onAfterHook` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `onAfterHook` 操作。 |
| `metricsHandler` | `(ctx: &mut Context): Response` | 调用 `metricsHandler` 并返回 `Response`。 |
| `createLoggerPlugin` | `(cfg: LoggerConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerLogger` | `(app: &mut App, cfg: LoggerConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { formatAccessLogJson } from "@web/logger";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatAccessLogJson("example", "example", 0, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
