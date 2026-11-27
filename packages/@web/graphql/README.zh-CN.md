[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/graphql

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official GraphQL endpoint, depth-limit guard & GraphiQL Playground plugin for cap:web

## 安装与导入

```bash
cap add @web/graphql
```

```cp
import { gqlBytesEql } from "@web/graphql";
```

## 核心导出 API

- `export fn gqlBytesEql(a: string, b: string): bool;`
- `export fn gqlWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn gqlWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn gqlHexLower(n: u8): u8;`
- `export fn gqlWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn gqlIsWs(c: u8): bool;`
- `export fn gqlIsIdentStart(c: u8): bool;`
- `export fn gqlIsIdentCont(c: u8): bool;`
- `export fn gqlSkipWsAndComments(s: string, start: usize): usize;`
- `export fn gqlCheckQueryDepth(query: string, max_depth: usize): bool;`
- `export fn gqlRegisterResolver(field_name: string, resolver: GraphQLResolverFn): bool;`
- `export fn gqlResetResolvers();`
- `export fn gqlFindResolver(field_name: string): GraphQLResolverFn?;`
- `export fn gqlSkipJsonWs(s: string, start: usize): usize;`
- `export fn gqlExtractJsonQuery(body: string, out_buf: []u8): usize?;`
- `export fn gqlLooksLikeJsonValue(val: string): bool;`
- `export fn gqlSkipBalanced(s: string, start: usize, open_ch: u8, close_ch: u8): usize;`
- `export fn gqlRenderPlaygroundHtml(endpoint: string, out_buf: []u8): usize?;`
- `export fn gqlGetHandler(ctx: &mut Context): Response;`
- `export fn gqlPostHandler(ctx: &mut Context): Response;`
- `export fn registerGraphQL(app: &mut App, cfg: GraphQLConfig): &mut App;`
