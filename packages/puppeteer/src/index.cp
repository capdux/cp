// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/index.ts | Synced: 2026-11-28
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
    MediaFeature,
    GeolocationOptions,
    NetworkConditions,
    Credentials,
    PageMetrics,
    WebWorkerInfo,
    WaitForNetworkIdleOptions,
    WaitForTargetOptions,
    DebugInfo,
    RequestInitiator,
    ContinueRequestOverrides,
    LocatorVisibility,
    LocatorScrollOptions,
    mapCdpError,
} from "./types";

export { CDPSession } from "./cdp_session";
export { Device, KnownDevices } from "./devices";
export { KeyDefinition, USKeyboardLayout } from "./keyboard_layout";
export { Keyboard, Mouse, Touchscreen } from "./input";
export {
    PSelectorKind,
    PCombinator,
    PSelectorSegment,
    ParsedPSelector,
    SimdSelectorScanner,
    PSelectorParser,
    PUPPETEER_INJECTED_SCRIPT,
    WaitTaskPolling,
    WaitForFunctionOptions,
    WaitTask,
} from "./selectors";
export {
    EventHandlerFn,
    PageEvent,
    pageEventName,
    EventListenerEntry,
    PuppeteerEventEmitter,
} from "./events";
export {
    FileChooser,
    ConsoleMessageType,
    ConsoleMessageLocation,
    ConsoleMessage,
    SecurityDetails,
    DEFAULT_TIMEOUT_MS,
    TimeoutSettings,
} from "./extras";
export {
    InterceptResolutionAction,
    InterceptResolutionState,
    HTTPRequest,
    HTTPResponse,
    Dialog,
} from "./network";
export { JSHandle, ElementHandle, PuppeteerLocator } from "./handles";
export { Frame } from "./frame";
export { TracingOptions, Tracing } from "./tracing";
export {
    CoverageRange,
    CoverageEntry,
    CoverageReport,
    JSCoverageOptions,
    CSSCoverageOptions,
    JSCoverage,
    CSSCoverage,
    Coverage,
    convertToDisjointRanges,
} from "./coverage";
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
