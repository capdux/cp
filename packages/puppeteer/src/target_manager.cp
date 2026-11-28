// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/TargetManager.ts | Synced: 2026-11-28
// Also absorbs _ref/puppeteer/packages/puppeteer-core/src/cdp/Accessibility.ts
// ============================================================================
// packages/puppeteer/src/target_manager.cp — TargetManager & AccessibilityTreeBuilder
// Multi-target auto-attach tracking (Page / Worker / ServiceWorker / OOPIF iframe)
// and recursive ARIA Accessibility Tree serialization with interestingOnly filtering.
// ============================================================================

const std = @import("std");
import {
    CdpEvent,
    CdpFrameScanner,
    AXNodeSummary,
    AccessibilityDomain,
} from "cap:cdp";
import {
    PuppeteerError,
    TargetType,
    TargetInfo,
} from "./types";
import { CDPSession } from "./cdp_session";
import {
    EventHandlerFn,
    PuppeteerEventEmitter,
} from "./events";

type TargetVec16 = @Vector(16, u8);

pub struct TargetSimdScanner {
    /// 16-lane SIMD scan for a single delimiter byte.
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize? {
        let i: usize = 0;
        const n = haystack.len;
        const v_needle: TargetVec16 = @splat(needle);
        while (i + 16 <= n) {
            const chunk: TargetVec16 = haystack[i..][0..16].*;
            const mask: u16 = @bitCast(chunk == v_needle);
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }
        while (i < n) {
            if (haystack[i] == needle) return i;
            i += 1;
        }
        return null;
    }

    /// 16-lane SIMD check whether a byte slice contains any non-whitespace ASCII character.
    pub static fn hasNonWhitespaceSimd(input: []const u8): bool {
        let i: usize = 0;
        const n = input.len;
        const v_sp: TargetVec16 = @splat(' ');
        const v_tab: TargetVec16 = @splat('\t');
        const v_cr: TargetVec16 = @splat('\r');
        const v_lf: TargetVec16 = @splat('\n');

        while (i + 16 <= n) {
            const chunk: TargetVec16 = input[i..][0..16].*;
            const m1: u16 = @bitCast(chunk == v_sp);
            const m2: u16 = @bitCast(chunk == v_tab);
            const m3: u16 = @bitCast(chunk == v_cr);
            const m4: u16 = @bitCast(chunk == v_lf);
            const ws_mask: u16 = m1 | m2 | m3 | m4;
            if (ws_mask != 0xFFFF) return true;
            i += 16;
        }
        while (i < n) {
            const c = input[i];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return true;
            i += 1;
        }
        return false;
    }
}

// ============================================================================
// Part 1: TargetManager (Page / Worker / ServiceWorker / OOPIF iframe)
// ============================================================================

pub enum ManagedTargetKind {
    Page,
    Iframe,
    Worker,
    ServiceWorker,
    SharedWorker,
    BackgroundPage,
    Browser,
    Webview,
    Other,
}

pub fn parseManagedTargetKind(raw: []const u8): ManagedTargetKind {
    if (std.mem.eql(u8, raw, "page")) return ManagedTargetKind.Page;
    if (std.mem.eql(u8, raw, "iframe")) return ManagedTargetKind.Iframe;
    if (std.mem.eql(u8, raw, "worker")) return ManagedTargetKind.Worker;
    if (std.mem.eql(u8, raw, "service_worker")) return ManagedTargetKind.ServiceWorker;
    if (std.mem.eql(u8, raw, "shared_worker")) return ManagedTargetKind.SharedWorker;
    if (std.mem.eql(u8, raw, "background_page")) return ManagedTargetKind.BackgroundPage;
    if (std.mem.eql(u8, raw, "browser")) return ManagedTargetKind.Browser;
    if (std.mem.eql(u8, raw, "webview")) return ManagedTargetKind.Webview;
    return ManagedTargetKind.Other;
}

fn toTargetType(kind: ManagedTargetKind): TargetType {
    switch (kind) {
        ManagedTargetKind.Page => return TargetType.Page,
        ManagedTargetKind.BackgroundPage => return TargetType.BackgroundPage,
        ManagedTargetKind.ServiceWorker => return TargetType.ServiceWorker,
        ManagedTargetKind.SharedWorker => return TargetType.SharedWorker,
        ManagedTargetKind.Browser => return TargetType.Browser,
        ManagedTargetKind.Webview => return TargetType.Webview,
        ManagedTargetKind.Iframe,
        ManagedTargetKind.Worker,
        ManagedTargetKind.Other => return TargetType.Other,
    }
}

pub struct ManagedTarget {
    target_id: []const u8 = "",
    session_id: []const u8 = "",
    parent_target_id: []const u8 = "",
    kind: ManagedTargetKind = ManagedTargetKind.Other,
    info: TargetInfo = .{},
    waiting_for_debugger: bool = false,
    destroyed: bool = false,

    pub fn id(): []const u8 {
        return _.target_id;
    }

    pub fn sessionId(): []const u8 {
        return _.session_id;
    }

    pub fn isAttached(): bool {
        return _.info.attached && _.session_id.len > 0 && !_.destroyed;
    }

    pub fn isOopif(): bool {
        return _.kind == ManagedTargetKind.Iframe;
    }
}

pub struct TargetManager {
    allocator: std.mem.Allocator = undefined,
    session: *CDPSession = undefined,
    emitter: PuppeteerEventEmitter = .{},
    targets_list: std.ArrayList(*ManagedTarget) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    auto_attach_enabled: bool = false,
    discover_enabled: bool = false,
    disposed: bool = false,

    pub static fn init(alloc: std.mem.Allocator, session: *CDPSession): TargetManager {
        return TargetManager{
            allocator: alloc,
            session: session,
            emitter: PuppeteerEventEmitter.init(),
            targets_list: .empty,
            owned_strings: .empty,
            auto_attach_enabled: false,
            discover_enabled: false,
            disposed: false,
        };
    }

    pub fn deinit(*_) {
        if (_.disposed) return;
        _.disposed = true;

        for (t of _.targets_list.items) {
            _.allocator.destroy(t);
        }
        _.targets_list.deinit(_.allocator);
        _.targets_list = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;

        _.emitter.removeAllListeners("");
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PuppeteerError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PuppeteerError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PuppeteerError.OutOfMemory;
        };
        return duped;
    }

    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.on(event_name, handler, ctx);
    }

    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.once(event_name, handler, ctx);
    }

    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn) {
        _.emitter.off(event_name, handler);
    }

    /// Sends `Target.setAutoAttach` (supports OOPIF iframes, Workers & ServiceWorkers).
    pub fn setAutoAttach(
        *_,
        auto_attach: bool,
        wait_for_debugger_on_start: bool,
        flatten: bool,
    ): void!PuppeteerError {
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"autoAttach\":{},\"waitForDebuggerOnStart\":{},\"flatten\":{}}}",
            .{ auto_attach, wait_for_debugger_on_start, flatten },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_1 = _.session.send("Target.setAutoAttach", params, &fbuf)?;
        * = _ign_1;
        _.auto_attach_enabled = auto_attach;
    }

    /// Sends `Target.setDiscoverTargets`.
    pub fn setDiscoverTargets(*_, discover: bool): void!PuppeteerError {
        let pbuf: [64]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(&pbuf, "{{\"discover\":{}}}", .{discover}) catch return PuppeteerError.BufferTooSmall;
        const _ign_1 = _.session.send("Target.setDiscoverTargets", params, &fbuf)?;
        * = _ign_1;
        _.discover_enabled = discover;
    }

    pub fn isAutoAttachEnabled(): bool {
        return _.auto_attach_enabled;
    }

    pub fn targetById(target_id: []const u8): (*ManagedTarget)? {
        for (t of _.targets_list.items) {
            if (std.mem.eql(u8, t.target_id, target_id)) return t;
        }
        return null;
    }

    pub fn targetBySessionId(session_id: []const u8): (*ManagedTarget)? {
        for (t of _.targets_list.items) {
            if (t.session_id.len > 0 && std.mem.eql(u8, t.session_id, session_id)) return t;
        }
        return null;
    }

    pub fn targets(): []const *ManagedTarget {
        return _.targets_list.items;
    }

    pub fn targetCount(): usize {
        return _.targets_list.items.len;
    }

    pub fn activeTargetCount(): usize {
        let total: usize = 0;
        for (t of _.targets_list.items) {
            if (!t.destroyed) total += 1;
        }
        return total;
    }

    pub fn attachedCount(): usize {
        let total: usize = 0;
        for (t of _.targets_list.items) {
            if (t.isAttached()) total += 1;
        }
        return total;
    }

    pub fn countByKind(kind: ManagedTargetKind): usize {
        let total: usize = 0;
        for (t of _.targets_list.items) {
            if (!t.destroyed && t.kind == kind) total += 1;
        }
        return total;
    }

    fn upsertTargetInfo(*_, info_obj: []const u8): (*ManagedTarget)!PuppeteerError {
        const tid_raw = CdpFrameScanner.extractTopLevelField(info_obj, "targetId", true) orelse return PuppeteerError.InvalidArgument;
        const type_raw = CdpFrameScanner.extractTopLevelField(info_obj, "type", true) orelse "other";
        const title_raw = CdpFrameScanner.extractTopLevelField(info_obj, "title", true) orelse "";
        const url_raw = CdpFrameScanner.extractTopLevelField(info_obj, "url", true) orelse "";
        const attached_raw = CdpFrameScanner.extractTopLevelField(info_obj, "attached", false) orelse "false";
        const ctx_id_raw = CdpFrameScanner.extractTopLevelField(info_obj, "browserContextId", true) orelse "";
        const opener_raw = CdpFrameScanner.extractTopLevelField(info_obj, "openerId", true) orelse "";

        const tid = _.intern(tid_raw)?;
        const title = _.intern(title_raw)?;
        const url = _.intern(url_raw)?;
        const ctx_id = _.intern(ctx_id_raw)?;
        const opener = _.intern(opener_raw)?;
        const kind = parseManagedTargetKind(type_raw);
        const attached = std.mem.eql(u8, attached_raw, "true");

        if (existing := _.targetById(tid)) {
            existing.kind = kind;
            existing.info.target_type = toTargetType(kind);
            existing.info.title = title;
            existing.info.url = url;
            if (attached) existing.info.attached = true;
            if (ctx_id.len > 0) existing.info.browser_context_id = ctx_id;
            if (opener.len > 0) existing.parent_target_id = opener;
            return existing;
        }

        let t = _.allocator.create(ManagedTarget) catch return PuppeteerError.OutOfMemory;
        t.* = ManagedTarget{
            target_id: tid,
            session_id: "",
            parent_target_id: opener,
            kind: kind,
            info: TargetInfo{
                target_id: tid,
                target_type: toTargetType(kind),
                title: title,
                url: url,
                attached: attached,
                browser_context_id: ctx_id,
            },
            waiting_for_debugger: false,
            destroyed: false,
        };
        _.targets_list.append(_.allocator, t) catch {
            _.allocator.destroy(t);
            return PuppeteerError.OutOfMemory;
        };
        return t;
    }

    /// Central CDP event router for Target.* domain events.
    pub fn handleCdpEvent(*_, ev: CdpEvent): void!PuppeteerError {
        if (std.mem.eql(u8, ev.method, "Target.targetCreated")) {
            return _.onTargetCreated(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Target.targetDestroyed")) {
            _.onTargetDestroyed(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Target.targetInfoChanged")) {
            return _.onTargetInfoChanged(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Target.attachedToTarget")) {
            return _.onAttachedToTarget(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Target.detachedFromTarget")) {
            _.onDetachedFromTarget(ev.params_json);
        }
    }

    pub fn onTargetCreated(*_, params_json: []const u8): void!PuppeteerError {
        const info_obj = CdpFrameScanner.extractTopLevelField(params_json, "targetInfo", false) orelse return;
        const t = _.upsertTargetInfo(info_obj)?;
        const _ign_ev = _.emitter.emit("targetcreated", t.target_id);
        * = _ign_ev;
    }

    pub fn onTargetInfoChanged(*_, params_json: []const u8): void!PuppeteerError {
        const info_obj = CdpFrameScanner.extractTopLevelField(params_json, "targetInfo", false) orelse return;
        const t = _.upsertTargetInfo(info_obj)?;
        const _ign_ev = _.emitter.emit("targetchanged", t.target_id);
        * = _ign_ev;
    }

    pub fn onTargetDestroyed(*_, params_json: []const u8) {
        const tid_raw = CdpFrameScanner.extractTopLevelField(params_json, "targetId", true) orelse return;
        if (t := _.targetById(tid_raw)) {
            t.destroyed = true;
            t.info.attached = false;
            const _ign_ev = _.emitter.emit("targetdestroyed", t.target_id);
            * = _ign_ev;
        }
    }

    pub fn onAttachedToTarget(*_, params_json: []const u8): void!PuppeteerError {
        const sid_raw = CdpFrameScanner.extractTopLevelField(params_json, "sessionId", true) orelse return;
        const info_obj = CdpFrameScanner.extractTopLevelField(params_json, "targetInfo", false) orelse return;
        const wait_raw = CdpFrameScanner.extractTopLevelField(params_json, "waitingForDebugger", false) orelse "false";

        const sid = _.intern(sid_raw)?;
        const t = _.upsertTargetInfo(info_obj)?;
        t.session_id = sid;
        t.info.attached = true;
        t.waiting_for_debugger = std.mem.eql(u8, wait_raw, "true");

        const _ign_ev = _.emitter.emit("attachedtotarget", sid);
        * = _ign_ev;
    }

    pub fn onDetachedFromTarget(*_, params_json: []const u8) {
        const sid_raw = CdpFrameScanner.extractTopLevelField(params_json, "sessionId", true) orelse "";
        const tid_raw = CdpFrameScanner.extractTopLevelField(params_json, "targetId", true) orelse "";

        let target_opt: (*ManagedTarget)? = null;
        if (sid_raw.len > 0) {
            target_opt = _.targetBySessionId(sid_raw);
        }
        if (target_opt == null && tid_raw.len > 0) {
            target_opt = _.targetById(tid_raw);
        }
        if (t := target_opt) {
            t.info.attached = false;
            t.session_id = "";
            const _ign_ev = _.emitter.emit("detachedfromtarget", t.target_id);
            * = _ign_ev;
        }
    }
}

// ============================================================================
// Part 2: AccessibilityTreeBuilder (SerializedAXNode recursive tree + ARIA)
// ============================================================================

pub struct SerializedAXNode {
    node_id: []const u8 = "",
    role: []const u8 = "",
    name: []const u8 = "",
    value: []const u8 = "",
    description: []const u8 = "",
    roledescription: []const u8 = "",
    valuetext: []const u8 = "",
    backend_dom_node_id: i64 = 0,
    disabled: bool = false,
    expanded: bool = false,
    focused: bool = false,
    modal: bool = false,
    multiline: bool = false,
    multiselectable: bool = false,
    readonly: bool = false,
    required: bool = false,
    selected: bool = false,
    checked: []const u8 = "",
    pressed: []const u8 = "",
    level: i32 = 0,
    has_level: bool = false,
    children: std.ArrayList(*SerializedAXNode) = .empty,

    pub fn childSlice(): []const *SerializedAXNode {
        return _.children.items;
    }
}

pub struct AXRawNode {
    summary: AXNodeSummary = .{},
    parent_id: []const u8 = "",
    child_ids: std.ArrayList([]const u8) = .empty,
    description: []const u8 = "",
    roledescription: []const u8 = "",
    valuetext: []const u8 = "",
    disabled: bool = false,
    expanded: bool = false,
    focused: bool = false,
    focusable: bool = false,
    hidden: bool = false,
    editable: bool = false,
    modal: bool = false,
    multiline: bool = false,
    multiselectable: bool = false,
    readonly: bool = false,
    required: bool = false,
    selected: bool = false,
    checked: []const u8 = "",
    pressed: []const u8 = "",
    level: i32 = 0,
    has_level: bool = false,
}

pub struct AccessibilityTreeBuilder {
    allocator: std.mem.Allocator = undefined,
    raw_nodes: std.ArrayList(*AXRawNode) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    disposed: bool = false,

    pub static fn init(alloc: std.mem.Allocator): AccessibilityTreeBuilder {
        return AccessibilityTreeBuilder{
            allocator: alloc,
            raw_nodes: .empty,
            owned_strings: .empty,
            disposed: false,
        };
    }

    pub fn deinit(*_) {
        if (_.disposed) return;
        _.disposed = true;

        for (rn of _.raw_nodes.items) {
            rn.child_ids.deinit(_.allocator);
            _.allocator.destroy(rn);
        }
        _.raw_nodes.deinit(_.allocator);
        _.raw_nodes = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PuppeteerError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PuppeteerError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PuppeteerError.OutOfMemory;
        };
        return duped;
    }

    /// Normalize Chrome AX roles to standard Puppeteer ARIA roles.
    pub static fn normalizeAriaRole(raw_role: []const u8): []const u8 {
        if (std.mem.eql(u8, raw_role, "RootWebArea")) return "WebArea";
        if (std.mem.eql(u8, raw_role, "StaticText")) return "text";
        if (std.mem.eql(u8, raw_role, "GenericContainer")) return "generic";
        if (std.mem.eql(u8, raw_role, "Paragraph")) return "paragraph";
        if (std.mem.eql(u8, raw_role, "Heading")) return "heading";
        if (std.mem.eql(u8, raw_role, "Button")) return "button";
        if (std.mem.eql(u8, raw_role, "Link")) return "link";
        if (std.mem.eql(u8, raw_role, "TextField")) return "textbox";
        if (std.mem.eql(u8, raw_role, "CheckBox")) return "checkbox";
        if (std.mem.eql(u8, raw_role, "RadioButton")) return "radio";
        if (std.mem.eql(u8, raw_role, "ComboBoxGrouping")) return "combobox";
        if (std.mem.eql(u8, raw_role, "ListItem")) return "listitem";
        if (std.mem.eql(u8, raw_role, "Image")) return "img";
        if (std.mem.eql(u8, raw_role, "Navigation")) return "navigation";
        return raw_role;
    }

    pub static fn isControlRole(role: []const u8): bool {
        return std.mem.eql(u8, role, "button") ||
            std.mem.eql(u8, role, "checkbox") ||
            std.mem.eql(u8, role, "combobox") ||
            std.mem.eql(u8, role, "link") ||
            std.mem.eql(u8, role, "listbox") ||
            std.mem.eql(u8, role, "menuitem") ||
            std.mem.eql(u8, role, "menuitemcheckbox") ||
            std.mem.eql(u8, role, "menuitemradio") ||
            std.mem.eql(u8, role, "option") ||
            std.mem.eql(u8, role, "radio") ||
            std.mem.eql(u8, role, "searchbox") ||
            std.mem.eql(u8, role, "slider") ||
            std.mem.eql(u8, role, "spinbutton") ||
            std.mem.eql(u8, role, "switch") ||
            std.mem.eql(u8, role, "tab") ||
            std.mem.eql(u8, role, "textbox");
    }

    pub static fn isLandmarkRole(role: []const u8): bool {
        return std.mem.eql(u8, role, "banner") ||
            std.mem.eql(u8, role, "complementary") ||
            std.mem.eql(u8, role, "contentinfo") ||
            std.mem.eql(u8, role, "form") ||
            std.mem.eql(u8, role, "main") ||
            std.mem.eql(u8, role, "navigation") ||
            std.mem.eql(u8, role, "region") ||
            std.mem.eql(u8, role, "search");
    }

    fn findRawNode(node_id: []const u8): (*AXRawNode)? {
        for (rn of _.raw_nodes.items) {
            if (std.mem.eql(u8, rn.summary.node_id, node_id)) return rn;
        }
        return null;
    }

    pub fn rawNodeCount(): usize {
        return _.raw_nodes.items.len;
    }

    /// Add a pre-parsed `AXNodeSummary` directly (e.g. from `AccessibilityDomain.parseAXNodeSummary`).
    pub fn addSummaryNode(*_, summary: AXNodeSummary, parent_id: []const u8): void!PuppeteerError {
        const nid = _.intern(summary.node_id)?;
        const role = _.intern(AccessibilityTreeBuilder.normalizeAriaRole(summary.role))?;
        const name = _.intern(summary.name)?;
        const val = _.intern(summary.value)?;
        const pid = _.intern(parent_id)?;

        let rn = _.allocator.create(AXRawNode) catch return PuppeteerError.OutOfMemory;
        rn.* = AXRawNode{
            summary: AXNodeSummary{
                node_id: nid,
                role: role,
                name: name,
                value: val,
                ignored: summary.ignored,
                backend_dom_node_id: summary.backend_dom_node_id,
            },
            parent_id: pid,
            child_ids: .empty,
        };
        _.raw_nodes.append(_.allocator, rn) catch {
            _.allocator.destroy(rn);
            return PuppeteerError.OutOfMemory;
        };

        if (pid.len > 0) {
            if (parent := _.findRawNode(pid)) {
                parent.child_ids.append(_.allocator, nid) catch return PuppeteerError.OutOfMemory;
            }
        }
    }

    fn parseProperties(*_, rn: *AXRawNode, props_array: []const u8): void!PuppeteerError {
        const trimmed = CdpFrameScanner.trimWs(props_array);
        if (trimmed.len < 2 || trimmed[0] != '[') return;
        let i: usize = 1;
        while (i < trimmed.len) {
            i = CdpFrameScanner.skipWs(trimmed, i);
            if (i >= trimmed.len || trimmed[i] == ']') break;
            if (trimmed[i] == ',') {
                i += 1;
                continue;
            }
            if (trimmed[i] != '{') break;
            const end_obj = CdpFrameScanner.findMatchingContainer(trimmed, i) orelse break;
            const obj = trimmed[i .. end_obj + 1];
            const pname = CdpFrameScanner.extractTopLevelField(obj, "name", true) orelse "";
            const val_obj = CdpFrameScanner.extractTopLevelField(obj, "value", false) orelse "{}";
            const pval = CdpFrameScanner.extractTopLevelField(val_obj, "value", true)
                orelse CdpFrameScanner.extractTopLevelField(val_obj, "value", false)
                orelse "";
            const is_true = std.mem.eql(u8, pval, "true");

            if (std.mem.eql(u8, pname, "disabled")) {
                rn.disabled = is_true;
            } else if (std.mem.eql(u8, pname, "expanded")) {
                rn.expanded = is_true;
            } else if (std.mem.eql(u8, pname, "focused")) {
                rn.focused = is_true;
            } else if (std.mem.eql(u8, pname, "focusable")) {
                rn.focusable = is_true;
            } else if (std.mem.eql(u8, pname, "hidden")) {
                rn.hidden = is_true;
            } else if (std.mem.eql(u8, pname, "editable")) {
                rn.editable = pval.len > 0 && !std.mem.eql(u8, pval, "false");
            } else if (std.mem.eql(u8, pname, "modal")) {
                rn.modal = is_true;
            } else if (std.mem.eql(u8, pname, "multiline")) {
                rn.multiline = is_true;
            } else if (std.mem.eql(u8, pname, "multiselectable")) {
                rn.multiselectable = is_true;
            } else if (std.mem.eql(u8, pname, "readonly")) {
                rn.readonly = is_true;
            } else if (std.mem.eql(u8, pname, "required")) {
                rn.required = is_true;
            } else if (std.mem.eql(u8, pname, "selected")) {
                rn.selected = is_true;
            } else if (std.mem.eql(u8, pname, "checked")) {
                rn.checked = _.intern(pval)?;
            } else if (std.mem.eql(u8, pname, "pressed")) {
                rn.pressed = _.intern(pval)?;
            } else if (std.mem.eql(u8, pname, "level")) {
                if (lvl := CdpFrameScanner.parseFastI64(pval)) {
                    rn.level = lvl as i32;
                    rn.has_level = true;
                }
            } else if (std.mem.eql(u8, pname, "roledescription")) {
                rn.roledescription = _.intern(pval)?;
            } else if (std.mem.eql(u8, pname, "valuetext")) {
                rn.valuetext = _.intern(pval)?;
            }
            i = end_obj + 1;
        }
    }

    fn parseChildIds(*_, rn: *AXRawNode, arr_json: []const u8): void!PuppeteerError {
        const trimmed = CdpFrameScanner.trimWs(arr_json);
        if (trimmed.len < 2 || trimmed[0] != '[') return;
        let i: usize = 1;
        while (i < trimmed.len) {
            i = CdpFrameScanner.skipWs(trimmed, i);
            if (i >= trimmed.len || trimmed[i] == ']') break;
            if (trimmed[i] == ',') {
                i += 1;
                continue;
            }
            if (trimmed[i] == '"') {
                const end_q = CdpFrameScanner.scanStringEnd(trimmed, i) orelse break;
                const cid = _.intern(trimmed[i + 1 .. end_q])?;
                rn.child_ids.append(_.allocator, cid) catch return PuppeteerError.OutOfMemory;
                i = end_q + 1;
            } else {
                break;
            }
        }
    }

    /// Parse a CDP `Accessibility.getFullAXTree` `nodes` JSON array into raw AX nodes.
    pub fn parseAXNodesJson(*_, nodes_array_json: []const u8): usize!PuppeteerError {
        const trimmed = CdpFrameScanner.trimWs(nodes_array_json);
        if (trimmed.len < 2 || trimmed[0] != '[') return 0;

        let parsed_count: usize = 0;
        let i: usize = 1;
        while (i < trimmed.len) {
            i = CdpFrameScanner.skipWs(trimmed, i);
            if (i >= trimmed.len || trimmed[i] == ']') break;
            if (trimmed[i] == ',') {
                i += 1;
                continue;
            }
            if (trimmed[i] != '{') break;
            const end_obj = CdpFrameScanner.findMatchingContainer(trimmed, i) orelse break;
            const node_json = trimmed[i .. end_obj + 1];

            const sum = AccessibilityDomain.parseAXNodeSummary(node_json);
            const nid = _.intern(sum.node_id)?;
            const role = _.intern(AccessibilityTreeBuilder.normalizeAriaRole(sum.role))?;
            const name = _.intern(sum.name)?;
            const val = _.intern(sum.value)?;
            const pid_raw = CdpFrameScanner.extractTopLevelField(node_json, "parentId", true) orelse "";
            const pid = _.intern(pid_raw)?;

            let desc: []const u8 = "";
            if (desc_obj := CdpFrameScanner.extractTopLevelField(node_json, "description", false)) {
                const d_raw = CdpFrameScanner.extractTopLevelField(desc_obj, "value", true) orelse "";
                desc = _.intern(d_raw)?;
            }

            let rn = _.allocator.create(AXRawNode) catch return PuppeteerError.OutOfMemory;
            rn.* = AXRawNode{
                summary: AXNodeSummary{
                    node_id: nid,
                    role: role,
                    name: name,
                    value: val,
                    ignored: sum.ignored,
                    backend_dom_node_id: sum.backend_dom_node_id,
                },
                parent_id: pid,
                child_ids: .empty,
                description: desc,
            };

            if (cids_json := CdpFrameScanner.extractTopLevelField(node_json, "childIds", false)) {
                _.parseChildIds(rn, cids_json)?;
            }
            if (props_json := CdpFrameScanner.extractTopLevelField(node_json, "properties", false)) {
                _.parseProperties(rn, props_json)?;
            }

            _.raw_nodes.append(_.allocator, rn) catch {
                rn.child_ids.deinit(_.allocator);
                _.allocator.destroy(rn);
                return PuppeteerError.OutOfMemory;
            };
            parsed_count += 1;
            i = end_obj + 1;
        }
        return parsed_count;
    }

    /// Puppeteer `AXNode.isInteresting(insideControl)` filtering heuristic.
    pub fn isInterestingNode(rn: *AXRawNode, inside_control: bool): bool {
        const role = rn.summary.role;
        if (rn.summary.ignored || rn.hidden) return false;
        if (std.mem.eql(u8, role, "WebArea")) return true;
        if (rn.focusable || rn.editable || rn.modal) return true;
        if (AccessibilityTreeBuilder.isControlRole(role)) return true;
        if (AccessibilityTreeBuilder.isLandmarkRole(role)) return true;
        if (std.mem.eql(u8, role, "heading") || std.mem.eql(u8, role, "img") || std.mem.eql(u8, role, "dialog")) {
            return true;
        }
        if (inside_control && std.mem.eql(u8, role, "text")) return false;
        if (std.mem.eql(u8, role, "generic") || std.mem.eql(u8, role, "none") ||
            std.mem.eql(u8, role, "InlineTextBox") || std.mem.eql(u8, role, "LineBreak"))
        {
            return false;
        }
        if (std.mem.eql(u8, role, "text")) {
            return TargetSimdScanner.hasNonWhitespaceSimd(rn.summary.name);
        }
        return TargetSimdScanner.hasNonWhitespaceSimd(rn.summary.name) || rn.summary.value.len > 0;
    }

    fn serializeSingle(*_, rn: *AXRawNode): (*SerializedAXNode)!PuppeteerError {
        let node = _.allocator.create(SerializedAXNode) catch return PuppeteerError.OutOfMemory;
        node.* = SerializedAXNode{
            node_id: rn.summary.node_id,
            role: rn.summary.role,
            name: rn.summary.name,
            value: rn.summary.value,
            description: rn.description,
            roledescription: rn.roledescription,
            valuetext: rn.valuetext,
            backend_dom_node_id: rn.summary.backend_dom_node_id,
            disabled: rn.disabled,
            expanded: rn.expanded,
            focused: rn.focused,
            modal: rn.modal,
            multiline: rn.multiline,
            multiselectable: rn.multiselectable,
            readonly: rn.readonly,
            required: rn.required,
            selected: rn.selected,
            checked: rn.checked,
            pressed: rn.pressed,
            level: rn.level,
            has_level: rn.has_level,
            children: .empty,
        };
        return node;
    }

    fn collectRecursive(
        *_,
        rn: *AXRawNode,
        interesting_only: bool,
        inside_control: bool,
        out_list: *std.ArrayList(*SerializedAXNode),
    ): void!PuppeteerError {
        const is_ctrl = inside_control || AccessibilityTreeBuilder.isControlRole(rn.summary.role);
        const keep = !interesting_only || _.isInterestingNode(rn, inside_control);

        if (keep) {
            const ser = _.serializeSingle(rn)?;
            for (cid of rn.child_ids.items) {
                if (child_rn := _.findRawNode(cid)) {
                    _.collectRecursive(child_rn, interesting_only, is_ctrl, *ser.children) catch |e| {
                        AccessibilityTreeBuilder.destroyTree(_.allocator, ser);
                        return e;
                    };
                }
            }
            out_list.append(_.allocator, ser) catch {
                AccessibilityTreeBuilder.destroyTree(_.allocator, ser);
                return PuppeteerError.OutOfMemory;
            };
        } else {
            // Uninteresting node: promote interesting descendants into parent's children list
            for (cid of rn.child_ids.items) {
                if (child_rn := _.findRawNode(cid)) {
                    _.collectRecursive(child_rn, interesting_only, is_ctrl, out_list)?;
                }
            }
        }
    }

    /// Build the complete recursive `SerializedAXNode` tree from parsed raw nodes.
    /// Caller must free the returned root via `AccessibilityTreeBuilder.destroyTree(alloc, root)`.
    pub fn buildTree(*_, interesting_only: bool): ((*SerializedAXNode)?)!PuppeteerError {
        if (_.raw_nodes.items.len == 0) return null;
        const root_rn = _.raw_nodes.items[0];

        let top_list: std.ArrayList(*SerializedAXNode) = .empty;
        defer top_list.deinit(_.allocator);

        _.collectRecursive(root_rn, interesting_only, false, *top_list) catch |e| {
            for (n of top_list.items) {
                AccessibilityTreeBuilder.destroyTree(_.allocator, n);
            }
            return e;
        };

        if (top_list.items.len == 0) return null;
        const root = top_list.items[0];
        let i: usize = 1;
        while (i < top_list.items.len) {
            AccessibilityTreeBuilder.destroyTree(_.allocator, top_list.items[i]);
            i += 1;
        }
        return root;
    }

    /// Recursively free a `SerializedAXNode` tree (zero-leak RAII helper).
    pub static fn destroyTree(alloc: std.mem.Allocator, node: *SerializedAXNode) {
        for (c of node.children.items) {
            AccessibilityTreeBuilder.destroyTree(alloc, c);
        }
        node.children.deinit(alloc);
        alloc.destroy(node);
    }

    /// Count total nodes in a `SerializedAXNode` tree.
    pub static fn countTreeNodes(node: *SerializedAXNode): usize {
        let total: usize = 1;
        for (c of node.children.items) {
            total += AccessibilityTreeBuilder.countTreeNodes(c);
        }
        return total;
    }
}
