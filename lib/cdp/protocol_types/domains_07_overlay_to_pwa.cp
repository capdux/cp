// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue, ProtocolStringMap } from "./common";
import {
    DOM_BackendNodeId,
    DOM_NodeId,
    DOM_Quad,
    DOM_RGBA,
    DOM_Rect,
} from "./domains_03_cast_to_domsnapshot";
import { Page_FrameId, Page_Viewport } from "./domains_08_page_to_performance";
import { Target_TargetID } from "./domains_10_target_to_console";
import { Runtime_RemoteObjectId } from "./domains_11_debugger_to_schema";

// ============================================================================
// Domain: Overlay
// This domain provides various functionality related to drawing atop the inspected page.
// ============================================================================

/// Configuration data for drawing the source order of an elements children.
pub struct Overlay_SourceOrderConfig {
    parentOutlineColor: DOM_RGBA = .{},
    childOutlineColor: DOM_RGBA = .{},
}
/// Configuration data for the highlighting of Grid elements.
pub struct Overlay_GridHighlightConfig {
    showGridExtensionLines: bool? = null,
    showPositiveLineNumbers: bool? = null,
    showNegativeLineNumbers: bool? = null,
    showAreaNames: bool? = null,
    showLineNames: bool? = null,
    showTrackSizes: bool? = null,
    gridBorderColor: DOM_RGBA? = null,
    cellBorderColor: DOM_RGBA? = null,
    rowLineColor: DOM_RGBA? = null,
    columnLineColor: DOM_RGBA? = null,
    gridBorderDash: bool? = null,
    cellBorderDash: bool? = null,
    rowLineDash: bool? = null,
    columnLineDash: bool? = null,
    rowGapColor: DOM_RGBA? = null,
    rowHatchColor: DOM_RGBA? = null,
    columnGapColor: DOM_RGBA? = null,
    columnHatchColor: DOM_RGBA? = null,
    areaBorderColor: DOM_RGBA? = null,
    gridBackgroundColor: DOM_RGBA? = null,
}
/// Configuration data for the highlighting of Flex container elements.
pub struct Overlay_FlexContainerHighlightConfig {
    containerBorder: Overlay_LineStyle? = null,
    lineSeparator: Overlay_LineStyle? = null,
    itemSeparator: Overlay_LineStyle? = null,
    mainDistributedSpace: Overlay_BoxStyle? = null,
    crossDistributedSpace: Overlay_BoxStyle? = null,
    rowGapSpace: Overlay_BoxStyle? = null,
    columnGapSpace: Overlay_BoxStyle? = null,
    crossAlignment: Overlay_LineStyle? = null,
}
/// Configuration data for the highlighting of Flex item elements.
pub struct Overlay_FlexItemHighlightConfig {
    baseSizeBox: Overlay_BoxStyle? = null,
    baseSizeBorder: Overlay_LineStyle? = null,
    flexibilityArrow: Overlay_LineStyle? = null,
}
/// Style information for drawing a line.
pub struct Overlay_LineStyle {
    color: DOM_RGBA? = null,
    pattern: ([]const u8)? = null, // enum: "dashed"|"dotted"
}
/// Style information for drawing a box.
pub struct Overlay_BoxStyle {
    fillColor: DOM_RGBA? = null,
    hatchColor: DOM_RGBA? = null,
}
/// enum: "aa"|"aaa"|"apca"
pub type Overlay_ContrastAlgorithm = []const u8;
/// Configuration for Inset-Modified Containing Block (IMCB) and CSS Anchor Positioning highlight.
pub struct Overlay_ImcbHighlightConfig {
    imcbBorderColor: DOM_RGBA? = null,
    imcbBackgroundColor: DOM_RGBA? = null,
    insetsBackgroundColor: DOM_RGBA? = null,
    insetsHatchColor: DOM_RGBA? = null,
    anchorBorderColor: DOM_RGBA? = null,
    anchorBackgroundColor: DOM_RGBA? = null,
    showPositionAreaGrid: bool? = null,
    positionAreaGridLineColor: DOM_RGBA? = null,
    positionAreaActiveRegionColor: DOM_RGBA? = null,
}
/// Configuration data for the highlighting of page elements.
pub struct Overlay_HighlightConfig {
    showInfo: bool? = null,
    showStyles: bool? = null,
    showRulers: bool? = null,
    showAccessibilityInfo: bool? = null,
    showExtensionLines: bool? = null,
    contentColor: DOM_RGBA? = null,
    paddingColor: DOM_RGBA? = null,
    borderColor: DOM_RGBA? = null,
    marginColor: DOM_RGBA? = null,
    eventTargetColor: DOM_RGBA? = null,
    shapeColor: DOM_RGBA? = null,
    shapeMarginColor: DOM_RGBA? = null,
    cssGridColor: DOM_RGBA? = null,
    colorFormat: Overlay_ColorFormat? = null,
    gridHighlightConfig: Overlay_GridHighlightConfig? = null,
    flexContainerHighlightConfig: Overlay_FlexContainerHighlightConfig? = null,
    flexItemHighlightConfig: Overlay_FlexItemHighlightConfig? = null,
    contrastAlgorithm: Overlay_ContrastAlgorithm? = null,
    containerQueryContainerHighlightConfig: Overlay_ContainerQueryContainerHighlightConfig? = null,
    imcbHighlightConfig: Overlay_ImcbHighlightConfig? = null,
}
/// enum: "rgb"|"hsl"|"hwb"|"hex"
pub type Overlay_ColorFormat = []const u8;
/// Configurations for Persistent Grid Highlight
pub struct Overlay_GridNodeHighlightConfig {
    gridHighlightConfig: Overlay_GridHighlightConfig = .{},
    nodeId: DOM_NodeId = 0.0,
}
pub struct Overlay_FlexNodeHighlightConfig {
    flexContainerHighlightConfig: Overlay_FlexContainerHighlightConfig = .{},
    nodeId: DOM_NodeId = 0.0,
}
pub struct Overlay_ScrollSnapContainerHighlightConfig {
    snapportBorder: Overlay_LineStyle? = null,
    snapAreaBorder: Overlay_LineStyle? = null,
    scrollMarginColor: DOM_RGBA? = null,
    scrollPaddingColor: DOM_RGBA? = null,
}
pub struct Overlay_ScrollSnapHighlightConfig {
    scrollSnapContainerHighlightConfig: Overlay_ScrollSnapContainerHighlightConfig = .{},
    nodeId: DOM_NodeId = 0.0,
}
/// Configuration for dual screen hinge
pub struct Overlay_HingeConfig {
    rect: DOM_Rect = .{},
    contentColor: DOM_RGBA? = null,
    outlineColor: DOM_RGBA? = null,
}
/// Supported display cutout shapes.
/// enum: "pill"|"notch"|"circle"|"rectangle"
pub type Overlay_DisplayCutoutShape = []const u8;
/// Configuration for a display cutout.
pub struct Overlay_DisplayCutoutConfig {
    rect: DOM_Rect = .{},
    shape: Overlay_DisplayCutoutShape = "",
    borderRadius: f64? = null,
    upperRadius: f64? = null,
    lowerRadius: f64? = null,
    cx: f64? = null,
    cy: f64? = null,
    radius: f64? = null,
    contentColor: DOM_RGBA? = null,
}
/// Configuration for Window Controls Overlay
pub struct Overlay_WindowControlsOverlayConfig {
    showCSS: bool = false,
    selectedPlatform: []const u8 = "",
    themeColor: []const u8 = "",
}
pub struct Overlay_ContainerQueryHighlightConfig {
    containerQueryContainerHighlightConfig: Overlay_ContainerQueryContainerHighlightConfig = .{},
    nodeId: DOM_NodeId = 0.0,
}
pub struct Overlay_ContainerQueryContainerHighlightConfig {
    containerBorder: Overlay_LineStyle? = null,
    descendantBorder: Overlay_LineStyle? = null,
}
pub struct Overlay_IsolatedElementHighlightConfig {
    isolationModeHighlightConfig: Overlay_IsolationModeHighlightConfig = .{},
    nodeId: DOM_NodeId = 0.0,
}
pub struct Overlay_IsolationModeHighlightConfig {
    resizerColor: DOM_RGBA? = null,
    resizerHandleColor: DOM_RGBA? = null,
    maskColor: DOM_RGBA? = null,
}
/// enum: "searchForNode"|"searchForUAShadowDOM"|"captureAreaScreenshot"|"none"
pub type Overlay_InspectMode = []const u8;
pub struct Overlay_InspectedElementAnchorConfig {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
}
/// Fired when the node should be inspected. This happens after call to `setInspectMode` or when
pub struct Overlay_inspectNodeRequestedPayload {
    backendNodeId: DOM_BackendNodeId = 0.0,
}
/// Fired when the node should be highlighted. This happens after call to `setInspectMode`.
pub struct Overlay_nodeHighlightRequestedPayload {
    nodeId: DOM_NodeId = 0.0,
}
/// Fired when user asks to capture screenshot of some area on the page.
pub struct Overlay_screenshotRequestedPayload {
    viewport: Page_Viewport = .{},
}
/// Fired when user asks to show the Inspect panel.
pub struct Overlay_inspectPanelShowRequestedPayload {
    backendNodeId: DOM_BackendNodeId = 0.0,
}
/// Fired when user asks to restore the Inspected Element floating window.
pub struct Overlay_inspectedElementWindowRestoredPayload {
    backendNodeId: DOM_BackendNodeId = 0.0,
}
/// Fired when user cancels the inspect mode.
pub type Overlay_inspectModeCanceledPayload = ProtocolJsonValue;
/// Disables domain notifications.
pub struct Overlay_disableParameters {}
pub struct Overlay_disableReturnValue {}
/// Enables domain notifications.
pub struct Overlay_enableParameters {}
pub struct Overlay_enableReturnValue {}
/// For testing.
pub struct Overlay_getHighlightObjectForTestParameters {
    nodeId: DOM_NodeId = 0.0,
    includeDistance: bool? = null,
    includeStyle: bool? = null,
    colorFormat: Overlay_ColorFormat? = null,
    showAccessibilityInfo: bool? = null,
}
pub struct Overlay_getHighlightObjectForTestReturnValue {
    highlight: ProtocolStringMap = "",
}
/// For Persistent Grid testing.
pub struct Overlay_getGridHighlightObjectsForTestParameters {
    nodeIds: []const DOM_NodeId = &.{},
}
pub struct Overlay_getGridHighlightObjectsForTestReturnValue {
    highlights: ProtocolStringMap = "",
}
/// For Source Order Viewer testing.
pub struct Overlay_getSourceOrderHighlightObjectForTestParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct Overlay_getSourceOrderHighlightObjectForTestReturnValue {
    highlight: ProtocolStringMap = "",
}
/// Hides any highlight.
pub struct Overlay_hideHighlightParameters {}
pub struct Overlay_hideHighlightReturnValue {}
/// Highlights owner element of the frame with given id.
pub struct Overlay_highlightFrameParameters {
    frameId: Page_FrameId = "",
    contentColor: DOM_RGBA? = null,
    contentOutlineColor: DOM_RGBA? = null,
}
pub struct Overlay_highlightFrameReturnValue {}
/// Highlights DOM node with given id or with the given JavaScript object wrapper. Either nodeId or
pub struct Overlay_highlightNodeParameters {
    highlightConfig: Overlay_HighlightConfig = .{},
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    selector: ([]const u8)? = null,
}
pub struct Overlay_highlightNodeReturnValue {}
/// Highlights given quad. Coordinates are absolute with respect to the main frame viewport.
pub struct Overlay_highlightQuadParameters {
    quad: DOM_Quad = &.{},
    color: DOM_RGBA? = null,
    outlineColor: DOM_RGBA? = null,
}
pub struct Overlay_highlightQuadReturnValue {}
/// Highlights given rectangle. Coordinates are absolute with respect to the main frame viewport.
pub struct Overlay_highlightRectParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    color: DOM_RGBA? = null,
    outlineColor: DOM_RGBA? = null,
}
pub struct Overlay_highlightRectReturnValue {}
/// Highlights the source order of the children of the DOM node with given id or with the given
pub struct Overlay_highlightSourceOrderParameters {
    sourceOrderConfig: Overlay_SourceOrderConfig = .{},
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
pub struct Overlay_highlightSourceOrderReturnValue {}
/// Enters the 'inspect' mode. In this mode, elements that user is hovering over are highlighted.
pub struct Overlay_setInspectModeParameters {
    mode: Overlay_InspectMode = "",
    highlightConfig: Overlay_HighlightConfig? = null,
}
pub struct Overlay_setInspectModeReturnValue {}
/// Highlights owner element of all frames detected to be ads.
pub struct Overlay_setShowAdHighlightsParameters {
    show: bool = false,
}
pub struct Overlay_setShowAdHighlightsReturnValue {}
pub struct Overlay_setPausedInDebuggerMessageParameters {
    message: ([]const u8)? = null,
}
pub struct Overlay_setPausedInDebuggerMessageReturnValue {}
/// Requests that backend shows debug borders on layers
pub struct Overlay_setShowDebugBordersParameters {
    show: bool = false,
}
pub struct Overlay_setShowDebugBordersReturnValue {}
/// Requests that backend shows the FPS counter
pub struct Overlay_setShowFPSCounterParameters {
    show: bool = false,
}
pub struct Overlay_setShowFPSCounterReturnValue {}
/// Highlight multiple elements with the CSS Grid overlay.
pub struct Overlay_setShowGridOverlaysParameters {
    gridNodeHighlightConfigs: []const Overlay_GridNodeHighlightConfig = &.{},
}
pub struct Overlay_setShowGridOverlaysReturnValue {}
pub struct Overlay_setShowFlexOverlaysParameters {
    flexNodeHighlightConfigs: []const Overlay_FlexNodeHighlightConfig = &.{},
}
pub struct Overlay_setShowFlexOverlaysReturnValue {}
pub struct Overlay_setShowScrollSnapOverlaysParameters {
    scrollSnapHighlightConfigs: []const Overlay_ScrollSnapHighlightConfig = &.{},
}
pub struct Overlay_setShowScrollSnapOverlaysReturnValue {}
pub struct Overlay_setShowContainerQueryOverlaysParameters {
    containerQueryHighlightConfigs: []const Overlay_ContainerQueryHighlightConfig = &.{},
}
pub struct Overlay_setShowContainerQueryOverlaysReturnValue {}
pub struct Overlay_setShowInspectedElementAnchorParameters {
    inspectedElementAnchorConfig: Overlay_InspectedElementAnchorConfig = .{},
}
pub struct Overlay_setShowInspectedElementAnchorReturnValue {}
/// Requests that backend shows paint rectangles
pub struct Overlay_setShowPaintRectsParameters {
    result: bool = false,
}
pub struct Overlay_setShowPaintRectsReturnValue {}
/// Requests that backend shows layout shift regions
pub struct Overlay_setShowLayoutShiftRegionsParameters {
    result: bool = false,
}
pub struct Overlay_setShowLayoutShiftRegionsReturnValue {}
/// Requests that backend shows scroll bottleneck rects
pub struct Overlay_setShowScrollBottleneckRectsParameters {
    show: bool = false,
}
pub struct Overlay_setShowScrollBottleneckRectsReturnValue {}
/// Deprecated, no longer has any effect.
pub struct Overlay_setShowHitTestBordersParameters {
    show: bool = false,
}
pub struct Overlay_setShowHitTestBordersReturnValue {}
/// Deprecated, no longer has any effect.
pub struct Overlay_setShowWebVitalsParameters {
    show: bool = false,
}
pub struct Overlay_setShowWebVitalsReturnValue {}
/// Paints viewport size upon main frame resize.
pub struct Overlay_setShowViewportSizeOnResizeParameters {
    show: bool = false,
}
pub struct Overlay_setShowViewportSizeOnResizeReturnValue {}
/// Add a dual screen device hinge
pub struct Overlay_setShowHingeParameters {
    hingeConfig: Overlay_HingeConfig? = null,
}
pub struct Overlay_setShowHingeReturnValue {}
/// Add a display cutout overlay.
pub struct Overlay_setShowDisplayCutoutParameters {
    displayCutoutConfig: Overlay_DisplayCutoutConfig? = null,
}
pub struct Overlay_setShowDisplayCutoutReturnValue {}
/// Show elements in isolation mode with overlays.
pub struct Overlay_setShowIsolatedElementsParameters {
    isolatedElementHighlightConfigs: []const Overlay_IsolatedElementHighlightConfig = &.{},
}
pub struct Overlay_setShowIsolatedElementsReturnValue {}
/// Show Window Controls Overlay for PWA
pub struct Overlay_setShowWindowControlsOverlayParameters {
    windowControlsOverlayConfig: Overlay_WindowControlsOverlayConfig? = null,
}
pub struct Overlay_setShowWindowControlsOverlayReturnValue {}

// ============================================================================
// Domain: PWA
// This domain allows interacting with the browser to control PWAs.
// ============================================================================

/// The following types are the replica of
pub struct PWA_FileHandlerAccept {
    mediaType: []const u8 = "",
    fileExtensions: []const []const u8 = &.{},
}
pub struct PWA_FileHandler {
    action: []const u8 = "",
    accepts: []const PWA_FileHandlerAccept = &.{},
    displayName: []const u8 = "",
}
/// If user prefers opening the app in browser or an app window.
/// enum: "standalone"|"browser"
pub type PWA_DisplayMode = []const u8;
/// Returns the following OS state for the given manifest id.
pub struct PWA_getOsAppStateParameters {
    manifestId: []const u8 = "",
}
pub struct PWA_getOsAppStateReturnValue {
    badgeCount: f64 = 0.0,
    fileHandlers: []const PWA_FileHandler = &.{},
}
/// Installs the given manifest identity, optionally using the given installUrlOrBundleUrl
pub struct PWA_installParameters {
    manifestId: []const u8 = "",
    installUrlOrBundleUrl: ([]const u8)? = null,
}
pub struct PWA_installReturnValue {}
/// Uninstalls the given manifest_id and closes any opened app windows.
pub struct PWA_uninstallParameters {
    manifestId: []const u8 = "",
}
pub struct PWA_uninstallReturnValue {}
/// Launches the installed web app, or an url in the same web app instead of the
pub struct PWA_launchParameters {
    manifestId: []const u8 = "",
    url: ([]const u8)? = null,
}
pub struct PWA_launchReturnValue {
    targetId: Target_TargetID = "",
}
/// Opens one or more local files from an installed web app identified by its
pub struct PWA_launchFilesInAppParameters {
    manifestId: []const u8 = "",
    files: []const []const u8 = &.{},
}
pub struct PWA_launchFilesInAppReturnValue {
    targetIds: []const Target_TargetID = &.{},
}
/// Opens the current page in its web app identified by the manifest id, needs
pub struct PWA_openCurrentPageInAppParameters {
    manifestId: []const u8 = "",
}
pub struct PWA_openCurrentPageInAppReturnValue {}
/// Changes user settings of the web app identified by its manifestId. If the
pub struct PWA_changeAppUserSettingsParameters {
    manifestId: []const u8 = "",
    linkCapturing: bool? = null,
    displayMode: PWA_DisplayMode? = null,
}
pub struct PWA_changeAppUserSettingsReturnValue {}
