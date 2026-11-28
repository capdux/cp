// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue } from "./common";
import {
    Network_AdAncestry,
    Network_AdProvenance,
    Network_ClientSecurityState,
    Network_CorsErrorStatus,
    Network_IPAddressSpace,
    Network_LoaderId,
    Network_RequestId,
    Network_TimeSinceEpoch,
} from "./domain_06_network";
import { DOM_BackendNodeId, DOM_NodeId, DOM_ScrollOrientation } from "./domains_03_cast_to_domsnapshot";
import { Page_FrameId } from "./domains_08_page_to_performance";
import { ServiceWorker_RegistrationID } from "./domains_09_performancetimeline_to_systeminfo";
import { Runtime_RemoteObject, Runtime_RemoteObjectId, Runtime_ScriptId, Runtime_StackTrace } from "./domains_11_debugger_to_schema";

// ============================================================================
// Domain: Accessibility
// ============================================================================

/// Unique accessibility node identifier.
pub type Accessibility_AXNodeId = []const u8;
/// Enum of possible property types.
/// enum: "boolean"|"tristate"|"booleanOrUndefined"|"idref"|"idrefList"|"integer"|"node"|"nodeList"|"number"|"string"|"computed...
pub type Accessibility_AXValueType = []const u8;
/// Enum of possible property sources.
/// enum: "attribute"|"implicit"|"style"|"contents"|"placeholder"|"relatedElement"
pub type Accessibility_AXValueSourceType = []const u8;
/// Enum of possible native property sources (as a subtype of a particular AXValueSourceType).
/// enum: "description"|"figcaption"|"label"|"labelfor"|"labelwrapped"|"legend"|"rubyannotation"|"tablecaption"|"title"|"other"
pub type Accessibility_AXValueNativeSourceType = []const u8;
/// A single source for a computed AX property.
pub struct Accessibility_AXValueSource {
    type_: Accessibility_AXValueSourceType = "", // protocol field: "type"
    value: Accessibility_AXValue? = null,
    attribute: ([]const u8)? = null,
    attributeValue: Accessibility_AXValue? = null,
    superseded: bool? = null,
    nativeSource: Accessibility_AXValueNativeSourceType? = null,
    nativeSourceValue: Accessibility_AXValue? = null,
    invalid: bool? = null,
    invalidReason: ([]const u8)? = null,
}
pub struct Accessibility_AXRelatedNode {
    backendDOMNodeId: DOM_BackendNodeId = 0.0,
    idref: ([]const u8)? = null,
    text: ([]const u8)? = null,
}
pub struct Accessibility_AXProperty {
    name: Accessibility_AXPropertyName = "",
    value: Accessibility_AXValue = .{},
}
/// A single computed AX property.
pub struct Accessibility_AXValue {
    type_: Accessibility_AXValueType = "", // protocol field: "type"
    value: ProtocolJsonValue? = null,
    relatedNodes: ([]const Accessibility_AXRelatedNode)? = null,
    sources: ([]const Accessibility_AXValueSource)? = null,
}
/// Values of AXProperty name:
/// enum: "actions"|"busy"|"disabled"|"editable"|"focusable"|"focused"|"hidden"|"hiddenRoot"|"invalid"|"keyshortcuts"|"settable...
pub type Accessibility_AXPropertyName = []const u8;
/// A node in the accessibility tree.
pub struct Accessibility_AXNode {
    nodeId: Accessibility_AXNodeId = "",
    ignored: bool = false,
    ignoredReasons: ([]const Accessibility_AXProperty)? = null,
    role: Accessibility_AXValue? = null,
    chromeRole: Accessibility_AXValue? = null,
    name: Accessibility_AXValue? = null,
    description: Accessibility_AXValue? = null,
    value: Accessibility_AXValue? = null,
    properties: ([]const Accessibility_AXProperty)? = null,
    parentId: Accessibility_AXNodeId? = null,
    childIds: ([]const Accessibility_AXNodeId)? = null,
    backendDOMNodeId: DOM_BackendNodeId? = null,
    frameId: Page_FrameId? = null,
}
/// The loadComplete event mirrors the load complete event sent by the browser to assistive
pub struct Accessibility_loadCompletePayload {
    root: Accessibility_AXNode = .{},
}
/// The nodesUpdated event is sent every time a previously requested node has changed the in tree.
pub struct Accessibility_nodesUpdatedPayload {
    nodes: []const Accessibility_AXNode = &.{},
}
/// Disables the accessibility domain.
pub struct Accessibility_disableParameters {}
pub struct Accessibility_disableReturnValue {}
/// Enables the accessibility domain which causes `AXNodeId`s to remain consistent between method calls.
pub struct Accessibility_enableParameters {}
pub struct Accessibility_enableReturnValue {}
/// Fetches the accessibility node and partial accessibility tree for this DOM node, if it exists.
pub struct Accessibility_getPartialAXTreeParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    fetchRelatives: bool? = null,
}
pub struct Accessibility_getPartialAXTreeReturnValue {
    nodes: []const Accessibility_AXNode = &.{},
}
/// Fetches the entire accessibility tree for the root Document
pub struct Accessibility_getFullAXTreeParameters {
    depth: f64? = null,
    frameId: Page_FrameId? = null,
}
pub struct Accessibility_getFullAXTreeReturnValue {
    nodes: []const Accessibility_AXNode = &.{},
}
/// Fetches the root node.
pub struct Accessibility_getRootAXNodeParameters {
    frameId: Page_FrameId? = null,
}
pub struct Accessibility_getRootAXNodeReturnValue {
    node: Accessibility_AXNode = .{},
}
/// Fetches a node and all ancestors up to and including the root.
pub struct Accessibility_getAXNodeAndAncestorsParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
pub struct Accessibility_getAXNodeAndAncestorsReturnValue {
    nodes: []const Accessibility_AXNode = &.{},
}
/// Fetches a particular accessibility node by AXNodeId.
pub struct Accessibility_getChildAXNodesParameters {
    id: Accessibility_AXNodeId = "",
    frameId: Page_FrameId? = null,
}
pub struct Accessibility_getChildAXNodesReturnValue {
    nodes: []const Accessibility_AXNode = &.{},
}
/// Query a DOM node's accessibility subtree for accessible name and role.
pub struct Accessibility_queryAXTreeParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    accessibleName: ([]const u8)? = null,
    role: ([]const u8)? = null,
}
pub struct Accessibility_queryAXTreeReturnValue {
    nodes: []const Accessibility_AXNode = &.{},
}

// ============================================================================
// Domain: Ads
// A domain for ad-related metrics and data.
// ============================================================================

/// Ad frame data.
pub struct Ads_AdFrameData {
    frameId: Page_FrameId = "",
    initialOrigin: ([]const u8)? = null,
    networkBytes: f64 = 0.0,
    cpuTime: f64 = 0.0,
}
/// Ad metrics for a page.
pub struct Ads_AdMetrics {
    viewportAdDensityByArea: f64 = 0.0,
    averageViewportAdDensityByArea: f64 = 0.0,
    viewportAdCount: f64 = 0.0,
    averageViewportAdCount: f64 = 0.0,
    totalAdCpuTime: f64 = 0.0,
    totalAdNetworkBytes: f64 = 0.0,
    updateAdFrames: []const Ads_AdFrameData = &.{},
    removeAdFrames: []const Page_FrameId = &.{},
}
/// An ad script.
pub struct Ads_AdScript {
    scriptId: Runtime_ScriptId = "",
    provenance: Network_AdProvenance = .{},
}
/// Retrieves ad metrics for the current page.
pub struct Ads_getAdMetricsParameters {}
pub struct Ads_getAdMetricsReturnValue {
    metrics: Ads_AdMetrics = .{},
}
/// Retrieves ad scripts for the current page. To minimize payload size, this
pub struct Ads_getAdScriptsParameters {}
pub struct Ads_getAdScriptsReturnValue {
    newScripts: []const Ads_AdScript = &.{},
}

// ============================================================================
// Domain: Animation
// ============================================================================

/// Animation instance.
pub struct Animation_Animation {
    id: []const u8 = "",
    name: []const u8 = "",
    pausedState: bool = false,
    playState: []const u8 = "",
    playbackRate: f64 = 0.0,
    startTime: f64 = 0.0,
    currentTime: f64 = 0.0,
    type_: []const u8 = "", // protocol field: "type"; enum: "CSSTransition"|"CSSAnimation"|"WebAnimation"
    source: Animation_AnimationEffect? = null,
    cssId: ([]const u8)? = null,
    viewOrScrollTimeline: Animation_ViewOrScrollTimeline? = null,
}
/// Timeline instance
pub struct Animation_ViewOrScrollTimeline {
    sourceNodeId: DOM_BackendNodeId? = null,
    startOffset: f64? = null,
    endOffset: f64? = null,
    subjectNodeId: DOM_BackendNodeId? = null,
    axis: DOM_ScrollOrientation = "",
}
/// AnimationEffect instance
pub struct Animation_AnimationEffect {
    delay: f64 = 0.0,
    endDelay: f64 = 0.0,
    iterationStart: f64 = 0.0,
    iterations: f64? = null,
    duration: f64 = 0.0,
    direction: []const u8 = "",
    fill: []const u8 = "",
    backendNodeId: DOM_BackendNodeId? = null,
    keyframesRule: Animation_KeyframesRule? = null,
    easing: []const u8 = "",
}
/// Keyframes Rule
pub struct Animation_KeyframesRule {
    name: ([]const u8)? = null,
    keyframes: []const Animation_KeyframeStyle = &.{},
}
/// Keyframe Style
pub struct Animation_KeyframeStyle {
    offset: []const u8 = "",
    easing: []const u8 = "",
}
/// Event for when an animation has been cancelled.
pub struct Animation_animationCanceledPayload {
    id: []const u8 = "",
}
/// Event for each animation that has been created.
pub struct Animation_animationCreatedPayload {
    id: []const u8 = "",
}
/// Event for animation that has been started.
pub struct Animation_animationStartedPayload {
    animation: Animation_Animation = .{},
}
/// Event for animation that has been updated.
pub struct Animation_animationUpdatedPayload {
    animation: Animation_Animation = .{},
}
/// Disables animation domain notifications.
pub struct Animation_disableParameters {}
pub struct Animation_disableReturnValue {}
/// Enables animation domain notifications.
pub struct Animation_enableParameters {}
pub struct Animation_enableReturnValue {}
/// Returns the current time of the an animation.
pub struct Animation_getCurrentTimeParameters {
    id: []const u8 = "",
}
pub struct Animation_getCurrentTimeReturnValue {
    currentTime: f64 = 0.0,
}
/// Gets the playback rate of the document timeline.
pub struct Animation_getPlaybackRateParameters {}
pub struct Animation_getPlaybackRateReturnValue {
    playbackRate: f64 = 0.0,
}
/// Releases a set of animations to no longer be manipulated.
pub struct Animation_releaseAnimationsParameters {
    animations: []const []const u8 = &.{},
}
pub struct Animation_releaseAnimationsReturnValue {}
/// Gets the remote object of the Animation.
pub struct Animation_resolveAnimationParameters {
    animationId: []const u8 = "",
}
pub struct Animation_resolveAnimationReturnValue {
    remoteObject: Runtime_RemoteObject = .{},
}
/// Seek a set of animations to a particular time within each animation.
pub struct Animation_seekAnimationsParameters {
    animations: []const []const u8 = &.{},
    currentTime: f64? = null,
    currentTimes: ([]const f64)? = null,
}
pub struct Animation_seekAnimationsReturnValue {}
/// Sets the paused state of a set of animations.
pub struct Animation_setPausedParameters {
    animations: []const []const u8 = &.{},
    paused: bool = false,
}
pub struct Animation_setPausedReturnValue {}
/// Sets the playback rate of the document timeline.
pub struct Animation_setPlaybackRateParameters {
    playbackRate: f64 = 0.0,
}
pub struct Animation_setPlaybackRateReturnValue {}
/// Sets the timing of an animation node.
pub struct Animation_setTimingParameters {
    animationId: []const u8 = "",
    duration: f64 = 0.0,
    delay: f64 = 0.0,
}
pub struct Animation_setTimingReturnValue {}

// ============================================================================
// Domain: Audits
// Audits domain allows investigation of page violations and possible improvements.
// ============================================================================

/// Information about a cookie that is affected by an inspector issue.
pub struct Audits_AffectedCookie {
    name: []const u8 = "",
    path: []const u8 = "",
    domain: []const u8 = "",
}
/// Information about a request that is affected by an inspector issue.
pub struct Audits_AffectedRequest {
    requestId: Network_RequestId? = null,
    url: []const u8 = "",
}
/// Information about the frame affected by an inspector issue.
pub struct Audits_AffectedFrame {
    frameId: Page_FrameId = "",
}
/// enum: "ExcludeSameSiteUnspecifiedTreatedAsLax"|"ExcludeSameSiteNoneInsecure"|"ExcludeSameSiteLax"|"ExcludeSameSiteStrict"|"...
pub type Audits_CookieExclusionReason = []const u8;
/// enum: "WarnSameSiteUnspecifiedCrossSiteContext"|"WarnSameSiteNoneInsecure"|"WarnSameSiteUnspecifiedLaxAllowUnsafe"|"WarnSam...
pub type Audits_CookieWarningReason = []const u8;
/// enum: "SetCookie"|"ReadCookie"
pub type Audits_CookieOperation = []const u8;
/// Represents the category of insight that a cookie issue falls under.
/// enum: "GitHubResource"|"GracePeriod"|"Heuristics"
pub type Audits_InsightType = []const u8;
/// Information about the suggested solution to a cookie issue.
pub struct Audits_CookieIssueInsight {
    type_: Audits_InsightType = "", // protocol field: "type"
    tableEntryUrl: ([]const u8)? = null,
}
/// This information is currently necessary, as the front-end has a difficult
pub struct Audits_CookieIssueDetails {
    cookie: Audits_AffectedCookie? = null,
    rawCookieLine: ([]const u8)? = null,
    cookieWarningReasons: []const Audits_CookieWarningReason = &.{},
    cookieExclusionReasons: []const Audits_CookieExclusionReason = &.{},
    operation: Audits_CookieOperation = "",
    siteForCookies: ([]const u8)? = null,
    cookieUrl: ([]const u8)? = null,
    request: Audits_AffectedRequest? = null,
    insight: Audits_CookieIssueInsight? = null,
}
/// enum: "DocumentCookie"
pub type Audits_PerformanceIssueType = []const u8;
/// Details for a performance issue.
pub struct Audits_PerformanceIssueDetails {
    performanceIssueType: Audits_PerformanceIssueType = "",
    sourceCodeLocation: Audits_SourceCodeLocation? = null,
}
/// enum: "MixedContentBlocked"|"MixedContentAutomaticallyUpgraded"|"MixedContentWarning"
pub type Audits_MixedContentResolutionStatus = []const u8;
/// enum: "Audio"|"Beacon"|"CSPReport"|"Download"|"EventSource"|"Favicon"|"Font"|"Form"|"Frame"|"Image"|"Import"|"JSON"|"Manife...
pub type Audits_MixedContentResourceType = []const u8;
pub struct Audits_MixedContentIssueDetails {
    resourceType: Audits_MixedContentResourceType? = null,
    resolutionStatus: Audits_MixedContentResolutionStatus = "",
    insecureURL: []const u8 = "",
    mainResourceURL: []const u8 = "",
    request: Audits_AffectedRequest? = null,
    frame: Audits_AffectedFrame? = null,
}
/// Enum indicating the reason a response has been blocked. These reasons are
/// enum: "CoepFrameResourceNeedsCoepHeader"|"CoopSandboxedIFrameCannotNavigateToCoopPage"|"CorpNotSameOrigin"|"CorpNotSameOrig...
pub type Audits_BlockedByResponseReason = []const u8;
/// Details for a request that has been blocked with the BLOCKED_BY_RESPONSE
pub struct Audits_BlockedByResponseIssueDetails {
    request: Audits_AffectedRequest = .{},
    parentFrame: Audits_AffectedFrame? = null,
    blockedFrame: Audits_AffectedFrame? = null,
    reason: Audits_BlockedByResponseReason = "",
}
/// enum: "HeavyAdBlocked"|"HeavyAdWarning"
pub type Audits_HeavyAdResolutionStatus = []const u8;
/// enum: "NetworkTotalLimit"|"CpuTotalLimit"|"CpuPeakLimit"
pub type Audits_HeavyAdReason = []const u8;
pub struct Audits_HeavyAdIssueDetails {
    resolution: Audits_HeavyAdResolutionStatus = "",
    reason: Audits_HeavyAdReason = "",
    frame: Audits_AffectedFrame = .{},
}
/// enum: "kInlineViolation"|"kEvalViolation"|"kURLViolation"|"kSRIViolation"|"kTrustedTypesSinkViolation"|"kTrustedTypesPolicy...
pub type Audits_ContentSecurityPolicyViolationType = []const u8;
pub struct Audits_SourceCodeLocation {
    scriptId: Runtime_ScriptId? = null,
    url: []const u8 = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
}
pub struct Audits_ContentSecurityPolicyIssueDetails {
    blockedURL: ([]const u8)? = null,
    violatedDirective: []const u8 = "",
    isReportOnly: bool = false,
    contentSecurityPolicyViolationType: Audits_ContentSecurityPolicyViolationType = "",
    frameAncestor: Audits_AffectedFrame? = null,
    sourceCodeLocation: Audits_SourceCodeLocation? = null,
    violatingNodeId: DOM_BackendNodeId? = null,
}
/// enum: "TransferIssue"|"CreationIssue"
pub type Audits_SharedArrayBufferIssueType = []const u8;
/// Details for a issue arising from an SAB being instantiated in, or
pub struct Audits_SharedArrayBufferIssueDetails {
    sourceCodeLocation: Audits_SourceCodeLocation = .{},
    isWarning: bool = false,
    type_: Audits_SharedArrayBufferIssueType = "", // protocol field: "type"
}
/// Details for a CORS related issue, e.g. a warning or error related to
pub struct Audits_CorsIssueDetails {
    corsErrorStatus: Network_CorsErrorStatus = .{},
    isWarning: bool = false,
    request: Audits_AffectedRequest = .{},
    location: Audits_SourceCodeLocation? = null,
    initiatorOrigin: ([]const u8)? = null,
    resourceIPAddressSpace: Network_IPAddressSpace? = null,
    clientSecurityState: Network_ClientSecurityState? = null,
}
/// enum: "UseErrorCrossOriginNoCorsRequest"|"UseErrorDictionaryLoadFailure"|"UseErrorMatchingDictionaryNotUsed"|"UseErrorUnexp...
pub type Audits_SharedDictionaryError = []const u8;
/// enum: "MissingSignatureHeader"|"MissingSignatureInputHeader"|"InvalidSignatureHeader"|"InvalidSignatureInputHeader"|"Signat...
pub type Audits_SRIMessageSignatureError = []const u8;
/// enum: "MalformedDictionary"|"UnknownAlgorithm"|"IncorrectDigestType"|"IncorrectDigestLength"
pub type Audits_UnencodedDigestError = []const u8;
/// enum: "InvalidHeader"|"MoreThanOneList"|"ItemNotInnerList"|"InvalidAllowlistItemType"|"ReportingEndpointNotToken"|"InvalidU...
pub type Audits_ConnectionAllowlistError = []const u8;
/// Details for issues about documents in Quirks Mode
pub struct Audits_QuirksModeIssueDetails {
    isLimitedQuirksMode: bool = false,
    documentNodeId: DOM_BackendNodeId = 0.0,
    url: []const u8 = "",
    frameId: Page_FrameId = "",
    loaderId: Network_LoaderId = "",
}
pub struct Audits_NavigatorUserAgentIssueDetails {
    url: []const u8 = "",
    location: Audits_SourceCodeLocation? = null,
}
pub struct Audits_SharedDictionaryIssueDetails {
    sharedDictionaryError: Audits_SharedDictionaryError = "",
    request: Audits_AffectedRequest = .{},
}
pub struct Audits_SRIMessageSignatureIssueDetails {
    error_: Audits_SRIMessageSignatureError = "", // protocol field: "error"
    signatureBase: []const u8 = "",
    integrityAssertions: []const []const u8 = &.{},
    request: Audits_AffectedRequest = .{},
}
pub struct Audits_UnencodedDigestIssueDetails {
    error_: Audits_UnencodedDigestError = "", // protocol field: "error"
    request: Audits_AffectedRequest = .{},
}
pub struct Audits_ConnectionAllowlistIssueDetails {
    error_: Audits_ConnectionAllowlistError = "", // protocol field: "error"
    request: Audits_AffectedRequest = .{},
}
/// enum: "FormLabelForNameError"|"FormDuplicateIdForInputError"|"FormInputWithNoLabelError"|"FormAutocompleteAttributeEmptyErr...
pub type Audits_GenericIssueErrorType = []const u8;
/// Depending on the concrete errorType, different properties are set.
pub struct Audits_GenericIssueDetails {
    errorType: Audits_GenericIssueErrorType = "",
    frameId: Page_FrameId? = null,
    violatingNodeId: DOM_BackendNodeId? = null,
    violatingNodeAttribute: ([]const u8)? = null,
    request: Audits_AffectedRequest? = null,
}
/// This issue tracks information needed to print a deprecation message.
pub struct Audits_DeprecationIssueDetails {
    affectedFrame: Audits_AffectedFrame? = null,
    sourceCodeLocation: Audits_SourceCodeLocation = .{},
    type_: []const u8 = "", // protocol field: "type"
}
/// This issue warns about sites in the redirect chain of a finished navigation
pub struct Audits_BounceTrackingIssueDetails {
    trackingSites: []const []const u8 = &.{},
}
/// This issue warns about third-party sites that are accessing cookies on the
pub struct Audits_CookieDeprecationMetadataIssueDetails {
    allowedSites: []const []const u8 = &.{},
    optOutPercentage: f64 = 0.0,
    isOptOutTopLevel: bool = false,
    operation: Audits_CookieOperation = "",
}
/// enum: "MetaTagAllowListInvalidOrigin"|"MetaTagModifiedHTML"
pub type Audits_ClientHintIssueReason = []const u8;
pub struct Audits_FederatedAuthRequestIssueDetails {
    federatedAuthRequestIssueReason: Audits_FederatedAuthRequestIssueReason = "",
}
/// Represents the failure reason when a federated authentication reason fails.
/// enum: "ShouldEmbargo"|"TooManyRequests"|"WellKnownHttpNotFound"|"WellKnownNoResponse"|"WellKnownBlockedByConnectionAllowlis...
pub type Audits_FederatedAuthRequestIssueReason = []const u8;
pub struct Audits_FederatedAuthUserInfoRequestIssueDetails {
    federatedAuthUserInfoRequestIssueReason: Audits_FederatedAuthUserInfoRequestIssueReason = "",
}
/// Represents the failure reason when a getUserInfo() call fails.
/// enum: "NotSameOrigin"|"NotIframe"|"NotPotentiallyTrustworthy"|"NoApiPermission"|"NotSignedInWithIdp"|"NoAccountSharingPermi...
pub type Audits_FederatedAuthUserInfoRequestIssueReason = []const u8;
pub struct Audits_EmailVerificationRequestIssueDetails {
    emailVerificationRequestIssueReason: Audits_EmailVerificationRequestIssueReason = "",
}
/// Represents the failure reason when an email verification request fails.
/// enum: "InvalidEmail"|"DnsFetchFailed"|"DnsInvalidRecord"|"WellKnownHttpNotFound"|"WellKnownNoResponse"|"WellKnownInvalidRes...
pub type Audits_EmailVerificationRequestIssueReason = []const u8;
/// This issue tracks client hints related issues. It's used to deprecate old
pub struct Audits_ClientHintIssueDetails {
    sourceCodeLocation: Audits_SourceCodeLocation = .{},
    clientHintIssueReason: Audits_ClientHintIssueReason = "",
}
pub struct Audits_FailedRequestInfo {
    url: []const u8 = "",
    failureMessage: []const u8 = "",
    requestId: Network_RequestId? = null,
}
/// enum: "BlockedCrossPartitionFetching"|"EnforceNoopenerForNavigation"
pub type Audits_PartitioningBlobURLInfo = []const u8;
pub struct Audits_PartitioningBlobURLIssueDetails {
    url: []const u8 = "",
    partitioningBlobURLInfo: Audits_PartitioningBlobURLInfo = "",
}
/// enum: "DisallowedSelectChild"|"DisallowedOptGroupChild"|"NonPhrasingContentOptionChild"|"InteractiveContentOptionChild"|"In...
pub type Audits_ElementAccessibilityIssueReason = []const u8;
/// This issue warns about errors in the select or summary element content model.
pub struct Audits_ElementAccessibilityIssueDetails {
    nodeId: DOM_BackendNodeId = 0.0,
    elementAccessibilityIssueReason: Audits_ElementAccessibilityIssueReason = "",
    hasDisallowedAttributes: bool = false,
}
/// enum: "LateImportRule"|"RequestFailed"
pub type Audits_StyleSheetLoadingIssueReason = []const u8;
/// This issue warns when a referenced stylesheet couldn't be loaded.
pub struct Audits_StylesheetLoadingIssueDetails {
    sourceCodeLocation: Audits_SourceCodeLocation = .{},
    styleSheetLoadingIssueReason: Audits_StyleSheetLoadingIssueReason = "",
    failedRequestInfo: Audits_FailedRequestInfo? = null,
}
/// enum: "InvalidSyntax"|"InvalidInitialValue"|"InvalidInherits"|"InvalidName"
pub type Audits_PropertyRuleIssueReason = []const u8;
/// This issue warns about errors in property rules that lead to property
pub struct Audits_PropertyRuleIssueDetails {
    sourceCodeLocation: Audits_SourceCodeLocation = .{},
    propertyRuleIssueReason: Audits_PropertyRuleIssueReason = "",
    propertyValue: ([]const u8)? = null,
}
/// enum: "BlockedFrameNavigation"|"BlockedSubresource"|"NoisedCanvasReadback"
pub type Audits_UserReidentificationIssueType = []const u8;
/// This issue warns about uses of APIs that may be considered misuse to
pub struct Audits_UserReidentificationIssueDetails {
    type_: Audits_UserReidentificationIssueType = "", // protocol field: "type"
    request: Audits_AffectedRequest? = null,
    sourceCodeLocation: Audits_SourceCodeLocation? = null,
}
/// enum: "InvalidType"|"FencedFrameDisallowed"|"CspFrameAncestorsMissing"|"PermissionsPolicyBlocked"|"PaddingRightUnsupported"...
pub type Audits_PermissionElementIssueType = []const u8;
/// This issue warns about improper usage of the <permission> element.
pub struct Audits_PermissionElementIssueDetails {
    issueType: Audits_PermissionElementIssueType = "",
    type_: ([]const u8)? = null, // protocol field: "type"
    nodeId: DOM_BackendNodeId? = null,
    isWarning: bool? = null,
    permissionName: ([]const u8)? = null,
    occluderNodeInfo: ([]const u8)? = null,
    occluderParentNodeInfo: ([]const u8)? = null,
    disableReason: ([]const u8)? = null,
}
/// enum: "ManifestParsingOrNetworkError"|"StartUrlInvalid"|"ManifestMissingNameOrShortName"|"ManifestMissingId"|"NoManifest"
pub type Audits_WebInstallIssueReason = []const u8;
/// This issue reports a failure involving a web app manifest used by a Web
pub struct Audits_WebInstallIssueDetails {
    manifestUrl: ([]const u8)? = null,
    reason: Audits_WebInstallIssueReason = "",
}
/// The issue warns about blocked calls to privacy sensitive APIs via the
pub struct Audits_SelectivePermissionsInterventionIssueDetails {
    apiName: []const u8 = "",
    adAncestry: Network_AdAncestry = .{},
    stackTrace: Runtime_StackTrace? = null,
}
/// Details for issues about lazy-loaded images without explicit dimensions.
pub struct Audits_LazyLoadImageIssueDetails {
    nodeId: DOM_BackendNodeId = 0.0,
    url: []const u8 = "",
    frameId: Page_FrameId = "",
}
/// A unique identifier for the type of issue. Each type may use one of the
/// enum: "CookieIssue"|"MixedContentIssue"|"BlockedByResponseIssue"|"HeavyAdIssue"|"ContentSecurityPolicyIssue"|"SharedArrayBu...
pub type Audits_InspectorIssueCode = []const u8;
/// This struct holds a list of optional fields with additional information
pub struct Audits_InspectorIssueDetails {
    cookieIssueDetails: Audits_CookieIssueDetails? = null,
    mixedContentIssueDetails: Audits_MixedContentIssueDetails? = null,
    blockedByResponseIssueDetails: Audits_BlockedByResponseIssueDetails? = null,
    heavyAdIssueDetails: Audits_HeavyAdIssueDetails? = null,
    contentSecurityPolicyIssueDetails: Audits_ContentSecurityPolicyIssueDetails? = null,
    sharedArrayBufferIssueDetails: Audits_SharedArrayBufferIssueDetails? = null,
    corsIssueDetails: Audits_CorsIssueDetails? = null,
    quirksModeIssueDetails: Audits_QuirksModeIssueDetails? = null,
    partitioningBlobURLIssueDetails: Audits_PartitioningBlobURLIssueDetails? = null,
    navigatorUserAgentIssueDetails: Audits_NavigatorUserAgentIssueDetails? = null,
    genericIssueDetails: Audits_GenericIssueDetails? = null,
    deprecationIssueDetails: Audits_DeprecationIssueDetails? = null,
    clientHintIssueDetails: Audits_ClientHintIssueDetails? = null,
    federatedAuthRequestIssueDetails: Audits_FederatedAuthRequestIssueDetails? = null,
    bounceTrackingIssueDetails: Audits_BounceTrackingIssueDetails? = null,
    cookieDeprecationMetadataIssueDetails: Audits_CookieDeprecationMetadataIssueDetails? = null,
    stylesheetLoadingIssueDetails: Audits_StylesheetLoadingIssueDetails? = null,
    propertyRuleIssueDetails: Audits_PropertyRuleIssueDetails? = null,
    federatedAuthUserInfoRequestIssueDetails: Audits_FederatedAuthUserInfoRequestIssueDetails? = null,
    sharedDictionaryIssueDetails: Audits_SharedDictionaryIssueDetails? = null,
    elementAccessibilityIssueDetails: Audits_ElementAccessibilityIssueDetails? = null,
    sriMessageSignatureIssueDetails: Audits_SRIMessageSignatureIssueDetails? = null,
    unencodedDigestIssueDetails: Audits_UnencodedDigestIssueDetails? = null,
    connectionAllowlistIssueDetails: Audits_ConnectionAllowlistIssueDetails? = null,
    userReidentificationIssueDetails: Audits_UserReidentificationIssueDetails? = null,
    permissionElementIssueDetails: Audits_PermissionElementIssueDetails? = null,
    performanceIssueDetails: Audits_PerformanceIssueDetails? = null,
    selectivePermissionsInterventionIssueDetails: Audits_SelectivePermissionsInterventionIssueDetails? = null,
    emailVerificationRequestIssueDetails: Audits_EmailVerificationRequestIssueDetails? = null,
    lazyLoadImageIssueDetails: Audits_LazyLoadImageIssueDetails? = null,
    webInstallIssueDetails: Audits_WebInstallIssueDetails? = null,
}
/// A unique id for a DevTools inspector issue. Allows other entities (e.g.
pub type Audits_IssueId = []const u8;
/// An inspector issue reported from the back-end.
pub struct Audits_InspectorIssue {
    code: Audits_InspectorIssueCode = "",
    details: Audits_InspectorIssueDetails = .{},
    issueId: Audits_IssueId? = null,
}
pub struct Audits_issueAddedPayload {
    issue: Audits_InspectorIssue = .{},
}
/// Returns the response body and size if it were re-encoded with the specified settings. Only
pub struct Audits_getEncodedResponseParameters {
    requestId: Network_RequestId = "",
    encoding: []const u8 = "", // enum: "webp"|"jpeg"|"png"
    quality: f64? = null,
    sizeOnly: bool? = null,
}
pub struct Audits_getEncodedResponseReturnValue {
    body: ProtocolBinary? = null,
    originalSize: f64 = 0.0,
    encodedSize: f64 = 0.0,
}
/// Disables issues domain, prevents further issues from being reported to the client.
pub struct Audits_disableParameters {}
pub struct Audits_disableReturnValue {}
/// Enables issues domain, sends the issues collected so far to the client by means of the
pub struct Audits_enableParameters {}
pub struct Audits_enableReturnValue {}
/// Runs the form issues check for the target page. Found issues are reported
pub struct Audits_checkFormsIssuesParameters {}
pub struct Audits_checkFormsIssuesReturnValue {
    formIssues: []const Audits_GenericIssueDetails = &.{},
}

// ============================================================================
// Domain: Autofill
// Defines commands and events for Autofill.
// ============================================================================

pub struct Autofill_CreditCard {
    number: []const u8 = "",
    name: []const u8 = "",
    expiryMonth: []const u8 = "",
    expiryYear: []const u8 = "",
    cvc: []const u8 = "",
}
pub struct Autofill_AddressField {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// A list of address fields.
pub struct Autofill_AddressFields {
    fields: []const Autofill_AddressField = &.{},
}
pub struct Autofill_Address {
    fields: []const Autofill_AddressField = &.{},
}
/// Defines how an address can be displayed like in chrome://settings/addresses.
pub struct Autofill_AddressUI {
    addressFields: []const Autofill_AddressFields = &.{},
}
/// Specified whether a filled field was done so by using the html autocomplete attribute or autofill heuristics.
/// enum: "autocompleteAttribute"|"autofillInferred"
pub type Autofill_FillingStrategy = []const u8;
pub struct Autofill_FilledField {
    htmlType: []const u8 = "",
    id: []const u8 = "",
    name: []const u8 = "",
    value: []const u8 = "",
    autofillType: []const u8 = "",
    fillingStrategy: Autofill_FillingStrategy = "",
    frameId: Page_FrameId = "",
    fieldId: DOM_BackendNodeId = 0.0,
}
/// Emitted when an address form is filled.
pub struct Autofill_addressFormFilledPayload {
    filledFields: []const Autofill_FilledField = &.{},
    addressUi: Autofill_AddressUI = .{},
}
/// Trigger autofill on a form identified by the fieldId.
pub struct Autofill_triggerParameters {
    fieldId: DOM_BackendNodeId = 0.0,
    frameId: Page_FrameId? = null,
    card: Autofill_CreditCard? = null,
    address: Autofill_Address? = null,
}
pub struct Autofill_triggerReturnValue {}
/// Set addresses so that developers can verify their forms implementation.
pub struct Autofill_setAddressesParameters {
    addresses: []const Autofill_Address = &.{},
}
pub struct Autofill_setAddressesReturnValue {}
/// Disables autofill domain notifications.
pub struct Autofill_disableParameters {}
pub struct Autofill_disableReturnValue {}
/// Enables autofill domain notifications.
pub struct Autofill_enableParameters {}
pub struct Autofill_enableReturnValue {}

// ============================================================================
// Domain: BackgroundService
// Defines events for background web platform features.
// ============================================================================

/// The Background Service that will be associated with the commands/events.
/// enum: "backgroundFetch"|"backgroundSync"|"pushMessaging"|"notifications"|"paymentHandler"|"periodicBackgroundSync"
pub type BackgroundService_ServiceName = []const u8;
/// A key-value pair for additional event information to pass along.
pub struct BackgroundService_EventMetadata {
    key: []const u8 = "",
    value: []const u8 = "",
}
pub struct BackgroundService_BackgroundServiceEvent {
    timestamp: Network_TimeSinceEpoch = 0.0,
    origin: []const u8 = "",
    serviceWorkerRegistrationId: ServiceWorker_RegistrationID = "",
    service: BackgroundService_ServiceName = "",
    eventName: []const u8 = "",
    instanceId: []const u8 = "",
    eventMetadata: []const BackgroundService_EventMetadata = &.{},
    storageKey: []const u8 = "",
}
/// Called when the recording state for the service has been updated.
pub struct BackgroundService_recordingStateChangedPayload {
    isRecording: bool = false,
    service: BackgroundService_ServiceName = "",
}
/// Called with all existing backgroundServiceEvents when enabled, and all new
pub struct BackgroundService_backgroundServiceEventReceivedPayload {
    backgroundServiceEvent: BackgroundService_BackgroundServiceEvent = .{},
}
/// Enables event updates for the service.
pub struct BackgroundService_startObservingParameters {
    service: BackgroundService_ServiceName = "",
}
pub struct BackgroundService_startObservingReturnValue {}
/// Disables event updates for the service.
pub struct BackgroundService_stopObservingParameters {
    service: BackgroundService_ServiceName = "",
}
pub struct BackgroundService_stopObservingReturnValue {}
/// Set the recording state for the service.
pub struct BackgroundService_setRecordingParameters {
    shouldRecord: bool = false,
    service: BackgroundService_ServiceName = "",
}
pub struct BackgroundService_setRecordingReturnValue {}
/// Clears all stored data for the service.
pub struct BackgroundService_clearEventsParameters {
    service: BackgroundService_ServiceName = "",
}
pub struct BackgroundService_clearEventsReturnValue {}

// ============================================================================
// Domain: BluetoothEmulation
// This domain allows configuring virtual Bluetooth devices to test
// the web-bluetooth API.
// ============================================================================

/// Indicates the various states of Central.
/// enum: "absent"|"powered-off"|"powered-on"
pub type BluetoothEmulation_CentralState = []const u8;
/// Indicates the various types of GATT event.
/// enum: "connection"|"discovery"
pub type BluetoothEmulation_GATTOperationType = []const u8;
/// Indicates the various types of characteristic write.
/// enum: "write-default-deprecated"|"write-with-response"|"write-without-response"
pub type BluetoothEmulation_CharacteristicWriteType = []const u8;
/// Indicates the various types of characteristic operation.
/// enum: "read"|"write"|"subscribe-to-notifications"|"unsubscribe-from-notifications"
pub type BluetoothEmulation_CharacteristicOperationType = []const u8;
/// Indicates the various types of descriptor operation.
/// enum: "read"|"write"
pub type BluetoothEmulation_DescriptorOperationType = []const u8;
/// Stores the manufacturer data
pub struct BluetoothEmulation_ManufacturerData {
    key: f64 = 0.0,
    data: ProtocolBinary = "",
}
/// Stores the byte data of the advertisement packet sent by a Bluetooth device.
pub struct BluetoothEmulation_ScanRecord {
    name: ([]const u8)? = null,
    uuids: ([]const []const u8)? = null,
    appearance: f64? = null,
    txPower: f64? = null,
    manufacturerData: ([]const BluetoothEmulation_ManufacturerData)? = null,
}
/// Stores the advertisement packet information that is sent by a Bluetooth device.
pub struct BluetoothEmulation_ScanEntry {
    deviceAddress: []const u8 = "",
    rssi: f64 = 0.0,
    scanRecord: BluetoothEmulation_ScanRecord = .{},
}
/// Describes the properties of a characteristic. This follows Bluetooth Core
pub struct BluetoothEmulation_CharacteristicProperties {
    broadcast: bool? = null,
    read: bool? = null,
    writeWithoutResponse: bool? = null,
    write: bool? = null,
    notify: bool? = null,
    indicate: bool? = null,
    authenticatedSignedWrites: bool? = null,
    extendedProperties: bool? = null,
}
/// Event for when a GATT operation of |type| to the peripheral with |address|
pub struct BluetoothEmulation_gattOperationReceivedPayload {
    address: []const u8 = "",
    type_: BluetoothEmulation_GATTOperationType = "", // protocol field: "type"
}
/// Event for when a characteristic operation of |type| to the characteristic
pub struct BluetoothEmulation_characteristicOperationReceivedPayload {
    characteristicId: []const u8 = "",
    type_: BluetoothEmulation_CharacteristicOperationType = "", // protocol field: "type"
    data: ProtocolBinary? = null,
    writeType: BluetoothEmulation_CharacteristicWriteType? = null,
}
/// Event for when a descriptor operation of |type| to the descriptor
pub struct BluetoothEmulation_descriptorOperationReceivedPayload {
    descriptorId: []const u8 = "",
    type_: BluetoothEmulation_DescriptorOperationType = "", // protocol field: "type"
    data: ProtocolBinary? = null,
}
/// Enable the BluetoothEmulation domain.
pub struct BluetoothEmulation_enableParameters {
    state: BluetoothEmulation_CentralState = "",
    leSupported: bool = false,
}
pub struct BluetoothEmulation_enableReturnValue {}
/// Set the state of the simulated central.
pub struct BluetoothEmulation_setSimulatedCentralStateParameters {
    state: BluetoothEmulation_CentralState = "",
}
pub struct BluetoothEmulation_setSimulatedCentralStateReturnValue {}
/// Disable the BluetoothEmulation domain.
pub struct BluetoothEmulation_disableParameters {}
pub struct BluetoothEmulation_disableReturnValue {}
/// Simulates a peripheral with |address|, |name| and |knownServiceUuids|
pub struct BluetoothEmulation_simulatePreconnectedPeripheralParameters {
    address: []const u8 = "",
    name: []const u8 = "",
    manufacturerData: []const BluetoothEmulation_ManufacturerData = &.{},
    knownServiceUuids: []const []const u8 = &.{},
}
pub struct BluetoothEmulation_simulatePreconnectedPeripheralReturnValue {}
/// Simulates an advertisement packet described in |entry| being received by
pub struct BluetoothEmulation_simulateAdvertisementParameters {
    entry: BluetoothEmulation_ScanEntry = .{},
}
pub struct BluetoothEmulation_simulateAdvertisementReturnValue {}
/// Simulates the response code from the peripheral with |address| for a
pub struct BluetoothEmulation_simulateGATTOperationResponseParameters {
    address: []const u8 = "",
    type_: BluetoothEmulation_GATTOperationType = "", // protocol field: "type"
    code: f64 = 0.0,
}
pub struct BluetoothEmulation_simulateGATTOperationResponseReturnValue {}
/// Simulates the response from the characteristic with |characteristicId| for a
pub struct BluetoothEmulation_simulateCharacteristicOperationResponseParameters {
    characteristicId: []const u8 = "",
    type_: BluetoothEmulation_CharacteristicOperationType = "", // protocol field: "type"
    code: f64 = 0.0,
    data: ProtocolBinary? = null,
}
pub struct BluetoothEmulation_simulateCharacteristicOperationResponseReturnValue {}
/// Simulates the response from the descriptor with |descriptorId| for a
pub struct BluetoothEmulation_simulateDescriptorOperationResponseParameters {
    descriptorId: []const u8 = "",
    type_: BluetoothEmulation_DescriptorOperationType = "", // protocol field: "type"
    code: f64 = 0.0,
    data: ProtocolBinary? = null,
}
pub struct BluetoothEmulation_simulateDescriptorOperationResponseReturnValue {}
/// Adds a service with |serviceUuid| to the peripheral with |address|.
pub struct BluetoothEmulation_addServiceParameters {
    address: []const u8 = "",
    serviceUuid: []const u8 = "",
}
pub struct BluetoothEmulation_addServiceReturnValue {
    serviceId: []const u8 = "",
}
/// Removes the service respresented by |serviceId| from the simulated central.
pub struct BluetoothEmulation_removeServiceParameters {
    serviceId: []const u8 = "",
}
pub struct BluetoothEmulation_removeServiceReturnValue {}
/// Adds a characteristic with |characteristicUuid| and |properties| to the
pub struct BluetoothEmulation_addCharacteristicParameters {
    serviceId: []const u8 = "",
    characteristicUuid: []const u8 = "",
    properties: BluetoothEmulation_CharacteristicProperties = .{},
}
pub struct BluetoothEmulation_addCharacteristicReturnValue {
    characteristicId: []const u8 = "",
}
/// Removes the characteristic respresented by |characteristicId| from the
pub struct BluetoothEmulation_removeCharacteristicParameters {
    characteristicId: []const u8 = "",
}
pub struct BluetoothEmulation_removeCharacteristicReturnValue {}
/// Adds a descriptor with |descriptorUuid| to the characteristic respresented
pub struct BluetoothEmulation_addDescriptorParameters {
    characteristicId: []const u8 = "",
    descriptorUuid: []const u8 = "",
}
pub struct BluetoothEmulation_addDescriptorReturnValue {
    descriptorId: []const u8 = "",
}
/// Removes the descriptor with |descriptorId| from the simulated central.
pub struct BluetoothEmulation_removeDescriptorParameters {
    descriptorId: []const u8 = "",
}
pub struct BluetoothEmulation_removeDescriptorReturnValue {}
/// Simulates a GATT disconnection from the peripheral with |address|.
pub struct BluetoothEmulation_simulateGATTDisconnectionParameters {
    address: []const u8 = "",
}
pub struct BluetoothEmulation_simulateGATTDisconnectionReturnValue {}
