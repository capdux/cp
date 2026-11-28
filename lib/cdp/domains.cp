// Source: Absorbed from devtools-protocol (Chrome DevTools Protocol 1.3) | Synced: 2026-11-28
// ============================================================================
// lib/cdp/domains.cp — Strongly-Typed Builders for High-Frequency CDP Domains
// Page, Runtime, DOM, Input, Network, and Accessibility (AI Agent Perception)
// ============================================================================

const std = @import("std");
import { CdpError, CdpRequest, CdpFrameScanner } from "./protocol";

fn cdpWriteRaw(dst: []u8, pos: usize, s: []const u8): usize!CdpError {
    if (pos + s.len > dst.len) return CdpError.BufferTooSmall;
    if (s.len > 0) @memcpy(dst[pos .. pos + s.len], s);
    return pos + s.len;
}

fn cdpWriteJsonEscaped(dst: []u8, start_pos: usize, s: []const u8): usize!CdpError {
    let pos = start_pos;
    for (b of s) {
        if (b == '"' || b == '\\') {
            if (pos + 2 > dst.len) return CdpError.BufferTooSmall;
            dst[pos] = '\\';
            dst[pos + 1] = b;
            pos += 2;
        } else if (b == '\n') {
            if (pos + 2 > dst.len) return CdpError.BufferTooSmall;
            dst[pos] = '\\';
            dst[pos + 1] = 'n';
            pos += 2;
        } else if (b == '\r') {
            if (pos + 2 > dst.len) return CdpError.BufferTooSmall;
            dst[pos] = '\\';
            dst[pos + 1] = 'r';
            pos += 2;
        } else if (b == '\t') {
            if (pos + 2 > dst.len) return CdpError.BufferTooSmall;
            dst[pos] = '\\';
            dst[pos + 1] = 't';
            pos += 2;
        } else if (b < 0x20) {
            if (pos + 6 > dst.len) return CdpError.BufferTooSmall;
            const hex = "0123456789abcdef";
            dst[pos] = '\\';
            dst[pos + 1] = 'u';
            dst[pos + 2] = '0';
            dst[pos + 3] = '0';
            dst[pos + 4] = hex[(b >> 4) & 0x0f];
            dst[pos + 5] = hex[b & 0x0f];
            pos += 6;
        } else {
            if (pos + 1 > dst.len) return CdpError.BufferTooSmall;
            dst[pos] = b;
            pos += 1;
        }
    }
    return pos;
}

// ----------------------------------------------------------------------------
// Page Domain
// ----------------------------------------------------------------------------

pub struct PageNavigateParams {
    url: []const u8 = "",
    referrer: []const u8 = "",
    transition_type: []const u8 = "",
}

pub struct PageCaptureScreenshotParams {
    format: []const u8 = "png",
    quality: u8 = 80,
    from_surface: bool = true,
    capture_beyond_viewport: bool = false,
}

pub struct PagePrintToPdfParams {
    landscape: bool = false,
    print_background: bool = true,
    scale: f64 = 1.0,
    paper_width: f64 = 8.5,
    paper_height: f64 = 11.0,
}

pub struct PageDomain {
    pub static fn navigate(id: i64, session_id: []const u8, p: PageNavigateParams, params_buf: []u8): CdpRequest!CdpError {
        let pos: usize = 0;
        pos = cdpWriteRaw(params_buf, pos, "{\"url\":\"")?;
        pos = cdpWriteJsonEscaped(params_buf, pos, p.url)?;
        if (p.referrer.len > 0) {
            pos = cdpWriteRaw(params_buf, pos, "\",\"referrer\":\"")?;
            pos = cdpWriteJsonEscaped(params_buf, pos, p.referrer)?;
        }
        pos = cdpWriteRaw(params_buf, pos, "\"}")?;
        return CdpRequest{
            id: id,
            method: "Page.navigate",
            params_json: params_buf[0..pos],
            session_id: session_id,
        };
    }

    pub static fn captureScreenshot(id: i64, session_id: []const u8, p: PageCaptureScreenshotParams, params_buf: []u8): CdpRequest!CdpError {
        const fs_str: []const u8 = if (p.from_surface) "true" else "false";
        const cbv_str: []const u8 = if (p.capture_beyond_viewport) "true" else "false";
        const s = std.fmt.bufPrint(
            params_buf,
            "{{\"format\":\"{s}\",\"quality\":{d},\"fromSurface\":{s},\"captureBeyondViewport\":{s}}}",
            .{ p.format, p.quality, fs_str, cbv_str },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{
            id: id,
            method: "Page.captureScreenshot",
            params_json: s,
            session_id: session_id,
        };
    }

    pub static fn printToPDF(id: i64, session_id: []const u8, p: PagePrintToPdfParams, params_buf: []u8): CdpRequest!CdpError {
        const ls_str: []const u8 = if (p.landscape) "true" else "false";
        const bg_str: []const u8 = if (p.print_background) "true" else "false";
        const s = std.fmt.bufPrint(
            params_buf,
            "{{\"landscape\":{s},\"printBackground\":{s},\"scale\":{d:.2},\"paperWidth\":{d:.2},\"paperHeight\":{d:.2}}}",
            .{ ls_str, bg_str, p.scale, p.paper_width, p.paper_height },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{
            id: id,
            method: "Page.printToPDF",
            params_json: s,
            session_id: session_id,
        };
    }
}

// ----------------------------------------------------------------------------
// Runtime Domain
// ----------------------------------------------------------------------------

pub struct RuntimeEvaluateParams {
    expression: []const u8 = "",
    return_by_value: bool = true,
    await_promise: bool = true,
    context_id: i64 = 0,
}

pub struct RuntimeDomain {
    pub static fn evaluate(id: i64, session_id: []const u8, p: RuntimeEvaluateParams, params_buf: []u8): CdpRequest!CdpError {
        const rbv: []const u8 = if (p.return_by_value) "true" else "false";
        const ap: []const u8 = if (p.await_promise) "true" else "false";
        let pos: usize = 0;
        pos = cdpWriteRaw(params_buf, pos, "{\"expression\":\"")?;
        pos = cdpWriteJsonEscaped(params_buf, pos, p.expression)?;
        let tail_buf: [128]u8 = undefined;
        const tail = if (p.context_id > 0)
            std.fmt.bufPrint(&tail_buf, "\",\"returnByValue\":{s},\"awaitPromise\":{s},\"contextId\":{d}}}", .{ rbv, ap, p.context_id }) catch return CdpError.BufferTooSmall
        else
            std.fmt.bufPrint(&tail_buf, "\",\"returnByValue\":{s},\"awaitPromise\":{s}}}", .{ rbv, ap }) catch return CdpError.BufferTooSmall;
        pos = cdpWriteRaw(params_buf, pos, tail)?;
        return CdpRequest{
            id: id,
            method: "Runtime.evaluate",
            params_json: params_buf[0..pos],
            session_id: session_id,
        };
    }
}

// ----------------------------------------------------------------------------
// DOM Domain
// ----------------------------------------------------------------------------

pub struct DomGetDocumentParams {
    depth: i32 = -1,
    pierce: bool = true,
}

pub struct DomQuerySelectorParams {
    node_id: i64 = 1,
    selector: []const u8 = "",
}

pub struct DomDomain {
    pub static fn getDocument(id: i64, session_id: []const u8, p: DomGetDocumentParams, params_buf: []u8): CdpRequest!CdpError {
        const pierce_str: []const u8 = if (p.pierce) "true" else "false";
        const s = std.fmt.bufPrint(
            params_buf,
            "{{\"depth\":{d},\"pierce\":{s}}}",
            .{ p.depth, pierce_str },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{
            id: id,
            method: "DOM.getDocument",
            params_json: s,
            session_id: session_id,
        };
    }

    pub static fn querySelector(id: i64, session_id: []const u8, p: DomQuerySelectorParams, params_buf: []u8): CdpRequest!CdpError {
        let head_buf: [64]u8 = undefined;
        const head = std.fmt.bufPrint(&head_buf, "{{\"nodeId\":{d},\"selector\":\"", .{p.node_id}) catch return CdpError.BufferTooSmall;
        let pos: usize = 0;
        pos = cdpWriteRaw(params_buf, pos, head)?;
        pos = cdpWriteJsonEscaped(params_buf, pos, p.selector)?;
        pos = cdpWriteRaw(params_buf, pos, "\"}")?;
        return CdpRequest{
            id: id,
            method: "DOM.querySelector",
            params_json: params_buf[0..pos],
            session_id: session_id,
        };
    }
}

// ----------------------------------------------------------------------------
// Input Domain
// ----------------------------------------------------------------------------

pub struct InputMouseEventParams {
    event_type: []const u8 = "mousePressed",
    x: f64 = 0.0,
    y: f64 = 0.0,
    button: []const u8 = "left",
    click_count: i32 = 1,
}

pub struct InputKeyEventParams {
    event_type: []const u8 = "keyDown",
    key: []const u8 = "",
    code: []const u8 = "",
    text: []const u8 = "",
    modifiers: i32 = 0,
}

pub struct InputDomain {
    pub static fn dispatchMouseEvent(id: i64, session_id: []const u8, p: InputMouseEventParams, params_buf: []u8): CdpRequest!CdpError {
        const s = std.fmt.bufPrint(
            params_buf,
            "{{\"type\":\"{s}\",\"x\":{d:.1},\"y\":{d:.1},\"button\":\"{s}\",\"clickCount\":{d}}}",
            .{ p.event_type, p.x, p.y, p.button, p.click_count },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{
            id: id,
            method: "Input.dispatchMouseEvent",
            params_json: s,
            session_id: session_id,
        };
    }

    pub static fn dispatchKeyEvent(id: i64, session_id: []const u8, p: InputKeyEventParams, params_buf: []u8): CdpRequest!CdpError {
        const s = std.fmt.bufPrint(
            params_buf,
            "{{\"type\":\"{s}\",\"key\":\"{s}\",\"code\":\"{s}\",\"text\":\"{s}\",\"modifiers\":{d}}}",
            .{ p.event_type, p.key, p.code, p.text, p.modifiers },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{
            id: id,
            method: "Input.dispatchKeyEvent",
            params_json: s,
            session_id: session_id,
        };
    }
}

// ----------------------------------------------------------------------------
// Network Domain
// ----------------------------------------------------------------------------

pub struct NetworkEnableParams {
    max_total_buffer_size: i64 = 0,
    max_resource_buffer_size: i64 = 0,
}

pub struct NetworkDomain {
    pub static fn enable(id: i64, session_id: []const u8, p: NetworkEnableParams, params_buf: []u8): CdpRequest!CdpError {
        let s: []const u8 = "{}";
        if (p.max_total_buffer_size > 0 || p.max_resource_buffer_size > 0) {
            s = std.fmt.bufPrint(
                params_buf,
                "{{\"maxTotalBufferSize\":{d},\"maxResourceBufferSize\":{d}}}",
                .{ p.max_total_buffer_size, p.max_resource_buffer_size },
            ) catch return CdpError.BufferTooSmall;
        }
        return CdpRequest{
            id: id,
            method: "Network.enable",
            params_json: s,
            session_id: session_id,
        };
    }
}

// ----------------------------------------------------------------------------
// Accessibility Domain (AI Agent Web Perception Core)
// ----------------------------------------------------------------------------

pub struct AccessibilityGetFullAXTreeParams {
    depth: i32 = -1,
    frame_id: []const u8 = "",
}

pub struct AXNodeSummary {
    node_id: []const u8 = "",
    role: []const u8 = "",
    name: []const u8 = "",
    value: []const u8 = "",
    ignored: bool = false,
    backend_dom_node_id: i64 = 0,
}

pub struct AccessibilityDomain {
    pub static fn getFullAXTree(id: i64, session_id: []const u8, p: AccessibilityGetFullAXTreeParams, params_buf: []u8): CdpRequest!CdpError {
        let s: []const u8 = "";
        if (p.frame_id.len > 0) {
            s = std.fmt.bufPrint(
                params_buf,
                "{{\"depth\":{d},\"frameId\":\"{s}\"}}",
                .{ p.depth, p.frame_id },
            ) catch return CdpError.BufferTooSmall;
        } else {
            s = std.fmt.bufPrint(
                params_buf,
                "{{\"depth\":{d}}}",
                .{p.depth},
            ) catch return CdpError.BufferTooSmall;
        }
        return CdpRequest{
            id: id,
            method: "Accessibility.getFullAXTree",
            params_json: s,
            session_id: session_id,
        };
    }

    /// Zero-heap parser for a single CDP AXNode JSON object into AXNodeSummary.
    pub static fn parseAXNodeSummary(node_json: []const u8): AXNodeSummary {
        const node_id = CdpFrameScanner.extractTopLevelField(node_json, "nodeId", true) orelse "";
        const ignored_raw = CdpFrameScanner.extractTopLevelField(node_json, "ignored", false) orelse "false";
        const ignored = std.mem.eql(u8, ignored_raw, "true");

        let backend_id: i64 = 0;
        if (bid_raw := CdpFrameScanner.extractTopLevelField(node_json, "backendDOMNodeId", false)) {
            if (bid := CdpFrameScanner.parseFastI64(bid_raw)) {
                backend_id = bid;
            }
        }

        let role: []const u8 = "";
        if (role_obj := CdpFrameScanner.extractTopLevelField(node_json, "role", false)) {
            role = CdpFrameScanner.extractTopLevelField(role_obj, "value", true) orelse "";
        }

        let name: []const u8 = "";
        if (name_obj := CdpFrameScanner.extractTopLevelField(node_json, "name", false)) {
            name = CdpFrameScanner.extractTopLevelField(name_obj, "value", true) orelse "";
        }

        let value: []const u8 = "";
        if (val_obj := CdpFrameScanner.extractTopLevelField(node_json, "value", false)) {
            value = CdpFrameScanner.extractTopLevelField(val_obj, "value", true) orelse "";
        }

        return AXNodeSummary{
            node_id: node_id,
            role: role,
            name: name,
            value: value,
            ignored: ignored,
            backend_dom_node_id: backend_id,
        };
    }
}
