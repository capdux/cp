// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue } from "./common";
import {
    Network_AdAncestry,
    Network_LoaderId,
    Network_MonotonicTime,
    Network_ResourceType,
    Network_TimeSinceEpoch,
} from "./domain_06_network";
import { DOM_BackendNodeId, DOM_Rect } from "./domains_03_cast_to_domsnapshot";
import { Emulation_ScreenOrientation } from "./domains_04_domstorage_to_headlessexperimental";
import { IO_StreamHandle } from "./domains_05_io_to_memory";
import { Debugger_SearchMatch, Runtime_ExecutionContextId, Runtime_StackTrace } from "./domains_11_debugger_to_schema";

// ============================================================================
// Domain: Page
// Actions and events related to the inspected page belong to the page domain.
// ============================================================================

/// Unique frame identifier.
pub type Page_FrameId = []const u8;
/// Indicates whether a frame has been identified as an ad.
/// enum: "none"|"child"|"root"
pub type Page_AdFrameType = []const u8;
/// enum: "ParentIsAd"|"CreatedByAdScript"|"MatchedBlockingRule"
pub type Page_AdFrameExplanation = []const u8;
/// Indicates whether a frame has been identified as an ad and why.
pub struct Page_AdFrameStatus {
    adFrameType: Page_AdFrameType = "",
    explanations: ([]const Page_AdFrameExplanation)? = null,
}
/// Indicates whether the frame is a secure context and why it is the case.
/// enum: "Secure"|"SecureLocalhost"|"InsecureScheme"|"InsecureAncestor"
pub type Page_SecureContextType = []const u8;
/// Indicates whether the frame is cross-origin isolated and why it is the case.
/// enum: "Isolated"|"NotIsolated"|"NotIsolatedFeatureDisabled"
pub type Page_CrossOriginIsolatedContextType = []const u8;
/// enum: "SharedArrayBuffers"|"SharedArrayBuffersTransferAllowed"|"PerformanceMeasureMemory"|"PerformanceProfile"
pub type Page_GatedAPIFeatures = []const u8;
/// All Permissions Policy features. This enum should match the one defined
/// enum: "accelerometer"|"all-screens-capture"|"ambient-light-sensor"|"aria-notify"|"autofill"|"autoplay"|"bluetooth"|"camera"...
pub type Page_PermissionsPolicyFeature = []const u8;
/// Reason for a permissions policy feature to be disabled.
/// enum: "Header"|"IframeAttribute"|"InFencedFrameTree"|"InIsolatedApp"
pub type Page_PermissionsPolicyBlockReason = []const u8;
pub struct Page_PermissionsPolicyBlockLocator {
    frameId: Page_FrameId = "",
    blockReason: Page_PermissionsPolicyBlockReason = "",
}
pub struct Page_PermissionsPolicyFeatureState {
    feature: Page_PermissionsPolicyFeature = "",
    allowed: bool = false,
    locator: Page_PermissionsPolicyBlockLocator? = null,
}
/// Origin Trial(https://www.chromium.org/blink/origin-trials) support.
/// enum: "Success"|"NotSupported"|"Insecure"|"Expired"|"WrongOrigin"|"InvalidSignature"|"Malformed"|"WrongVersion"|"FeatureDis...
pub type Page_OriginTrialTokenStatus = []const u8;
/// Status for an Origin Trial.
/// enum: "Enabled"|"ValidTokenNotProvided"|"OSNotSupported"|"TrialNotAllowed"
pub type Page_OriginTrialStatus = []const u8;
/// enum: "None"|"Subset"
pub type Page_OriginTrialUsageRestriction = []const u8;
pub struct Page_OriginTrialToken {
    origin: []const u8 = "",
    matchSubDomains: bool = false,
    trialName: []const u8 = "",
    expiryTime: Network_TimeSinceEpoch = 0.0,
    isThirdParty: bool = false,
    usageRestriction: Page_OriginTrialUsageRestriction = "",
}
pub struct Page_OriginTrialTokenWithStatus {
    rawTokenText: []const u8 = "",
    parsedToken: Page_OriginTrialToken? = null,
    status: Page_OriginTrialTokenStatus = "",
}
pub struct Page_OriginTrial {
    trialName: []const u8 = "",
    status: Page_OriginTrialStatus = "",
    tokensWithStatus: []const Page_OriginTrialTokenWithStatus = &.{},
}
/// Additional information about the frame document's security origin.
pub struct Page_SecurityOriginDetails {
    isLocalhost: bool = false,
}
/// Information about the Frame on the page.
pub struct Page_Frame {
    id: Page_FrameId = "",
    parentId: Page_FrameId? = null,
    loaderId: Network_LoaderId = "",
    name: ([]const u8)? = null,
    url: []const u8 = "",
    urlFragment: ([]const u8)? = null,
    domainAndRegistry: []const u8 = "",
    securityOrigin: []const u8 = "",
    securityOriginDetails: Page_SecurityOriginDetails? = null,
    mimeType: []const u8 = "",
    unreachableUrl: ([]const u8)? = null,
    adFrameStatus: Page_AdFrameStatus? = null,
    secureContextType: Page_SecureContextType = "",
    crossOriginIsolatedContextType: Page_CrossOriginIsolatedContextType = "",
    gatedAPIFeatures: []const Page_GatedAPIFeatures = &.{},
}
/// Information about the Resource on the page.
pub struct Page_FrameResource {
    url: []const u8 = "",
    type_: Network_ResourceType = "", // protocol field: "type"
    mimeType: []const u8 = "",
    lastModified: Network_TimeSinceEpoch? = null,
    contentSize: f64? = null,
    failed: bool? = null,
    canceled: bool? = null,
}
/// Information about the Frame hierarchy along with their cached resources.
pub struct Page_FrameResourceTree {
    frame: Page_Frame = .{},
    childFrames: ([]const Page_FrameResourceTree)? = null,
    resources: []const Page_FrameResource = &.{},
}
/// Information about the Frame hierarchy.
pub struct Page_FrameTree {
    frame: Page_Frame = .{},
    childFrames: ([]const Page_FrameTree)? = null,
}
/// Unique script identifier.
pub type Page_ScriptIdentifier = []const u8;
/// Transition type.
/// enum: "link"|"typed"|"address_bar"|"auto_bookmark"|"auto_subframe"|"manual_subframe"|"generated"|"auto_toplevel"|"form_subm...
pub type Page_TransitionType = []const u8;
/// Navigation history entry.
pub struct Page_NavigationEntry {
    id: f64 = 0.0,
    url: []const u8 = "",
    userTypedURL: []const u8 = "",
    title: []const u8 = "",
    transitionType: Page_TransitionType = "",
}
/// Screencast frame metadata.
pub struct Page_ScreencastFrameMetadata {
    offsetTop: f64 = 0.0,
    pageScaleFactor: f64 = 0.0,
    deviceWidth: f64 = 0.0,
    deviceHeight: f64 = 0.0,
    scrollOffsetX: f64 = 0.0,
    scrollOffsetY: f64 = 0.0,
    timestamp: Network_TimeSinceEpoch? = null,
    monotonicTimestamp: Network_MonotonicTime? = null,
}
/// Javascript dialog type.
/// enum: "alert"|"confirm"|"prompt"|"beforeunload"
pub type Page_DialogType = []const u8;
/// Error while paring app manifest.
pub struct Page_AppManifestError {
    message: []const u8 = "",
    critical: f64 = 0.0,
    line: f64 = 0.0,
    column: f64 = 0.0,
}
/// Parsed app manifest properties.
pub struct Page_AppManifestParsedProperties {
    scope: []const u8 = "",
}
/// Layout viewport position and dimensions.
pub struct Page_LayoutViewport {
    pageX: f64 = 0.0,
    pageY: f64 = 0.0,
    clientWidth: f64 = 0.0,
    clientHeight: f64 = 0.0,
}
/// Visual viewport position, dimensions, and scale.
pub struct Page_VisualViewport {
    offsetX: f64 = 0.0,
    offsetY: f64 = 0.0,
    pageX: f64 = 0.0,
    pageY: f64 = 0.0,
    clientWidth: f64 = 0.0,
    clientHeight: f64 = 0.0,
    scale: f64 = 0.0,
    zoom: f64? = null,
}
/// Viewport for capturing screenshot.
pub struct Page_Viewport {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    scale: f64 = 0.0,
}
/// Generic font families collection.
pub struct Page_FontFamilies {
    standard: ([]const u8)? = null,
    fixed: ([]const u8)? = null,
    serif: ([]const u8)? = null,
    sansSerif: ([]const u8)? = null,
    cursive: ([]const u8)? = null,
    fantasy: ([]const u8)? = null,
    math: ([]const u8)? = null,
}
/// Font families collection for a script.
pub struct Page_ScriptFontFamilies {
    script: []const u8 = "",
    fontFamilies: Page_FontFamilies = .{},
}
/// Default font sizes.
pub struct Page_FontSizes {
    standard: f64? = null,
    fixed: f64? = null,
}
/// enum: "anchorClick"|"formSubmissionGet"|"formSubmissionPost"|"httpHeaderRefresh"|"initialFrameNavigation"|"metaTagRefresh"|...
pub type Page_ClientNavigationReason = []const u8;
/// enum: "currentTab"|"newTab"|"newWindow"|"download"
pub type Page_ClientNavigationDisposition = []const u8;
pub struct Page_InstallabilityErrorArgument {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// The installability error
pub struct Page_InstallabilityError {
    errorId: []const u8 = "",
    errorArguments: []const Page_InstallabilityErrorArgument = &.{},
}
/// The referring-policy used for the navigation.
/// enum: "noReferrer"|"noReferrerWhenDowngrade"|"origin"|"originWhenCrossOrigin"|"sameOrigin"|"strictOrigin"|"strictOriginWhen...
pub type Page_ReferrerPolicy = []const u8;
/// Per-script compilation cache parameters for `Page.produceCompilationCache`
pub struct Page_CompilationCacheParams {
    url: []const u8 = "",
    eager: bool? = null,
}
pub struct Page_FileFilter {
    name: ([]const u8)? = null,
    accepts: ([]const []const u8)? = null,
}
pub struct Page_FileHandler {
    action: []const u8 = "",
    name: []const u8 = "",
    accepts: ([]const Page_FileFilter)? = null,
    launchType: []const u8 = "",
}
/// The image definition used in both icon and screenshot.
pub struct Page_ImageResource {
    url: []const u8 = "",
    sizes: ([]const u8)? = null,
    type_: ([]const u8)? = null, // protocol field: "type"
}
pub struct Page_LaunchHandler {
    clientMode: []const u8 = "",
}
pub struct Page_ProtocolHandler {
    protocol: []const u8 = "",
    url: []const u8 = "",
}
pub struct Page_RelatedApplication {
    id: ([]const u8)? = null,
    url: []const u8 = "",
}
pub struct Page_ScopeExtension {
    origin: []const u8 = "",
    hasOriginWildcard: bool = false,
}
pub struct Page_Screenshot {
    image: Page_ImageResource = .{},
    formFactor: []const u8 = "",
    label: ([]const u8)? = null,
}
pub struct Page_ShareTarget {
    action: []const u8 = "",
    method: []const u8 = "",
    enctype: []const u8 = "",
    title: ([]const u8)? = null,
    text: ([]const u8)? = null,
    url: ([]const u8)? = null,
    files: ([]const Page_FileFilter)? = null,
}
pub struct Page_Shortcut {
    name: []const u8 = "",
    url: []const u8 = "",
}
pub struct Page_WebAppManifest {
    backgroundColor: ([]const u8)? = null,
    description: ([]const u8)? = null,
    dir: ([]const u8)? = null,
    display: ([]const u8)? = null,
    displayOverrides: ([]const []const u8)? = null,
    fileHandlers: ([]const Page_FileHandler)? = null,
    icons: ([]const Page_ImageResource)? = null,
    id: ([]const u8)? = null,
    lang: ([]const u8)? = null,
    launchHandler: Page_LaunchHandler? = null,
    name: ([]const u8)? = null,
    orientation: ([]const u8)? = null,
    preferRelatedApplications: bool? = null,
    protocolHandlers: ([]const Page_ProtocolHandler)? = null,
    relatedApplications: ([]const Page_RelatedApplication)? = null,
    scope: ([]const u8)? = null,
    scopeExtensions: ([]const Page_ScopeExtension)? = null,
    screenshots: ([]const Page_Screenshot)? = null,
    shareTarget: Page_ShareTarget? = null,
    shortName: ([]const u8)? = null,
    shortcuts: ([]const Page_Shortcut)? = null,
    startUrl: ([]const u8)? = null,
    themeColor: ([]const u8)? = null,
}
pub struct Page_SubApp {
    name: []const u8 = "",
    scope: []const u8 = "",
    manifestId: []const u8 = "",
    startUrl: []const u8 = "",
}
/// The type of a frameNavigated event.
/// enum: "Navigation"|"BackForwardCacheRestore"
pub type Page_NavigationType = []const u8;
/// List of not restored reasons for back-forward cache.
/// enum: "NotPrimaryMainFrame"|"BackForwardCacheDisabled"|"RelatedActiveContentsExist"|"HTTPStatusNotOK"|"SchemeNotHTTPOrHTTPS...
pub type Page_BackForwardCacheNotRestoredReason = []const u8;
/// Types of not restored reasons for back-forward cache.
/// enum: "SupportPending"|"PageSupportNeeded"|"Circumstantial"
pub type Page_BackForwardCacheNotRestoredReasonType = []const u8;
pub struct Page_BackForwardCacheBlockingDetails {
    url: ([]const u8)? = null,
    function: ([]const u8)? = null,
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
}
pub struct Page_BackForwardCacheNotRestoredExplanation {
    type_: Page_BackForwardCacheNotRestoredReasonType = "", // protocol field: "type"
    reason: Page_BackForwardCacheNotRestoredReason = "",
    context: ([]const u8)? = null,
    details: ([]const Page_BackForwardCacheBlockingDetails)? = null,
}
pub struct Page_BackForwardCacheNotRestoredExplanationTree {
    url: []const u8 = "",
    explanations: []const Page_BackForwardCacheNotRestoredExplanation = &.{},
    children: []const Page_BackForwardCacheNotRestoredExplanationTree = &.{},
}
pub struct Page_domContentEventFiredPayload {
    timestamp: Network_MonotonicTime = 0.0,
}
/// Emitted only when `page.interceptFileChooser` is enabled.
pub struct Page_fileChooserOpenedPayload {
    frameId: Page_FrameId = "",
    mode: []const u8 = "", // enum: "selectSingle"|"selectMultiple"
    backendNodeId: DOM_BackendNodeId? = null,
}
/// Fired when frame has been attached to its parent.
pub struct Page_frameAttachedPayload {
    frameId: Page_FrameId = "",
    parentFrameId: Page_FrameId = "",
    stack: Runtime_StackTrace? = null,
}
/// Fired when frame no longer has a scheduled navigation.
pub struct Page_frameClearedScheduledNavigationPayload {
    frameId: Page_FrameId = "",
}
/// Fired when frame has been detached from its parent.
pub struct Page_frameDetachedPayload {
    frameId: Page_FrameId = "",
    reason: []const u8 = "", // enum: "remove"|"swap"
}
/// Fired before frame subtree is detached. Emitted before any frame of the
pub struct Page_frameSubtreeWillBeDetachedPayload {
    frameId: Page_FrameId = "",
}
/// Fired once navigation of the frame has completed. Frame is now associated with the new loader.
pub struct Page_frameNavigatedPayload {
    frame: Page_Frame = .{},
    type_: Page_NavigationType = "", // protocol field: "type"
}
/// Fired when opening document to write to.
pub struct Page_documentOpenedPayload {
    frame: Page_Frame = .{},
}
pub type Page_frameResizedPayload = ProtocolJsonValue;
/// Fired when a navigation starts. This event is fired for both
pub struct Page_frameStartedNavigatingPayload {
    frameId: Page_FrameId = "",
    url: []const u8 = "",
    loaderId: Network_LoaderId = "",
    navigationType: []const u8 = "", // enum: "reload"|"reloadBypassingCache"|"restore"|"restoreWithPost"|"historySameDocument"|"historyDifferentDocument"|"sameDoc...
}
/// Fired when a renderer-initiated navigation is requested.
pub struct Page_frameRequestedNavigationPayload {
    frameId: Page_FrameId = "",
    reason: Page_ClientNavigationReason = "",
    url: []const u8 = "",
    disposition: Page_ClientNavigationDisposition = "",
}
/// Fired when frame schedules a potential navigation.
pub struct Page_frameScheduledNavigationPayload {
    frameId: Page_FrameId = "",
    delay: f64 = 0.0,
    reason: Page_ClientNavigationReason = "",
    url: []const u8 = "",
}
/// Fired when frame has started loading.
pub struct Page_frameStartedLoadingPayload {
    frameId: Page_FrameId = "",
}
/// Fired when frame has stopped loading.
pub struct Page_frameStoppedLoadingPayload {
    frameId: Page_FrameId = "",
}
/// Fired when page is about to start a download.
pub struct Page_downloadWillBeginPayload {
    frameId: Page_FrameId = "",
    guid: []const u8 = "",
    url: []const u8 = "",
    suggestedFilename: []const u8 = "",
}
/// Fired when download makes progress. Last call has |done| == true.
pub struct Page_downloadProgressPayload {
    guid: []const u8 = "",
    totalBytes: f64 = 0.0,
    receivedBytes: f64 = 0.0,
    state: []const u8 = "", // enum: "inProgress"|"completed"|"canceled"
}
/// Fired when interstitial page was hidden
pub type Page_interstitialHiddenPayload = ProtocolJsonValue;
/// Fired when interstitial page was shown
pub type Page_interstitialShownPayload = ProtocolJsonValue;
/// Fired when a JavaScript initiated dialog (alert, confirm, prompt, or onbeforeunload) has been
pub struct Page_javascriptDialogClosedPayload {
    frameId: Page_FrameId = "",
    result: bool = false,
    userInput: []const u8 = "",
}
/// Fired when a JavaScript initiated dialog (alert, confirm, prompt, or onbeforeunload) is about to
pub struct Page_javascriptDialogOpeningPayload {
    url: []const u8 = "",
    frameId: Page_FrameId = "",
    message: []const u8 = "",
    type_: Page_DialogType = "", // protocol field: "type"
    hasBrowserHandler: bool = false,
    defaultPrompt: ([]const u8)? = null,
}
/// Fired for lifecycle events (navigation, load, paint, etc) in the current
pub struct Page_lifecycleEventPayload {
    frameId: Page_FrameId = "",
    loaderId: Network_LoaderId = "",
    name: []const u8 = "",
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired for failed bfcache history navigations if BackForwardCache feature is enabled. Do
pub struct Page_backForwardCacheNotUsedPayload {
    loaderId: Network_LoaderId = "",
    frameId: Page_FrameId = "",
    notRestoredExplanations: []const Page_BackForwardCacheNotRestoredExplanation = &.{},
    notRestoredExplanationsTree: Page_BackForwardCacheNotRestoredExplanationTree? = null,
}
pub struct Page_loadEventFiredPayload {
    timestamp: Network_MonotonicTime = 0.0,
}
/// Fired when same-document navigation happens, e.g. due to history API usage or anchor navigation.
pub struct Page_navigatedWithinDocumentPayload {
    frameId: Page_FrameId = "",
    url: []const u8 = "",
    navigationType: []const u8 = "", // enum: "fragment"|"historyApi"|"other"
}
/// Compressed image data requested by the `startScreencast`.
pub struct Page_screencastFramePayload {
    data: ProtocolBinary = "",
    metadata: Page_ScreencastFrameMetadata = .{},
    sessionId: f64 = 0.0,
}
/// Fired when the page with currently enabled screencast was shown or hidden `.
pub struct Page_screencastVisibilityChangedPayload {
    visible: bool = false,
}
/// Fired when a new window is going to be opened, via window.open(), link click, form submission,
pub struct Page_windowOpenPayload {
    url: []const u8 = "",
    windowName: []const u8 = "",
    windowFeatures: []const []const u8 = &.{},
    userGesture: bool = false,
}
/// Issued for every compilation cache generated.
pub struct Page_compilationCacheProducedPayload {
    url: []const u8 = "",
    data: ProtocolBinary = "",
}
/// Deprecated, please use addScriptToEvaluateOnNewDocument instead.
pub struct Page_addScriptToEvaluateOnLoadParameters {
    scriptSource: []const u8 = "",
}
pub struct Page_addScriptToEvaluateOnLoadReturnValue {
    identifier: Page_ScriptIdentifier = "",
}
/// Evaluates given script in every frame upon creation (before loading frame's scripts).
pub struct Page_addScriptToEvaluateOnNewDocumentParameters {
    source: []const u8 = "",
    worldName: ([]const u8)? = null,
    includeCommandLineAPI: bool? = null,
    runImmediately: bool? = null,
}
pub struct Page_addScriptToEvaluateOnNewDocumentReturnValue {
    identifier: Page_ScriptIdentifier = "",
}
/// Brings page to front (activates tab).
pub struct Page_bringToFrontParameters {}
pub struct Page_bringToFrontReturnValue {}
/// Capture page screenshot.
pub struct Page_captureScreenshotParameters {
    format: ([]const u8)? = null, // enum: "jpeg"|"png"|"webp"
    quality: f64? = null,
    clip: Page_Viewport? = null,
    fromSurface: bool? = null,
    captureBeyondViewport: bool? = null,
    optimizeForSpeed: bool? = null,
}
pub struct Page_captureScreenshotReturnValue {
    data: ProtocolBinary = "",
}
/// Returns a snapshot of the page as a string. For MHTML format, the serialization includes
pub struct Page_captureSnapshotParameters {
    format: ([]const u8)? = null, // enum: "mhtml"
}
pub struct Page_captureSnapshotReturnValue {
    data: []const u8 = "",
}
/// Clears the overridden device metrics.
pub struct Page_clearDeviceMetricsOverrideParameters {}
pub struct Page_clearDeviceMetricsOverrideReturnValue {}
/// Clears the overridden Device Orientation.
pub struct Page_clearDeviceOrientationOverrideParameters {}
pub struct Page_clearDeviceOrientationOverrideReturnValue {}
/// Clears the overridden Geolocation Position and Error.
pub struct Page_clearGeolocationOverrideParameters {}
pub struct Page_clearGeolocationOverrideReturnValue {}
/// Creates an isolated world for the given frame.
pub struct Page_createIsolatedWorldParameters {
    frameId: Page_FrameId = "",
    worldName: ([]const u8)? = null,
    grantUniveralAccess: bool? = null,
    contentSecurityPolicy: ([]const u8)? = null,
}
pub struct Page_createIsolatedWorldReturnValue {
    executionContextId: Runtime_ExecutionContextId = 0.0,
}
/// Deletes browser cookie with given name, domain and path.
pub struct Page_deleteCookieParameters {
    cookieName: []const u8 = "",
    url: []const u8 = "",
}
pub struct Page_deleteCookieReturnValue {}
/// Disables page domain notifications.
pub struct Page_disableParameters {}
pub struct Page_disableReturnValue {}
/// Enables page domain notifications.
pub struct Page_enableParameters {
    enableFileChooserOpenedEvent: bool? = null,
}
pub struct Page_enableReturnValue {}
/// Gets the processed manifest for this current document.
pub struct Page_getAppManifestParameters {
    manifestId: ([]const u8)? = null,
}
pub struct Page_getAppManifestReturnValue {
    url: []const u8 = "",
    errors: []const Page_AppManifestError = &.{},
    data: ([]const u8)? = null,
    parsed: Page_AppManifestParsedProperties? = null,
    manifest: Page_WebAppManifest = .{},
}
pub struct Page_getInstallabilityErrorsParameters {}
pub struct Page_getInstallabilityErrorsReturnValue {
    installabilityErrors: []const Page_InstallabilityError = &.{},
}
/// Deprecated because it's not guaranteed that the returned icon is in fact the one used for PWA installation.
pub struct Page_getManifestIconsParameters {}
pub struct Page_getManifestIconsReturnValue {
    primaryIcon: ProtocolBinary? = null,
}
/// Returns the unique (PWA) app id, along with IWA bundle ID and parent app info.
pub struct Page_getAppIdParameters {}
pub struct Page_getAppIdReturnValue {
    appId: ([]const u8)? = null,
    recommendedId: ([]const u8)? = null,
    bundleId: ([]const u8)? = null,
    parentAppName: ([]const u8)? = null,
}
/// Returns the list of installed child Sub-Apps for the inspected parent app.
pub struct Page_getSubAppsParameters {}
pub struct Page_getSubAppsReturnValue {
    subApps: []const Page_SubApp = &.{},
}
/// Returns the list of sibling Sub-Apps sharing the same parent app if the inspected context is a Sub-App.
pub struct Page_getSiblingSubAppsParameters {}
pub struct Page_getSiblingSubAppsReturnValue {
    subApps: []const Page_SubApp = &.{},
}
pub struct Page_getAdScriptAncestryParameters {
    frameId: Page_FrameId = "",
}
pub struct Page_getAdScriptAncestryReturnValue {
    adScriptAncestry: Network_AdAncestry? = null,
}
/// Returns the words that the frame's document added to its spell check custom
pub struct Page_getSpellCheckCustomDictionaryParameters {
    frameId: Page_FrameId = "",
}
pub struct Page_getSpellCheckCustomDictionaryReturnValue {
    words: []const []const u8 = &.{},
}
/// Returns present frame tree structure.
pub struct Page_getFrameTreeParameters {}
pub struct Page_getFrameTreeReturnValue {
    frameTree: Page_FrameTree = .{},
}
/// Returns metrics relating to the layouting of the page, such as viewport bounds/scale.
pub struct Page_getLayoutMetricsParameters {}
pub struct Page_getLayoutMetricsReturnValue {
    layoutViewport: Page_LayoutViewport = .{},
    visualViewport: Page_VisualViewport = .{},
    contentSize: DOM_Rect = .{},
    cssLayoutViewport: Page_LayoutViewport = .{},
    cssVisualViewport: Page_VisualViewport = .{},
    cssContentSize: DOM_Rect = .{},
}
/// Returns navigation history for the current page.
pub struct Page_getNavigationHistoryParameters {}
pub struct Page_getNavigationHistoryReturnValue {
    currentIndex: f64 = 0.0,
    entries: []const Page_NavigationEntry = &.{},
}
/// Resets navigation history for the current page.
pub struct Page_resetNavigationHistoryParameters {}
pub struct Page_resetNavigationHistoryReturnValue {}
/// Returns content of the given resource.
pub struct Page_getResourceContentParameters {
    frameId: Page_FrameId = "",
    url: []const u8 = "",
}
pub struct Page_getResourceContentReturnValue {
    content: []const u8 = "",
    base64Encoded: bool = false,
}
/// Returns present frame / resource tree structure.
pub struct Page_getResourceTreeParameters {}
pub struct Page_getResourceTreeReturnValue {
    frameTree: Page_FrameResourceTree = .{},
}
/// Accepts or dismisses a JavaScript initiated dialog (alert, confirm, prompt, or onbeforeunload).
pub struct Page_handleJavaScriptDialogParameters {
    accept: bool = false,
    promptText: ([]const u8)? = null,
}
pub struct Page_handleJavaScriptDialogReturnValue {}
/// Navigates current page to the given URL.
pub struct Page_navigateParameters {
    url: []const u8 = "",
    referrer: ([]const u8)? = null,
    transitionType: Page_TransitionType? = null,
    frameId: Page_FrameId? = null,
    referrerPolicy: Page_ReferrerPolicy? = null,
}
pub struct Page_navigateReturnValue {
    frameId: Page_FrameId = "",
    loaderId: Network_LoaderId? = null,
    errorText: ([]const u8)? = null,
    isDownload: bool? = null,
}
/// Navigates current page to the given history entry.
pub struct Page_navigateToHistoryEntryParameters {
    entryId: f64 = 0.0,
}
pub struct Page_navigateToHistoryEntryReturnValue {}
/// Print page as PDF.
pub struct Page_printToPDFParameters {
    landscape: bool? = null,
    displayHeaderFooter: bool? = null,
    printBackground: bool? = null,
    scale: f64? = null,
    paperWidth: f64? = null,
    paperHeight: f64? = null,
    marginTop: f64? = null,
    marginBottom: f64? = null,
    marginLeft: f64? = null,
    marginRight: f64? = null,
    pageRanges: ([]const u8)? = null,
    headerTemplate: ([]const u8)? = null,
    footerTemplate: ([]const u8)? = null,
    preferCSSPageSize: bool? = null,
    transferMode: ([]const u8)? = null, // enum: "ReturnAsBase64"|"ReturnAsStream"
    generateTaggedPDF: bool? = null,
    generateDocumentOutline: bool? = null,
}
pub struct Page_printToPDFReturnValue {
    data: ProtocolBinary = "",
    stream: IO_StreamHandle? = null,
}
/// Reloads given page optionally ignoring the cache.
pub struct Page_reloadParameters {
    ignoreCache: bool? = null,
    scriptToEvaluateOnLoad: ([]const u8)? = null,
    loaderId: Network_LoaderId? = null,
}
pub struct Page_reloadReturnValue {}
/// Deprecated, please use removeScriptToEvaluateOnNewDocument instead.
pub struct Page_removeScriptToEvaluateOnLoadParameters {
    identifier: Page_ScriptIdentifier = "",
}
pub struct Page_removeScriptToEvaluateOnLoadReturnValue {}
/// Removes given script from the list.
pub struct Page_removeScriptToEvaluateOnNewDocumentParameters {
    identifier: Page_ScriptIdentifier = "",
}
pub struct Page_removeScriptToEvaluateOnNewDocumentReturnValue {}
/// Acknowledges that a screencast frame has been received by the frontend.
pub struct Page_screencastFrameAckParameters {
    sessionId: f64 = 0.0,
}
pub struct Page_screencastFrameAckReturnValue {}
/// Searches for given string in resource content.
pub struct Page_searchInResourceParameters {
    frameId: Page_FrameId = "",
    url: []const u8 = "",
    query: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Page_searchInResourceReturnValue {
    result: []const Debugger_SearchMatch = &.{},
}
/// Enable Chrome's experimental ad filter on all sites.
pub struct Page_setAdBlockingEnabledParameters {
    enabled: bool = false,
}
pub struct Page_setAdBlockingEnabledReturnValue {}
/// Enable page Content Security Policy by-passing.
pub struct Page_setBypassCSPParameters {
    enabled: bool = false,
}
pub struct Page_setBypassCSPReturnValue {}
/// Get Permissions Policy state on given frame.
pub struct Page_getPermissionsPolicyStateParameters {
    frameId: Page_FrameId = "",
}
pub struct Page_getPermissionsPolicyStateReturnValue {
    states: []const Page_PermissionsPolicyFeatureState = &.{},
}
/// Get Origin Trials on given frame.
pub struct Page_getOriginTrialsParameters {
    frameId: Page_FrameId = "",
}
pub struct Page_getOriginTrialsReturnValue {
    originTrials: []const Page_OriginTrial = &.{},
}
/// Overrides the values of device screen dimensions (window.screen.width, window.screen.height,
pub struct Page_setDeviceMetricsOverrideParameters {
    width: f64 = 0.0,
    height: f64 = 0.0,
    deviceScaleFactor: f64 = 0.0,
    mobile: bool = false,
    scale: f64? = null,
    screenWidth: f64? = null,
    screenHeight: f64? = null,
    positionX: f64? = null,
    positionY: f64? = null,
    dontSetVisibleSize: bool? = null,
    screenOrientation: Emulation_ScreenOrientation? = null,
    viewport: Page_Viewport? = null,
}
pub struct Page_setDeviceMetricsOverrideReturnValue {}
/// Overrides the Device Orientation.
pub struct Page_setDeviceOrientationOverrideParameters {
    alpha: f64 = 0.0,
    beta: f64 = 0.0,
    gamma: f64 = 0.0,
}
pub struct Page_setDeviceOrientationOverrideReturnValue {}
/// Set generic font families.
pub struct Page_setFontFamiliesParameters {
    fontFamilies: Page_FontFamilies = .{},
    forScripts: ([]const Page_ScriptFontFamilies)? = null,
}
pub struct Page_setFontFamiliesReturnValue {}
/// Set default font sizes.
pub struct Page_setFontSizesParameters {
    fontSizes: Page_FontSizes = .{},
}
pub struct Page_setFontSizesReturnValue {}
/// Sets given markup as the document's HTML.
pub struct Page_setDocumentContentParameters {
    frameId: Page_FrameId = "",
    html: []const u8 = "",
}
pub struct Page_setDocumentContentReturnValue {}
/// Set the behavior when downloading a file.
pub struct Page_setDownloadBehaviorParameters {
    behavior: []const u8 = "", // enum: "deny"|"allow"|"default"
    downloadPath: ([]const u8)? = null,
}
pub struct Page_setDownloadBehaviorReturnValue {}
/// Overrides the Geolocation Position or Error. Omitting any of the parameters emulates position
pub struct Page_setGeolocationOverrideParameters {
    latitude: f64? = null,
    longitude: f64? = null,
    accuracy: f64? = null,
}
pub struct Page_setGeolocationOverrideReturnValue {}
/// Controls whether page will emit lifecycle events.
pub struct Page_setLifecycleEventsEnabledParameters {
    enabled: bool = false,
}
pub struct Page_setLifecycleEventsEnabledReturnValue {}
/// Toggles mouse event-based touch event emulation.
pub struct Page_setTouchEmulationEnabledParameters {
    enabled: bool = false,
    configuration: ([]const u8)? = null, // enum: "mobile"|"desktop"
}
pub struct Page_setTouchEmulationEnabledReturnValue {}
/// Starts sending each frame using the `screencastFrame` event.
pub struct Page_startScreencastParameters {
    format: ([]const u8)? = null, // enum: "jpeg"|"png"
    quality: f64? = null,
    maxWidth: f64? = null,
    maxHeight: f64? = null,
    everyNthFrame: f64? = null,
    maxFramesInFlight: f64? = null,
    sendLastFrame: bool? = null,
}
pub struct Page_startScreencastReturnValue {}
/// Starts screencast video recording.
pub struct Page_startScreenRecordingParameters {
    audio: bool? = null,
    maxWidth: f64? = null,
    maxHeight: f64? = null,
    frameRate: f64? = null,
    captureArea: ([]const u8)? = null, // enum: "target"|"browserWindow"
}
pub struct Page_startScreenRecordingReturnValue {
    stream: IO_StreamHandle = "",
}
/// Stops screencast video recording.
pub struct Page_stopScreenRecordingParameters {}
pub struct Page_stopScreenRecordingReturnValue {
    stream: IO_StreamHandle = "",
}
/// Force the page stop all navigations and pending resource fetches.
pub struct Page_stopLoadingParameters {}
pub struct Page_stopLoadingReturnValue {}
/// Crashes renderer on the IO thread, generates minidumps.
pub struct Page_crashParameters {}
pub struct Page_crashReturnValue {}
/// Tries to close page, running its beforeunload hooks, if any.
pub struct Page_closeParameters {}
pub struct Page_closeReturnValue {}
/// Tries to update the web lifecycle state of the page.
pub struct Page_setWebLifecycleStateParameters {
    state: []const u8 = "", // enum: "frozen"|"active"
}
pub struct Page_setWebLifecycleStateReturnValue {}
/// Stops sending each frame in the `screencastFrame`.
pub struct Page_stopScreencastParameters {}
pub struct Page_stopScreencastReturnValue {}
/// Requests backend to produce compilation cache for the specified scripts.
pub struct Page_produceCompilationCacheParameters {
    scripts: []const Page_CompilationCacheParams = &.{},
}
pub struct Page_produceCompilationCacheReturnValue {}
/// Seeds compilation cache for given url. Compilation cache does not survive
pub struct Page_addCompilationCacheParameters {
    url: []const u8 = "",
    data: ProtocolBinary = "",
}
pub struct Page_addCompilationCacheReturnValue {}
/// Clears seeded compilation cache.
pub struct Page_clearCompilationCacheParameters {}
pub struct Page_clearCompilationCacheReturnValue {}
/// Sets the Secure Payment Confirmation transaction mode.
pub struct Page_setSPCTransactionModeParameters {
    mode: []const u8 = "", // enum: "none"|"autoAccept"|"autoChooseToAuthAnotherWay"|"autoReject"|"autoOptOut"
}
pub struct Page_setSPCTransactionModeReturnValue {}
/// Extensions for Custom Handlers API:
pub struct Page_setRPHRegistrationModeParameters {
    mode: []const u8 = "", // enum: "none"|"autoAccept"|"autoReject"
}
pub struct Page_setRPHRegistrationModeReturnValue {}
/// Generates a report for testing.
pub struct Page_generateTestReportParameters {
    message: []const u8 = "",
    group: ([]const u8)? = null,
}
pub struct Page_generateTestReportReturnValue {}
/// Pauses page execution. Can be resumed using generic Runtime.runIfWaitingForDebugger.
pub struct Page_waitForDebuggerParameters {}
pub struct Page_waitForDebuggerReturnValue {}
/// Intercept file chooser requests and transfer control to protocol clients.
pub struct Page_setInterceptFileChooserDialogParameters {
    enabled: bool = false,
    cancel: bool? = null,
}
pub struct Page_setInterceptFileChooserDialogReturnValue {}
/// Enable/disable prerendering manually.
pub struct Page_setPrerenderingAllowedParameters {
    isAllowed: bool = false,
}
pub struct Page_setPrerenderingAllowedReturnValue {}
/// Get the annotated page content for the main frame.
pub struct Page_getAnnotatedPageContentParameters {
    includeActionableInformation: bool? = null,
}
pub struct Page_getAnnotatedPageContentReturnValue {
    content: ProtocolBinary = "",
}
/// Configures input visualization on the page.
pub struct Page_setInputVisualizationParameters {
    clicks: bool? = null,
}
pub struct Page_setInputVisualizationReturnValue {}

// ============================================================================
// Domain: Performance
// ============================================================================

/// Run-time execution metric.
pub struct Performance_Metric {
    name: []const u8 = "",
    value: f64 = 0.0,
}
/// Current values of the metrics.
pub struct Performance_metricsPayload {
    metrics: []const Performance_Metric = &.{},
    title: []const u8 = "",
}
/// Disable collecting and reporting metrics.
pub struct Performance_disableParameters {}
pub struct Performance_disableReturnValue {}
/// Enable collecting and reporting metrics.
pub struct Performance_enableParameters {
    timeDomain: ([]const u8)? = null, // enum: "timeTicks"|"threadTicks"
}
pub struct Performance_enableReturnValue {}
/// Sets time domain to use for collecting and reporting duration metrics.
pub struct Performance_setTimeDomainParameters {
    timeDomain: []const u8 = "", // enum: "timeTicks"|"threadTicks"
}
pub struct Performance_setTimeDomainReturnValue {}
/// Retrieve current values of run-time metrics.
pub struct Performance_getMetricsParameters {}
pub struct Performance_getMetricsReturnValue {
    metrics: []const Performance_Metric = &.{},
}
