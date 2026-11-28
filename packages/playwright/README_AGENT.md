# playwright — AI Agent Quick Ref

Playwright browser automation & testing package for CP (`playwright` / `playwright-core` / `playwright-chromium` compatible), built zero-copy on `cap:cdp` (`v1.0.0`, `MIT`).

## Import

```cp
import {
    chromium,
    webkit,
    Browser,
    WebKitBrowser,
    BrowserContext,
    Page,
    WebKitPage,
    WkConnection,
    Locator,
    Route,
    Request,
    Response,
    AriaSnapshot,
    Cookie,
    PlaywrightError,
} from "playwright";
```

## Core API Signatures

```cp
// Chromium launcher & CDP connect
chromium.launch(alloc: std.mem.Allocator, opts: LaunchOptions): Browser!PlaywrightError
chromium.connectOverCDP(alloc: std.mem.Allocator, endpoint_url: []const u8, opts: ConnectOverCdpOptions): Browser!PlaywrightError
chromium.executablePath(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)?

// WebKit launcher, WkConnection & Inspector connect (driver only; no bundled binary)
webkit.name(): []const u8 // "webkit"
webkit.executablePath(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)?
webkit.buildLaunchArgs(alloc: std.mem.Allocator, opts: WebKitLaunchOptions): ([]([]u8))!PlaywrightError
webkit.launch(alloc: std.mem.Allocator, opts: WebKitLaunchOptions): WebKitBrowser!PlaywrightError
webkit.launchWithTransport(alloc: std.mem.Allocator, opts: WebKitLaunchOptions, cb: CdpSendTransportFn, ctx: usize): WebKitBrowser!PlaywrightError
webkit.connectWithTransport(alloc: std.mem.Allocator, opts: WebKitLaunchOptions, cb: CdpSendTransportFn, ctx: usize): WebKitBrowser!PlaywrightError

// Browser & BrowserContext
browser.newContext(*_, opts: BrowserContextOptions): (*BrowserContext)!PlaywrightError
browser.newPage(*_): (*Page)!PlaywrightError
browser.contexts(): []*BrowserContext
browser.version(): []const u8
browser.close(*_)

ctx.newPage(*_): (*Page)!PlaywrightError
ctx.pages(): []*Page
ctx.addCookies(*_, new_cookies: []const Cookie): void!PlaywrightError
ctx.cookies(): []const Cookie
ctx.clearCookies(*_)
ctx.storageState(alloc: std.mem.Allocator): StorageState!PlaywrightError
ctx.route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError
ctx.unroute(*_, pattern: []const u8)
ctx.close(*_)

// Page & Locator
page.goto(*_, target_url: []const u8, opts: GotoOptions): Response!PlaywrightError
page.locator(selector: []const u8): Locator
page.getByRole(role: []const u8, opts: ByRoleOptions): Locator
page.getByText(text: []const u8, exact: bool): Locator
page.getByLabel(text: []const u8, exact: bool): Locator
page.getByPlaceholder(text: []const u8, exact: bool): Locator
page.getByTestId(test_id: []const u8): Locator
page.ariaSnapshot(*_, alloc: std.mem.Allocator): AriaSnapshot!PlaywrightError
page.screenshot(*_, opts: ScreenshotOptions): ([]const u8)!PlaywrightError
page.pdf(*_, opts: PdfOptions): ([]const u8)!PlaywrightError

loc.locator(sub: []const u8): Locator
loc.first(): Locator
loc.last(): Locator
loc.nth(index: i32): Locator
loc.filter(opts: LocatorOptions): Locator
loc.click(): void!PlaywrightError
loc.fill(value: []const u8): void!PlaywrightError
loc.press(key: []const u8): void!PlaywrightError
loc.check(): void!PlaywrightError
loc.uncheck(): void!PlaywrightError
loc.selectOption(value: []const u8): void!PlaywrightError
loc.textContent(alloc: std.mem.Allocator): ([]u8)!PlaywrightError
loc.innerText(alloc: std.mem.Allocator): ([]u8)!PlaywrightError
loc.getAttribute(alloc: std.mem.Allocator, name: []const u8): ([]u8?)!PlaywrightError
loc.isVisible(): bool!PlaywrightError
loc.isEnabled(): bool!PlaywrightError
loc.count(): usize!PlaywrightError
loc.waitFor(opts: WaitForOptions): void!PlaywrightError
```

## Ownership & Pitfalls

- **RAII Cleanup**: `Browser`, `BrowserContext`, `Page`, `AriaSnapshot`, and `StorageState` implement `deinit(*_)` and `~init()`. Call `defer browser.deinit();` to free all contexts, pages, and CDP sessions with zero leaks.
- **Value-safe Locators**: `Locator` uses an inline 512-byte selector buffer with zero heap allocation, so chaining `.locator(...).first().nth(0).filter(...)` is leak-free.
- **No `cap:*` in `cap.json`**: `cap:cdp` is a built-in standard library module and is imported directly without appearing in `"dep"`.
- **WebKit Binary Not Bundled**: `webkit.cp` implements only the CP WebKit Inspector protocol driver. Users must install the browser binary separately (`npx playwright install webkit` / `npx playwright install-deps webkit`) and may override via `PLAYWRIGHT_WEBKIT_EXECUTABLE_PATH` or `WebKitLaunchOptions.executable_path`.
