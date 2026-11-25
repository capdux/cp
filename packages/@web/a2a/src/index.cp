// Official @web/a2a Plugin — Google Agent-to-Agent (A2A) Protocol Server for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const A2A_MAX_TASKS: usize = 32;
pub const A2A_TASK_ID_CAP: usize = 128;
pub const A2A_TASK_RESULT_CAP: usize = 2048;
pub const A2A_RESP_BUF_CAP: usize = 16384;
pub const A2A_STR_BUF_CAP: usize = 1024;

pub struct A2aConfig {
    agent_name: string = "cp-agent",
    agent_description: string = "High-performance CP AI Agent",
    agent_url: string = "http://localhost:3000/a2a",
    card_path: string = "/.well-known/agent.json",
    rpc_path: string = "/a2a",
    version: string = "1.0.0",
}

pub enum A2aTaskState {
    Submitted,
    Working,
    Completed,
    Canceled,
    Failed,

    pub fn asString(): string {
        match (_) {
            .Submitted => return "submitted",
            .Working => return "working",
            .Completed => return "completed",
            .Canceled => return "canceled",
            .Failed => return "failed",
        }
    }
}

pub type A2aTaskHandlerFn = *const fn (*Context, string): string;

struct A2aTaskEntry {
    used: bool = false,
    id_buf: [A2A_TASK_ID_CAP]u8 = [0; A2A_TASK_ID_CAP],
    id_len: usize = 0,
    state: A2aTaskState = A2aTaskState.Submitted,
    result_buf: [A2A_TASK_RESULT_CAP]u8 = [0; A2A_TASK_RESULT_CAP],
    result_len: usize = 0,
}

let a2a_cfg: A2aConfig = A2aConfig{};
let a2a_task_handler: A2aTaskHandlerFn? = null;
let a2a_tasks: [A2A_MAX_TASKS]A2aTaskEntry = [A2aTaskEntry{}; A2A_MAX_TASKS];
let a2a_task_seq: u64 = 0;
let a2a_resp_buf: [A2A_RESP_BUF_CAP]u8 = [0; A2A_RESP_BUF_CAP];
let a2a_method_buf: [A2A_STR_BUF_CAP]u8 = [0; A2A_STR_BUF_CAP];
let a2a_task_id_buf: [A2A_TASK_ID_CAP]u8 = [0; A2A_TASK_ID_CAP];

fn a2aBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn a2aWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn a2aWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn a2aWriteU64(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        return a2aWriteByte(out_buf, pos, '0');
    }
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        if (!a2aWriteByte(out_buf, pos, tmp[n])) return false;
    }
    return true;
}

fn a2aHexLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn a2aWriteJsonEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    if (!a2aWriteByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!a2aWriteRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!a2aWriteRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!a2aWriteRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!a2aWriteRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!a2aWriteRaw(out_buf, pos, "\\t")) return false;
        } else if (b < 0x20) {
            if (!a2aWriteRaw(out_buf, pos, "\\u00")) return false;
            if (!a2aWriteByte(out_buf, pos, a2aHexLower((b >> 4) & 0x0F))) return false;
            if (!a2aWriteByte(out_buf, pos, a2aHexLower(b & 0x0F))) return false;
        } else {
            if (!a2aWriteByte(out_buf, pos, b)) return false;
        }
    }
    return a2aWriteByte(out_buf, pos, '"');
}

fn a2aSkipWs(s: string, start: usize): usize {
    let i: usize = start;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
    return i;
}

fn a2aSkipJsonValue(s: string, start: usize): usize {
    let i = a2aSkipWs(s, start);
    if (i >= s.len) return i;
    const c = s[i];
    if (c == '"') {
        i += 1;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (s[i] == '"') return i + 1;
            i += 1;
        }
        return s.len;
    }
    if (c == '{' || c == '[') {
        const open_ch = c;
        const close_ch: u8 = if (c == '{') '}' else ']';
        let depth: usize = 1;
        i += 1;
        while (i < s.len && depth > 0) {
            if (s[i] == '"') {
                i += 1;
                while (i < s.len) {
                    if (s[i] == '\\' && i + 1 < s.len) {
                        i += 2;
                        continue;
                    }
                    if (s[i] == '"') {
                        i += 1;
                        break;
                    }
                    i += 1;
                }
                continue;
            }
            if (s[i] == open_ch) {
                depth += 1;
            } else if (s[i] == close_ch) {
                depth -= 1;
            }
            i += 1;
        }
        return i;
    }
    while (i < s.len) {
        const ch = s[i];
        if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            break;
        }
        i += 1;
    }
    return i;
}

fn a2aFindObjectFieldRaw(obj: string, target_key: string): string? {
    let i = a2aSkipWs(obj, 0);
    if (i >= obj.len || obj[i] != '{') return null;
    i += 1;
    while (i < obj.len) {
        i = a2aSkipWs(obj, i);
        if (i >= obj.len || obj[i] == '}') return null;
        if (obj[i] == ',') {
            i += 1;
            continue;
        }
        if (obj[i] != '"') return null;
        const key_start = i + 1;
        i += 1;
        while (i < obj.len) {
            if (obj[i] == '\\' && i + 1 < obj.len) {
                i += 2;
                continue;
            }
            if (obj[i] == '"') break;
            i += 1;
        }
        if (i >= obj.len) return null;
        const key = obj[key_start..i];
        i = a2aSkipWs(obj, i + 1);
        if (i >= obj.len || obj[i] != ':') return null;
        const val_start = a2aSkipWs(obj, i + 1);
        const val_end = a2aSkipJsonValue(obj, val_start);
        if (a2aBytesEql(key, target_key)) {
            return obj[val_start..val_end];
        }
        i = val_end;
    }
    return null;
}

fn a2aUnescapeJsonString(raw: string, out_buf: []u8): usize? {
    if (raw.len < 2 || raw[0] != '"' || raw[raw.len - 1] != '"') return null;
    let p: usize = 1;
    let out_pos: usize = 0;
    const end = raw.len - 1;
    while (p < end) {
        const ch = raw[p];
        if (ch == '\\' && p + 1 < end) {
            const nxt = raw[p + 1];
            if (nxt == '"') {
                if (!a2aWriteByte(out_buf, *out_pos, '"')) return null;
            } else if (nxt == '\\') {
                if (!a2aWriteByte(out_buf, *out_pos, '\\')) return null;
            } else if (nxt == 'n') {
                if (!a2aWriteByte(out_buf, *out_pos, '\n')) return null;
            } else if (nxt == 'r') {
                if (!a2aWriteByte(out_buf, *out_pos, '\r')) return null;
            } else if (nxt == 't') {
                if (!a2aWriteByte(out_buf, *out_pos, '\t')) return null;
            } else {
                if (!a2aWriteByte(out_buf, *out_pos, nxt)) return null;
            }
            p += 2;
            continue;
        }
        if (!a2aWriteByte(out_buf, *out_pos, ch)) return null;
        p += 1;
    }
    return out_pos;
}

/// Set the A2A task execution callback handler.
pub fn a2aSetTaskHandler(handler: A2aTaskHandlerFn) {
    a2a_task_handler = handler;
}

/// Clear all A2A tasks in the 32-slot task table and reset sequence counter.
pub fn a2aResetTasks() {
    let i: usize = 0;
    while (i < A2A_MAX_TASKS) : (i += 1) {
        a2a_tasks[i].used = false;
        a2a_tasks[i].id_len = 0;
        a2a_tasks[i].state = A2aTaskState.Submitted;
        a2a_tasks[i].result_len = 0;
    }
    a2a_task_seq = 0;
}

fn a2aFindTaskIndex(task_id: string): usize? {
    let i: usize = 0;
    while (i < A2A_MAX_TASKS) : (i += 1) {
        if (a2a_tasks[i].used && a2aBytesEql(a2a_tasks[i].id_buf[0..a2a_tasks[i].id_len], task_id)) {
            return i;
        }
    }
    return null;
}

fn a2aAllocTaskSlot(task_id: string): usize? {
    if (existing := a2aFindTaskIndex(task_id)) {
        return existing;
    }
    let i: usize = 0;
    while (i < A2A_MAX_TASKS) : (i += 1) {
        if (!a2a_tasks[i].used) {
            let pos: usize = 0;
            if (!a2aWriteRaw(a2a_tasks[i].id_buf[0..], *pos, task_id)) return null;
            a2a_tasks[i].used = true;
            a2a_tasks[i].id_len = pos;
            a2a_tasks[i].state = A2aTaskState.Submitted;
            a2a_tasks[i].result_len = 0;
            return i;
        }
    }
    return null;
}

fn a2aJsonRpcError(id_raw: string, code_str: string, message: string): Response {
    let pos: usize = 0;
    const fallback = "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}";
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, id_raw)) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"error\":{\"code\":")) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, code_str)) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"message\":")) return Response.json(500, fallback);
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, message)) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, "}}")) return Response.json(500, fallback);
    return Response.json(200, a2a_resp_buf[0..pos]);
}

fn a2aTaskJsonResponse(id_raw: string, idx: usize): Response {
    let pos: usize = 0;
    const task_id = a2a_tasks[idx].id_buf[0..a2a_tasks[idx].id_len];
    const state_str = a2a_tasks[idx].state.asString();
    const res_text = a2a_tasks[idx].result_buf[0..a2a_tasks[idx].result_len];

    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, id_raw)) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"result\":{\"id\":")) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, task_id)) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"status\":{\"state\":")) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, state_str)) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, "},\"artifacts\":[{\"parts\":[{\"type\":\"text\",\"text\":")) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, res_text)) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, "}]}]}}")) {
        return a2aJsonRpcError(id_raw, "-32603", "Buffer overflow");
    }
    return Response.json(200, a2a_resp_buf[0..pos]);
}

fn a2aCardHandler(ctx: *Context): Response {
    * = ctx;
    let pos: usize = 0;
    const fallback = "{\"error\":\"card_overflow\"}";
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, "{\"name\":")) return Response.json(500, fallback);
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, a2a_cfg.agent_name)) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"description\":")) return Response.json(500, fallback);
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, a2a_cfg.agent_description)) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"url\":")) return Response.json(500, fallback);
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, a2a_cfg.agent_url)) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"version\":")) return Response.json(500, fallback);
    if (!a2aWriteJsonEscaped(a2a_resp_buf[0..], *pos, a2a_cfg.version)) return Response.json(500, fallback);
    if (!a2aWriteRaw(a2a_resp_buf[0..], *pos, ",\"capabilities\":{\"streaming\":false,\"pushNotifications\":false,\"stateTransitionHistory\":false}}")) {
        return Response.json(500, fallback);
    }
    return Response.json(200, a2a_resp_buf[0..pos]);
}

fn a2aRpcHandler(ctx: *Context): Response {
    let id_raw: string = "null";
    if (found_id := a2aFindObjectFieldRaw(ctx.body, "id")) {
        if (found_id.len > 0) {
            id_raw = found_id;
        }
    }

    let method_len: usize = 0;
    if (method_raw := a2aFindObjectFieldRaw(ctx.body, "method")) {
        if (n := a2aUnescapeJsonString(method_raw, a2a_method_buf[0..])) {
            method_len = n;
        } else {
            return a2aJsonRpcError(id_raw, "-32600", "Invalid Request");
        }
    } else {
        return a2aJsonRpcError(id_raw, "-32600", "Invalid Request");
    }

    const method = a2a_method_buf[0..method_len];

    let params_raw: string = "{}";
    if (p_raw := a2aFindObjectFieldRaw(ctx.body, "params")) {
        params_raw = p_raw;
    }

    if (a2aBytesEql(method, "tasks/send")) {
        let task_id_len: usize = 0;
        if (tid_raw := a2aFindObjectFieldRaw(params_raw, "id")) {
            if (n := a2aUnescapeJsonString(tid_raw, a2a_task_id_buf[0..])) {
                task_id_len = n;
            } else {
                return a2aJsonRpcError(id_raw, "-32602", "Invalid params: id");
            }
        } else {
            a2a_task_seq += 1;
            let gen_pos: usize = 0;
            * = a2aWriteRaw(a2a_task_id_buf[0..], *gen_pos, "task-");
            * = a2aWriteU64(a2a_task_id_buf[0..], *gen_pos, a2a_task_seq);
            task_id_len = gen_pos;
        }

        const task_id = a2a_task_id_buf[0..task_id_len];
        if (slot := a2aAllocTaskSlot(task_id)) {
            a2a_tasks[slot].state = A2aTaskState.Working;
            let input_payload: string = params_raw;
            if (msg_raw := a2aFindObjectFieldRaw(params_raw, "message")) {
                input_payload = msg_raw;
            }
            let out_text: string = "";
            if (h := a2a_task_handler) {
                out_text = h(ctx, input_payload);
            }
            let rpos: usize = 0;
            if (out_text.len > 0) {
                const copy_len = if (out_text.len <= A2A_TASK_RESULT_CAP) out_text.len else A2A_TASK_RESULT_CAP;
                * = a2aWriteRaw(a2a_tasks[slot].result_buf[0..], *rpos, out_text[0..copy_len]);
            }
            a2a_tasks[slot].result_len = rpos;
            a2a_tasks[slot].state = A2aTaskState.Completed;
            return a2aTaskJsonResponse(id_raw, slot);
        }
        return a2aJsonRpcError(id_raw, "-32000", "Task table full");
    }

    if (a2aBytesEql(method, "tasks/get")) {
        let task_id_len: usize = 0;
        if (tid_raw := a2aFindObjectFieldRaw(params_raw, "id")) {
            if (n := a2aUnescapeJsonString(tid_raw, a2a_task_id_buf[0..])) {
                task_id_len = n;
            } else {
                return a2aJsonRpcError(id_raw, "-32602", "Invalid params: id");
            }
        } else {
            return a2aJsonRpcError(id_raw, "-32602", "Invalid params: missing id");
        }
        const task_id = a2a_task_id_buf[0..task_id_len];
        if (slot := a2aFindTaskIndex(task_id)) {
            return a2aTaskJsonResponse(id_raw, slot);
        }
        return a2aJsonRpcError(id_raw, "-32001", "Task not found");
    }

    if (a2aBytesEql(method, "tasks/cancel")) {
        let task_id_len: usize = 0;
        if (tid_raw := a2aFindObjectFieldRaw(params_raw, "id")) {
            if (n := a2aUnescapeJsonString(tid_raw, a2a_task_id_buf[0..])) {
                task_id_len = n;
            } else {
                return a2aJsonRpcError(id_raw, "-32602", "Invalid params: id");
            }
        } else {
            return a2aJsonRpcError(id_raw, "-32602", "Invalid params: missing id");
        }
        const task_id = a2a_task_id_buf[0..task_id_len];
        if (slot := a2aFindTaskIndex(task_id)) {
            a2a_tasks[slot].state = A2aTaskState.Canceled;
            return a2aTaskJsonResponse(id_raw, slot);
        }
        return a2aJsonRpcError(id_raw, "-32001", "Task not found");
    }

    return a2aJsonRpcError(id_raw, "-32601", "Method not found");
}

/// Register the A2A Agent Card (`GET cfg.card_path`) and JSON-RPC 2.0 endpoint (`POST cfg.rpc_path`) on `app`.
pub fn registerA2a(app: *App, cfg: A2aConfig): *App {
    a2a_cfg = cfg;
    let p = Plugin.init("@web/a2a");
    * = p.withScope(PluginScope.Global);
    * = p.route(AppMethod.GET, cfg.card_path, "A2A Agent Card", &a2aCardHandler);
    * = p.route(AppMethod.POST, cfg.rpc_path, "A2A JSON-RPC 2.0", &a2aRpcHandler);
    return app.usePlugin(&p);
}
