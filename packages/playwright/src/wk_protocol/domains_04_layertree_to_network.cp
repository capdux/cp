// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue, ProtocolStringMap } from "./common";
import { Console_StackTrace, DOM_NodeId } from "./domains_02_console_to_dom";
import { Page_ResourceType } from "./domains_05_page_to_recording";
import { Runtime_RemoteObject, Security_Connection, Security_Security } from "./domains_06_runtime_to_worker";

// ============================================================================
// Domain: LayerTree
// ============================================================================

/// Unique RenderLayer identifier.
pub type LayerTree_LayerId = []const u8;
/// Unique PseudoElement identifier.
pub type LayerTree_PseudoElementId = []const u8;
/// A rectangle.
pub struct LayerTree_IntRect {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
}
/// Information about a compositing layer.
pub struct LayerTree_Layer {
    layerId: LayerTree_LayerId = "",
    nodeId: DOM_NodeId = 0.0,
    bounds: LayerTree_IntRect = .{},
    paintCount: f64 = 0.0,
    memory: f64 = 0.0,
    compositedBounds: LayerTree_IntRect = .{},
    isInShadowTree: bool? = null,
    isReflection: bool? = null,
    isGeneratedContent: bool? = null,
    isAnonymous: bool? = null,
    pseudoElementId: LayerTree_PseudoElementId? = null,
    pseudoElement: ([]const u8)? = null,
    contentFrameRect: LayerTree_IntRect? = null,
}
/// An object containing the reasons why the layer was composited as properties.
pub struct LayerTree_CompositingReasons {
    transform3D: bool? = null,
    video: bool? = null,
    canvas: bool? = null,
    plugin: bool? = null,
    iFrame: bool? = null,
    model: bool? = null,
    backfaceVisibilityHidden: bool? = null,
    clipsCompositingDescendants: bool? = null,
    animation: bool? = null,
    filters: bool? = null,
    positionFixed: bool? = null,
    positionSticky: bool? = null,
    overflowScrollingTouch: bool? = null,
    stacking: bool? = null,
    overlap: bool? = null,
    negativeZIndexChildren: bool? = null,
    transformWithCompositedDescendants: bool? = null,
    opacityWithCompositedDescendants: bool? = null,
    maskWithCompositedDescendants: bool? = null,
    reflectionWithCompositedDescendants: bool? = null,
    filterWithCompositedDescendants: bool? = null,
    blendingWithCompositedDescendants: bool? = null,
    isolatesCompositedBlendingDescendants: bool? = null,
    perspective: bool? = null,
    preserve3D: bool? = null,
    willChange: bool? = null,
    root: bool? = null,
    blending: bool? = null,
    backdropRoot: bool? = null,
    overflowScrollPositioning: bool? = null,
    anchorPositioning: bool? = null,
    spatialPortal: bool? = null,
    spatialPortalHosting: bool? = null,
    screenCaptureProtection: bool? = null,
}
pub type LayerTree_layerTreeDidChangePayload = ProtocolJsonValue;
/// Enables compositing tree inspection.
pub struct LayerTree_enableParameters {}
pub struct LayerTree_enableReturnValue {}
/// Disables compositing tree inspection.
pub struct LayerTree_disableParameters {}
pub struct LayerTree_disableReturnValue {}
/// Returns the layer tree structure of the current page.
pub struct LayerTree_layersForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct LayerTree_layersForNodeReturnValue {
    layers: []const LayerTree_Layer = &.{},
}
/// Provides the reasons why the given layer was composited.
pub struct LayerTree_reasonsForCompositingLayerParameters {
    layerId: LayerTree_LayerId = "",
}
pub struct LayerTree_reasonsForCompositingLayerReturnValue {
    compositingReasons: LayerTree_CompositingReasons = .{},
}
/// Captures a snapshot of the layer's rendered content as a PNG data URL.
pub struct LayerTree_requestContentParameters {
    layerId: LayerTree_LayerId = "",
}
pub struct LayerTree_requestContentReturnValue {
    content: []const u8 = "",
}

// ============================================================================
// Domain: Memory
// Memory domain exposes page memory tracking.
// ============================================================================

pub struct Memory_Event {
    timestamp: f64 = 0.0,
    categories: []const Memory_CategoryData = &.{},
}
pub struct Memory_CategoryData {
    type_: []const u8 = "", // protocol field: "type"; enum: "javascript"|"jit"|"images"|"layers"|"page"|"other"
    size: f64 = 0.0,
}
/// Memory pressure was encountered.
pub struct Memory_memoryPressurePayload {
    timestamp: f64 = 0.0,
    severity: []const u8 = "", // enum: "critical"|"non-critical"
}
/// Tracking started.
pub struct Memory_trackingStartPayload {
    timestamp: f64 = 0.0,
}
/// Periodic tracking updates with event data.
pub struct Memory_trackingUpdatePayload {
    event: Memory_Event = .{},
}
/// Tracking stopped.
pub struct Memory_trackingCompletePayload {
    timestamp: f64 = 0.0,
}
/// Enables Memory domain events.
pub struct Memory_enableParameters {}
pub struct Memory_enableReturnValue {}
/// Disables Memory domain events.
pub struct Memory_disableParameters {}
pub struct Memory_disableReturnValue {}
/// Start tracking memory. This will produce a `trackingStart` event.
pub struct Memory_startTrackingParameters {}
pub struct Memory_startTrackingReturnValue {}
/// Stop tracking memory. This will produce a `trackingComplete` event.
pub struct Memory_stopTrackingParameters {}
pub struct Memory_stopTrackingReturnValue {}

// ============================================================================
// Domain: Network
// Network domain allows tracking network activities of the page. It exposes information about http, file, data and other requests and responses, their headers, bodies, timing, etc.
// ============================================================================

/// Unique loader identifier.
pub type Network_LoaderId = []const u8;
/// Unique frame identifier.
pub type Network_FrameId = []const u8;
/// Unique request identifier.
pub type Network_RequestId = []const u8;
/// Elapsed seconds since frontend connected.
pub type Network_Timestamp = f64;
/// Number of seconds since epoch.
pub type Network_Walltime = f64;
/// Controls how much referrer information is sent with the request
/// enum: "empty-string"|"no-referrer"|"no-referrer-when-downgrade"|"same-origin"|"origin"|"strict-origin"|"origin-when-cross-o...
pub type Network_ReferrerPolicy = []const u8;
/// Request / response headers as keys / values of JSON object.
pub type Network_Headers = ProtocolStringMap;
/// Timing information for the request.
pub struct Network_ResourceTiming {
    startTime: Network_Timestamp = 0.0,
    redirectStart: Network_Timestamp = 0.0,
    redirectEnd: Network_Timestamp = 0.0,
    fetchStart: Network_Timestamp = 0.0,
    domainLookupStart: f64 = 0.0,
    domainLookupEnd: f64 = 0.0,
    connectStart: f64 = 0.0,
    connectEnd: f64 = 0.0,
    secureConnectionStart: f64 = 0.0,
    requestStart: f64 = 0.0,
    responseStart: f64 = 0.0,
    responseEnd: f64 = 0.0,
}
/// HTTP request data.
pub struct Network_Request {
    url: []const u8 = "",
    method: []const u8 = "",
    headers: Network_Headers = "",
    postData: ([]const u8)? = null,
    referrerPolicy: Network_ReferrerPolicy? = null,
    integrity: ([]const u8)? = null,
}
/// HTTP response data.
pub struct Network_Response {
    url: []const u8 = "",
    status: f64 = 0.0,
    statusText: []const u8 = "",
    headers: Network_Headers = "",
    mimeType: []const u8 = "",
    source: []const u8 = "", // enum: "unknown"|"network"|"memory-cache"|"disk-cache"|"service-worker"|"inspector-override"
    requestHeaders: Network_Headers? = null,
    timing: Network_ResourceTiming? = null,
    security: Security_Security? = null,
}
/// Network load metrics.
pub struct Network_Metrics {
    protocol: ([]const u8)? = null,
    priority: ([]const u8)? = null, // enum: "low"|"medium"|"high"
    connectionIdentifier: ([]const u8)? = null,
    remoteAddress: ([]const u8)? = null,
    requestHeaders: Network_Headers? = null,
    requestHeaderBytesSent: f64? = null,
    requestBodyBytesSent: f64? = null,
    responseHeaderBytesReceived: f64? = null,
    responseBodyBytesReceived: f64? = null,
    responseBodyDecodedSize: f64? = null,
    securityConnection: Security_Connection? = null,
    isProxyConnection: bool? = null,
}
/// WebSocket request data.
pub struct Network_WebSocketRequest {
    headers: Network_Headers = "",
}
/// WebSocket response data.
pub struct Network_WebSocketResponse {
    status: f64 = 0.0,
    statusText: []const u8 = "",
    headers: Network_Headers = "",
}
/// WebSocket frame data.
pub struct Network_WebSocketFrame {
    opcode: f64 = 0.0,
    mask: bool = false,
    payloadData: []const u8 = "",
    payloadLength: f64 = 0.0,
}
/// Information about the cached resource.
pub struct Network_CachedResource {
    url: []const u8 = "",
    type_: Page_ResourceType = "", // protocol field: "type"
    response: Network_Response? = null,
    bodySize: f64 = 0.0,
    sourceMapURL: ([]const u8)? = null,
}
/// Information about the request initiator.
pub struct Network_Initiator {
    type_: []const u8 = "", // protocol field: "type"; enum: "parser"|"script"|"other"
    stackTrace: Console_StackTrace? = null,
    url: ([]const u8)? = null,
    lineNumber: f64? = null,
    nodeId: DOM_NodeId? = null,
}
/// Different stages of a network request.
/// enum: "request"|"response"
pub type Network_NetworkStage = []const u8;
/// Different stages of a network request.
/// enum: "General"|"AccessControl"|"Cancellation"|"Timeout"
pub type Network_ResourceErrorType = []const u8;
/// Fired when page is about to send HTTP request.
pub struct Network_requestWillBeSentPayload {
    requestId: Network_RequestId = "",
    frameId: Network_FrameId = "",
    loaderId: Network_LoaderId = "",
    documentURL: []const u8 = "",
    request: Network_Request = .{},
    timestamp: Network_Timestamp = 0.0,
    walltime: Network_Walltime = 0.0,
    initiator: Network_Initiator = .{},
    redirectResponse: Network_Response? = null,
    type_: Page_ResourceType? = null, // protocol field: "type"
    targetId: ([]const u8)? = null,
}
/// Fired when HTTP response is available.
pub struct Network_responseReceivedPayload {
    requestId: Network_RequestId = "",
    frameId: Network_FrameId = "",
    loaderId: Network_LoaderId = "",
    timestamp: Network_Timestamp = 0.0,
    type_: Page_ResourceType = "", // protocol field: "type"
    response: Network_Response = .{},
}
/// Fired when data chunk was received over the network.
pub struct Network_dataReceivedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    dataLength: f64 = 0.0,
    encodedDataLength: f64 = 0.0,
}
/// Fired when HTTP request has finished loading.
pub struct Network_loadingFinishedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    sourceMapURL: ([]const u8)? = null,
    metrics: Network_Metrics? = null,
}
/// Fired when HTTP request has failed to load.
pub struct Network_loadingFailedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    errorText: []const u8 = "",
    canceled: bool? = null,
}
/// Fired when HTTP request has been served from memory cache.
pub struct Network_requestServedFromMemoryCachePayload {
    requestId: Network_RequestId = "",
    frameId: Network_FrameId = "",
    loaderId: Network_LoaderId = "",
    documentURL: []const u8 = "",
    timestamp: Network_Timestamp = 0.0,
    initiator: Network_Initiator = .{},
    resource: Network_CachedResource = .{},
}
/// Fired when HTTP request has been intercepted. The frontend must respond with <code>Network.interceptContinue</code>, <code>Network.interceptWithRequest</code>` or <code>Network.interceptWithResponse</code>` to resolve this request.
pub struct Network_requestInterceptedPayload {
    requestId: Network_RequestId = "",
    request: Network_Request = .{},
}
/// Fired when HTTP response has been intercepted. The frontend must response with <code>Network.interceptContinue</code> or <code>Network.interceptWithResponse</code>` to continue this response.
pub struct Network_responseInterceptedPayload {
    requestId: Network_RequestId = "",
    response: Network_Response = .{},
}
/// Fired when WebSocket is about to initiate handshake.
pub struct Network_webSocketWillSendHandshakeRequestPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    walltime: Network_Walltime = 0.0,
    request: Network_WebSocketRequest = .{},
}
/// Fired when WebSocket handshake response becomes available.
pub struct Network_webSocketHandshakeResponseReceivedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    response: Network_WebSocketResponse = .{},
}
/// Fired upon WebSocket creation.
pub struct Network_webSocketCreatedPayload {
    requestId: Network_RequestId = "",
    url: []const u8 = "",
    frameId: Network_FrameId? = null,
    loaderId: Network_LoaderId? = null,
}
/// Fired when WebSocket is closed.
pub struct Network_webSocketClosedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
}
/// Fired when WebSocket frame is received.
pub struct Network_webSocketFrameReceivedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    response: Network_WebSocketFrame = .{},
}
/// Fired when WebSocket frame error occurs.
pub struct Network_webSocketFrameErrorPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    errorMessage: []const u8 = "",
}
/// Fired when WebSocket frame is sent.
pub struct Network_webSocketFrameSentPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_Timestamp = 0.0,
    response: Network_WebSocketFrame = .{},
}
/// Enables network tracking, network events will now be delivered to the client.
pub struct Network_enableParameters {}
pub struct Network_enableReturnValue {}
/// Disables network tracking, prevents network events from being sent to the client.
pub struct Network_disableParameters {}
pub struct Network_disableReturnValue {}
/// Specifies whether to always send extra HTTP headers with the requests from this page.
pub struct Network_setExtraHTTPHeadersParameters {
    headers: Network_Headers = "",
}
pub struct Network_setExtraHTTPHeadersReturnValue {}
/// Returns content served for the given request.
pub struct Network_getResponseBodyParameters {
    requestId: Network_RequestId = "",
}
pub struct Network_getResponseBodyReturnValue {
    body: []const u8 = "",
    base64Encoded: bool = false,
}
/// Toggles whether the resource cache may be used when loading resources in the inspected page. If <code>true</code>, the resource cache will not be used when loading resources.
pub struct Network_setResourceCachingDisabledParameters {
    disabled: bool = false,
}
pub struct Network_setResourceCachingDisabledReturnValue {}
/// Toggles whether resource data is cleared on page navigations / reloads.
pub struct Network_setClearResourceDataOnNavigateParameters {
    clearResourceDataOnNavigate: bool = false,
}
pub struct Network_setClearResourceDataOnNavigateReturnValue {}
/// Loads a resource in the context of a frame on the inspected page without cross origin checks.
pub struct Network_loadResourceParameters {
    frameId: Network_FrameId = "",
    url: []const u8 = "",
}
pub struct Network_loadResourceReturnValue {
    content: []const u8 = "",
    mimeType: []const u8 = "",
    status: f64 = 0.0,
}
/// Fetches a serialized secure certificate for the given requestId to be displayed via InspectorFrontendHost.showCertificate.
pub struct Network_getSerializedCertificateParameters {
    requestId: Network_RequestId = "",
}
pub struct Network_getSerializedCertificateReturnValue {
    serializedCertificate: []const u8 = "",
}
/// Resolves JavaScript WebSocket object for given request id.
pub struct Network_resolveWebSocketParameters {
    requestId: Network_RequestId = "",
    objectGroup: ([]const u8)? = null,
}
pub struct Network_resolveWebSocketReturnValue {
    object: Runtime_RemoteObject = .{},
}
/// Enable interception of network requests.
pub struct Network_setInterceptionEnabledParameters {
    enabled: bool = false,
}
pub struct Network_setInterceptionEnabledReturnValue {}
/// Add an interception.
pub struct Network_addInterceptionParameters {
    url: []const u8 = "",
    stage: Network_NetworkStage = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Network_addInterceptionReturnValue {}
/// Remove an interception.
pub struct Network_removeInterceptionParameters {
    url: []const u8 = "",
    stage: Network_NetworkStage = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Network_removeInterceptionReturnValue {}
/// Continue request or response without modifications.
pub struct Network_interceptContinueParameters {
    requestId: Network_RequestId = "",
    stage: Network_NetworkStage = "",
}
pub struct Network_interceptContinueReturnValue {}
/// Replace intercepted request with the provided one.
pub struct Network_interceptWithRequestParameters {
    requestId: Network_RequestId = "",
    url: ([]const u8)? = null,
    method: ([]const u8)? = null,
    headers: Network_Headers? = null,
    postData: ([]const u8)? = null,
}
pub struct Network_interceptWithRequestReturnValue {}
/// Provide response content for an intercepted response.
pub struct Network_interceptWithResponseParameters {
    requestId: Network_RequestId = "",
    content: []const u8 = "",
    base64Encoded: bool = false,
    mimeType: ([]const u8)? = null,
    status: f64? = null,
    statusText: ([]const u8)? = null,
    headers: Network_Headers? = null,
}
pub struct Network_interceptWithResponseReturnValue {}
/// Provide response for an intercepted request. Request completely bypasses the network in this case and is immediately fulfilled with the provided data.
pub struct Network_interceptRequestWithResponseParameters {
    requestId: Network_RequestId = "",
    content: []const u8 = "",
    base64Encoded: bool = false,
    mimeType: []const u8 = "",
    status: f64 = 0.0,
    statusText: []const u8 = "",
    headers: Network_Headers = "",
}
pub struct Network_interceptRequestWithResponseReturnValue {}
/// Fail request with given error type.
pub struct Network_interceptRequestWithErrorParameters {
    requestId: Network_RequestId = "",
    errorType: Network_ResourceErrorType = "",
}
pub struct Network_interceptRequestWithErrorReturnValue {}
/// Emulate various network conditions (e.g. bytes per second, latency, etc.).
pub struct Network_setEmulatedConditionsParameters {
    bandwidth: f64? = null,
    latency: f64? = null,
}
pub struct Network_setEmulatedConditionsReturnValue {}
/// Emulate offline state overriding the actual state.
pub struct Network_setEmulateOfflineStateParameters {
    offline: bool = false,
}
pub struct Network_setEmulateOfflineStateReturnValue {}
