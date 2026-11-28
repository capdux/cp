// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue, ProtocolStringMap } from "./common";
import { DOM_NodeId, DOM_RGBAColor } from "./domains_02_console_to_dom";
import { GenericTypes_SearchMatch } from "./domains_03_domdebugger_to_inspector";
import { Network_FrameId, Network_LoaderId, Network_RequestId } from "./domains_04_layertree_to_network";
import { Runtime_RemoteObject } from "./domains_06_runtime_to_worker";

// ============================================================================
// Domain: Page
// Actions and events related to the inspected page belong to the page domain.
// ============================================================================

/// List of settings able to be overridden by WebInspector.
/// enum: "PrivateClickMeasurementDebugModeEnabled"|"AuthorAndUserStylesEnabled"|"FixedBackgroundsPaintRelativeToDocument"|"Ful...
pub type Page_Setting = []const u8;
/// A user preference that can be overriden by Web Inspector, like an accessibility preference.
pub struct Page_UserPreference {
    name: Page_UserPreferenceName = "",
    value: Page_UserPreferenceValue = "",
}
/// User preference name.
/// enum: "PrefersReducedMotion"|"PrefersContrast"|"PrefersColorScheme"
pub type Page_UserPreferenceName = []const u8;
/// User preference value.
/// enum: "NoPreference"|"Reduce"|"More"|"Light"|"Dark"
pub type Page_UserPreferenceValue = []const u8;
/// Resource type as it was perceived by the rendering engine.
/// enum: "Document"|"StyleSheet"|"Image"|"Font"|"Script"|"XHR"|"Fetch"|"Ping"|"Beacon"|"WebSocket"|"EventSource"|"Other"
pub type Page_ResourceType = []const u8;
/// Coordinate system used by supplied coordinates.
/// enum: "Viewport"|"Page"
pub type Page_CoordinateSystem = []const u8;
/// Image format used to encode a captured snapshot.
/// enum: "png"|"jpeg"|"webp"
pub type Page_ImageFormat = []const u8;
/// Same-Site policy of a cookie.
/// enum: "None"|"Lax"|"Strict"
pub type Page_CookieSameSitePolicy = []const u8;
/// Page forced-colors media query override.
/// enum: "Active"|"None"
pub type Page_ForcedColors = []const u8;
/// Information about the Frame on the page.
pub struct Page_Frame {
    id: []const u8 = "",
    parentId: ([]const u8)? = null,
    loaderId: Network_LoaderId = "",
    name: ([]const u8)? = null,
    url: []const u8 = "",
    securityOrigin: []const u8 = "",
    mimeType: []const u8 = "",
}
pub struct Page_FrameResource {
    url: []const u8 = "",
    type_: Page_ResourceType = "", // protocol field: "type"
    mimeType: []const u8 = "",
    failed: bool? = null,
    canceled: bool? = null,
    sourceMapURL: ([]const u8)? = null,
    targetId: ([]const u8)? = null,
}
/// Information about the Frame hierarchy along with their cached resources.
pub struct Page_FrameResourceTree {
    frame: Page_Frame = .{},
    childFrames: ([]const Page_FrameResourceTree)? = null,
    resources: []const Page_FrameResource = &.{},
}
/// Search result for resource.
pub struct Page_SearchResult {
    url: []const u8 = "",
    frameId: Network_FrameId = "",
    matchesCount: f64 = 0.0,
    requestId: Network_RequestId? = null,
}
/// Cookie object
pub struct Page_Cookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "",
    expires: f64 = 0.0,
    session: bool = false,
    httpOnly: bool = false,
    secure: bool = false,
    sameSite: Page_CookieSameSitePolicy = "",
    partitionKey: ([]const u8)? = null,
}
pub struct Page_Insets {
    top: f64 = 0.0,
    right: f64 = 0.0,
    bottom: f64 = 0.0,
    left: f64 = 0.0,
}
pub struct Page_domContentEventFiredPayload {
    timestamp: f64 = 0.0,
    frameId: Network_FrameId = "",
}
pub struct Page_loadEventFiredPayload {
    timestamp: f64 = 0.0,
    frameId: Network_FrameId = "",
}
/// Fired once navigation of the frame has completed. Frame is now associated with the new loader.
pub struct Page_frameNavigatedPayload {
    frame: Page_Frame = .{},
}
/// Fired when frame has been attached to its parent.
pub struct Page_frameAttachedPayload {
    frameId: Network_FrameId = "",
    parentFrameId: Network_FrameId? = null,
}
/// Fired when frame has been detached from its parent.
pub struct Page_frameDetachedPayload {
    frameId: Network_FrameId = "",
}
/// Fired when same-document navigation happens, e.g. due to history API usage or anchor navigation.
pub struct Page_navigatedWithinDocumentPayload {
    frameId: Network_FrameId = "",
    url: []const u8 = "",
}
/// Fired when the default value of a user preference changes at the system level.
pub struct Page_defaultUserPreferencesDidChangePayload {
    preferences: []const Page_UserPreference = &.{},
}
/// Fired when page is about to check policy for newly triggered navigation.
pub struct Page_willCheckNavigationPolicyPayload {
    frameId: Network_FrameId = "",
}
/// Fired when page has received navigation policy decision.
pub struct Page_didCheckNavigationPolicyPayload {
    frameId: Network_FrameId = "",
    cancel: bool? = null,
}
/// Fired when the page shows file chooser for it's <input type=file>.
pub struct Page_fileChooserOpenedPayload {
    frameId: Network_FrameId = "",
    element: Runtime_RemoteObject = .{},
}
/// Enables page domain notifications.
pub struct Page_enableParameters {}
pub struct Page_enableReturnValue {}
/// Disables page domain notifications.
pub struct Page_disableParameters {}
pub struct Page_disableReturnValue {}
/// Reloads the main frame of the inspected page.
pub struct Page_reloadParameters {
    ignoreCache: bool? = null,
    revalidateAllResources: bool? = null,
}
pub struct Page_reloadReturnValue {}
/// Goes back in the history.
pub struct Page_goBackParameters {}
pub struct Page_goBackReturnValue {}
/// Goes forward in the history.
pub struct Page_goForwardParameters {}
pub struct Page_goForwardReturnValue {}
/// Override's the user agent of the inspected page
pub struct Page_overrideUserAgentParameters {
    value: ([]const u8)? = null,
}
pub struct Page_overrideUserAgentReturnValue {}
/// Override's the navigator.platform of the inspected page
pub struct Page_overridePlatformParameters {
    value: ([]const u8)? = null,
}
pub struct Page_overridePlatformReturnValue {}
/// Allows the frontend to override the inspected page's settings.
pub struct Page_overrideSettingParameters {
    setting: Page_Setting = "",
    value: bool? = null,
}
pub struct Page_overrideSettingReturnValue {}
/// Allows the frontend to override the user's preferences on the inspected page.
pub struct Page_overrideUserPreferenceParameters {
    name: Page_UserPreferenceName = "",
    value: Page_UserPreferenceValue? = null,
}
pub struct Page_overrideUserPreferenceReturnValue {}
/// Returns all browser cookies. Depending on the backend support, will return detailed cookie information in the <code>cookies</code> field.
pub struct Page_getCookiesParameters {}
pub struct Page_getCookiesReturnValue {
    cookies: []const Page_Cookie = &.{},
}
/// Sets a new browser cookie with the given name, domain, and path.
pub struct Page_setCookieParameters {
    cookie: Page_Cookie = .{},
    shouldPartition: bool? = null,
}
pub struct Page_setCookieReturnValue {}
/// Deletes browser cookie with given name, domain, and path.
pub struct Page_deleteCookieParameters {
    cookieName: []const u8 = "",
    url: []const u8 = "",
}
pub struct Page_deleteCookieReturnValue {}
/// Returns present frame / resource tree structure.
pub struct Page_getResourceTreeParameters {}
pub struct Page_getResourceTreeReturnValue {
    frameTree: Page_FrameResourceTree = .{},
}
/// Returns content of the given resource.
pub struct Page_getResourceContentParameters {
    frameId: Network_FrameId = "",
    url: []const u8 = "",
}
pub struct Page_getResourceContentReturnValue {
    content: []const u8 = "",
    base64Encoded: bool = false,
}
pub struct Page_setBootstrapScriptParameters {
    source: ([]const u8)? = null,
}
pub struct Page_setBootstrapScriptReturnValue {}
/// Searches for given string in resource content.
pub struct Page_searchInResourceParameters {
    frameId: Network_FrameId = "",
    url: []const u8 = "",
    query: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
    requestId: Network_RequestId? = null,
}
pub struct Page_searchInResourceReturnValue {
    result: []const GenericTypes_SearchMatch = &.{},
}
/// Searches for given string in frame / resource tree structure.
pub struct Page_searchInResourcesParameters {
    text: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Page_searchInResourcesReturnValue {
    result: []const Page_SearchResult = &.{},
}
/// Requests that backend draw rulers in the inspector overlay
pub struct Page_setShowRulersParameters {
    result: bool = false,
}
pub struct Page_setShowRulersReturnValue {}
/// Requests that backend shows paint rectangles
pub struct Page_setShowPaintRectsParameters {
    result: bool = false,
}
pub struct Page_setShowPaintRectsReturnValue {}
/// Emulates the given media for CSS media queries.
pub struct Page_setEmulatedMediaParameters {
    media: []const u8 = "",
}
pub struct Page_setEmulatedMediaReturnValue {}
/// Forces the forced-colors media query for the page.
pub struct Page_setForcedColorsParameters {
    forcedColors: Page_ForcedColors? = null,
}
pub struct Page_setForcedColorsReturnValue {}
/// Enables time zone emulation.
pub struct Page_setTimeZoneParameters {
    timeZone: ([]const u8)? = null,
}
pub struct Page_setTimeZoneReturnValue {}
/// Enables touch events on platforms that lack them.
pub struct Page_setTouchEmulationEnabledParameters {
    enabled: bool = false,
}
pub struct Page_setTouchEmulationEnabledReturnValue {}
/// Capture a snapshot of the specified node that does not include unrelated layers.
pub struct Page_snapshotNodeParameters {
    nodeId: DOM_NodeId = 0.0,
    targetId: ([]const u8)? = null,
}
pub struct Page_snapshotNodeReturnValue {
    dataURL: []const u8 = "",
}
/// Capture a snapshot of the page within the specified rectangle and coordinate system.
pub struct Page_snapshotRectParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    coordinateSystem: Page_CoordinateSystem = "",
    omitDeviceScaleFactor: bool? = null,
    format: Page_ImageFormat? = null,
    quality: f64? = null,
}
pub struct Page_snapshotRectReturnValue {
    dataURL: []const u8 = "",
}
/// Grab an archive of the page.
pub struct Page_archiveParameters {}
pub struct Page_archiveReturnValue {
    data: []const u8 = "",
}
/// Overrides screen size exposed to DOM and used in media queries for testing with provided values.
pub struct Page_setScreenSizeOverrideParameters {
    width: f64? = null,
    height: f64? = null,
}
pub struct Page_setScreenSizeOverrideReturnValue {}
/// Insert text into the current selection of the page.
pub struct Page_insertTextParameters {
    text: []const u8 = "",
}
pub struct Page_insertTextReturnValue {}
/// Intercepts file chooser dialog
pub struct Page_setInterceptFileChooserDialogParameters {
    enabled: bool = false,
}
pub struct Page_setInterceptFileChooserDialogReturnValue {}
/// Sets or clears an override of the default background color of the frame. This override is used if the content does not specify one.
pub struct Page_setDefaultBackgroundColorOverrideParameters {
    color: DOM_RGBAColor? = null,
}
pub struct Page_setDefaultBackgroundColorOverrideReturnValue {}
/// Creates an user world for every loaded frame.
pub struct Page_createUserWorldParameters {
    name: []const u8 = "",
}
pub struct Page_createUserWorldReturnValue {}
/// Enable page Content Security Policy by-passing.
pub struct Page_setBypassCSPParameters {
    enabled: bool = false,
}
pub struct Page_setBypassCSPReturnValue {}
/// Crashes the page process
pub struct Page_crashParameters {}
pub struct Page_crashReturnValue {}
/// Ensures that the scroll regions are up to date.
pub struct Page_updateScrollingStateParameters {}
pub struct Page_updateScrollingStateReturnValue {}

// ============================================================================
// Domain: Playwright
// ============================================================================

/// Id of Browser context.
pub type Playwright_ContextID = []const u8;
/// Id of WebPageProxy.
pub type Playwright_PageProxyID = []const u8;
/// Same-Site policy of a cookie.
/// enum: "None"|"Lax"|"Strict"
pub type Playwright_CookieSameSitePolicy = []const u8;
/// Cookie object
pub struct Playwright_Cookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "",
    expires: f64 = 0.0,
    httpOnly: bool = false,
    secure: bool = false,
    session: bool = false,
    sameSite: Playwright_CookieSameSitePolicy = "",
    partitionKey: ([]const u8)? = null,
}
/// Cookie object
pub struct Playwright_SetCookieParam {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "",
    expires: f64? = null,
    httpOnly: bool? = null,
    secure: bool? = null,
    session: bool? = null,
    sameSite: Playwright_CookieSameSitePolicy? = null,
    partitionKey: ([]const u8)? = null,
}
/// Name-value pair
pub struct Playwright_NameValue {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Origin object
pub struct Playwright_OriginStorage {
    origin: []const u8 = "",
    items: []const Playwright_NameValue = &.{},
}
/// Geolocation
pub struct Playwright_Geolocation {
    timestamp: f64 = 0.0,
    latitude: f64 = 0.0,
    longitude: f64 = 0.0,
    accuracy: f64 = 0.0,
}
pub struct Playwright_pageProxyCreatedPayload {
    browserContextId: Playwright_ContextID = "",
    pageProxyId: Playwright_PageProxyID = "",
    openerId: Playwright_PageProxyID? = null,
}
pub struct Playwright_pageProxyDestroyedPayload {
    pageProxyId: Playwright_PageProxyID = "",
}
/// Fired when provisional load fails.
pub struct Playwright_provisionalLoadFailedPayload {
    pageProxyId: Playwright_PageProxyID = "",
    loaderId: Network_LoaderId = "",
    error_: []const u8 = "", // protocol field: "error"
}
/// Fired when page opens a new window.
pub struct Playwright_windowOpenPayload {
    pageProxyId: Playwright_PageProxyID = "",
    url: []const u8 = "",
    windowFeatures: []const []const u8 = &.{},
}
pub struct Playwright_downloadCreatedPayload {
    pageProxyId: Playwright_PageProxyID = "",
    frameId: Network_FrameId = "",
    uuid: []const u8 = "",
    url: []const u8 = "",
}
pub struct Playwright_downloadFilenameSuggestedPayload {
    uuid: []const u8 = "",
    suggestedFilename: []const u8 = "",
}
pub struct Playwright_downloadFinishedPayload {
    uuid: []const u8 = "",
    error_: []const u8 = "", // protocol field: "error"
}
pub struct Playwright_enableParameters {}
pub struct Playwright_enableReturnValue {}
pub struct Playwright_disableParameters {}
pub struct Playwright_disableReturnValue {}
pub struct Playwright_getInfoParameters {}
pub struct Playwright_getInfoReturnValue {
    os: []const u8 = "",
}
/// Close browser.
pub struct Playwright_closeParameters {}
pub struct Playwright_closeReturnValue {}
/// Creates new ephemeral browser context.
pub struct Playwright_createContextParameters {
    proxyServer: ([]const u8)? = null,
    proxyBypassList: ([]const u8)? = null,
    enableStoragePartitioning: bool? = null,
}
pub struct Playwright_createContextReturnValue {
    browserContextId: Playwright_ContextID = "",
}
/// Deletes browser context previously created with createContect. The command will automatically close all pages that use the context.
pub struct Playwright_deleteContextParameters {
    browserContextId: Playwright_ContextID = "",
}
pub struct Playwright_deleteContextReturnValue {}
pub struct Playwright_createPageParameters {
    browserContextId: Playwright_ContextID? = null,
    background: bool? = null,
}
pub struct Playwright_createPageReturnValue {
    pageProxyId: Playwright_PageProxyID = "",
}
/// Maximizes the native browser window.
pub struct Playwright_maximizeWindowParameters {
    pageProxyId: Playwright_PageProxyID = "",
}
pub struct Playwright_maximizeWindowReturnValue {}
/// Navigates current page to the given URL.
pub struct Playwright_navigateParameters {
    url: []const u8 = "",
    pageProxyId: Playwright_PageProxyID = "",
    frameId: Network_FrameId? = null,
    referrer: ([]const u8)? = null,
}
pub struct Playwright_navigateReturnValue {
    loaderId: Network_LoaderId? = null,
}
/// Grants read access for the specified files to the web process of the page.
pub struct Playwright_grantFileReadAccessParameters {
    pageProxyId: Playwright_PageProxyID = "",
    paths: []const []const u8 = &.{},
}
pub struct Playwright_grantFileReadAccessReturnValue {}
/// Capture a snapshot of the page.
pub struct Playwright_takePageScreenshotParameters {
    pageProxyId: Playwright_PageProxyID = "",
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    omitDeviceScaleFactor: bool? = null,
}
pub struct Playwright_takePageScreenshotReturnValue {
    dataURL: []const u8 = "",
}
/// Change whether all certificate errors should be ignored.
pub struct Playwright_setIgnoreCertificateErrorsParameters {
    browserContextId: Playwright_ContextID? = null,
    ignore: bool = false,
}
pub struct Playwright_setIgnoreCertificateErrorsReturnValue {}
/// Changes page zoom factor.
pub struct Playwright_setPageZoomFactorParameters {
    pageProxyId: Playwright_PageProxyID = "",
    zoomFactor: f64 = 0.0,
}
pub struct Playwright_setPageZoomFactorReturnValue {}
/// Closes the page, destroying the page proxy. Works for both live and crashed pages.
pub struct Playwright_closePageParameters {
    pageProxyId: Playwright_PageProxyID = "",
    runBeforeUnload: bool? = null,
}
pub struct Playwright_closePageReturnValue {}
/// Returns all cookies in the given browser context.
pub struct Playwright_getAllCookiesParameters {
    browserContextId: Playwright_ContextID? = null,
}
pub struct Playwright_getAllCookiesReturnValue {
    cookies: []const Playwright_Cookie = &.{},
}
/// Sets cookies in the given browser context.
pub struct Playwright_setCookiesParameters {
    browserContextId: Playwright_ContextID? = null,
    cookies: []const Playwright_SetCookieParam = &.{},
}
pub struct Playwright_setCookiesReturnValue {}
/// Deletes cookies in the given browser context.
pub struct Playwright_deleteAllCookiesParameters {
    browserContextId: Playwright_ContextID? = null,
}
pub struct Playwright_deleteAllCookiesReturnValue {}
/// Overrides the geolocation position or error.
pub struct Playwright_setGeolocationOverrideParameters {
    browserContextId: Playwright_ContextID? = null,
    geolocation: Playwright_Geolocation? = null,
}
pub struct Playwright_setGeolocationOverrideReturnValue {}
/// Allows to set locale language for context.
pub struct Playwright_setLanguagesParameters {
    languages: []const []const u8 = &.{},
    browserContextId: Playwright_ContextID? = null,
}
pub struct Playwright_setLanguagesReturnValue {}
/// Allows to override download behavior.
pub struct Playwright_setDownloadBehaviorParameters {
    behavior: ([]const u8)? = null, // enum: "allow"|"deny"
    downloadPath: ([]const u8)? = null,
    browserContextId: Playwright_ContextID? = null,
}
pub struct Playwright_setDownloadBehaviorReturnValue {}
/// Cancels a current running download.
pub struct Playwright_cancelDownloadParameters {
    uuid: []const u8 = "",
}
pub struct Playwright_cancelDownloadReturnValue {}
/// Clears browser memory cache.
pub struct Playwright_clearMemoryCacheParameters {
    browserContextId: Playwright_ContextID? = null,
}
pub struct Playwright_clearMemoryCacheReturnValue {}

// ============================================================================
// Domain: Recording
// General types used for recordings of actions performed in the inspected page.
// ============================================================================

/// The type of the recording.
/// enum: "canvas-2d"|"offscreen-canvas-2d"|"canvas-bitmaprenderer"|"offscreen-canvas-bitmaprenderer"|"canvas-webgl"|"offscreen...
pub type Recording_Type = []const u8;
/// enum: "frontend"|"console"|"auto-capture"
pub type Recording_Initiator = []const u8;
/// Information about the initial state of the recorded object.
pub struct Recording_InitialState {
    attributes: ProtocolStringMap? = null,
    states: ([]const ProtocolStringMap)? = null,
    parameters: ([]const ProtocolJsonValue)? = null,
    content: ([]const u8)? = null,
}
/// Container object for a single frame of the recording.
pub struct Recording_Frame {
    actions: []const ProtocolJsonValue = &.{},
    duration: f64? = null,
    incomplete: bool? = null,
}
pub struct Recording_Recording {
    version: f64 = 0.0,
    type_: Recording_Type = "", // protocol field: "type"
    initialState: Recording_InitialState = .{},
    data: []const ProtocolJsonValue = &.{},
    name: ([]const u8)? = null,
}
