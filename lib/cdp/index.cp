// Source: Absorbed from _ref/puppeteer & _ref/playwright CDP protocol | Synced: 2026-11-28
// ============================================================================
// lib/cdp/index.cp — Official CP Standard CDP & Browser Protocol Core (cap:cdp)
// Shared Native Foundation for Puppeteer & Playwright Ecosystems
// ============================================================================

export {
    CdpError,
    CdpMessageKind,
    CdpRequest,
    CdpResponse,
    CdpEvent,
    CdpMessage,
    CdpFrameScanner,
} from "./protocol";

export {
    CdpConnectionState,
    CdpEventCallback,
    CdpSendTransportFn,
    CdpSessionInfo,
    CdpPendingCall,
    CdpEventListener,
    CdpSession,
    CdpEndpoint,
} from "./session";

export {
    CdpBrowserLaunchOptions,
    CdpWsEndpoint,
    CdpBrowserLauncher,
} from "./launcher";

export {
    PageNavigateParams,
    PageCaptureScreenshotParams,
    PagePrintToPdfParams,
    PageDomain,
    RuntimeEvaluateParams,
    RuntimeDomain,
    DomGetDocumentParams,
    DomQuerySelectorParams,
    DomDomain,
    InputMouseEventParams,
    InputKeyEventParams,
    InputDomain,
    NetworkEnableParams,
    NetworkDomain,
    AccessibilityGetFullAXTreeParams,
    AXNodeSummary,
    AccessibilityDomain,
} from "./domains";

export {
    TargetCreateTargetParams,
    TargetDomain,
    EmulationDeviceMetricsParams,
    EmulationDomain,
    FetchDomain,
    StorageDomain,
    BrowserDomain,
    LogDomain,
    TracingDomain,
} from "./devtools_types";

import { CdpFrameScanner } from "./protocol";
import { CdpEndpoint, CdpSession } from "./session";
import { CdpBrowserLauncher } from "./launcher";
import {
    PageDomain,
    RuntimeDomain,
    DomDomain,
    InputDomain,
    NetworkDomain,
    AccessibilityDomain,
} from "./domains";

pub const cdp = struct {
    pub const FrameScanner = CdpFrameScanner;
    pub const Endpoint = CdpEndpoint;
    pub const Session = CdpSession;
    pub const Launcher = CdpBrowserLauncher;
    pub const Page = PageDomain;
    pub const Runtime = RuntimeDomain;
    pub const DOM = DomDomain;
    pub const Input = InputDomain;
    pub const Network = NetworkDomain;
    pub const Accessibility = AccessibilityDomain;
};
