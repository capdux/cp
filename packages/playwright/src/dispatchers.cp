// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/dispatchers/dispatcher.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/dispatchers.cp — Playwright Cross-Language JSON-RPC Dispatcher Layer
// Absorbed from _ref/playwright/packages/playwright-core/src/server/dispatchers/
// (dispatcher.ts, browserTypeDispatcher.ts, browserDispatcher.ts,
//  browserContextDispatcher.ts, pageDispatcher.ts, frameDispatcher.ts).
//
// Features:
//   - Full RpcObjectKind hierarchy (14 Playwright protocol object types)
//   - DispatcherScope: GUID object registry with parent-child cascading RAII
//     dispose/deinit and zero-leak ownership tracking
//   - RpcDispatcher: standard Playwright JSON-RPC dispatch(alloc, guid, method, params_json)
//     using = undefined stack buffers, ArenaAllocator, and 16-lane SIMD JSON scanning
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonField } from "cap:json";
import { CdpSendTransportFn, CdpConnectionState } from "cap:cdp";
import { PlaywrightError, Route, Request, Response } from "./network";
import { Browser } from "./browser";
import { BrowserContext, BrowserContextOptions, Cookie } from "./context";
import { ChromiumBrowserType, LaunchOptions, ConnectOverCdpOptions } from "./chromium";
import { Page, GotoOptions, ScreenshotOptions, PdfOptions } from "./page";
import { Frame } from "./frames";
import { Locator } from "./locator";
import { Clock } from "./clock";
import { FileChooser } from "./artifacts";
import { Tracing, TracingOptions } from "./tracing";
import { APIRequestContext } from "./request_context";

type DispatchVec16 = @Vector(16, u8);

/// 16-lane SIMD scanner: finds the next '"' or '\\' byte starting from `start`.
pub fn simdScanJsonStringEnd(data: []const u8, start: usize): usize? {
    let i: usize = start;
    const v_quote: DispatchVec16 = @splat('"');
    const v_bslash: DispatchVec16 = @splat('\\');
    while (i + 16 <= data.len) : (i += 16) {
        const chunk: DispatchVec16 = data[i..][0..16].*;
        const mask = (chunk == v_quote) | (chunk == v_bslash);
        if (@reduce(.Or, mask)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = data[i + k];
                if (b == '"' || b == '\\') return i + k;
            }
        }
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '"' || b == '\\') return i;
    }
    return null;
}

/// 16-lane SIMD scanner: finds the next structural JSON delimiter (`"`, `{`, `}`, `[`, `]`, `,`, `:`).
pub fn simdFindJsonStructural(data: []const u8, start: usize): usize? {
    let i: usize = start;
    const v_quote: DispatchVec16 = @splat('"');
    const v_lbrace: DispatchVec16 = @splat('{');
    const v_rbrace: DispatchVec16 = @splat('}');
    const v_lbrack: DispatchVec16 = @splat('[');
    const v_rbrack: DispatchVec16 = @splat(']');
    const v_comma: DispatchVec16 = @splat(',');
    const v_colon: DispatchVec16 = @splat(':');

    while (i + 16 <= data.len) : (i += 16) {
        const chunk: DispatchVec16 = data[i..][0..16].*;
        const mask =
            (chunk == v_quote) |
            (chunk == v_lbrace) |
            (chunk == v_rbrace) |
            (chunk == v_lbrack) |
            (chunk == v_rbrack) |
            (chunk == v_comma) |
            (chunk == v_colon);
        if (@reduce(.Or, mask)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = data[i + k];
                if (b == '"' || b == '{' || b == '}' || b == '[' || b == ']' || b == ',' || b == ':') {
                    return i + k;
                }
            }
        }
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '"' || b == '{' || b == '}' || b == '[' || b == ']' || b == ',' || b == ':') {
            return i;
        }
    }
    return null;
}

fn skipJsonWs(data: []const u8, start: usize): usize {
    let i = start;
    while (i < data.len) {
        const b = data[i];
        if (b == ' ' || b == '\t' || b == '\r' || b == '\n') {
            i += 1;
        } else {
            break;
        }
    }
    return i;
}

fn scanJsonStringClosingQuote(data: []const u8, open_quote: usize): usize? {
    let pos = open_quote + 1;
    while (pos < data.len) {
        if (hit := simdScanJsonStringEnd(data, pos)) {
            if (data[hit] == '\\') {
                pos = hit + 2;
                continue;
            }
            return hit;
        }
        return null;
    }
    return null;
}

fn skipJsonValue(data: []const u8, start: usize): usize? {
    const pos = skipJsonWs(data, start);
    if (pos >= data.len) return null;
    const ch = data[pos];
    if (ch == '"') {
        const close_q = scanJsonStringClosingQuote(data, pos) orelse return null;
        return close_q + 1;
    }
    if (ch == '{' || ch == '[') {
        let depth: usize = 0;
        let cur = pos;
        while (cur < data.len) {
            const hit = simdFindJsonStructural(data, cur) orelse return null;
            const b = data[hit];
            if (b == '"') {
                const close_q = scanJsonStringClosingQuote(data, hit) orelse return null;
                cur = close_q + 1;
                continue;
            }
            if (b == '{' || b == '[') {
                depth += 1;
            } else if (b == '}' || b == ']') {
                if (depth == 0) return null;
                depth -= 1;
                if (depth == 0) return hit + 1;
            }
            cur = hit + 1;
        }
        return null;
    }
    // Primitive (number, true, false, null)
    let cur = pos;
    while (cur < data.len) {
        const b = data[cur];
        if (b == ',' || b == '}' || b == ']' || b == ' ' || b == '\t' || b == '\r' || b == '\n') {
            break;
        }
        cur += 1;
    }
    return cur;
}

/// 16-lane SIMD top-level JSON object field extractor.
/// When `unquote` is true and the value is a JSON string, surrounding quotes are stripped.
pub fn simdExtractJsonField(json: []const u8, key: []const u8, unquote: bool): ([]const u8)? {
    let pos = skipJsonWs(json, 0);
    if (pos >= json.len || json[pos] != '{') return null;
    pos += 1;

    while (pos < json.len) {
        pos = skipJsonWs(json, pos);
        if (pos >= json.len || json[pos] == '}') return null;
        if (json[pos] == ',') {
            pos += 1;
            continue;
        }
        if (json[pos] != '"') return null;
        const key_end = scanJsonStringClosingQuote(json, pos) orelse return null;
        const found_key = json[(pos + 1)..key_end];

        pos = skipJsonWs(json, key_end + 1);
        if (pos >= json.len || json[pos] != ':') return null;
        pos = skipJsonWs(json, pos + 1);
        if (pos >= json.len) return null;

        const val_start = pos;
        const val_end = skipJsonValue(json, val_start) orelse return null;
        if (std.mem.eql(u8, found_key, key)) {
            if (unquote && val_end >= val_start + 2 && json[val_start] == '"' && json[val_end - 1] == '"') {
                return json[(val_start + 1)..(val_end - 1)];
            }
            return json[val_start..val_end];
        }
        pos = val_end;
    }
    return null;
}

/// Extract an integer field from a top-level JSON object using 16-lane SIMD scanning.
pub fn simdExtractJsonInt(json: []const u8, key: []const u8): i64? {
    const raw = simdExtractJsonField(json, key, true) orelse return null;
    if (raw.len == 0) return null;
    return std.fmt.parseInt(i64, raw, 10) catch null;
}

/// Extract a boolean field from a top-level JSON object using 16-lane SIMD scanning.
pub fn simdExtractJsonBool(json: []const u8, key: []const u8): bool? {
    const raw = simdExtractJsonField(json, key, false) orelse return null;
    if (std.mem.eql(u8, raw, "true")) return true;
    if (std.mem.eql(u8, raw, "false")) return false;
    return null;
}

/// Parse a flat JSON string array (e.g. `["/a.txt", "/b.png"]`) into `out_slices` (up to cap).
fn parseJsonStringArray(arr_json: []const u8, out_slices: [][]const u8): usize {
    let pos = skipJsonWs(arr_json, 0);
    if (pos >= arr_json.len || arr_json[pos] != '[') return 0;
    pos += 1;
    let count: usize = 0;
    while (pos < arr_json.len && count < out_slices.len) {
        pos = skipJsonWs(arr_json, pos);
        if (pos >= arr_json.len || arr_json[pos] == ']') break;
        if (arr_json[pos] == ',') {
            pos += 1;
            continue;
        }
        if (arr_json[pos] == '"') {
            const close_q = scanJsonStringClosingQuote(arr_json, pos) orelse break;
            out_slices[count] = arr_json[(pos + 1)..close_q];
            count += 1;
            pos = close_q + 1;
        } else {
            const next_pos = skipJsonValue(arr_json, pos) orelse break;
            pos = next_pos;
        }
    }
    return count;
}

// ============================================================================
// RpcObjectKind & Standard Protocol Metadata
// ============================================================================

pub enum RpcObjectKind {
    Playwright,
    BrowserType,
    Browser,
    BrowserContext,
    Page,
    Frame,
    Locator,
    Route,
    Request,
    Response,
    FileChooser,
    Tracing,
    Clock,
    APIRequestContext,
}

pub fn rpcObjectKindName(kind: RpcObjectKind): []const u8 {
    return switch (kind) {
        RpcObjectKind.Playwright => "Playwright",
        RpcObjectKind.BrowserType => "BrowserType",
        RpcObjectKind.Browser => "Browser",
        RpcObjectKind.BrowserContext => "BrowserContext",
        RpcObjectKind.Page => "Page",
        RpcObjectKind.Frame => "Frame",
        RpcObjectKind.Locator => "Locator",
        RpcObjectKind.Route => "Route",
        RpcObjectKind.Request => "Request",
        RpcObjectKind.Response => "Response",
        RpcObjectKind.FileChooser => "FileChooser",
        RpcObjectKind.Tracing => "Tracing",
        RpcObjectKind.Clock => "Clock",
        RpcObjectKind.APIRequestContext => "APIRequestContext",
    };
}

pub enum RpcErrorCode {
    InvalidRequest,
    MethodNotFound,
    InvalidParams,
    ObjectNotFound,
    ObjectDisposed,
    NotConnected,
    InternalError,
}

pub fn rpcErrorCodeInt(code: RpcErrorCode): i64 {
    return switch (code) {
        RpcErrorCode.InvalidRequest => -32600,
        RpcErrorCode.MethodNotFound => -32601,
        RpcErrorCode.InvalidParams => -32602,
        RpcErrorCode.ObjectNotFound => -32001,
        RpcErrorCode.ObjectDisposed => -32002,
        RpcErrorCode.NotConnected => -32003,
        RpcErrorCode.InternalError => -32000,
    };
}

pub fn rpcErrorCodeName(code: RpcErrorCode): []const u8 {
    return switch (code) {
        RpcErrorCode.InvalidRequest => "InvalidRequestError",
        RpcErrorCode.MethodNotFound => "MethodNotFoundError",
        RpcErrorCode.InvalidParams => "InvalidParamsError",
        RpcErrorCode.ObjectNotFound => "TargetClosedError",
        RpcErrorCode.ObjectDisposed => "ObjectDisposedError",
        RpcErrorCode.NotConnected => "NotConnectedError",
        RpcErrorCode.InternalError => "InternalError",
    };
}

// ============================================================================
// DispatcherScope — GUID Object Registry & Cascading Parent-Child RAII Tree
// ============================================================================

pub struct DispatcherNode {
    pub guid: []const u8 = "",
    pub kind: RpcObjectKind = RpcObjectKind.Playwright,
    pub parent_guid: []const u8 = "",
    pub object_ptr: usize = 0,
    pub owns_object: bool = false,
    pub disposed: bool = false,
    pub child_guids: std.ArrayList([]const u8) = .empty,

    pub static fn init(
        guid: []const u8,
        kind: RpcObjectKind,
        parent_guid: []const u8,
        object_ptr: usize,
        owns_object: bool,
    ): DispatcherNode {
        return DispatcherNode{
            guid: guid,
            kind: kind,
            parent_guid: parent_guid,
            object_ptr: object_ptr,
            owns_object: owns_object,
            disposed: false,
            child_guids: .empty,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.child_guids.deinit(alloc);
        _.child_guids = .empty;
        _.disposed = true;
    }
}

pub struct DispatcherScope {
    allocator: std.mem.Allocator = undefined,
    nodes: std.ArrayList(*DispatcherNode) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    next_seq: u64 = 1,
    pub disposed_count: usize = 0,

    pub static fn init(alloc: std.mem.Allocator): DispatcherScope {
        return DispatcherScope{
            allocator: alloc,
            nodes: .empty,
            owned_strings: .empty,
            next_seq: 1,
            disposed_count: 0,
        };
    }

    pub fn deinit(*_) {
        // Dispose all top-level nodes first (which cascades to children), then any stragglers.
        while (_.nodes.items.len > 0) {
            // Find a root-most node (parent not present in scope) so parent teardown cascades cleanly.
            let target_idx: usize = 0;
            let i: usize = 0;
            while (i < _.nodes.items.len) : (i += 1) {
                const candidate = _.nodes.items[i];
                if (candidate.parent_guid.len == 0 || _.findNodeIndex(candidate.parent_guid) == null) {
                    target_idx = i;
                    break;
                }
            }
            const target_guid = _.nodes.items[target_idx].guid;
            * = _.dispose(target_guid);
        }
        _.nodes.deinit(_.allocator);
        _.nodes = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        return duped;
    }

    /// Format a fresh unique GUID into caller-provided `= undefined` stack buffer.
    pub fn nextGuid(*_, prefix: []const u8, out_buf: []u8): ([]const u8)!PlaywrightError {
        const seq = _.next_seq;
        _.next_seq += 1;
        return std.fmt.bufPrint(out_buf, "{s}@{d}", .{ prefix, seq }) catch return PlaywrightError.BufferTooSmall;
    }

    fn findNodeIndex(guid: []const u8): usize? {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.nodes.items[i].guid, guid)) {
                return i;
            }
        }
        return null;
    }

    pub fn get(guid: []const u8): (*DispatcherNode)? {
        if (idx := _.findNodeIndex(guid)) {
            return _.nodes.items[idx];
        }
        return null;
    }

    pub fn contains(guid: []const u8): bool {
        return _.findNodeIndex(guid) != null;
    }

    pub fn count(): usize {
        return _.nodes.items.len;
    }

    pub fn childCount(guid: []const u8): usize {
        if (node := _.get(guid)) {
            return node.child_guids.items.len;
        }
        return 0;
    }

    /// Register a new object node under `parent_guid` (empty string for root).
    pub fn register(
        *_,
        guid: []const u8,
        kind: RpcObjectKind,
        parent_guid: []const u8,
        object_ptr: usize,
        owns_object: bool,
    ): (*DispatcherNode)!PlaywrightError {
        if (_.contains(guid)) return PlaywrightError.ProtocolError;

        const interned_guid = _.intern(guid)?;
        const interned_parent = _.intern(parent_guid)?;

        let node = _.allocator.create(DispatcherNode) catch return PlaywrightError.OutOfMemory;
        node.* = DispatcherNode.init(interned_guid, kind, interned_parent, object_ptr, owns_object);

        _.nodes.append(_.allocator, node) catch {
            node.deinit(_.allocator);
            _.allocator.destroy(node);
            return PlaywrightError.OutOfMemory;
        };

        if (interned_parent.len > 0) {
            if (parent := _.get(interned_parent)) {
                parent.child_guids.append(_.allocator, interned_guid) catch {
                    * = _.nodes.pop();
                    node.deinit(_.allocator);
                    _.allocator.destroy(node);
                    return PlaywrightError.OutOfMemory;
                };
            }
        }
        return node;
    }

    /// Reparent an existing child node to `new_parent_guid` (mirrors TS `Dispatcher.adopt`).
    pub fn adopt(*_, new_parent_guid: []const u8, child_guid: []const u8): bool {
        const child = _.get(child_guid) orelse return false;
        const new_parent = _.get(new_parent_guid) orelse return false;
        if (std.mem.eql(u8, child.parent_guid, new_parent_guid)) return true;

        if (child.parent_guid.len > 0) {
            if (old_parent := _.get(child.parent_guid)) {
                removeChildGuid(old_parent, child.guid);
            }
        }
        new_parent.child_guids.append(_.allocator, child.guid) catch return false;
        child.parent_guid = new_parent.guid;
        return true;
    }

    fn releaseNodeObject(*_, node: *DispatcherNode) {
        if (node.object_ptr == 0) return;

        if (node.owns_object) {
            switch (node.kind) {
                RpcObjectKind.Browser => {
                    const b = unsafe { node.object_ptr as *Browser! };
                    b.deinit();
                    _.allocator.destroy(b);
                },
                RpcObjectKind.Clock => {
                    const c = unsafe { node.object_ptr as *Clock! };
                    c.uninstall();
                    _.allocator.destroy(c);
                },
                RpcObjectKind.FileChooser => {
                    const fc = unsafe { node.object_ptr as *FileChooser! };
                    fc.deinit();
                    _.allocator.destroy(fc);
                },
                RpcObjectKind.Locator => {
                    const loc = unsafe { node.object_ptr as *Locator! };
                    _.allocator.destroy(loc);
                },
                RpcObjectKind.APIRequestContext => {
                    const api = unsafe { node.object_ptr as *APIRequestContext! };
                    api.deinit();
                    _.allocator.destroy(api);
                },
                RpcObjectKind.Route => {
                    const r = unsafe { node.object_ptr as *Route! };
                    _.allocator.destroy(r);
                },
                RpcObjectKind.Request => {
                    const req = unsafe { node.object_ptr as *Request! };
                    _.allocator.destroy(req);
                },
                RpcObjectKind.Response => {
                    const resp = unsafe { node.object_ptr as *Response! };
                    _.allocator.destroy(resp);
                },
                else => {},
            }
        } else {
            // Non-owning handles: mark closed if appropriate, parent owns heap memory.
            switch (node.kind) {
                RpcObjectKind.BrowserContext => {
                    const ctx = unsafe { node.object_ptr as *BrowserContext! };
                    ctx.close();
                },
                RpcObjectKind.Page => {
                    const page = unsafe { node.object_ptr as *Page! };
                    page.close();
                },
                else => {},
            }
        }
        node.object_ptr = 0;
    }

    /// Recursively dispose `guid` and all descendant nodes in post-order (children first).
    /// Returns the total number of disposed nodes.
    pub fn dispose(*_, guid: []const u8): usize {
        const idx = _.findNodeIndex(guid) orelse return 0;
        let node = _.nodes.items[idx];
        if (node.disposed) return 0;
        node.disposed = true;

        let total: usize = 0;

        // Post-order cascade: dispose all children while they are still attached.
        while (node.child_guids.items.len > 0) {
            const child_guid = node.child_guids.pop() orelse break;
            total += _.dispose(child_guid);
        }

        // Detach from parent's child list if parent still exists.
        if (node.parent_guid.len > 0) {
            if (parent := _.get(node.parent_guid)) {
                removeChildGuid(parent, node.guid);
            }
        }

        // Release underlying native object (zero-leak, double-free safe).
        _.releaseNodeObject(node);

        // Remove node from registry and destroy node shell.
        if (cur_idx := _.findNodeIndex(node.guid)) {
            * = _.nodes.orderedRemove(cur_idx);
        }
        node.deinit(_.allocator);
        _.allocator.destroy(node);

        total += 1;
        _.disposed_count += 1;
        return total;
    }
}

fn removeChildGuid(parent: *DispatcherNode, target_guid: []const u8) {
    let i: usize = parent.child_guids.items.len;
    while (i > 0) {
        i -= 1;
        if (std.mem.eql(u8, parent.child_guids.items[i], target_guid)) {
            * = parent.child_guids.orderedRemove(i);
            return;
        }
    }
}

fn rpcDefaultRouteHandler(ctx: usize, r: *Route) {
    * = ctx;
    r.continueRequest() catch {};
}

// ============================================================================
// RpcDispatcher — Standard Playwright JSON-RPC Request Router
// ============================================================================

pub struct RpcDispatcher {
    allocator: std.mem.Allocator = undefined,
    pub scope: DispatcherScope = undefined,
    pub root_guid: []const u8 = "playwright@root",
    pub browser_type_guid: []const u8 = "browser-type@chromium",
    transport_fn: CdpSendTransportFn? = null,
    transport_ctx: usize = 0,
    pub request_count: u64 = 0,

    pub static fn init(alloc: std.mem.Allocator): RpcDispatcher!PlaywrightError {
        let d = RpcDispatcher{
            allocator: alloc,
            scope: DispatcherScope.init(alloc),
            root_guid: "playwright@root",
            browser_type_guid: "browser-type@chromium",
            transport_fn: null,
            transport_ctx: 0,
            request_count: 0,
        };
        errdefer d.deinit();

        * = d.scope.register("playwright@root", RpcObjectKind.Playwright, "", 0, false)?;
        * = d.scope.register("browser-type@chromium", RpcObjectKind.BrowserType, "playwright@root", 0, false)?;
        return d;
    }

    pub fn deinit(*_) {
        _.scope.deinit();
    }

    ~init() {
        _.deinit();
    }

    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize) {
        _.transport_fn = cb;
        _.transport_ctx = ctx;
    }

    /// Attach a `Clock` object to a `Page` GUID so remote clients can call Clock RPC methods.
    pub fn registerClockForPage(*_, page_guid: []const u8): ([]const u8)!PlaywrightError {
        const page_node = _.scope.get(page_guid) orelse return PlaywrightError.TargetNotFound;
        if (page_node.kind != RpcObjectKind.Page || page_node.object_ptr == 0) {
            return PlaywrightError.ProtocolError;
        }
        const page = unsafe { page_node.object_ptr as *Page! };
        let clk = _.allocator.create(Clock) catch return PlaywrightError.OutOfMemory;
        clk.* = page.clock();

        let guid_buf: [64]u8 = undefined;
        const g = _.scope.nextGuid("clock", &guid_buf)?;
        const node = _.scope.register(g, RpcObjectKind.Clock, page_node.guid, unsafe { @intFromPtr(clk) }, true) catch |e| {
            _.allocator.destroy(clk);
            return e;
        };
        return node.guid;
    }

    /// Attach a `FileChooser` object to a `Page` GUID so remote clients can call `setFiles`.
    pub fn registerFileChooserForPage(
        *_,
        page_guid: []const u8,
        selector: []const u8,
        is_multiple: bool,
    ): ([]const u8)!PlaywrightError {
        const page_node = _.scope.get(page_guid) orelse return PlaywrightError.TargetNotFound;
        if (page_node.kind != RpcObjectKind.Page || page_node.object_ptr == 0) {
            return PlaywrightError.ProtocolError;
        }
        const page = unsafe { page_node.object_ptr as *Page! };
        const interned_sel = _.scope.intern(selector)?;

        let fc = _.allocator.create(FileChooser) catch return PlaywrightError.OutOfMemory;
        fc.* = FileChooser.init(_.allocator, interned_sel, is_multiple, page.endpoint, page.session_id);

        let guid_buf: [64]u8 = undefined;
        const g = _.scope.nextGuid("file-chooser", &guid_buf)?;
        const node = _.scope.register(g, RpcObjectKind.FileChooser, page_node.guid, unsafe { @intFromPtr(fc) }, true) catch |e| {
            fc.deinit();
            _.allocator.destroy(fc);
            return e;
        };
        return node.guid;
    }

    // ------------------------------------------------------------------------
    // JSON-RPC Envelope Helpers (ArenaAllocator + cap:json)
    // ------------------------------------------------------------------------

    fn formatErrorJson(
        alloc: std.mem.Allocator,
        code: RpcErrorCode,
        message: []const u8,
    ): ([]u8)!PlaywrightError {
        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        let err_fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
        err_fields[0] = JsonField{ key: "code", value: JsonValue{ tag: .Number, num_val: @floatFromInt(rpcErrorCodeInt(code)) } };
        err_fields[1] = JsonField{ key: "name", value: JsonValue{ tag: .String, str_val: rpcErrorCodeName(code) } };
        err_fields[2] = JsonField{ key: "message", value: JsonValue{ tag: .String, str_val: message } };

        let root_fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
        root_fields[0] = JsonField{ key: "error", value: JsonValue{ tag: .Object, obj_val: err_fields } };

        return JSON.stringifyValue(alloc, JsonValue{ tag: .Object, obj_val: root_fields }) catch return PlaywrightError.OutOfMemory;
    }

    fn formatPlaywrightError(alloc: std.mem.Allocator, err: PlaywrightError): ([]u8)!PlaywrightError {
        const code = switch (err) {
            PlaywrightError.NotConnected, PlaywrightError.ConnectionClosed => RpcErrorCode.NotConnected,
            PlaywrightError.TargetNotFound, PlaywrightError.PageClosed, PlaywrightError.ContextClosed, PlaywrightError.BrowserClosed => RpcErrorCode.ObjectDisposed,
            PlaywrightError.InvalidUrl, PlaywrightError.InvalidSelector => RpcErrorCode.InvalidParams,
            else => RpcErrorCode.InternalError,
        };
        let msg_buf: [128]u8 = undefined;
        const msg = std.fmt.bufPrint(&msg_buf, "PlaywrightError.{s}", .{@errorName(err)}) catch "PlaywrightError";
        return formatErrorJson(alloc, code, msg);
    }

    fn formatOkResult(alloc: std.mem.Allocator, result_val: JsonValue): ([]u8)!PlaywrightError {
        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        let root_fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
        root_fields[0] = JsonField{ key: "result", value: result_val };
        return JSON.stringifyValue(alloc, JsonValue{ tag: .Object, obj_val: root_fields }) catch return PlaywrightError.OutOfMemory;
    }

    fn makeGuidRef(aa: std.mem.Allocator, guid: []const u8): JsonValue!PlaywrightError {
        let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
        fields[0] = JsonField{ key: "guid", value: JsonValue{ tag: .String, str_val: guid } };
        return JsonValue{ tag: .Object, obj_val: fields };
    }

    // ------------------------------------------------------------------------
    // Public Entry Points
    // ------------------------------------------------------------------------

    /// Standard Playwright JSON-RPC request dispatch.
    /// `params_json` may be empty or a JSON object string (e.g. `{"url":"https://example.com"}`).
    /// Caller owns the returned JSON byte slice and must free it with `alloc.free(...)`.
    pub fn dispatch(
        *_,
        alloc: std.mem.Allocator,
        guid: []const u8,
        method: []const u8,
        params_json: []const u8,
    ): ([]u8)!PlaywrightError {
        _.request_count += 1;

        if (method.len == 0) {
            return formatErrorJson(alloc, RpcErrorCode.InvalidRequest, "Missing RPC method");
        }

        const effective_guid = if (guid.len == 0) _.root_guid else guid;
        const node = _.scope.get(effective_guid) orelse {
            let msg_buf: [160]u8 = undefined;
            const msg = std.fmt.bufPrint(&msg_buf, "Object with guid '{s}' not found", .{effective_guid}) catch "Object not found";
            return formatErrorJson(alloc, RpcErrorCode.ObjectNotFound, msg);
        };

        if (node.disposed) {
            return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "Target object has been disposed");
        }

        const params = if (params_json.len > 0) params_json else "{}";

        return switch (node.kind) {
            RpcObjectKind.Playwright => _.dispatchPlaywright(alloc, node, method, params),
            RpcObjectKind.BrowserType => _.dispatchBrowserType(alloc, node, method, params),
            RpcObjectKind.Browser => _.dispatchBrowser(alloc, node, method, params),
            RpcObjectKind.BrowserContext => _.dispatchBrowserContext(alloc, node, method, params),
            RpcObjectKind.Page => _.dispatchPage(alloc, node, method, params),
            RpcObjectKind.Frame => _.dispatchFrame(alloc, node, method, params),
            RpcObjectKind.Clock => _.dispatchClock(alloc, node, method, params),
            RpcObjectKind.FileChooser => _.dispatchFileChooser(alloc, node, method, params),
            RpcObjectKind.Tracing => _.dispatchTracing(alloc, node, method, params),
            RpcObjectKind.Locator => _.dispatchLocator(alloc, node, method, params),
            else => {
                let msg_buf: [160]u8 = undefined;
                const msg = std.fmt.bufPrint(
                    &msg_buf,
                    "Dispatcher '{s}' does not implement method '{s}'",
                    .{ rpcObjectKindName(node.kind), method },
                ) catch "Method not implemented";
                return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, msg);
            },
        };
    }

    /// Dispatch a full Playwright wire JSON-RPC frame:
    /// `{"id":1,"guid":"browser-type@chromium","method":"launch","params":{"headless":true}}`
    pub fn dispatchRpcFrame(
        *_,
        alloc: std.mem.Allocator,
        frame_json: []const u8,
    ): ([]u8)!PlaywrightError {
        const id_raw = simdExtractJsonField(frame_json, "id", false) orelse "1";
        const guid = simdExtractJsonField(frame_json, "guid", true) orelse "";
        const method = simdExtractJsonField(frame_json, "method", true) orelse {
            return formatErrorJson(alloc, RpcErrorCode.InvalidRequest, "Missing 'method' field in RPC frame");
        };
        const params = simdExtractJsonField(frame_json, "params", false) orelse "{}";

        const body = _.dispatch(alloc, guid, method, params)?;
        defer alloc.free(body);

        // Splice `"id":<id_raw>,` at the front of the JSON object envelope.
        if (body.len >= 2 && body[0] == '{') {
            return std.fmt.allocPrint(alloc, "{{\"id\":{s},{s}", .{ id_raw, body[1..] }) catch return PlaywrightError.OutOfMemory;
        }
        return alloc.dupe(u8, body) catch return PlaywrightError.OutOfMemory;
    }

    // ------------------------------------------------------------------------
    // Playwright Root Dispatcher
    // ------------------------------------------------------------------------

    fn dispatchPlaywright(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        * = params;
        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "initialize")) {
            let fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "playwright", value: makeGuidRef(aa, node.guid)? };
            fields[1] = JsonField{ key: "chromium", value: makeGuidRef(aa, _.browser_type_guid)? };
            fields[2] = JsonField{ key: "version", value: JsonValue{ tag: .String, str_val: "playwright-cp/1.66.0" } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown Playwright method");
    }

    // ------------------------------------------------------------------------
    // BrowserType Dispatcher: launch, connectOverCDP
    // ------------------------------------------------------------------------

    fn dispatchBrowserType(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "launch")) {
            const headless = simdExtractJsonBool(params, "headless") orelse true;
            const exe_path = simdExtractJsonField(params, "executablePath", true) orelse "";
            const timeout_i = simdExtractJsonInt(params, "timeout") orelse 30000;
            const timeout_ms: u32 = if (timeout_i > 0) (timeout_i as u32) else 30000;

            let bp = _.allocator.create(Browser) catch return PlaywrightError.OutOfMemory;
            bp.* = ChromiumBrowserType.launch(_.allocator, LaunchOptions{
                headless: headless,
                executable_path: exe_path,
                timeout_ms: timeout_ms,
                transport_fn: _.transport_fn,
                transport_ctx: _.transport_ctx,
            }) catch |e| {
                _.allocator.destroy(bp);
                return formatPlaywrightError(alloc, e);
            };

            let guid_buf: [64]u8 = undefined;
            const bg = _.scope.nextGuid("browser", &guid_buf)?;
            const bnode = _.scope.register(bg, RpcObjectKind.Browser, node.guid, unsafe { @intFromPtr(bp) }, true) catch |e| {
                bp.deinit();
                _.allocator.destroy(bp);
                return formatPlaywrightError(alloc, e);
            };

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "browser", value: makeGuidRef(aa, bnode.guid)? };
            fields[1] = JsonField{ key: "version", value: JsonValue{ tag: .String, str_val: bp.version() } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "connectOverCDP")) {
            const endpoint_url =
                simdExtractJsonField(params, "endpointURL", true) orelse
                simdExtractJsonField(params, "url", true) orelse {
                    return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "connectOverCDP requires 'endpointURL'");
                };
            const timeout_i = simdExtractJsonInt(params, "timeout") orelse 30000;
            const timeout_ms: u32 = if (timeout_i > 0) (timeout_i as u32) else 30000;

            let bp = _.allocator.create(Browser) catch return PlaywrightError.OutOfMemory;
            bp.* = ChromiumBrowserType.connectOverCDP(_.allocator, endpoint_url, ConnectOverCdpOptions{
                timeout_ms: timeout_ms,
                transport_fn: _.transport_fn,
                transport_ctx: _.transport_ctx,
            }) catch |e| {
                _.allocator.destroy(bp);
                return formatPlaywrightError(alloc, e);
            };

            let guid_buf: [64]u8 = undefined;
            const bg = _.scope.nextGuid("browser", &guid_buf)?;
            const bnode = _.scope.register(bg, RpcObjectKind.Browser, node.guid, unsafe { @intFromPtr(bp) }, true) catch |e| {
                bp.deinit();
                _.allocator.destroy(bp);
                return formatPlaywrightError(alloc, e);
            };

            let fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "browser", value: makeGuidRef(aa, bnode.guid)? };
            fields[1] = JsonField{ key: "wsEndpoint", value: JsonValue{ tag: .String, str_val: bp.ws_endpoint } };
            fields[2] = JsonField{ key: "version", value: JsonValue{ tag: .String, str_val: bp.version() } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown BrowserType method");
    }

    // ------------------------------------------------------------------------
    // Browser Dispatcher: newContext, newPage, close
    // ------------------------------------------------------------------------

    fn registerContextWithTracing(
        *_,
        browser_guid: []const u8,
        ctx: *BrowserContext,
    ): (*DispatcherNode)!PlaywrightError {
        let guid_buf: [64]u8 = undefined;
        const cg = _.scope.nextGuid("browser-context", &guid_buf)?;
        const cnode = _.scope.register(cg, RpcObjectKind.BrowserContext, browser_guid, unsafe { @intFromPtr(ctx) }, false)?;

        let tbuf: [64]u8 = undefined;
        const tg = _.scope.nextGuid("tracing", &tbuf)?;
        * = _.scope.register(tg, RpcObjectKind.Tracing, cnode.guid, unsafe { @intFromPtr(ctx.tracing()) }, false)?;
        return cnode;
    }

    fn registerPageWithMainFrame(
        *_,
        context_guid: []const u8,
        page: *Page,
    ): (*DispatcherNode)!PlaywrightError {
        let guid_buf: [64]u8 = undefined;
        const pg = _.scope.nextGuid("page", &guid_buf)?;
        const pnode = _.scope.register(pg, RpcObjectKind.Page, context_guid, unsafe { @intFromPtr(page) }, false)?;

        const mf = page.mainFrame()?;
        let fbuf: [64]u8 = undefined;
        const fg = _.scope.nextGuid("frame", &fbuf)?;
        * = _.scope.register(fg, RpcObjectKind.Frame, pnode.guid, unsafe { @intFromPtr(mf) }, false)?;
        return pnode;
    }

    fn dispatchBrowser(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "Browser is disposed");
        const browser = unsafe { node.object_ptr as *Browser! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "newContext")) {
            const vw_i = simdExtractJsonInt(params, "viewportWidth") orelse 1280;
            const vh_i = simdExtractJsonInt(params, "viewportHeight") orelse 720;
            const ignore_https = simdExtractJsonBool(params, "ignoreHTTPSErrors") orelse false;
            const ua = simdExtractJsonField(params, "userAgent", true) orelse "";
            const loc = simdExtractJsonField(params, "locale", true) orelse "en-US";

            const ctx = browser.newContext(BrowserContextOptions{
                viewport_width: if (vw_i > 0) (vw_i as u32) else 1280,
                viewport_height: if (vh_i > 0) (vh_i as u32) else 720,
                ignore_https_errors: ignore_https,
                user_agent: ua,
                locale: loc,
                default_timeout_ms: 30000,
            }) catch |e| return formatPlaywrightError(alloc, e);

            const cnode = _.registerContextWithTracing(node.guid, ctx)?;
            const tracing_guid = if (cnode.child_guids.items.len > 0) cnode.child_guids.items[0] else "";

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "context", value: makeGuidRef(aa, cnode.guid)? };
            fields[1] = JsonField{ key: "tracing", value: makeGuidRef(aa, tracing_guid)? };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "newPage")) {
            const ctx = browser.newContext(.{}) catch |e| return formatPlaywrightError(alloc, e);
            const cnode = _.registerContextWithTracing(node.guid, ctx)?;
            const page = ctx.newPage() catch |e| return formatPlaywrightError(alloc, e);
            const pnode = _.registerPageWithMainFrame(cnode.guid, page)?;
            const frame_guid = if (pnode.child_guids.items.len > 0) pnode.child_guids.items[0] else "";

            let fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "page", value: makeGuidRef(aa, pnode.guid)? };
            fields[1] = JsonField{ key: "context", value: makeGuidRef(aa, cnode.guid)? };
            fields[2] = JsonField{ key: "mainFrame", value: makeGuidRef(aa, frame_guid)? };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "close")) {
            browser.close();
            const disposed = _.scope.dispose(node.guid);
            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "closed", value: JsonValue{ tag: .Bool, bool_val: true } };
            fields[1] = JsonField{ key: "disposed", value: JsonValue{ tag: .Number, num_val: @floatFromInt(disposed) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown Browser method");
    }

    // ------------------------------------------------------------------------
    // BrowserContext Dispatcher:
    //   newPage, cookies, addCookies, clearCookies, storageState,
    //   tracingStart, tracingStop, close
    // ------------------------------------------------------------------------

    fn dispatchBrowserContext(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "BrowserContext is disposed");
        const ctx = unsafe { node.object_ptr as *BrowserContext! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "newPage")) {
            const page = ctx.newPage() catch |e| return formatPlaywrightError(alloc, e);
            const pnode = _.registerPageWithMainFrame(node.guid, page)?;
            const frame_guid = if (pnode.child_guids.items.len > 0) pnode.child_guids.items[0] else "";

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "page", value: makeGuidRef(aa, pnode.guid)? };
            fields[1] = JsonField{ key: "mainFrame", value: makeGuidRef(aa, frame_guid)? };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "cookies")) {
            const list = ctx.cookies();
            let items = aa.alloc(JsonValue, list.len) catch return PlaywrightError.OutOfMemory;
            let i: usize = 0;
            while (i < list.len) : (i += 1) {
                const c = list[i];
                let cfields = aa.alloc(JsonField, 4) catch return PlaywrightError.OutOfMemory;
                cfields[0] = JsonField{ key: "name", value: JsonValue{ tag: .String, str_val: c.name } };
                cfields[1] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: c.value } };
                cfields[2] = JsonField{ key: "domain", value: JsonValue{ tag: .String, str_val: c.domain } };
                cfields[3] = JsonField{ key: "path", value: JsonValue{ tag: .String, str_val: c.path } };
                items[i] = JsonValue{ tag: .Object, obj_val: cfields };
            }
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "cookies", value: JsonValue{ tag: .Array, arr_val: items } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "addCookies")) {
            // Support both flat fields {"name":"a","value":"b","domain":"example.com"}
            // and nested single-element {"cookies":[{"name":"a","value":"b",...}]}
            let cookie_obj = params;
            if (arr := simdExtractJsonField(params, "cookies", false)) {
                let pos = skipJsonWs(arr, 0);
                if (pos < arr.len && arr[pos] == '[') {
                    pos = skipJsonWs(arr, pos + 1);
                    if (pos < arr.len && arr[pos] == '{') {
                        if (end_pos := skipJsonValue(arr, pos)) {
                            cookie_obj = arr[pos..end_pos];
                        }
                    }
                }
            }
            const cname = simdExtractJsonField(cookie_obj, "name", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "addCookies requires cookie 'name'");
            };
            const cval = simdExtractJsonField(cookie_obj, "value", true) orelse "";
            const cdom = simdExtractJsonField(cookie_obj, "domain", true) orelse "";
            const cpath = simdExtractJsonField(cookie_obj, "path", true) orelse "/";

            const batch: [1]Cookie = .{
                Cookie{
                    name: cname,
                    value: cval,
                    domain: cdom,
                    path: cpath,
                },
            };
            ctx.addCookies(&batch) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "added", value: JsonValue{ tag: .Number, num_val: 1.0 } };
            fields[1] = JsonField{ key: "total", value: JsonValue{ tag: .Number, num_val: @floatFromInt(ctx.cookies().len) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "clearCookies")) {
            ctx.clearCookies();
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "cleared", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "storageState")) {
            let state = ctx.storageState(aa) catch |e| return formatPlaywrightError(alloc, e);
            defer state.deinit();
            const state_json = state.toJson(aa) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "stateJson", value: JsonValue{ tag: .String, str_val: state_json } };
            fields[1] = JsonField{ key: "cookieCount", value: JsonValue{ tag: .Number, num_val: @floatFromInt(state.cookies.items.len) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "tracingStart")) {
            const tname = simdExtractJsonField(params, "name", true) orelse "trace";
            const ttitle = simdExtractJsonField(params, "title", true) orelse "";
            const screenshots = simdExtractJsonBool(params, "screenshots") orelse false;
            const snapshots = simdExtractJsonBool(params, "snapshots") orelse false;
            ctx.tracing().start(TracingOptions{
                name: tname,
                title: ttitle,
                screenshots: screenshots,
                snapshots: snapshots,
            }) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "recording", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "tracingStop")) {
            const out_path = simdExtractJsonField(params, "path", true) orelse "";
            const zip_bytes = ctx.tracing().stop(out_path) catch |e| return formatPlaywrightError(alloc, e);
            defer _.allocator.free(zip_bytes);

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "recording", value: JsonValue{ tag: .Bool, bool_val: false } };
            fields[1] = JsonField{ key: "bytes", value: JsonValue{ tag: .Number, num_val: @floatFromInt(zip_bytes.len) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "close")) {
            ctx.close();
            const disposed = _.scope.dispose(node.guid);
            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "closed", value: JsonValue{ tag: .Bool, bool_val: true } };
            fields[1] = JsonField{ key: "disposed", value: JsonValue{ tag: .Number, num_val: @floatFromInt(disposed) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown BrowserContext method");
    }

    // ------------------------------------------------------------------------
    // Page Dispatcher:
    //   goto, reload, evaluate, content, title, screenshot, pdf,
    //   click, fill, locator, route, close
    // ------------------------------------------------------------------------

    fn dispatchPage(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "Page is disposed");
        const page = unsafe { node.object_ptr as *Page! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "goto")) {
            const url = simdExtractJsonField(params, "url", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Page.goto requires 'url'");
            };
            const referer = simdExtractJsonField(params, "referer", true) orelse "";
            const resp = page.goto(url, GotoOptions{ referer: referer }) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "url", value: JsonValue{ tag: .String, str_val: resp.url } };
            fields[1] = JsonField{ key: "status", value: JsonValue{ tag: .Number, num_val: @floatFromInt(resp.status) } };
            fields[2] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: resp.ok } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "reload")) {
            const resp = page.reload() catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "url", value: JsonValue{ tag: .String, str_val: resp.url } };
            fields[1] = JsonField{ key: "status", value: JsonValue{ tag: .Number, num_val: @floatFromInt(resp.status) } };
            fields[2] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: resp.ok } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "evaluate")) {
            const expr = simdExtractJsonField(params, "expression", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Page.evaluate requires 'expression'");
            };
            const val = page.evaluate(expr) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: val } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "content")) {
            const html = page.content();
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: html } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "setContent")) {
            const html = simdExtractJsonField(params, "html", true) orelse "";
            page.setContent(html) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "title")) {
            const t = page.title();
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: t } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "screenshot")) {
            const fmt = simdExtractJsonField(params, "type", true) orelse
                simdExtractJsonField(params, "format", true) orelse "png";
            const full_page = simdExtractJsonBool(params, "fullPage") orelse false;
            const q_i = simdExtractJsonInt(params, "quality") orelse 80;
            const res = page.screenshot(ScreenshotOptions{
                format: fmt,
                quality: if (q_i > 0 && q_i <= 100) (q_i as u8) else 80,
                full_page: full_page,
            }) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "binary", value: JsonValue{ tag: .String, str_val: res } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "pdf")) {
            const landscape = simdExtractJsonBool(params, "landscape") orelse false;
            const print_bg = simdExtractJsonBool(params, "printBackground") orelse false;
            const res = page.pdf(PdfOptions{
                landscape: landscape,
                print_background: print_bg,
            }) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "pdf", value: JsonValue{ tag: .String, str_val: res } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "click")) {
            const selector = simdExtractJsonField(params, "selector", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Page.click requires 'selector'");
            };
            page.click(selector) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "fill")) {
            const selector = simdExtractJsonField(params, "selector", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Page.fill requires 'selector'");
            };
            const val = simdExtractJsonField(params, "value", true) orelse "";
            page.fill(selector, val) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "locator")) {
            const selector = simdExtractJsonField(params, "selector", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Page.locator requires 'selector'");
            };
            const interned_sel = _.scope.intern(selector)?;
            let loc_ptr = _.allocator.create(Locator) catch return PlaywrightError.OutOfMemory;
            loc_ptr.* = page.locator(interned_sel);

            let guid_buf: [64]u8 = undefined;
            const lg = _.scope.nextGuid("locator", &guid_buf)?;
            const lnode = _.scope.register(lg, RpcObjectKind.Locator, node.guid, unsafe { @intFromPtr(loc_ptr) }, true) catch |e| {
                _.allocator.destroy(loc_ptr);
                return formatPlaywrightError(alloc, e);
            };

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "locator", value: makeGuidRef(aa, lnode.guid)? };
            fields[1] = JsonField{ key: "selector", value: JsonValue{ tag: .String, str_val: interned_sel } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "route")) {
            const pattern =
                simdExtractJsonField(params, "url", true) orelse
                simdExtractJsonField(params, "pattern", true) orelse {
                    return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Page.route requires 'url' or 'pattern'");
                };
            page.route(pattern, rpcDefaultRouteHandler, 0) catch |e| return formatPlaywrightError(alloc, e);

            let guid_buf: [64]u8 = undefined;
            const rg = _.scope.nextGuid("route", &guid_buf)?;
            const rnode = _.scope.register(rg, RpcObjectKind.Route, node.guid, 0, false)?;

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "route", value: makeGuidRef(aa, rnode.guid)? };
            fields[1] = JsonField{ key: "pattern", value: JsonValue{ tag: .String, str_val: pattern } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "close")) {
            page.close();
            const disposed = _.scope.dispose(node.guid);
            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "closed", value: JsonValue{ tag: .Bool, bool_val: true } };
            fields[1] = JsonField{ key: "disposed", value: JsonValue{ tag: .Number, num_val: @floatFromInt(disposed) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown Page method");
    }

    // ------------------------------------------------------------------------
    // Frame Dispatcher:
    //   goto, reload, evaluate, content, title, screenshot, pdf,
    //   click, fill, locator, route, close
    // ------------------------------------------------------------------------

    fn dispatchFrame(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "Frame is disposed");
        const frame = unsafe { node.object_ptr as *Frame! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "goto")) {
            const url = simdExtractJsonField(params, "url", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Frame.goto requires 'url'");
            };
            const referer = simdExtractJsonField(params, "referer", true) orelse "";
            const resp = frame.goto(url, referer) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "url", value: JsonValue{ tag: .String, str_val: resp.url } };
            fields[1] = JsonField{ key: "status", value: JsonValue{ tag: .Number, num_val: @floatFromInt(resp.status) } };
            fields[2] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: resp.ok } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "evaluate")) {
            const expr = simdExtractJsonField(params, "expression", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Frame.evaluate requires 'expression'");
            };
            const val = frame.evaluate(expr) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: val } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "click")) {
            const selector = simdExtractJsonField(params, "selector", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Frame.click requires 'selector'");
            };
            const loc = frame.locator(selector);
            loc.click() catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "fill")) {
            const selector = simdExtractJsonField(params, "selector", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Frame.fill requires 'selector'");
            };
            const val = simdExtractJsonField(params, "value", true) orelse "";
            const loc = frame.locator(selector);
            loc.fill(val) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "locator")) {
            const selector = simdExtractJsonField(params, "selector", true) orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Frame.locator requires 'selector'");
            };
            const interned_sel = _.scope.intern(selector)?;
            let loc_ptr = _.allocator.create(Locator) catch return PlaywrightError.OutOfMemory;
            loc_ptr.* = frame.locator(interned_sel);

            let guid_buf: [64]u8 = undefined;
            const lg = _.scope.nextGuid("locator", &guid_buf)?;
            const lnode = _.scope.register(lg, RpcObjectKind.Locator, node.guid, unsafe { @intFromPtr(loc_ptr) }, true) catch |e| {
                _.allocator.destroy(loc_ptr);
                return formatPlaywrightError(alloc, e);
            };

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "locator", value: makeGuidRef(aa, lnode.guid)? };
            fields[1] = JsonField{ key: "selector", value: JsonValue{ tag: .String, str_val: interned_sel } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        // Methods that delegate to parent Page when available (reload, content, title, screenshot, pdf, route)
        if (std.mem.eql(u8, method, "reload") ||
            std.mem.eql(u8, method, "content") ||
            std.mem.eql(u8, method, "title") ||
            std.mem.eql(u8, method, "screenshot") ||
            std.mem.eql(u8, method, "pdf") ||
            std.mem.eql(u8, method, "route"))
        {
            if (node.parent_guid.len > 0) {
                if (pnode := _.scope.get(node.parent_guid)) {
                    if (pnode.kind == RpcObjectKind.Page) {
                        return _.dispatchPage(alloc, pnode, method, params);
                    }
                }
            }
            if (std.mem.eql(u8, method, "title")) {
                let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
                fields[0] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: frame.name() } };
                return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
            }
            if (std.mem.eql(u8, method, "content")) {
                let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
                fields[0] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: "" } };
                return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
            }
        }

        if (std.mem.eql(u8, method, "close")) {
            frame.detached = true;
            const disposed = _.scope.dispose(node.guid);
            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "closed", value: JsonValue{ tag: .Bool, bool_val: true } };
            fields[1] = JsonField{ key: "disposed", value: JsonValue{ tag: .Number, num_val: @floatFromInt(disposed) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown Frame method");
    }

    // ------------------------------------------------------------------------
    // Clock Dispatcher: install, fastForward, pauseAt, setFixedTime
    // ------------------------------------------------------------------------

    fn dispatchClock(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "Clock is disposed");
        const clk = unsafe { node.object_ptr as *Clock! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "install")) {
            const time_opt = simdExtractJsonInt(params, "time");
            clk.install(time_opt) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "installed", value: JsonValue{ tag: .Bool, bool_val: clk.installed } };
            fields[1] = JsonField{ key: "now", value: JsonValue{ tag: .Number, num_val: @floatFromInt(clk.now()) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "fastForward")) {
            if (ticks_ms := simdExtractJsonInt(params, "ticksNumber")) {
                clk.fastForward(ticks_ms) catch |e| return formatPlaywrightError(alloc, e);
            } else if (ticks_ms := simdExtractJsonInt(params, "ticks")) {
                clk.fastForward(ticks_ms) catch |e| return formatPlaywrightError(alloc, e);
            } else if (ticks_str := simdExtractJsonField(params, "ticksString", true)) {
                clk.fastForwardStr(ticks_str) catch |e| return formatPlaywrightError(alloc, e);
            } else if (ticks_str := simdExtractJsonField(params, "ticks", true)) {
                clk.fastForwardStr(ticks_str) catch |e| return formatPlaywrightError(alloc, e);
            } else {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Clock.fastForward requires 'ticks' or 'ticksNumber'");
            }
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "now", value: JsonValue{ tag: .Number, num_val: @floatFromInt(clk.now()) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "pauseAt")) {
            const t = simdExtractJsonInt(params, "time") orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Clock.pauseAt requires integer 'time'");
            };
            clk.pauseAt(t) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "paused", value: JsonValue{ tag: .Bool, bool_val: true } };
            fields[1] = JsonField{ key: "now", value: JsonValue{ tag: .Number, num_val: @floatFromInt(clk.now()) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "setFixedTime")) {
            const t = simdExtractJsonInt(params, "time") orelse {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "Clock.setFixedTime requires integer 'time'");
            };
            clk.setFixedTime(t) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "fixed", value: JsonValue{ tag: .Bool, bool_val: true } };
            fields[1] = JsonField{ key: "now", value: JsonValue{ tag: .Number, num_val: @floatFromInt(clk.now()) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown Clock method");
    }

    // ------------------------------------------------------------------------
    // FileChooser Dispatcher: setFiles
    // ------------------------------------------------------------------------

    fn dispatchFileChooser(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "FileChooser is disposed");
        const fc = unsafe { node.object_ptr as *FileChooser! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "setFiles")) {
            let paths_buf: [32][]const u8 = undefined;
            let count: usize = 0;

            if (files_arr := simdExtractJsonField(params, "files", false)) {
                count = parseJsonStringArray(files_arr, &paths_buf);
            } else if (single_file := simdExtractJsonField(params, "file", true)) {
                paths_buf[0] = single_file;
                count = 1;
            } else if (single_path := simdExtractJsonField(params, "path", true)) {
                paths_buf[0] = single_path;
                count = 1;
            } else {
                return formatErrorJson(alloc, RpcErrorCode.InvalidParams, "FileChooser.setFiles requires 'files' array or 'file'");
            }

            fc.setFiles(paths_buf[0..count]) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "count", value: JsonValue{ tag: .Number, num_val: @floatFromInt(fc.files.items.len) } };
            fields[1] = JsonField{ key: "isMultiple", value: JsonValue{ tag: .Bool, bool_val: fc.isMultiple() } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown FileChooser method");
    }

    // ------------------------------------------------------------------------
    // Tracing Dispatcher: start, stop
    // ------------------------------------------------------------------------

    fn dispatchTracing(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "Tracing is disposed");
        const tr = unsafe { node.object_ptr as *Tracing! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "start")) {
            const tname = simdExtractJsonField(params, "name", true) orelse "trace";
            const ttitle = simdExtractJsonField(params, "title", true) orelse "";
            const screenshots = simdExtractJsonBool(params, "screenshots") orelse false;
            const snapshots = simdExtractJsonBool(params, "snapshots") orelse false;
            tr.start(TracingOptions{
                name: tname,
                title: ttitle,
                screenshots: screenshots,
                snapshots: snapshots,
            }) catch |e| return formatPlaywrightError(alloc, e);

            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "recording", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "stop")) {
            const out_path = simdExtractJsonField(params, "path", true) orelse "";
            const zip_bytes = tr.stop(out_path) catch |e| return formatPlaywrightError(alloc, e);
            defer _.allocator.free(zip_bytes);

            let fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "recording", value: JsonValue{ tag: .Bool, bool_val: false } };
            fields[1] = JsonField{ key: "bytes", value: JsonValue{ tag: .Number, num_val: @floatFromInt(zip_bytes.len) } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown Tracing method");
    }

    // ------------------------------------------------------------------------
    // Locator Dispatcher: click, fill
    // ------------------------------------------------------------------------

    fn dispatchLocator(
        *_,
        alloc: std.mem.Allocator,
        node: *DispatcherNode,
        method: []const u8,
        params: []const u8,
    ): ([]u8)!PlaywrightError {
        if (node.object_ptr == 0) return formatErrorJson(alloc, RpcErrorCode.ObjectDisposed, "Locator is disposed");
        const loc = unsafe { node.object_ptr as *Locator! };

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        if (std.mem.eql(u8, method, "click")) {
            loc.click() catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        if (std.mem.eql(u8, method, "fill")) {
            const val = simdExtractJsonField(params, "value", true) orelse "";
            loc.fill(val) catch |e| return formatPlaywrightError(alloc, e);
            let fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "ok", value: JsonValue{ tag: .Bool, bool_val: true } };
            return formatOkResult(alloc, JsonValue{ tag: .Object, obj_val: fields });
        }

        return formatErrorJson(alloc, RpcErrorCode.MethodNotFound, "Unknown Locator method");
    }
}
