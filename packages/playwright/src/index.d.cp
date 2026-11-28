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

export struct FulfillOptions {
    status: i32,
    content_type: []const u8,
    body: []const u8,
    headers_json: []const u8,
}

export struct Request {
    url: []const u8,
    method: []const u8,
    resource_type: []const u8,
    post_data: []const u8,
    headers_json: []const u8,
    is_navigation_request: bool,
    fn isNavigationRequest(): bool;
}

export struct Response {
    url: []const u8,
    status: i32,
    status_text: []const u8,
    ok: bool,
    headers_json: []const u8,
    body: []const u8,
    fn isOk(): bool;
}

export struct Route {
    request: Request,
    action: RouteAction,
    fn isHandled(): bool;
    fn fulfill(*_, opts: FulfillOptions): void!PlaywrightError;
    fn abort(*_, error_code: []const u8): void!PlaywrightError;
    fn continue_(*_, override_url: []const u8, override_method: []const u8): void!PlaywrightError;
    fn fallback(*_): void!PlaywrightError;
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

export struct Page {
    fn url(): []const u8;
    fn title(): []const u8;
    fn isClosed(): bool;
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
    fn fill(*_, selector: []const u8, value: []const u8): void!PlaywrightError;
    fn typeText(*_, selector: []const u8, text: []const u8): void!PlaywrightError;
    fn press(*_, selector: []const u8, key: []const u8): void!PlaywrightError;
    fn waitForSelector(*_, selector: []const u8, opts: WaitForOptions): Locator!PlaywrightError;
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

export struct BrowserContext {
    fn newPage(*_): (*Page)!PlaywrightError;
    fn pages(): []*Page;
    fn tracing(*_): *Tracing;
    fn cookies(): []const Cookie;
    fn addCookies(*_, new_cookies: []const Cookie): void!PlaywrightError;
    fn clearCookies(*_);
    fn storageState(alloc: std.mem.Allocator): StorageState!PlaywrightError;
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
    fn isDetached(): bool;
    fn parentFrame(): (*Frame)?;
    fn childFrames(): []*Frame;
    fn addChildFrame(*_, child_frame_id: []const u8, child_name: []const u8, child_url: []const u8): (*Frame)!PlaywrightError;
    fn frameByName(*_, target_name: []const u8): (*Frame)?;
    fn frameByUrl(*_, url_pattern: []const u8): (*Frame)?;
    fn locator(selector: []const u8): Locator;
    fn goto(*_, target_url: []const u8, referer: []const u8): Response!PlaywrightError;
    fn evaluate(*_, expression: []const u8): ([]const u8)!PlaywrightError;
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

export struct Coverage {
    fn isJSCoverageActive(): bool;
    fn isCSSCoverageActive(): bool;
    fn startJSCoverage(*_, opts: JSCoverageOptions): void!PlaywrightError;
    fn stopJSCoverage(*_, alloc: std.mem.Allocator): ([]JSCoverageEntry)!PlaywrightError;
    fn startCSSCoverage(*_, opts: CSSCoverageOptions): void!PlaywrightError;
    fn stopCSSCoverage(*_, alloc: std.mem.Allocator): ([]CSSCoverageEntry)!PlaywrightError;
}

export struct APIRequestContext {
    static fn init(alloc: std.mem.Allocator, opts: APIRequestOptions): APIRequestContext;
    fn deinit(*_);
    fn dispose(*_);
    fn addCookie(*_, c: Cookie): void!PlaywrightError;
    fn cookies(): []const Cookie;
    fn syncCookiesFromContext(*_, ctx: *BrowserContext): void!PlaywrightError;
    fn syncCookiesToContext(*_, ctx: *BrowserContext): void!PlaywrightError;
    fn fetch(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn get(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn post(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn put(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
    fn delete(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError;
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
