// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue } from "./common";
import { Network_AdProvenance } from "./domain_06_network";
import { Page_FrameId } from "./domains_08_page_to_performance";
import {
    Runtime_ExecutionContextId,
    Runtime_RemoteObject,
    Runtime_RemoteObjectId,
    Runtime_ScriptId,
    Runtime_StackTrace,
} from "./domains_11_debugger_to_schema";

// ============================================================================
// Domain: Cast
// A domain for interacting with Cast, Presentation API, and Remote Playback API
// functionalities.
// ============================================================================

pub struct Cast_Sink {
    name: []const u8 = "",
    id: []const u8 = "",
    session: ([]const u8)? = null,
}
/// This is fired whenever the list of available sinks changes. A sink is a
pub struct Cast_sinksUpdatedPayload {
    sinks: []const Cast_Sink = &.{},
}
/// This is fired whenever the outstanding issue/error message changes.
pub struct Cast_issueUpdatedPayload {
    issueMessage: []const u8 = "",
}
/// Starts observing for sinks that can be used for tab mirroring, and if set,
pub struct Cast_enableParameters {
    presentationUrl: ([]const u8)? = null,
}
pub struct Cast_enableReturnValue {}
/// Stops observing for sinks and issues.
pub struct Cast_disableParameters {}
pub struct Cast_disableReturnValue {}
/// Sets a sink to be used when the web page requests the browser to choose a
pub struct Cast_setSinkToUseParameters {
    sinkName: []const u8 = "",
}
pub struct Cast_setSinkToUseReturnValue {}
/// Starts mirroring the desktop to the sink.
pub struct Cast_startDesktopMirroringParameters {
    sinkName: []const u8 = "",
}
pub struct Cast_startDesktopMirroringReturnValue {}
/// Starts mirroring the tab to the sink.
pub struct Cast_startTabMirroringParameters {
    sinkName: []const u8 = "",
}
pub struct Cast_startTabMirroringReturnValue {}
/// Stops the active Cast session on the sink.
pub struct Cast_stopCastingParameters {
    sinkName: []const u8 = "",
}
pub struct Cast_stopCastingReturnValue {}

// ============================================================================
// Domain: CrashReportContext
// This domain exposes the current state of the CrashReportContext API.
// ============================================================================

/// Key-value pair in CrashReportContext.
pub struct CrashReportContext_CrashReportContextEntry {
    key: []const u8 = "",
    value: []const u8 = "",
    frameId: Page_FrameId = "",
}
/// Returns all entries in the CrashReportContext across all frames in the page.
pub struct CrashReportContext_getEntriesParameters {}
pub struct CrashReportContext_getEntriesReturnValue {
    entries: []const CrashReportContext_CrashReportContextEntry = &.{},
}

// ============================================================================
// Domain: DOM
// This domain exposes DOM read/write operations. Each DOM Node is represented with its mirror object
// that has an `id`. This `id` can be used to get additional information on the Node, resolve it into
// ============================================================================

/// Unique DOM node identifier.
pub type DOM_NodeId = f64;
/// Unique DOM node identifier used to reference a node that may not have been pushed to the
pub type DOM_BackendNodeId = f64;
/// Unique identifier for a CSS stylesheet.
pub type DOM_StyleSheetId = []const u8;
/// Backend node with a friendly name.
pub struct DOM_BackendNode {
    nodeType: f64 = 0.0,
    nodeName: []const u8 = "",
    backendNodeId: DOM_BackendNodeId = 0.0,
}
/// Pseudo element type.
/// enum: "first-line"|"first-letter"|"checkmark"|"before"|"after"|"expand-icon"|"picker-icon"|"interest-button"|"marker"|"back...
pub type DOM_PseudoType = []const u8;
/// Shadow root type.
/// enum: "user-agent"|"open"|"closed"
pub type DOM_ShadowRootType = []const u8;
/// Document compatibility mode.
/// enum: "QuirksMode"|"LimitedQuirksMode"|"NoQuirksMode"
pub type DOM_CompatibilityMode = []const u8;
/// ContainerSelector physical axes
/// enum: "Horizontal"|"Vertical"|"Both"
pub type DOM_PhysicalAxes = []const u8;
/// ContainerSelector logical axes
/// enum: "Inline"|"Block"|"Both"
pub type DOM_LogicalAxes = []const u8;
/// Physical scroll orientation
/// enum: "horizontal"|"vertical"
pub type DOM_ScrollOrientation = []const u8;
/// DOM interaction is implemented in terms of mirror objects that represent the actual DOM nodes.
pub struct DOM_Node {
    nodeId: DOM_NodeId = 0.0,
    parentId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId = 0.0,
    nodeType: f64 = 0.0,
    nodeName: []const u8 = "",
    localName: []const u8 = "",
    nodeValue: []const u8 = "",
    childNodeCount: f64? = null,
    children: ([]const DOM_Node)? = null,
    attributes: ([]const []const u8)? = null,
    documentURL: ([]const u8)? = null,
    baseURL: ([]const u8)? = null,
    publicId: ([]const u8)? = null,
    systemId: ([]const u8)? = null,
    internalSubset: ([]const u8)? = null,
    xmlVersion: ([]const u8)? = null,
    name: ([]const u8)? = null,
    value: ([]const u8)? = null,
    pseudoType: DOM_PseudoType? = null,
    pseudoIdentifier: ([]const u8)? = null,
    shadowRootType: DOM_ShadowRootType? = null,
    frameId: Page_FrameId? = null,
    contentDocument: (*const DOM_Node)? = null,
    shadowRoots: ([]const DOM_Node)? = null,
    templateContent: (*const DOM_Node)? = null,
    pseudoElements: ([]const DOM_Node)? = null,
    importedDocument: (*const DOM_Node)? = null,
    distributedNodes: ([]const DOM_BackendNode)? = null,
    isSVG: bool? = null,
    compatibilityMode: DOM_CompatibilityMode? = null,
    assignedSlot: DOM_BackendNode? = null,
    isScrollable: bool? = null,
    affectedByStartingStyles: bool? = null,
    adoptedStyleSheets: ([]const DOM_StyleSheetId)? = null,
    adProvenance: Network_AdProvenance? = null,
}
/// A structure to hold the top-level node of a detached tree and an array of its retained descendants.
pub struct DOM_DetachedElementInfo {
    treeNode: DOM_Node = .{},
    retainedNodeIds: []const DOM_NodeId = &.{},
}
/// A structure holding an RGBA color.
pub struct DOM_RGBA {
    r: f64 = 0.0,
    g: f64 = 0.0,
    b: f64 = 0.0,
    a: f64? = null,
}
/// An array of quad vertices, x immediately followed by y for each point, points clock-wise.
pub type DOM_Quad = []const f64;
/// Box model.
pub struct DOM_BoxModel {
    content: DOM_Quad = &.{},
    padding: DOM_Quad = &.{},
    border: DOM_Quad = &.{},
    margin: DOM_Quad = &.{},
    width: f64 = 0.0,
    height: f64 = 0.0,
    shapeOutside: DOM_ShapeOutsideInfo? = null,
}
/// CSS Shape Outside details.
pub struct DOM_ShapeOutsideInfo {
    bounds: DOM_Quad = &.{},
    shape: []const ProtocolJsonValue = &.{},
    marginShape: []const ProtocolJsonValue = &.{},
}
/// Rectangle.
pub struct DOM_Rect {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
}
pub struct DOM_CSSComputedStyleProperty {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Fired when `Element`'s attribute is modified.
pub struct DOM_attributeModifiedPayload {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Fired when `Element`'s adoptedStyleSheets are modified.
pub struct DOM_adoptedStyleSheetsModifiedPayload {
    nodeId: DOM_NodeId = 0.0,
    adoptedStyleSheets: []const DOM_StyleSheetId = &.{},
}
/// Fired when `Element`'s attribute is removed.
pub struct DOM_attributeRemovedPayload {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
}
/// Mirrors `DOMCharacterDataModified` event.
pub struct DOM_characterDataModifiedPayload {
    nodeId: DOM_NodeId = 0.0,
    characterData: []const u8 = "",
}
/// Fired when `Container`'s child node count has changed.
pub struct DOM_childNodeCountUpdatedPayload {
    nodeId: DOM_NodeId = 0.0,
    childNodeCount: f64 = 0.0,
}
/// Mirrors `DOMNodeInserted` event.
pub struct DOM_childNodeInsertedPayload {
    parentNodeId: DOM_NodeId = 0.0,
    previousNodeId: DOM_NodeId = 0.0,
    node: DOM_Node = .{},
}
/// Mirrors `DOMNodeRemoved` event.
pub struct DOM_childNodeRemovedPayload {
    parentNodeId: DOM_NodeId = 0.0,
    nodeId: DOM_NodeId = 0.0,
}
/// Called when distribution is changed.
pub struct DOM_distributedNodesUpdatedPayload {
    insertionPointId: DOM_NodeId = 0.0,
    distributedNodes: []const DOM_BackendNode = &.{},
}
/// Fired when `Document` has been totally updated. Node ids are no longer valid.
pub type DOM_documentUpdatedPayload = ProtocolJsonValue;
/// Fired when `Element`'s inline style is modified via a CSS property modification.
pub struct DOM_inlineStyleInvalidatedPayload {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Called when a pseudo element is added to an element.
pub struct DOM_pseudoElementAddedPayload {
    parentId: DOM_NodeId = 0.0,
    pseudoElement: DOM_Node = .{},
}
/// Called when top layer elements are changed.
pub type DOM_topLayerElementsUpdatedPayload = ProtocolJsonValue;
/// Fired when a node's scrollability state changes.
pub struct DOM_scrollableFlagUpdatedPayload {
    nodeId: DOM_NodeId = 0.0,
    isScrollable: bool = false,
}
/// Fired when a node's ad related state changes.
pub struct DOM_adRelatedStateUpdatedPayload {
    nodeId: DOM_NodeId = 0.0,
    adProvenance: Network_AdProvenance? = null,
}
/// Fired when a node's starting styles changes.
pub struct DOM_affectedByStartingStylesFlagUpdatedPayload {
    nodeId: DOM_NodeId = 0.0,
    affectedByStartingStyles: bool = false,
}
/// Called when a pseudo element is removed from an element.
pub struct DOM_pseudoElementRemovedPayload {
    parentId: DOM_NodeId = 0.0,
    pseudoElementId: DOM_NodeId = 0.0,
}
/// Fired when backend wants to provide client with the missing DOM structure. This happens upon
pub struct DOM_setChildNodesPayload {
    parentId: DOM_NodeId = 0.0,
    nodes: []const DOM_Node = &.{},
}
/// Called when shadow root is popped from the element.
pub struct DOM_shadowRootPoppedPayload {
    hostId: DOM_NodeId = 0.0,
    rootId: DOM_NodeId = 0.0,
}
/// Called when shadow root is pushed into the element.
pub struct DOM_shadowRootPushedPayload {
    hostId: DOM_NodeId = 0.0,
    root: DOM_Node = .{},
}
/// Collects class names for the node with given id and all of it's child nodes.
pub struct DOM_collectClassNamesFromSubtreeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_collectClassNamesFromSubtreeReturnValue {
    classNames: []const []const u8 = &.{},
}
/// Creates a deep copy of the specified node and places it into the target container before the
pub struct DOM_copyToParameters {
    nodeId: DOM_NodeId = 0.0,
    targetNodeId: DOM_NodeId = 0.0,
    insertBeforeNodeId: DOM_NodeId? = null,
}
pub struct DOM_copyToReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Describes node given its id, does not require domain to be enabled. Does not start tracking any
pub struct DOM_describeNodeParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    depth: f64? = null,
    pierce: bool? = null,
}
pub struct DOM_describeNodeReturnValue {
    node: DOM_Node = .{},
}
/// Scrolls the specified rect of the given node into view if not already visible.
pub struct DOM_scrollIntoViewIfNeededParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    rect: DOM_Rect? = null,
}
pub struct DOM_scrollIntoViewIfNeededReturnValue {}
/// Disables DOM agent for the given page.
pub struct DOM_disableParameters {}
pub struct DOM_disableReturnValue {}
/// Discards search results from the session with the given id. `getSearchResults` should no longer
pub struct DOM_discardSearchResultsParameters {
    searchId: []const u8 = "",
}
pub struct DOM_discardSearchResultsReturnValue {}
/// Enables DOM agent for the given page.
pub struct DOM_enableParameters {
    includeWhitespace: ([]const u8)? = null, // enum: "none"|"all"
}
pub struct DOM_enableReturnValue {}
/// Focuses the given element.
pub struct DOM_focusParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
pub struct DOM_focusReturnValue {}
/// Returns attributes for the specified node.
pub struct DOM_getAttributesParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getAttributesReturnValue {
    attributes: []const []const u8 = &.{},
}
/// Returns boxes for the given node.
pub struct DOM_getBoxModelParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
pub struct DOM_getBoxModelReturnValue {
    model: DOM_BoxModel = .{},
}
/// Returns quads that describe node position on the page. This method
pub struct DOM_getContentQuadsParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
pub struct DOM_getContentQuadsReturnValue {
    quads: []const DOM_Quad = &.{},
}
/// Returns the root DOM node (and optionally the subtree) to the caller.
pub struct DOM_getDocumentParameters {
    depth: f64? = null,
    pierce: bool? = null,
}
pub struct DOM_getDocumentReturnValue {
    root: DOM_Node = .{},
}
/// Returns the root DOM node (and optionally the subtree) to the caller.
pub struct DOM_getFlattenedDocumentParameters {
    depth: f64? = null,
    pierce: bool? = null,
}
pub struct DOM_getFlattenedDocumentReturnValue {
    nodes: []const DOM_Node = &.{},
}
/// Finds nodes with a given computed style in a subtree.
pub struct DOM_getNodesForSubtreeByStyleParameters {
    nodeId: DOM_NodeId = 0.0,
    computedStyles: []const DOM_CSSComputedStyleProperty = &.{},
    pierce: bool? = null,
}
pub struct DOM_getNodesForSubtreeByStyleReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Returns node id at given location. Depending on whether DOM domain is enabled, nodeId is
pub struct DOM_getNodeForLocationParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    includeUserAgentShadowDOM: bool? = null,
    ignorePointerEventsNone: bool? = null,
}
pub struct DOM_getNodeForLocationReturnValue {
    backendNodeId: DOM_BackendNodeId = 0.0,
    frameId: Page_FrameId = "",
    nodeId: DOM_NodeId? = null,
}
/// Returns node's HTML markup.
pub struct DOM_getOuterHTMLParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    includeShadowDOM: bool? = null,
}
pub struct DOM_getOuterHTMLReturnValue {
    outerHTML: []const u8 = "",
}
/// Returns the id of the nearest ancestor that is a relayout boundary.
pub struct DOM_getRelayoutBoundaryParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getRelayoutBoundaryReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Returns search results from given `fromIndex` to given `toIndex` from the search with the given
pub struct DOM_getSearchResultsParameters {
    searchId: []const u8 = "",
    fromIndex: f64 = 0.0,
    toIndex: f64 = 0.0,
}
pub struct DOM_getSearchResultsReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Hides any highlight.
pub struct DOM_hideHighlightParameters {}
pub struct DOM_hideHighlightReturnValue {}
/// Highlights DOM node.
pub struct DOM_highlightNodeParameters {}
pub struct DOM_highlightNodeReturnValue {}
/// Highlights given rectangle.
pub struct DOM_highlightRectParameters {}
pub struct DOM_highlightRectReturnValue {}
/// Marks last undoable state.
pub struct DOM_markUndoableStateParameters {}
pub struct DOM_markUndoableStateReturnValue {}
/// Moves node into the new container, places it before the given anchor.
pub struct DOM_moveToParameters {
    nodeId: DOM_NodeId = 0.0,
    targetNodeId: DOM_NodeId = 0.0,
    insertBeforeNodeId: DOM_NodeId? = null,
}
pub struct DOM_moveToReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Searches for a given string in the DOM tree. Use `getSearchResults` to access search results or
pub struct DOM_performSearchParameters {
    query: []const u8 = "",
    includeUserAgentShadowDOM: bool? = null,
}
pub struct DOM_performSearchReturnValue {
    searchId: []const u8 = "",
    resultCount: f64 = 0.0,
}
/// Requests that the node is sent to the caller given its path. // FIXME, use XPath
pub struct DOM_pushNodeByPathToFrontendParameters {
    path: []const u8 = "",
}
pub struct DOM_pushNodeByPathToFrontendReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Requests that a batch of nodes is sent to the caller given their backend node ids.
pub struct DOM_pushNodesByBackendIdsToFrontendParameters {
    backendNodeIds: []const DOM_BackendNodeId = &.{},
}
pub struct DOM_pushNodesByBackendIdsToFrontendReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Executes `querySelector` on a given node.
pub struct DOM_querySelectorParameters {
    nodeId: DOM_NodeId = 0.0,
    selector: []const u8 = "",
}
pub struct DOM_querySelectorReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Executes `querySelectorAll` on a given node.
pub struct DOM_querySelectorAllParameters {
    nodeId: DOM_NodeId = 0.0,
    selector: []const u8 = "",
}
pub struct DOM_querySelectorAllReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Returns NodeIds of current top layer elements.
pub struct DOM_getTopLayerElementsParameters {}
pub struct DOM_getTopLayerElementsReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Returns the NodeId of the matched element according to certain relations.
pub struct DOM_getElementByRelationParameters {
    nodeId: DOM_NodeId = 0.0,
    relation: []const u8 = "", // enum: "PopoverTarget"|"InterestTarget"|"CommandFor"
}
pub struct DOM_getElementByRelationReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Re-does the last undone action.
pub struct DOM_redoParameters {}
pub struct DOM_redoReturnValue {}
/// Removes attribute with given name from an element with given id.
pub struct DOM_removeAttributeParameters {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
}
pub struct DOM_removeAttributeReturnValue {}
/// Removes node with given id.
pub struct DOM_removeNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_removeNodeReturnValue {}
/// Requests that children of the node with given id are returned to the caller in form of
pub struct DOM_requestChildNodesParameters {
    nodeId: DOM_NodeId = 0.0,
    depth: f64? = null,
    pierce: bool? = null,
}
pub struct DOM_requestChildNodesReturnValue {}
/// Requests that the node is sent to the caller given the JavaScript node object reference. All
pub struct DOM_requestNodeParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct DOM_requestNodeReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Resolves the JavaScript node object for a given NodeId or BackendNodeId.
pub struct DOM_resolveNodeParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectGroup: ([]const u8)? = null,
    executionContextId: Runtime_ExecutionContextId? = null,
}
pub struct DOM_resolveNodeReturnValue {
    object: Runtime_RemoteObject = .{},
}
/// Sets attribute for an element with given id.
pub struct DOM_setAttributeValueParameters {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
    value: []const u8 = "",
}
pub struct DOM_setAttributeValueReturnValue {}
/// Sets attributes on element with given id. This method is useful when user edits some existing
pub struct DOM_setAttributesAsTextParameters {
    nodeId: DOM_NodeId = 0.0,
    text: []const u8 = "",
    name: ([]const u8)? = null,
}
pub struct DOM_setAttributesAsTextReturnValue {}
/// Sets files for the given file input element.
pub struct DOM_setFileInputFilesParameters {
    files: []const []const u8 = &.{},
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
pub struct DOM_setFileInputFilesReturnValue {}
/// Sets if stack traces should be captured for Nodes. See `Node.getNodeStackTraces`. Default is disabled.
pub struct DOM_setNodeStackTracesEnabledParameters {
    enable: bool = false,
}
pub struct DOM_setNodeStackTracesEnabledReturnValue {}
/// Gets stack traces associated with a Node. As of now, only provides stack trace for Node creation.
pub struct DOM_getNodeStackTracesParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getNodeStackTracesReturnValue {
    creation: Runtime_StackTrace? = null,
}
/// Returns file information for the given
pub struct DOM_getFileInfoParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct DOM_getFileInfoReturnValue {
    path: []const u8 = "",
}
/// Returns list of detached nodes
pub struct DOM_getDetachedDomNodesParameters {}
pub struct DOM_getDetachedDomNodesReturnValue {
    detachedNodes: []const DOM_DetachedElementInfo = &.{},
}
/// Enables console to refer to the node with given id via $x (see Command Line API for more details
pub struct DOM_setInspectedNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_setInspectedNodeReturnValue {}
/// Sets node name for a node with given id.
pub struct DOM_setNodeNameParameters {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
}
pub struct DOM_setNodeNameReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Sets node value for a node with given id.
pub struct DOM_setNodeValueParameters {
    nodeId: DOM_NodeId = 0.0,
    value: []const u8 = "",
}
pub struct DOM_setNodeValueReturnValue {}
/// Sets node HTML markup, returns new node id.
pub struct DOM_setOuterHTMLParameters {
    nodeId: DOM_NodeId = 0.0,
    outerHTML: []const u8 = "",
}
pub struct DOM_setOuterHTMLReturnValue {}
/// Undoes the last performed action.
pub struct DOM_undoParameters {}
pub struct DOM_undoReturnValue {}
/// Returns iframe node that owns iframe with the given domain.
pub struct DOM_getFrameOwnerParameters {
    frameId: Page_FrameId = "",
}
pub struct DOM_getFrameOwnerReturnValue {
    backendNodeId: DOM_BackendNodeId = 0.0,
    nodeId: DOM_NodeId? = null,
}
/// Returns the query container of the given node based on container query
pub struct DOM_getContainerForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
    containerName: ([]const u8)? = null,
    physicalAxes: DOM_PhysicalAxes? = null,
    logicalAxes: DOM_LogicalAxes? = null,
    queriesScrollState: bool? = null,
    queriesAnchored: bool? = null,
}
pub struct DOM_getContainerForNodeReturnValue {
    nodeId: DOM_NodeId? = null,
}
/// Returns the descendants of a container query container that have
pub struct DOM_getQueryingDescendantsForContainerParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getQueryingDescendantsForContainerReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Returns the target anchor element of the given anchor query according to
pub struct DOM_getAnchorElementParameters {
    nodeId: DOM_NodeId = 0.0,
    anchorSpecifier: ([]const u8)? = null,
}
pub struct DOM_getAnchorElementReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// When enabling, this API force-opens the popover identified by nodeId
pub struct DOM_forceShowPopoverParameters {
    nodeId: DOM_NodeId = 0.0,
    enable: bool = false,
    invokerNodeId: DOM_BackendNodeId? = null,
}
pub struct DOM_forceShowPopoverReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Returns candidate nodes that are configured as triggers for the given popover.
pub struct DOM_getImplicitAnchorCandidatesParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getImplicitAnchorCandidatesReturnValue {
    backendNodeIds: []const DOM_BackendNodeId = &.{},
}
/// When enabling, this API forces an element to gain interest in its target,
pub struct DOM_forceShowInterestParameters {
    nodeId: DOM_NodeId = 0.0,
    enable: bool = false,
}
pub struct DOM_forceShowInterestReturnValue {}
/// Sets a spelling or grammar error marker on the given range of text.
pub struct DOM_setTextMarkerParameters {
    nodeId: DOM_NodeId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    type_: []const u8 = "", // protocol field: "type"; enum: "spelling"|"grammar"
    start: f64 = 0.0,
    end: f64 = 0.0,
}
pub struct DOM_setTextMarkerReturnValue {}
/// Clears the spelling and grammar error text markers overlapping the ranges
pub struct DOM_clearTextMarkersParameters {}
pub struct DOM_clearTextMarkersReturnValue {}

// ============================================================================
// Domain: DOMDebugger
// DOM debugging allows setting breakpoints on particular DOM operations and events. JavaScript
// execution will stop on these operations as if there was a regular breakpoint set.
// ============================================================================

/// DOM breakpoint type.
/// enum: "subtree-modified"|"attribute-modified"|"node-removed"
pub type DOMDebugger_DOMBreakpointType = []const u8;
/// CSP Violation type.
/// enum: "trustedtype-sink-violation"|"trustedtype-policy-violation"
pub type DOMDebugger_CSPViolationType = []const u8;
/// Object event listener.
pub struct DOMDebugger_EventListener {
    type_: []const u8 = "", // protocol field: "type"
    useCapture: bool = false,
    passive: bool = false,
    once: bool = false,
    scriptId: Runtime_ScriptId = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
    handler: Runtime_RemoteObject? = null,
    originalHandler: Runtime_RemoteObject? = null,
    backendNodeId: DOM_BackendNodeId? = null,
}
/// Returns event listeners of the given object.
pub struct DOMDebugger_getEventListenersParameters {
    objectId: Runtime_RemoteObjectId = "",
    depth: f64? = null,
    pierce: bool? = null,
}
pub struct DOMDebugger_getEventListenersReturnValue {
    listeners: []const DOMDebugger_EventListener = &.{},
}
/// Removes DOM breakpoint that was set using `setDOMBreakpoint`.
pub struct DOMDebugger_removeDOMBreakpointParameters {
    nodeId: DOM_NodeId = 0.0,
    type_: DOMDebugger_DOMBreakpointType = "", // protocol field: "type"
}
pub struct DOMDebugger_removeDOMBreakpointReturnValue {}
/// Removes breakpoint on particular DOM event.
pub struct DOMDebugger_removeEventListenerBreakpointParameters {
    eventName: []const u8 = "",
    targetName: ([]const u8)? = null,
}
pub struct DOMDebugger_removeEventListenerBreakpointReturnValue {}
/// Removes breakpoint on particular native event.
pub struct DOMDebugger_removeInstrumentationBreakpointParameters {
    eventName: []const u8 = "",
}
pub struct DOMDebugger_removeInstrumentationBreakpointReturnValue {}
/// Removes breakpoint from XMLHttpRequest.
pub struct DOMDebugger_removeXHRBreakpointParameters {
    url: []const u8 = "",
}
pub struct DOMDebugger_removeXHRBreakpointReturnValue {}
/// Sets breakpoint on particular CSP violations.
pub struct DOMDebugger_setBreakOnCSPViolationParameters {
    violationTypes: []const DOMDebugger_CSPViolationType = &.{},
}
pub struct DOMDebugger_setBreakOnCSPViolationReturnValue {}
/// Sets breakpoint on particular operation with DOM.
pub struct DOMDebugger_setDOMBreakpointParameters {
    nodeId: DOM_NodeId = 0.0,
    type_: DOMDebugger_DOMBreakpointType = "", // protocol field: "type"
}
pub struct DOMDebugger_setDOMBreakpointReturnValue {}
/// Sets breakpoint on particular DOM event.
pub struct DOMDebugger_setEventListenerBreakpointParameters {
    eventName: []const u8 = "",
    targetName: ([]const u8)? = null,
}
pub struct DOMDebugger_setEventListenerBreakpointReturnValue {}
/// Sets breakpoint on particular native event.
pub struct DOMDebugger_setInstrumentationBreakpointParameters {
    eventName: []const u8 = "",
}
pub struct DOMDebugger_setInstrumentationBreakpointReturnValue {}
/// Sets breakpoint on XMLHttpRequest.
pub struct DOMDebugger_setXHRBreakpointParameters {
    url: []const u8 = "",
}
pub struct DOMDebugger_setXHRBreakpointReturnValue {}

// ============================================================================
// Domain: DOMSnapshot
// This domain facilitates obtaining document snapshots with DOM, layout, and style information.
// ============================================================================

/// A Node in the DOM tree.
pub struct DOMSnapshot_DOMNode {
    nodeType: f64 = 0.0,
    nodeName: []const u8 = "",
    nodeValue: []const u8 = "",
    textValue: ([]const u8)? = null,
    inputValue: ([]const u8)? = null,
    inputChecked: bool? = null,
    optionSelected: bool? = null,
    backendNodeId: DOM_BackendNodeId = 0.0,
    childNodeIndexes: ([]const f64)? = null,
    attributes: ([]const DOMSnapshot_NameValue)? = null,
    pseudoElementIndexes: ([]const f64)? = null,
    layoutNodeIndex: f64? = null,
    documentURL: ([]const u8)? = null,
    baseURL: ([]const u8)? = null,
    contentLanguage: ([]const u8)? = null,
    documentEncoding: ([]const u8)? = null,
    publicId: ([]const u8)? = null,
    systemId: ([]const u8)? = null,
    frameId: Page_FrameId? = null,
    contentDocumentIndex: f64? = null,
    pseudoType: DOM_PseudoType? = null,
    shadowRootType: DOM_ShadowRootType? = null,
    isClickable: bool? = null,
    eventListeners: ([]const DOMDebugger_EventListener)? = null,
    currentSourceURL: ([]const u8)? = null,
    originURL: ([]const u8)? = null,
    scrollOffsetX: f64? = null,
    scrollOffsetY: f64? = null,
}
/// Details of post layout rendered text positions. The exact layout should not be regarded as
pub struct DOMSnapshot_InlineTextBox {
    boundingBox: DOM_Rect = .{},
    startCharacterIndex: f64 = 0.0,
    numCharacters: f64 = 0.0,
}
/// Details of an element in the DOM tree with a LayoutObject.
pub struct DOMSnapshot_LayoutTreeNode {
    domNodeIndex: f64 = 0.0,
    boundingBox: DOM_Rect = .{},
    layoutText: ([]const u8)? = null,
    inlineTextNodes: ([]const DOMSnapshot_InlineTextBox)? = null,
    styleIndex: f64? = null,
    paintOrder: f64? = null,
    isStackingContext: bool? = null,
}
/// A subset of the full ComputedStyle as defined by the request whitelist.
pub struct DOMSnapshot_ComputedStyle {
    properties: []const DOMSnapshot_NameValue = &.{},
}
/// A name/value pair.
pub struct DOMSnapshot_NameValue {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Index of the string in the strings table.
pub type DOMSnapshot_StringIndex = f64;
/// Index of the string in the strings table.
pub type DOMSnapshot_ArrayOfStrings = []const DOMSnapshot_StringIndex;
/// Data that is only present on rare nodes.
pub struct DOMSnapshot_RareStringData {
    index: []const f64 = &.{},
    value: []const DOMSnapshot_StringIndex = &.{},
}
pub struct DOMSnapshot_RareBooleanData {
    index: []const f64 = &.{},
}
pub struct DOMSnapshot_RareIntegerData {
    index: []const f64 = &.{},
    value: []const f64 = &.{},
}
pub type DOMSnapshot_Rectangle = []const f64;
/// Document snapshot.
pub struct DOMSnapshot_DocumentSnapshot {
    documentURL: DOMSnapshot_StringIndex = 0.0,
    title: DOMSnapshot_StringIndex = 0.0,
    baseURL: DOMSnapshot_StringIndex = 0.0,
    contentLanguage: DOMSnapshot_StringIndex = 0.0,
    encodingName: DOMSnapshot_StringIndex = 0.0,
    publicId: DOMSnapshot_StringIndex = 0.0,
    systemId: DOMSnapshot_StringIndex = 0.0,
    frameId: DOMSnapshot_StringIndex = 0.0,
    nodes: DOMSnapshot_NodeTreeSnapshot = .{},
    layout: DOMSnapshot_LayoutTreeSnapshot = .{},
    textBoxes: DOMSnapshot_TextBoxSnapshot = .{},
    scrollOffsetX: f64? = null,
    scrollOffsetY: f64? = null,
    contentWidth: f64? = null,
    contentHeight: f64? = null,
}
/// Table containing nodes.
pub struct DOMSnapshot_NodeTreeSnapshot {
    parentIndex: ([]const f64)? = null,
    nodeType: ([]const f64)? = null,
    shadowRootType: DOMSnapshot_RareStringData? = null,
    nodeName: ([]const DOMSnapshot_StringIndex)? = null,
    nodeValue: ([]const DOMSnapshot_StringIndex)? = null,
    backendNodeId: ([]const DOM_BackendNodeId)? = null,
    attributes: ([]const DOMSnapshot_ArrayOfStrings)? = null,
    textValue: DOMSnapshot_RareStringData? = null,
    inputValue: DOMSnapshot_RareStringData? = null,
    inputChecked: DOMSnapshot_RareBooleanData? = null,
    optionSelected: DOMSnapshot_RareBooleanData? = null,
    contentDocumentIndex: DOMSnapshot_RareIntegerData? = null,
    pseudoType: DOMSnapshot_RareStringData? = null,
    pseudoIdentifier: DOMSnapshot_RareStringData? = null,
    isClickable: DOMSnapshot_RareBooleanData? = null,
    currentSourceURL: DOMSnapshot_RareStringData? = null,
    originURL: DOMSnapshot_RareStringData? = null,
}
/// Table of details of an element in the DOM tree with a LayoutObject.
pub struct DOMSnapshot_LayoutTreeSnapshot {
    nodeIndex: []const f64 = &.{},
    styles: []const DOMSnapshot_ArrayOfStrings = &.{},
    bounds: []const DOMSnapshot_Rectangle = &.{},
    text: []const DOMSnapshot_StringIndex = &.{},
    stackingContexts: DOMSnapshot_RareBooleanData = .{},
    paintOrders: ([]const f64)? = null,
    offsetRects: ([]const DOMSnapshot_Rectangle)? = null,
    scrollRects: ([]const DOMSnapshot_Rectangle)? = null,
    clientRects: ([]const DOMSnapshot_Rectangle)? = null,
    blendedBackgroundColors: ([]const DOMSnapshot_StringIndex)? = null,
    textColorOpacities: ([]const f64)? = null,
}
/// Table of details of the post layout rendered text positions. The exact layout should not be regarded as
pub struct DOMSnapshot_TextBoxSnapshot {
    layoutIndex: []const f64 = &.{},
    bounds: []const DOMSnapshot_Rectangle = &.{},
    start: []const f64 = &.{},
    length: []const f64 = &.{},
}
/// Disables DOM snapshot agent for the given page.
pub struct DOMSnapshot_disableParameters {}
pub struct DOMSnapshot_disableReturnValue {}
/// Enables DOM snapshot agent for the given page.
pub struct DOMSnapshot_enableParameters {}
pub struct DOMSnapshot_enableReturnValue {}
/// Returns a document snapshot, including the full DOM tree of the root node (including iframes,
pub struct DOMSnapshot_getSnapshotParameters {
    computedStyleWhitelist: []const []const u8 = &.{},
    includeEventListeners: bool? = null,
    includePaintOrder: bool? = null,
    includeUserAgentShadowTree: bool? = null,
}
pub struct DOMSnapshot_getSnapshotReturnValue {
    domNodes: []const DOMSnapshot_DOMNode = &.{},
    layoutTreeNodes: []const DOMSnapshot_LayoutTreeNode = &.{},
    computedStyles: []const DOMSnapshot_ComputedStyle = &.{},
}
/// Returns a document snapshot, including the full DOM tree of the root node (including iframes,
pub struct DOMSnapshot_captureSnapshotParameters {
    computedStyles: []const []const u8 = &.{},
    includePaintOrder: bool? = null,
    includeDOMRects: bool? = null,
    includeBlendedBackgroundColors: bool? = null,
    includeTextColorOpacities: bool? = null,
}
pub struct DOMSnapshot_captureSnapshotReturnValue {
    documents: []const DOMSnapshot_DocumentSnapshot = &.{},
    strings: []const []const u8 = &.{},
}
