// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/bidi/core/BrowsingContext.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/bidi/context.cp — BidiBrowsingContext, BidiNavigation,
// BidiFrame, BidiRealm & BidiDialog
// Implements browsingContext.* (create/navigate/reload/close/captureScreenshot/
// print/setViewport/locateNodes/handleUserPrompt/activate/traverseHistory/getTree)
// and script.* (evaluate/callFunction/addPreloadScript/removePreloadScript/disown).
// ============================================================================

const std = @import("std");
import {
    BidiError,
    CreateContextType,
    ReadinessState,
    UserPromptType,
    NavigateResult,
    CaptureScreenshotOrigin,
    CaptureScreenshotParams,
    PrintOrientation,
    PrintParams,
    SetViewportParams,
    LocatorType,
    BidiLocator,
    LocateNodesParams,
    HandleUserPromptParams,
    RealmType,
    ResultOwnership,
    EvaluateResultKind,
    BidiRemoteValue,
    BidiExceptionDetails,
    EvaluateResult,
    createContextTypeName,
    readinessStateName,
    locatorTypeName,
    resultOwnershipName,
    parseUserPromptType,
    bidiWriteStr,
    bidiWriteJsonEscaped,
} from "./protocol";
import { BidiFrameScanner } from "./connection";
import { BidiSession } from "./session";

// ============================================================================
// BidiNavigation — Navigation Lifecycle State Tracker
// ============================================================================

pub enum BidiNavigationState {
    Pending,
    Started,
    Committed,
    DomContentLoaded,
    Loaded,
    FragmentNavigated,
    Failed,
    Aborted,
}

pub struct BidiNavigation {
    navigation_id_buf: [128]u8 = undefined,
    navigation_id_len: usize = 0,
    context_id_buf: [128]u8 = undefined,
    context_id_len: usize = 0,
    url_buf: [512]u8 = undefined,
    url_len: usize = 0,
    pub state: BidiNavigationState = BidiNavigationState.Pending,
    pub timestamp: i64 = 0,

    pub static fn init(context_id: []const u8, navigation_id: []const u8, nav_url: []const u8): BidiNavigation {
        let nav = BidiNavigation{
            navigation_id_len: 0,
            context_id_len: 0,
            url_len: 0,
            state: BidiNavigationState.Pending,
            timestamp: 0,
        };
        nav.setContextId(context_id);
        nav.setNavigationId(navigation_id);
        nav.setUrl(nav_url);
        return nav;
    }

    pub fn id(): []const u8 {
        return _.navigation_id_buf[0.._.navigation_id_len];
    }

    pub fn contextId(): []const u8 {
        return _.context_id_buf[0.._.context_id_len];
    }

    pub fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    fn setContextId(*_, s: []const u8) {
        const n = if (s.len <= 128) s.len else 128;
        if (n > 0) @memcpy(_.context_id_buf[0..n], s[0..n]);
        _.context_id_len = n;
    }

    fn setNavigationId(*_, s: []const u8) {
        const n = if (s.len <= 128) s.len else 128;
        if (n > 0) @memcpy(_.navigation_id_buf[0..n], s[0..n]);
        _.navigation_id_len = n;
    }

    fn setUrl(*_, s: []const u8) {
        const n = if (s.len <= 512) s.len else 512;
        if (n > 0) @memcpy(_.url_buf[0..n], s[0..n]);
        _.url_len = n;
    }

    pub fn applyEvent(*_, event_method: []const u8, params_json: []const u8) {
        if (u := BidiFrameScanner.extractField(params_json, "url", true)) {
            _.setUrl(u);
        }
        if (nid := BidiFrameScanner.extractField(params_json, "navigation", true)) {
            _.setNavigationId(nid);
        }
        if (ts_raw := BidiFrameScanner.extractField(params_json, "timestamp", false)) {
            if (ts := BidiFrameScanner.parseI64(ts_raw)) {
                _.timestamp = ts;
            }
        }
        if (std.mem.eql(u8, event_method, "browsingContext.navigationStarted")) {
            _.state = BidiNavigationState.Started;
        } else if (std.mem.eql(u8, event_method, "browsingContext.navigationCommitted")) {
            _.state = BidiNavigationState.Committed;
        } else if (std.mem.eql(u8, event_method, "browsingContext.domContentLoaded")) {
            _.state = BidiNavigationState.DomContentLoaded;
        } else if (std.mem.eql(u8, event_method, "browsingContext.load")) {
            _.state = BidiNavigationState.Loaded;
        } else if (std.mem.eql(u8, event_method, "browsingContext.fragmentNavigated")) {
            _.state = BidiNavigationState.FragmentNavigated;
        } else if (std.mem.eql(u8, event_method, "browsingContext.navigationFailed")) {
            _.state = BidiNavigationState.Failed;
        } else if (std.mem.eql(u8, event_method, "browsingContext.navigationAborted")) {
            _.state = BidiNavigationState.Aborted;
        }
    }
}

// ============================================================================
// BidiDialog — User Prompt Wrapper (alert / confirm / prompt / beforeunload)
// ============================================================================

pub struct BidiDialog {
    session: *BidiSession = undefined,
    context_id_buf: [128]u8 = undefined,
    context_id_len: usize = 0,
    message_buf: [512]u8 = undefined,
    message_len: usize = 0,
    pub prompt_type: UserPromptType = UserPromptType.Alert,
    pub handled: bool = false,

    pub static fn init(
        session: *BidiSession,
        context_id: []const u8,
        prompt_type: UserPromptType,
        prompt_msg: []const u8,
    ): BidiDialog {
        let d = BidiDialog{
            session: session,
            context_id_len: 0,
            message_len: 0,
            prompt_type: prompt_type,
            handled: false,
        };
        const cn = if (context_id.len <= 128) context_id.len else 128;
        if (cn > 0) @memcpy(d.context_id_buf[0..cn], context_id[0..cn]);
        d.context_id_len = cn;
        const mn = if (prompt_msg.len <= 512) prompt_msg.len else 512;
        if (mn > 0) @memcpy(d.message_buf[0..mn], prompt_msg[0..mn]);
        d.message_len = mn;
        return d;
    }

    pub fn message(): []const u8 {
        return _.message_buf[0.._.message_len];
    }

    pub fn accept(*_, user_text: []const u8): void!BidiError {
        if (_.handled) return;
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.context_id_buf[0.._.context_id_len])?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"accept\":true")?;
        if (user_text.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"userText\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, user_text)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1536]u8 = undefined;
        * = _.session.sendCommand("browsingContext.handleUserPrompt", pbuf[0..pos], &frame_buf)?;
        _.handled = true;
    }

    pub fn dismiss(*_): void!BidiError {
        if (_.handled) return;
        let pbuf: [512]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.context_id_buf[0.._.context_id_len])?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"accept\":false}")?;

        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("browsingContext.handleUserPrompt", pbuf[0..pos], &frame_buf)?;
        _.handled = true;
    }
}

// ============================================================================
// BidiRealm — Script Execution Realm (Window / Sandbox / Worker)
// ============================================================================

pub struct BidiRealm {
    session: *BidiSession = undefined,
    realm_id_buf: [128]u8 = undefined,
    realm_id_len: usize = 0,
    context_id_buf: [128]u8 = undefined,
    context_id_len: usize = 0,
    origin_buf: [256]u8 = undefined,
    origin_len: usize = 0,
    sandbox_buf: [128]u8 = undefined,
    sandbox_len: usize = 0,
    pub realm_type: RealmType = RealmType.Window,
    pub disposed: bool = false,

    pub static fn init(
        session: *BidiSession,
        context_id: []const u8,
        realm_id: []const u8,
        realm_origin: []const u8,
        realm_sandbox: []const u8,
        realm_type: RealmType,
    ): BidiRealm {
        let r = BidiRealm{
            session: session,
            realm_id_len: 0,
            context_id_len: 0,
            origin_len: 0,
            sandbox_len: 0,
            realm_type: realm_type,
            disposed: false,
        };
        const rn = if (realm_id.len <= 128) realm_id.len else 128;
        if (rn > 0) @memcpy(r.realm_id_buf[0..rn], realm_id[0..rn]);
        r.realm_id_len = rn;
        const cn = if (context_id.len <= 128) context_id.len else 128;
        if (cn > 0) @memcpy(r.context_id_buf[0..cn], context_id[0..cn]);
        r.context_id_len = cn;
        const on = if (realm_origin.len <= 256) realm_origin.len else 256;
        if (on > 0) @memcpy(r.origin_buf[0..on], realm_origin[0..on]);
        r.origin_len = on;
        const sn = if (realm_sandbox.len <= 128) realm_sandbox.len else 128;
        if (sn > 0) @memcpy(r.sandbox_buf[0..sn], realm_sandbox[0..sn]);
        r.sandbox_len = sn;
        return r;
    }

    pub fn id(): []const u8 {
        return _.realm_id_buf[0.._.realm_id_len];
    }

    pub fn contextId(): []const u8 {
        return _.context_id_buf[0.._.context_id_len];
    }

    pub fn origin(): []const u8 {
        return _.origin_buf[0.._.origin_len];
    }

    pub fn sandbox(): []const u8 {
        return _.sandbox_buf[0.._.sandbox_len];
    }

    fn writeTargetJson(buf: []u8, pos: usize): usize!BidiError {
        let p = pos;
        p = bidiWriteStr(buf, p, "\"target\":{")?;
        if (_.realm_id_len > 0 && _.context_id_len == 0) {
            p = bidiWriteStr(buf, p, "\"realm\":\"")?;
            p = bidiWriteJsonEscaped(buf, p, _.id())?;
            p = bidiWriteStr(buf, p, "\"")?;
        } else {
            p = bidiWriteStr(buf, p, "\"context\":\"")?;
            p = bidiWriteJsonEscaped(buf, p, _.contextId())?;
            p = bidiWriteStr(buf, p, "\"")?;
            if (_.sandbox_len > 0) {
                p = bidiWriteStr(buf, p, ",\"sandbox\":\"")?;
                p = bidiWriteJsonEscaped(buf, p, _.sandbox())?;
                p = bidiWriteStr(buf, p, "\"")?;
            }
        }
        p = bidiWriteStr(buf, p, "}")?;
        return p;
    }

    pub static fn parseEvaluateResult(result_json: []const u8): EvaluateResult {
        const res_type = BidiFrameScanner.extractField(result_json, "type", true) orelse "success";
        const realm = BidiFrameScanner.extractField(result_json, "realm", true) orelse "";

        if (std.mem.eql(u8, res_type, "exception")) {
            let ex_text: []const u8 = "";
            let line_no: i64 = 0;
            let col_no: i64 = 0;
            if (ex_obj := BidiFrameScanner.extractField(result_json, "exceptionDetails", false)) {
                ex_text = BidiFrameScanner.extractField(ex_obj, "text", true) orelse "";
                if (lr := BidiFrameScanner.extractField(ex_obj, "lineNumber", false)) {
                    line_no = BidiFrameScanner.parseI64(lr) orelse 0;
                }
                if (cr := BidiFrameScanner.extractField(ex_obj, "columnNumber", false)) {
                    col_no = BidiFrameScanner.parseI64(cr) orelse 0;
                }
            }
            return EvaluateResult{
                kind: EvaluateResultKind.Exception,
                realm: realm,
                exception: BidiExceptionDetails{
                    text: ex_text,
                    line_number: line_no,
                    column_number: col_no,
                },
            };
        }

        let rv = BidiRemoteValue{};
        if (val_obj := BidiFrameScanner.extractField(result_json, "result", false)) {
            rv.value_type = BidiFrameScanner.extractField(val_obj, "type", true) orelse "undefined";
            rv.value_raw = BidiFrameScanner.extractField(val_obj, "value", true) orelse "";
            rv.handle = BidiFrameScanner.extractField(val_obj, "handle", true) orelse "";
            rv.shared_id = BidiFrameScanner.extractField(val_obj, "sharedId", true) orelse "";
        }
        return EvaluateResult{
            kind: EvaluateResultKind.Success,
            realm: realm,
            result: rv,
        };
    }

    /// Execute `script.evaluate` inside this realm.
    pub fn evaluate(
        *_,
        expression: []const u8,
        await_promise: bool,
        ownership: ResultOwnership,
    ): EvaluateResult!BidiError {
        if (_.disposed) return BidiError.ContextClosed;

        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"expression\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, expression)?;
        pos = bidiWriteStr(&pbuf, pos, "\",")?;
        pos = _.writeTargetJson(&pbuf, pos)?;
        pos = bidiWriteStr(&pbuf, pos, ",\"awaitPromise\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (await_promise) "true" else "false")?;
        pos = bidiWriteStr(&pbuf, pos, ",\"resultOwnership\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, resultOwnershipName(ownership))?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [2560]u8 = undefined;
        const resp = _.session.sendCommand("script.evaluate", pbuf[0..pos], &frame_buf)?;
        return BidiRealm.parseEvaluateResult(resp.result_json);
    }

    /// Execute `script.callFunction` inside this realm.
    pub fn callFunction(
        *_,
        function_declaration: []const u8,
        arguments_json: []const u8,
        await_promise: bool,
        ownership: ResultOwnership,
    ): EvaluateResult!BidiError {
        if (_.disposed) return BidiError.ContextClosed;

        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"functionDeclaration\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, function_declaration)?;
        pos = bidiWriteStr(&pbuf, pos, "\",")?;
        pos = _.writeTargetJson(&pbuf, pos)?;
        pos = bidiWriteStr(&pbuf, pos, ",\"arguments\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (arguments_json.len > 0) arguments_json else "[]")?;
        pos = bidiWriteStr(&pbuf, pos, ",\"awaitPromise\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (await_promise) "true" else "false")?;
        pos = bidiWriteStr(&pbuf, pos, ",\"resultOwnership\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, resultOwnershipName(ownership))?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [2560]u8 = undefined;
        const resp = _.session.sendCommand("script.callFunction", pbuf[0..pos], &frame_buf)?;
        return BidiRealm.parseEvaluateResult(resp.result_json);
    }

    /// Release owned remote handles via `script.disown`.
    pub fn disown(*_, handles: []const []const u8): void!BidiError {
        if (_.disposed) return BidiError.ContextClosed;

        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"handles\":[")?;
        let i: usize = 0;
        while (i < handles.len) {
            if (i > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, handles[i])?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            i += 1;
        }
        pos = bidiWriteStr(&pbuf, pos, "],")?;
        pos = _.writeTargetJson(&pbuf, pos)?;
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1536]u8 = undefined;
        * = _.session.sendCommand("script.disown", pbuf[0..pos], &frame_buf)?;
    }
}

// ============================================================================
// BidiBrowsingContext — Core Tab / Frame Control Surface
// ============================================================================

pub struct BidiBrowsingContext {
    session: *BidiSession = undefined,
    id_buf: [128]u8 = undefined,
    id_len: usize = 0,
    parent_id_buf: [128]u8 = undefined,
    parent_id_len: usize = 0,
    user_context_buf: [128]u8 = undefined,
    user_context_len: usize = 0,
    url_buf: [512]u8 = undefined,
    url_len: usize = 0,
    pub closed: bool = false,

    pub static fn init(
        session: *BidiSession,
        context_id: []const u8,
        user_context: []const u8,
        parent_id: []const u8,
        initial_url: []const u8,
    ): BidiBrowsingContext {
        let ctx = BidiBrowsingContext{
            session: session,
            id_len: 0,
            parent_id_len: 0,
            user_context_len: 0,
            url_len: 0,
            closed: false,
        };
        const n = if (context_id.len <= 128) context_id.len else 128;
        if (n > 0) @memcpy(ctx.id_buf[0..n], context_id[0..n]);
        ctx.id_len = n;
        const pn = if (parent_id.len <= 128) parent_id.len else 128;
        if (pn > 0) @memcpy(ctx.parent_id_buf[0..pn], parent_id[0..pn]);
        ctx.parent_id_len = pn;
        const uc = if (user_context.len > 0) user_context else "default";
        const un = if (uc.len <= 128) uc.len else 128;
        if (un > 0) @memcpy(ctx.user_context_buf[0..un], uc[0..un]);
        ctx.user_context_len = un;
        ctx.setUrl(if (initial_url.len > 0) initial_url else "about:blank");
        return ctx;
    }

    /// Create a new browsing context via `browsingContext.create`.
    pub static fn create(
        session: *BidiSession,
        ctx_type: CreateContextType,
        user_context: []const u8,
        background: bool,
    ): BidiBrowsingContext!BidiError {
        let pbuf: [512]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"type\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, createContextTypeName(ctx_type))?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"background\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (background) "true" else "false")?;
        if (user_context.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"userContext\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, user_context)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1024]u8 = undefined;
        const resp = session.sendCommand("browsingContext.create", pbuf[0..pos], &frame_buf)?;
        const cid = BidiFrameScanner.extractField(resp.result_json, "context", true) orelse return BidiError.InvalidJson;
        return BidiBrowsingContext.init(session, cid, user_context, "", "about:blank");
    }

    pub fn id(): []const u8 {
        return _.id_buf[0.._.id_len];
    }

    pub fn parentId(): []const u8 {
        return _.parent_id_buf[0.._.parent_id_len];
    }

    pub fn userContextId(): []const u8 {
        return _.user_context_buf[0.._.user_context_len];
    }

    pub fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    fn setUrl(*_, new_url: []const u8) {
        const n = if (new_url.len <= 512) new_url.len else 512;
        if (n > 0) @memcpy(_.url_buf[0..n], new_url[0..n]);
        _.url_len = n;
    }

    pub fn defaultRealm(): BidiRealm {
        return BidiRealm.init(_.session, _.id(), "", "", "", RealmType.Window);
    }

    pub fn sandboxRealm(sandbox_name: []const u8): BidiRealm {
        return BidiRealm.init(_.session, _.id(), "", "", sandbox_name, RealmType.Window);
    }

    /// Execute `browsingContext.navigate` and return a tracked `BidiNavigation`.
    pub fn navigate(*_, target_url: []const u8, wait: ReadinessState): BidiNavigation!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"url\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, target_url)?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"wait\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, readinessStateName(wait))?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [1536]u8 = undefined;
        const resp = _.session.sendCommand("browsingContext.navigate", pbuf[0..pos], &frame_buf)?;
        const nav_id = BidiFrameScanner.extractField(resp.result_json, "navigation", true) orelse "";
        const final_url = BidiFrameScanner.extractField(resp.result_json, "url", true) orelse target_url;
        _.setUrl(final_url);

        let nav = BidiNavigation.init(_.id(), nav_id, final_url);
        nav.state = BidiNavigationState.Loaded;
        return nav;
    }

    /// Execute `browsingContext.reload`.
    pub fn reload(*_, ignore_cache: bool, wait: ReadinessState): NavigateResult!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [512]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"ignoreCache\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (ignore_cache) "true" else "false")?;
        pos = bidiWriteStr(&pbuf, pos, ",\"wait\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, readinessStateName(wait))?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [1024]u8 = undefined;
        const resp = _.session.sendCommand("browsingContext.reload", pbuf[0..pos], &frame_buf)?;
        const nav_id = BidiFrameScanner.extractField(resp.result_json, "navigation", true) orelse "";
        const final_url = BidiFrameScanner.extractField(resp.result_json, "url", true) orelse _.url();
        return NavigateResult{ navigation: nav_id, url: final_url };
    }

    /// Execute `browsingContext.activate`.
    pub fn activate(*_): void!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [256]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;
        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("browsingContext.activate", pbuf[0..pos], &frame_buf)?;
    }

    /// Execute `browsingContext.traverseHistory`.
    pub fn traverseHistory(*_, delta: i32): void!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [256]u8 = undefined;
        const s = std.fmt.bufPrint(&pbuf, "{{\"context\":\"{s}\",\"delta\":{d}}}", .{ _.id(), delta }) catch return BidiError.BufferTooSmall;
        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("browsingContext.traverseHistory", s, &frame_buf)?;
    }

    /// Execute `browsingContext.captureScreenshot`, returning base64 PNG/JPEG data.
    pub fn captureScreenshot(*_, params: CaptureScreenshotParams): ([]const u8)!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"origin\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, if (params.origin == CaptureScreenshotOrigin.Document) "document" else "viewport")?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"format\":{\"type\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, if (params.format_type.len > 0) params.format_type else "image/png")?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;
        if (params.clip.enabled) {
            let cbuf: [256]u8 = undefined;
            const cs = std.fmt.bufPrint(
                &cbuf,
                ",\"clip\":{{\"type\":\"box\",\"x\":{d:.2},\"y\":{d:.2},\"width\":{d:.2},\"height\":{d:.2}}}",
                .{ params.clip.x, params.clip.y, params.clip.width, params.clip.height },
            ) catch return BidiError.BufferTooSmall;
            pos = bidiWriteStr(&pbuf, pos, cs)?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1536]u8 = undefined;
        const resp = _.session.sendCommand("browsingContext.captureScreenshot", pbuf[0..pos], &frame_buf)?;
        return BidiFrameScanner.extractField(resp.result_json, "data", true) orelse return BidiError.InvalidJson;
    }

    /// Execute `browsingContext.print`, returning base64 PDF data.
    pub fn print(*_, params: PrintParams): ([]const u8)!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [1024]u8 = undefined;
        const orient = if (params.orientation == PrintOrientation.Landscape) "landscape" else "portrait";
        const s = std.fmt.bufPrint(
            &pbuf,
            "{{\"context\":\"{s}\",\"background\":{},\"orientation\":\"{s}\",\"scale\":{d:.2},\"shrinkToFit\":{},\"margin\":{{\"top\":{d:.2},\"bottom\":{d:.2},\"left\":{d:.2},\"right\":{d:.2}}},\"page\":{{\"width\":{d:.2},\"height\":{d:.2}}}}}",
            .{
                _.id(),
                params.background,
                orient,
                params.scale,
                params.shrink_to_fit,
                params.margin.top,
                params.margin.bottom,
                params.margin.left,
                params.margin.right,
                params.page.width,
                params.page.height,
            },
        ) catch return BidiError.BufferTooSmall;

        let frame_buf: [1536]u8 = undefined;
        const resp = _.session.sendCommand("browsingContext.print", s, &frame_buf)?;
        return BidiFrameScanner.extractField(resp.result_json, "data", true) orelse return BidiError.InvalidJson;
    }

    /// Execute `browsingContext.setViewport`.
    pub fn setViewport(*_, params: SetViewportParams): void!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [512]u8 = undefined;
        let s: []const u8 = "";
        if (params.reset_viewport) {
            s = std.fmt.bufPrint(
                &pbuf,
                "{{\"context\":\"{s}\",\"viewport\":null,\"devicePixelRatio\":null}}",
                .{_.id()},
            ) catch return BidiError.BufferTooSmall;
        } else {
            s = std.fmt.bufPrint(
                &pbuf,
                "{{\"context\":\"{s}\",\"viewport\":{{\"width\":{d},\"height\":{d}}},\"devicePixelRatio\":{d:.2}}}",
                .{ _.id(), params.width, params.height, params.device_pixel_ratio },
            ) catch return BidiError.BufferTooSmall;
        }
        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("browsingContext.setViewport", s, &frame_buf)?;
    }

    /// Execute `browsingContext.locateNodes` and return raw `nodes` JSON array slice.
    pub fn locateNodes(*_, locator: BidiLocator, max_node_count: u32): ([]const u8)!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"locator\":{\"type\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, locatorTypeName(locator.locator_type))?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"value\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, locator.value)?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;
        if (max_node_count > 0) {
            let mbuf: [64]u8 = undefined;
            const ms = std.fmt.bufPrint(&mbuf, ",\"maxNodeCount\":{d}", .{max_node_count}) catch return BidiError.BufferTooSmall;
            pos = bidiWriteStr(&pbuf, pos, ms)?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1536]u8 = undefined;
        const resp = _.session.sendCommand("browsingContext.locateNodes", pbuf[0..pos], &frame_buf)?;
        return BidiFrameScanner.extractField(resp.result_json, "nodes", false) orelse "[]";
    }

    /// Execute `browsingContext.handleUserPrompt`.
    pub fn handleUserPrompt(*_, accept: bool, user_text: []const u8): void!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"accept\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (accept) "true" else "false")?;
        if (user_text.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"userText\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, user_text)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1536]u8 = undefined;
        * = _.session.sendCommand("browsingContext.handleUserPrompt", pbuf[0..pos], &frame_buf)?;
    }

    /// Execute `script.evaluate` in this browsing context's default window realm.
    pub fn evaluate(*_, expression: []const u8, await_promise: bool): EvaluateResult!BidiError {
        let realm = _.defaultRealm();
        return realm.evaluate(expression, await_promise, ResultOwnership.None);
    }

    /// Execute `script.callFunction` in this browsing context's default window realm.
    pub fn callFunction(*_, fn_decl: []const u8, args_json: []const u8, await_promise: bool): EvaluateResult!BidiError {
        let realm = _.defaultRealm();
        return realm.callFunction(fn_decl, args_json, await_promise, ResultOwnership.None);
    }

    /// Register a preload script for this context via `script.addPreloadScript`, returning script ID.
    pub fn addPreloadScript(*_, function_declaration: []const u8, sandbox: []const u8): ([]const u8)!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"functionDeclaration\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, function_declaration)?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"contexts\":[\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\"]")?;
        if (sandbox.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"sandbox\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, sandbox)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [2560]u8 = undefined;
        const resp = _.session.sendCommand("script.addPreloadScript", pbuf[0..pos], &frame_buf)?;
        return BidiFrameScanner.extractField(resp.result_json, "script", true) orelse return BidiError.InvalidJson;
    }

    /// Release owned script handles in this context via `script.disown`.
    pub fn disown(*_, handles: []const []const u8): void!BidiError {
        let realm = _.defaultRealm();
        return realm.disown(handles);
    }

    /// Close this browsing context via `browsingContext.close`.
    pub fn close(*_, prompt_unload: bool): void!BidiError {
        if (_.closed) return;
        let pbuf: [256]u8 = undefined;
        const s = std.fmt.bufPrint(
            &pbuf,
            "{{\"context\":\"{s}\",\"promptUnload\":{}}}",
            .{ _.id(), prompt_unload },
        ) catch return BidiError.BufferTooSmall;
        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("browsingContext.close", s, &frame_buf)?;
        _.closed = true;
    }
}

// ============================================================================
// BidiFrame — High-Level Puppeteer Frame Facade over BidiBrowsingContext
// ============================================================================

pub struct BidiFrame {
    context: *BidiBrowsingContext = undefined,
    name_buf: [128]u8 = undefined,
    name_len: usize = 0,

    pub static fn init(ctx: *BidiBrowsingContext, frame_name: []const u8): BidiFrame {
        let f = BidiFrame{
            context: ctx,
            name_len: 0,
        };
        const n = if (frame_name.len <= 128) frame_name.len else 128;
        if (n > 0) @memcpy(f.name_buf[0..n], frame_name[0..n]);
        f.name_len = n;
        return f;
    }

    pub fn id(): []const u8 {
        return _.context.id();
    }

    pub fn url(): []const u8 {
        return _.context.url();
    }

    pub fn name(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    pub fn detached(): bool {
        return _.context.closed;
    }

    pub fn goto(*_, target_url: []const u8, wait: ReadinessState): BidiNavigation!BidiError {
        return _.context.navigate(target_url, wait);
    }

    pub fn evaluate(*_, expression: []const u8): EvaluateResult!BidiError {
        return _.context.evaluate(expression, true);
    }

    pub fn locateCss(*_, selector: []const u8, max_count: u32): ([]const u8)!BidiError {
        return _.context.locateNodes(BidiLocator{
            locator_type: LocatorType.Css,
            value: selector,
        }, max_count);
    }
}
