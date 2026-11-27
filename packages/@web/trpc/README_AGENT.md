# @web/trpc — AI Agent Quick Ref

Official type-safe tRPC-style query & mutation RPC router plugin for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum TrpcProcKind { Query, Mutation }
class TrpcConfig { base_path: string }
class TrpcProcEntry { name: string, kind: TrpcProcKind, handler: TrpcHandlerFn? }
```

## API Signatures

```cp
fn trpcBytesEql(a: string, b: string): bool;
fn trpcWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn trpcRegisterProc(name: string, kind: TrpcProcKind, handler: TrpcHandlerFn): bool;
fn trpcQuery(name: string, handler: TrpcHandlerFn): bool;
fn trpcMutation(name: string, handler: TrpcHandlerFn): bool;
fn trpcReset();
fn trpcFindProcIndex(name: string): usize?;
fn trpcWrapResult(raw_json: string): Response;
fn trpcGetHandler(ctx: &mut Context): Response;
fn trpcPostHandler(ctx: &mut Context): Response;
fn trpcBuildRoutePattern(base_path: string): string;
fn registerTrpc(app: &mut App, cfg: TrpcConfig): &mut App;
```

## Minimal Usage

```cp
import { trpcBytesEql } from "@web/trpc";

pub fn main() {
    let result = trpcBytesEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/trpc"`; install via `cap add @web/trpc`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
