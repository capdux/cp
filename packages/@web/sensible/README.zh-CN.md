[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/sensible

Official RFC 9457 Problem Details & standard HTTP error helpers plugin for cap:web (inspired by @fastify/sensible)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/sensible` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/sensible
```

```cp
import {
    SensibleConfig,
    sensWriteRaw,
    sensWriteEscaped,
    sensWriteU16,
    problemResponse,
    httpBadRequest,
    httpUnauthorized,
    httpForbidden,
} from "@web/sensible";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SensibleConfig` | `class` | 字段: `expose_problem_type_base: string, default_error_header: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sensWriteRaw` | `(dst: []u8, pos: &mut usize, s: string): bool` | 调用 `sensWriteRaw` 并返回 `bool`。 |
| `sensWriteEscaped` | `(dst: []u8, pos: &mut usize, s: string): bool` | 调用 `sensWriteEscaped` 并返回 `bool`。 |
| `sensWriteU16` | `(dst: []u8, pos: &mut usize, v: u16): bool` | 调用 `sensWriteU16` 并返回 `bool`。 |
| `problemResponse` | `(status_code: u16, title: string, detail: string): Response` | 调用 `problemResponse` 并返回 `Response`。 |
| `httpBadRequest` | `(detail: string): Response` | 调用 `httpBadRequest` 并返回 `Response`。 |
| `httpUnauthorized` | `(detail: string): Response` | 调用 `httpUnauthorized` 并返回 `Response`。 |
| `httpForbidden` | `(detail: string): Response` | 调用 `httpForbidden` 并返回 `Response`。 |
| `httpNotFound` | `(detail: string): Response` | 调用 `httpNotFound` 并返回 `Response`。 |
| `httpConflict` | `(detail: string): Response` | 调用 `httpConflict` 并返回 `Response`。 |
| `httpUnprocessable` | `(detail: string): Response` | 调用 `httpUnprocessable` 并返回 `Response`。 |
| `httpTooManyRequests` | `(detail: string): Response` | 调用 `httpTooManyRequests` 并返回 `Response`。 |
| `httpInternalError` | `(detail: string): Response` | 调用 `httpInternalError` 并返回 `Response`。 |
| `sensibleAssert` | `(ctx: &mut Context, cond: bool, status_code: u16, title: string, detail: string): bool` | 调用 `sensibleAssert` 并返回 `bool`。 |
| `sensibleAfterHook` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `sensibleAfterHook` 操作。 |
| `createSensiblePlugin` | `(cfg: SensibleConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerSensible` | `(app: &mut App, cfg: SensibleConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { sensWriteRaw } from "@web/sensible";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sensWriteRaw(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
