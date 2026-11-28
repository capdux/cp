// Source: cp/cap/test/testCapProtocolTypes.cp
// Compile + Runtime Integration Test for Full CDP (59 domains) & WebKit (33 domains) Protocol Types

const std = @import("std");
import { test, expect, expect_eq } from "cap:test";

// --- CDP Protocol Types (lib/cdp/protocol_types/) ---
import {
    ProtocolBinary,
    ProtocolJsonValue,
    ProtocolStringMap,
    CDP_DOMAIN_COUNT,
    CDP_EVENT_COUNT,
    CDP_COMMAND_COUNT,
    CDP_DOMAINS,
    CDP_EVENT_NAMES,
    CDP_COMMAND_NAMES,
} from "../../lib/cdp/protocol_types/common.cp";
import {
    Accessibility_AXNodeId,
    Accessibility_AXValue,
    Accessibility_AXNode,
    Accessibility_loadCompletePayload,
    Accessibility_getPartialAXTreeParameters,
} from "../../lib/cdp/protocol_types/domains_01_accessibility_to_bluetoothemulation.cp";
import {
    Network_RequestId,
    Network_LoaderId,
    Network_ResourceType,
    Network_ResourceTiming,
    Network_Request,
    Network_requestWillBeSentPayload,
    Network_enableParameters,
} from "../../lib/cdp/protocol_types/domain_06_network.cp";
import {
    Page_FrameId,
    Page_Frame,
    Page_loadEventFiredPayload,
    Page_navigateParameters,
    Page_navigateReturnValue,
} from "../../lib/cdp/protocol_types/domains_08_page_to_performance.cp";
import {
    Target_TargetID,
    Target_SessionID,
    Target_TargetInfo,
    Target_attachedToTargetPayload,
    Target_attachToTargetParameters,
} from "../../lib/cdp/protocol_types/domains_10_target_to_console.cp";
import {
    Debugger_BreakpointId,
    Debugger_Location,
    Debugger_ResolvedBreakpoint,
    Debugger_breakpointResolvedPayload,
    Debugger_pausedPayload,
} from "../../lib/cdp/protocol_types/domains_11_debugger_to_schema.cp";

// --- WebKit Protocol Types (packages/playwright/src/wk_protocol/) ---
import {
    WK_DOMAIN_COUNT,
    WK_EVENT_COUNT,
    WK_COMMAND_COUNT,
    WK_DOMAINS,
    WK_EVENT_NAMES,
    WK_COMMAND_NAMES,
} from "../../packages/playwright/src/wk_protocol/common.cp";
import {
    Animation_AnimationId,
    Animation_Animation,
    Animation_animationCreatedPayload,
    Animation_trackingUpdatePayload,
} from "../../packages/playwright/src/wk_protocol/domains_01_animation_to_canvas.cp";
import {
    LayerTree_LayerId,
    LayerTree_IntRect,
    LayerTree_Layer,
    LayerTree_layersForNodeParameters,
} from "../../packages/playwright/src/wk_protocol/domains_04_layertree_to_network.cp";
import {
    Page_Frame as WkPage_Frame,
    Page_Cookie as WkPage_Cookie,
    Page_loadEventFiredPayload as WkPage_loadEventFiredPayload,
    Playwright_Geolocation,
    Playwright_pageProxyCreatedPayload,
    Playwright_navigateParameters,
} from "../../packages/playwright/src/wk_protocol/domains_05_page_to_recording.cp";
import {
    Runtime_RemoteObjectId as WkRuntime_RemoteObjectId,
    Runtime_RemoteObject as WkRuntime_RemoteObject,
    Runtime_ExecutionContextDescription as WkRuntime_ExecutionContextDescription,
} from "../../packages/playwright/src/wk_protocol/domains_06_runtime_to_worker.cp";

fn sliceContains(haystack: []const []const u8, needle: []const u8): bool {
    for (item of haystack) {
        if (std.mem.eql(u8, item, needle)) return true;
    }
    return false;
}

test("CDP common registry metadata & primitive aliases") {
    expect_eq(CDP_DOMAIN_COUNT, 59);
    expect_eq(CDP_EVENT_COUNT, 232);
    expect_eq(CDP_COMMAND_COUNT, 677);
    expect_eq(CDP_DOMAINS.len, CDP_DOMAIN_COUNT);
    expect_eq(CDP_EVENT_NAMES.len, CDP_EVENT_COUNT);
    expect_eq(CDP_COMMAND_NAMES.len, CDP_COMMAND_COUNT);

    expect(sliceContains(CDP_DOMAINS, "Accessibility"));
    expect(sliceContains(CDP_DOMAINS, "Network"));
    expect(sliceContains(CDP_DOMAINS, "Page"));
    expect(sliceContains(CDP_DOMAINS, "Target"));
    expect(sliceContains(CDP_DOMAINS, "Debugger"));
    expect(sliceContains(CDP_EVENT_NAMES, "Accessibility.loadComplete"));
    expect(sliceContains(CDP_EVENT_NAMES, "DOM.attributeModified"));
    expect(sliceContains(CDP_COMMAND_NAMES, "Accessibility.enable"));
    expect(sliceContains(CDP_COMMAND_NAMES, "Accessibility.getPartialAXTree"));

    const bin: ProtocolBinary = @as(ProtocolBinary, "AQID");
    const raw_json: ProtocolJsonValue = @as(ProtocolJsonValue, "{\"ok\":true}");
    const headers: ProtocolStringMap = @as(ProtocolStringMap, "{\"content-type\":\"application/json\"}");
    expect_eq(bin.len, 4);
    expect(std.mem.eql(u8, raw_json, "{\"ok\":true}"));
    expect(headers.len > 0);
}

test("CDP Accessibility domain structs, defaults & field assignment") {
    const default_node = Accessibility_AXNode{};
    expect_eq(default_node.nodeId.len, 0);
    expect_eq(default_node.ignored, false);
    expect(default_node.role == null);

    const node_id: Accessibility_AXNodeId = @as(Accessibility_AXNodeId, "ax-root-1");
    let payload = Accessibility_loadCompletePayload{};
    payload.root.nodeId = node_id;
    payload.root.ignored = false;
    payload.root.role = Accessibility_AXValue{ .type_ = "role", .value = "\"RootWebArea\"" };
    expect(std.mem.eql(u8, payload.root.nodeId, "ax-root-1"));
    expect(payload.root.role != null);
    expect(std.mem.eql(u8, payload.root.role.?.type_, "role"));

    let params = Accessibility_getPartialAXTreeParameters{};
    params.fetchRelatives = true;
    expect(params.nodeId == null);
    expect_eq(params.fetchRelatives.?, true);
}

test("CDP Network & Page domain structs, events & commands") {
    const default_timing = Network_ResourceTiming{};
    expect_eq(default_timing.requestTime, 0.0);
    expect(default_timing.workerRouterEvaluationStart == null);

    const req_id: Network_RequestId = @as(Network_RequestId, "req-42");
    const res_type: Network_ResourceType = @as(Network_ResourceType, "Document");
    let ev = Network_requestWillBeSentPayload{};
    ev.requestId = req_id;
    ev.loaderId = @as(Network_LoaderId, "loader-1");
    ev.documentURL = "https://example.com/";
    ev.request = Network_Request{
        .url = "https://example.com/",
        .method = "GET",
        .initialPriority = "VeryHigh",
    };
    ev.type_ = res_type;
    expect(std.mem.eql(u8, ev.requestId, "req-42"));
    expect(std.mem.eql(u8, ev.request.method, "GET"));
    expect(std.mem.eql(u8, ev.type_.?, "Document"));

    let net_en = Network_enableParameters{};
    net_en.maxTotalBufferSize = 1048576.0;
    net_en.reportDirectSocketTraffic = true;
    expect_eq(net_en.maxTotalBufferSize.?, 1048576.0);
    expect_eq(net_en.reportDirectSocketTraffic.?, true);

    const frame_id: Page_FrameId = @as(Page_FrameId, "frame-main");
    let frame = Page_Frame{};
    frame.id = frame_id;
    frame.url = "https://example.com/";
    frame.mimeType = "text/html";
    expect(std.mem.eql(u8, frame.id, "frame-main"));
    expect(frame.parentId == null);

    let nav_params = Page_navigateParameters{};
    nav_params.url = "https://example.com/docs";
    nav_params.frameId = frame_id;
    let nav_ret = Page_navigateReturnValue{};
    nav_ret.frameId = frame_id;
    const load_ev = Page_loadEventFiredPayload{ .timestamp = 1234.5 };
    expect(std.mem.eql(u8, nav_params.url, "https://example.com/docs"));
    expect(std.mem.eql(u8, nav_ret.frameId, "frame-main"));
    expect_eq(load_ev.timestamp, 1234.5);
}

test("CDP Target & Debugger domain structs, events & commands") {
    const tid: Target_TargetID = @as(Target_TargetID, "target-1");
    const sid: Target_SessionID = @as(Target_SessionID, "session-1");
    let attached = Target_attachedToTargetPayload{};
    attached.sessionId = sid;
    attached.targetInfo = Target_TargetInfo{
        .targetId = tid,
        .type_ = "page",
        .title = "CP Integration",
        .url = "https://cp.dev",
        .attached = true,
    };
    attached.waitingForDebugger = true;
    expect(std.mem.eql(u8, attached.sessionId, "session-1"));
    expect(std.mem.eql(u8, attached.targetInfo.type_, "page"));
    expect_eq(attached.targetInfo.attached, true);

    let attach_cmd = Target_attachToTargetParameters{};
    attach_cmd.targetId = tid;
    attach_cmd.flatten = true;
    expect_eq(attach_cmd.flatten.?, true);

    const bp_id: Debugger_BreakpointId = @as(Debugger_BreakpointId, "bp:1:10");
    const loc = Debugger_Location{ .scriptId = "script-7", .lineNumber = 10.0, .columnNumber = 4.0 };
    const resolved = Debugger_ResolvedBreakpoint{ .breakpointId = bp_id, .location = loc };
    const bp_ev = Debugger_breakpointResolvedPayload{ .breakpointId = bp_id, .location = loc };
    const paused = Debugger_pausedPayload{ .reason = "breakpoint" };
    expect(std.mem.eql(u8, resolved.breakpointId, "bp:1:10"));
    expect_eq(bp_ev.location.lineNumber, 10.0);
    expect_eq(bp_ev.location.columnNumber.?, 4.0);
    expect(std.mem.eql(u8, paused.reason, "breakpoint"));
    expect_eq(paused.callFrames.len, 0);
}

test("WebKit common registry metadata & constants") {
    expect_eq(WK_DOMAIN_COUNT, 33);
    expect_eq(WK_EVENT_COUNT, 132);
    expect_eq(WK_COMMAND_COUNT, 311);
    expect_eq(WK_DOMAINS.len, WK_DOMAIN_COUNT);
    expect_eq(WK_EVENT_NAMES.len, WK_EVENT_COUNT);
    expect_eq(WK_COMMAND_NAMES.len, WK_COMMAND_COUNT);

    expect(sliceContains(WK_DOMAINS, "Animation"));
    expect(sliceContains(WK_DOMAINS, "LayerTree"));
    expect(sliceContains(WK_DOMAINS, "Network"));
    expect(sliceContains(WK_DOMAINS, "Page"));
    expect(sliceContains(WK_DOMAINS, "Playwright"));
    expect(sliceContains(WK_DOMAINS, "Runtime"));
    expect(sliceContains(WK_EVENT_NAMES, "Animation.animationCreated"));
    expect(sliceContains(WK_EVENT_NAMES, "Debugger.scriptParsed"));
    expect(sliceContains(WK_COMMAND_NAMES, "Animation.enable"));
    expect(sliceContains(WK_COMMAND_NAMES, "CSS.getMatchedStylesForNode"));
}

test("WebKit Animation, LayerTree, Page, Playwright & Runtime structs") {
    const anim_id: Animation_AnimationId = @as(Animation_AnimationId, "wk-anim-1");
    let anim_ev = Animation_animationCreatedPayload{};
    anim_ev.animation = Animation_Animation{ .animationId = anim_id, .name = "fadeIn" };
    const track_ev = Animation_trackingUpdatePayload{ .timestamp = 55.25 };
    expect(std.mem.eql(u8, anim_ev.animation.animationId, "wk-anim-1"));
    expect(std.mem.eql(u8, anim_ev.animation.name.?, "fadeIn"));
    expect_eq(track_ev.timestamp, 55.25);

    const layer_id: LayerTree_LayerId = @as(LayerTree_LayerId, "layer-0");
    let layer = LayerTree_Layer{};
    layer.layerId = layer_id;
    layer.bounds = LayerTree_IntRect{ .x = 0.0, .y = 0.0, .width = 1280.0, .height = 720.0 };
    const layer_params = LayerTree_layersForNodeParameters{ .nodeId = 12.0 };
    expect(std.mem.eql(u8, layer.layerId, "layer-0"));
    expect_eq(layer.bounds.width, 1280.0);
    expect_eq(layer_params.nodeId, 12.0);

    let wk_frame = WkPage_Frame{};
    wk_frame.id = "wk-frame-1";
    wk_frame.url = "https://webkit.org/";
    const wk_cookie = WkPage_Cookie{ .name = "sid", .value = "abc", .secure = true, .httpOnly = true };
    const wk_load = WkPage_loadEventFiredPayload{ .timestamp = 99.5 };
    expect(std.mem.eql(u8, wk_frame.id, "wk-frame-1"));
    expect_eq(wk_cookie.secure, true);
    expect_eq(wk_load.timestamp, 99.5);

    const geo = Playwright_Geolocation{ .latitude = 37.7749, .longitude = -122.4194, .accuracy = 10.0 };
    const proxy_ev = Playwright_pageProxyCreatedPayload{ .browserContextId = "ctx-1", .pageProxyId = "pp-1" };
    const pw_nav = Playwright_navigateParameters{ .url = "https://playwright.dev", .pageProxyId = "pp-1" };
    expect_eq(geo.accuracy, 10.0);
    expect(std.mem.eql(u8, proxy_ev.pageProxyId, "pp-1"));
    expect(std.mem.eql(u8, pw_nav.url, "https://playwright.dev"));

    const obj_id: WkRuntime_RemoteObjectId = @as(WkRuntime_RemoteObjectId, "wk-obj-1");
    let remote = WkRuntime_RemoteObject{};
    remote.type_ = "object";
    remote.objectId = obj_id;
    const ctx_desc = WkRuntime_ExecutionContextDescription{ .id = 1.0, .type_ = "normal", .name = "main", .frameId = "wk-frame-1" };
    expect(std.mem.eql(u8, remote.objectId.?, "wk-obj-1"));
    expect(std.mem.eql(u8, ctx_desc.name, "main"));
}
