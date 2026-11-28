// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/common/types.ts & Errors.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/types.cp — Core Types, Options & Error Definitions
// Pure CP Puppeteer API Model built on top of cap:cdp
// ============================================================================

const std = @import("std");
import {
    CdpError,
    CdpBrowserLaunchOptions,
    AXNodeSummary,
} from "cap:cdp";

pub error PuppeteerError {
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

pub fn mapCdpError(err: CdpError): PuppeteerError {
    switch (err) {
        CdpError.NotConnected,
        CdpError.ConnectionClosed => return PuppeteerError.NotConnected,
        CdpError.SessionClosed => return PuppeteerError.SessionClosed,
        CdpError.SessionNotFound => return PuppeteerError.TargetNotFound,
        CdpError.ExecutableNotFound => return PuppeteerError.ExecutableNotFound,
        CdpError.InvalidUrl => return PuppeteerError.InvalidArgument,
        CdpError.Timeout => return PuppeteerError.Timeout,
        CdpError.BufferTooSmall => return PuppeteerError.BufferTooSmall,
        CdpError.OutOfMemory => return PuppeteerError.OutOfMemory,
        CdpError.InvalidFrame,
        CdpError.InvalidJson,
        CdpError.MissingId,
        CdpError.MissingMethod,
        CdpError.ProtocolError => return PuppeteerError.ProtocolError,
    }
}

pub enum WaitUntil {
    Load,
    DomContentLoaded,
    NetworkIdle0,
    NetworkIdle2,
}

pub enum DialogType {
    Alert,
    Confirm,
    Prompt,
    BeforeUnload,
}

pub enum ResourceType {
    Document,
    Stylesheet,
    Image,
    Media,
    Font,
    Script,
    TextTrack,
    XHR,
    Fetch,
    EventSource,
    WebSocket,
    Manifest,
    Other,
}

pub enum TargetType {
    Page,
    BackgroundPage,
    ServiceWorker,
    SharedWorker,
    Browser,
    Webview,
    Other,
}

pub struct Viewport {
    width: u32 = 1280,
    height: u32 = 720,
    device_scale_factor: f64 = 1.0,
    is_mobile: bool = false,
    has_touch: bool = false,
    is_landscape: bool = false,
}

pub struct LaunchOptions {
    headless: bool = true,
    executable_path: []const u8 = "",
    user_data_dir: []const u8 = "",
    args: []const []const u8 = &.{},
    viewport_width: u32 = 1280,
    viewport_height: u32 = 720,
    timeout_ms: u32 = 30000,
    remote_debugging_port: u16 = 0,
    ignore_https_errors: bool = false,
    default_viewport_enabled: bool = true,

    pub fn toCdpOptions(): CdpBrowserLaunchOptions {
        return CdpBrowserLaunchOptions{
            headless: _.headless,
            executable_path: _.executable_path,
            user_data_dir: _.user_data_dir,
            args: _.args,
            viewport_width: _.viewport_width,
            viewport_height: _.viewport_height,
            timeout_ms: _.timeout_ms,
            remote_debugging_port: _.remote_debugging_port,
            ignore_https_errors: _.ignore_https_errors,
        };
    }
}

pub struct ConnectOptions {
    browser_ws_url: []const u8 = "",
    ignore_https_errors: bool = false,
    default_viewport_width: u32 = 1280,
    default_viewport_height: u32 = 720,
    timeout_ms: u32 = 30000,
}

pub struct GoToOptions {
    timeout_ms: u32 = 30000,
    wait_until: WaitUntil = WaitUntil.Load,
    referer: []const u8 = "",
}

pub struct ScreenshotOptions {
    format: []const u8 = "png",
    quality: u8 = 80,
    full_page: bool = false,
    from_surface: bool = true,
    omit_background: bool = false,
}

pub struct PdfOptions {
    landscape: bool = false,
    print_background: bool = true,
    scale: f64 = 1.0,
    paper_width: f64 = 8.5,
    paper_height: f64 = 11.0,
    prefer_css_page_size: bool = false,
}

pub struct ClickOptions {
    button: []const u8 = "left",
    click_count: i32 = 1,
    delay_ms: u32 = 0,
}

pub struct TypeOptions {
    delay_ms: u32 = 0,
}

pub struct WaitForSelectorOptions {
    visible: bool = false,
    hidden: bool = false,
    timeout_ms: u32 = 30000,
}

pub struct AccessibilitySnapshotOptions {
    interesting_only: bool = true,
    depth: i32 = -1,
    frame_id: []const u8 = "",
}

pub struct Cookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "/",
    expires: f64 = -1.0,
    size: i64 = 0,
    http_only: bool = false,
    secure: bool = false,
    session: bool = true,
    same_site: []const u8 = "Lax",
}

pub struct CookieParam {
    name: []const u8 = "",
    value: []const u8 = "",
    url: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "/",
    http_only: bool = false,
    secure: bool = false,
    same_site: []const u8 = "",
    expires: f64 = -1.0,
}

pub struct HttpHeader {
    name: []const u8 = "",
    value: []const u8 = "",
}

pub struct BoundingBox {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
}

pub struct BoxModel {
    content: BoundingBox = .{},
    padding: BoundingBox = .{},
    border: BoundingBox = .{},
    margin: BoundingBox = .{},
    width: i64 = 0,
    height: i64 = 0,
}

pub struct RemoteAddress {
    ip: []const u8 = "",
    port: u16 = 0,
}

pub struct ResponseTiming {
    request_time: f64 = 0.0,
    dns_start: f64 = -1.0,
    dns_end: f64 = -1.0,
    connect_start: f64 = -1.0,
    connect_end: f64 = -1.0,
    ssl_start: f64 = -1.0,
    ssl_end: f64 = -1.0,
    send_start: f64 = 0.0,
    send_end: f64 = 0.0,
    receive_headers_end: f64 = 0.0,
}

pub struct TargetInfo {
    target_id: []const u8 = "",
    target_type: TargetType = TargetType.Page,
    title: []const u8 = "",
    url: []const u8 = "about:blank",
    attached: bool = false,
    browser_context_id: []const u8 = "",
}

pub struct EvaluateResult {
    value_json: []const u8 = "null",
    value_type: []const u8 = "undefined",
    description: []const u8 = "",
    exception_text: []const u8 = "",
    has_exception: bool = false,
}

pub struct AccessibilitySnapshot {
    nodes: [64]AXNodeSummary = undefined,
    count: usize = 0,

    pub fn slice(): []const AXNodeSummary {
        return _.nodes[0.._.count];
    }
}
