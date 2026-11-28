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
    pub fn postData(): []const u8;
    pub fn resourceType(): ResourceType;
    pub fn frameId(): []const u8;
    pub fn isNavigationRequest(): bool;
    pub fn headers(): []const HttpHeader;
    pub fn isInterceptResolutionHandled(): bool;
    pub fn continueRequest(*_, override_url: []const u8, override_method: []const u8): void!PuppeteerError;
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
    pub fn encodedDataLength(): usize;
    pub fn buffer(): []const u8;
    pub fn text(): []const u8;
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

export struct Frame {
    pub fn id(): []const u8;
    pub fn parentFrameId(): []const u8;
    pub fn name(): []const u8;
    pub fn url(): []const u8;
    pub fn isDetached(): bool;
    pub fn goto(*_, target_url: []const u8, options: GoToOptions): void!PuppeteerError;
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn title(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn content(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn setContent(*_, html: []const u8): void!PuppeteerError;
    pub fn querySelector(*_, selector: []const u8): (ElementHandle?)!PuppeteerError;
    pub fn querySelectorAll(*_, selector: []const u8, out_handles: []ElementHandle): usize!PuppeteerError;
    pub fn waitForSelector(*_, selector: []const u8, options: WaitForSelectorOptions): ElementHandle!PuppeteerError;
    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn click(*_, selector: []const u8, options: ClickOptions): void!PuppeteerError;
    pub fn focus(*_, selector: []const u8): void!PuppeteerError;
    pub fn hover(*_, selector: []const u8): void!PuppeteerError;
    pub fn tap(*_, selector: []const u8): void!PuppeteerError;
    pub fn typeText(*_, selector: []const u8, text: []const u8, options: TypeOptions): void!PuppeteerError;
    pub fn addScriptTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError;
    pub fn addStyleTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError;
    pub fn accessibilitySnapshot(*_, options: AccessibilitySnapshotOptions): AccessibilitySnapshot!PuppeteerError;
    pub fn tracing(*_): *Tracing;
    pub fn coverage(*_): *Coverage;
}

export struct Page {
    pub fn isClosed(): bool;
    pub fn mainFrame(*_): *Frame;
    pub fn keyboard(*_): *Keyboard;
    pub fn mouse(*_): *Mouse;
    pub fn touchscreen(*_): *Touchscreen;
    pub fn createCDPSession(*_, child_session_id: []const u8): CDPSession!PuppeteerError;
    pub fn url(): []const u8;
    pub fn title(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn content(*_, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn setContent(*_, html: []const u8): void!PuppeteerError;
    pub fn goto(*_, target_url: []const u8, options: GoToOptions): void!PuppeteerError;
    pub fn goBack(*_): void!PuppeteerError;
    pub fn goForward(*_): void!PuppeteerError;
    pub fn reload(*_, ignore_cache: bool): void!PuppeteerError;
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn evaluateOnNewDocument(*_, script: []const u8): void!PuppeteerError;
    pub fn setViewport(*_, vp: Viewport): void!PuppeteerError;
    pub fn viewport(): Viewport;
    pub fn emulate(*_, device: Device): void!PuppeteerError;
    pub fn screenshot(*_, options: ScreenshotOptions, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn pdf(*_, options: PdfOptions, out_buf: []u8): ([]const u8)!PuppeteerError;
    pub fn querySelector(*_, selector: []const u8): (ElementHandle?)!PuppeteerError;
    pub fn querySelectorAll(*_, selector: []const u8, out_handles: []ElementHandle): usize!PuppeteerError;
    pub fn waitForSelector(*_, selector: []const u8, options: WaitForSelectorOptions): ElementHandle!PuppeteerError;
    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError;
    pub fn click(*_, selector: []const u8, options: ClickOptions): void!PuppeteerError;
    pub fn typeText(*_, selector: []const u8, text: []const u8, options: TypeOptions): void!PuppeteerError;
    pub fn focus(*_, selector: []const u8): void!PuppeteerError;
    pub fn hover(*_, selector: []const u8): void!PuppeteerError;
    pub fn tap(*_, selector: []const u8): void!PuppeteerError;
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
    pub fn overridePermissions(*_, origin: []const u8, permissions_json: []const u8): void!PuppeteerError;
    pub fn close(*_): void!PuppeteerError;
}

export struct Browser {
    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize);
    pub fn feedIncoming(*_, payload: []const u8): void!PuppeteerError;
    pub fn wsEndpoint(): []const u8;
    pub fn executablePath(): []const u8;
    pub fn isConnected(): bool;
    pub fn defaultBrowserContext(*_): *BrowserContext;
    pub fn browserContexts(): []const BrowserContext;
    pub fn createBrowserContext(*_): BrowserContext!PuppeteerError;
    pub fn newPage(*_): (*Page)!PuppeteerError;
    pub fn pages(): []*Page;
    pub fn targets(): []const TargetInfo;
    pub fn target(): TargetInfo;
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
