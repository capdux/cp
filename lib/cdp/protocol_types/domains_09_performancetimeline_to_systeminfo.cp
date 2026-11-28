// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue, ProtocolStringMap } from "./common";
import {
    Network_Cookie,
    Network_CookieParam,
    Network_LoaderId,
    Network_RequestId,
    Network_TimeSinceEpoch,
} from "./domain_06_network";
import { Browser_BrowserContextID } from "./domains_02_browser_to_cachestorage";
import { DOM_BackendNodeId, DOM_Rect } from "./domains_03_cast_to_domsnapshot";
import { Page_FrameId } from "./domains_08_page_to_performance";
import { Target_TargetID } from "./domains_10_target_to_console";

// ============================================================================
// Domain: PerformanceTimeline
// Reporting of performance timeline events, as specified in
// https://w3c.github.io/performance-timeline/#dom-performanceobserver.
// ============================================================================

/// See https://github.com/WICG/LargestContentfulPaint and largest_contentful_paint.idl
pub struct PerformanceTimeline_LargestContentfulPaint {
    renderTime: Network_TimeSinceEpoch = 0.0,
    loadTime: Network_TimeSinceEpoch = 0.0,
    size: f64 = 0.0,
    elementId: ([]const u8)? = null,
    url: ([]const u8)? = null,
    nodeId: DOM_BackendNodeId? = null,
}
pub struct PerformanceTimeline_LayoutShiftAttribution {
    previousRect: DOM_Rect = .{},
    currentRect: DOM_Rect = .{},
    nodeId: DOM_BackendNodeId? = null,
}
/// See https://wicg.github.io/layout-instability/#sec-layout-shift and layout_shift.idl
pub struct PerformanceTimeline_LayoutShift {
    value: f64 = 0.0,
    hadRecentInput: bool = false,
    lastInputTime: Network_TimeSinceEpoch = 0.0,
    sources: []const PerformanceTimeline_LayoutShiftAttribution = &.{},
}
pub struct PerformanceTimeline_TimelineEvent {
    frameId: Page_FrameId = "",
    type_: []const u8 = "", // protocol field: "type"
    name: []const u8 = "",
    time: Network_TimeSinceEpoch = 0.0,
    duration: f64? = null,
    lcpDetails: PerformanceTimeline_LargestContentfulPaint? = null,
    layoutShiftDetails: PerformanceTimeline_LayoutShift? = null,
}
/// Sent when a performance timeline event is added. See reportPerformanceTimeline method.
pub struct PerformanceTimeline_timelineEventAddedPayload {
    event: PerformanceTimeline_TimelineEvent = .{},
}
/// Previously buffered events would be reported before method returns.
pub struct PerformanceTimeline_enableParameters {
    eventTypes: []const []const u8 = &.{},
}
pub struct PerformanceTimeline_enableReturnValue {}

// ============================================================================
// Domain: Preload
// ============================================================================

/// Unique id
pub type Preload_RuleSetId = []const u8;
/// Corresponds to SpeculationRuleSet
pub struct Preload_RuleSet {
    id: Preload_RuleSetId = "",
    loaderId: Network_LoaderId = "",
    sourceText: []const u8 = "",
    backendNodeId: DOM_BackendNodeId? = null,
    url: ([]const u8)? = null,
    requestId: Network_RequestId? = null,
    errorType: Preload_RuleSetErrorType? = null,
    errorMessage: ([]const u8)? = null,
    tag: ([]const u8)? = null,
}
/// enum: "SourceIsNotJsonObject"|"InvalidRulesSkipped"|"InvalidRulesetLevelTag"
pub type Preload_RuleSetErrorType = []const u8;
/// The type of preloading attempted. It corresponds to
/// enum: "Prefetch"|"Prerender"|"PrerenderUntilScript"
pub type Preload_SpeculationAction = []const u8;
/// Corresponds to mojom::SpeculationTargetHint.
/// enum: "Blank"|"Self"
pub type Preload_SpeculationTargetHint = []const u8;
/// A key that identifies a preloading attempt.
pub struct Preload_PreloadingAttemptKey {
    loaderId: Network_LoaderId = "",
    action: Preload_SpeculationAction = "",
    url: []const u8 = "",
    formSubmission: bool? = null,
    targetHint: Preload_SpeculationTargetHint? = null,
}
/// Lists sources for a preloading attempt, specifically the ids of rule sets
pub struct Preload_PreloadingAttemptSource {
    key: Preload_PreloadingAttemptKey = .{},
    ruleSetIds: []const Preload_RuleSetId = &.{},
    nodeIds: []const DOM_BackendNodeId = &.{},
}
/// Chrome manages different types of preloads together using a
pub type Preload_PreloadPipelineId = []const u8;
/// List of FinalStatus reasons for Prerender2.
/// enum: "Activated"|"Destroyed"|"LowEndDevice"|"InvalidSchemeRedirect"|"InvalidSchemeNavigation"|"NavigationRequestBlockedByC...
pub type Preload_PrerenderFinalStatus = []const u8;
/// Preloading status values, see also PreloadingTriggeringOutcome. This
/// enum: "Pending"|"Running"|"Ready"|"Success"|"Failure"|"NotSupported"
pub type Preload_PreloadingStatus = []const u8;
/// TODO(https://crbug.com/1384419): revisit the list of PrefetchStatus and
/// enum: "PrefetchAllowed"|"PrefetchFailedIneligibleRedirect"|"PrefetchFailedInvalidRedirect"|"PrefetchFailedMIMENotSupported"...
pub type Preload_PrefetchStatus = []const u8;
/// Information of headers to be displayed when the header mismatch occurred.
pub struct Preload_PrerenderMismatchedHeaders {
    headerName: []const u8 = "",
    initialValue: ([]const u8)? = null,
    activationValue: ([]const u8)? = null,
}
/// Upsert. Currently, it is only emitted when a rule set added.
pub struct Preload_ruleSetUpdatedPayload {
    ruleSet: Preload_RuleSet = .{},
}
pub struct Preload_ruleSetRemovedPayload {
    id: Preload_RuleSetId = "",
}
/// Fired when a preload enabled state is updated.
pub struct Preload_preloadEnabledStateUpdatedPayload {
    disabledByPreference: bool = false,
    disabledByDataSaver: bool = false,
    disabledByBatterySaver: bool = false,
    disabledByHoldbackPrefetchSpeculationRules: bool = false,
    disabledByHoldbackPrerenderSpeculationRules: bool = false,
}
/// Fired when a prefetch attempt is updated.
pub struct Preload_prefetchStatusUpdatedPayload {
    key: Preload_PreloadingAttemptKey = .{},
    pipelineId: Preload_PreloadPipelineId = "",
    initiatingFrameId: Page_FrameId = "",
    prefetchUrl: []const u8 = "",
    status: Preload_PreloadingStatus = "",
    prefetchStatus: Preload_PrefetchStatus = "",
    requestId: Network_RequestId = "",
}
/// Fired when a prerender attempt is updated.
pub struct Preload_prerenderStatusUpdatedPayload {
    key: Preload_PreloadingAttemptKey = .{},
    pipelineId: Preload_PreloadPipelineId = "",
    status: Preload_PreloadingStatus = "",
    effectiveAction: Preload_SpeculationAction? = null,
    prerenderStatus: Preload_PrerenderFinalStatus? = null,
    disallowedMojoInterface: ([]const u8)? = null,
    mismatchedHeaders: ([]const Preload_PrerenderMismatchedHeaders)? = null,
}
/// Send a list of sources for all preloading attempts in a document.
pub struct Preload_preloadingAttemptSourcesUpdatedPayload {
    loaderId: Network_LoaderId = "",
    preloadingAttemptSources: []const Preload_PreloadingAttemptSource = &.{},
}
pub struct Preload_enableParameters {}
pub struct Preload_enableReturnValue {}
pub struct Preload_disableParameters {}
pub struct Preload_disableReturnValue {}

// ============================================================================
// Domain: Security
// ============================================================================

/// An internal certificate ID value.
pub type Security_CertificateId = f64;
/// A description of mixed content (HTTP resources on HTTPS pages), as defined by
/// enum: "blockable"|"optionally-blockable"|"none"
pub type Security_MixedContentType = []const u8;
/// The security level of a page or resource.
/// enum: "unknown"|"neutral"|"insecure"|"secure"|"info"|"insecure-broken"
pub type Security_SecurityState = []const u8;
/// Details about the security state of the page certificate.
pub struct Security_CertificateSecurityState {
    protocol: []const u8 = "",
    keyExchange: []const u8 = "",
    keyExchangeGroup: ([]const u8)? = null,
    cipher: []const u8 = "",
    mac: ([]const u8)? = null,
    certificate: []const []const u8 = &.{},
    subjectName: []const u8 = "",
    issuer: []const u8 = "",
    validFrom: Network_TimeSinceEpoch = 0.0,
    validTo: Network_TimeSinceEpoch = 0.0,
    certificateNetworkError: ([]const u8)? = null,
    certificateHasWeakSignature: bool = false,
    certificateHasSha1Signature: bool = false,
    modernSSL: bool = false,
    obsoleteSslProtocol: bool = false,
    obsoleteSslKeyExchange: bool = false,
    obsoleteSslCipher: bool = false,
    obsoleteSslSignature: bool = false,
}
/// enum: "badReputation"|"lookalike"
pub type Security_SafetyTipStatus = []const u8;
pub struct Security_SafetyTipInfo {
    safetyTipStatus: Security_SafetyTipStatus = "",
    safeUrl: ([]const u8)? = null,
}
/// Security state information about the page.
pub struct Security_VisibleSecurityState {
    securityState: Security_SecurityState = "",
    certificateSecurityState: Security_CertificateSecurityState? = null,
    safetyTipInfo: Security_SafetyTipInfo? = null,
    securityStateIssueIds: []const []const u8 = &.{},
}
/// An explanation of an factor contributing to the security state.
pub struct Security_SecurityStateExplanation {
    securityState: Security_SecurityState = "",
    title: []const u8 = "",
    summary: []const u8 = "",
    description: []const u8 = "",
    mixedContentType: Security_MixedContentType = "",
    certificate: []const []const u8 = &.{},
    recommendations: ([]const []const u8)? = null,
}
/// Information about insecure content on the page.
pub struct Security_InsecureContentStatus {
    ranMixedContent: bool = false,
    displayedMixedContent: bool = false,
    containedMixedForm: bool = false,
    ranContentWithCertErrors: bool = false,
    displayedContentWithCertErrors: bool = false,
    ranInsecureContentStyle: Security_SecurityState = "",
    displayedInsecureContentStyle: Security_SecurityState = "",
}
/// The action to take when a certificate error occurs. continue will continue processing the
/// enum: "continue"|"cancel"
pub type Security_CertificateErrorAction = []const u8;
/// There is a certificate error. If overriding certificate errors is enabled, then it should be
pub struct Security_certificateErrorPayload {
    eventId: f64 = 0.0,
    errorType: []const u8 = "",
    requestURL: []const u8 = "",
}
/// The security state of the page changed.
pub struct Security_visibleSecurityStateChangedPayload {
    visibleSecurityState: Security_VisibleSecurityState = .{},
}
/// The security state of the page changed. No longer being sent.
pub struct Security_securityStateChangedPayload {
    securityState: Security_SecurityState = "",
    schemeIsCryptographic: bool = false,
    explanations: []const Security_SecurityStateExplanation = &.{},
    insecureContentStatus: Security_InsecureContentStatus = .{},
    summary: ([]const u8)? = null,
}
/// Disables tracking security state changes.
pub struct Security_disableParameters {}
pub struct Security_disableReturnValue {}
/// Enables tracking security state changes.
pub struct Security_enableParameters {}
pub struct Security_enableReturnValue {}
/// Enable/disable whether all certificate errors should be ignored.
pub struct Security_setIgnoreCertificateErrorsParameters {
    ignore: bool = false,
}
pub struct Security_setIgnoreCertificateErrorsReturnValue {}
/// Handles a certificate error that fired a certificateError event.
pub struct Security_handleCertificateErrorParameters {
    eventId: f64 = 0.0,
    action: Security_CertificateErrorAction = "",
}
pub struct Security_handleCertificateErrorReturnValue {}
/// Enable/disable overriding certificate errors. If enabled, all certificate error events need to
pub struct Security_setOverrideCertificateErrorsParameters {
    override: bool = false,
}
pub struct Security_setOverrideCertificateErrorsReturnValue {}

// ============================================================================
// Domain: ServiceWorker
// ============================================================================

pub type ServiceWorker_RegistrationID = []const u8;
/// ServiceWorker registration.
pub struct ServiceWorker_ServiceWorkerRegistration {
    registrationId: ServiceWorker_RegistrationID = "",
    scopeURL: []const u8 = "",
    isDeleted: bool = false,
}
/// enum: "stopped"|"starting"|"running"|"stopping"
pub type ServiceWorker_ServiceWorkerVersionRunningStatus = []const u8;
/// enum: "new"|"installing"|"installed"|"activating"|"activated"|"redundant"
pub type ServiceWorker_ServiceWorkerVersionStatus = []const u8;
/// Mostly corresponds to `RouterCondition` in ServiceWorker spec
pub struct ServiceWorker_ServiceWorkerRouterCondition {
    urlPattern: ([]const u8)? = null,
    requestMethod: ([]const u8)? = null,
    requestMode: ([]const u8)? = null,
    requestDestination: ([]const u8)? = null,
    runningStatus: ServiceWorker_ServiceWorkerVersionRunningStatus? = null,
    or_: ([]const ServiceWorker_ServiceWorkerRouterCondition)? = null, // protocol field: "or"
    not_: (*const ServiceWorker_ServiceWorkerRouterCondition)? = null, // protocol field: "not"
}
/// enum: "cache"|"fetchEvent"|"network"|"raceNetworkAndFetchHandler"|"raceNetworkAndCache"|"sourceDict"
pub type ServiceWorker_ServiceWorkerRouterSourceType = []const u8;
/// https://www.w3.org/TR/service-workers/#dictdef-routersourcedict
pub struct ServiceWorker_ServiceWorkerRouterSourceDict {
    cacheName: []const u8 = "",
}
/// Corresponds to `RouterSource` in the spec while the representation is different as follows.
pub struct ServiceWorker_ServiceWorkerRouterSource {
    type_: ServiceWorker_ServiceWorkerRouterSourceType = "", // protocol field: "type"
    sourceDict: ServiceWorker_ServiceWorkerRouterSourceDict? = null,
}
pub struct ServiceWorker_ServiceWorkerRouterRule {
    condition: ServiceWorker_ServiceWorkerRouterCondition = .{},
    source: ServiceWorker_ServiceWorkerRouterSource = .{},
    id: f64 = 0.0,
}
/// ServiceWorker version.
pub struct ServiceWorker_ServiceWorkerVersion {
    versionId: []const u8 = "",
    registrationId: ServiceWorker_RegistrationID = "",
    scriptURL: []const u8 = "",
    runningStatus: ServiceWorker_ServiceWorkerVersionRunningStatus = "",
    status: ServiceWorker_ServiceWorkerVersionStatus = "",
    scriptLastModified: f64? = null,
    scriptResponseTime: f64? = null,
    controlledClients: ([]const Target_TargetID)? = null,
    targetId: Target_TargetID? = null,
    routerRules: ([]const u8)? = null,
    typedRouterRules: ([]const ServiceWorker_ServiceWorkerRouterRule)? = null,
}
/// ServiceWorker error message.
pub struct ServiceWorker_ServiceWorkerErrorMessage {
    errorMessage: []const u8 = "",
    registrationId: ServiceWorker_RegistrationID = "",
    versionId: []const u8 = "",
    sourceURL: []const u8 = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
}
pub struct ServiceWorker_workerErrorReportedPayload {
    errorMessage: ServiceWorker_ServiceWorkerErrorMessage = .{},
}
pub struct ServiceWorker_workerRegistrationUpdatedPayload {
    registrations: []const ServiceWorker_ServiceWorkerRegistration = &.{},
}
pub struct ServiceWorker_workerVersionUpdatedPayload {
    versions: []const ServiceWorker_ServiceWorkerVersion = &.{},
}
pub struct ServiceWorker_deliverPushMessageParameters {
    origin: []const u8 = "",
    registrationId: ServiceWorker_RegistrationID = "",
    data: []const u8 = "",
}
pub struct ServiceWorker_deliverPushMessageReturnValue {}
pub struct ServiceWorker_disableParameters {}
pub struct ServiceWorker_disableReturnValue {}
pub struct ServiceWorker_dispatchSyncEventParameters {
    origin: []const u8 = "",
    registrationId: ServiceWorker_RegistrationID = "",
    tag: []const u8 = "",
    lastChance: bool = false,
}
pub struct ServiceWorker_dispatchSyncEventReturnValue {}
pub struct ServiceWorker_dispatchPeriodicSyncEventParameters {
    origin: []const u8 = "",
    registrationId: ServiceWorker_RegistrationID = "",
    tag: []const u8 = "",
}
pub struct ServiceWorker_dispatchPeriodicSyncEventReturnValue {}
pub struct ServiceWorker_enableParameters {}
pub struct ServiceWorker_enableReturnValue {}
pub struct ServiceWorker_setForceUpdateOnPageLoadParameters {
    forceUpdateOnPageLoad: bool = false,
}
pub struct ServiceWorker_setForceUpdateOnPageLoadReturnValue {}
pub struct ServiceWorker_skipWaitingParameters {
    scopeURL: []const u8 = "",
}
pub struct ServiceWorker_skipWaitingReturnValue {}
pub struct ServiceWorker_startWorkerParameters {
    scopeURL: []const u8 = "",
}
pub struct ServiceWorker_startWorkerReturnValue {}
pub struct ServiceWorker_stopAllWorkersParameters {}
pub struct ServiceWorker_stopAllWorkersReturnValue {}
pub struct ServiceWorker_stopWorkerParameters {
    versionId: []const u8 = "",
}
pub struct ServiceWorker_stopWorkerReturnValue {}
pub struct ServiceWorker_unregisterParameters {
    scopeURL: []const u8 = "",
}
pub struct ServiceWorker_unregisterReturnValue {}
pub struct ServiceWorker_updateRegistrationParameters {
    scopeURL: []const u8 = "",
}
pub struct ServiceWorker_updateRegistrationReturnValue {}

// ============================================================================
// Domain: SmartCardEmulation
// ============================================================================

/// Indicates the PC/SC error code.
/// enum: "success"|"removed-card"|"reset-card"|"unpowered-card"|"unresponsive-card"|"unsupported-card"|"reader-unavailable"|"s...
pub type SmartCardEmulation_ResultCode = []const u8;
/// Maps to the |SCARD_SHARE_*| values.
/// enum: "shared"|"exclusive"|"direct"
pub type SmartCardEmulation_ShareMode = []const u8;
/// Indicates what the reader should do with the card.
/// enum: "leave-card"|"reset-card"|"unpower-card"|"eject-card"
pub type SmartCardEmulation_Disposition = []const u8;
/// Maps to |SCARD_*| connection state values.
/// enum: "absent"|"present"|"swallowed"|"powered"|"negotiable"|"specific"
pub type SmartCardEmulation_ConnectionState = []const u8;
/// Maps to the |SCARD_STATE_*| flags.
pub struct SmartCardEmulation_ReaderStateFlags {
    unaware: bool? = null,
    ignore: bool? = null,
    changed: bool? = null,
    unknown: bool? = null,
    unavailable: bool? = null,
    empty: bool? = null,
    present: bool? = null,
    exclusive: bool? = null,
    inuse: bool? = null,
    mute: bool? = null,
    unpowered: bool? = null,
}
/// Maps to the |SCARD_PROTOCOL_*| flags.
pub struct SmartCardEmulation_ProtocolSet {
    t0: bool? = null,
    t1: bool? = null,
    raw: bool? = null,
}
/// Maps to the |SCARD_PROTOCOL_*| values.
/// enum: "t0"|"t1"|"raw"
pub type SmartCardEmulation_Protocol = []const u8;
pub struct SmartCardEmulation_ReaderStateIn {
    reader: []const u8 = "",
    currentState: SmartCardEmulation_ReaderStateFlags = .{},
    currentInsertionCount: f64 = 0.0,
}
pub struct SmartCardEmulation_ReaderStateOut {
    reader: []const u8 = "",
    eventState: SmartCardEmulation_ReaderStateFlags = .{},
    eventCount: f64 = 0.0,
    atr: ProtocolBinary = "",
}
/// Fired when |SCardEstablishContext| is called.
pub struct SmartCardEmulation_establishContextRequestedPayload {
    requestId: []const u8 = "",
}
/// Fired when |SCardReleaseContext| is called.
pub struct SmartCardEmulation_releaseContextRequestedPayload {
    requestId: []const u8 = "",
    contextId: f64 = 0.0,
}
/// Fired when |SCardListReaders| is called.
pub struct SmartCardEmulation_listReadersRequestedPayload {
    requestId: []const u8 = "",
    contextId: f64 = 0.0,
}
/// Fired when |SCardGetStatusChange| is called. Timeout is specified in milliseconds.
pub struct SmartCardEmulation_getStatusChangeRequestedPayload {
    requestId: []const u8 = "",
    contextId: f64 = 0.0,
    readerStates: []const SmartCardEmulation_ReaderStateIn = &.{},
    timeout: f64? = null,
}
/// Fired when |SCardCancel| is called.
pub struct SmartCardEmulation_cancelRequestedPayload {
    requestId: []const u8 = "",
    contextId: f64 = 0.0,
}
/// Fired when |SCardConnect| is called.
pub struct SmartCardEmulation_connectRequestedPayload {
    requestId: []const u8 = "",
    contextId: f64 = 0.0,
    reader: []const u8 = "",
    shareMode: SmartCardEmulation_ShareMode = "",
    preferredProtocols: SmartCardEmulation_ProtocolSet = .{},
}
/// Fired when |SCardDisconnect| is called.
pub struct SmartCardEmulation_disconnectRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
    disposition: SmartCardEmulation_Disposition = "",
}
/// Fired when |SCardTransmit| is called.
pub struct SmartCardEmulation_transmitRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
    data: ProtocolBinary = "",
    protocol: SmartCardEmulation_Protocol? = null,
}
/// Fired when |SCardControl| is called.
pub struct SmartCardEmulation_controlRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
    controlCode: f64 = 0.0,
    data: ProtocolBinary = "",
}
/// Fired when |SCardGetAttrib| is called.
pub struct SmartCardEmulation_getAttribRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
    attribId: f64 = 0.0,
}
/// Fired when |SCardSetAttrib| is called.
pub struct SmartCardEmulation_setAttribRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
    attribId: f64 = 0.0,
    data: ProtocolBinary = "",
}
/// Fired when |SCardStatus| is called.
pub struct SmartCardEmulation_statusRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
}
/// Fired when |SCardBeginTransaction| is called.
pub struct SmartCardEmulation_beginTransactionRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
}
/// Fired when |SCardEndTransaction| is called.
pub struct SmartCardEmulation_endTransactionRequestedPayload {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
    disposition: SmartCardEmulation_Disposition = "",
}
/// Enables the |SmartCardEmulation| domain.
pub struct SmartCardEmulation_enableParameters {}
pub struct SmartCardEmulation_enableReturnValue {}
/// Disables the |SmartCardEmulation| domain.
pub struct SmartCardEmulation_disableParameters {}
pub struct SmartCardEmulation_disableReturnValue {}
/// Reports the successful result of a |SCardEstablishContext| call.
pub struct SmartCardEmulation_reportEstablishContextResultParameters {
    requestId: []const u8 = "",
    contextId: f64 = 0.0,
}
pub struct SmartCardEmulation_reportEstablishContextResultReturnValue {}
/// Reports the successful result of a |SCardReleaseContext| call.
pub struct SmartCardEmulation_reportReleaseContextResultParameters {
    requestId: []const u8 = "",
}
pub struct SmartCardEmulation_reportReleaseContextResultReturnValue {}
/// Reports the successful result of a |SCardListReaders| call.
pub struct SmartCardEmulation_reportListReadersResultParameters {
    requestId: []const u8 = "",
    readers: []const []const u8 = &.{},
}
pub struct SmartCardEmulation_reportListReadersResultReturnValue {}
/// Reports the successful result of a |SCardGetStatusChange| call.
pub struct SmartCardEmulation_reportGetStatusChangeResultParameters {
    requestId: []const u8 = "",
    readerStates: []const SmartCardEmulation_ReaderStateOut = &.{},
}
pub struct SmartCardEmulation_reportGetStatusChangeResultReturnValue {}
/// Reports the result of a |SCardBeginTransaction| call.
pub struct SmartCardEmulation_reportBeginTransactionResultParameters {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
}
pub struct SmartCardEmulation_reportBeginTransactionResultReturnValue {}
/// Reports the successful result of a call that returns only a result code.
pub struct SmartCardEmulation_reportPlainResultParameters {
    requestId: []const u8 = "",
}
pub struct SmartCardEmulation_reportPlainResultReturnValue {}
/// Reports the successful result of a |SCardConnect| call.
pub struct SmartCardEmulation_reportConnectResultParameters {
    requestId: []const u8 = "",
    handle: f64 = 0.0,
    activeProtocol: SmartCardEmulation_Protocol? = null,
}
pub struct SmartCardEmulation_reportConnectResultReturnValue {}
/// Reports the successful result of a call that sends back data on success.
pub struct SmartCardEmulation_reportDataResultParameters {
    requestId: []const u8 = "",
    data: ProtocolBinary = "",
}
pub struct SmartCardEmulation_reportDataResultReturnValue {}
/// Reports the successful result of a |SCardStatus| call.
pub struct SmartCardEmulation_reportStatusResultParameters {
    requestId: []const u8 = "",
    readerName: []const u8 = "",
    state: SmartCardEmulation_ConnectionState = "",
    atr: ProtocolBinary = "",
    protocol: SmartCardEmulation_Protocol? = null,
}
pub struct SmartCardEmulation_reportStatusResultReturnValue {}
/// Reports an error result for the given request.
pub struct SmartCardEmulation_reportErrorParameters {
    requestId: []const u8 = "",
    resultCode: SmartCardEmulation_ResultCode = "",
}
pub struct SmartCardEmulation_reportErrorReturnValue {}

// ============================================================================
// Domain: Storage
// ============================================================================

pub type Storage_SerializedStorageKey = []const u8;
/// Enum of possible storage types.
/// enum: "cookies"|"file_systems"|"indexeddb"|"local_storage"|"shader_cache"|"websql"|"service_workers"|"cache_storage"|"stora...
pub type Storage_StorageType = []const u8;
/// Usage for a storage type.
pub struct Storage_UsageForType {
    storageType: Storage_StorageType = "",
    usage: f64 = 0.0,
}
/// Pair of issuer origin and number of available (signed, but not used) Trust
pub struct Storage_TrustTokens {
    issuerOrigin: []const u8 = "",
    count: f64 = 0.0,
}
/// Details of a stored Private Verification Token.
pub struct Storage_PrivateVerificationToken {
    id: []const u8 = "",
    issuerOrigin: []const u8 = "",
    keyId: f64 = 0.0,
    expiration: Network_TimeSinceEpoch = 0.0,
    creationTime: Network_TimeSinceEpoch = 0.0,
    version: f64 = 0.0,
    token: []const u8 = "",
}
/// Configuration for a Private Verification Tokens issuer.
pub struct Storage_PrivateVerificationTokensIssuerConfig {
    issuerOrigin: []const u8 = "",
    redeemerOrigins: []const []const u8 = &.{},
}
/// enum: "relaxed"|"strict"
pub type Storage_StorageBucketsDurability = []const u8;
pub struct Storage_StorageBucket {
    storageKey: Storage_SerializedStorageKey = "",
    name: ([]const u8)? = null,
}
pub struct Storage_StorageBucketInfo {
    bucket: Storage_StorageBucket = .{},
    id: []const u8 = "",
    expiration: Network_TimeSinceEpoch = 0.0,
    quota: f64 = 0.0,
    persistent: bool = false,
    durability: Storage_StorageBucketsDurability = "",
}
/// A cache's contents have been modified.
pub struct Storage_cacheStorageContentUpdatedPayload {
    origin: []const u8 = "",
    storageKey: []const u8 = "",
    bucketId: []const u8 = "",
    cacheName: []const u8 = "",
}
/// A cache has been added/deleted.
pub struct Storage_cacheStorageListUpdatedPayload {
    origin: []const u8 = "",
    storageKey: []const u8 = "",
    bucketId: []const u8 = "",
}
/// The origin's IndexedDB object store has been modified.
pub struct Storage_indexedDBContentUpdatedPayload {
    origin: []const u8 = "",
    storageKey: []const u8 = "",
    bucketId: []const u8 = "",
    databaseName: []const u8 = "",
    objectStoreName: []const u8 = "",
}
/// The origin's IndexedDB database list has been modified.
pub struct Storage_indexedDBListUpdatedPayload {
    origin: []const u8 = "",
    storageKey: []const u8 = "",
    bucketId: []const u8 = "",
}
pub struct Storage_storageBucketCreatedOrUpdatedPayload {
    bucketInfo: Storage_StorageBucketInfo = .{},
}
pub struct Storage_storageBucketDeletedPayload {
    bucketId: []const u8 = "",
}
/// Private Verification Tokens have been stored or deleted.
pub type Storage_privateVerificationTokensUpdatedPayload = ProtocolJsonValue;
/// Returns a storage key given a frame id.
pub struct Storage_getStorageKeyForFrameParameters {
    frameId: Page_FrameId = "",
}
pub struct Storage_getStorageKeyForFrameReturnValue {
    storageKey: Storage_SerializedStorageKey = "",
}
/// Returns storage key for the given frame. If no frame ID is provided,
pub struct Storage_getStorageKeyParameters {
    frameId: Page_FrameId? = null,
}
pub struct Storage_getStorageKeyReturnValue {
    storageKey: Storage_SerializedStorageKey = "",
}
/// Clears storage for origin.
pub struct Storage_clearDataForOriginParameters {
    origin: []const u8 = "",
    storageTypes: []const u8 = "",
}
pub struct Storage_clearDataForOriginReturnValue {}
/// Clears storage for storage key.
pub struct Storage_clearDataForStorageKeyParameters {
    storageKey: []const u8 = "",
    storageTypes: []const u8 = "",
}
pub struct Storage_clearDataForStorageKeyReturnValue {}
/// Returns all browser cookies.
pub struct Storage_getCookiesParameters {
    browserContextId: Browser_BrowserContextID? = null,
}
pub struct Storage_getCookiesReturnValue {
    cookies: []const Network_Cookie = &.{},
}
/// Sets given cookies.
pub struct Storage_setCookiesParameters {
    cookies: []const Network_CookieParam = &.{},
    browserContextId: Browser_BrowserContextID? = null,
}
pub struct Storage_setCookiesReturnValue {}
/// Clears cookies.
pub struct Storage_clearCookiesParameters {
    browserContextId: Browser_BrowserContextID? = null,
}
pub struct Storage_clearCookiesReturnValue {}
/// Returns usage and quota in bytes.
pub struct Storage_getUsageAndQuotaParameters {
    origin: []const u8 = "",
}
pub struct Storage_getUsageAndQuotaReturnValue {
    usage: f64 = 0.0,
    quota: f64 = 0.0,
    overrideActive: bool = false,
    usageBreakdown: []const Storage_UsageForType = &.{},
}
/// Override quota for the specified origin
pub struct Storage_overrideQuotaForOriginParameters {
    origin: []const u8 = "",
    quotaSize: f64? = null,
}
pub struct Storage_overrideQuotaForOriginReturnValue {}
/// Registers origin to be notified when an update occurs to its cache storage list.
pub struct Storage_trackCacheStorageForOriginParameters {
    origin: []const u8 = "",
}
pub struct Storage_trackCacheStorageForOriginReturnValue {}
/// Registers storage key to be notified when an update occurs to its cache storage list.
pub struct Storage_trackCacheStorageForStorageKeyParameters {
    storageKey: []const u8 = "",
}
pub struct Storage_trackCacheStorageForStorageKeyReturnValue {}
/// Registers origin to be notified when an update occurs to its IndexedDB.
pub struct Storage_trackIndexedDBForOriginParameters {
    origin: []const u8 = "",
}
pub struct Storage_trackIndexedDBForOriginReturnValue {}
/// Registers storage key to be notified when an update occurs to its IndexedDB.
pub struct Storage_trackIndexedDBForStorageKeyParameters {
    storageKey: []const u8 = "",
}
pub struct Storage_trackIndexedDBForStorageKeyReturnValue {}
/// Unregisters origin from receiving notifications for cache storage.
pub struct Storage_untrackCacheStorageForOriginParameters {
    origin: []const u8 = "",
}
pub struct Storage_untrackCacheStorageForOriginReturnValue {}
/// Unregisters storage key from receiving notifications for cache storage.
pub struct Storage_untrackCacheStorageForStorageKeyParameters {
    storageKey: []const u8 = "",
}
pub struct Storage_untrackCacheStorageForStorageKeyReturnValue {}
/// Unregisters origin from receiving notifications for IndexedDB.
pub struct Storage_untrackIndexedDBForOriginParameters {
    origin: []const u8 = "",
}
pub struct Storage_untrackIndexedDBForOriginReturnValue {}
/// Unregisters storage key from receiving notifications for IndexedDB.
pub struct Storage_untrackIndexedDBForStorageKeyParameters {
    storageKey: []const u8 = "",
}
pub struct Storage_untrackIndexedDBForStorageKeyReturnValue {}
/// Returns the number of stored Trust Tokens per issuer for the
pub struct Storage_getTrustTokensParameters {}
pub struct Storage_getTrustTokensReturnValue {
    tokens: []const Storage_TrustTokens = &.{},
}
/// Removes all Trust Tokens issued by the provided issuerOrigin.
pub struct Storage_clearTrustTokensParameters {
    issuerOrigin: []const u8 = "",
}
pub struct Storage_clearTrustTokensReturnValue {
    didDeleteTokens: bool = false,
}
/// Returns all stored Private Verification Tokens for the current browsing
pub struct Storage_getPrivateVerificationTokensParameters {}
pub struct Storage_getPrivateVerificationTokensReturnValue {
    tokens: []const Storage_PrivateVerificationToken = &.{},
}
/// Returns the configured Private Verification Tokens issuers and their redeemer
pub struct Storage_getPrivateVerificationTokensIssuerConfigsParameters {}
pub struct Storage_getPrivateVerificationTokensIssuerConfigsReturnValue {
    configs: []const Storage_PrivateVerificationTokensIssuerConfig = &.{},
}
/// Removes all Private Verification Tokens issued by the provided issuerOrigin.
pub struct Storage_clearPrivateVerificationTokensParameters {
    issuerOrigin: []const u8 = "",
}
pub struct Storage_clearPrivateVerificationTokensReturnValue {}
/// Removes a specific Private Verification Token by its ID.
pub struct Storage_deletePrivateVerificationTokenParameters {
    tokenId: []const u8 = "",
}
pub struct Storage_deletePrivateVerificationTokenReturnValue {}
/// Set tracking for Private Verification Tokens.
pub struct Storage_setPrivateVerificationTokensTrackingParameters {
    enable: bool = false,
}
pub struct Storage_setPrivateVerificationTokensTrackingReturnValue {}
/// Set tracking for a storage key's buckets.
pub struct Storage_setStorageBucketTrackingParameters {
    storageKey: []const u8 = "",
    enable: bool = false,
}
pub struct Storage_setStorageBucketTrackingReturnValue {}
/// Deletes the Storage Bucket with the given storage key and bucket name.
pub struct Storage_deleteStorageBucketParameters {
    bucket: Storage_StorageBucket = .{},
}
pub struct Storage_deleteStorageBucketReturnValue {}
/// Deletes state for sites identified as potential bounce trackers, immediately.
pub struct Storage_runBounceTrackingMitigationsParameters {}
pub struct Storage_runBounceTrackingMitigationsReturnValue {
    deletedSites: []const []const u8 = &.{},
}

// ============================================================================
// Domain: SystemInfo
// The SystemInfo domain defines methods and events for querying low-level system information.
// ============================================================================

/// Describes a single graphics processor (GPU).
pub struct SystemInfo_GPUDevice {
    vendorId: f64 = 0.0,
    deviceId: f64 = 0.0,
    subSysId: f64? = null,
    revision: f64? = null,
    vendorString: []const u8 = "",
    deviceString: []const u8 = "",
    driverVendor: []const u8 = "",
    driverVersion: []const u8 = "",
}
/// Describes the width and height dimensions of an entity.
pub struct SystemInfo_Size {
    width: f64 = 0.0,
    height: f64 = 0.0,
}
/// Describes a supported video decoding profile with its associated minimum and
pub struct SystemInfo_VideoDecodeAcceleratorCapability {
    profile: []const u8 = "",
    maxResolution: SystemInfo_Size = .{},
    minResolution: SystemInfo_Size = .{},
}
/// Describes a supported video encoding profile with its associated maximum
pub struct SystemInfo_VideoEncodeAcceleratorCapability {
    profile: []const u8 = "",
    maxResolution: SystemInfo_Size = .{},
    maxFramerateNumerator: f64 = 0.0,
    maxFramerateDenominator: f64 = 0.0,
}
/// YUV subsampling type of the pixels of a given image.
/// enum: "yuv420"|"yuv422"|"yuv444"
pub type SystemInfo_SubsamplingFormat = []const u8;
/// Image format of a given image.
/// enum: "jpeg"|"webp"|"unknown"
pub type SystemInfo_ImageType = []const u8;
/// Provides information about the GPU(s) on the system.
pub struct SystemInfo_GPUInfo {
    devices: []const SystemInfo_GPUDevice = &.{},
    auxAttributes: ProtocolStringMap? = null,
    featureStatus: ProtocolStringMap? = null,
    driverBugWorkarounds: []const []const u8 = &.{},
    videoDecoding: []const SystemInfo_VideoDecodeAcceleratorCapability = &.{},
    videoEncoding: []const SystemInfo_VideoEncodeAcceleratorCapability = &.{},
}
/// Represents process info.
pub struct SystemInfo_ProcessInfo {
    type_: []const u8 = "", // protocol field: "type"
    id: f64 = 0.0,
    cpuTime: f64 = 0.0,
}
/// Returns information about the system.
pub struct SystemInfo_getInfoParameters {}
pub struct SystemInfo_getInfoReturnValue {
    gpu: SystemInfo_GPUInfo = .{},
    modelName: []const u8 = "",
    modelVersion: []const u8 = "",
    commandLine: []const u8 = "",
}
/// Returns information about the feature state.
pub struct SystemInfo_getFeatureStateParameters {
    featureState: []const u8 = "",
}
pub struct SystemInfo_getFeatureStateReturnValue {
    featureEnabled: bool = false,
}
/// Returns information about all running processes.
pub struct SystemInfo_getProcessInfoParameters {}
pub struct SystemInfo_getProcessInfoReturnValue {
    processInfo: []const SystemInfo_ProcessInfo = &.{},
}
