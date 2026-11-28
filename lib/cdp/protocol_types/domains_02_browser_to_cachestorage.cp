// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue, ProtocolStringMap } from "./common";
import {
    DOM_BackendNodeId,
    DOM_LogicalAxes,
    DOM_NodeId,
    DOM_PhysicalAxes,
    DOM_PseudoType,
    DOM_StyleSheetId,
} from "./domains_03_cast_to_domsnapshot";
import { Page_FrameId } from "./domains_08_page_to_performance";
import { Storage_StorageBucket } from "./domains_09_performancetimeline_to_systeminfo";
import { Target_TargetID } from "./domains_10_target_to_console";

// ============================================================================
// Domain: Browser
// The Browser domain defines methods and events for browser managing.
// ============================================================================

pub type Browser_BrowserContextID = []const u8;
pub type Browser_WindowID = f64;
/// The state of the browser window.
/// enum: "normal"|"minimized"|"maximized"|"fullscreen"
pub type Browser_WindowState = []const u8;
/// Browser window bounds information
pub struct Browser_Bounds {
    left: f64? = null,
    top: f64? = null,
    width: f64? = null,
    height: f64? = null,
    windowState: Browser_WindowState? = null,
}
/// enum: "ar"|"audioCapture"|"automaticFullscreen"|"backgroundFetch"|"backgroundSync"|"cameraPanTiltZoom"|"capturedSurfaceCont...
pub type Browser_PermissionType = []const u8;
/// enum: "granted"|"denied"|"prompt"
pub type Browser_PermissionSetting = []const u8;
/// Definition of PermissionDescriptor defined in the Permissions API:
pub struct Browser_PermissionDescriptor {
    name: []const u8 = "",
    sysex: bool? = null,
    userVisibleOnly: bool? = null,
    allowWithoutSanitization: bool? = null,
    allowWithoutGesture: bool? = null,
    panTiltZoom: bool? = null,
}
/// Browser command ids used by executeBrowserCommand.
/// enum: "openTabSearch"|"closeTabSearch"|"openGlic"
pub type Browser_BrowserCommandId = []const u8;
/// Chrome histogram bucket.
pub struct Browser_Bucket {
    low: f64 = 0.0,
    high: f64 = 0.0,
    count: f64 = 0.0,
}
/// Chrome histogram.
pub struct Browser_Histogram {
    name: []const u8 = "",
    sum: f64 = 0.0,
    count: f64 = 0.0,
    buckets: []const Browser_Bucket = &.{},
}
/// Fired when page is about to start a download.
pub struct Browser_downloadWillBeginPayload {
    frameId: Page_FrameId = "",
    guid: []const u8 = "",
    url: []const u8 = "",
    suggestedFilename: []const u8 = "",
}
/// Fired when download makes progress. Last call has |done| == true.
pub struct Browser_downloadProgressPayload {
    guid: []const u8 = "",
    totalBytes: f64 = 0.0,
    receivedBytes: f64 = 0.0,
    state: []const u8 = "", // enum: "inProgress"|"completed"|"canceled"
    filePath: ([]const u8)? = null,
}
/// Set permission settings for given embedding and embedded origins.
pub struct Browser_setPermissionParameters {
    permission: Browser_PermissionDescriptor = .{},
    setting: Browser_PermissionSetting = "",
    origin: ([]const u8)? = null,
    embeddedOrigin: ([]const u8)? = null,
    browserContextId: Browser_BrowserContextID? = null,
}
pub struct Browser_setPermissionReturnValue {}
/// Grant specific permissions to the given origin and reject all others. Deprecated. Use
pub struct Browser_grantPermissionsParameters {
    permissions: []const Browser_PermissionType = &.{},
    origin: ([]const u8)? = null,
    browserContextId: Browser_BrowserContextID? = null,
}
pub struct Browser_grantPermissionsReturnValue {}
/// Reset all permission management for all origins.
pub struct Browser_resetPermissionsParameters {
    browserContextId: Browser_BrowserContextID? = null,
}
pub struct Browser_resetPermissionsReturnValue {}
/// Set the behavior when downloading a file.
pub struct Browser_setDownloadBehaviorParameters {
    behavior: []const u8 = "", // enum: "deny"|"allow"|"allowAndName"|"default"
    browserContextId: Browser_BrowserContextID? = null,
    downloadPath: ([]const u8)? = null,
    eventsEnabled: bool? = null,
}
pub struct Browser_setDownloadBehaviorReturnValue {}
/// Cancel a download if in progress
pub struct Browser_cancelDownloadParameters {
    guid: []const u8 = "",
    browserContextId: Browser_BrowserContextID? = null,
}
pub struct Browser_cancelDownloadReturnValue {}
/// Close browser gracefully.
pub struct Browser_closeParameters {}
pub struct Browser_closeReturnValue {}
/// Crashes browser on the main thread.
pub struct Browser_crashParameters {}
pub struct Browser_crashReturnValue {}
/// Crashes GPU process.
pub struct Browser_crashGpuProcessParameters {}
pub struct Browser_crashGpuProcessReturnValue {}
/// Returns version information.
pub struct Browser_getVersionParameters {}
pub struct Browser_getVersionReturnValue {
    protocolVersion: []const u8 = "",
    product: []const u8 = "",
    revision: []const u8 = "",
    userAgent: []const u8 = "",
    jsVersion: []const u8 = "",
}
/// Returns the command line switches for the browser process if, and only if
pub struct Browser_getBrowserCommandLineParameters {}
pub struct Browser_getBrowserCommandLineReturnValue {
    arguments: []const []const u8 = &.{},
}
/// Adds or updates a mock camera in the shared video capture device list for
pub struct Browser_addMockCameraParameters {
    deviceId: []const u8 = "",
}
pub struct Browser_addMockCameraReturnValue {}
/// Get Chrome histograms.
pub struct Browser_getHistogramsParameters {
    query: ([]const u8)? = null,
    delta: bool? = null,
}
pub struct Browser_getHistogramsReturnValue {
    histograms: []const Browser_Histogram = &.{},
}
/// Get a Chrome histogram by name.
pub struct Browser_getHistogramParameters {
    name: []const u8 = "",
    delta: bool? = null,
}
pub struct Browser_getHistogramReturnValue {
    histogram: Browser_Histogram = .{},
}
/// Get position and size of the browser window.
pub struct Browser_getWindowBoundsParameters {
    windowId: Browser_WindowID = 0.0,
}
pub struct Browser_getWindowBoundsReturnValue {
    bounds: Browser_Bounds = .{},
}
/// Get the browser window that contains the devtools target.
pub struct Browser_getWindowForTargetParameters {
    targetId: Target_TargetID? = null,
}
pub struct Browser_getWindowForTargetReturnValue {
    windowId: Browser_WindowID = 0.0,
    bounds: Browser_Bounds = .{},
}
/// Set position and/or size of the browser window.
pub struct Browser_setWindowBoundsParameters {
    windowId: Browser_WindowID = 0.0,
    bounds: Browser_Bounds = .{},
}
pub struct Browser_setWindowBoundsReturnValue {}
/// Set size of the browser contents resizing browser window as necessary.
pub struct Browser_setContentsSizeParameters {
    windowId: Browser_WindowID = 0.0,
    width: f64? = null,
    height: f64? = null,
}
pub struct Browser_setContentsSizeReturnValue {}
/// Set dock tile details, platform-specific.
pub struct Browser_setDockTileParameters {
    badgeLabel: ([]const u8)? = null,
    image: ProtocolBinary? = null,
}
pub struct Browser_setDockTileReturnValue {}
/// Invoke custom browser commands used by telemetry.
pub struct Browser_executeBrowserCommandParameters {
    commandId: Browser_BrowserCommandId = "",
}
pub struct Browser_executeBrowserCommandReturnValue {}
/// Allows a site to use privacy sandbox features that require enrollment
pub struct Browser_addPrivacySandboxEnrollmentOverrideParameters {
    url: []const u8 = "",
}
pub struct Browser_addPrivacySandboxEnrollmentOverrideReturnValue {}
/// Gets the current globally-applied privacy control status
pub struct Browser_getGlobalPrivacyControlParameters {}
pub struct Browser_getGlobalPrivacyControlReturnValue {
    gpc: bool = false,
}
/// Sets and then gets the current globally-applied privacy control status
pub struct Browser_setGlobalPrivacyControlParameters {
    gpc: bool = false,
}
pub struct Browser_setGlobalPrivacyControlReturnValue {
    gpc: bool = false,
}

// ============================================================================
// Domain: CSS
// This domain exposes CSS read/write operations. All CSS objects (stylesheets, rules, and styles)
// have an associated `id` used in subsequent operations on the related object. Each object type has
// ============================================================================

/// Stylesheet type: "injected" for stylesheets injected via extension, "user-agent" for user-agent
/// enum: "injected"|"user-agent"|"inspector"|"regular"
pub type CSS_StyleSheetOrigin = []const u8;
/// CSS rule collection for a single pseudo style.
pub struct CSS_PseudoElementMatches {
    pseudoType: DOM_PseudoType = "",
    pseudoIdentifier: ([]const u8)? = null,
    matches: []const CSS_RuleMatch = &.{},
}
/// CSS style coming from animations with the name of the animation.
pub struct CSS_CSSAnimationStyle {
    name: ([]const u8)? = null,
    style: CSS_CSSStyle = .{},
}
/// Inherited CSS rule collection from ancestor node.
pub struct CSS_InheritedStyleEntry {
    inlineStyle: CSS_CSSStyle? = null,
    matchedCSSRules: []const CSS_RuleMatch = &.{},
}
/// Inherited CSS style collection for animated styles from ancestor node.
pub struct CSS_InheritedAnimatedStyleEntry {
    animationStyles: ([]const CSS_CSSAnimationStyle)? = null,
    transitionsStyle: CSS_CSSStyle? = null,
}
/// Inherited pseudo element matches from pseudos of an ancestor node.
pub struct CSS_InheritedPseudoElementMatches {
    pseudoElements: []const CSS_PseudoElementMatches = &.{},
}
/// Match data for a CSS rule.
pub struct CSS_RuleMatch {
    rule: CSS_CSSRule = .{},
    matchingSelectors: []const f64 = &.{},
}
/// Data for a simple selector (these are delimited by commas in a selector list).
pub struct CSS_Value {
    text: []const u8 = "",
    range: CSS_SourceRange? = null,
    specificity: CSS_Specificity? = null,
}
/// Contribution of an individual simple selector to specificity.
pub struct CSS_SpecificityComponent {
    text: []const u8 = "",
    a: f64 = 0.0,
    b: f64 = 0.0,
    c: f64 = 0.0,
}
/// Specificity:
pub struct CSS_Specificity {
    a: f64 = 0.0,
    b: f64 = 0.0,
    c: f64 = 0.0,
    components: ([]const CSS_SpecificityComponent)? = null,
}
/// Selector list data.
pub struct CSS_SelectorList {
    selectors: []const CSS_Value = &.{},
    text: []const u8 = "",
}
/// CSS stylesheet metainformation.
pub struct CSS_CSSStyleSheetHeader {
    styleSheetId: DOM_StyleSheetId = "",
    frameId: Page_FrameId = "",
    sourceURL: []const u8 = "",
    sourceMapURL: ([]const u8)? = null,
    origin: CSS_StyleSheetOrigin = "",
    title: []const u8 = "",
    ownerNode: DOM_BackendNodeId? = null,
    disabled: bool = false,
    hasSourceURL: bool? = null,
    isInline: bool = false,
    isMutable: bool = false,
    isConstructed: bool = false,
    startLine: f64 = 0.0,
    startColumn: f64 = 0.0,
    length: f64 = 0.0,
    endLine: f64 = 0.0,
    endColumn: f64 = 0.0,
    loadingFailed: bool? = null,
}
/// CSS rule representation.
pub struct CSS_CSSRule {
    styleSheetId: DOM_StyleSheetId? = null,
    selectorList: CSS_SelectorList = .{},
    nestingSelectors: ([]const []const u8)? = null,
    origin: CSS_StyleSheetOrigin = "",
    style: CSS_CSSStyle = .{},
    originTreeScopeNodeId: DOM_BackendNodeId? = null,
    media: ([]const CSS_CSSMedia)? = null,
    containerQueries: ([]const CSS_CSSContainerQuery)? = null,
    supports: ([]const CSS_CSSSupports)? = null,
    layers: ([]const CSS_CSSLayer)? = null,
    scopes: ([]const CSS_CSSScope)? = null,
    ruleTypes: ([]const CSS_CSSRuleType)? = null,
    startingStyles: ([]const CSS_CSSStartingStyle)? = null,
    navigations: ([]const CSS_CSSNavigation)? = null,
}
/// Enum indicating the type of a CSS rule, used to represent the order of a style rule's ancestors.
/// enum: "MediaRule"|"SupportsRule"|"ContainerRule"|"LayerRule"|"ScopeRule"|"StyleRule"|"StartingStyleRule"|"NavigationRule"
pub type CSS_CSSRuleType = []const u8;
/// CSS coverage information.
pub struct CSS_RuleUsage {
    styleSheetId: DOM_StyleSheetId = "",
    startOffset: f64 = 0.0,
    endOffset: f64 = 0.0,
    used: bool = false,
}
/// Text range within a resource. All numbers are zero-based.
pub struct CSS_SourceRange {
    startLine: f64 = 0.0,
    startColumn: f64 = 0.0,
    endLine: f64 = 0.0,
    endColumn: f64 = 0.0,
}
pub struct CSS_ShorthandEntry {
    name: []const u8 = "",
    value: []const u8 = "",
    important: bool? = null,
}
pub struct CSS_CSSComputedStyleProperty {
    name: []const u8 = "",
    value: []const u8 = "",
}
pub struct CSS_ComputedStyleExtraFields {
    isAppearanceBase: bool = false,
}
/// CSS style representation.
pub struct CSS_CSSStyle {
    styleSheetId: DOM_StyleSheetId? = null,
    cssProperties: []const CSS_CSSProperty = &.{},
    shorthandEntries: []const CSS_ShorthandEntry = &.{},
    cssText: ([]const u8)? = null,
    range: CSS_SourceRange? = null,
}
/// CSS property declaration data.
pub struct CSS_CSSProperty {
    name: []const u8 = "",
    value: []const u8 = "",
    important: bool? = null,
    implicit: bool? = null,
    text: ([]const u8)? = null,
    parsedOk: bool? = null,
    disabled: bool? = null,
    range: CSS_SourceRange? = null,
    longhandProperties: ([]const CSS_CSSProperty)? = null,
}
/// CSS media rule descriptor.
pub struct CSS_CSSMedia {
    text: []const u8 = "",
    source: []const u8 = "", // enum: "mediaRule"|"importRule"|"linkedSheet"|"inlineSheet"
    sourceURL: ([]const u8)? = null,
    range: CSS_SourceRange? = null,
    styleSheetId: DOM_StyleSheetId? = null,
    mediaList: ([]const CSS_MediaQuery)? = null,
}
/// Media query descriptor.
pub struct CSS_MediaQuery {
    expressions: []const CSS_MediaQueryExpression = &.{},
    active: bool = false,
}
/// Media query expression descriptor.
pub struct CSS_MediaQueryExpression {
    value: f64 = 0.0,
    unit: []const u8 = "",
    feature: []const u8 = "",
    valueRange: CSS_SourceRange? = null,
    computedLength: f64? = null,
}
/// CSS container query rule descriptor.
pub struct CSS_CSSContainerQuery {
    text: []const u8 = "",
    range: CSS_SourceRange? = null,
    styleSheetId: DOM_StyleSheetId? = null,
    name: ([]const u8)? = null,
    physicalAxes: DOM_PhysicalAxes? = null,
    logicalAxes: DOM_LogicalAxes? = null,
    queriesScrollState: bool? = null,
    queriesAnchored: bool? = null,
    conditionText: []const u8 = "",
}
/// CSS Supports at-rule descriptor.
pub struct CSS_CSSSupports {
    text: []const u8 = "",
    active: bool = false,
    range: CSS_SourceRange? = null,
    styleSheetId: DOM_StyleSheetId? = null,
}
/// CSS Navigation at-rule descriptor.
pub struct CSS_CSSNavigation {
    text: []const u8 = "",
    active: bool? = null,
    range: CSS_SourceRange? = null,
    styleSheetId: DOM_StyleSheetId? = null,
}
/// CSS Scope at-rule descriptor.
pub struct CSS_CSSScope {
    text: []const u8 = "",
    range: CSS_SourceRange? = null,
    styleSheetId: DOM_StyleSheetId? = null,
}
/// CSS Layer at-rule descriptor.
pub struct CSS_CSSLayer {
    text: []const u8 = "",
    range: CSS_SourceRange? = null,
    styleSheetId: DOM_StyleSheetId? = null,
}
/// CSS Starting Style at-rule descriptor.
pub struct CSS_CSSStartingStyle {
    range: CSS_SourceRange? = null,
    styleSheetId: DOM_StyleSheetId? = null,
}
/// CSS Layer data.
pub struct CSS_CSSLayerData {
    name: []const u8 = "",
    subLayers: ([]const CSS_CSSLayerData)? = null,
    order: f64 = 0.0,
}
/// Information about amount of glyphs that were rendered with given font.
pub struct CSS_PlatformFontUsage {
    familyName: []const u8 = "",
    postScriptName: []const u8 = "",
    isCustomFont: bool = false,
    glyphCount: f64 = 0.0,
}
/// Information about font variation axes for variable fonts
pub struct CSS_FontVariationAxis {
    tag: []const u8 = "",
    name: []const u8 = "",
    minValue: f64 = 0.0,
    maxValue: f64 = 0.0,
    defaultValue: f64 = 0.0,
}
/// Properties of a web font: https://www.w3.org/TR/2008/REC-CSS2-20080411/fonts.html#font-descriptions
pub struct CSS_FontFace {
    fontFamily: []const u8 = "",
    fontStyle: []const u8 = "",
    fontVariant: []const u8 = "",
    fontWeight: []const u8 = "",
    fontStretch: []const u8 = "",
    fontDisplay: []const u8 = "",
    unicodeRange: []const u8 = "",
    src: []const u8 = "",
    platformFontFamily: []const u8 = "",
    fontVariationAxes: ([]const CSS_FontVariationAxis)? = null,
}
/// CSS try rule representation.
pub struct CSS_CSSTryRule {
    styleSheetId: DOM_StyleSheetId? = null,
    origin: CSS_StyleSheetOrigin = "",
    style: CSS_CSSStyle = .{},
}
/// CSS @position-try rule representation.
pub struct CSS_CSSPositionTryRule {
    name: CSS_Value = .{},
    styleSheetId: DOM_StyleSheetId? = null,
    origin: CSS_StyleSheetOrigin = "",
    style: CSS_CSSStyle = .{},
    active: bool = false,
}
/// CSS keyframes rule representation.
pub struct CSS_CSSKeyframesRule {
    animationName: CSS_Value = .{},
    keyframes: []const CSS_CSSKeyframeRule = &.{},
}
/// Representation of a custom property registration through CSS.registerProperty
pub struct CSS_CSSPropertyRegistration {
    propertyName: []const u8 = "",
    initialValue: CSS_Value? = null,
    inherits: bool = false,
    syntax: []const u8 = "",
}
/// CSS generic @rule representation.
pub struct CSS_CSSAtRule {
    type_: []const u8 = "", // protocol field: "type"; enum: "font-face"|"font-feature-values"|"font-palette-values"|"counter-style"
    subsection: ([]const u8)? = null, // enum: "swash"|"annotation"|"ornaments"|"stylistic"|"styleset"|"character-variant"
    name: CSS_Value? = null,
    styleSheetId: DOM_StyleSheetId? = null,
    origin: CSS_StyleSheetOrigin = "",
    style: CSS_CSSStyle = .{},
}
/// CSS property at-rule representation.
pub struct CSS_CSSPropertyRule {
    styleSheetId: DOM_StyleSheetId? = null,
    origin: CSS_StyleSheetOrigin = "",
    propertyName: CSS_Value = .{},
    style: CSS_CSSStyle = .{},
}
/// CSS function argument representation.
pub struct CSS_CSSFunctionParameter {
    name: []const u8 = "",
    type_: []const u8 = "", // protocol field: "type"
}
/// CSS function conditional block representation.
pub struct CSS_CSSFunctionConditionNode {
    media: CSS_CSSMedia? = null,
    containerQueries: CSS_CSSContainerQuery? = null,
    supports: CSS_CSSSupports? = null,
    navigation: CSS_CSSNavigation? = null,
    children: []const CSS_CSSFunctionNode = &.{},
    conditionText: []const u8 = "",
}
/// Section of the body of a CSS function rule.
pub struct CSS_CSSFunctionNode {
    condition: CSS_CSSFunctionConditionNode? = null,
    style: CSS_CSSStyle? = null,
}
/// CSS function at-rule representation.
pub struct CSS_CSSFunctionRule {
    name: CSS_Value = .{},
    styleSheetId: DOM_StyleSheetId? = null,
    origin: CSS_StyleSheetOrigin = "",
    parameters: []const CSS_CSSFunctionParameter = &.{},
    children: []const CSS_CSSFunctionNode = &.{},
    originTreeScopeNodeId: DOM_BackendNodeId? = null,
}
/// CSS keyframe rule representation.
pub struct CSS_CSSKeyframeRule {
    styleSheetId: DOM_StyleSheetId? = null,
    origin: CSS_StyleSheetOrigin = "",
    keyText: CSS_Value = .{},
    style: CSS_CSSStyle = .{},
}
/// A descriptor of operation to mutate style declaration text.
pub struct CSS_StyleDeclarationEdit {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    text: []const u8 = "",
}
/// Fires whenever a web font is updated.  A non-empty font parameter indicates a successfully loaded
pub struct CSS_fontsUpdatedPayload {
    font: CSS_FontFace? = null,
}
/// Fires whenever a MediaQuery result changes (for example, after a browser window has been
pub type CSS_mediaQueryResultChangedPayload = ProtocolJsonValue;
/// Fired whenever an active document stylesheet is added.
pub struct CSS_styleSheetAddedPayload {
    header: CSS_CSSStyleSheetHeader = .{},
}
/// Fired whenever a stylesheet is changed as a result of the client operation.
pub struct CSS_styleSheetChangedPayload {
    styleSheetId: DOM_StyleSheetId = "",
}
/// Fired whenever an active document stylesheet is removed.
pub struct CSS_styleSheetRemovedPayload {
    styleSheetId: DOM_StyleSheetId = "",
}
pub struct CSS_computedStyleUpdatedPayload {
    nodeId: DOM_NodeId = 0.0,
}
/// Inserts a new rule with the given `ruleText` in a stylesheet with given `styleSheetId`, at the
pub struct CSS_addRuleParameters {
    styleSheetId: DOM_StyleSheetId = "",
    ruleText: []const u8 = "",
    location: CSS_SourceRange = .{},
    nodeForPropertySyntaxValidation: DOM_NodeId? = null,
}
pub struct CSS_addRuleReturnValue {
    rule: CSS_CSSRule = .{},
}
/// Returns all class names from specified stylesheet.
pub struct CSS_collectClassNamesParameters {
    styleSheetId: DOM_StyleSheetId = "",
}
pub struct CSS_collectClassNamesReturnValue {
    classNames: []const []const u8 = &.{},
}
/// Creates a new special "via-inspector" stylesheet in the frame with given `frameId`.
pub struct CSS_createStyleSheetParameters {
    frameId: Page_FrameId = "",
    force: bool? = null,
}
pub struct CSS_createStyleSheetReturnValue {
    styleSheetId: DOM_StyleSheetId = "",
}
/// Disables the CSS agent for the given page.
pub struct CSS_disableParameters {}
pub struct CSS_disableReturnValue {}
/// Enables the CSS agent for the given page. Clients should not assume that the CSS agent has been
pub struct CSS_enableParameters {}
pub struct CSS_enableReturnValue {}
/// Ensures that the given node will have specified pseudo-classes whenever its style is computed by
pub struct CSS_forcePseudoStateParameters {
    nodeId: DOM_NodeId = 0.0,
    forcedPseudoClasses: []const []const u8 = &.{},
}
pub struct CSS_forcePseudoStateReturnValue {}
/// Ensures that the given node is in its starting-style state.
pub struct CSS_forceStartingStyleParameters {
    nodeId: DOM_NodeId = 0.0,
    forced: bool = false,
}
pub struct CSS_forceStartingStyleReturnValue {}
/// Forces a position-try option for the given node.
pub struct CSS_forcePositionTryOptionParameters {
    nodeId: DOM_NodeId = 0.0,
    index: f64? = null,
}
pub struct CSS_forcePositionTryOptionReturnValue {}
pub struct CSS_getBackgroundColorsParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getBackgroundColorsReturnValue {
    backgroundColors: ([]const []const u8)? = null,
    computedFontSize: ([]const u8)? = null,
    computedFontWeight: ([]const u8)? = null,
}
/// Returns the computed style for a DOM node identified by `nodeId`.
pub struct CSS_getComputedStyleForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getComputedStyleForNodeReturnValue {
    computedStyle: []const CSS_CSSComputedStyleProperty = &.{},
    extraFields: CSS_ComputedStyleExtraFields = .{},
}
/// Resolve the specified values in the context of the provided element.
pub struct CSS_resolveValuesParameters {
    values: []const []const u8 = &.{},
    nodeId: DOM_NodeId = 0.0,
    propertyName: ([]const u8)? = null,
    pseudoType: DOM_PseudoType? = null,
    pseudoIdentifier: ([]const u8)? = null,
}
pub struct CSS_resolveValuesReturnValue {
    results: []const []const u8 = &.{},
}
pub struct CSS_getLonghandPropertiesParameters {
    shorthandName: []const u8 = "",
    value: []const u8 = "",
}
pub struct CSS_getLonghandPropertiesReturnValue {
    longhandProperties: []const CSS_CSSProperty = &.{},
}
/// Returns the styles defined inline (explicitly in the "style" attribute and implicitly, using DOM
pub struct CSS_getInlineStylesForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getInlineStylesForNodeReturnValue {
    inlineStyle: CSS_CSSStyle? = null,
    attributesStyle: CSS_CSSStyle? = null,
}
/// Returns the styles coming from animations & transitions
pub struct CSS_getAnimatedStylesForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getAnimatedStylesForNodeReturnValue {
    animationStyles: ([]const CSS_CSSAnimationStyle)? = null,
    transitionsStyle: CSS_CSSStyle? = null,
    inherited: ([]const CSS_InheritedAnimatedStyleEntry)? = null,
}
/// Returns requested styles for a DOM node identified by `nodeId`.
pub struct CSS_getMatchedStylesForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getMatchedStylesForNodeReturnValue {
    inlineStyle: CSS_CSSStyle? = null,
    attributesStyle: CSS_CSSStyle? = null,
    matchedCSSRules: ([]const CSS_RuleMatch)? = null,
    pseudoElements: ([]const CSS_PseudoElementMatches)? = null,
    inherited: ([]const CSS_InheritedStyleEntry)? = null,
    inheritedPseudoElements: ([]const CSS_InheritedPseudoElementMatches)? = null,
    cssKeyframesRules: ([]const CSS_CSSKeyframesRule)? = null,
    cssPositionTryRules: ([]const CSS_CSSPositionTryRule)? = null,
    activePositionFallbackIndex: f64? = null,
    cssPropertyRules: ([]const CSS_CSSPropertyRule)? = null,
    cssPropertyRegistrations: ([]const CSS_CSSPropertyRegistration)? = null,
    cssAtRules: ([]const CSS_CSSAtRule)? = null,
    parentLayoutNodeId: DOM_NodeId? = null,
    cssFunctionRules: ([]const CSS_CSSFunctionRule)? = null,
}
/// Returns the values of the default UA-defined environment variables used in env()
pub struct CSS_getEnvironmentVariablesParameters {}
pub struct CSS_getEnvironmentVariablesReturnValue {
    environmentVariables: ProtocolStringMap = "",
}
/// Returns all media queries parsed by the rendering engine.
pub struct CSS_getMediaQueriesParameters {}
pub struct CSS_getMediaQueriesReturnValue {
    medias: []const CSS_CSSMedia = &.{},
}
/// Requests information about platform fonts which we used to render child TextNodes in the given
pub struct CSS_getPlatformFontsForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getPlatformFontsForNodeReturnValue {
    fonts: []const CSS_PlatformFontUsage = &.{},
}
/// Returns the current textual content for a stylesheet.
pub struct CSS_getStyleSheetTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
}
pub struct CSS_getStyleSheetTextReturnValue {
    text: []const u8 = "",
}
/// Returns all layers parsed by the rendering engine for the tree scope of a node.
pub struct CSS_getLayersForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getLayersForNodeReturnValue {
    rootLayer: CSS_CSSLayerData = .{},
}
/// Given a CSS selector text and a style sheet ID, getLocationForSelector
pub struct CSS_getLocationForSelectorParameters {
    styleSheetId: DOM_StyleSheetId = "",
    selectorText: []const u8 = "",
}
pub struct CSS_getLocationForSelectorReturnValue {
    ranges: []const CSS_SourceRange = &.{},
}
/// Starts tracking the given node for the computed style updates
pub struct CSS_trackComputedStyleUpdatesForNodeParameters {
    nodeId: DOM_NodeId? = null,
}
pub struct CSS_trackComputedStyleUpdatesForNodeReturnValue {}
/// Starts tracking the given computed styles for updates. The specified array of properties
pub struct CSS_trackComputedStyleUpdatesParameters {
    propertiesToTrack: []const CSS_CSSComputedStyleProperty = &.{},
}
pub struct CSS_trackComputedStyleUpdatesReturnValue {}
/// Polls the next batch of computed style updates.
pub struct CSS_takeComputedStyleUpdatesParameters {}
pub struct CSS_takeComputedStyleUpdatesReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Find a rule with the given active property for the given node and set the new value for this
pub struct CSS_setEffectivePropertyValueForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
    propertyName: []const u8 = "",
    value: []const u8 = "",
}
pub struct CSS_setEffectivePropertyValueForNodeReturnValue {}
/// Modifies the property rule property name.
pub struct CSS_setPropertyRulePropertyNameParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    propertyName: []const u8 = "",
}
pub struct CSS_setPropertyRulePropertyNameReturnValue {
    propertyName: CSS_Value = .{},
}
/// Modifies the keyframe rule key text.
pub struct CSS_setKeyframeKeyParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    keyText: []const u8 = "",
}
pub struct CSS_setKeyframeKeyReturnValue {
    keyText: CSS_Value = .{},
}
/// Modifies the rule selector.
pub struct CSS_setMediaTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    text: []const u8 = "",
}
pub struct CSS_setMediaTextReturnValue {
    media: CSS_CSSMedia = .{},
}
/// Modifies the expression of a container query.
pub struct CSS_setContainerQueryTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    text: []const u8 = "",
}
pub struct CSS_setContainerQueryTextReturnValue {
    containerQuery: CSS_CSSContainerQuery = .{},
}
pub struct CSS_setContainerQueryConditionTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    text: []const u8 = "",
}
pub struct CSS_setContainerQueryConditionTextReturnValue {
    containerQuery: CSS_CSSContainerQuery = .{},
}
/// Modifies the expression of a supports at-rule.
pub struct CSS_setSupportsTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    text: []const u8 = "",
}
pub struct CSS_setSupportsTextReturnValue {
    supports: CSS_CSSSupports = .{},
}
/// Modifies the expression of a navigation at-rule.
pub struct CSS_setNavigationTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    text: []const u8 = "",
}
pub struct CSS_setNavigationTextReturnValue {
    navigation: CSS_CSSNavigation = .{},
}
/// Modifies the expression of a scope at-rule.
pub struct CSS_setScopeTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    text: []const u8 = "",
}
pub struct CSS_setScopeTextReturnValue {
    scope: CSS_CSSScope = .{},
}
/// Modifies the rule selector.
pub struct CSS_setRuleSelectorParameters {
    styleSheetId: DOM_StyleSheetId = "",
    range: CSS_SourceRange = .{},
    selector: []const u8 = "",
}
pub struct CSS_setRuleSelectorReturnValue {
    selectorList: CSS_SelectorList = .{},
}
/// Sets the new stylesheet text.
pub struct CSS_setStyleSheetTextParameters {
    styleSheetId: DOM_StyleSheetId = "",
    text: []const u8 = "",
}
pub struct CSS_setStyleSheetTextReturnValue {
    sourceMapURL: ([]const u8)? = null,
}
/// Applies specified style edits one after another in the given order.
pub struct CSS_setStyleTextsParameters {
    edits: []const CSS_StyleDeclarationEdit = &.{},
    nodeForPropertySyntaxValidation: DOM_NodeId? = null,
}
pub struct CSS_setStyleTextsReturnValue {
    styles: []const CSS_CSSStyle = &.{},
}
/// Enables the selector recording.
pub struct CSS_startRuleUsageTrackingParameters {}
pub struct CSS_startRuleUsageTrackingReturnValue {}
/// Stop tracking rule usage and return the list of rules that were used since last call to
pub struct CSS_stopRuleUsageTrackingParameters {}
pub struct CSS_stopRuleUsageTrackingReturnValue {
    ruleUsage: []const CSS_RuleUsage = &.{},
}
/// Obtain list of rules that became used since last call to this method (or since start of coverage
pub struct CSS_takeCoverageDeltaParameters {}
pub struct CSS_takeCoverageDeltaReturnValue {
    coverage: []const CSS_RuleUsage = &.{},
    timestamp: f64 = 0.0,
}
/// Enables/disables rendering of local CSS fonts (enabled by default).
pub struct CSS_setLocalFontsEnabledParameters {
    enabled: bool = false,
}
pub struct CSS_setLocalFontsEnabledReturnValue {}

// ============================================================================
// Domain: CacheStorage
// ============================================================================

/// Unique identifier of the Cache object.
pub type CacheStorage_CacheId = []const u8;
/// type of HTTP response cached
/// enum: "basic"|"cors"|"default"|"error"|"opaqueResponse"|"opaqueRedirect"
pub type CacheStorage_CachedResponseType = []const u8;
/// Data entry.
pub struct CacheStorage_DataEntry {
    requestURL: []const u8 = "",
    requestMethod: []const u8 = "",
    requestHeaders: []const CacheStorage_Header = &.{},
    responseTime: f64 = 0.0,
    responseStatus: f64 = 0.0,
    responseStatusText: []const u8 = "",
    responseType: CacheStorage_CachedResponseType = "",
    responseHeaders: []const CacheStorage_Header = &.{},
}
/// Cache identifier.
pub struct CacheStorage_Cache {
    cacheId: CacheStorage_CacheId = "",
    securityOrigin: []const u8 = "",
    storageKey: []const u8 = "",
    storageBucket: Storage_StorageBucket? = null,
    cacheName: []const u8 = "",
}
pub struct CacheStorage_Header {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Cached response
pub struct CacheStorage_CachedResponse {
    body: ProtocolBinary = "",
}
/// Deletes a cache.
pub struct CacheStorage_deleteCacheParameters {
    cacheId: CacheStorage_CacheId = "",
}
pub struct CacheStorage_deleteCacheReturnValue {}
/// Deletes a cache entry.
pub struct CacheStorage_deleteEntryParameters {
    cacheId: CacheStorage_CacheId = "",
    request: []const u8 = "",
}
pub struct CacheStorage_deleteEntryReturnValue {}
/// Requests cache names.
pub struct CacheStorage_requestCacheNamesParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
}
pub struct CacheStorage_requestCacheNamesReturnValue {
    caches: []const CacheStorage_Cache = &.{},
}
/// Fetches cache entry.
pub struct CacheStorage_requestCachedResponseParameters {
    cacheId: CacheStorage_CacheId = "",
    requestURL: []const u8 = "",
    requestHeaders: []const CacheStorage_Header = &.{},
}
pub struct CacheStorage_requestCachedResponseReturnValue {
    response: CacheStorage_CachedResponse = .{},
}
/// Requests data from cache.
pub struct CacheStorage_requestEntriesParameters {
    cacheId: CacheStorage_CacheId = "",
    skipCount: f64? = null,
    pageSize: f64? = null,
    pathFilter: ([]const u8)? = null,
}
pub struct CacheStorage_requestEntriesReturnValue {
    cacheDataEntries: []const CacheStorage_DataEntry = &.{},
    returnCount: f64 = 0.0,
}
