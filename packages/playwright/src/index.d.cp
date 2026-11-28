// Source: Absorbed from _ref/playwright/packages/playwright-core/src/index.ts | Synced: 2026-11-28
// Type definitions for CP (`playwright` / `playwright-core` / `playwright-chromium`)

export err PlaywrightError {
    NotConnected,
    ConnectionClosed,
    BrowserClosed,
    ContextClosed,
    PageClosed,
    TargetNotFound,
    Timeout,
    InvalidSelector,
    InvalidUrl,
    ExecutableNotFound,
    ProtocolError,
    BufferTooSmall,
    RouteAlreadyHandled,
    OutOfMemory,
}

export enum RouteAction {
    Pending,
    Continued,
    Fulfilled,
    Aborted,
    Fallback,
}

export struct NameValue {
    name: []const u8,
    value: []const u8,
}

export struct ResourceTiming {
    start_time: f64,
    domain_lookup_start: f64,
    domain_lookup_end: f64,
    connect_start: f64,
    connect_end: f64,
    secure_connection_start: f64,
    response_start: f64,
    response_end: f64,
}

export struct RequestSizes {
    request_body_size: i64,
    request_headers_size: i64,
    response_body_size: i64,
    response_headers_size: i64,
}

export struct RequestFailure {
    error_text: []const u8,
}

export struct SecurityDetails {
    issuer: []const u8,
    protocol: []const u8,
    subject_name: []const u8,
    valid_from: i64,
    valid_to: i64,
}

export struct RemoteAddr {
    ip_address: []const u8,
    port: u16,
}

export struct FulfillOptions {
    status: i32,
    content_type: []const u8,
    body: []const u8,
    headers_json: []const u8,
}

export struct RouteContinueOverrides {
    url: []const u8,
    method: []const u8,
    headers_json: []const u8,
    post_data: []const u8,
}

export struct ScriptTagOptions {
    url: []const u8,
    path: []const u8,
    content: []const u8,
    tag_type: []const u8,
}

export struct StyleTagOptions {
    url: []const u8,
    path: []const u8,
    content: []const u8,
}

export struct WorkerInfo {
    url: []const u8,
    target_id: []const u8,
}

export struct PageEventRecord {
    name: []const u8,
    payload: []const u8,
}

export struct Request {
    url: []const u8,
    method: []const u8,
    resource_type: []const u8,
    post_data: []const u8,
    headers_json: []const u8,
    is_navigation_request: bool,
    frame_id: []const u8,
    fn isNavigationRequest(): bool;
    fn resourceType(): []const u8;
    fn allHeaders(): []const u8;
    fn headersArray(alloc: std.mem.Allocator): ([]NameValue)!PlaywrightError;
    fn headerValue(name: []const u8): ([]const u8)?;
    fn postData(): ([]const u8)?;
    fn postDataJSON(): ([]const u8)?;
    fn redirectedFrom(): (*Request)?;
    fn redirectedTo(): (*Request)?;
    fn timing(): ResourceTiming;
    fn sizes(): RequestSizes;
    fn failure(): RequestFailure?;
    fn response(): (*Response)?;
    fn frame(): []const u8;
}

export struct Response {
    url: []const u8,
    status: i32,
    status_text: []const u8,
    ok: bool,
    headers_json: []const u8,
    body_bytes: []const u8,
    from_service_worker: bool,
    fn isOk(): bool;
    fn statusText(): []const u8;
    fn allHeaders(): []const u8;
    fn headersArray(alloc: std.mem.Allocator): ([]NameValue)!PlaywrightError;
    fn headerValue(name: []const u8): ([]const u8)?;
    fn body(): []const u8;
    fn text(): []const u8;
    fn json(): ([]const u8)!PlaywrightError;
    fn finished(): ([]const u8)?;
    fn fromServiceWorker(): bool;
    fn securityDetails(): SecurityDetails?;
    fn serverAddr(): RemoteAddr?;
    fn request(): (*Request)?;
}

export struct Route {
    request: Request,
    action: RouteAction,
    fn isHandled(): bool;
    fn fulfill(*_, opts: FulfillOptions): void!PlaywrightError;
    fn abort(*_, error_code: []const u8): void!PlaywrightError;
    fn continue_(*_, override_url: []const u8, override_method: []const u8): void!PlaywrightError;
    fn continueWithOverrides(*_, overrides: RouteContinueOverrides): void!PlaywrightError;
    fn fallback(*_): void!PlaywrightError;
    fn fetch(*_, overrides: RouteContinueOverrides): Response!PlaywrightError;
}

export struct AriaRoleNode {
    role: []const u8,
    name: []const u8,
    value: []const u8,
    level: i32,
    checked: bool,
    disabled: bool,
    backend_dom_node_id: i64,
}

export struct AriaSnapshot {
    static fn init(alloc: std.mem.Allocator): AriaSnapshot;
    fn deinit(*_);
    fn addFromCdpAxNodeJson(*_, node_json: []const u8): void!PlaywrightError;
    fn addNode(*_, node: AriaRoleNode): void!PlaywrightError;
    fn count(): usize;
    fn findByRoleAndName(role: []const u8, name: []const u8): AriaRoleNode?;
    fn toYamlString(alloc: std.mem.Allocator): ([]u8)!PlaywrightError;
}

export struct Locator {
    fn selector(): []const u8;
    fn locator(sub_selector: []const u8): Locator;
    fn first(): Locator;
    fn last(): Locator;
    fn nth(index: i32): Locator;
    fn filter(opts: LocatorOptions): Locator;
    fn waitFor(opts: WaitForOptions): void!PlaywrightError;
    fn click(): void!PlaywrightError;
    fn fill(value: []const u8): void!PlaywrightError;
    fn press(key: []const u8): void!PlaywrightError;
    fn check(): void!PlaywrightError;
    fn uncheck(): void!PlaywrightError;
    fn selectOption(value: []const u8): void!PlaywrightError;
    fn textContent(alloc: std.mem.Allocator): ([]u8)!PlaywrightError;
    fn innerText(alloc: std.mem.Allocator): ([]u8)!PlaywrightError;
    fn getAttribute(alloc: std.mem.Allocator, name: []const u8): (([]u8)?)!PlaywrightError;
    fn isVisible(): bool!PlaywrightError;
    fn isEnabled(): bool!PlaywrightError;
    fn count(): usize!PlaywrightError;
}

export struct ViewportSize {
    width: u32,
    height: u32,
}

export struct EmulateMediaOptions {
    media: []const u8,
    color_scheme: []const u8,
    reduced_motion: []const u8,
    forced_colors: []const u8,
}

export struct Page {
    fn url(): []const u8;
    fn title(): []const u8;
    fn isClosed(): bool;
    fn opener(): (*Page)?;
    fn workers(): []const WorkerInfo;
    fn viewportSize(): ViewportSize;
    fn setViewportSize(*_, width: u32, height: u32): void!PlaywrightError;
    fn bringToFront(*_): void!PlaywrightError;
    fn emulateMedia(*_, opts: EmulateMediaOptions): void!PlaywrightError;
    fn setExtraHTTPHeaders(*_, headers_json: []const u8): void!PlaywrightError;
    fn pause(*_): void!PlaywrightError;
    fn addInitScript(*_, script: []const u8): void!PlaywrightError;
    fn addScriptTag(*_, opts: ScriptTagOptions): ([]const u8)!PlaywrightError;
    fn addStyleTag(*_, opts: StyleTagOptions): ([]const u8)!PlaywrightError;
    fn exposeBinding(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError;
    fn exposeFunction(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError;
    fn goto(*_, target_url: []const u8, opts: GotoOptions): Response!PlaywrightError;
    fn reload(*_): Response!PlaywrightError;
    fn goBack(*_): Response?!PlaywrightError;
    fn goForward(*_): Response?!PlaywrightError;
    fn setContent(*_, html: []const u8): void!PlaywrightError;
    fn content(): []const u8;
    fn evaluate(*_, expression: []const u8): ([]const u8)!PlaywrightError;
    fn locator(selector: []const u8): Locator;
    fn getByRole(role: []const u8, opts: ByRoleOptions): Locator;
    fn getByText(text: []const u8, exact: bool): Locator;
    fn getByLabel(text: []const u8, exact: bool): Locator;
    fn getByPlaceholder(text: []const u8, exact: bool): Locator;
    fn getByTestId(test_id: []const u8): Locator;
    fn click(*_, selector: []const u8): void!PlaywrightError;
    fn dblclick(*_, selector: []const u8): void!PlaywrightError;
    fn hover(*_, selector: []const u8): void!PlaywrightError;
    fn tap(*_, selector: []const u8): void!PlaywrightError;
    fn fill(*_, selector: []const u8, value: []const u8): void!PlaywrightError;
    fn typeText(*_, selector: []const u8, text: []const u8): void!PlaywrightError;
    fn press(*_, selector: []const u8, key: []const u8): void!PlaywrightError;
    fn check(*_, selector: []const u8): void!PlaywrightError;
    fn uncheck(*_, selector: []const u8): void!PlaywrightError;
    fn selectOption(*_, selector: []const u8, value: []const u8): void!PlaywrightError;
    fn focus(*_, selector: []const u8): void!PlaywrightError;
    fn dispatchEvent(*_, selector: []const u8, event_type: []const u8, event_init_json: []const u8): void!PlaywrightError;
    fn dragAndDrop(*_, source_selector: []const u8, target_selector: []const u8): void!PlaywrightError;
    fn innerHTML(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn innerText(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn textContent(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn inputValue(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn getAttribute(*_, alloc: std.mem.Allocator, selector: []const u8, attr_name: []const u8): (([]u8)?)!PlaywrightError;
    fn isChecked(*_, selector: []const u8): bool!PlaywrightError;
    fn isDisabled(*_, selector: []const u8): bool!PlaywrightError;
    fn isEditable(*_, selector: []const u8): bool!PlaywrightError;
    fn isEnabled(*_, selector: []const u8): bool!PlaywrightError;
    fn isHidden(*_, selector: []const u8): bool!PlaywrightError;
    fn isVisible(*_, selector: []const u8): bool!PlaywrightError;
    fn waitForSelector(*_, selector: []const u8, opts: WaitForOptions): Locator!PlaywrightError;
    fn waitForEvent(*_, event_name: []const u8, timeout_ms: u32): ([]const u8)!PlaywrightError;
    fn waitForFunction(*_, expression: []const u8, timeout_ms: u32): ([]const u8)!PlaywrightError;
    fn waitForLoadState(*_, state: []const u8): void!PlaywrightError;
    fn waitForRequest(*_, url_pattern: []const u8, timeout_ms: u32): Request!PlaywrightError;
    fn waitForResponse(*_, url_pattern: []const u8, timeout_ms: u32): Response!PlaywrightError;
    fn waitForURL(*_, url_pattern: []const u8, timeout_ms: u32): void!PlaywrightError;
    fn waitForTimeout(*_, timeout_ms: u32): void!PlaywrightError;
    fn route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError;
    fn unroute(*_, pattern: []const u8);
    fn ariaSnapshot(*_, alloc: std.mem.Allocator): AriaSnapshot!PlaywrightError;
    fn screenshot(*_, opts: ScreenshotOptions): ([]const u8)!PlaywrightError;
    fn pdf(*_, opts: PdfOptions): ([]const u8)!PlaywrightError;
    fn clock(*_): Clock;
    fn coverage(*_): Coverage;
    fn mainFrame(*_): (*Frame)!PlaywrightError;
    fn frames(*_, alloc: std.mem.Allocator): ([]*Frame)!PlaywrightError;
    fn frameByName(*_, target_name: []const u8): (*Frame)?;
    fn frameByUrl(*_, url_pattern: []const u8): (*Frame)?;
    fn routeWebSocket(*_, pattern: []const u8, handler: WebSocketRouteHandlerFn, ctx: usize): void!PlaywrightError;
    fn unrouteWebSocket(*_, pattern: []const u8);
    fn video(*_): (*Video)!PlaywrightError;
    fn close(*_);
}

export struct Geolocation {
    latitude: f64,
    longitude: f64,
    accuracy: f64,
}

export struct HTTPCredentials {
    username: []const u8,
    password: []const u8,
    origin: []const u8,
}

export struct CDPSession {
    fn sessionId(): []const u8;
    fn send(*_, method: []const u8, params_json: []const u8): ([]const u8)!PlaywrightError;
    fn detach(*_): void!PlaywrightError;
}

export struct BrowserContext {
    fn browser(): usize;
    fn newPage(*_): (*Page)!PlaywrightError;
    fn pages(): []*Page;
    fn backgroundPages(): []*Page;
    fn serviceWorkers(): []const WorkerInfo;
    fn tracing(*_): *Tracing;
    fn cookies(): []const Cookie;
    fn addCookies(*_, new_cookies: []const Cookie): void!PlaywrightError;
    fn clearCookies(*_);
    fn storageState(alloc: std.mem.Allocator): StorageState!PlaywrightError;
    fn addInitScript(*_, script: []const u8): void!PlaywrightError;
    fn exposeBinding(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError;
    fn exposeFunction(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError;
    fn grantPermissions(*_, permissions: []const []const u8, origin: []const u8): void!PlaywrightError;
    fn clearPermissions(*_): void!PlaywrightError;
    fn setGeolocation(*_, geo: Geolocation?): void!PlaywrightError;
    fn setExtraHTTPHeaders(*_, headers_json: []const u8): void!PlaywrightError;
    fn setOffline(*_, offline: bool): void!PlaywrightError;
    fn setHTTPCredentials(*_, creds: HTTPCredentials?): void!PlaywrightError;
    fn setDefaultTimeout(*_, timeout_ms: u32);
    fn setDefaultNavigationTimeout(*_, timeout_ms: u32);
    fn routeFromHAR(*_, har_path: []const u8, url_pattern: []const u8): void!PlaywrightError;
    fn waitForEvent(*_, event_name: []const u8, timeout_ms: u32): ([]const u8)!PlaywrightError;
    fn newCDPSession(*_, page: *Page): CDPSession!PlaywrightError;
    fn route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError;
    fn unroute(*_, pattern: []const u8);
    fn close(*_);
}

export struct Browser {
    fn newContext(*_, opts: BrowserContextOptions): (*BrowserContext)!PlaywrightError;
    fn newPage(*_): (*Page)!PlaywrightError;
    fn contexts(): []*BrowserContext;
    fn version(): []const u8;
    fn isConnected(): bool;
    fn close(*_);
}

export struct ChromiumBrowserType {
    static fn name(): []const u8;
    static fn executablePath(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)?;
    static fn launch(alloc: std.mem.Allocator, opts: LaunchOptions): Browser!PlaywrightError;
    static fn launchWithTransport(alloc: std.mem.Allocator, opts: LaunchOptions, cb: CdpSendTransportFn, ctx: usize): Browser!PlaywrightError;
    static fn connectOverCDP(alloc: std.mem.Allocator, endpoint_url: []const u8, opts: ConnectOverCdpOptions): Browser!PlaywrightError;
    static fn connectOverCDPWithTransport(alloc: std.mem.Allocator, endpoint_url: []const u8, opts: ConnectOverCdpOptions, cb: CdpSendTransportFn, ctx: usize): Browser!PlaywrightError;
}

export const chromium = ChromiumBrowserType;

export struct WebKitLaunchOptions {
    headless: bool,
    executable_path: []const u8,
    user_data_dir: []const u8,
    args: []const []const u8,
    viewport_width: u32,
    viewport_height: u32,
    timeout_ms: u32,
    ignore_https_errors: bool,
    require_executable: bool,
    proxy_server: []const u8,
    proxy_bypass: []const u8,
    transport_fn: CdpSendTransportFn?,
    transport_ctx: usize,
}

export struct WkPageProxySession {
    page_proxy_id: []const u8,
    target_id: []const u8,
    crashed: bool,
    disposed: bool,
    messages_received: usize,
    fn send(*_, method: []const u8, params_json: []const u8): i64!PlaywrightError;
    fn sendToTarget(*_, inner_method: []const u8, inner_params_json: []const u8): i64!PlaywrightError;
    fn markAsCrashed(*_);
    fn isDisposed(): bool;
    fn dispose(*_);
}

export struct WkConnection {
    static fn init(alloc: std.mem.Allocator): WkConnection;
    fn deinit(*_);
    fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize);
    fn nextMessageId(*_): i64;
    fn isClosed(): bool;
    fn close(*_);
    static fn encodeFrame(id: i64, method: []const u8, params_json: []const u8, page_proxy_id: []const u8, out_buf: []u8): usize!PlaywrightError;
    static fn decodeFrame(raw: []const u8): WkMessage!PlaywrightError;
    fn send(*_, method: []const u8, params_json: []const u8): i64!PlaywrightError;
    fn sendWithProxy(*_, method: []const u8, params_json: []const u8, page_proxy_id: []const u8): i64!PlaywrightError;
    fn dispatchMessage(*_, raw: []const u8): void!PlaywrightError;
    fn sessionCount(): usize;
}

export struct WebKitPage {
    fn url(): []const u8;
    fn title(): []const u8;
    fn content(): []const u8;
    fn isClosed(): bool;
    fn goto(*_, target_url: []const u8, opts: GotoOptions): Response!PlaywrightError;
    fn setContent(*_, html: []const u8): void!PlaywrightError;
    fn evaluate(*_, expression: []const u8): ([]const u8)!PlaywrightError;
    fn setViewport(*_, width: u32, height: u32): void!PlaywrightError;
    fn screenshot(*_, opts: ScreenshotOptions): ([]const u8)!PlaywrightError;
    fn keyboardDown(*_, key: []const u8, modifiers: u32): void!PlaywrightError;
    fn keyboardUp(*_, key: []const u8, modifiers: u32): void!PlaywrightError;
    fn insertText(*_, text: []const u8): void!PlaywrightError;
    fn mouseMove(*_, x: f64, y: f64): void!PlaywrightError;
    fn mouseDown(*_, x: f64, y: f64, button: []const u8): void!PlaywrightError;
    fn mouseUp(*_, x: f64, y: f64, button: []const u8): void!PlaywrightError;
    fn click(*_, x: f64, y: f64): void!PlaywrightError;
    fn route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError;
    fn unroute(*_, pattern: []const u8);
    fn close(*_);
}

export struct WebKitBrowser {
    fn newContext(*_, opts: BrowserContextOptions): (*WebKitBrowserContext)!PlaywrightError;
    fn newPage(*_): (*WebKitPage)!PlaywrightError;
    fn contexts(): []*WebKitBrowserContext;
    fn version(): []const u8;
    fn isConnected(): bool;
    fn close(*_);
    fn deinit(*_);
}

export struct WebKitBrowserType {
    static fn name(): []const u8;
    static fn executablePath(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)?;
    static fn buildLaunchArgs(alloc: std.mem.Allocator, opts: WebKitLaunchOptions): ([]([]u8))!PlaywrightError;
    static fn freeLaunchArgs(alloc: std.mem.Allocator, args: [][]u8);
    static fn launch(alloc: std.mem.Allocator, opts: WebKitLaunchOptions): WebKitBrowser!PlaywrightError;
    static fn launchWithTransport(alloc: std.mem.Allocator, opts: WebKitLaunchOptions, cb: CdpSendTransportFn, ctx: usize): WebKitBrowser!PlaywrightError;
    static fn connectWithTransport(alloc: std.mem.Allocator, opts: WebKitLaunchOptions, cb: CdpSendTransportFn, ctx: usize): WebKitBrowser!PlaywrightError;
}

export const webkit = WebKitBrowserType;

export enum ActionabilityState {
    Pending,
    Visible,
    Hidden,
    Stable,
    Enabled,
    Editable,
    InViewport,
    HitTargetVerified,
    Done,
    Timeout,
    NotConnected,
}

export enum ActionabilityCheckKind {
    Visible,
    Hidden,
    Stable,
    Enabled,
    Editable,
    Attached,
    Detached,
}

export struct ActionabilityOptions {
    timeout_ms: u32,
    force: bool,
    trial: bool,
    scroll_into_view: bool,
    check_hit_target: bool,
    position_x: f64,
    position_y: f64,
}

export struct ActionabilityReport {
    state: ActionabilityState,
    attempts: u32,
    scrolled: bool,
    hit_target_ok: bool,
    trial_only: bool,
    error_message: []const u8,
}

export enum ClockMode {
    Real,
    Installed,
    Paused,
    Fixed,
}

export struct Clock {
    fn now(): i64;
    fn install(*_, time_ms: i64?): void!PlaywrightError;
    fn fastForward(*_, ticks_ms: i64): void!PlaywrightError;
    fn fastForwardStr(*_, ticks_str: []const u8): void!PlaywrightError;
    fn runFor(*_, ticks_ms: i64): void!PlaywrightError;
    fn pauseAt(*_, time_ms: i64): void!PlaywrightError;
    fn resume_(*_): void!PlaywrightError;
    fn setFixedTime(*_, time_ms: i64): void!PlaywrightError;
    fn setSystemTime(*_, time_ms: i64): void!PlaywrightError;
    fn uninstall(*_);
}

export struct Download {
    fn suggestedFilename(): []const u8;
    fn saveAs(*_, dest_path: []const u8): void!PlaywrightError;
    fn path(): ([]const u8)?;
    fn cancel(*_);
}

export struct FileChooser {
    fn isMultiple(): bool;
    fn setFiles(*_, file_paths: []const []const u8): void!PlaywrightError;
}

export struct HarRecorder {
    static fn init(alloc: std.mem.Allocator, opts: HarRecordOptions): HarRecorder;
    fn deinit(*_);
    fn recordEntry(*_, entry: HarEntry): bool!PlaywrightError;
    fn entryCount(): usize;
    fn toJson(*_, alloc: std.mem.Allocator): ([]u8)!PlaywrightError;
}

export struct PlaywrightRemoteServer {
    static fn init(alloc: std.mem.Allocator, config: PlaywrightRemoteConfig): PlaywrightRemoteServer;
    fn deinit(*_);
    fn start(*_): void!PlaywrightError;
    fn stop(*_);
    fn isListening(): bool;
    fn wsEndpoint(out_buf: []u8): ([]const u8)!PlaywrightError;
    fn handleRpcMessage(*_, alloc: std.mem.Allocator, msg_json: []const u8): ([]u8)!PlaywrightError;
}

export struct PlaywrightCli {
    static fn parseArgs(args: []const []const u8): CliOptions!PlaywrightError;
    static fn generateCodegenSkeleton(alloc: std.mem.Allocator, opts: CliOptions): ([]u8)!PlaywrightError;
}

export struct Frame {
    fn id(): []const u8;
    fn name(): []const u8;
    fn url(): []const u8;
    fn title(): []const u8;
    fn content(): []const u8;
    fn setContent(*_, html: []const u8): void!PlaywrightError;
    fn isDetached(): bool;
    fn page(): usize;
    fn parentFrame(): (*Frame)?;
    fn childFrames(): []*Frame;
    fn addChildFrame(*_, child_frame_id: []const u8, child_name: []const u8, child_url: []const u8): (*Frame)!PlaywrightError;
    fn frameByName(*_, target_name: []const u8): (*Frame)?;
    fn frameByUrl(*_, url_pattern: []const u8): (*Frame)?;
    fn locator(selector: []const u8): Locator;
    fn getByRole(role: []const u8, opts: ByRoleOptions): Locator;
    fn getByText(text: []const u8, exact: bool): Locator;
    fn getByLabel(text: []const u8, exact: bool): Locator;
    fn getByPlaceholder(text: []const u8, exact: bool): Locator;
    fn getByTestId(test_id: []const u8): Locator;
    fn goto(*_, target_url: []const u8, referer: []const u8): Response!PlaywrightError;
    fn evaluate(*_, expression: []const u8): ([]const u8)!PlaywrightError;
    fn addScriptTag(*_, opts: ScriptTagOptions): ([]const u8)!PlaywrightError;
    fn addStyleTag(*_, opts: StyleTagOptions): ([]const u8)!PlaywrightError;
    fn click(*_, selector: []const u8): void!PlaywrightError;
    fn dblclick(*_, selector: []const u8): void!PlaywrightError;
    fn fill(*_, selector: []const u8, value: []const u8): void!PlaywrightError;
    fn typeText(*_, selector: []const u8, text: []const u8): void!PlaywrightError;
    fn press(*_, selector: []const u8, key: []const u8): void!PlaywrightError;
    fn hover(*_, selector: []const u8): void!PlaywrightError;
    fn focus(*_, selector: []const u8): void!PlaywrightError;
    fn check(*_, selector: []const u8): void!PlaywrightError;
    fn uncheck(*_, selector: []const u8): void!PlaywrightError;
    fn selectOption(*_, selector: []const u8, value: []const u8): void!PlaywrightError;
    fn tap(*_, selector: []const u8): void!PlaywrightError;
    fn dispatchEvent(*_, selector: []const u8, event_type: []const u8, event_init_json: []const u8): void!PlaywrightError;
    fn dragAndDrop(*_, source_selector: []const u8, target_selector: []const u8): void!PlaywrightError;
    fn innerHTML(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn innerText(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn textContent(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn inputValue(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError;
    fn getAttribute(*_, alloc: std.mem.Allocator, selector: []const u8, attr_name: []const u8): (([]u8)?)!PlaywrightError;
    fn isChecked(*_, selector: []const u8): bool!PlaywrightError;
    fn isDisabled(*_, selector: []const u8): bool!PlaywrightError;
    fn isEditable(*_, selector: []const u8): bool!PlaywrightError;
    fn isEnabled(*_, selector: []const u8): bool!PlaywrightError;
    fn isHidden(*_, selector: []const u8): bool!PlaywrightError;
    fn isVisible(*_, selector: []const u8): bool!PlaywrightError;
    fn waitForSelector(*_, selector: []const u8, opts: WaitForOptions): Locator!PlaywrightError;
    fn waitForFunction(*_, expression: []const u8, timeout_ms: u32): ([]const u8)!PlaywrightError;
    fn waitForLoadState(*_, state: []const u8): void!PlaywrightError;
    fn waitForURL(*_, url_pattern: []const u8, timeout_ms: u32): void!PlaywrightError;
    fn frameElement(*_): ([]const u8)!PlaywrightError;
}

export struct Tracing {
    fn isRecording(): bool;
    fn eventCount(): usize;
    fn start(*_, opts: TracingOptions): void!PlaywrightError;
    fn startChunk(*_, title: []const u8): void!PlaywrightError;
    fn recordEvent(*_, event_type: []const u8, method: []const u8, page_id: []const u8, detail_json: []const u8): void!PlaywrightError;
    fn stopChunk(*_, out_path: []const u8): ([]u8)!PlaywrightError;
    fn stop(*_, out_path: []const u8): ([]u8)!PlaywrightError;
}

export struct TraceViewerOptions {
    host: []const u8,
    port: u16,
    workers: usize,
    headless: bool,
    trace_path: []const u8,
}

export struct TraceEntryRecord {
    name: []const u8,
    data: []const u8,
    mime_type: []const u8,
    size: usize,
}

export struct TraceHttpResponse {
    status: u16,
    content_type: []const u8,
    body: []u8,
    fn deinit(*_);
}

export struct TraceViewerServer {
    static fn init(alloc: std.mem.Allocator, opts: TraceViewerOptions): TraceViewerServer;
    fn deinit(*_);
    fn loadZipBytes(*_, raw_zip: []const u8): void!PlaywrightError;
    fn loadTraceFile(*_, path: []const u8): void!PlaywrightError;
    fn entryCount(): usize;
    fn findEntry(path: []const u8): TraceEntryRecord?;
    fn handleHttpRequest(*_, alloc: std.mem.Allocator, method: []const u8, path_and_query: []const u8): TraceHttpResponse!PlaywrightError;
    fn startServer(*_): void!PlaywrightError;
    fn url(out_buf: []u8): ([]const u8)!PlaywrightError;
    fn stop(*_);
}

export struct Coverage {
    fn isJSCoverageActive(): bool;
    fn isCSSCoverageActive(): bool;
    fn startJSCoverage(*_, opts: JSCoverageOptions): void!PlaywrightError;
    fn stopJSCoverage(*_, alloc: std.mem.Allocator): ([]JSCoverageEntry)!PlaywrightError;
    fn startCSSCoverage(*_, opts: CSSCoverageOptions): void!PlaywrightError;
    fn stopCSSCoverage(*_, alloc: std.mem.Allocator): ([]CSSCoverageEntry)!PlaywrightError;
}

export struct ProxySettings {
    server: []const u8,
    bypass: []const u8,
    username: []const u8,
    password: []const u8,
    fn shouldBypass(host: []const u8): bool;
    fn buildProxyAuthHeader(out_buf: []u8): ([]const u8)!PlaywrightError;
}

export struct MultipartField {
    name: []const u8,
    value: []const u8,
    filename: []const u8,
    content_type: []const u8,
}

export struct MultipartFormData {
    boundary: []const u8,
    static fn init(alloc: std.mem.Allocator): MultipartFormData;
    static fn initWithBoundary(alloc: std.mem.Allocator, seed: u64): MultipartFormData!PlaywrightError;
    fn deinit(*_);
    fn addField(*_, name: []const u8, value: []const u8): void!PlaywrightError;
    fn addFile(*_, name: []const u8, filename: []const u8, content_type: []const u8, data: []const u8): void!PlaywrightError;
    fn contentTypeHeader(out_buf: []u8): ([]const u8)!PlaywrightError;
    fn encode(alloc: std.mem.Allocator): ([]u8)!PlaywrightError;
}

export struct RedirectHop {
    from_url: []const u8,
    to_url: []const u8,
    status: i32,
    method: []const u8,
}

export struct FetchTransportResult {
    status: i32,
    status_text: []const u8,
    headers_json: []const u8,
    body: []const u8,
}

export struct APIRequestContext {
    static fn init(alloc: std.mem.Allocator, opts: APIRequestOptions): APIRequestContext;
    fn deinit(*_);
    fn dispose(*_);
    fn setTransport(*_, cb: FetchTransportFn, ctx: usize);
    fn addCookie(*_, c: Cookie): void!PlaywrightError;
    fn cookies(): []const Cookie;
    fn syncCookiesFromContext(*_, ctx: *BrowserContext): void!PlaywrightError;
    fn syncCookiesToContext(*_, ctx: *BrowserContext): void!PlaywrightError;
    static fn isRedirectStatus(status: i32): bool;
    static fn resolveRedirectMethod(status: i32, current_method: []const u8): []const u8;
    fn resolveRedirectUrl(*_, current_url: []const u8, location: []const u8): ([]const u8)!PlaywrightError;
    fn fetch(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn get(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn post(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn put(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn delete(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
}

export enum FrameLifecycleState {
    Commit,
    DomContentLoaded,
    Load,
    NetworkIdle,
}

export struct ManagedFrame {
    frame_id: []const u8,
    parent_id: []const u8,
    name: []const u8,
    url: []const u8,
    loader_id: []const u8,
    detached: bool,
    lifecycle_mask: u8,
    inflight_requests: usize,
    navigation_count: u32,
    fn hasReached(state: FrameLifecycleState): bool;
    fn isMainFrame(): bool;
}

export struct FrameManager {
    static fn init(alloc: std.mem.Allocator): FrameManager;
    fn deinit(*_);
    fn onFrameAttached(*_, frame_id: []const u8, parent_frame_id: []const u8): (*ManagedFrame)!PlaywrightError;
    fn onFrameNavigated(*_, frame_id: []const u8, parent_frame_id: []const u8, frame_name: []const u8, url: []const u8, loader_id: []const u8, now_ms: i64): (*ManagedFrame)!PlaywrightError;
    fn onFrameDetached(*_, frame_id: []const u8): usize;
    fn onLifecycleEvent(*_, frame_id: []const u8, loader_id: []const u8, event_name: []const u8, now_ms: i64): bool;
    fn onRequestStarted(*_, frame_id: []const u8, now_ms: i64);
    fn onRequestFinished(*_, frame_id: []const u8, now_ms: i64);
    fn checkNetworkIdle(*_, frame_id: []const u8, now_ms: i64): bool;
    fn handleCdpEvent(*_, method: []const u8, params_json: []const u8, now_ms: i64): bool!PlaywrightError;
    fn mainFrame(*_): (*ManagedFrame)?;
    fn findFrame(*_, frame_id: []const u8): (*ManagedFrame)?;
    fn findFrameByName(*_, target_name: []const u8): (*ManagedFrame)?;
    fn findFrameByUrl(*_, url_pattern: []const u8): (*ManagedFrame)?;
    fn childFrameIds(alloc: std.mem.Allocator, parent_id: []const u8): ([][]const u8)!PlaywrightError;
    fn activeFrameCount(): usize;
    fn isLifecycleReached(frame_id: []const u8, state: FrameLifecycleState): bool;
}

export struct ActionabilityProbeSnapshot {
    attached: bool,
    visible: bool,
    enabled: bool,
    editable: bool,
    in_viewport: bool,
    hit_target_ok: bool,
    box: ElementBox,
    obscured_by: []const u8,
}

export struct DomActionabilityLoop {
    static fn backoffDelayMs(attempt: u32): u32;
    static fn run(selector: []const u8, checks: []const ActionabilityCheckKind, opts: ActionabilityOptions, probe_fn: ActionabilityProbeFn, probe_ctx: usize): ActionabilityReport!PlaywrightError;
    static fn runOverCdp(endpoint: *CdpEndpoint, session_id: []const u8, selector: []const u8, checks: []const ActionabilityCheckKind, opts: ActionabilityOptions): ActionabilityReport!PlaywrightError;
}

export enum CrNetworkRequestState {
    Pending,
    Paused,
    Continued,
    Fulfilled,
    Aborted,
    ResponseReceived,
    Finished,
    Failed,
}

export enum CrAuthResponseAction {
    Default,
    CancelAuth,
    ProvideCredentials,
}

export struct CrAuthChallenge {
    source: []const u8,
    origin: []const u8,
    scheme: []const u8,
    realm: []const u8,
}

export struct CrManagedRequest {
    request_id: []const u8,
    interception_id: []const u8,
    frame_id: []const u8,
    url: []const u8,
    method: []const u8,
    resource_type: []const u8,
    status: i32,
    state: CrNetworkRequestState,
    redirected_from_id: []const u8,
    redirected_to_id: []const u8,
    redirect_count: u32,
}

export struct CrNetworkManager {
    static fn init(alloc: std.mem.Allocator): CrNetworkManager;
    static fn initWithEndpoint(alloc: std.mem.Allocator, endpoint: *CdpEndpoint, session_id: []const u8): CrNetworkManager;
    fn deinit(*_);
    fn enableInterception(*_): void!PlaywrightError;
    fn addRoute(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError;
    fn removeRoute(*_, pattern: []const u8);
    fn setHttpCredentials(*_, creds: HTTPCredentials?): void!PlaywrightError;
    fn findRequest(*_, request_id: []const u8): (*CrManagedRequest)?;
    fn requestCount(): usize;
    fn onRequestWillBeSent(*_, request_id: []const u8, frame_id: []const u8, url: []const u8, method: []const u8, resource_type: []const u8, headers_json: []const u8, post_data: []const u8, redirect_status: i32): (*CrManagedRequest)!PlaywrightError;
    fn onFetchRequestPaused(*_, interception_id: []const u8, request_id: []const u8, url: []const u8, method: []const u8, resource_type: []const u8, headers_json: []const u8, post_data: []const u8): RouteAction!PlaywrightError;
    fn onFetchAuthRequired(*_, interception_id: []const u8, challenge: CrAuthChallenge): CrAuthResponseAction!PlaywrightError;
    fn onResponseReceived(*_, request_id: []const u8, status: i32, status_text: []const u8, mime_type: []const u8, response_headers_json: []const u8, timing: ResourceTiming): bool!PlaywrightError;
    fn onLoadingFinished(*_, request_id: []const u8, encoded_data_length: i64): bool;
    fn onLoadingFailed(*_, request_id: []const u8, error_text: []const u8, canceled: bool): bool!PlaywrightError;
    fn handleCdpEvent(*_, method: []const u8, params_json: []const u8): bool!PlaywrightError;
}

export struct SnapshotResource {
    sha1_hex: [40]u8,
    fnv64: u64,
    content_type: []const u8,
    body: []const u8,
    ref_count: u32,
}

export struct FrameSnapshot {
    snapshot_name: []const u8,
    call_id: []const u8,
    timestamp_ms: i64,
    page_id: []const u8,
    frame_id: []const u8,
    frame_url: []const u8,
    doctype: []const u8,
    html_json: []const u8,
    viewport_width: u32,
    viewport_height: u32,
}

export struct Snapshotter {
    static fn init(alloc: std.mem.Allocator): Snapshotter;
    fn deinit(*_);
    fn injectIntoEndpoint(*_, endpoint: *CdpEndpoint, session_id: []const u8): void!PlaywrightError;
    fn addResource(*_, content_type: []const u8, body: []const u8): ([40]u8)!PlaywrightError;
    fn captureSnapshot(*_, page_id: []const u8, frame_id: []const u8, frame_url: []const u8, snapshot_name: []const u8, call_id: []const u8, html_json: []const u8, viewport_w: u32, viewport_h: u32, timestamp_ms: i64): (*FrameSnapshot)!PlaywrightError;
    fn exportToTracing(*_, tracing: *Tracing): usize!PlaywrightError;
    fn snapshotCount(): usize;
    fn uniqueResourceCount(): usize;
    fn deduplicatedHits(): usize;
}

export struct HarTimings {
    blocked: f64,
    dns: f64,
    connect: f64,
    ssl: f64,
    send: f64,
    wait: f64,
    receive: f64,
    static fn fromResourceTiming(rt: ResourceTiming): HarTimings;
    fn totalTimeMs(): f64;
}

export struct HarTracerOptions {
    url_glob: []const u8,
    minimal: bool,
    omit_content: bool,
    base64_binary_bodies: bool,
}

export struct HarTracerEntry {
    started_date_time: []const u8,
    page_ref: []const u8,
    method: []const u8,
    url: []const u8,
    http_version: []const u8,
    status: i32,
    status_text: []const u8,
    mime_type: []const u8,
    request_headers_json: []const u8,
    response_headers_json: []const u8,
    post_data: []const u8,
    response_body: []const u8,
    response_is_binary: bool,
    body_size: usize,
    timings: HarTimings,
    server_ip_address: []const u8,
}

export struct HarTracer {
    static fn init(alloc: std.mem.Allocator, opts: HarTracerOptions): HarTracer;
    fn deinit(*_);
    fn recordExchange(*_, entry: HarTracerEntry): bool!PlaywrightError;
    fn recordFromRequestResponse(*_, req: Request, resp: Response, started_date_time: []const u8): bool!PlaywrightError;
    fn entryCount(): usize;
    fn toJson(*_, alloc: std.mem.Allocator): ([]u8)!PlaywrightError;
}

export struct WebSocketRoute {
    fn connectToServer(*_);
    fn send(*_, message: []const u8): void!PlaywrightError;
    fn sendToServer(*_, message: []const u8): void!PlaywrightError;
    fn close(*_, code: u16, reason: []const u8);
    fn isClosed(): bool;
}

export struct Video {
    fn start(*_, format: []const u8, quality: u8): void!PlaywrightError;
    fn recordFrame(*_, base64_data: []const u8, ack_session_id: i64): void!PlaywrightError;
    fn stop(*_): void!PlaywrightError;
    fn frameCount(): usize;
    fn path(): ([]const u8)?;
    fn saveAs(*_, dest_path: []const u8): void!PlaywrightError;
}
