// ============================================================================
// packages/playwright/src/index.cp — Official CP Playwright Package Entry
// Compatible with `playwright`, `playwright-core`, and `playwright-chromium`.
// Zero-copy built on `cap:cdp` (`lib/cdp/`).
// ============================================================================

export {
    PlaywrightError,
    RouteAction,
    FulfillOptions,
    Request,
    Response,
    Route,
    RouteHandlerFn,
    RouteEntry,
    UrlMatcher,
} from "./network";

export {
    AriaRoleNode,
    AriaSnapshot,
} from "./aria";

export {
    WaitForState,
    WaitForOptions,
    ByRoleOptions,
    LocatorOptions,
    SelectorCompiler,
    Locator,
} from "./locator";

export {
    GotoOptions,
    ScreenshotOptions,
    PdfOptions,
    Page,
} from "./page";

export {
    Cookie,
    StorageOrigin,
    StorageState,
    BrowserContextOptions,
    BrowserContext,
} from "./context";

export {
    Browser,
} from "./browser";

export {
    LaunchOptions,
    ConnectOverCdpOptions,
    ChromiumBrowserType,
    chromium,
} from "./chromium";
