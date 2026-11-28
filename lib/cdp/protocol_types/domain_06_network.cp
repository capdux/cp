// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue, ProtocolStringMap } from "./common";
import { Emulation_UserAgentMetadata } from "./domains_04_domstorage_to_headlessexperimental";
import { IO_StreamHandle } from "./domains_05_io_to_memory";
import { Page_FrameId } from "./domains_08_page_to_performance";
import { Security_CertificateId, Security_MixedContentType, Security_SecurityState } from "./domains_09_performancetimeline_to_systeminfo";
import { Debugger_SearchMatch, Runtime_ScriptId, Runtime_StackTrace, Runtime_UniqueDebuggerId } from "./domains_11_debugger_to_schema";

// ============================================================================
// Domain: Network
// Network domain allows tracking network activities of the page. It exposes information about http,
// file, data and other requests and responses, their headers, bodies, timing, etc.
// ============================================================================

/// Resource type as it was perceived by the rendering engine.
/// enum: "Document"|"Stylesheet"|"Image"|"Media"|"Font"|"Script"|"TextTrack"|"XHR"|"Fetch"|"Prefetch"|"EventSource"|"WebSocket...
pub type Network_ResourceType = []const u8;
/// Unique loader identifier.
pub type Network_LoaderId = []const u8;
/// Unique network request identifier.
pub type Network_RequestId = []const u8;
/// Network level fetch failure reason.
/// enum: "Failed"|"Aborted"|"TimedOut"|"AccessDenied"|"ConnectionClosed"|"ConnectionReset"|"ConnectionRefused"|"ConnectionAbor...
pub type Network_ErrorReason = []const u8;
/// UTC time in seconds, counted from January 1, 1970.
pub type Network_TimeSinceEpoch = f64;
/// Monotonically increasing time in seconds since an arbitrary point in the past.
pub type Network_MonotonicTime = f64;
/// Request / response headers as keys / values of JSON object.
pub type Network_Headers = ProtocolStringMap;
/// The underlying connection technology that the browser is supposedly using.
/// enum: "none"|"cellular2g"|"cellular3g"|"cellular4g"|"bluetooth"|"ethernet"|"wifi"|"wimax"|"other"
pub type Network_ConnectionType = []const u8;
/// Represents the cookie's 'SameSite' status:
/// enum: "Strict"|"Lax"|"None"
pub type Network_CookieSameSite = []const u8;
/// Represents the cookie's 'Priority' status:
/// enum: "Low"|"Medium"|"High"
pub type Network_CookiePriority = []const u8;
/// Represents the source scheme of the origin that originally set the cookie.
/// enum: "Unset"|"NonSecure"|"Secure"
pub type Network_CookieSourceScheme = []const u8;
/// Timing information for the request.
pub struct Network_ResourceTiming {
    requestTime: f64 = 0.0,
    proxyStart: f64 = 0.0,
    proxyEnd: f64 = 0.0,
    dnsStart: f64 = 0.0,
    dnsEnd: f64 = 0.0,
    connectStart: f64 = 0.0,
    connectEnd: f64 = 0.0,
    sslStart: f64 = 0.0,
    sslEnd: f64 = 0.0,
    workerStart: f64 = 0.0,
    workerReady: f64 = 0.0,
    workerFetchStart: f64 = 0.0,
    workerRespondWithSettled: f64 = 0.0,
    workerRouterEvaluationStart: f64? = null,
    workerCacheLookupStart: f64? = null,
    sendStart: f64 = 0.0,
    sendEnd: f64 = 0.0,
    pushStart: f64 = 0.0,
    pushEnd: f64 = 0.0,
    receiveHeadersStart: f64 = 0.0,
    receiveHeadersEnd: f64 = 0.0,
}
/// Loading priority of a resource request.
/// enum: "VeryLow"|"Low"|"Medium"|"High"|"VeryHigh"
pub type Network_ResourcePriority = []const u8;
/// The render-blocking behavior of a resource request.
/// enum: "Blocking"|"InBodyParserBlocking"|"NonBlocking"|"NonBlockingDynamic"|"PotentiallyBlocking"
pub type Network_RenderBlockingBehavior = []const u8;
/// Post data entry for HTTP request
pub struct Network_PostDataEntry {
    bytes: ProtocolBinary? = null,
}
/// HTTP request data.
pub struct Network_Request {
    url: []const u8 = "",
    urlFragment: ([]const u8)? = null,
    method: []const u8 = "",
    headers: Network_Headers = "",
    postData: ([]const u8)? = null,
    hasPostData: bool? = null,
    postDataEntries: ([]const Network_PostDataEntry)? = null,
    mixedContentType: Security_MixedContentType? = null,
    initialPriority: Network_ResourcePriority = "",
    referrerPolicy: []const u8 = "", // enum: "unsafe-url"|"no-referrer-when-downgrade"|"no-referrer"|"origin"|"origin-when-cross-origin"|"same-origin"|"strict-ori...
    isLinkPreload: bool? = null,
    trustTokenParams: Network_TrustTokenParams? = null,
    isSameSite: bool? = null,
    isAdRelated: bool? = null,
}
/// Details of a signed certificate timestamp (SCT).
pub struct Network_SignedCertificateTimestamp {
    status: []const u8 = "",
    origin: []const u8 = "",
    logDescription: []const u8 = "",
    logId: []const u8 = "",
    timestamp: f64 = 0.0,
    hashAlgorithm: []const u8 = "",
    signatureAlgorithm: []const u8 = "",
    signatureData: []const u8 = "",
}
/// Security details about a request.
pub struct Network_SecurityDetails {
    protocol: []const u8 = "",
    keyExchange: []const u8 = "",
    keyExchangeGroup: ([]const u8)? = null,
    cipher: []const u8 = "",
    mac: ([]const u8)? = null,
    certificateId: Security_CertificateId = 0.0,
    subjectName: []const u8 = "",
    sanList: []const []const u8 = &.{},
    issuer: []const u8 = "",
    validFrom: Network_TimeSinceEpoch = 0.0,
    validTo: Network_TimeSinceEpoch = 0.0,
    signedCertificateTimestampList: []const Network_SignedCertificateTimestamp = &.{},
    certificateTransparencyCompliance: Network_CertificateTransparencyCompliance = "",
    serverSignatureAlgorithm: f64? = null,
    encryptedClientHello: bool = false,
}
/// Whether the request complied with Certificate Transparency policy.
/// enum: "unknown"|"not-compliant"|"compliant"
pub type Network_CertificateTransparencyCompliance = []const u8;
/// The reason why request was blocked.
/// enum: "other"|"csp"|"mixed-content"|"origin"|"inspector"|"integrity"|"subresource-filter"|"content-type"|"coep-frame-resour...
pub type Network_BlockedReason = []const u8;
/// The reason why request was blocked.
/// enum: "DisallowedByMode"|"InvalidResponse"|"WildcardOriginNotAllowed"|"MissingAllowOriginHeader"|"MultipleAllowOriginValues...
pub type Network_CorsError = []const u8;
pub struct Network_CorsErrorStatus {
    corsError: Network_CorsError = "",
    failedParameter: []const u8 = "",
}
/// Source of serviceworker response.
/// enum: "cache-storage"|"http-cache"|"fallback-code"|"network"
pub type Network_ServiceWorkerResponseSource = []const u8;
/// Determines what type of Trust Token operation is executed and
pub struct Network_TrustTokenParams {
    operation: Network_TrustTokenOperationType = "",
    refreshPolicy: []const u8 = "", // enum: "UseCached"|"Refresh"
    issuers: ([]const []const u8)? = null,
}
/// enum: "Issuance"|"Redemption"|"Signing"
pub type Network_TrustTokenOperationType = []const u8;
/// The reason why Chrome uses a specific transport protocol for HTTP semantics.
/// enum: "alternativeJobWonWithoutRace"|"alternativeJobWonRace"|"mainJobWonRace"|"mappingMissing"|"broken"|"dnsAlpnH3JobWonWit...
pub type Network_AlternateProtocolUsage = []const u8;
/// Source of service worker router.
/// enum: "network"|"cache"|"fetch-event"|"race-network-and-fetch-handler"|"race-network-and-cache"
pub type Network_ServiceWorkerRouterSource = []const u8;
pub struct Network_ServiceWorkerRouterInfo {
    ruleIdMatched: f64? = null,
    matchedSourceType: Network_ServiceWorkerRouterSource? = null,
    actualSourceType: Network_ServiceWorkerRouterSource? = null,
}
/// HTTP response data.
pub struct Network_Response {
    url: []const u8 = "",
    status: f64 = 0.0,
    statusText: []const u8 = "",
    headers: Network_Headers = "",
    headersText: ([]const u8)? = null,
    mimeType: []const u8 = "",
    charset: []const u8 = "",
    requestHeaders: Network_Headers? = null,
    requestHeadersText: ([]const u8)? = null,
    connectionReused: bool = false,
    connectionId: f64 = 0.0,
    remoteIPAddress: ([]const u8)? = null,
    remotePort: f64? = null,
    fromDiskCache: bool? = null,
    fromServiceWorker: bool? = null,
    fromPrefetchCache: bool? = null,
    fromEarlyHints: bool? = null,
    serviceWorkerRouterInfo: Network_ServiceWorkerRouterInfo? = null,
    encodedDataLength: f64 = 0.0,
    timing: Network_ResourceTiming? = null,
    serviceWorkerResponseSource: Network_ServiceWorkerResponseSource? = null,
    responseTime: Network_TimeSinceEpoch? = null,
    cacheStorageCacheName: ([]const u8)? = null,
    protocol: ([]const u8)? = null,
    alternateProtocolUsage: Network_AlternateProtocolUsage? = null,
    securityState: Security_SecurityState = "",
    securityDetails: Network_SecurityDetails? = null,
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
    headersText: ([]const u8)? = null,
    requestHeaders: Network_Headers? = null,
    requestHeadersText: ([]const u8)? = null,
}
/// WebSocket message data. This represents an entire WebSocket message, not just a fragmented frame as the name suggests.
pub struct Network_WebSocketFrame {
    opcode: f64 = 0.0,
    mask: bool = false,
    payloadData: []const u8 = "",
}
/// Information about the cached resource.
pub struct Network_CachedResource {
    url: []const u8 = "",
    type_: Network_ResourceType = "", // protocol field: "type"
    response: Network_Response? = null,
    bodySize: f64 = 0.0,
}
/// Information about the request initiator.
pub struct Network_Initiator {
    type_: []const u8 = "", // protocol field: "type"; enum: "parser"|"script"|"preload"|"SignedExchange"|"preflight"|"FedCM"|"other"
    stack: Runtime_StackTrace? = null,
    url: ([]const u8)? = null,
    lineNumber: f64? = null,
    columnNumber: f64? = null,
    requestId: Network_RequestId? = null,
}
/// cookiePartitionKey object
pub struct Network_CookiePartitionKey {
    topLevelSite: []const u8 = "",
    hasCrossSiteAncestor: bool = false,
}
/// Cookie object
pub struct Network_Cookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "",
    expires: f64 = 0.0,
    size: f64 = 0.0,
    httpOnly: bool = false,
    secure: bool = false,
    session: bool = false,
    sameSite: Network_CookieSameSite? = null,
    priority: Network_CookiePriority = "",
    sourceScheme: Network_CookieSourceScheme = "",
    sourcePort: f64 = 0.0,
    partitionKey: Network_CookiePartitionKey? = null,
    partitionKeyOpaque: bool? = null,
}
/// Types of reasons why a cookie may not be stored from a response.
/// enum: "SecureOnly"|"SameSiteStrict"|"SameSiteLax"|"SameSiteUnspecifiedTreatedAsLax"|"SameSiteNoneInsecure"|"UserPreferences...
pub type Network_SetCookieBlockedReason = []const u8;
/// Types of reasons why a cookie may not be sent with a request.
/// enum: "SecureOnly"|"NotOnPath"|"DomainMismatch"|"SameSiteStrict"|"SameSiteLax"|"SameSiteUnspecifiedTreatedAsLax"|"SameSiteN...
pub type Network_CookieBlockedReason = []const u8;
/// Types of reasons why a cookie should have been blocked by 3PCD but is exempted for the request.
/// enum: "None"|"UserSetting"|"EnterprisePolicy"|"StorageAccess"|"TopLevelStorageAccess"|"Scheme"|"SameSiteNoneCookiesInSandbox"
pub type Network_CookieExemptionReason = []const u8;
/// A cookie which was not stored from a response with the corresponding reason.
pub struct Network_BlockedSetCookieWithReason {
    blockedReasons: []const Network_SetCookieBlockedReason = &.{},
    cookieLine: []const u8 = "",
    cookie: Network_Cookie? = null,
}
/// A cookie should have been blocked by 3PCD but is exempted and stored from a response with the
pub struct Network_ExemptedSetCookieWithReason {
    exemptionReason: Network_CookieExemptionReason = "",
    cookieLine: []const u8 = "",
    cookie: Network_Cookie = .{},
}
/// A cookie associated with the request which may or may not be sent with it.
pub struct Network_AssociatedCookie {
    cookie: Network_Cookie = .{},
    blockedReasons: []const Network_CookieBlockedReason = &.{},
    exemptionReason: Network_CookieExemptionReason? = null,
}
/// Cookie parameter object
pub struct Network_CookieParam {
    name: []const u8 = "",
    value: []const u8 = "",
    url: ([]const u8)? = null,
    domain: ([]const u8)? = null,
    path: ([]const u8)? = null,
    secure: bool? = null,
    httpOnly: bool? = null,
    sameSite: Network_CookieSameSite? = null,
    expires: Network_TimeSinceEpoch? = null,
    priority: Network_CookiePriority? = null,
    sourceScheme: Network_CookieSourceScheme? = null,
    sourcePort: f64? = null,
    partitionKey: Network_CookiePartitionKey? = null,
}
/// Authorization challenge for HTTP status code 401 or 407.
pub struct Network_AuthChallenge {
    source: ([]const u8)? = null, // enum: "Server"|"Proxy"
    origin: []const u8 = "",
    scheme: []const u8 = "",
    realm: []const u8 = "",
}
/// Response to an AuthChallenge.
pub struct Network_AuthChallengeResponse {
    response: []const u8 = "", // enum: "Default"|"CancelAuth"|"ProvideCredentials"
    username: ([]const u8)? = null,
    password: ([]const u8)? = null,
}
/// Information about a signed exchange signature.
pub struct Network_SignedExchangeSignature {
    label: []const u8 = "",
    signature: []const u8 = "",
    integrity: []const u8 = "",
    certUrl: ([]const u8)? = null,
    certSha256: ([]const u8)? = null,
    validityUrl: []const u8 = "",
    date: f64 = 0.0,
    expires: f64 = 0.0,
    certificates: ([]const []const u8)? = null,
}
/// Information about a signed exchange header.
pub struct Network_SignedExchangeHeader {
    requestUrl: []const u8 = "",
    responseCode: f64 = 0.0,
    responseHeaders: Network_Headers = "",
    signatures: []const Network_SignedExchangeSignature = &.{},
    headerIntegrity: []const u8 = "",
}
/// Field type for a signed exchange related error.
/// enum: "signatureSig"|"signatureIntegrity"|"signatureCertUrl"|"signatureCertSha256"|"signatureValidityUrl"|"signatureTimesta...
pub type Network_SignedExchangeErrorField = []const u8;
/// Information about a signed exchange response.
pub struct Network_SignedExchangeError {
    message: []const u8 = "",
    signatureIndex: f64? = null,
    errorField: Network_SignedExchangeErrorField? = null,
}
/// Information about a signed exchange response.
pub struct Network_SignedExchangeInfo {
    outerResponse: Network_Response = .{},
    hasExtraInfo: bool = false,
    header: Network_SignedExchangeHeader? = null,
    securityDetails: Network_SecurityDetails? = null,
    errors: ([]const Network_SignedExchangeError)? = null,
}
pub struct Network_NetworkConditions {
    urlPattern: []const u8 = "",
    latency: f64 = 0.0,
    downloadThroughput: f64 = 0.0,
    uploadThroughput: f64 = 0.0,
    connectionType: Network_ConnectionType? = null,
    packetLoss: f64? = null,
    packetQueueLength: f64? = null,
    packetReordering: bool? = null,
    offline: bool? = null,
}
pub struct Network_BlockPattern {
    urlPattern: []const u8 = "",
    block: bool = false,
}
/// enum: "ipv4"|"ipv6"
pub type Network_DirectSocketDnsQueryType = []const u8;
pub struct Network_DirectTCPSocketOptions {
    noDelay: bool = false,
    keepAliveDelay: f64? = null,
    sendBufferSize: f64? = null,
    receiveBufferSize: f64? = null,
    dnsQueryType: Network_DirectSocketDnsQueryType? = null,
}
pub struct Network_DirectUDPSocketOptions {
    remoteAddr: ([]const u8)? = null,
    remotePort: f64? = null,
    localAddr: ([]const u8)? = null,
    localPort: f64? = null,
    dnsQueryType: Network_DirectSocketDnsQueryType? = null,
    sendBufferSize: f64? = null,
    receiveBufferSize: f64? = null,
    multicastLoopback: bool? = null,
    multicastTimeToLive: f64? = null,
    multicastAllowAddressSharing: bool? = null,
}
pub struct Network_DirectUDPMessage {
    data: ProtocolBinary = "",
    remoteAddr: ([]const u8)? = null,
    remotePort: f64? = null,
}
/// enum: "Allow"|"BlockFromInsecureToMorePrivate"|"WarnFromInsecureToMorePrivate"|"PermissionBlock"|"PermissionWarn"
pub type Network_LocalNetworkAccessRequestPolicy = []const u8;
/// enum: "Loopback"|"Local"|"Public"|"Unknown"
pub type Network_IPAddressSpace = []const u8;
pub struct Network_ConnectTiming {
    requestTime: f64 = 0.0,
}
pub struct Network_ClientSecurityState {
    initiatorIsSecureContext: bool = false,
    initiatorIPAddressSpace: Network_IPAddressSpace = "",
    localNetworkAccessRequestPolicy: Network_LocalNetworkAccessRequestPolicy = "",
}
/// Identifies the script on the stack that caused a resource or element to be
pub struct Network_AdScriptIdentifier {
    scriptId: Runtime_ScriptId = "",
    debuggerId: Runtime_UniqueDebuggerId = "",
    name: []const u8 = "",
}
/// Encapsulates the script ancestry and the root script filter list rule that
pub struct Network_AdAncestry {
    ancestryChain: []const Network_AdScriptIdentifier = &.{},
    rootScriptFilterlistRule: ([]const u8)? = null,
}
/// Represents the provenance of an ad resource or element. Only one of
pub struct Network_AdProvenance {
    filterlistRule: ([]const u8)? = null,
    adScriptAncestry: Network_AdAncestry? = null,
}
/// enum: "SameOrigin"|"SameOriginAllowPopups"|"RestrictProperties"|"UnsafeNone"|"SameOriginPlusCoep"|"RestrictPropertiesPlusCo...
pub type Network_CrossOriginOpenerPolicyValue = []const u8;
pub struct Network_CrossOriginOpenerPolicyStatus {
    value: Network_CrossOriginOpenerPolicyValue = "",
    reportOnlyValue: Network_CrossOriginOpenerPolicyValue = "",
    reportingEndpoint: ([]const u8)? = null,
    reportOnlyReportingEndpoint: ([]const u8)? = null,
}
/// enum: "None"|"Credentialless"|"RequireCorp"
pub type Network_CrossOriginEmbedderPolicyValue = []const u8;
pub struct Network_CrossOriginEmbedderPolicyStatus {
    value: Network_CrossOriginEmbedderPolicyValue = "",
    reportOnlyValue: Network_CrossOriginEmbedderPolicyValue = "",
    reportingEndpoint: ([]const u8)? = null,
    reportOnlyReportingEndpoint: ([]const u8)? = null,
}
/// enum: "HTTP"|"Meta"
pub type Network_ContentSecurityPolicySource = []const u8;
pub struct Network_ContentSecurityPolicyStatus {
    effectiveDirectives: []const u8 = "",
    isEnforced: bool = false,
    source: Network_ContentSecurityPolicySource = "",
}
pub struct Network_SecurityIsolationStatus {
    coop: Network_CrossOriginOpenerPolicyStatus? = null,
    coep: Network_CrossOriginEmbedderPolicyStatus? = null,
    csp: ([]const Network_ContentSecurityPolicyStatus)? = null,
}
/// The status of a Reporting API report.
/// enum: "Queued"|"Pending"|"MarkedForRemoval"|"Success"
pub type Network_ReportStatus = []const u8;
pub type Network_ReportId = []const u8;
/// An object representing a report generated by the Reporting API.
pub struct Network_ReportingApiReport {
    id: Network_ReportId = "",
    initiatorUrl: []const u8 = "",
    destination: []const u8 = "",
    type_: []const u8 = "", // protocol field: "type"
    timestamp: Network_TimeSinceEpoch = 0.0,
    depth: f64 = 0.0,
    completedAttempts: f64 = 0.0,
    body: ProtocolStringMap = "",
    status: Network_ReportStatus = "",
}
pub struct Network_ReportingApiEndpoint {
    url: []const u8 = "",
    groupName: []const u8 = "",
}
/// Unique identifier for a device bound session.
pub struct Network_DeviceBoundSessionKey {
    site: []const u8 = "",
    id: []const u8 = "",
}
/// How a device bound session was used during a request.
pub struct Network_DeviceBoundSessionWithUsage {
    sessionKey: Network_DeviceBoundSessionKey = .{},
    usage: []const u8 = "", // enum: "NotInScope"|"InScopeRefreshNotYetNeeded"|"InScopeRefreshNotAllowed"|"ProactiveRefreshNotPossible"|"ProactiveRefreshA...
}
/// A device bound session's cookie craving.
pub struct Network_DeviceBoundSessionCookieCraving {
    name: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "",
    secure: bool = false,
    httpOnly: bool = false,
    sameSite: Network_CookieSameSite? = null,
}
/// A device bound session's inclusion URL rule.
pub struct Network_DeviceBoundSessionUrlRule {
    ruleType: []const u8 = "", // enum: "Exclude"|"Include"
    hostPattern: []const u8 = "",
    pathPrefix: []const u8 = "",
}
/// A device bound session's inclusion rules.
pub struct Network_DeviceBoundSessionInclusionRules {
    origin: []const u8 = "",
    includeSite: bool = false,
    urlRules: []const Network_DeviceBoundSessionUrlRule = &.{},
}
/// A device bound session.
pub struct Network_DeviceBoundSession {
    key: Network_DeviceBoundSessionKey = .{},
    refreshUrl: []const u8 = "",
    inclusionRules: Network_DeviceBoundSessionInclusionRules = .{},
    cookieCravings: []const Network_DeviceBoundSessionCookieCraving = &.{},
    expiryDate: Network_TimeSinceEpoch = 0.0,
    cachedChallenge: ([]const u8)? = null,
    allowedRefreshInitiators: []const []const u8 = &.{},
}
/// A unique identifier for a device bound session event.
pub type Network_DeviceBoundSessionEventId = []const u8;
/// A fetch result for a device bound session creation or refresh.
/// enum: "Success"|"SigningKeyGenerationError"|"AttestationKeyGenerationError"|"SigningError"|"TransientSigningError"|"ServerR...
pub type Network_DeviceBoundSessionFetchResult = []const u8;
/// Details about a failed device bound session network request.
pub struct Network_DeviceBoundSessionFailedRequest {
    requestUrl: []const u8 = "",
    netError: ([]const u8)? = null,
    responseError: f64? = null,
    responseErrorBody: ([]const u8)? = null,
}
/// Session event details specific to creation.
pub struct Network_CreationEventDetails {
    fetchResult: Network_DeviceBoundSessionFetchResult = "",
    newSession: Network_DeviceBoundSession? = null,
    failedRequest: Network_DeviceBoundSessionFailedRequest? = null,
}
/// Session event details specific to refresh.
pub struct Network_RefreshEventDetails {
    refreshResult: []const u8 = "", // enum: "Refreshed"|"InitializedService"|"Unreachable"|"ServerError"|"FatalError"|"SigningQuotaExceeded"|"RefreshedAsWaiter"|...
    fetchResult: Network_DeviceBoundSessionFetchResult? = null,
    newSession: Network_DeviceBoundSession? = null,
    wasFullyProactiveRefresh: bool = false,
    failedRequest: Network_DeviceBoundSessionFailedRequest? = null,
}
/// Session event details specific to termination.
pub struct Network_TerminationEventDetails {
    deletionReason: []const u8 = "", // enum: "Expired"|"FailedToRestoreKey"|"FailedToUnwrapKey"|"StoragePartitionCleared"|"ClearBrowsingData"|"ServerRequested"|"I...
}
/// Session event details specific to challenges.
pub struct Network_ChallengeEventDetails {
    challengeResult: []const u8 = "", // enum: "Success"|"NoSessionId"|"NoSessionMatch"|"CantSetBoundCookie"
    challenge: []const u8 = "",
}
/// An object providing the result of a network resource load.
pub struct Network_LoadNetworkResourcePageResult {
    success: bool = false,
    netError: f64? = null,
    netErrorName: ([]const u8)? = null,
    httpStatusCode: f64? = null,
    stream: IO_StreamHandle? = null,
    headers: Network_Headers? = null,
}
/// An options object that may be extended later to better support CORS,
pub struct Network_LoadNetworkResourceOptions {
    disableCache: bool = false,
    includeCredentials: bool = false,
}
/// Fired when data chunk was received over the network.
pub struct Network_dataReceivedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    dataLength: f64 = 0.0,
    encodedDataLength: f64 = 0.0,
    data: ProtocolBinary? = null,
}
/// Fired when EventSource message is received.
pub struct Network_eventSourceMessageReceivedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    eventName: []const u8 = "",
    eventId: []const u8 = "",
    data: []const u8 = "",
}
/// Fired when HTTP request has failed to load.
pub struct Network_loadingFailedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    type_: Network_ResourceType = "", // protocol field: "type"
    errorText: []const u8 = "",
    canceled: bool? = null,
    blockedReason: Network_BlockedReason? = null,
    corsErrorStatus: Network_CorsErrorStatus? = null,
}
/// Fired when HTTP request has finished loading.
pub struct Network_loadingFinishedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    encodedDataLength: f64 = 0.0,
    encodedBodyLength: f64? = null,
}
/// Fired if request ended up loading from cache.
pub struct Network_requestServedFromCachePayload {
    requestId: Network_RequestId = "",
}
/// Fired when page is about to send HTTP request.
pub struct Network_requestWillBeSentPayload {
    requestId: Network_RequestId = "",
    loaderId: Network_LoaderId = "",
    documentURL: []const u8 = "",
    request: Network_Request = .{},
    timestamp: Network_MonotonicTime = 0.0,
    wallTime: Network_TimeSinceEpoch = 0.0,
    initiator: Network_Initiator = .{},
    redirectHasExtraInfo: bool = false,
    redirectResponse: Network_Response? = null,
    type_: Network_ResourceType? = null, // protocol field: "type"
    frameId: Page_FrameId? = null,
    hasUserGesture: bool? = null,
    renderBlockingBehavior: Network_RenderBlockingBehavior? = null,
}
/// Fired when resource loading priority is changed
pub struct Network_resourceChangedPriorityPayload {
    requestId: Network_RequestId = "",
    newPriority: Network_ResourcePriority = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when a signed exchange was received over the network
pub struct Network_signedExchangeReceivedPayload {
    requestId: Network_RequestId = "",
    info: Network_SignedExchangeInfo = .{},
}
/// Fired when HTTP response is available.
pub struct Network_responseReceivedPayload {
    requestId: Network_RequestId = "",
    loaderId: Network_LoaderId = "",
    timestamp: Network_MonotonicTime = 0.0,
    type_: Network_ResourceType = "", // protocol field: "type"
    response: Network_Response = .{},
    hasExtraInfo: bool = false,
    frameId: Page_FrameId? = null,
}
/// Fired when WebSocket is closed.
pub struct Network_webSocketClosedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired upon WebSocket creation.
pub struct Network_webSocketCreatedPayload {
    requestId: Network_RequestId = "",
    url: []const u8 = "",
    initiator: Network_Initiator? = null,
}
/// Fired when WebSocket message error occurs.
pub struct Network_webSocketFrameErrorPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    errorMessage: []const u8 = "",
}
/// Fired when WebSocket message is received.
pub struct Network_webSocketFrameReceivedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    response: Network_WebSocketFrame = .{},
}
/// Fired when WebSocket message is sent.
pub struct Network_webSocketFrameSentPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    response: Network_WebSocketFrame = .{},
}
/// Fired when WebSocket handshake response becomes available.
pub struct Network_webSocketHandshakeResponseReceivedPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    response: Network_WebSocketResponse = .{},
}
/// Fired when WebSocket is about to initiate handshake.
pub struct Network_webSocketWillSendHandshakeRequestPayload {
    requestId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
    wallTime: Network_TimeSinceEpoch = 0.0,
    request: Network_WebSocketRequest = .{},
}
/// Fired upon WebTransport creation.
pub struct Network_webTransportCreatedPayload {
    transportId: Network_RequestId = "",
    url: []const u8 = "",
    timestamp: Network_MonotonicTime = 0.0,
    initiator: Network_Initiator? = null,
}
/// Fired when WebTransport handshake is finished.
pub struct Network_webTransportConnectionEstablishedPayload {
    transportId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when WebTransport is disposed.
pub struct Network_webTransportClosedPayload {
    transportId: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired upon direct_socket.TCPSocket creation.
pub struct Network_directTCPSocketCreatedPayload {
    identifier: Network_RequestId = "",
    remoteAddr: []const u8 = "",
    remotePort: f64 = 0.0,
    options: Network_DirectTCPSocketOptions = .{},
    timestamp: Network_MonotonicTime = 0.0,
    initiator: Network_Initiator? = null,
}
/// Fired when direct_socket.TCPSocket connection is opened.
pub struct Network_directTCPSocketOpenedPayload {
    identifier: Network_RequestId = "",
    remoteAddr: []const u8 = "",
    remotePort: f64 = 0.0,
    timestamp: Network_MonotonicTime = 0.0,
    localAddr: ([]const u8)? = null,
    localPort: f64? = null,
}
/// Fired when direct_socket.TCPSocket is aborted.
pub struct Network_directTCPSocketAbortedPayload {
    identifier: Network_RequestId = "",
    errorMessage: Network_ErrorReason = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when direct_socket.TCPSocket is closed.
pub struct Network_directTCPSocketClosedPayload {
    identifier: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when data is sent to tcp direct socket stream.
pub struct Network_directTCPSocketChunkSentPayload {
    identifier: Network_RequestId = "",
    data: ProtocolBinary = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when data is received from tcp direct socket stream.
pub struct Network_directTCPSocketChunkReceivedPayload {
    identifier: Network_RequestId = "",
    data: ProtocolBinary = "",
    timestamp: Network_MonotonicTime = 0.0,
}
pub struct Network_directUDPSocketJoinedMulticastGroupPayload {
    identifier: Network_RequestId = "",
    IPAddress: []const u8 = "",
    sourceAddress: ([]const u8)? = null,
}
pub struct Network_directUDPSocketLeftMulticastGroupPayload {
    identifier: Network_RequestId = "",
    IPAddress: []const u8 = "",
    sourceAddress: ([]const u8)? = null,
}
/// Fired upon direct_socket.UDPSocket creation.
pub struct Network_directUDPSocketCreatedPayload {
    identifier: Network_RequestId = "",
    options: Network_DirectUDPSocketOptions = .{},
    timestamp: Network_MonotonicTime = 0.0,
    initiator: Network_Initiator? = null,
}
/// Fired when direct_socket.UDPSocket connection is opened.
pub struct Network_directUDPSocketOpenedPayload {
    identifier: Network_RequestId = "",
    localAddr: []const u8 = "",
    localPort: f64 = 0.0,
    timestamp: Network_MonotonicTime = 0.0,
    remoteAddr: ([]const u8)? = null,
    remotePort: f64? = null,
}
/// Fired when direct_socket.UDPSocket is aborted.
pub struct Network_directUDPSocketAbortedPayload {
    identifier: Network_RequestId = "",
    errorMessage: Network_ErrorReason = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when direct_socket.UDPSocket is closed.
pub struct Network_directUDPSocketClosedPayload {
    identifier: Network_RequestId = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when message is sent to udp direct socket stream.
pub struct Network_directUDPSocketChunkSentPayload {
    identifier: Network_RequestId = "",
    message: Network_DirectUDPMessage = .{},
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when message is received from udp direct socket stream.
pub struct Network_directUDPSocketChunkReceivedPayload {
    identifier: Network_RequestId = "",
    message: Network_DirectUDPMessage = .{},
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when additional information about a requestWillBeSent event is available from the
pub struct Network_requestWillBeSentExtraInfoPayload {
    requestId: Network_RequestId = "",
    associatedCookies: []const Network_AssociatedCookie = &.{},
    headers: Network_Headers = "",
    connectTiming: Network_ConnectTiming = .{},
    deviceBoundSessionUsages: ([]const Network_DeviceBoundSessionWithUsage)? = null,
    clientSecurityState: Network_ClientSecurityState? = null,
    siteHasCookieInOtherPartition: bool? = null,
    appliedNetworkConditionsId: ([]const u8)? = null,
}
/// Fired when additional information about a responseReceived event is available from the network
pub struct Network_responseReceivedExtraInfoPayload {
    requestId: Network_RequestId = "",
    blockedCookies: []const Network_BlockedSetCookieWithReason = &.{},
    headers: Network_Headers = "",
    resourceIPAddressSpace: Network_IPAddressSpace = "",
    statusCode: f64 = 0.0,
    headersText: ([]const u8)? = null,
    cookiePartitionKey: Network_CookiePartitionKey? = null,
    cookiePartitionKeyOpaque: bool? = null,
    exemptedCookies: ([]const Network_ExemptedSetCookieWithReason)? = null,
}
/// Fired when 103 Early Hints headers is received in addition to the common response.
pub struct Network_responseReceivedEarlyHintsPayload {
    requestId: Network_RequestId = "",
    headers: Network_Headers = "",
}
/// Fired exactly once for each Trust Token operation. Depending on
pub struct Network_trustTokenOperationDonePayload {
    status: []const u8 = "", // enum: "Ok"|"InvalidArgument"|"MissingIssuerKeys"|"FailedPrecondition"|"ResourceExhausted"|"AlreadyExists"|"ResourceLimited"...
    type_: Network_TrustTokenOperationType = "", // protocol field: "type"
    requestId: Network_RequestId = "",
    topLevelOrigin: ([]const u8)? = null,
    issuerOrigin: ([]const u8)? = null,
    issuedTokenCount: f64? = null,
}
/// Fired once security policy has been updated.
pub type Network_policyUpdatedPayload = ProtocolJsonValue;
/// Is sent whenever a new report is added.
pub struct Network_reportingApiReportAddedPayload {
    report: Network_ReportingApiReport = .{},
}
pub struct Network_reportingApiReportUpdatedPayload {
    report: Network_ReportingApiReport = .{},
}
pub struct Network_reportingApiEndpointsChangedForOriginPayload {
    origin: []const u8 = "",
    endpoints: []const Network_ReportingApiEndpoint = &.{},
}
/// Triggered when the initial set of device bound sessions is added.
pub struct Network_deviceBoundSessionsAddedPayload {
    sessions: []const Network_DeviceBoundSession = &.{},
}
/// Triggered when a device bound session event occurs.
pub struct Network_deviceBoundSessionEventOccurredPayload {
    eventId: Network_DeviceBoundSessionEventId = "",
    site: []const u8 = "",
    succeeded: bool = false,
    sessionId: ([]const u8)? = null,
    creationEventDetails: Network_CreationEventDetails? = null,
    refreshEventDetails: Network_RefreshEventDetails? = null,
    terminationEventDetails: Network_TerminationEventDetails? = null,
    challengeEventDetails: Network_ChallengeEventDetails? = null,
}
/// Tells whether clearing browser cache is supported.
pub struct Network_canClearBrowserCacheParameters {}
pub struct Network_canClearBrowserCacheReturnValue {
    result: bool = false,
}
/// Tells whether clearing browser cookies is supported.
pub struct Network_canClearBrowserCookiesParameters {}
pub struct Network_canClearBrowserCookiesReturnValue {
    result: bool = false,
}
/// Tells whether emulation of network conditions is supported.
pub struct Network_canEmulateNetworkConditionsParameters {}
pub struct Network_canEmulateNetworkConditionsReturnValue {
    result: bool = false,
}
/// Clears browser cache.
pub struct Network_clearBrowserCacheParameters {}
pub struct Network_clearBrowserCacheReturnValue {}
/// Clears browser cookies.
pub struct Network_clearBrowserCookiesParameters {}
pub struct Network_clearBrowserCookiesReturnValue {}
/// Deletes browser cookies with matching name and url or domain/path/partitionKey pair.
pub struct Network_deleteCookiesParameters {
    name: []const u8 = "",
    url: ([]const u8)? = null,
    domain: ([]const u8)? = null,
    path: ([]const u8)? = null,
    partitionKey: Network_CookiePartitionKey? = null,
}
pub struct Network_deleteCookiesReturnValue {}
/// Disables network tracking, prevents network events from being sent to the client.
pub struct Network_disableParameters {}
pub struct Network_disableReturnValue {}
/// Activates emulation of network conditions. This command is deprecated in favor of the emulateNetworkConditionsByRule
pub struct Network_emulateNetworkConditionsParameters {
    offline: bool = false,
    latency: f64 = 0.0,
    downloadThroughput: f64 = 0.0,
    uploadThroughput: f64 = 0.0,
    connectionType: Network_ConnectionType? = null,
    packetLoss: f64? = null,
    packetQueueLength: f64? = null,
    packetReordering: bool? = null,
}
pub struct Network_emulateNetworkConditionsReturnValue {}
/// Activates emulation of network conditions for individual requests using URL match patterns. Unlike the deprecated
pub struct Network_emulateNetworkConditionsByRuleParameters {
    offline: bool? = null,
    emulateOfflineServiceWorker: bool? = null,
    matchedNetworkConditions: []const Network_NetworkConditions = &.{},
}
pub struct Network_emulateNetworkConditionsByRuleReturnValue {
    ruleIds: []const []const u8 = &.{},
}
/// Override the state of navigator.onLine and navigator.connection.
pub struct Network_overrideNetworkStateParameters {
    offline: bool = false,
    latency: f64 = 0.0,
    downloadThroughput: f64 = 0.0,
    uploadThroughput: f64 = 0.0,
    connectionType: Network_ConnectionType? = null,
}
pub struct Network_overrideNetworkStateReturnValue {}
/// Enables network tracking, network events will now be delivered to the client.
pub struct Network_enableParameters {
    maxTotalBufferSize: f64? = null,
    maxResourceBufferSize: f64? = null,
    maxPostDataSize: f64? = null,
    reportDirectSocketTraffic: bool? = null,
    enableDurableMessages: bool? = null,
}
pub struct Network_enableReturnValue {}
/// Configures storing response bodies outside of renderer, so that these survive
pub struct Network_configureDurableMessagesParameters {
    maxTotalBufferSize: f64? = null,
    maxResourceBufferSize: f64? = null,
}
pub struct Network_configureDurableMessagesReturnValue {}
/// Returns all browser cookies. Depending on the backend support, will return detailed cookie
pub struct Network_getAllCookiesParameters {}
pub struct Network_getAllCookiesReturnValue {
    cookies: []const Network_Cookie = &.{},
}
/// Returns the DER-encoded certificate.
pub struct Network_getCertificateParameters {
    origin: []const u8 = "",
}
pub struct Network_getCertificateReturnValue {
    tableNames: []const []const u8 = &.{},
}
/// Returns all browser cookies for the current URL. Depending on the backend support, will return
pub struct Network_getCookiesParameters {
    urls: ([]const []const u8)? = null,
}
pub struct Network_getCookiesReturnValue {
    cookies: []const Network_Cookie = &.{},
}
/// Returns content served for the given request.
pub struct Network_getResponseBodyParameters {
    requestId: Network_RequestId = "",
}
pub struct Network_getResponseBodyReturnValue {
    body: []const u8 = "",
    base64Encoded: bool = false,
}
/// Returns post data sent with the request. Returns an error when no data was sent with the request.
pub struct Network_getRequestPostDataParameters {
    requestId: Network_RequestId = "",
}
pub struct Network_getRequestPostDataReturnValue {
    postData: []const u8 = "",
    base64Encoded: bool = false,
}
/// This method sends a new XMLHttpRequest which is identical to the original one. The following
pub struct Network_replayXHRParameters {
    requestId: Network_RequestId = "",
}
pub struct Network_replayXHRReturnValue {}
/// Searches for given string in response content.
pub struct Network_searchInResponseBodyParameters {
    requestId: Network_RequestId = "",
    query: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Network_searchInResponseBodyReturnValue {
    result: []const Debugger_SearchMatch = &.{},
}
/// Blocks URLs from loading.
pub struct Network_setBlockedURLsParameters {
    urlPatterns: ([]const Network_BlockPattern)? = null,
    urls: ([]const []const u8)? = null,
}
pub struct Network_setBlockedURLsReturnValue {}
/// Toggles ignoring of service worker for each request.
pub struct Network_setBypassServiceWorkerParameters {
    bypass: bool = false,
}
pub struct Network_setBypassServiceWorkerReturnValue {}
/// Toggles ignoring cache for each request. If `true`, cache will not be used.
pub struct Network_setCacheDisabledParameters {
    cacheDisabled: bool = false,
}
pub struct Network_setCacheDisabledReturnValue {}
/// Sets a cookie with the given cookie data; may overwrite equivalent cookies if they exist.
pub struct Network_setCookieParameters {
    name: []const u8 = "",
    value: []const u8 = "",
    url: ([]const u8)? = null,
    domain: ([]const u8)? = null,
    path: ([]const u8)? = null,
    secure: bool? = null,
    httpOnly: bool? = null,
    sameSite: Network_CookieSameSite? = null,
    expires: Network_TimeSinceEpoch? = null,
    priority: Network_CookiePriority? = null,
    sourceScheme: Network_CookieSourceScheme? = null,
    sourcePort: f64? = null,
    partitionKey: Network_CookiePartitionKey? = null,
}
pub struct Network_setCookieReturnValue {
    success: bool = false,
}
/// Sets given cookies.
pub struct Network_setCookiesParameters {
    cookies: []const Network_CookieParam = &.{},
}
pub struct Network_setCookiesReturnValue {}
/// Specifies whether to always send extra HTTP headers with the requests from this page.
pub struct Network_setExtraHTTPHeadersParameters {
    headers: Network_Headers = "",
}
pub struct Network_setExtraHTTPHeadersReturnValue {}
/// Specifies whether to attach a page script stack id in requests
pub struct Network_setAttachDebugStackParameters {
    enabled: bool = false,
}
pub struct Network_setAttachDebugStackReturnValue {}
/// Allows overriding user agent with the given string.
pub struct Network_setUserAgentOverrideParameters {
    userAgent: []const u8 = "",
    acceptLanguage: ([]const u8)? = null,
    platform: ([]const u8)? = null,
    userAgentMetadata: Emulation_UserAgentMetadata? = null,
}
pub struct Network_setUserAgentOverrideReturnValue {}
/// Enables streaming of the response for the given requestId.
pub struct Network_streamResourceContentParameters {
    requestId: Network_RequestId = "",
}
pub struct Network_streamResourceContentReturnValue {
    bufferedData: ProtocolBinary = "",
}
/// Returns information about the COEP/COOP isolation status.
pub struct Network_getSecurityIsolationStatusParameters {
    frameId: Page_FrameId? = null,
}
pub struct Network_getSecurityIsolationStatusReturnValue {
    status: Network_SecurityIsolationStatus = .{},
}
/// Enables tracking for the Reporting API, events generated by the Reporting API will now be delivered to the client.
pub struct Network_enableReportingApiParameters {
    enable: bool = false,
}
pub struct Network_enableReportingApiReturnValue {}
/// Sets up tracking device bound sessions and fetching of initial set of sessions.
pub struct Network_enableDeviceBoundSessionsParameters {
    enable: bool = false,
}
pub struct Network_enableDeviceBoundSessionsReturnValue {}
/// Deletes a device bound session.
pub struct Network_deleteDeviceBoundSessionParameters {
    key: Network_DeviceBoundSessionKey = .{},
}
pub struct Network_deleteDeviceBoundSessionReturnValue {}
/// Fetches the schemeful site for a specific origin.
pub struct Network_fetchSchemefulSiteParameters {
    origin: []const u8 = "",
}
pub struct Network_fetchSchemefulSiteReturnValue {
    schemefulSite: []const u8 = "",
}
/// Fetches the resource and returns the content.
pub struct Network_loadNetworkResourceParameters {
    frameId: Page_FrameId? = null,
    url: []const u8 = "",
    options: Network_LoadNetworkResourceOptions = .{},
}
pub struct Network_loadNetworkResourceReturnValue {
    resource: Network_LoadNetworkResourcePageResult = .{},
}
/// Sets Controls for third-party cookie access
pub struct Network_setCookieControlsParameters {
    enableThirdPartyCookieRestriction: bool = false,
}
pub struct Network_setCookieControlsReturnValue {}
