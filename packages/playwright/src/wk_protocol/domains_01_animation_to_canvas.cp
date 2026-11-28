// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue } from "./common";
import { Console_StackTrace, DOM_NodeId, DOM_Styleable } from "./domains_02_console_to_dom";
import { GenericTypes_Size } from "./domains_03_domdebugger_to_inspector";
import { Network_FrameId } from "./domains_04_layertree_to_network";
import { Recording_Frame, Recording_Initiator, Recording_Recording } from "./domains_05_page_to_recording";
import { Runtime_ExecutionContextId, Runtime_RemoteObject } from "./domains_06_runtime_to_worker";

// ============================================================================
// Domain: Animation
// Domain for tracking/modifying Web Animations, as well as CSS (declarative) animations and transitions.
// ============================================================================

/// Unique Web Animation identifier.
pub type Animation_AnimationId = []const u8;
/// enum: "ready"|"delayed"|"active"|"canceled"|"done"
pub type Animation_AnimationState = []const u8;
/// enum: "normal"|"reverse"|"alternate"|"alternate-reverse"
pub type Animation_PlaybackDirection = []const u8;
/// enum: "none"|"forwards"|"backwards"|"both"|"auto"
pub type Animation_FillMode = []const u8;
pub struct Animation_Animation {
    animationId: Animation_AnimationId = "",
    name: ([]const u8)? = null,
    cssAnimationName: ([]const u8)? = null,
    cssTransitionProperty: ([]const u8)? = null,
    stackTrace: Console_StackTrace? = null,
}
pub struct Animation_Effect {
    startDelay: f64? = null,
    endDelay: f64? = null,
    iterationCount: f64? = null,
    iterationStart: f64? = null,
    iterationDuration: f64? = null,
    timingFunction: ([]const u8)? = null,
    playbackDirection: Animation_PlaybackDirection? = null,
    fillMode: Animation_FillMode? = null,
    keyframes: ([]const Animation_Keyframe)? = null,
}
pub struct Animation_Keyframe {
    offset: f64 = 0.0,
    easing: ([]const u8)? = null,
    style: ([]const u8)? = null,
}
pub struct Animation_TrackingUpdate {
    trackingAnimationId: Animation_AnimationId = "",
    animationState: Animation_AnimationState = "",
    nodeId: DOM_NodeId? = null,
    animationName: ([]const u8)? = null,
    transitionProperty: ([]const u8)? = null,
}
/// Dispatched whenever a `WebAnimation` is created.
pub struct Animation_animationCreatedPayload {
    animation: Animation_Animation = .{},
}
/// Dispatched whenever `Animation.prototype.set id` is called.
pub struct Animation_nameChangedPayload {
    animationId: Animation_AnimationId = "",
    name: ([]const u8)? = null,
}
/// Dispatched whenever the effect of any animation is changed in any way.
pub struct Animation_effectChangedPayload {
    animationId: Animation_AnimationId = "",
}
/// Dispatched whenever the target of any effect of any animation is changed in any way.
pub struct Animation_targetChangedPayload {
    animationId: Animation_AnimationId = "",
}
/// Dispatched whenever a `WebAnimation` is destroyed.
pub struct Animation_animationDestroyedPayload {
    animationId: Animation_AnimationId = "",
}
/// Dispatched after `startTracking` command.
pub struct Animation_trackingStartPayload {
    timestamp: f64 = 0.0,
}
/// Fired for each phase of Web Animation.
pub struct Animation_trackingUpdatePayload {
    timestamp: f64 = 0.0,
    event: Animation_TrackingUpdate = .{},
}
/// Dispatched after `stopTracking` command.
pub struct Animation_trackingCompletePayload {
    timestamp: f64 = 0.0,
}
/// Enables Canvas domain events.
pub struct Animation_enableParameters {}
pub struct Animation_enableReturnValue {}
/// Disables Canvas domain events.
pub struct Animation_disableParameters {}
pub struct Animation_disableReturnValue {}
/// Gets the `Effect` for the animation with the given `AnimationId`.
pub struct Animation_requestEffectParameters {
    animationId: Animation_AnimationId = "",
}
pub struct Animation_requestEffectReturnValue {
    effect: Animation_Effect? = null,
}
/// Gets the `DOM.NodeId` for the target of the effect of the animation with the given `AnimationId`.
pub struct Animation_requestEffectTargetParameters {
    animationId: Animation_AnimationId = "",
}
pub struct Animation_requestEffectTargetReturnValue {
    effectTarget: DOM_Styleable = .{},
}
/// Resolves JavaScript `WebAnimation` object for given `AnimationId`.
pub struct Animation_resolveAnimationParameters {
    animationId: Animation_AnimationId = "",
    objectGroup: ([]const u8)? = null,
}
pub struct Animation_resolveAnimationReturnValue {
    object: Runtime_RemoteObject = .{},
}
/// Start tracking animations. This will produce a `trackingStart` event.
pub struct Animation_startTrackingParameters {}
pub struct Animation_startTrackingReturnValue {}
/// Stop tracking animations. This will produce a `trackingComplete` event.
pub struct Animation_stopTrackingParameters {}
pub struct Animation_stopTrackingReturnValue {}

// ============================================================================
// Domain: Audit
// ============================================================================

/// Creates the `WebInspectorAudit` object that is passed to run. Must call teardown before calling setup more than once.
pub struct Audit_setupParameters {
    contextId: Runtime_ExecutionContextId? = null,
}
pub struct Audit_setupReturnValue {}
/// Parses and evaluates the given test string and sends back the result. Returned values are saved to the "audit" object group. Call setup before and teardown after if the `WebInspectorAudit` object should be passed into the test.
pub struct Audit_runParameters {
    test_: []const u8 = "", // protocol field: "test"
    contextId: Runtime_ExecutionContextId? = null,
}
pub struct Audit_runReturnValue {
    result: Runtime_RemoteObject = .{},
    wasThrown: bool? = null,
}
/// Destroys the `WebInspectorAudit` object that is passed to run. Must call setup before calling teardown.
pub struct Audit_teardownParameters {}
pub struct Audit_teardownReturnValue {}

// ============================================================================
// Domain: Browser
// The Browser domain contains commands and events related to getting information about the browser
// ============================================================================

/// Unique extension identifier.
pub type Browser_ExtensionId = []const u8;
/// Information about an extension.
pub struct Browser_Extension {
    extensionId: Browser_ExtensionId = "",
    name: []const u8 = "",
}
pub struct Browser_extensionsEnabledPayload {
    extensions: []const Browser_Extension = &.{},
}
pub struct Browser_extensionsDisabledPayload {
    extensionIds: []const Browser_ExtensionId = &.{},
}
/// Enables Browser domain events.
pub struct Browser_enableParameters {}
pub struct Browser_enableReturnValue {}
/// Disables Browser domain events.
pub struct Browser_disableParameters {}
pub struct Browser_disableReturnValue {}

// ============================================================================
// Domain: CPUProfiler
// CPUProfiler domain exposes cpu usage tracking. On a frame target, the values cover the whole process that hosts the frame, and every target in one process reports the same numbers.
// ============================================================================

/// CPU usage for an individual thread.
pub struct CPUProfiler_ThreadInfo {
    name: []const u8 = "",
    usage: f64 = 0.0,
    type_: ([]const u8)? = null, // protocol field: "type"; enum: "main"|"webkit"
    targetId: ([]const u8)? = null,
}
pub struct CPUProfiler_Event {
    timestamp: f64 = 0.0,
    usage: f64 = 0.0,
    threads: ([]const CPUProfiler_ThreadInfo)? = null,
}
/// Tracking started.
pub struct CPUProfiler_trackingStartPayload {
    timestamp: f64 = 0.0,
}
/// Periodic tracking updates with event data.
pub struct CPUProfiler_trackingUpdatePayload {
    event: CPUProfiler_Event = .{},
}
/// Tracking stopped.
pub struct CPUProfiler_trackingCompletePayload {
    timestamp: f64 = 0.0,
}
/// Start tracking cpu usage.
pub struct CPUProfiler_startTrackingParameters {}
pub struct CPUProfiler_startTrackingReturnValue {}
/// Stop tracking cpu usage. This will produce a `trackingComplete` event.
pub struct CPUProfiler_stopTrackingParameters {}
pub struct CPUProfiler_stopTrackingReturnValue {}

// ============================================================================
// Domain: CSS
// This domain exposes CSS read/write operations. All CSS objects, like stylesheets, rules, and styles, have an associated <code>id</code> used in subsequent operations on the related object. Each object type has a specific <code>id</code> structure, and those are not interchangeable between objects of different kinds. CSS objects can be loaded using the <code>get*ForNode()</code> calls (which accept a DOM node id). Alternatively, a client can discover all the existing stylesheets with the <code>getAllStyleSheets()</code> method and subsequently load the required stylesheet contents using the <code>getStyleSheet[Text]()</code> methods.
// ============================================================================

pub type CSS_StyleSheetId = []const u8;
/// This object identifies a CSS style in a unique way.
pub struct CSS_CSSStyleId {
    styleSheetId: CSS_StyleSheetId = "",
    ordinal: f64 = 0.0,
}
/// Stylesheet type: "user" for user stylesheets, "user-agent" for user-agent stylesheets, "inspector" for stylesheets created by the inspector (i.e. those holding the "via inspector" rules), "regular" for regular stylesheets.
/// enum: "user"|"user-agent"|"author"|"inspector"
pub type CSS_StyleSheetOrigin = []const u8;
/// This object identifies a CSS rule in a unique way.
pub struct CSS_CSSRuleId {
    styleSheetId: CSS_StyleSheetId = "",
    ordinal: f64 = 0.0,
}
/// Pseudo-style identifier (see <code>enum PseudoId</code> in <code>RenderStyleConstants.h</code>).
/// enum: "first-line"|"first-letter"|"grammar-error"|"highlight"|"marker"|"before"|"after"|"selection"|"backdrop"|"spelling-er...
pub type CSS_PseudoId = []const u8;
/// Pseudo-style identifier (see <code>enum PseudoId</code> in <code>RenderStyleConstants.h</code>).
/// enum: "active"|"focus"|"focus-visible"|"focus-within"|"hover"|"target"|"visited"
pub type CSS_ForceablePseudoClass = []const u8;
/// CSS rule collection for a single pseudo style.
pub struct CSS_PseudoIdMatches {
    pseudoId: CSS_PseudoId = "",
    matches: []const CSS_RuleMatch = &.{},
    keyframes: ([]const CSS_CSSRule)? = null,
}
/// CSS rule collection for a single pseudo style.
pub struct CSS_InheritedStyleEntry {
    inlineStyle: CSS_CSSStyle? = null,
    matchedCSSRules: []const CSS_RuleMatch = &.{},
}
/// Match data for a CSS rule.
pub struct CSS_RuleMatch {
    rule: CSS_CSSRule = .{},
    matchingSelectors: []const f64 = &.{},
}
/// CSS selector.
pub struct CSS_CSSSelector {
    text: []const u8 = "",
    specificity: ([]const f64)? = null,
    dynamic: bool? = null,
}
/// Selector list data.
pub struct CSS_SelectorList {
    selectors: []const CSS_CSSSelector = &.{},
    text: []const u8 = "",
    range: CSS_SourceRange? = null,
}
/// CSS style information for a DOM style attribute.
pub struct CSS_CSSStyleAttribute {
    name: []const u8 = "",
    style: CSS_CSSStyle = .{},
}
/// CSS stylesheet meta-information.
pub struct CSS_CSSStyleSheetHeader {
    styleSheetId: CSS_StyleSheetId = "",
    frameId: Network_FrameId = "",
    sourceURL: []const u8 = "",
    origin: CSS_StyleSheetOrigin = "",
    title: []const u8 = "",
    disabled: bool = false,
    isInline: bool = false,
    startLine: f64 = 0.0,
    startColumn: f64 = 0.0,
}
/// CSS stylesheet contents.
pub struct CSS_CSSStyleSheetBody {
    styleSheetId: CSS_StyleSheetId = "",
    rules: []const CSS_CSSRule = &.{},
    text: ([]const u8)? = null,
}
/// CSS rule representation.
pub struct CSS_CSSRule {
    ruleId: CSS_CSSRuleId? = null,
    selectorList: CSS_SelectorList = .{},
    sourceURL: ([]const u8)? = null,
    sourceLine: f64 = 0.0,
    origin: CSS_StyleSheetOrigin = "",
    style: CSS_CSSStyle = .{},
    groupings: ([]const CSS_Grouping)? = null,
    isImplicitlyNested: bool? = null,
}
/// Text range within a resource.
pub struct CSS_SourceRange {
    startLine: f64 = 0.0,
    startColumn: f64 = 0.0,
    endLine: f64 = 0.0,
    endColumn: f64 = 0.0,
}
pub struct CSS_ShorthandEntry {
    name: []const u8 = "",
    value: []const u8 = "",
}
pub struct CSS_CSSPropertyInfo {
    name: []const u8 = "",
    aliases: ([]const []const u8)? = null,
    longhands: ([]const []const u8)? = null,
    values: ([]const []const u8)? = null,
    inherited: bool? = null,
}
pub struct CSS_CSSComputedStyleProperty {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// CSS style representation.
pub struct CSS_CSSStyle {
    styleId: CSS_CSSStyleId? = null,
    cssProperties: []const CSS_CSSProperty = &.{},
    shorthandEntries: []const CSS_ShorthandEntry = &.{},
    cssText: ([]const u8)? = null,
    range: CSS_SourceRange? = null,
    width: ([]const u8)? = null,
    height: ([]const u8)? = null,
}
/// The property status: "active" if the property is effective in the style, "inactive" if the property is overridden by a same-named property in this style later on, "disabled" if the property is disabled by the user, "style" (implied if absent) if the property is reported by the browser rather than by the CSS source parser.
/// enum: "active"|"inactive"|"disabled"|"style"
pub type CSS_CSSPropertyStatus = []const u8;
/// CSS style effective visual dimensions and source offsets.
pub struct CSS_CSSProperty {
    name: []const u8 = "",
    value: []const u8 = "",
    priority: ([]const u8)? = null,
    implicit: bool? = null,
    text: ([]const u8)? = null,
    parsedOk: bool? = null,
    status: CSS_CSSPropertyStatus? = null,
    range: CSS_SourceRange? = null,
}
/// CSS @media (as well as other users of media queries, like @import, <style>, <link>, etc.), @supports, and @layer descriptor.
pub struct CSS_Grouping {
    type_: []const u8 = "", // protocol field: "type"; enum: "media-rule"|"media-import-rule"|"media-link-node"|"media-style-node"|"supports-rule"|"layer-rule"|"layer-import-rule...
    ruleId: CSS_CSSRuleId? = null,
    text: ([]const u8)? = null,
    sourceURL: ([]const u8)? = null,
    range: CSS_SourceRange? = null,
}
/// An inclusive range of Unicode scalar values.
pub struct CSS_CharacterRange {
    start: f64 = 0.0,
    end: f64 = 0.0,
}
/// A representation of WebCore::Font. Conceptually this is backed by either a font file on disk or from the network.
pub struct CSS_Font {
    displayName: []const u8 = "",
    url: ([]const u8)? = null,
    variationAxes: ([]const CSS_FontVariationAxis)? = null,
    synthesizedBold: bool? = null,
    synthesizedOblique: bool? = null,
    characterRanges: ([]const CSS_CharacterRange)? = null,
}
/// A single variation axis associated with a Font.
pub struct CSS_FontVariationAxis {
    name: ([]const u8)? = null,
    tag: []const u8 = "",
    minimumValue: f64 = 0.0,
    maximumValue: f64 = 0.0,
    defaultValue: f64 = 0.0,
}
/// Relevant layout information about the node. Things not in this list are not important to Web Inspector.
/// enum: "rendered"|"scrollable"|"flex"|"grid"|"subgrid"|"grid-lanes"|"event"|"slot-assigned"|"slot-filled"
pub type CSS_LayoutFlag = []const u8;
/// The mode for how layout context type changes are handled (default: <code>Observed</code>). <code>Observed</code> limits handling to those nodes already known to the frontend by other means (generally, this means the node is a visible item in the Elements tab). <code>All</code> informs the frontend of all layout context type changes and all nodes with a known layout context are sent to the frontend.
/// enum: "observed"|"all"
pub type CSS_LayoutContextTypeChangedMode = []const u8;
/// Fired when font data previously returned by this target may have changed.
pub type CSS_fontDataChangedPayload = ProtocolJsonValue;
/// Fires whenever a MediaQuery result changes (for example, after a browser window has been resized.) The current implementation considers only viewport-dependent media features.
pub type CSS_mediaQueryResultChangedPayload = ProtocolJsonValue;
/// Fired whenever a stylesheet is changed as a result of the client operation.
pub struct CSS_styleSheetChangedPayload {
    styleSheetId: CSS_StyleSheetId = "",
}
/// Fired whenever an active document stylesheet is added.
pub struct CSS_styleSheetAddedPayload {
    header: CSS_CSSStyleSheetHeader = .{},
}
/// Fired whenever an active document stylesheet is removed.
pub struct CSS_styleSheetRemovedPayload {
    styleSheetId: CSS_StyleSheetId = "",
}
/// Called when the layout of a node changes in a way that is important to Web Inspector.
pub struct CSS_nodeLayoutFlagsChangedPayload {
    nodeId: DOM_NodeId = 0.0,
    layoutFlags: ([]const []const u8)? = null,
}
/// Enables the CSS agent for the given page. Clients should not assume that the CSS agent has been enabled until the result of this command is received.
pub struct CSS_enableParameters {}
pub struct CSS_enableReturnValue {}
/// Disables the CSS agent for the given page.
pub struct CSS_disableParameters {}
pub struct CSS_disableReturnValue {}
/// Returns requested styles for a DOM node identified by <code>nodeId</code>.
pub struct CSS_getMatchedStylesForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
    includePseudo: bool? = null,
    includeInherited: bool? = null,
    includeKeyframes: bool? = null,
}
pub struct CSS_getMatchedStylesForNodeReturnValue {
    matchedCSSRules: ([]const CSS_RuleMatch)? = null,
    pseudoElements: ([]const CSS_PseudoIdMatches)? = null,
    inherited: ([]const CSS_InheritedStyleEntry)? = null,
    keyframes: ([]const CSS_CSSRule)? = null,
}
/// Returns the styles defined inline (explicitly in the "style" attribute and implicitly, using DOM attributes) for a DOM node identified by <code>nodeId</code>.
pub struct CSS_getInlineStylesForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getInlineStylesForNodeReturnValue {
    inlineStyle: CSS_CSSStyle? = null,
    attributesStyle: CSS_CSSStyle? = null,
}
/// Returns the computed style for a DOM node identified by <code>nodeId</code>.
pub struct CSS_getComputedStyleForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getComputedStyleForNodeReturnValue {
    computedStyle: []const CSS_CSSComputedStyleProperty = &.{},
}
/// Returns the primary font of the computed font cascade for a DOM node identified by <code>nodeId</code> and the fonts used to render text in its subtree.
pub struct CSS_getFontDataForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct CSS_getFontDataForNodeReturnValue {
    primaryFont: CSS_Font = .{},
    renderedFonts: ([]const CSS_Font)? = null,
    missingCharacterRanges: ([]const CSS_CharacterRange)? = null,
}
/// Returns metainfo entries for all known stylesheets.
pub struct CSS_getAllStyleSheetsParameters {}
pub struct CSS_getAllStyleSheetsReturnValue {
    headers: []const CSS_CSSStyleSheetHeader = &.{},
}
/// Returns stylesheet data for the specified <code>styleSheetId</code>.
pub struct CSS_getStyleSheetParameters {
    styleSheetId: CSS_StyleSheetId = "",
}
pub struct CSS_getStyleSheetReturnValue {
    styleSheet: CSS_CSSStyleSheetBody = .{},
}
/// Returns the current textual content and the URL for a stylesheet.
pub struct CSS_getStyleSheetTextParameters {
    styleSheetId: CSS_StyleSheetId = "",
}
pub struct CSS_getStyleSheetTextReturnValue {
    text: []const u8 = "",
}
/// Sets the new stylesheet text, thereby invalidating all existing <code>CSSStyleId</code>'s and <code>CSSRuleId</code>'s contained by this stylesheet.
pub struct CSS_setStyleSheetTextParameters {
    styleSheetId: CSS_StyleSheetId = "",
    text: []const u8 = "",
}
pub struct CSS_setStyleSheetTextReturnValue {}
/// Sets the new <code>text</code> for the respective style.
pub struct CSS_setStyleTextParameters {
    styleId: CSS_CSSStyleId = .{},
    text: []const u8 = "",
}
pub struct CSS_setStyleTextReturnValue {
    style: CSS_CSSStyle = .{},
}
/// Modifies the rule selector.
pub struct CSS_setRuleSelectorParameters {
    ruleId: CSS_CSSRuleId = .{},
    selector: []const u8 = "",
}
pub struct CSS_setRuleSelectorReturnValue {
    rule: CSS_CSSRule = .{},
}
/// Modifies an @rule grouping's header text.
pub struct CSS_setGroupingHeaderTextParameters {
    ruleId: CSS_CSSRuleId = .{},
    headerText: []const u8 = "",
}
pub struct CSS_setGroupingHeaderTextReturnValue {
    grouping: CSS_Grouping = .{},
}
/// Creates a new special "inspector" stylesheet in the frame with given <code>frameId</code>.
pub struct CSS_createStyleSheetParameters {
    frameId: Network_FrameId = "",
}
pub struct CSS_createStyleSheetReturnValue {
    styleSheetId: CSS_StyleSheetId = "",
}
/// Creates a new empty rule with the given <code>selector</code> in a stylesheet with given <code>styleSheetId</code>.
pub struct CSS_addRuleParameters {
    styleSheetId: CSS_StyleSheetId = "",
    selector: []const u8 = "",
}
pub struct CSS_addRuleReturnValue {
    rule: CSS_CSSRule = .{},
}
/// Returns all supported CSS property names.
pub struct CSS_getSupportedCSSPropertiesParameters {}
pub struct CSS_getSupportedCSSPropertiesReturnValue {
    cssProperties: []const CSS_CSSPropertyInfo = &.{},
}
/// Returns all supported system font family names.
pub struct CSS_getSupportedSystemFontFamilyNamesParameters {}
pub struct CSS_getSupportedSystemFontFamilyNamesReturnValue {
    fontFamilyNames: []const []const u8 = &.{},
}
/// Ensures that the given node will have specified pseudo-classes whenever its style is computed by the browser.
pub struct CSS_forcePseudoStateParameters {
    nodeId: DOM_NodeId = 0.0,
    forcedPseudoClasses: []const CSS_ForceablePseudoClass = &.{},
}
pub struct CSS_forcePseudoStateReturnValue {}
/// Change how layout context type changes are handled for nodes. When the new mode would observe nodes the frontend has not yet recieved, those nodes will be sent to the frontend immediately.
pub struct CSS_setLayoutContextTypeChangedModeParameters {
    mode: CSS_LayoutContextTypeChangedMode = "",
}
pub struct CSS_setLayoutContextTypeChangedModeReturnValue {}

// ============================================================================
// Domain: Canvas
// Canvas domain allows tracking of canvases that have an associated graphics context. Tracks canvases in the DOM and CSS canvases created with -webkit-canvas.
// ============================================================================

/// Unique canvas identifier.
pub type Canvas_CanvasId = []const u8;
/// Unique shader program identifier.
pub type Canvas_ProgramId = []const u8;
/// enum: "srgb"|"srgb-linear"|"display-p3"|"display-p3-linear"
pub type Canvas_ColorSpace = []const u8;
/// The type of rendering context backing the canvas element.
/// enum: "canvas-2d"|"offscreen-canvas-2d"|"bitmaprenderer"|"offscreen-bitmaprenderer"|"webgl"|"offscreen-webgl"|"webgl2"|"off...
pub type Canvas_ContextType = []const u8;
/// enum: "compute"|"render"|"vertex"
pub type Canvas_ProgramType = []const u8;
/// enum: "compute"|"fragment"|"vertex"
pub type Canvas_ShaderType = []const u8;
/// Drawing surface attributes.
pub struct Canvas_ContextAttributes {
    alpha: bool? = null,
    colorSpace: Canvas_ColorSpace? = null,
    desynchronized: bool? = null,
    willReadFrequently: bool? = null,
    depth: bool? = null,
    stencil: bool? = null,
    antialias: bool? = null,
    premultipliedAlpha: bool? = null,
    preserveDrawingBuffer: bool? = null,
    failIfMajorPerformanceCaveat: bool? = null,
    powerPreference: ([]const u8)? = null,
}
/// Information about a canvas for which a rendering context has been created.
pub struct Canvas_Canvas {
    canvasId: Canvas_CanvasId = "",
    contextType: Canvas_ContextType = "",
    sizes: ([]const GenericTypes_Size)? = null,
    cssCanvasNames: ([]const []const u8)? = null,
    contextAttributes: Canvas_ContextAttributes? = null,
    features: ([]const []const u8)? = null,
    memoryCost: f64? = null,
    stackTrace: Console_StackTrace? = null,
    name: ([]const u8)? = null,
}
/// Information about a WebGL/WebGL2 shader program or WebGPU shader pipeline.
pub struct Canvas_ShaderProgram {
    programId: Canvas_ProgramId = "",
    programType: Canvas_ProgramType = "",
    canvasId: Canvas_CanvasId = "",
    sharesVertexFragmentShader: bool? = null,
    name: ([]const u8)? = null,
}
pub struct Canvas_canvasAddedPayload {
    canvas: Canvas_Canvas = .{},
}
pub struct Canvas_canvasRemovedPayload {
    canvasId: Canvas_CanvasId = "",
}
pub struct Canvas_canvasNameChangedPayload {
    canvasId: Canvas_CanvasId = "",
    name: []const u8 = "",
}
pub struct Canvas_canvasSizeChangedPayload {
    canvasId: Canvas_CanvasId = "",
    sizes: ([]const GenericTypes_Size)? = null,
}
pub struct Canvas_canvasMemoryChangedPayload {
    canvasId: Canvas_CanvasId = "",
    memoryCost: f64 = 0.0,
}
pub struct Canvas_extensionEnabledPayload {
    canvasId: Canvas_CanvasId = "",
    extension: []const u8 = "",
}
pub struct Canvas_nodesChangedPayload {
    canvasId: Canvas_CanvasId = "",
}
pub struct Canvas_cssCanvasClientNodesChangedPayload {
    canvasId: Canvas_CanvasId = "",
}
pub struct Canvas_cssCanvasNamesChangedPayload {
    canvasId: Canvas_CanvasId = "",
    cssCanvasNames: []const []const u8 = &.{},
}
pub struct Canvas_recordingStartedPayload {
    canvasId: Canvas_CanvasId = "",
    initiator: Recording_Initiator = "",
}
pub struct Canvas_recordingProgressPayload {
    canvasId: Canvas_CanvasId = "",
    frames: []const Recording_Frame = &.{},
    bufferUsed: f64 = 0.0,
}
pub struct Canvas_recordingFinishedPayload {
    canvasId: Canvas_CanvasId = "",
    recording: Recording_Recording? = null,
}
pub struct Canvas_programCreatedPayload {
    shaderProgram: Canvas_ShaderProgram = .{},
}
pub struct Canvas_programDeletedPayload {
    programId: Canvas_ProgramId = "",
}
pub struct Canvas_programNameChangedPayload {
    programId: Canvas_ProgramId = "",
    name: []const u8 = "",
}
/// Enables Canvas domain events.
pub struct Canvas_enableParameters {}
pub struct Canvas_enableReturnValue {}
/// Disables Canvas domain events.
pub struct Canvas_disableParameters {}
pub struct Canvas_disableReturnValue {}
/// Gets the NodeIds for the canvas nodes with the given CanvasId.
pub struct Canvas_requestNodesParameters {
    canvasId: Canvas_CanvasId = "",
}
pub struct Canvas_requestNodesReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Gets the data for the canvas node with the given CanvasId.
pub struct Canvas_requestContentParameters {
    canvasId: Canvas_CanvasId = "",
}
pub struct Canvas_requestContentReturnValue {
    content: []const u8 = "",
}
/// Gets all nodes using a <code>-webkit-canvas</code> image associated with the given CanvasId.
pub struct Canvas_requestCSSCanvasClientNodesParameters {
    canvasId: Canvas_CanvasId = "",
}
pub struct Canvas_requestCSSCanvasClientNodesReturnValue {
    clientNodeIds: []const DOM_NodeId = &.{},
}
/// Resolves JavaScript canvas/device context object for given canvasId.
pub struct Canvas_resolveContextParameters {
    canvasId: Canvas_CanvasId = "",
    objectGroup: ([]const u8)? = null,
}
pub struct Canvas_resolveContextReturnValue {
    object: Runtime_RemoteObject = .{},
}
/// Tells the backend to record `count` frames whenever a new context is created.
pub struct Canvas_setRecordingAutoCaptureFrameCountParameters {
    count: f64 = 0.0,
}
pub struct Canvas_setRecordingAutoCaptureFrameCountReturnValue {}
/// Record the next frame, or up to the given number of bytes of data, for the given canvas.
pub struct Canvas_startRecordingParameters {
    canvasId: Canvas_CanvasId = "",
    frameCount: f64? = null,
    memoryLimit: f64? = null,
}
pub struct Canvas_startRecordingReturnValue {}
/// Stop recording the given canvas.
pub struct Canvas_stopRecordingParameters {
    canvasId: Canvas_CanvasId = "",
}
pub struct Canvas_stopRecordingReturnValue {}
/// Requests the source of the shader of the given type from the program with the given id.
pub struct Canvas_requestShaderSourceParameters {
    programId: Canvas_ProgramId = "",
    shaderType: Canvas_ShaderType = "",
}
pub struct Canvas_requestShaderSourceReturnValue {
    source: []const u8 = "",
}
/// Compiles and links the shader with identifier and type with the given source code.
pub struct Canvas_updateShaderParameters {
    programId: Canvas_ProgramId = "",
    shaderType: Canvas_ShaderType = "",
    source: []const u8 = "",
}
pub struct Canvas_updateShaderReturnValue {}
/// Enable/disable the visibility of the given shader program.
pub struct Canvas_setShaderProgramDisabledParameters {
    programId: Canvas_ProgramId = "",
    disabled: bool = false,
}
pub struct Canvas_setShaderProgramDisabledReturnValue {}
/// Enable/disable highlighting of the given shader program.
pub struct Canvas_setShaderProgramHighlightedParameters {
    programId: Canvas_ProgramId = "",
    highlighted: bool = false,
}
pub struct Canvas_setShaderProgramHighlightedReturnValue {}
