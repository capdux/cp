[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/graphql

Official GraphQL endpoint, depth-limit guard & GraphiQL Playground plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/graphql` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/graphql
```

```cp
import {
    GraphQLConfig,
    GqlResolverEntry,
    gqlBytesEql,
    gqlWriteByte,
    gqlWriteRaw,
    gqlHexLower,
    gqlWriteJsonEscaped,
    gqlIsWs,
} from "@web/graphql";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GraphQLConfig` | `class` | 字段: `endpoint_path: string, enable_playground: bool, max_depth: usize` |
| `GqlResolverEntry` | `class` | 字段: `name: string, resolver: GraphQLResolverFn?` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `gqlBytesEql` | `(a: string, b: string): bool` | 调用 `gqlBytesEql` 并返回 `bool`。 |
| `gqlWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `gqlWriteByte` 并返回 `bool`。 |
| `gqlWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `gqlWriteRaw` 并返回 `bool`。 |
| `gqlHexLower` | `(n: u8): u8` | 调用 `gqlHexLower` 并返回 `u8`。 |
| `gqlWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `gqlWriteJsonEscaped` 并返回 `bool`。 |
| `gqlIsWs` | `(c: u8): bool` | 调用 `gqlIsWs` 并返回 `bool`。 |
| `gqlIsIdentStart` | `(c: u8): bool` | 调用 `gqlIsIdentStart` 并返回 `bool`。 |
| `gqlIsIdentCont` | `(c: u8): bool` | 调用 `gqlIsIdentCont` 并返回 `bool`。 |
| `gqlSkipWsAndComments` | `(s: string, start: usize): usize` | 调用 `gqlSkipWsAndComments` 并返回 `usize`。 |
| `gqlCheckQueryDepth` | `(query: string, max_depth: usize): bool` | 调用 `gqlCheckQueryDepth` 并返回 `bool`。 |
| `gqlRegisterResolver` | `(field_name: string, resolver: GraphQLResolverFn): bool` | 调用 `gqlRegisterResolver` 并返回 `bool`。 |
| `gqlResetResolvers` | `()` | 就地执行 `gqlResetResolvers` 操作。 |
| `gqlFindResolver` | `(field_name: string): GraphQLResolverFn?` | 调用 `gqlFindResolver` 并返回 `GraphQLResolverFn?`。 |
| `gqlSkipJsonWs` | `(s: string, start: usize): usize` | 调用 `gqlSkipJsonWs` 并返回 `usize`。 |
| `gqlExtractJsonQuery` | `(body: string, out_buf: []u8): usize?` | 调用 `gqlExtractJsonQuery` 并返回 `usize?`。 |
| `gqlLooksLikeJsonValue` | `(val: string): bool` | 调用 `gqlLooksLikeJsonValue` 并返回 `bool`。 |
| `gqlSkipBalanced` | `(s: string, start: usize, open_ch: u8, close_ch: u8): usize` | 调用 `gqlSkipBalanced` 并返回 `usize`。 |
| `gqlRenderPlaygroundHtml` | `(endpoint: string, out_buf: []u8): usize?` | 调用 `gqlRenderPlaygroundHtml` 并返回 `usize?`。 |
| `gqlGetHandler` | `(ctx: &mut Context): Response` | 调用 `gqlGetHandler` 并返回 `Response`。 |
| `gqlPostHandler` | `(ctx: &mut Context): Response` | 调用 `gqlPostHandler` 并返回 `Response`。 |
| `registerGraphQL` | `(app: &mut App, cfg: GraphQLConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { gqlBytesEql } from "@web/graphql";

pub fn main() {
    let result = gqlBytesEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
