// ============================================================================
// packages/puppeteer/src/index.cp — Official CP Puppeteer Package Entrypoint
// Pure CP Browser Automation SDK built on cap:cdp
// ============================================================================

export {
    PuppeteerError,
    WaitUntil,
    DialogType,
    ResourceType,
    TargetType,
    Viewport,
    LaunchOptions,
    ConnectOptions,
    GoToOptions,
    ScreenshotOptions,
    PdfOptions,
    ClickOptions,
    TypeOptions,
    WaitForSelectorOptions,
    AccessibilitySnapshotOptions,
    AccessibilitySnapshot,
    Cookie,
    CookieParam,
    HttpHeader,
    BoundingBox,
    BoxModel,
    RemoteAddress,
    ResponseTiming,
    TargetInfo,
    EvaluateResult,
    mapCdpError,
} from "./types";

export { CDPSession } from "./cdp_session";
export { Keyboard, Mouse, Touchscreen } from "./input";
export {
    InterceptResolutionAction,
    InterceptResolutionState,
    HTTPRequest,
    HTTPResponse,
    Dialog,
} from "./network";
export { JSHandle, ElementHandle } from "./handles";
export { Frame } from "./frame";
export { Page } from "./page";
export { BrowserContext, Browser } from "./browser";
export {
    Puppeteer,
    launch,
    launchWithTransport,
    connect,
    connectWithTransport,
    defaultExecutablePath,
} from "./puppeteer";

export {
    CdpError,
    CdpResponse,
    CdpEvent,
    CdpEventCallback,
    CdpSendTransportFn,
    CdpWsEndpoint,
    AXNodeSummary,
} from "cap:cdp";
