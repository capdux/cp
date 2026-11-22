// Origin: Pure-CP Stdlib (MCP Tool Catalog, Policy-Gated Tool Dispatcher & Zero-Alloc CallToolResult Builder) | Created: 2026-11-22
// Pure CP zero-heap-allocation Model Context Protocol (MCP) tool server dispatch engine:
//   1. `McpToolCatalog` / `McpToolSpec`: fixed-slot tool registry with risk/concurrency metadata & `tools/list` JSON emission
//   2. `McpToolDispatcher` / `McpCallRequest` / `McpDispatchDecision`: deterministic tool lookup, required-param validation, read-only/approval/concurrency gating & per-tool metrics
//   3. `McpToolResultBuilder`: zero-heap RFC 8259 escaped MCP `CallToolResult` JSON response builder

const std = @import("std");

// ============================================================================
// Internal Buffer, JSON Escaping & Top-Level Argument Key Helpers
// ============================================================================

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn writeByte(out_buf: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeRaw(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn hexDigitLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn writeJsonString(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (!writeByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b: u8 = s[i];
        if (b == '"') {
            if (!writeRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!writeRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!writeRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!writeRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!writeRaw(out_buf, pos, "\\t")) return false;
        } else if (b == 0x08) {
            if (!writeRaw(out_buf, pos, "\\b")) return false;
        } else if (b == 0x0C) {
            if (!writeRaw(out_buf, pos, "\\f")) return false;
        } else if (b < 0x20) {
            if (!writeRaw(out_buf, pos, "\\u00")) return false;
            if (!writeByte(out_buf, pos, hexDigitLower((b >> 4) & 0x0F))) return false;
            if (!writeByte(out_buf, pos, hexDigitLower(b & 0x0F))) return false;
        } else {
            if (!writeByte(out_buf, pos, b)) return false;
        }
    }
    return writeByte(out_buf, pos, '"');
}

fn skipJsonString(text: string, start: usize): usize? {
    if (start >= text.len || text[start] != '"') return null;
    let i: usize = start + 1;
    let escaped = false;
    while (i < text.len) : (i += 1) {
        const c: u8 = text[i];
        if (escaped) {
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            return i + 1;
        }
    }
    return null;
}

fn skipJsonValue(text: string, start: usize): usize? {
    let i: usize = start;
    while (i < text.len && isWs(text[i])) : (i += 1) {}
    if (i >= text.len) return null;

    const c: u8 = text[i];
    if (c == '"') return skipJsonString(text, i);
    if (c == '{' || c == '[') {
        let depth: usize = 0;
        let in_str = false;
        let esc = false;
        while (i < text.len) : (i += 1) {
            const ch: u8 = text[i];
            if (in_str) {
                if (esc) {
                    esc = false;
                } else if (ch == '\\') {
                    esc = true;
                } else if (ch == '"') {
                    in_str = false;
                }
                continue;
            }
            if (ch == '"') {
                in_str = true;
            } else if (ch == '{' || ch == '[') {
                depth += 1;
            } else if (ch == '}' || ch == ']') {
                if (depth > 0) {
                    depth -= 1;
                    if (depth == 0) return i + 1;
                }
            }
        }
        return null;
    }

    while (i < text.len && !isWs(text[i]) && text[i] != ',' && text[i] != '}' && text[i] != ']') : (i += 1) {}
    return i;
}

fn jsonObjectHasTopLevelKey(json_text: string, target_key: string): bool {
    let i: usize = 0;
    while (i < json_text.len && isWs(json_text[i])) : (i += 1) {}
    if (i >= json_text.len || json_text[i] != '{') return false;
    i += 1;

    while (i < json_text.len) {
        while (i < json_text.len && isWs(json_text[i])) : (i += 1) {}
        if (i >= json_text.len || json_text[i] == '}') return false;
        if (json_text[i] == ',') {
            i += 1;
            continue;
        }
        if (json_text[i] != '"') return false;

        const key_start: usize = i + 1;
        const key_end_quote: usize = skipJsonString(json_text, i) ?? return false;
        const key_slice: string = json_text[key_start .. key_end_quote - 1];
        i = key_end_quote;

        while (i < json_text.len && isWs(json_text[i])) : (i += 1) {}
        if (i >= json_text.len || json_text[i] != ':') return false;
        i += 1;

        if (std.mem.eql(u8, key_slice, target_key)) return true;
        i = skipJsonValue(json_text, i) ?? return false;
    }
    return false;
}

fn isJsonObjectText(text: string): bool {
    let i: usize = 0;
    while (i < text.len && isWs(text[i])) : (i += 1) {}
    return i < text.len && text[i] == '{';
}

// ============================================================================
// 1. MCP Tool Catalog & `tools/list` Emitter (`McpToolCatalog`)
// ============================================================================

/// Risk / access classification for an MCP tool.
pub enum McpToolAccess {
    ReadOnly,
    Mutating,
    ApprovalRequired,
}

/// Fixed-slot descriptor and runtime metrics for a registered MCP tool.
pub struct McpToolSpec {
    occupied: bool = false;
    tool_id: u32 = 0;
    name: string = "";
    description: string = "";
    required_params_csv: string = "";
    access: McpToolAccess = McpToolAccess.ReadOnly;
    max_concurrency: u16 = 0;
    inflight: u16 = 0;
    call_count: u64 = 0;
    error_count: u64 = 0;
}

/// Zero-heap-allocation MCP tool catalog with duplicate prevention and `tools/list` JSON emission.
pub struct McpToolCatalog {
    slots: []McpToolSpec;
    active_count: usize = 0;

    /// Initializes an empty catalog backed by caller-provided `slots`.
    static fn init(slots: *[]McpToolSpec): McpToolCatalog {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = McpToolSpec{};
        }
        return McpToolCatalog{
            slots: slots,
            active_count: 0,
        };
    }

    /// Registers a new MCP tool. Rejects empty `name`, duplicate `name` or `tool_id`, or full catalog.
    fn register(
        *_,
        tool_id: u32,
        name: string,
        description: string,
        required_params_csv: string,
        access: McpToolAccess,
        max_concurrency: u16,
    ): bool {
        if (name.len == 0) return false;

        let free_idx: usize = _.slots.len;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].tool_id == tool_id) return false;
                if (std.mem.eql(u8, _.slots[i].name, name)) return false;
            } else if (free_idx == _.slots.len) {
                free_idx = i;
            }
        }

        if (free_idx >= _.slots.len) return false;

        _.slots[free_idx] = McpToolSpec{
            occupied: true,
            tool_id: tool_id,
            name: name,
            description: description,
            required_params_csv: required_params_csv,
            access: access,
            max_concurrency: max_concurrency,
            inflight: 0,
            call_count: 0,
            error_count: 0,
        };
        _.active_count += 1;
        return true;
    }

    /// Unregisters a tool by exact `name`. Returns `true` if found and removed.
    fn unregister(*_, name: string): bool {
        if (name.len == 0) return false;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && std.mem.eql(u8, _.slots[i].name, name)) {
                _.slots[i] = McpToolSpec{};
                if (_.active_count > 0) _.active_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Looks up a registered tool by exact `name`, returning a copy of its spec or `null`.
    fn findByName(name: string): McpToolSpec? {
        if (name.len == 0) return null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && std.mem.eql(u8, _.slots[i].name, name)) {
                return _.slots[i];
            }
        }
        return null;
    }

    /// Returns the number of currently registered tools.
    fn count(): usize {
        return _.active_count;
    }

    /// Returns the fixed slot capacity of this catalog.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Emits an MCP-compliant `tools/list` JSON object into `out_buf`:
    /// `{"tools":[{"name":...,"description":...,"inputSchema":{"type":"object","properties":{...},"required":[...]}}]}`.
    /// Returns the number of bytes written, or `null` if `out_buf` is too small.
    fn renderToolsListJson(out_buf: *[]u8): usize? {
        let pos: usize = 0;
        if (!writeRaw(out_buf, &pos, "{\"tools\":[")) return null;

        let first_tool = true;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            const spec = _.slots[i];

            if (!first_tool) {
                if (!writeByte(out_buf, &pos, ',')) return null;
            }
            first_tool = false;

            if (!writeRaw(out_buf, &pos, "{\"name\":")) return null;
            if (!writeJsonString(out_buf, &pos, spec.name)) return null;
            if (!writeRaw(out_buf, &pos, ",\"description\":")) return null;
            if (!writeJsonString(out_buf, &pos, spec.description)) return null;
            if (!writeRaw(out_buf, &pos, ",\"inputSchema\":{\"type\":\"object\",\"properties\":{")) return null;

            // Emit properties for each required parameter in CSV
            let first_prop = true;
            let p: usize = 0;
            while (p < spec.required_params_csv.len) {
                let seg_end: usize = p;
                while (seg_end < spec.required_params_csv.len && spec.required_params_csv[seg_end] != ',') : (seg_end += 1) {}
                let s: usize = p;
                let e: usize = seg_end;
                while (s < e && isWs(spec.required_params_csv[s])) : (s += 1) {}
                while (e > s && isWs(spec.required_params_csv[e - 1])) : (e -= 1) {}
                if (e > s) {
                    if (!first_prop) {
                        if (!writeByte(out_buf, &pos, ',')) return null;
                    }
                    first_prop = false;
                    if (!writeJsonString(out_buf, &pos, spec.required_params_csv[s..e])) return null;
                    if (!writeRaw(out_buf, &pos, ":{}")) return null;
                }
                p = if (seg_end < spec.required_params_csv.len) seg_end + 1 else seg_end;
            }

            if (!writeRaw(out_buf, &pos, "},\"required\":[")) return null;
            let first_req = true;
            let q: usize = 0;
            while (q < spec.required_params_csv.len) {
                let seg_end2: usize = q;
                while (seg_end2 < spec.required_params_csv.len && spec.required_params_csv[seg_end2] != ',') : (seg_end2 += 1) {}
                let s2: usize = q;
                let e2: usize = seg_end2;
                while (s2 < e2 && isWs(spec.required_params_csv[s2])) : (s2 += 1) {}
                while (e2 > s2 && isWs(spec.required_params_csv[e2 - 1])) : (e2 -= 1) {}
                if (e2 > s2) {
                    if (!first_req) {
                        if (!writeByte(out_buf, &pos, ',')) return null;
                    }
                    first_req = false;
                    if (!writeJsonString(out_buf, &pos, spec.required_params_csv[s2..e2])) return null;
                }
                q = if (seg_end2 < spec.required_params_csv.len) seg_end2 + 1 else seg_end2;
            }

            if (!writeRaw(out_buf, &pos, "]}}")) return null;
        }

        if (!writeRaw(out_buf, &pos, "]}")) return null;
        return pos;
    }
}

// ============================================================================
// 2. Deterministic Policy-Gated Tool Dispatcher (`McpToolDispatcher`)
// ============================================================================

/// Incoming MCP `tools/call` dispatch request.
pub struct McpCallRequest {
    tool_name: string = "";
    arguments_json: string = "{}";
    read_only_mode: bool = false;
    approved: bool = false;
}

/// Outcome of `McpToolDispatcher.dispatch`.
pub enum McpDispatchStatus {
    Allowed,
    ToolNotFound,
    MissingRequiredParam,
    ReadOnlyBlocked,
    ApprovalRequired,
    ConcurrencyLimitReached,
    InvalidArguments,
}

/// Structured routing decision returned by `McpToolDispatcher.dispatch`.
pub struct McpDispatchDecision {
    status: McpDispatchStatus = McpDispatchStatus.ToolNotFound;
    tool_id: u32 = 0;
    slot_index: usize = 0;
    missing_param: string = "";
}

/// Zero-heap-allocation MCP tool call dispatcher enforcing access policy, required arguments,
/// per-tool concurrency limits, and call/error telemetry.
pub struct McpToolDispatcher {
    default_read_only: bool = false;
    total_allowed: u64 = 0;
    total_denied: u64 = 0;

    /// Initializes a dispatcher with an optional server-wide `default_read_only` mode.
    static fn init(default_read_only: bool): McpToolDispatcher {
        return McpToolDispatcher{
            default_read_only: default_read_only,
            total_allowed: 0,
            total_denied: 0,
        };
    }

    /// Evaluates policy gates, required top-level JSON parameters, and concurrency limits
    /// for `req` against `catalog`. On `.Allowed`, atomically increments `inflight` and `call_count`.
    fn dispatch(*_, catalog: *McpToolCatalog, req: McpCallRequest): McpDispatchDecision {
        // 1. Locate tool by exact name
        let found_idx: usize = catalog.slots.len;
        let i: usize = 0;
        while (i < catalog.slots.len) : (i += 1) {
            if (catalog.slots[i].occupied && std.mem.eql(u8, catalog.slots[i].name, req.tool_name)) {
                found_idx = i;
                break;
            }
        }
        if (found_idx >= catalog.slots.len) {
            _.total_denied += 1;
            return McpDispatchDecision{ status: McpDispatchStatus.ToolNotFound };
        }

        const tool_id: u32 = catalog.slots[found_idx].tool_id;
        const access = catalog.slots[found_idx].access;

        // 2. Read-only & HITL approval gates
        if ((req.read_only_mode || _.default_read_only) && access != McpToolAccess.ReadOnly) {
            _.total_denied += 1;
            return McpDispatchDecision{
                status: McpDispatchStatus.ReadOnlyBlocked,
                tool_id: tool_id,
                slot_index: found_idx,
            };
        }
        if (access == McpToolAccess.ApprovalRequired && !req.approved) {
            _.total_denied += 1;
            return McpDispatchDecision{
                status: McpDispatchStatus.ApprovalRequired,
                tool_id: tool_id,
                slot_index: found_idx,
            };
        }

        // 3. Validate JSON object arguments & required top-level params
        if (!isJsonObjectText(req.arguments_json)) {
            _.total_denied += 1;
            return McpDispatchDecision{
                status: McpDispatchStatus.InvalidArguments,
                tool_id: tool_id,
                slot_index: found_idx,
            };
        }

        const req_csv: string = catalog.slots[found_idx].required_params_csv;
        let p: usize = 0;
        while (p < req_csv.len) {
            let seg_end: usize = p;
            while (seg_end < req_csv.len && req_csv[seg_end] != ',') : (seg_end += 1) {}
            let s: usize = p;
            let e: usize = seg_end;
            while (s < e && isWs(req_csv[s])) : (s += 1) {}
            while (e > s && isWs(req_csv[e - 1])) : (e -= 1) {}
            if (e > s) {
                const param_name: string = req_csv[s..e];
                if (!jsonObjectHasTopLevelKey(req.arguments_json, param_name)) {
                    _.total_denied += 1;
                    return McpDispatchDecision{
                        status: McpDispatchStatus.MissingRequiredParam,
                        tool_id: tool_id,
                        slot_index: found_idx,
                        missing_param: param_name,
                    };
                }
            }
            p = if (seg_end < req_csv.len) seg_end + 1 else seg_end;
        }

        // 4. Per-tool concurrency cap
        const max_conc: u16 = catalog.slots[found_idx].max_concurrency;
        if (max_conc > 0 && catalog.slots[found_idx].inflight >= max_conc) {
            _.total_denied += 1;
            return McpDispatchDecision{
                status: McpDispatchStatus.ConcurrencyLimitReached,
                tool_id: tool_id,
                slot_index: found_idx,
            };
        }

        catalog.slots[found_idx].inflight += 1;
        catalog.slots[found_idx].call_count += 1;
        _.total_allowed += 1;

        return McpDispatchDecision{
            status: McpDispatchStatus.Allowed,
            tool_id: tool_id,
            slot_index: found_idx,
            missing_param: "",
        };
    }

    /// Marks an in-flight tool call as finished, releasing its concurrency slot and recording errors.
    fn finishCall(*_, catalog: *McpToolCatalog, slot_index: usize, is_error: bool): bool {
        * = _.total_allowed;
        if (slot_index >= catalog.slots.len || !catalog.slots[slot_index].occupied) return false;
        if (catalog.slots[slot_index].inflight > 0) {
            catalog.slots[slot_index].inflight -= 1;
        }
        if (is_error) {
            catalog.slots[slot_index].error_count += 1;
        }
        return true;
    }

    /// Returns total allowed dispatches.
    fn allowedCount(): u64 {
        return _.total_allowed;
    }

    /// Returns total denied dispatches.
    fn deniedCount(): u64 {
        return _.total_denied;
    }
}

// ============================================================================
// 3. Zero-Alloc MCP `CallToolResult` JSON Builder (`McpToolResultBuilder`)
// ============================================================================

/// Zero-heap-allocation builder for MCP `CallToolResult` JSON payloads:
/// `{"content":[{"type":"text","text":...}, ...],"isError":false}`.
pub struct McpToolResultBuilder {
    out_buf: []u8;
    pos: usize = 0;
    item_count: usize = 0;
    is_error: bool = false;
    overflowed: bool = false;

    /// Initializes a `CallToolResult` builder writing directly into caller-provided `out_buf`.
    static fn init(out_buf: *[]u8, is_error: bool): McpToolResultBuilder {
        let b = McpToolResultBuilder{
            out_buf: out_buf,
            pos: 0,
            item_count: 0,
            is_error: is_error,
            overflowed: false,
        };
        if (!writeRaw(b.out_buf, &b.pos, "{\"content\":[")) {
            b.overflowed = true;
        }
        return b;
    }

    /// Appends a `{"type":"text","text":...}` content block with full RFC 8259 JSON escaping.
    fn addText(*_, text: string): bool {
        if (_.overflowed) return false;
        if (_.item_count > 0) {
            if (!writeByte(_.out_buf, &_.pos, ',')) {
                _.overflowed = true;
                return false;
            }
        }
        if (!writeRaw(_.out_buf, &_.pos, "{\"type\":\"text\",\"text\":")) {
            _.overflowed = true;
            return false;
        }
        if (!writeJsonString(_.out_buf, &_.pos, text)) {
            _.overflowed = true;
            return false;
        }
        if (!writeByte(_.out_buf, &_.pos, '}')) {
            _.overflowed = true;
            return false;
        }
        _.item_count += 1;
        return true;
    }

    /// Appends an embedded MCP resource content block `{"type":"resource","resource":{...}}`.
    fn addResourceText(*_, uri: string, mime_type: string, text: string): bool {
        if (_.overflowed) return false;
        if (_.item_count > 0) {
            if (!writeByte(_.out_buf, &_.pos, ',')) {
                _.overflowed = true;
                return false;
            }
        }
        if (!writeRaw(_.out_buf, &_.pos, "{\"type\":\"resource\",\"resource\":{\"uri\":")) {
            _.overflowed = true;
            return false;
        }
        if (!writeJsonString(_.out_buf, &_.pos, uri)) {
            _.overflowed = true;
            return false;
        }
        if (!writeRaw(_.out_buf, &_.pos, ",\"mimeType\":")) {
            _.overflowed = true;
            return false;
        }
        if (!writeJsonString(_.out_buf, &_.pos, mime_type)) {
            _.overflowed = true;
            return false;
        }
        if (!writeRaw(_.out_buf, &_.pos, ",\"text\":")) {
            _.overflowed = true;
            return false;
        }
        if (!writeJsonString(_.out_buf, &_.pos, text)) {
            _.overflowed = true;
            return false;
        }
        if (!writeRaw(_.out_buf, &_.pos, "}}")) {
            _.overflowed = true;
            return false;
        }
        _.item_count += 1;
        return true;
    }

    /// Finalizes the JSON payload with `],"isError":true|false}` and returns total bytes written,
    /// or `null` if the caller buffer overflowed at any point.
    fn finish(*_): usize? {
        if (_.overflowed) return null;
        const suffix: string = if (_.is_error) "],\"isError\":true}" else "],\"isError\":false}";
        if (!writeRaw(_.out_buf, &_.pos, suffix)) {
            _.overflowed = true;
            return null;
        }
        return _.pos;
    }

    /// One-shot convenience helper building a single-text-block MCP `CallToolResult`.
    static fn buildTextResult(text: string, is_error: bool, out_buf: *[]u8): usize? {
        let b = McpToolResultBuilder.init(out_buf, is_error);
        if (!b.addText(text)) return null;
        return b.finish();
    }

    /// One-shot convenience helper building an `isError: true` MCP `CallToolResult`.
    static fn buildErrorResult(message: string, out_buf: *[]u8): usize? {
        return McpToolResultBuilder.buildTextResult(message, true, out_buf);
    }
}
