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
    fn close(*_);
}

export struct BrowserContext {
    fn newPage(*_): (*Page)!PlaywrightError;
    fn pages(): []*Page;
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
    static fn connectOverCDP(alloc: std.mem.Allocator, endpoint_url: []const u8, opts: ConnectOverCdpOptions): Browser!PlaywrightError;
}

export const chromium = ChromiumBrowserType;
