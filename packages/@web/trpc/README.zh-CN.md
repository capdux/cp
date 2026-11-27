[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/trpc

Official type-safe tRPC-style query & mutation RPC router plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/trpc` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/trpc
```

```cp
import {
    TrpcProcKind,
    TrpcConfig,
    TrpcProcEntry,
    trpcBytesEql,
    trpcWriteRaw,
    trpcRegisterProc,
    trpcQuery,
    trpcMutation,
} from "@web/trpc";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `TrpcProcKind` | `Query, Mutation` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TrpcConfig` | `class` | 字段: `base_path: string` |
| `TrpcProcEntry` | `class` | 字段: `name: string, kind: TrpcProcKind, handler: TrpcHandlerFn?` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `trpcBytesEql` | `(a: string, b: string): bool` | 调用 `trpcBytesEql` 并返回 `bool`。 |
| `trpcWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `trpcWriteRaw` 并返回 `bool`。 |
| `trpcRegisterProc` | `(name: string, kind: TrpcProcKind, handler: TrpcHandlerFn): bool` | 调用 `trpcRegisterProc` 并返回 `bool`。 |
| `trpcQuery` | `(name: string, handler: TrpcHandlerFn): bool` | 调用 `trpcQuery` 并返回 `bool`。 |
| `trpcMutation` | `(name: string, handler: TrpcHandlerFn): bool` | 调用 `trpcMutation` 并返回 `bool`。 |
| `trpcReset` | `()` | 就地执行 `trpcReset` 操作。 |
| `trpcFindProcIndex` | `(name: string): usize?` | 调用 `trpcFindProcIndex` 并返回 `usize?`。 |
| `trpcWrapResult` | `(raw_json: string): Response` | 调用 `trpcWrapResult` 并返回 `Response`。 |
| `trpcGetHandler` | `(ctx: &mut Context): Response` | 调用 `trpcGetHandler` 并返回 `Response`。 |
| `trpcPostHandler` | `(ctx: &mut Context): Response` | 调用 `trpcPostHandler` 并返回 `Response`。 |
| `trpcBuildRoutePattern` | `(base_path: string): string` | 调用 `trpcBuildRoutePattern` 并返回 `string`。 |
| `registerTrpc` | `(app: &mut App, cfg: TrpcConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { trpcBytesEql } from "@web/trpc";

pub fn main() {
    let result = trpcBytesEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
