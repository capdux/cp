// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class TrpcConfig {
    base_path: string,
}

export enum TrpcProcKind {
    Query,
    Mutation
}

export class TrpcProcEntry {
    name: string,
    kind: TrpcProcKind,
    handler: TrpcHandlerFn?,
}

export fn trpcBytesEql(a: string, b: string): bool;
export fn trpcWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn trpcRegisterProc(name: string, kind: TrpcProcKind, handler: TrpcHandlerFn): bool;
export fn trpcQuery(name: string, handler: TrpcHandlerFn): bool;
export fn trpcMutation(name: string, handler: TrpcHandlerFn): bool;
export fn trpcReset();
export fn trpcFindProcIndex(name: string): usize?;
export fn trpcWrapResult(raw_json: string): Response;
export fn trpcGetHandler(ctx: &mut Context): Response;
export fn trpcPostHandler(ctx: &mut Context): Response;
export fn trpcBuildRoutePattern(base_path: string): string;
export fn registerTrpc(app: &mut App, cfg: TrpcConfig): &mut App;
