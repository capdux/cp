// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/recorder/recorderApp.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/recorder.cp — Interactive Action Recorder & Codegen
// Native CP browser action recording, consecutive fill collapsing, CDP script
// injection (Page.addScriptToEvaluateOnNewDocument + Runtime.evaluate), and
// multi-language (CP / TypeScript / Python) code generation with 16-lane SIMD
// string escape scanning and = undefined stack buffers.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    RuntimeDomain,
    RuntimeEvaluateParams,
} from "cap:cdp";
import { PlaywrightError } from "./network";

type RecorderVec16 = @Vector(16, u8);

/// 16-lane SIMD scanner: returns the index of the first byte that requires
/// escaping in JSON or single/double-quoted codegen string literals
/// ('"', '\'', '\\', or control byte < 0x20), or null if the entire slice is clean.
pub fn simdScanEscapeIndex(data: []const u8): usize? {
    let i: usize = 0;
    const v_dquote: RecorderVec16 = @splat('"');
    const v_squote: RecorderVec16 = @splat('\'');
    const v_bslash: RecorderVec16 = @splat('\\');
    const v_ctrl: RecorderVec16 = @splat(0x20);
    while (i + 16 <= data.len) : (i += 16) {
        const chunk: RecorderVec16 = data[i..][0..16].*;
        const mask = (chunk == v_dquote) | (chunk == v_squote) | (chunk == v_bslash) | (chunk < v_ctrl);
        if (@reduce(.Or, mask)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = data[i + k];
                if (b == '"' || b == '\'' || b == '\\' || b < 0x20) return i + k;
            }
        }
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '"' || b == '\'' || b == '\\' || b < 0x20) return i;
    }
    return null;
}

/// Append `raw` to `out` with escaping for the target `quote_char` ('"' or '\''),
/// using 16-lane SIMD scanning to bulk-copy clean spans without byte-by-byte overhead.
pub fn writeEscapedString(
    out: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    raw: []const u8,
    quote_char: u8,
): void!PlaywrightError {
    let pos: usize = 0;
    while (pos < raw.len) {
        const rest = raw[pos..];
        const hit = simdScanEscapeIndex(rest);
        if (hit == null) {
            out.appendSlice(alloc, rest) catch return PlaywrightError.OutOfMemory;
            return;
        }
        const idx = hit.?;
        if (idx > 0) {
            out.appendSlice(alloc, rest[0..idx]) catch return PlaywrightError.OutOfMemory;
        }
        const b = rest[idx];
        if (b == '\\') {
            out.appendSlice(alloc, "\\\\") catch return PlaywrightError.OutOfMemory;
        } else if (b == '"' && quote_char == '"') {
            out.appendSlice(alloc, "\\\"") catch return PlaywrightError.OutOfMemory;
        } else if (b == '\'' && quote_char == '\'') {
            out.appendSlice(alloc, "\\'") catch return PlaywrightError.OutOfMemory;
        } else if (b == '"' || b == '\'') {
            out.append(alloc, b) catch return PlaywrightError.OutOfMemory;
        } else if (b == '\n') {
            out.appendSlice(alloc, "\\n") catch return PlaywrightError.OutOfMemory;
        } else if (b == '\r') {
            out.appendSlice(alloc, "\\r") catch return PlaywrightError.OutOfMemory;
        } else if (b == '\t') {
            out.appendSlice(alloc, "\\t") catch return PlaywrightError.OutOfMemory;
        } else if (b == 0x08) {
            out.appendSlice(alloc, "\\b") catch return PlaywrightError.OutOfMemory;
        } else if (b == 0x0c) {
            out.appendSlice(alloc, "\\f") catch return PlaywrightError.OutOfMemory;
        } else {
            let hex_buf: [6]u8 = undefined;
            const hex = std.fmt.bufPrint(&hex_buf, "\\u{x:0>4}", .{b}) catch return PlaywrightError.BufferTooSmall;
            out.appendSlice(alloc, hex) catch return PlaywrightError.OutOfMemory;
        }
        pos += idx + 1;
    }
}

/// Escape `raw` into a caller-provided stack buffer for JSON double-quoted strings.
/// Zero heap allocation; uses 16-lane SIMD bulk copy for clean spans.
pub fn escapeJsonIntoBuf(raw: []const u8, buf: []u8): ([]const u8)!PlaywrightError {
    let out_pos: usize = 0;
    let pos: usize = 0;
    while (pos < raw.len) {
        const rest = raw[pos..];
        const hit = simdScanEscapeIndex(rest);
        if (hit == null) {
            if (out_pos + rest.len > buf.len) return PlaywrightError.BufferTooSmall;
            @memcpy(buf[out_pos .. out_pos + rest.len], rest);
            out_pos += rest.len;
            return buf[0..out_pos];
        }
        const idx = hit.?;
        if (idx > 0) {
            if (out_pos + idx > buf.len) return PlaywrightError.BufferTooSmall;
            @memcpy(buf[out_pos .. out_pos + idx], rest[0..idx]);
            out_pos += idx;
        }
        const b = rest[idx];
        if (b == '\\') {
            if (out_pos + 2 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\\';
            buf[out_pos + 1] = '\\';
            out_pos += 2;
        } else if (b == '"') {
            if (out_pos + 2 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\\';
            buf[out_pos + 1] = '"';
            out_pos += 2;
        } else if (b == '\'') {
            if (out_pos + 1 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\'';
            out_pos += 1;
        } else if (b == '\n') {
            if (out_pos + 2 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\\';
            buf[out_pos + 1] = 'n';
            out_pos += 2;
        } else if (b == '\r') {
            if (out_pos + 2 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\\';
            buf[out_pos + 1] = 'r';
            out_pos += 2;
        } else if (b == '\t') {
            if (out_pos + 2 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\\';
            buf[out_pos + 1] = 't';
            out_pos += 2;
        } else if (b == 0x08) {
            if (out_pos + 2 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\\';
            buf[out_pos + 1] = 'b';
            out_pos += 2;
        } else if (b == 0x0c) {
            if (out_pos + 2 > buf.len) return PlaywrightError.BufferTooSmall;
            buf[out_pos] = '\\';
            buf[out_pos + 1] = 'f';
            out_pos += 2;
        } else {
            if (out_pos + 6 > buf.len) return PlaywrightError.BufferTooSmall;
            const hex = std.fmt.bufPrint(buf[out_pos .. out_pos + 6], "\\u{x:0>4}", .{b}) catch return PlaywrightError.BufferTooSmall;
            out_pos += hex.len;
        }
        pos += idx + 1;
    }
    return buf[0..out_pos];
}

/// Allocate and return a JSON-escaped version of `raw` (without surrounding quotes).
pub fn escapeJsonString(alloc: std.mem.Allocator, raw: []const u8): ([]u8)!PlaywrightError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);
    writeEscapedString(&out, alloc, raw, '"')?;
    return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
}

pub enum RecordedActionKind {
    Navigate,
    Click,
    DblClick,
    Fill,
    Press,
    Check,
    Uncheck,
    SelectOption,
    AssertText,

    pub fn asName(): []const u8 {
        if (_ == RecordedActionKind.Navigate) return "navigate";
        if (_ == RecordedActionKind.Click) return "click";
        if (_ == RecordedActionKind.DblClick) return "dblclick";
        if (_ == RecordedActionKind.Fill) return "fill";
        if (_ == RecordedActionKind.Press) return "press";
        if (_ == RecordedActionKind.Check) return "check";
        if (_ == RecordedActionKind.Uncheck) return "uncheck";
        if (_ == RecordedActionKind.SelectOption) return "selectOption";
        return "assertText";
    }

    pub static fn fromName(raw_name: []const u8): RecordedActionKind? {
        if (std.mem.eql(u8, raw_name, "navigate")) return RecordedActionKind.Navigate;
        if (std.mem.eql(u8, raw_name, "click")) return RecordedActionKind.Click;
        if (std.mem.eql(u8, raw_name, "dblclick")) return RecordedActionKind.DblClick;
        if (std.mem.eql(u8, raw_name, "fill")) return RecordedActionKind.Fill;
        if (std.mem.eql(u8, raw_name, "press")) return RecordedActionKind.Press;
        if (std.mem.eql(u8, raw_name, "check")) return RecordedActionKind.Check;
        if (std.mem.eql(u8, raw_name, "uncheck")) return RecordedActionKind.Uncheck;
        if (std.mem.eql(u8, raw_name, "selectOption")) return RecordedActionKind.SelectOption;
        if (std.mem.eql(u8, raw_name, "assertText")) return RecordedActionKind.AssertText;
        return null;
    }
}

pub enum CodegenLang {
    CP,
    TypeScript,
    Python,

    pub static fn fromString(s: []const u8): CodegenLang {
        if (std.mem.eql(u8, s, "ts") || std.mem.eql(u8, s, "typescript")) {
            return CodegenLang.TypeScript;
        }
        if (std.mem.eql(u8, s, "py") || std.mem.eql(u8, s, "python")) {
            return CodegenLang.Python;
        }
        return CodegenLang.CP;
    }
}

pub struct RecordedAction {
    pub kind: RecordedActionKind = RecordedActionKind.Click,
    pub selector: []const u8 = "",
    pub value: []const u8 = "",
    pub timestamp_ms: i64 = 0,
    pub page_id: []const u8 = "main",
}

/// Zero-dependency browser-side DOM event recorder & optimal selector generator.
/// Selector priority: data-testid > aria-label/role > #id > CSS path.
pub const RECORDER_INJECTED_SCRIPT: []const u8 =
    "(function(){" ++
    "if(window.__pwRecorderInstalled)return;" ++
    "window.__pwRecorderInstalled=true;" ++
    "window.__pwRecorderPaused=false;" ++
    "window.__pwRecordedActions=[];" ++
    "function escAttr(s){return String(s).replace(/\\\\/g,'\\\\\\\\').replace(/\"/g,'\\\\\"');}" ++
    "function bestSelector(el){" ++
    "if(!el||el.nodeType!==1)return '';" ++
    "var tid=el.getAttribute('data-testid');" ++
    "if(tid)return '[data-testid=\"'+escAttr(tid)+'\"]';" ++
    "var label=el.getAttribute('aria-label');" ++
    "var role=el.getAttribute('role');" ++
    "if(!role){var tag=el.tagName.toLowerCase();if(tag==='button')role='button';else if(tag==='a'&&el.hasAttribute('href'))role='link';else if(tag==='input')role=(el.type==='checkbox'?'checkbox':(el.type==='radio'?'radio':'textbox'));else if(tag==='select')role='combobox';}" ++
    "if(role&&label)return 'role='+role+'[name=\"'+escAttr(label)+'\"]';" ++
    "if(label)return '[aria-label=\"'+escAttr(label)+'\"]';" ++
    "if(el.id)return '#'+el.id;" ++
    "var parts=[];var cur=el;" ++
    "while(cur&&cur.nodeType===1&&cur.tagName.toLowerCase()!=='html'){" ++
    "if(cur.id){parts.unshift('#'+cur.id);break;}" ++
    "var seg=cur.tagName.toLowerCase();" ++
    "var idx=1,sib=cur.previousElementSibling;" ++
    "while(sib){if(sib.tagName===cur.tagName)idx++;sib=sib.previousElementSibling;}" ++
    "seg+=':nth-of-type('+idx+')';" ++
    "parts.unshift(seg);cur=cur.parentElement;" ++
    "}" ++
    "return parts.join(' > ');" ++
    "}" ++
    "function pushAct(kind,el,val){" ++
    "if(window.__pwRecorderPaused)return;" ++
    "window.__pwRecordedActions.push({kind:kind,selector:bestSelector(el),value:val||'',ts:Date.now()});" ++
    "}" ++
    "document.addEventListener('click',function(e){pushAct('click',e.target,'');},true);" ++
    "document.addEventListener('dblclick',function(e){pushAct('dblclick',e.target,'');},true);" ++
    "document.addEventListener('input',function(e){var t=e.target;if(!t)return;if(t.type==='checkbox'||t.type==='radio')return;pushAct('fill',t,t.value||'');},true);" ++
    "document.addEventListener('change',function(e){var t=e.target;if(!t)return;if(t.tagName==='SELECT')pushAct('selectOption',t,t.value||'');else if(t.type==='checkbox'||t.type==='radio')pushAct(t.checked?'check':'uncheck',t,'');},true);" ++
    "document.addEventListener('keydown',function(e){if(e.key==='Enter'||e.key==='Tab'||e.key==='Escape')pushAct('press',e.target,e.key);},true);" ++
    "window.__pwDrainRecordedActions=function(){var a=window.__pwRecordedActions;window.__pwRecordedActions=[];return a;};" ++
    "})();";

pub struct ActionRecorder {
    arena: std.heap.ArenaAllocator = undefined,
    actions: std.ArrayList(RecordedAction) = .empty,
    endpoint: (*CdpEndpoint)? = null,
    session_id: []const u8 = "",
    deinitialized: bool = false,
    pub active: bool = false,
    pub injected: bool = false,
    pub merged_fill_count: usize = 0,
    pub drained_polls: usize = 0,

    pub static fn init(alloc: std.mem.Allocator): ActionRecorder {
        return ActionRecorder{
            arena: std.heap.ArenaAllocator.init(alloc),
            actions: .empty,
            endpoint: null,
            session_id: "",
            deinitialized: false,
            active: false,
            injected: false,
            merged_fill_count: 0,
            drained_polls: 0,
        };
    }

    pub static fn initWithEndpoint(
        alloc: std.mem.Allocator,
        endpoint: *CdpEndpoint,
        session_id: []const u8,
    ): ActionRecorder {
        return ActionRecorder{
            arena: std.heap.ArenaAllocator.init(alloc),
            actions: .empty,
            endpoint: endpoint,
            session_id: session_id,
            deinitialized: false,
            active: false,
            injected: false,
            merged_fill_count: 0,
            drained_polls: 0,
        };
    }

    pub fn deinit(*_) {
        if (_.deinitialized) return;
        _.deinitialized = true;
        _.actions = .empty;
        _.arena.deinit();
    }

    ~init() {
        _.deinit();
    }

    pub fn attachEndpoint(*_, endpoint: *CdpEndpoint, session_id: []const u8) {
        _.endpoint = endpoint;
        _.session_id = session_id;
    }

    fn arenaDupe(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const a = _.arena.allocator();
        return a.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
    }

    /// Inject RECORDER_INJECTED_SCRIPT into future documents via CDP
    /// Page.addScriptToEvaluateOnNewDocument and immediately into the active
    /// document via Runtime.evaluate.
    pub fn start(*_): void!PlaywrightError {
        if (ep := _.endpoint) {
            if (ep.state != CdpConnectionState.Connected) {
                return PlaywrightError.NotConnected;
            }
            // 1. Register on new documents via Page.addScriptToEvaluateOnNewDocument
            let escaped_buf: [4096]u8 = undefined;
            const escaped_script = escapeJsonIntoBuf(RECORDER_INJECTED_SCRIPT, &escaped_buf)?;

            let params_buf: [4608]u8 = undefined;
            const params = std.fmt.bufPrint(
                &params_buf,
                "{{\"source\":\"{s}\"}}",
                .{escaped_script},
            ) catch return PlaywrightError.BufferTooSmall;

            let frame_buf: [5120]u8 = undefined;
            const add_id = ep.sendCommand(
                _.session_id,
                "Page.addScriptToEvaluateOnNewDocument",
                params,
                &frame_buf,
            ) catch return PlaywrightError.ProtocolError;
            const add_resp = ep.takeResponse(add_id);
            * = add_resp;

            // 2. Evaluate immediately on current document via Runtime.evaluate
            let eval_params_buf: [4608]u8 = undefined;
            let eval_frame_buf: [5120]u8 = undefined;
            const eval_req = RuntimeDomain.evaluate(
                ep.next_id,
                _.session_id,
                RuntimeEvaluateParams{
                    expression: RECORDER_INJECTED_SCRIPT,
                    return_by_value: true,
                    await_promise: false,
                },
                &eval_params_buf,
            ) catch return PlaywrightError.BufferTooSmall;
            const eval_id = ep.sendCommand(
                _.session_id,
                eval_req.method,
                eval_req.params_json,
                &eval_frame_buf,
            ) catch return PlaywrightError.ProtocolError;
            const eval_resp = ep.takeResponse(eval_id);
            * = eval_resp;

            _.injected = true;
        }
        _.active = true;
    }

    /// Pause browser-side recording via Runtime.evaluate and mark inactive.
    pub fn stop(*_): void!PlaywrightError {
        if (ep := _.endpoint) {
            if (ep.state == CdpConnectionState.Connected) {
                let params_buf: [512]u8 = undefined;
                let frame_buf: [768]u8 = undefined;
                const req = RuntimeDomain.evaluate(
                    ep.next_id,
                    _.session_id,
                    RuntimeEvaluateParams{
                        expression: "window.__pwRecorderPaused=true;",
                        return_by_value: true,
                        await_promise: false,
                    },
                    &params_buf,
                ) catch return PlaywrightError.BufferTooSmall;
                const cmd_id = ep.sendCommand(
                    _.session_id,
                    req.method,
                    req.params_json,
                    &frame_buf,
                ) catch return PlaywrightError.ProtocolError;
                const resp = ep.takeResponse(cmd_id);
                * = resp;
            }
        }
        _.active = false;
    }

    /// Poll queued browser actions via CDP Runtime.evaluate.
    pub fn pollBrowserActions(*_): usize!PlaywrightError {
        const ep = _.endpoint orelse return PlaywrightError.NotConnected;
        if (ep.state != CdpConnectionState.Connected) {
            return PlaywrightError.NotConnected;
        }
        let params_buf: [512]u8 = undefined;
        let frame_buf: [1024]u8 = undefined;
        const req = RuntimeDomain.evaluate(
            ep.next_id,
            _.session_id,
            RuntimeEvaluateParams{
                expression: "JSON.stringify(window.__pwDrainRecordedActions?window.__pwDrainRecordedActions():[])",
                return_by_value: true,
                await_promise: false,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        const cmd_id = ep.sendCommand(
            _.session_id,
            req.method,
            req.params_json,
            &frame_buf,
        ) catch return PlaywrightError.ProtocolError;
        const resp = ep.takeResponse(cmd_id);
        * = resp;
        _.drained_polls += 1;
        return _.actions.items.len;
    }

    /// Record an action into the arena-backed queue.
    /// Automatically merges consecutive `Fill` actions targeting the same
    /// selector and page_id (mirroring Playwright's `shouldMergeAction`),
    /// and deduplicates consecutive identical `Navigate` actions.
    pub fn record(*_, action: RecordedAction): void!PlaywrightError {
        const count = _.actions.items.len;
        if (count > 0) {
            let last = &_.actions.items[count - 1];
            if (action.kind == RecordedActionKind.Fill &&
                last.kind == RecordedActionKind.Fill &&
                std.mem.eql(u8, action.selector, last.selector) &&
                std.mem.eql(u8, action.page_id, last.page_id))
            {
                last.value = _.arenaDupe(action.value)?;
                last.timestamp_ms = action.timestamp_ms;
                _.merged_fill_count += 1;
                return;
            }
            if (action.kind == RecordedActionKind.Navigate &&
                last.kind == RecordedActionKind.Navigate &&
                std.mem.eql(u8, action.value, last.value) &&
                std.mem.eql(u8, action.page_id, last.page_id))
            {
                last.timestamp_ms = action.timestamp_ms;
                return;
            }
        }

        const owned_selector = _.arenaDupe(action.selector)?;
        const owned_value = _.arenaDupe(action.value)?;
        const owned_page = _.arenaDupe(action.page_id)?;

        const a = _.arena.allocator();
        _.actions.append(a, RecordedAction{
            kind: action.kind,
            selector: owned_selector,
            value: owned_value,
            timestamp_ms: action.timestamp_ms,
            page_id: owned_page,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn recordNavigate(*_, url: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.Navigate,
            selector: "",
            value: url,
        });
    }

    pub fn recordClick(*_, selector: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.Click,
            selector: selector,
            value: "",
        });
    }

    pub fn recordDblClick(*_, selector: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.DblClick,
            selector: selector,
            value: "",
        });
    }

    pub fn recordFill(*_, selector: []const u8, value: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.Fill,
            selector: selector,
            value: value,
        });
    }

    pub fn recordPress(*_, selector: []const u8, key: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.Press,
            selector: selector,
            value: key,
        });
    }

    pub fn recordCheck(*_, selector: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.Check,
            selector: selector,
            value: "",
        });
    }

    pub fn recordUncheck(*_, selector: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.Uncheck,
            selector: selector,
            value: "",
        });
    }

    pub fn recordSelectOption(*_, selector: []const u8, value: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.SelectOption,
            selector: selector,
            value: value,
        });
    }

    pub fn recordAssertText(*_, selector: []const u8, expected_text: []const u8): void!PlaywrightError {
        return _.record(RecordedAction{
            kind: RecordedActionKind.AssertText,
            selector: selector,
            value: expected_text,
        });
    }

    pub fn actionCount(): usize {
        return _.actions.items.len;
    }

    pub fn getActions(): []const RecordedAction {
        return _.actions.items;
    }

    pub fn clear(*_) {
        _.actions = .empty;
        * = _.arena.reset(.retain_capacity);
        _.merged_fill_count = 0;
    }

    /// Generate complete runnable automation code in CP, TypeScript, or Python.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn generateCode(alloc: std.mem.Allocator, lang: CodegenLang): ([]u8)!PlaywrightError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (lang == CodegenLang.CP) {
            _.emitCpCode(&out, alloc)?;
        } else if (lang == CodegenLang.TypeScript) {
            _.emitTsCode(&out, alloc)?;
        } else {
            _.emitPythonCode(&out, alloc)?;
        }

        return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }

    static fn appendLit(out: *std.ArrayList(u8), alloc: std.mem.Allocator, text: []const u8): void!PlaywrightError {
        out.appendSlice(alloc, text) catch return PlaywrightError.OutOfMemory;
    }

    fn emitCpCode(out: *std.ArrayList(u8), alloc: std.mem.Allocator): void!PlaywrightError {
        ActionRecorder.appendLit(
            out,
            alloc,
            "const std = @import(\"std\");\n" ++
            "import { chromium } from \"playwright\";\n\n" ++
            "fn main(): !void {\n" ++
            "    let gpa: std.heap.DebugAllocator(.{}) = .{};\n" ++
            "    const alloc = gpa.allocator();\n" ++
            "    let browser = chromium.launch(alloc, .{ headless: false })?;\n" ++
            "    defer browser.deinit();\n" ++
            "    const page = browser.newPage()?;\n",
        )?;

        for (act of _.actions.items) {
            if (act.kind == RecordedActionKind.Navigate) {
                ActionRecorder.appendLit(out, alloc, "    * = page.goto(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\", .{})?;\n")?;
            } else if (act.kind == RecordedActionKind.Click) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").click(.{})?;\n")?;
            } else if (act.kind == RecordedActionKind.DblClick) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").dblclick(.{})?;\n")?;
            } else if (act.kind == RecordedActionKind.Fill) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").fill(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")?;\n")?;
            } else if (act.kind == RecordedActionKind.Press) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").press(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")?;\n")?;
            } else if (act.kind == RecordedActionKind.Check) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").check()?;\n")?;
            } else if (act.kind == RecordedActionKind.Uncheck) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").uncheck()?;\n")?;
            } else if (act.kind == RecordedActionKind.SelectOption) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").selectOption(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")?;\n")?;
            } else if (act.kind == RecordedActionKind.AssertText) {
                ActionRecorder.appendLit(out, alloc, "    // Assert text: \"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\"\n    const txt = page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").textContent()?;\n    * = txt;\n")?;
            }
        }

        ActionRecorder.appendLit(out, alloc, "}\n")?;
    }

    fn emitTsCode(out: *std.ArrayList(u8), alloc: std.mem.Allocator): void!PlaywrightError {
        ActionRecorder.appendLit(
            out,
            alloc,
            "import { chromium, expect } from 'playwright';\n\n" ++
            "(async () => {\n" ++
            "  const browser = await chromium.launch({ headless: false });\n" ++
            "  const context = await browser.newContext();\n" ++
            "  const page = await context.newPage();\n",
        )?;

        for (act of _.actions.items) {
            if (act.kind == RecordedActionKind.Navigate) {
                ActionRecorder.appendLit(out, alloc, "  await page.goto('")?;
                writeEscapedString(out, alloc, act.value, '\'')?;
                ActionRecorder.appendLit(out, alloc, "');\n")?;
            } else if (act.kind == RecordedActionKind.Click) {
                ActionRecorder.appendLit(out, alloc, "  await page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "').click();\n")?;
            } else if (act.kind == RecordedActionKind.DblClick) {
                ActionRecorder.appendLit(out, alloc, "  await page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "').dblclick();\n")?;
            } else if (act.kind == RecordedActionKind.Fill) {
                ActionRecorder.appendLit(out, alloc, "  await page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "').fill('")?;
                writeEscapedString(out, alloc, act.value, '\'')?;
                ActionRecorder.appendLit(out, alloc, "');\n")?;
            } else if (act.kind == RecordedActionKind.Press) {
                ActionRecorder.appendLit(out, alloc, "  await page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "').press('")?;
                writeEscapedString(out, alloc, act.value, '\'')?;
                ActionRecorder.appendLit(out, alloc, "');\n")?;
            } else if (act.kind == RecordedActionKind.Check) {
                ActionRecorder.appendLit(out, alloc, "  await page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "').check();\n")?;
            } else if (act.kind == RecordedActionKind.Uncheck) {
                ActionRecorder.appendLit(out, alloc, "  await page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "').uncheck();\n")?;
            } else if (act.kind == RecordedActionKind.SelectOption) {
                ActionRecorder.appendLit(out, alloc, "  await page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "').selectOption('")?;
                writeEscapedString(out, alloc, act.value, '\'')?;
                ActionRecorder.appendLit(out, alloc, "');\n")?;
            } else if (act.kind == RecordedActionKind.AssertText) {
                ActionRecorder.appendLit(out, alloc, "  await expect(page.locator('")?;
                writeEscapedString(out, alloc, act.selector, '\'')?;
                ActionRecorder.appendLit(out, alloc, "')).toContainText('")?;
                writeEscapedString(out, alloc, act.value, '\'')?;
                ActionRecorder.appendLit(out, alloc, "');\n")?;
            }
        }

        ActionRecorder.appendLit(
            out,
            alloc,
            "  await context.close();\n" ++
            "  await browser.close();\n" ++
            "})();\n",
        )?;
    }

    fn emitPythonCode(out: *std.ArrayList(u8), alloc: std.mem.Allocator): void!PlaywrightError {
        ActionRecorder.appendLit(
            out,
            alloc,
            "from playwright.sync_api import sync_playwright, expect\n\n" ++
            "with sync_playwright() as p:\n" ++
            "    browser = p.chromium.launch(headless=False)\n" ++
            "    context = browser.new_context()\n" ++
            "    page = context.new_page()\n",
        )?;

        for (act of _.actions.items) {
            if (act.kind == RecordedActionKind.Navigate) {
                ActionRecorder.appendLit(out, alloc, "    page.goto(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")\n")?;
            } else if (act.kind == RecordedActionKind.Click) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").click()\n")?;
            } else if (act.kind == RecordedActionKind.DblClick) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").dblclick()\n")?;
            } else if (act.kind == RecordedActionKind.Fill) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").fill(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")\n")?;
            } else if (act.kind == RecordedActionKind.Press) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").press(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")\n")?;
            } else if (act.kind == RecordedActionKind.Check) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").check()\n")?;
            } else if (act.kind == RecordedActionKind.Uncheck) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").uncheck()\n")?;
            } else if (act.kind == RecordedActionKind.SelectOption) {
                ActionRecorder.appendLit(out, alloc, "    page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\").select_option(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")\n")?;
            } else if (act.kind == RecordedActionKind.AssertText) {
                ActionRecorder.appendLit(out, alloc, "    expect(page.locator(\"")?;
                writeEscapedString(out, alloc, act.selector, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")).to_contain_text(\"")?;
                writeEscapedString(out, alloc, act.value, '"')?;
                ActionRecorder.appendLit(out, alloc, "\")\n")?;
            }
        }

        ActionRecorder.appendLit(
            out,
            alloc,
            "    context.close()\n" ++
            "    browser.close()\n",
        )?;
    }
}
