// Source: Absorbed from devtools-protocol (Chrome DevTools Protocol 1.3) | Synced: 2026-11-28
// ============================================================================
// lib/cdp/domains.cp — Strongly-Typed Builders for High-Frequency CDP Domains
// Page, Runtime, DOM, Input, Network, and Accessibility (AI Agent Perception)
// ============================================================================

const std = @import("std");
import { CdpError, CdpRequest, CdpFrameScanner } from "./protocol";

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
        let s: []const u8 = "";
        if (p.referrer.len > 0) {
            s = std.fmt.bufPrint(
                params_buf,
                "{{\"url\":\"{s}\",\"referrer\":\"{s}\"}}",
                .{ p.url, p.referrer },
            ) catch return CdpError.BufferTooSmall;
        } else {
            s = std.fmt.bufPrint(
                params_buf,
                "{{\"url\":\"{s}\"}}",
                .{p.url},
            ) catch return CdpError.BufferTooSmall;
        }
        return CdpRequest{
            id: id,
            method: "Page.navigate",
            params_json: s,
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
        let s: []const u8 = "";
        if (p.context_id > 0) {
            s = std.fmt.bufPrint(
                params_buf,
                "{{\"expression\":\"{s}\",\"returnByValue\":{s},\"awaitPromise\":{s},\"contextId\":{d}}}",
                .{ p.expression, rbv, ap, p.context_id },
            ) catch return CdpError.BufferTooSmall;
        } else {
            s = std.fmt.bufPrint(
                params_buf,
                "{{\"expression\":\"{s}\",\"returnByValue\":{s},\"awaitPromise\":{s}}}",
                .{ p.expression, rbv, ap },
            ) catch return CdpError.BufferTooSmall;
        }
        return CdpRequest{
            id: id,
            method: "Runtime.evaluate",
            params_json: s,
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
        const s = std.fmt.bufPrint(
            params_buf,
            "{{\"nodeId\":{d},\"selector\":\"{s}\"}}",
            .{ p.node_id, p.selector },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{
            id: id,
            method: "DOM.querySelector",
            params_json: s,
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
