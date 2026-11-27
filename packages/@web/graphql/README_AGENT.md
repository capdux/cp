# @web/graphql — AI Agent Quick Ref

Official GraphQL endpoint, depth-limit guard & GraphiQL Playground plugin for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class GraphQLConfig { endpoint_path: string, enable_playground: bool, max_depth: usize }
class GqlResolverEntry { name: string, resolver: GraphQLResolverFn? }
```

## API Signatures

```cp
fn gqlBytesEql(a: string, b: string): bool;
fn gqlWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn gqlWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn gqlHexLower(n: u8): u8;
fn gqlWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
fn gqlIsWs(c: u8): bool;
fn gqlIsIdentStart(c: u8): bool;
fn gqlIsIdentCont(c: u8): bool;
fn gqlSkipWsAndComments(s: string, start: usize): usize;
fn gqlCheckQueryDepth(query: string, max_depth: usize): bool;
fn gqlRegisterResolver(field_name: string, resolver: GraphQLResolverFn): bool;
fn gqlResetResolvers();
fn gqlFindResolver(field_name: string): GraphQLResolverFn?;
fn gqlSkipJsonWs(s: string, start: usize): usize;
fn gqlExtractJsonQuery(body: string, out_buf: []u8): usize?;
fn gqlLooksLikeJsonValue(val: string): bool;
fn gqlSkipBalanced(s: string, start: usize, open_ch: u8, close_ch: u8): usize;
fn gqlRenderPlaygroundHtml(endpoint: string, out_buf: []u8): usize?;
fn gqlGetHandler(ctx: &mut Context): Response;
fn gqlPostHandler(ctx: &mut Context): Response;
fn registerGraphQL(app: &mut App, cfg: GraphQLConfig): &mut App;
```

## Minimal Usage

```cp
import { gqlBytesEql } from "@web/graphql";

pub fn main() {
    let result = gqlBytesEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/graphql"`; install via `cap add @web/graphql`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
