[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/trpc

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official type-safe tRPC-style query & mutation RPC router plugin for cap:web

## 安装与导入

```bash
cap add @web/trpc
```

```cp
import { trpcBytesEql } from "@web/trpc";
```

## 核心导出 API

- `export enum TrpcProcKind`
- `export fn trpcBytesEql(a: string, b: string): bool;`
- `export fn trpcWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn trpcRegisterProc(name: string, kind: TrpcProcKind, handler: TrpcHandlerFn): bool;`
- `export fn trpcQuery(name: string, handler: TrpcHandlerFn): bool;`
- `export fn trpcMutation(name: string, handler: TrpcHandlerFn): bool;`
- `export fn trpcReset();`
- `export fn trpcFindProcIndex(name: string): usize?;`
- `export fn trpcWrapResult(raw_json: string): Response;`
- `export fn trpcGetHandler(ctx: &mut Context): Response;`
- `export fn trpcPostHandler(ctx: &mut Context): Response;`
- `export fn trpcBuildRoutePattern(base_path: string): string;`
- `export fn registerTrpc(app: &mut App, cfg: TrpcConfig): &mut App;`
