// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/index.ts | Synced: 2026-11-28
// Type definitions for puppeteer (CP 25.19.0)

export error PuppeteerError {
    NotConnected,
    BrowserLaunchFailed,
    ExecutableNotFound,
    SessionClosed,
    TargetNotFound,
    NavigationFailed,
    Timeout,
    EvaluationFailed,
    ElementNotFound,
    ProtocolError,
    InvalidArgument,
    BufferTooSmall,
    OutOfMemory,
}

export enum WaitUntil { Load, DomContentLoaded, NetworkIdle0, NetworkIdle2 }
export enum DialogType { Alert, Confirm, Prompt, BeforeUnload }
export enum ResourceType { Document, Stylesheet, Image, Media, Font, Script, TextTrack, XHR, Fetch, EventSource, WebSocket, Manifest, Other }
export enum TargetType { Page, BackgroundPage, ServiceWorker, SharedWorker, Browser, Webview, Other }
export enum InterceptResolutionAction { Abort, Respond, Continue, None }

export struct Viewport {
    width: u32,
    height: u32,
    device_scale_factor: f64,
    is_mobile: bool,
    has_touch: bool,
    is_landscape: bool,
}

export struct LaunchOptions {
    headless: bool,
    executable_path: []const u8,
    user_data_dir: []const u8,
    args: []const []const u8,
    viewport_width: u32,
    viewport_height: u32,
    timeout_ms: u32,
    remote_debugging_port: u16,
    ignore_https_errors: bool,
    default_viewport_enabled: bool,
}

export struct ConnectOptions {
    browser_ws_url: []const u8,
    ignore_https_errors: bool,
    default_viewport_width: u32,
    default_viewport_height: u32,
    timeout_ms: u32,
}

export struct GoToOptions { timeout_ms: u32, wait_until: WaitUntil, referer: []const u8 }
export struct ScreenshotOptions { format: []const u8, quality: u8, full_page: bool, from_surface: bool, omit_background: bool }
export struct PdfOptions { landscape: bool, print_background: bool, scale: f64, paper_width: f64, paper_height: f64, prefer_css_page_size: bool }
export struct ClickOptions { button: []const u8, click_count: i32, delay_ms: u32 }
export struct TypeOptions { delay_ms: u32 }
export struct WaitForSelectorOptions { visible: bool, hidden: bool, timeout_ms: u32 }
export struct AccessibilitySnapshotOptions { interesting_only: bool, depth: i32, frame_id: []const u8 }

export struct Cookie {
    name: []const u8,
    value: []const u8,
    domain: []const u8,
    path: []const u8,
    expires: f64,
    size: i64,
    http_only: bool,
    secure: bool,
    session: bool,
    same_site: []const u8,
}

export struct CookieParam {
    name: []const u8,
    value: []const u8,
    url: []const u8,
    domain: []const u8,
    path: []const u8,
    http_only: bool,
    secure: bool,
    same_site: []const u8,
    expires: f64,
}

export struct HttpHeader { name: []const u8, value: []const u8 }
export struct BoundingBox { x: f64, y: f64, width: f64, height: f64 }
export struct BoxModel { content: BoundingBox, padding: BoundingBox, border: BoundingBox, margin: BoundingBox, width: i64, height: i64 }
export struct RemoteAddress { ip: []const u8, port: u16 }
export struct ResponseTiming {
    request_time: f64,
    dns_start: f64,
    dns_end: f64,
    connect_start: f64,
    connect_end: f64,
    ssl_start: f64,
    ssl_end: f64,
    send_start: f64,
    send_end: f64,
    receive_headers_end: f64,
}

export struct TargetInfo {
    target_id: []const u8,
    target_type: TargetType,
    title: []const u8,
    url: []const u8,
    attached: bool,
    browser_context_id: []const u8,
}

export struct EvaluateResult {
    value_json: []const u8,
    value_type: []const u8,
    description: []const u8,
    exception_text: []const u8,
    has_exception: bool,
}

export struct AccessibilitySnapshot {
    nodes: [64]AXNodeSummary,
    count: usize,
    pub fn slice(): []const AXNodeSummary;
}

export struct MediaFeature { name: []const u8, value: []const u8 }
export struct GeolocationOptions { latitude: f64, longitude: f64, accuracy: f64 }
export struct NetworkConditions { offline: bool, download_throughput: f64, upload_throughput: f64, latency: f64 }
export struct Credentials { username: []const u8, password: []const u8 }
export struct PageMetrics {
    timestamp: f64,
    documents: f64,
    frames: f64,
    js_event_listeners: f64,
    nodes: f64,
    layout_count: f64,
    recalc_style_count: f64,
    layout_duration: f64,
    recalc_style_duration: f64,
    script_duration: f64,
    task_duration: f64,
    js_heap_used_size: f64,
    js_heap_total_size: f64,
}
export struct WebWorkerInfo { url: []const u8, target_id: []const u8 }
export struct WaitForNetworkIdleOptions { idle_time_ms: u32, concurrency: u32, timeout_ms: u32 }
export struct WaitForTargetOptions { timeout_ms: u32 }
export struct DebugInfo { pending_protocol_errors_count: usize, active_sessions_count: usize, connected: bool }
export struct RequestInitiator { kind: []const u8, url: []const u8, line_number: u32, column_number: u32 }
export struct ContinueRequestOverrides { url: []const u8, method: []const u8, post_data: []const u8, headers: []const HttpHeader }
export enum LocatorVisibility { Any, Visible, Hidden }
export struct LocatorScrollOptions { scroll_top: f64, scroll_left: f64 }

export struct CDPSession {
    pub fn id(): []const u8;
    pub fn targetType(): []const u8;
    pub fn targetId(): []const u8;
    pub fn parentSessionId(): []const u8;
    pub fn detached(): bool;
    pub fn send(*_, method: []const u8, params_json: []const u8, out_buf: []u8): i64!PuppeteerError;
    pub fn sendAndTakeResponse(*_, method: []const u8, params_json: []const u8, out_buf: []u8): CdpResponse!PuppeteerError;
    pub fn on(*_, method_filter: []const u8, callback: CdpEventCallback, ctx: usize): void!PuppeteerError;
    pub fn detach(*_): void!PuppeteerError;
}

export struct Keyboard {
    pub fn down(*_, key: []const u8): void!PuppeteerError;
    pub fn up(*_, key: []const u8): void!PuppeteerError;
    pub fn press(*_, key: []const u8, options: TypeOptions): void!PuppeteerError;
    pub fn sendCharacter(*_, ch: []const u8): void!PuppeteerError;
    pub fn typeText(*_, text: []const u8, options: TypeOptions): void!PuppeteerError;
}

export struct Mouse {
    pub fn move(*_, x: f64, y: f64): void!PuppeteerError;
    pub fn down(*_, options: ClickOptions): void!PuppeteerError;
    pub fn up(*_, options: ClickOptions): void!PuppeteerError;
    pub fn click(*_, x: f64, y: f64, options: ClickOptions): void!PuppeteerError;
    pub fn dblclick(*_, x: f64, y: f64): void!PuppeteerError;
    pub fn wheel(*_, delta_x: f64, delta_y: f64): void!PuppeteerError;
    pub fn reset(*_): void!PuppeteerError;
}

export struct Touchscreen {
    pub fn touchStart(*_, x: f64, y: f64): void!PuppeteerError;
    pub fn touchMove(*_, x: f64, y: f64): void!PuppeteerError;
    pub fn touchEnd(*_): void!PuppeteerError;
    pub fn tap(*_, x: f64, y: f64): void!PuppeteerError;
}

export struct HTTPRequest {
    pub fn id(): []const u8;
    pub fn url(): []const u8;
    pub fn method(): []const u8;
    pub fn hasPostData(): bool;
    pub fn postData(): []const u8;
    pub fn setPostData(*_, data: []const u8);
    pub fn resourceType(): ResourceType;
    pub fn frameId(): []const u8;
    pub fn isNavigationRequest(): bool;
    pub fn headers(): []const HttpHeader;
    pub fn redirectChain(): []const []const u8;
    pub fn pushRedirectUrl(*_, redirect_url: []const u8);
    pub fn failure(): ([]const u8)?;
    pub fn initiator(): RequestInitiator;
    pub fn setInitiator(*_, init_info: RequestInitiator);
    pub fn response(): (*HTTPResponse)?;
    pub fn setResponse(*_, resp: *HTTPResponse);
    pub fn isInterceptResolutionHandled(): bool;
    pub fn interceptResolutionState(): InterceptResolutionState;
    pub fn continueRequest(*_, override_url: []const u8, override_method: []const u8): void!PuppeteerError;
    pub fn continueWithOverrides(*_, overrides: ContinueRequestOverrides): void!PuppeteerError;
    pub fn respond(*_, status: u16, content_type: []const u8, body: []const u8): void!PuppeteerError;
    pub fn abort(*_, error_reason: []const u8): void!PuppeteerError;
}

export struct HTTPResponse {
    pub fn url(): []const u8;
    pub fn status(): u16;
    pub fn statusText(): []const u8;
    pub fn ok(): bool;
    pub fn headers(): []const HttpHeader;
    pub fn remoteAddress(): RemoteAddress;
    pub fn timing(): ResponseTiming;
    pub fn fromCache(): bool;
    pub fn fromServiceWorker(): bool;
    pub fn securityDetails(): SecurityDetails?;
    pub fn setSecurityDetails(*_, details: SecurityDetails);
    pub fn request(): (*HTTPRequest)?;
    pub fn setRequest(*_, req: *HTTPRequest);
    pub fn encodedDataLength(): usize;
    pub fn buffer(): []const u8;
    pub fn text(): []const u8;
    pub fn json(): ([]const u8)!PuppeteerError;
}

export struct Dialog {
    pub fn kind(): DialogType;
    pub fn message(): []const u8;
    pub fn defaultValue(): []const u8;
    pub fn accept(*_, prompt_text: []const u8): void!PuppeteerError;
    pub fn dismiss(*_): void!PuppeteerError;
}

export struct JSHandle {
    pub fn id(): []const u8;
    pub fn remoteType(): []const u8;
    pub fn jsonValue(): []const u8;
    pub fn isDisposed(): bool;
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn getProperty(*_, property_name: []const u8): JSHandle!PuppeteerError;
    pub fn dispose(*_): void!PuppeteerError;
}

export struct ElementHandle {
    pub fn nodeId(): i64;
    pub fn isDisposed(): bool;
    pub fn boundingBox(*_): BoundingBox?;
    pub fn boxModel(*_): BoxModel?;
    pub fn scrollIntoViewIfNeeded(*_): void!PuppeteerError;
    pub fn focus(*_): void!PuppeteerError;
    pub fn hover(*_): void!PuppeteerError;
    pub fn click(*_, options: ClickOptions): void!PuppeteerError;
    pub fn tap(*_): void!PuppeteerError;
    pub fn typeText(*_, text: []const u8, options: TypeOptions): void!PuppeteerError;
    pub fn press(*_, key: []const u8, options: TypeOptions): void!PuppeteerError;
    pub fn querySelector(*_, sub_selector: []const u8): (ElementHandle?)!PuppeteerError;
    pub fn screenshot(*_, options: ScreenshotOptions): ([]const u8)!PuppeteerError;
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn dispose(*_): void!PuppeteerError;
}

export struct PuppeteerLocator {
    pub static fn init(session: *CDPSession, selector_expr: []const u8): PuppeteerLocator;
    pub fn selector(): []const u8;
    pub fn timeout(): u32;
    pub fn ensureElementIsInTheViewport(): bool;
    pub fn visibility(): LocatorVisibility;
    pub fn waitForEnabled(): bool;
    pub fn waitForStableBoundingBox(): bool;
    pub fn filterExpression(): []const u8;
    pub fn setTimeout(timeout_ms: u32): PuppeteerLocator;
    pub fn setEnsureElementIsInTheViewport(value: bool): PuppeteerLocator;
    pub fn setVisibility(vis: LocatorVisibility): PuppeteerLocator;
    pub fn setWaitForEnabled(value: bool): PuppeteerLocator;
    pub fn setWaitForStableBoundingBox(value: bool): PuppeteerLocator;
    pub fn filter(predicate_js: []const u8): PuppeteerLocator;
    pub fn wait(*_): ElementHandle!PuppeteerError;
    pub fn click(*_, options: ClickOptions): void!PuppeteerError;
    pub fn fill(*_, value: []const u8): void!PuppeteerError;
    pub fn hover(*_): void!PuppeteerError;
    pub fn scroll(*_, options: LocatorScrollOptions): void!PuppeteerError;
}

export struct Frame {
    pub fn id(): []const u8;
    pub fn parentFrameId(): []const u8;
    pub fn parentFrame(): (*Frame)?;
    pub fn childFrames(): []const *Frame;
    pub fn addChildFrame(*_, child: *Frame);
    pub fn name(): []const u8;
    pub fn setName(*_, new_name: []const u8);
    pub fn url(): []const u8;
    pub fn isDetached(): bool;
    pub fn goto(*_, target_url: []const u8, options: GoToOptions): void!PuppeteerError;
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn evalOnSelector(*_, selector: []const u8, page_function: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn evalOnSelectorAll(*_, selector: []const u8, page_function: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn title(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn content(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn setContent(*_, html: []const u8): void!PuppeteerError;
    pub fn querySelector(*_, selector: []const u8): ElementHandle!PuppeteerError;
    pub fn querySelectorAll(*_, selector: []const u8, out_handles: []ElementHandle): usize!PuppeteerError;
    pub fn waitForSelector(*_, selector: []const u8, options: WaitForSelectorOptions): ElementHandle!PuppeteerError;
    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn waitForNavigation(*_, options: GoToOptions): void!PuppeteerError;
    pub fn click(*_, selector: []const u8, options: ClickOptions): void!PuppeteerError;
    pub fn focus(*_, selector: []const u8): void!PuppeteerError;
    pub fn hover(*_, selector: []const u8): void!PuppeteerError;
    pub fn tap(*_, selector: []const u8): void!PuppeteerError;
    pub fn typeText(*_, selector: []const u8, text: []const u8, options: TypeOptions): void!PuppeteerError;
    pub fn select(*_, selector: []const u8, values: []const []const u8, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn locator(*_, selector: []const u8): PuppeteerLocator;
    pub fn addScriptTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError;
    pub fn addStyleTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError;
    pub fn accessibilitySnapshot(*_, options: AccessibilitySnapshotOptions): AccessibilitySnapshot!PuppeteerError;
}

export struct Page {
    pub fn isClosed(): bool;
    pub fn mainFrame(*_): *Frame;
    pub fn frames(*_): []const *Frame;
    pub fn workers(): []const WebWorkerInfo;
    pub fn registerWorker(*_, info: WebWorkerInfo);
    pub fn keyboard(*_): *Keyboard;
    pub fn mouse(*_): *Mouse;
    pub fn touchscreen(*_): *Touchscreen;
    pub fn createCDPSession(*_, child_session_id: []const u8): CDPSession!PuppeteerError;
    pub fn url(): []const u8;
    pub fn title(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn content(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn setContent(*_, html: []const u8): void!PuppeteerError;
    pub fn goto(*_, target_url: []const u8, options: GoToOptions): void!PuppeteerError;
    pub fn bringToFront(*_): void!PuppeteerError;
    pub fn goBack(*_): void!PuppeteerError;
    pub fn goForward(*_): void!PuppeteerError;
    pub fn reload(*_, ignore_cache: bool): void!PuppeteerError;
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn evalOnSelector(*_, selector: []const u8, page_function: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn evalOnSelectorAll(*_, selector: []const u8, page_function: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn evaluateOnNewDocument(*_, script: []const u8): void!PuppeteerError;
    pub fn setViewport(*_, vp: Viewport): void!PuppeteerError;
    pub fn viewport(): Viewport;
    pub fn emulate(*_, device: Device): void!PuppeteerError;
    pub fn emulateMediaType(*_, media_type: []const u8): void!PuppeteerError;
    pub fn emulateMediaFeatures(*_, features: []const MediaFeature): void!PuppeteerError;
    pub fn emulateTimezone(*_, timezone_id: []const u8): void!PuppeteerError;
    pub fn emulateVisionDeficiency(*_, vision_type: []const u8): void!PuppeteerError;
    pub fn emulateCPUThrottling(*_, factor: f64): void!PuppeteerError;
    pub fn emulateNetworkConditions(*_, conditions: NetworkConditions): void!PuppeteerError;
    pub fn setGeolocation(*_, options: GeolocationOptions): void!PuppeteerError;
    pub fn setOfflineMode(*_, enabled: bool): void!PuppeteerError;
    pub fn setCacheEnabled(*_, enabled: bool): void!PuppeteerError;
    pub fn setBypassCSP(*_, enabled: bool): void!PuppeteerError;
    pub fn setJavaScriptEnabled(*_, enabled: bool): void!PuppeteerError;
    pub fn isJavaScriptEnabled(): bool;
    pub fn setBypassServiceWorker(*_, bypass: bool): void!PuppeteerError;
    pub fn isServiceWorkerBypassed(): bool;
    pub fn setDragInterception(*_, enabled: bool): void!PuppeteerError;
    pub fn authenticate(*_, credentials: Credentials?): void!PuppeteerError;
    pub fn screenshot(*_, options: ScreenshotOptions, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn pdf(*_, options: PdfOptions, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn querySelector(*_, selector: []const u8): ElementHandle!PuppeteerError;
    pub fn querySelectorAll(*_, selector: []const u8, out_handles: []ElementHandle): usize!PuppeteerError;
    pub fn waitForSelector(*_, selector: []const u8, options: WaitForSelectorOptions): ElementHandle!PuppeteerError;
    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn waitForNavigation(*_, options: GoToOptions): void!PuppeteerError;
    pub fn waitForRequest(*_, url_pattern: []const u8, timeout_ms: u32): HTTPRequest!PuppeteerError;
    pub fn waitForResponse(*_, url_pattern: []const u8, timeout_ms: u32): HTTPResponse!PuppeteerError;
    pub fn waitForFrame(*_, url_or_name: []const u8, timeout_ms: u32): (*Frame)!PuppeteerError;
    pub fn waitForNetworkIdle(*_, options: WaitForNetworkIdleOptions): void!PuppeteerError;
    pub fn waitForFileChooser(*_, timeout_ms: u32): FileChooser!PuppeteerError;
    pub fn recordRequest(*_, req: HTTPRequest);
    pub fn recordResponse(*_, resp: HTTPResponse);
    pub fn click(*_, selector: []const u8, options: ClickOptions): void!PuppeteerError;
    pub fn typeText(*_, selector: []const u8, text: []const u8, options: TypeOptions): void!PuppeteerError;
    pub fn focus(*_, selector: []const u8): void!PuppeteerError;
    pub fn hover(*_, selector: []const u8): void!PuppeteerError;
    pub fn tap(*_, selector: []const u8): void!PuppeteerError;
    pub fn select(*_, selector: []const u8, values: []const []const u8, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn addScriptTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError;
    pub fn addStyleTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError;
    pub fn exposeFunction(*_, name: []const u8): void!PuppeteerError;
    pub fn removeExposedFunction(*_, name: []const u8): void!PuppeteerError;
    pub fn metrics(*_): PageMetrics!PuppeteerError;
    pub fn locator(*_, selector: []const u8): PuppeteerLocator;
    pub fn setExtraHttpHeaders(*_, headers: []const HttpHeader): void!PuppeteerError;
    pub fn setRequestInterception(*_, enabled: bool): void!PuppeteerError;
    pub fn setUserAgent(*_, user_agent: []const u8): void!PuppeteerError;
    pub fn setCookie(*_, cookie_list: []const CookieParam): void!PuppeteerError;
    pub fn cookies(*_): []const Cookie;
    pub fn deleteCookie(*_, name: []const u8, domain: []const u8): void!PuppeteerError;
    pub fn accessibilitySnapshot(*_, options: AccessibilitySnapshotOptions): AccessibilitySnapshot!PuppeteerError;
    pub fn tracing(*_): *Tracing;
    pub fn coverage(*_): *Coverage;
    pub fn setDefaultTimeout(*_, timeout_ms: u32);
    pub fn setDefaultNavigationTimeout(*_, timeout_ms: u32);
    pub fn getDefaultTimeout(): u32;
    pub fn getDefaultNavigationTimeout(): u32;
    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn);
    pub fn emit(*_, event_name: []const u8, payload: []const u8): bool;
    pub fn close(*_): void!PuppeteerError;
}

export struct BrowserContext {
    pub fn id(): []const u8;
    pub fn isIncognito(): bool;
    pub fn isClosed(): bool;
    pub fn pages(): []const *Page;
    pub fn targets(): []const TargetInfo;
    pub fn cookies(): []const Cookie;
    pub fn setCookie(*_, cookie_list: []const CookieParam): void!PuppeteerError;
    pub fn deleteCookie(*_, name: []const u8, domain: []const u8): void!PuppeteerError;
    pub fn overridePermissions(*_, origin: []const u8, permissions_json: []const u8): void!PuppeteerError;
    pub fn clearPermissionOverrides(*_): void!PuppeteerError;
    pub fn close(*_): void!PuppeteerError;
}

export struct Browser {
    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize);
    pub fn feedIncoming(*_, payload: []const u8): void!PuppeteerError;
    pub fn wsEndpoint(): []const u8;
    pub fn executablePath(): []const u8;
    pub fn isConnected(): bool;
    pub fn connected(): bool;
    pub fn defaultBrowserContext(*_): *BrowserContext;
    pub fn browserContexts(): []const BrowserContext;
    pub fn createBrowserContext(*_): BrowserContext!PuppeteerError;
    pub fn newPage(*_): (*Page)!PuppeteerError;
    pub fn pages(): []*Page;
    pub fn targets(): []const TargetInfo;
    pub fn target(): TargetInfo;
    pub fn waitForTarget(*_, url_or_id_substr: []const u8, options: WaitForTargetOptions): TargetInfo!PuppeteerError;
    pub fn debugInfo(): DebugInfo;
    pub fn cookies(*_): []const Cookie;
    pub fn setCookie(*_, cookie_list: []const CookieParam): void!PuppeteerError;
    pub fn deleteCookie(*_, name: []const u8, domain: []const u8): void!PuppeteerError;
    pub fn version(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn userAgent(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn disconnect(*_);
    pub fn close(*_): void!PuppeteerError;
}

export fn launch(alloc: std.mem.Allocator, options: LaunchOptions): Browser!PuppeteerError;
export fn launchWithTransport(alloc: std.mem.Allocator, options: LaunchOptions, ws_url: []const u8, transport_fn: CdpSendTransportFn, transport_ctx: usize): Browser!PuppeteerError;
export fn connect(alloc: std.mem.Allocator, browser_ws_url: []const u8): Browser!PuppeteerError;
export fn connectWithTransport(alloc: std.mem.Allocator, options: ConnectOptions, transport_fn: CdpSendTransportFn, transport_ctx: usize): Browser!PuppeteerError;
export fn defaultExecutablePath(alloc: std.mem.Allocator): ([]u8)?;

// --- Devices, Keyboard Layout, P-Selectors, Events & Extras ---
export struct Device {
    name: []const u8,
    user_agent: []const u8,
    viewport: Viewport,
}
export struct KnownDevices {
    pub static fn count(): usize;
    pub static fn all(): []const Device;
    pub static fn get(name: []const u8): Device?;
    pub static fn iPhone12(): Device;
    pub static fn iPhone13(): Device;
    pub static fn iPhone14(): Device;
    pub static fn iPhone15Pro(): Device;
    pub static fn iPhone16Pro(): Device;
    pub static fn iPhone17Pro(): Device;
    pub static fn iPad(): Device;
    pub static fn iPadPro(): Device;
    pub static fn pixel5(): Device;
    pub static fn desktop1080p(): Device;
}

export struct KeyDefinition {
    name: []const u8,
    key_code: u16,
    shift_key_code: u16,
    key: []const u8,
    shift_key: []const u8,
    code: []const u8,
    text: []const u8,
    shift_text: []const u8,
    location: u8,
}
export struct USKeyboardLayout {
    pub static fn count(): usize;
    pub static fn get(key_input: []const u8): KeyDefinition?;
    pub static fn resolve(key_input: []const u8, shift_down: bool): KeyDefinition;
}

export enum PSelectorKind { Css, Text, XPath, Aria, Pierce, Custom }
export enum PCombinator { None, Descendent, Child }
export enum WaitTaskPolling { Raf, Mutation, Interval }
export struct PSelectorSegment {
    kind: PSelectorKind,
    combinator: PCombinator,
    name: []const u8,
    value: []const u8,
}
export struct ParsedPSelector {
    segments: [16]PSelectorSegment,
    count: usize,
    is_pure_css: bool,
    has_aria: bool,
    has_pierce: bool,
    pub fn slice(): []const PSelectorSegment;
}
export struct SimdSelectorScanner {
    pub static fn needsPSelectorParsing(selector: []const u8): bool;
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize?;
}
export struct PSelectorParser {
    pub static fn parse(selector: []const u8): ParsedPSelector;
}
export struct WaitForFunctionOptions {
    polling: WaitTaskPolling,
    polling_ms: u32,
    timeout_ms: u32,
}
export struct WaitTask {
    pub static fn init(expression: []const u8, options: WaitForFunctionOptions): WaitTask;
    pub fn buildPollerExpression(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
}

export enum PageEvent { Console, Dialog, Request, Response, Load, DomContentLoaded, Close, Error, FrameAttached, FrameDetached, FileChooser, Wildcard }
export struct PuppeteerEventEmitter {
    pub static fn init(): PuppeteerEventEmitter;
    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn);
    pub fn removeAllListeners(*_, event_name: []const u8);
    pub fn listenerCount(event_name: []const u8): usize;
    pub fn emit(*_, event_name: []const u8, payload: []const u8): bool;
}

export struct FileChooser {
    pub static fn init(element: ElementHandle, multiple: bool): FileChooser;
    pub fn isMultiple(): bool;
    pub fn isHandled(): bool;
    pub fn accept(*_, paths: []const []const u8): void!PuppeteerError;
    pub fn cancel(*_): void!PuppeteerError;
}
export enum ConsoleMessageType { Log, Debug, Info, Error, Warn, Dir, Table, Trace, Assert, Verbose }
export struct ConsoleMessageLocation { url: []const u8, line_number: u32, column_number: u32, script_id: []const u8 }
export struct ConsoleMessage {
    pub static fn init(msg_type: ConsoleMessageType, text_val: []const u8, loc: ConsoleMessageLocation, args_count: usize): ConsoleMessage;
    pub static fn fromCdpPayload(params_json: []const u8): ConsoleMessage;
    pub fn kind(): ConsoleMessageType;
    pub fn text(): []const u8;
    pub fn location(): ConsoleMessageLocation;
    pub fn argsCount(): usize;
}
export struct SecurityDetails {
    pub static fn init(subject_name: []const u8, issuer_val: []const u8, valid_from: f64, valid_to: f64, protocol_val: []const u8): SecurityDetails;
    pub static fn fromCdpPayload(security_json: []const u8): SecurityDetails;
    pub fn issuer(): []const u8;
    pub fn validFrom(): f64;
    pub fn validTo(): f64;
    pub fn protocol(): []const u8;
    pub fn subjectName(): []const u8;
    pub fn subjectAlternativeNames(): []const []const u8;
}
export struct TimeoutSettings {
    pub static fn init(): TimeoutSettings;
    pub fn setDefaultTimeout(*_, timeout_ms: u32);
    pub fn setDefaultNavigationTimeout(*_, timeout_ms: u32);
    pub fn navigationTimeout(): u32;
    pub fn timeout(): u32;
}

export struct TracingOptions {
    path: []const u8,
    screenshots: bool,
    categories: []const []const u8,
    buffer_size: u32,
}
export struct Tracing {
    pub static fn init(session: *CDPSession): Tracing;
    pub fn isRecording(): bool;
    pub fn start(*_, options: TracingOptions): void!PuppeteerError;
    pub fn stop(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
}

export struct CoverageRange { start: usize, end: usize }
export struct CoverageEntry {
    pub fn url(): []const u8;
    pub fn text(): []const u8;
    pub fn rangeSlice(): []const CoverageRange;
}
export struct CoverageReport {
    pub fn slice(): []const CoverageEntry;
}
export struct JSCoverageOptions {
    reset_on_navigation: bool,
    report_anonymous_scripts: bool,
    include_raw_script_coverage: bool,
    use_block_coverage: bool,
}
export struct CSSCoverageOptions {
    reset_on_navigation: bool,
}
export struct JSCoverage {
    pub static fn init(session: *CDPSession): JSCoverage;
    pub fn isEnabled(): bool;
    pub fn start(*_, options: JSCoverageOptions): void!PuppeteerError;
    pub fn stop(*_): CoverageReport!PuppeteerError;
}
export struct CSSCoverage {
    pub static fn init(session: *CDPSession): CSSCoverage;
    pub fn isEnabled(): bool;
    pub fn start(*_, options: CSSCoverageOptions): void!PuppeteerError;
    pub fn stop(*_): CoverageReport!PuppeteerError;
}
export struct Coverage {
    pub static fn init(session: *CDPSession): Coverage;
    pub fn startJSCoverage(*_, options: JSCoverageOptions): void!PuppeteerError;
    pub fn stopJSCoverage(*_): CoverageReport!PuppeteerError;
    pub fn startCSSCoverage(*_, options: CSSCoverageOptions): void!PuppeteerError;
    pub fn stopCSSCoverage(*_): CoverageReport!PuppeteerError;
}
export fn convertToDisjointRanges(starts: []const usize, ends: []const usize, counts: []const usize, out_ranges: []CoverageRange): usize;

// --- Internal Event State Machine Managers (NetworkManager, FrameManager, ExecutionContext, TargetManager, AccessibilityTreeBuilder) ---

export struct NetworkSimdScanner {
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize?;
    pub static fn eqlIgnoreCaseSimd(a: []const u8, b: []const u8): bool;
}
export enum NetworkEventKind { Request, Response, RequestFinished, RequestFailed, RequestServedFromCache, AuthRequired }
export struct InterceptDecision {
    state: InterceptResolutionState,
    abort_reason: []const u8,
    respond_status: u16,
    respond_content_type: []const u8,
    respond_body: []const u8,
    continue_overrides: ContinueRequestOverrides,
    pub static fn none(): InterceptDecision;
    pub static fn abort(reason: []const u8, priority: i32): InterceptDecision;
    pub static fn respond(status: u16, content_type: []const u8, body: []const u8, priority: i32): InterceptDecision;
    pub static fn continueWith(overrides: ContinueRequestOverrides, priority: i32): InterceptDecision;
}
export type InterceptHandlerFn = fn (req: *HTTPRequest, ctx: usize) InterceptDecision;
export struct InterceptHandlerEntry {
    handler: InterceptHandlerFn,
    priority: i32,
    ctx: usize,
    active: bool,
}
export struct ManagedRequestRecord {
    request: HTTPRequest,
    response: HTTPResponse,
    has_response: bool,
    finished: bool,
    failed: bool,
    served_from_cache: bool,
    auth_attempted: bool,
    last_resolution_priority: i32,
}
export struct NetworkManager {
    pub static fn init(alloc: std.mem.Allocator, session: *CDPSession): NetworkManager;
    pub fn deinit(*_);
    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn);
    pub fn addInterceptor(*_, handler: InterceptHandlerFn, priority: i32, ctx: usize): bool;
    pub fn clearInterceptors(*_);
    pub fn setRequestInterception(*_, enabled: bool): void!PuppeteerError;
    pub fn authenticate(*_, creds: Credentials?): void!PuppeteerError;
    pub fn setCacheEnabled(*_, enabled: bool): void!PuppeteerError;
    pub fn isCacheEnabled(): bool;
    pub fn setOfflineMode(*_, enabled: bool): void!PuppeteerError;
    pub fn isOffline(): bool;
    pub fn emulateNetworkConditions(*_, conditions: NetworkConditions): void!PuppeteerError;
    pub fn setExtraHTTPHeaders(*_, headers: []const HttpHeader): void!PuppeteerError;
    pub fn extraHTTPHeaders(): []const HttpHeader;
    pub fn handleCdpEvent(*_, ev: CdpEvent): void!PuppeteerError;
    pub fn onRequestWillBeSent(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onRequestPaused(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onAuthRequired(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onResponseReceived(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onRequestServedFromCache(*_, params_json: []const u8);
    pub fn onLoadingFinished(*_, params_json: []const u8);
    pub fn onLoadingFailed(*_, params_json: []const u8);
    pub fn getRequestById(request_id: []const u8): (*HTTPRequest)?;
    pub fn getResponseById(request_id: []const u8): (*HTTPResponse)?;
    pub fn getRecordById(request_id: []const u8): (*ManagedRequestRecord)?;
    pub fn inFlightCount(): usize;
    pub fn requestCount(): usize;
}

export struct FrameSimdScanner {
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize?;
    pub static fn stripFragment(url: []const u8): []const u8;
}
export struct ExecutionContext {
    pub static fn init(session: *CDPSession, context_id: i64, frame_id_val: []const u8, origin_val: []const u8, name_val: []const u8, unique_id_val: []const u8, is_default_val: bool): ExecutionContext;
    pub fn id(): i64;
    pub fn frameId(): []const u8;
    pub fn origin(): []const u8;
    pub fn name(): []const u8;
    pub fn uniqueId(): []const u8;
    pub fn isDefault(): bool;
    pub fn isDestroyed(): bool;
    pub fn markDestroyed(*_);
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
}
export struct IsolatedWorld {
    pub static fn init(frame_id: []const u8, world_name: []const u8): IsolatedWorld;
    pub fn frameId(): []const u8;
    pub fn name(): []const u8;
    pub fn hasContext(): bool;
    pub fn executionContext(): (*ExecutionContext)?;
    pub fn setContext(*_, ctx: *ExecutionContext);
    pub fn clearContext(*_);
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
}
export struct ManagedFrame {
    pub static fn init(session: *CDPSession, frame_id: []const u8, parent_frame_id: []const u8): ManagedFrame;
    pub fn id(): []const u8;
    pub fn loaderId(): []const u8;
    pub fn url(): []const u8;
    pub fn name(): []const u8;
    pub fn isDetached(): bool;
    pub fn lifecycleEvents(): []const []const u8;
    pub fn resetLifecycle(*_, new_loader_id: []const u8);
    pub fn recordLifecycleEvent(*_, event_name: []const u8);
    pub fn hasLifecycleEvent(event_name: []const u8): bool;
    pub fn isWaitUntilSatisfied(wait_until: WaitUntil): bool;
}
export struct FrameManager {
    pub static fn init(alloc: std.mem.Allocator, session: *CDPSession): FrameManager;
    pub fn deinit(*_);
    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn);
    pub fn findManagedFrame(frame_id: []const u8): (*ManagedFrame)?;
    pub fn mainFrame(): (*ManagedFrame)?;
    pub fn frames(): []const *ManagedFrame;
    pub fn frameCount(): usize;
    pub fn activeFrameCount(): usize;
    pub fn contextById(context_id: i64): (*ExecutionContext)?;
    pub fn contextCount(): usize;
    pub fn activeContextCount(): usize;
    pub fn handleCdpEvent(*_, ev: CdpEvent): void!PuppeteerError;
    pub fn onFrameAttached(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onFrameNavigated(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onNavigatedWithinDocument(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onFrameDetached(*_, params_json: []const u8);
    pub fn onLifecycleEvent(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onExecutionContextCreated(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onExecutionContextDestroyed(*_, params_json: []const u8);
    pub fn onExecutionContextsCleared(*_);
    pub fn isLifecycleSatisfied(frame_id: []const u8, wait_until: WaitUntil): bool;
}

export struct TargetSimdScanner {
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize?;
    pub static fn hasNonWhitespaceSimd(input: []const u8): bool;
}
export enum ManagedTargetKind { Page, Iframe, Worker, ServiceWorker, SharedWorker, BackgroundPage, Browser, Webview, Other }
export fn parseManagedTargetKind(raw: []const u8): ManagedTargetKind;
export struct ManagedTarget {
    pub fn id(): []const u8;
    pub fn sessionId(): []const u8;
    pub fn isAttached(): bool;
    pub fn isOopif(): bool;
}
export struct TargetManager {
    pub static fn init(alloc: std.mem.Allocator, session: *CDPSession): TargetManager;
    pub fn deinit(*_);
    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool;
    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn);
    pub fn setAutoAttach(*_, auto_attach: bool, wait_for_debugger_on_start: bool, flatten: bool): void!PuppeteerError;
    pub fn setDiscoverTargets(*_, discover: bool): void!PuppeteerError;
    pub fn isAutoAttachEnabled(): bool;
    pub fn targetById(target_id: []const u8): (*ManagedTarget)?;
    pub fn targetBySessionId(session_id: []const u8): (*ManagedTarget)?;
    pub fn targets(): []const *ManagedTarget;
    pub fn targetCount(): usize;
    pub fn activeTargetCount(): usize;
    pub fn attachedCount(): usize;
    pub fn countByKind(kind: ManagedTargetKind): usize;
    pub fn handleCdpEvent(*_, ev: CdpEvent): void!PuppeteerError;
    pub fn onTargetCreated(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onTargetInfoChanged(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onTargetDestroyed(*_, params_json: []const u8);
    pub fn onAttachedToTarget(*_, params_json: []const u8): void!PuppeteerError;
    pub fn onDetachedFromTarget(*_, params_json: []const u8);
}
export struct SerializedAXNode {
    node_id: []const u8,
    role: []const u8,
    name: []const u8,
    value: []const u8,
    description: []const u8,
    roledescription: []const u8,
    valuetext: []const u8,
    backend_dom_node_id: i64,
    disabled: bool,
    expanded: bool,
    focused: bool,
    modal: bool,
    multiline: bool,
    multiselectable: bool,
    readonly: bool,
    required: bool,
    selected: bool,
    checked: []const u8,
    pressed: []const u8,
    level: i32,
    has_level: bool,
    pub fn childSlice(): []const *SerializedAXNode;
}
export struct AccessibilityTreeBuilder {
    pub static fn init(alloc: std.mem.Allocator): AccessibilityTreeBuilder;
    pub fn deinit(*_);
    pub static fn normalizeAriaRole(raw_role: []const u8): []const u8;
    pub static fn isControlRole(role: []const u8): bool;
    pub static fn isLandmarkRole(role: []const u8): bool;
    pub fn rawNodeCount(): usize;
    pub fn addSummaryNode(*_, summary: AXNodeSummary, parent_id: []const u8): void!PuppeteerError;
    pub fn parseAXNodesJson(*_, nodes_array_json: []const u8): usize!PuppeteerError;
    pub fn buildTree(*_, interesting_only: bool): ((*SerializedAXNode)?)!PuppeteerError;
    pub static fn destroyTree(alloc: std.mem.Allocator, node: *SerializedAXNode);
    pub static fn countTreeNodes(node: *SerializedAXNode): usize;
}
