// Official @web/trpc Plugin — Type-Safe tRPC-Style Query & Mutation RPC Router for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const TRPC_MAX_PROCS: usize = 64;
pub const TRPC_RESP_BUF_CAP: usize = 16384;
pub const TRPC_ROUTE_BUF_CAP: usize = 256;

pub struct TrpcConfig {
    base_path: string = "/trpc",
}

pub enum TrpcProcKind {
    Query,
    Mutation,
}

pub type TrpcHandlerFn = *const fn (*Context, string): string;

struct TrpcProcEntry {
    name: string = "",
    kind: TrpcProcKind = TrpcProcKind.Query,
    handler: TrpcHandlerFn? = null,
}

let trpc_cfg: TrpcConfig = TrpcConfig{};
let trpc_procs: [TRPC_MAX_PROCS]TrpcProcEntry = [TrpcProcEntry{}; TRPC_MAX_PROCS];
let trpc_proc_len: usize = 0;
let trpc_resp_buf: [TRPC_RESP_BUF_CAP]u8 = [0; TRPC_RESP_BUF_CAP];
let trpc_route_buf: [TRPC_ROUTE_BUF_CAP]u8 = [0; TRPC_ROUTE_BUF_CAP];
let trpc_route_len: usize = 0;

fn trpcBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn trpcWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn trpcRegisterProc(name: string, kind: TrpcProcKind, handler: TrpcHandlerFn): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < trpc_proc_len) : (i += 1) {
        if (trpcBytesEql(trpc_procs[i].name, name)) {
            trpc_procs[i].kind = kind;
            trpc_procs[i].handler = handler;
            return true;
        }
    }
    if (trpc_proc_len >= TRPC_MAX_PROCS) return false;
    trpc_procs[trpc_proc_len] = TrpcProcEntry{
        name: name,
        kind: kind,
        handler: handler,
    };
    trpc_proc_len += 1;
    return true;
}

/// Register a tRPC Query procedure (invoked via `GET <base_path>/:proc?input=...`).
pub fn trpcQuery(name: string, handler: TrpcHandlerFn): bool {
    return trpcRegisterProc(name, TrpcProcKind.Query, handler);
}

/// Register a tRPC Mutation procedure (invoked via `POST <base_path>/:proc` with JSON body).
pub fn trpcMutation(name: string, handler: TrpcHandlerFn): bool {
    return trpcRegisterProc(name, TrpcProcKind.Mutation, handler);
}

/// Clear all registered tRPC procedures.
pub fn trpcReset() {
    trpc_proc_len = 0;
}

fn trpcFindProcIndex(name: string): usize? {
    let i: usize = 0;
    while (i < trpc_proc_len) : (i += 1) {
        if (trpcBytesEql(trpc_procs[i].name, name)) {
            return i;
        }
    }
    return null;
}

fn trpcWrapResult(raw_json: string): Response {
    let pos: usize = 0;
    const payload = if (raw_json.len > 0) raw_json else "null";
    if (!trpcWriteRaw(trpc_resp_buf[0..], *pos, "{\"result\":{\"data\":")) {
        return Response.json(500, "{\"error\":{\"code\":\"INTERNAL_SERVER_ERROR\"}}");
    }
    if (!trpcWriteRaw(trpc_resp_buf[0..], *pos, payload)) {
        return Response.json(500, "{\"error\":{\"code\":\"INTERNAL_SERVER_ERROR\"}}");
    }
    if (!trpcWriteRaw(trpc_resp_buf[0..], *pos, "}}")) {
        return Response.json(500, "{\"error\":{\"code\":\"INTERNAL_SERVER_ERROR\"}}");
    }
    return Response.json(200, trpc_resp_buf[0..pos]);
}

fn trpcGetHandler(ctx: *Context): Response {
    if (proc_name := ctx.param("proc")) {
        if (idx := trpcFindProcIndex(proc_name)) {
            const entry = trpc_procs[idx];
            if (entry.kind != TrpcProcKind.Query) {
                return Response.json(405, "{\"error\":{\"code\":\"METHOD_NOT_ALLOWED\"}}");
            }
            if (h := entry.handler) {
                let input_str: string = "";
                if (q := ctx.query("input")) {
                    input_str = q;
                }
                const out_json = h(ctx, input_str);
                return trpcWrapResult(out_json);
            }
        }
    }
    return Response.json(404, "{\"error\":{\"code\":\"NOT_FOUND\"}}");
}

fn trpcPostHandler(ctx: *Context): Response {
    if (proc_name := ctx.param("proc")) {
        if (idx := trpcFindProcIndex(proc_name)) {
            const entry = trpc_procs[idx];
            if (entry.kind != TrpcProcKind.Mutation) {
                return Response.json(405, "{\"error\":{\"code\":\"METHOD_NOT_ALLOWED\"}}");
            }
            if (h := entry.handler) {
                const out_json = h(ctx, ctx.body);
                return trpcWrapResult(out_json);
            }
        }
    }
    return Response.json(404, "{\"error\":{\"code\":\"NOT_FOUND\"}}");
}

fn trpcBuildRoutePattern(base_path: string): string {
    let trimmed = base_path;
    while (trimmed.len > 0 && trimmed[trimmed.len - 1] == '/') {
        trimmed = trimmed[0 .. trimmed.len - 1];
    }
    let pos: usize = 0;
    if (trimmed.len > 0) {
        if (trimmed[0] != '/') {
            * = trpcWriteRaw(trpc_route_buf[0..], *pos, "/");
        }
        * = trpcWriteRaw(trpc_route_buf[0..], *pos, trimmed);
    }
    * = trpcWriteRaw(trpc_route_buf[0..], *pos, "/:proc");
    trpc_route_len = pos;
    return trpc_route_buf[0..trpc_route_len];
}

/// Register the tRPC `GET <base_path>/:proc` (Query) and `POST <base_path>/:proc` (Mutation) endpoints on `app`.
pub fn registerTrpc(app: *App, cfg: TrpcConfig): *App {
    trpc_cfg = cfg;
    const route_pattern = trpcBuildRoutePattern(cfg.base_path);
    let p = Plugin.init("@web/trpc");
    * = p.withScope(PluginScope.Global);
    * = p.route(AppMethod.GET, route_pattern, "tRPC Query", &trpcGetHandler);
    * = p.route(AppMethod.POST, route_pattern, "tRPC Mutation", &trpcPostHandler);
    return app.usePlugin(&p);
}
