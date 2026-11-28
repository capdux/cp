// Source: Absorbed from _ref/playwright/packages/playwright-core/src/index.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/index.cp — Official CP Playwright Package Entry
// Compatible with `playwright`, `playwright-core`, and `playwright-chromium`.
// Zero-copy built on `cap:cdp` (`lib/cdp/`) with optional `cap:web` WS RPC.
// ============================================================================

export {
    PlaywrightError,
    RouteAction,
    NameValue,
    ResourceTiming,
    RequestSizes,
    RequestFailure,
    SecurityDetails,
    RemoteAddr,
    FulfillOptions,
    RouteContinueOverrides,
    ScriptTagOptions,
    StyleTagOptions,
    WorkerInfo,
    PageEventRecord,
    ExposedBindingFn,
    ExposedBindingEntry,
    findHeaderInJson,
    parseHeadersJsonArray,
    Request,
    Response,
    Route,
    RouteHandlerFn,
    RouteEntry,
    WebSocketRoute,
    WebSocketRouteHandlerFn,
    WebSocketRouteEntry,
    UrlMatcher,
} from "./network";

export {
    AriaRoleNode,
    AriaSnapshot,
} from "./aria";

export {
    ActionabilityState,
    ActionabilityCheckKind,
    ActionabilityOptions,
    ElementBox,
    ActionabilityReport,
    ActionabilityEngine,
} from "./actionability";

export {
    ClockMode,
    Clock,
} from "./clock";

export {
    simdScanHarCleanSpan,
    Download,
    FileChooser,
    ConsoleMessage,
    HarRecordOptions,
    HarEntry,
    HarRecorder,
    Video,
} from "./artifacts";

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
    ViewportSize,
    EmulateMediaOptions,
    Page,
} from "./page";

export {
    Cookie,
    StorageOrigin,
    Geolocation,
    HTTPCredentials,
    HarRouteRule,
    CDPSession,
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

export {
    WebKitLaunchOptions,
    WkMessageKind,
    WkMessage,
    WkPageProxySession,
    WkConnection,
    WebKitPage,
    WebKitBrowserContext,
    WebKitBrowser,
    WebKitBrowserType,
    webkit,
} from "./webkit";

export {
    RemoteMode,
    PlaywrightRemoteConfig,
    PlaywrightRemoteServer,
    PlaywrightRemoteClient,
} from "./remote";

export {
    CliCommand,
    CliOptions,
    CliResult,
    PlaywrightCli,
} from "./cli";

export {
    Frame,
} from "./frames";

export {
    TracingOptions,
    TraceEvent,
    Tracing,
} from "./tracing";

export {
    TraceViewerOptions,
    TraceEntryRecord,
    TraceHttpResponse,
    TraceViewerServer,
} from "./trace_viewer";

export {
    JSCoverageOptions,
    CSSCoverageOptions,
    CoverageRange,
    JSCoverageEntry,
    CSSCoverageEntry,
    Coverage,
} from "./coverage";

export {
    simdHasHeaderUnsafeBytes,
    ProxySettings,
    MultipartField,
    MultipartFormData,
    RedirectHop,
    FetchTransportResult,
    FetchTransportFn,
    APIRequestOptions,
    APIRequestFetchOptions,
    APIResponse,
    APIRequestContext,
} from "./request_context";

export {
    simdFindByteFrame,
    FrameLifecycleState,
    ManagedFrame,
    FrameManager,
    ActionabilityProbeSnapshot,
    ActionabilityProbeFn,
    DomActionabilityLoop,
} from "./frame_manager";

export {
    simdScanNetEscape,
    encodeBase64Alloc,
    mapErrorReasonToCdp,
    CrNetworkRequestState,
    CrAuthResponseAction,
    CrAuthChallenge,
    CrManagedRequest,
    CrNetworkManager,
} from "./cr_network_manager";

export {
    simdHasBinaryBytes,
    computeFnv1a64,
    computeSha1Hex,
    SNAPSHOTTER_INJECTED_SCRIPT,
    SnapshotResource,
    ResourceOverride,
    FrameSnapshot,
    Snapshotter,
    HarTimings,
    HarTracerOptions,
    HarTracerEntry,
    HarTracer,
} from "./snapshotter";

export {
    RecordedActionKind,
    RecordedAction,
    CodegenLang,
    RECORDER_INJECTED_SCRIPT,
    ActionRecorder,
    simdScanEscapeIndex,
    writeEscapedString,
    escapeJsonIntoBuf,
    escapeJsonString,
} from "./recorder";

export {
    RpcObjectKind,
    rpcObjectKindName,
    RpcErrorCode,
    DispatcherScope,
    RpcDispatcher,
    simdScanJsonStringEnd,
    simdFindJsonStructural,
    simdExtractJsonField,
    simdExtractJsonInt,
    simdExtractJsonBool,
} from "./dispatchers";
