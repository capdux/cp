// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/bidi/core/Connection.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/bidi/protocol.cp — W3C WebDriver BiDi Protocol Types
// Strongly-typed Session / Browser / BrowsingContext / Script / Network /
// Input / Storage / Log / Emulation commands, events & result structures.
// ============================================================================

const std = @import("std");

pub err BidiError {
    InvalidFrame,
    InvalidJson,
    MissingId,
    MissingMethod,
    ProtocolError,
    ConnectionClosed,
    SessionClosed,
    ContextClosed,
    UserContextNotFound,
    InterceptNotFound,
    Timeout,
    InvalidArgument,
    BufferTooSmall,
    OutOfMemory,
    UnsupportedOperation,
}

// ============================================================================
// Core BiDi JSON-RPC Envelope Types
// ============================================================================

pub enum BidiMessageType {
    Command,
    Success,
    Error,
    Event,
}

pub struct BidiCommand {
    id: i64 = 0,
    method: []const u8 = "",
    params_json: []const u8 = "{}",
}

pub struct BidiSuccessResponse {
    id: i64 = 0,
    result_json: []const u8 = "{}",
}

pub struct BidiErrorResponse {
    id: i64 = 0,
    has_id: bool = false,
    error_code: []const u8 = "",
    message: []const u8 = "",
    stacktrace: []const u8 = "",
}

pub struct BidiEvent {
    method: []const u8 = "",
    params_json: []const u8 = "{}",
}

pub struct BidiMessage {
    msg_type: BidiMessageType = BidiMessageType.Event,
    id: i64 = 0,
    has_id: bool = false,
    method: []const u8 = "",
    params_json: []const u8 = "{}",
    result_json: []const u8 = "{}",
    error_code: []const u8 = "",
    error_message: []const u8 = "",
    stacktrace: []const u8 = "",

    pub fn isSuccess(): bool {
        return _.msg_type == BidiMessageType.Success;
    }

    pub fn isError(): bool {
        return _.msg_type == BidiMessageType.Error;
    }

    pub fn isEvent(): bool {
        return _.msg_type == BidiMessageType.Event;
    }

    pub fn isResponse(): bool {
        return _.msg_type == BidiMessageType.Success || _.msg_type == BidiMessageType.Error;
    }

    pub fn isCdpEvent(): bool {
        return _.msg_type == BidiMessageType.Event && std.mem.startsWith(u8, _.method, "goog:cdp.");
    }

    pub fn toSuccessResponse(): BidiSuccessResponse {
        return BidiSuccessResponse{
            id: _.id,
            result_json: _.result_json,
        };
    }

    pub fn toErrorResponse(): BidiErrorResponse {
        return BidiErrorResponse{
            id: _.id,
            has_id: _.has_id,
            error_code: _.error_code,
            message: _.error_message,
            stacktrace: _.stacktrace,
        };
    }

    pub fn toEvent(): BidiEvent {
        return BidiEvent{
            method: _.method,
            params_json: _.params_json,
        };
    }
}

// ============================================================================
// 1. Session Module (session.*)
// ============================================================================

pub enum ProxyType {
    Direct,
    Manual,
    Pac,
    Autodetect,
    System,
}

pub fn proxyTypeName(pt: ProxyType): []const u8 {
    switch (pt) {
        ProxyType.Direct => return "direct",
        ProxyType.Manual => return "manual",
        ProxyType.Pac => return "pac",
        ProxyType.Autodetect => return "autodetect",
        ProxyType.System => return "system",
    }
}

pub struct ProxyConfiguration {
    proxy_type: ProxyType = ProxyType.Direct,
    http_proxy: []const u8 = "",
    ssl_proxy: []const u8 = "",
    socks_proxy: []const u8 = "",
    socks_version: u8 = 5,
    proxy_autoconfig_url: []const u8 = "",
    no_proxy: []const []const u8 = &.{},
}

pub struct SessionCapabilitiesRequest {
    accept_insecure_certs: bool = false,
    browser_name: []const u8 = "",
    browser_version: []const u8 = "",
    platform_name: []const u8 = "",
    unhandled_prompt_behavior: []const u8 = "ignore",
    web_socket_url: bool = true,
}

pub struct SessionCapabilities {
    accept_insecure_certs: bool = false,
    browser_name: []const u8 = "",
    browser_version: []const u8 = "",
    platform_name: []const u8 = "",
    user_agent: []const u8 = "",
    set_window_rect: bool = false,
}

pub struct SessionNewResult {
    session_id: []const u8 = "",
    capabilities: SessionCapabilities = .{},
}

pub struct SessionStatusResult {
    ready: bool = false,
    message: []const u8 = "",
}

pub struct SessionSubscribeParams {
    events: []const []const u8 = &.{},
    contexts: []const []const u8 = &.{},
    user_contexts: []const []const u8 = &.{},
}

pub struct SessionUnsubscribeParams {
    events: []const []const u8 = &.{},
    contexts: []const []const u8 = &.{},
    subscriptions: []const []const u8 = &.{},
}

// ============================================================================
// 2. Browser Module (browser.*)
// ============================================================================

pub enum ClientWindowState {
    Fullscreen,
    Maximized,
    Minimized,
    Normal,
}

pub struct ClientWindowInfo {
    client_window: []const u8 = "",
    active: bool = false,
    state: ClientWindowState = ClientWindowState.Normal,
    x: i32 = 0,
    y: i32 = 0,
    width: u32 = 0,
    height: u32 = 0,
}

pub struct UserContextInfo {
    user_context: []const u8 = "default",
}

pub struct CreateUserContextParams {
    accept_insecure_certs: bool = false,
    has_proxy: bool = false,
    proxy: ProxyConfiguration = .{},
}

pub enum DownloadBehaviorPolicy {
    Allowed,
    Denied,
    Default,
}

pub struct DownloadBehaviorParams {
    policy: DownloadBehaviorPolicy = DownloadBehaviorPolicy.Default,
    destination_folder: []const u8 = "",
    user_contexts: []const []const u8 = &.{},
}

// ============================================================================
// 3. BrowsingContext Module (browsingContext.*)
// ============================================================================

pub enum CreateContextType {
    Tab,
    Window,
}

pub fn createContextTypeName(ct: CreateContextType): []const u8 {
    switch (ct) {
        CreateContextType.Tab => return "tab",
        CreateContextType.Window => return "window",
    }
}

pub enum ReadinessState {
    None,
    Interactive,
    Complete,
}

pub fn readinessStateName(rs: ReadinessState): []const u8 {
    switch (rs) {
        ReadinessState.None => return "none",
        ReadinessState.Interactive => return "interactive",
        ReadinessState.Complete => return "complete",
    }
}

pub enum UserPromptType {
    Alert,
    Confirm,
    Prompt,
    BeforeUnload,
}

pub fn userPromptTypeName(pt: UserPromptType): []const u8 {
    switch (pt) {
        UserPromptType.Alert => return "alert",
        UserPromptType.Confirm => return "confirm",
        UserPromptType.Prompt => return "prompt",
        UserPromptType.BeforeUnload => return "beforeunload",
    }
}

pub fn parseUserPromptType(raw: []const u8): UserPromptType {
    if (std.mem.eql(u8, raw, "confirm")) return UserPromptType.Confirm;
    if (std.mem.eql(u8, raw, "prompt")) return UserPromptType.Prompt;
    if (std.mem.eql(u8, raw, "beforeunload")) return UserPromptType.BeforeUnload;
    return UserPromptType.Alert;
}

pub struct BrowsingContextInfo {
    context: []const u8 = "",
    url: []const u8 = "about:blank",
    parent: []const u8 = "",
    user_context: []const u8 = "default",
    original_opener: []const u8 = "",
    client_window: []const u8 = "",
}

pub struct BidiNavigationInfo {
    context: []const u8 = "",
    navigation: []const u8 = "",
    timestamp: i64 = 0,
    url: []const u8 = "",
}

pub struct CreateContextParams {
    context_type: CreateContextType = CreateContextType.Tab,
    reference_context: []const u8 = "",
    background: bool = false,
    user_context: []const u8 = "",
}

pub struct NavigateParams {
    context: []const u8 = "",
    url: []const u8 = "",
    wait: ReadinessState = ReadinessState.Complete,
}

pub struct NavigateResult {
    navigation: []const u8 = "",
    url: []const u8 = "",
}

pub struct ReloadParams {
    context: []const u8 = "",
    ignore_cache: bool = false,
    wait: ReadinessState = ReadinessState.Complete,
}

pub enum CaptureScreenshotOrigin {
    Viewport,
    Document,
}

pub struct CaptureScreenshotClip {
    enabled: bool = false,
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
}

pub struct CaptureScreenshotParams {
    context: []const u8 = "",
    origin: CaptureScreenshotOrigin = CaptureScreenshotOrigin.Viewport,
    format_type: []const u8 = "image/png",
    quality: f64 = -1.0,
    clip: CaptureScreenshotClip = .{},
}

pub enum PrintOrientation {
    Portrait,
    Landscape,
}

pub struct PrintMargin {
    top: f64 = 1.0,
    bottom: f64 = 1.0,
    left: f64 = 1.0,
    right: f64 = 1.0,
}

pub struct PrintPageSize {
    width: f64 = 21.59,
    height: f64 = 27.94,
}

pub struct PrintParams {
    context: []const u8 = "",
    background: bool = false,
    orientation: PrintOrientation = PrintOrientation.Portrait,
    scale: f64 = 1.0,
    shrink_to_fit: bool = true,
    margin: PrintMargin = .{},
    page: PrintPageSize = .{},
}

pub struct SetViewportParams {
    context: []const u8 = "",
    width: u32 = 1280,
    height: u32 = 720,
    device_pixel_ratio: f64 = 1.0,
    reset_viewport: bool = false,
}

pub enum LocatorType {
    Css,
    XPath,
    InnerText,
    Accessibility,
    Context,
}

pub fn locatorTypeName(lt: LocatorType): []const u8 {
    switch (lt) {
        LocatorType.Css => return "css",
        LocatorType.XPath => return "xpath",
        LocatorType.InnerText => return "innerText",
        LocatorType.Accessibility => return "accessibility",
        LocatorType.Context => return "context",
    }
}

pub struct BidiLocator {
    locator_type: LocatorType = LocatorType.Css,
    value: []const u8 = "",
    ignore_case: bool = false,
    match_type: []const u8 = "full",
    role: []const u8 = "",
    name: []const u8 = "",
}

pub struct LocateNodesParams {
    context: []const u8 = "",
    locator: BidiLocator = .{},
    max_node_count: u32 = 0,
    sandbox: []const u8 = "",
}

pub struct HandleUserPromptParams {
    context: []const u8 = "",
    accept: bool = true,
    user_text: []const u8 = "",
    has_user_text: bool = false,
}

pub struct UserPromptOpenedEvent {
    context: []const u8 = "",
    prompt_type: UserPromptType = UserPromptType.Alert,
    message: []const u8 = "",
    default_value: []const u8 = "",
}

// ============================================================================
// 4. Script Module (script.*)
// ============================================================================

pub enum RealmType {
    Window,
    DedicatedWorker,
    SharedWorker,
    ServiceWorker,
    Worker,
    PaintWorklet,
    AudioWorklet,
    Worklet,
}

pub fn realmTypeName(rt: RealmType): []const u8 {
    switch (rt) {
        RealmType.Window => return "window",
        RealmType.DedicatedWorker => return "dedicated-worker",
        RealmType.SharedWorker => return "shared-worker",
        RealmType.ServiceWorker => return "service-worker",
        RealmType.Worker => return "worker",
        RealmType.PaintWorklet => return "paint-worklet",
        RealmType.AudioWorklet => return "audio-worklet",
        RealmType.Worklet => return "worklet",
    }
}

pub fn parseRealmType(raw: []const u8): RealmType {
    if (std.mem.eql(u8, raw, "dedicated-worker")) return RealmType.DedicatedWorker;
    if (std.mem.eql(u8, raw, "shared-worker")) return RealmType.SharedWorker;
    if (std.mem.eql(u8, raw, "service-worker")) return RealmType.ServiceWorker;
    if (std.mem.eql(u8, raw, "worker")) return RealmType.Worker;
    if (std.mem.eql(u8, raw, "paint-worklet")) return RealmType.PaintWorklet;
    if (std.mem.eql(u8, raw, "audio-worklet")) return RealmType.AudioWorklet;
    if (std.mem.eql(u8, raw, "worklet")) return RealmType.Worklet;
    return RealmType.Window;
}

pub enum ResultOwnership {
    None,
    Root,
}

pub fn resultOwnershipName(ro: ResultOwnership): []const u8 {
    switch (ro) {
        ResultOwnership.None => return "none",
        ResultOwnership.Root => return "root",
    }
}

pub struct ScriptTarget {
    context: []const u8 = "",
    sandbox: []const u8 = "",
    realm: []const u8 = "",
}

pub struct SerializationOptions {
    max_dom_depth: i32 = 0,
    max_object_depth: i32 = -1,
    include_shadow_tree: []const u8 = "none",
}

pub enum EvaluateResultKind {
    Success,
    Exception,
}

pub struct BidiRemoteValue {
    value_type: []const u8 = "undefined",
    value_raw: []const u8 = "",
    handle: []const u8 = "",
    shared_id: []const u8 = "",
}

pub struct BidiExceptionDetails {
    text: []const u8 = "",
    line_number: i64 = 0,
    column_number: i64 = 0,
}

pub struct EvaluateParams {
    expression: []const u8 = "",
    target: ScriptTarget = .{},
    await_promise: bool = true,
    result_ownership: ResultOwnership = ResultOwnership.None,
    user_activation: bool = false,
}

pub struct CallFunctionParams {
    function_declaration: []const u8 = "",
    target: ScriptTarget = .{},
    arguments_json: []const u8 = "[]",
    this_json: []const u8 = "",
    await_promise: bool = true,
    result_ownership: ResultOwnership = ResultOwnership.None,
    user_activation: bool = false,
}

pub struct EvaluateResult {
    kind: EvaluateResultKind = EvaluateResultKind.Success,
    realm: []const u8 = "",
    result: BidiRemoteValue = .{},
    exception: BidiExceptionDetails = .{},
}

pub struct AddPreloadScriptParams {
    function_declaration: []const u8 = "",
    contexts: []const []const u8 = &.{},
    user_contexts: []const []const u8 = &.{},
    sandbox: []const u8 = "",
}

pub struct DisownParams {
    handles: []const []const u8 = &.{},
    target: ScriptTarget = .{},
}

pub struct RealmInfo {
    realm: []const u8 = "",
    origin: []const u8 = "",
    realm_type: RealmType = RealmType.Window,
    context: []const u8 = "",
    sandbox: []const u8 = "",
}

// ============================================================================
// 5. Network Module (network.*)
// ============================================================================

pub enum InterceptPhase {
    BeforeRequestSent,
    ResponseStarted,
    AuthRequired,
}

pub fn interceptPhaseName(phase: InterceptPhase): []const u8 {
    switch (phase) {
        InterceptPhase.BeforeRequestSent => return "beforeRequestSent",
        InterceptPhase.ResponseStarted => return "responseStarted",
        InterceptPhase.AuthRequired => return "authRequired",
    }
}

pub enum BytesValueType {
    String,
    Base64,
}

pub struct BytesValue {
    value_type: BytesValueType = BytesValueType.String,
    value: []const u8 = "",
}

pub struct BidiHeader {
    name: []const u8 = "",
    value: []const u8 = "",
}

pub struct BidiCookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "/",
    size: u64 = 0,
    http_only: bool = false,
    secure: bool = false,
    same_site: []const u8 = "none",
    expiry: i64 = 0,
}

pub enum UrlPatternType {
    String,
    Pattern,
}

pub struct UrlPattern {
    pattern_type: UrlPatternType = UrlPatternType.String,
    pattern: []const u8 = "",
    protocol: []const u8 = "",
    hostname: []const u8 = "",
    port: []const u8 = "",
    pathname: []const u8 = "",
    search: []const u8 = "",
}

pub struct AddInterceptParams {
    phases: []const InterceptPhase = &.{},
    contexts: []const []const u8 = &.{},
    url_patterns: []const UrlPattern = &.{},
}

pub struct ContinueRequestParams {
    request: []const u8 = "",
    url: []const u8 = "",
    method: []const u8 = "",
    body: []const u8 = "",
    headers: []const BidiHeader = &.{},
}

pub struct ProvideResponseParams {
    request: []const u8 = "",
    status_code: u16 = 200,
    reason_phrase: []const u8 = "OK",
    body: []const u8 = "",
    body_base64: bool = false,
    headers: []const BidiHeader = &.{},
}

pub struct ContinueResponseParams {
    request: []const u8 = "",
    status_code: u16 = 0,
    reason_phrase: []const u8 = "",
    headers: []const BidiHeader = &.{},
}

pub enum AuthAction {
    ProvideCredentials,
    Default,
    Cancel,
}

pub struct ContinueWithAuthParams {
    request: []const u8 = "",
    action: AuthAction = AuthAction.Default,
    username: []const u8 = "",
    password: []const u8 = "",
}

pub enum CacheBehavior {
    Default,
    Bypass,
}

// ============================================================================
// 6. Input Module (input.*)
// ============================================================================

pub enum InputSourceType {
    None,
    Key,
    Pointer,
    Wheel,
}

pub enum BidiPointerType {
    Mouse,
    Pen,
    Touch,
}

pub enum BidiMouseButton {
    Left,
    Middle,
    Right,
    Back,
    Forward,
}

pub fn bidiMouseButtonCode(btn: BidiMouseButton): u8 {
    switch (btn) {
        BidiMouseButton.Left => return 0,
        BidiMouseButton.Middle => return 1,
        BidiMouseButton.Right => return 2,
        BidiMouseButton.Back => return 3,
        BidiMouseButton.Forward => return 4,
    }
}

pub struct SetFilesParams {
    context: []const u8 = "",
    element_shared_id: []const u8 = "",
    files: []const []const u8 = &.{},
}

// ============================================================================
// 7. Storage, Log & Emulation Modules
// ============================================================================

pub enum PartitionKind {
    Context,
    StorageKey,
}

pub struct PartitionDescriptor {
    kind: PartitionKind = PartitionKind.Context,
    context: []const u8 = "",
    user_context: []const u8 = "",
    source_origin: []const u8 = "",
}

pub enum LogLevel {
    Debug,
    Info,
    Warn,
    Error,
}

pub fn parseLogLevel(raw: []const u8): LogLevel {
    if (std.mem.eql(u8, raw, "debug")) return LogLevel.Debug;
    if (std.mem.eql(u8, raw, "warn")) return LogLevel.Warn;
    if (std.mem.eql(u8, raw, "error")) return LogLevel.Error;
    return LogLevel.Info;
}

pub struct LogEntry {
    level: LogLevel = LogLevel.Info,
    entry_type: []const u8 = "console",
    method: []const u8 = "log",
    text: []const u8 = "",
    timestamp: i64 = 0,
    context: []const u8 = "",
    realm: []const u8 = "",
}

pub enum NetworkConditionType {
    Online,
    Offline,
}

pub struct EmulationUserAgentParams {
    user_agent: []const u8 = "",
    contexts: []const []const u8 = &.{},
    user_contexts: []const []const u8 = &.{},
}

pub struct EmulationLocaleParams {
    locale: []const u8 = "",
    contexts: []const []const u8 = &.{},
    user_contexts: []const []const u8 = &.{},
}

// ============================================================================
// Shared JSON Escape Helper (Zero-Heap)
// ============================================================================

pub fn bidiWriteJsonEscaped(buf: []u8, pos: usize, s: []const u8): usize!BidiError {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            if (p + 2 > buf.len) return BidiError.BufferTooSmall;
            buf[p] = '\\';
            buf[p + 1] = c;
            p += 2;
        } else if (c == '\n') {
            if (p + 2 > buf.len) return BidiError.BufferTooSmall;
            buf[p] = '\\';
            buf[p + 1] = 'n';
            p += 2;
        } else if (c == '\r') {
            if (p + 2 > buf.len) return BidiError.BufferTooSmall;
            buf[p] = '\\';
            buf[p + 1] = 'r';
            p += 2;
        } else if (c == '\t') {
            if (p + 2 > buf.len) return BidiError.BufferTooSmall;
            buf[p] = '\\';
            buf[p + 1] = 't';
            p += 2;
        } else {
            if (p + 1 > buf.len) return BidiError.BufferTooSmall;
            buf[p] = c;
            p += 1;
        }
        i += 1;
    }
    return p;
}

pub fn bidiWriteStr(buf: []u8, pos: usize, s: []const u8): usize!BidiError {
    if (pos + s.len > buf.len) return BidiError.BufferTooSmall;
    if (s.len > 0) @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}
