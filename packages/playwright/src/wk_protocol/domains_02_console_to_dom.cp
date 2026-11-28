// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue, ProtocolStringMap } from "./common";
import { CSS_PseudoId } from "./domains_01_animation_to_canvas";
import { Debugger_BreakpointOptions, Debugger_Location, Debugger_ScriptId, Heap_HeapSnapshotData } from "./domains_03_domdebugger_to_inspector";
import { Network_FrameId, Network_RequestId, Network_Timestamp } from "./domains_04_layertree_to_network";
import { Runtime_ExecutionContextId, Runtime_RemoteObject, Runtime_RemoteObjectId } from "./domains_06_runtime_to_worker";

// ============================================================================
// Domain: Console
// Console domain defines methods and events for interaction with the JavaScript console. Console collects messages created by means of the <a href='http://getfirebug.com/wiki/index.php/Console_API'>JavaScript Console API</a>. One needs to enable this domain using <code>enable</code> command in order to start receiving the console messages. Browser collects messages issued while console domain is not enabled as well and reports them using <code>messageAdded</code> notification upon enabling.
// ============================================================================

/// Channels for different types of log messages.
/// enum: "xml"|"javascript"|"network"|"console-api"|"storage"|"rendering"|"css"|"accessibility"|"security"|"content-blocker"|"...
pub type Console_ChannelSource = []const u8;
/// Level of logging.
/// enum: "off"|"basic"|"verbose"
pub type Console_ChannelLevel = []const u8;
/// The reason the console is being cleared.
/// enum: "console-api"|"frontend"|"main-frame-navigation"
pub type Console_ClearReason = []const u8;
/// Logging channel.
pub struct Console_Channel {
    source: Console_ChannelSource = "",
    level: Console_ChannelLevel = "",
}
/// Console message.
pub struct Console_ConsoleMessage {
    source: Console_ChannelSource = "",
    level: []const u8 = "", // enum: "log"|"info"|"warning"|"error"|"debug"
    text: []const u8 = "",
    type_: ([]const u8)? = null, // protocol field: "type"; enum: "log"|"dir"|"dirxml"|"table"|"trace"|"clear"|"startGroup"|"startGroupCollapsed"|"endGroup"|"assert"|"timing"|"profile...
    url: ([]const u8)? = null,
    line: f64? = null,
    column: f64? = null,
    repeatCount: f64? = null,
    parameters: ([]const Runtime_RemoteObject)? = null,
    stackTrace: Console_StackTrace? = null,
    networkRequestId: Network_RequestId? = null,
    timestamp: f64? = null,
}
/// Stack entry for console errors and assertions.
pub struct Console_CallFrame {
    functionName: []const u8 = "",
    url: []const u8 = "",
    scriptId: Debugger_ScriptId = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
}
/// Call frames for async function calls, console assertions, and error messages.
pub struct Console_StackTrace {
    callFrames: []const Console_CallFrame = &.{},
    topCallFrameIsBoundary: bool? = null,
    truncated: bool? = null,
    parentStackTrace: (*const Console_StackTrace)? = null,
}
/// Issued when new console message is added.
pub struct Console_messageAddedPayload {
    message: Console_ConsoleMessage = .{},
}
/// Issued when subsequent message(s) are equal to the previous one(s).
pub struct Console_messageRepeatCountUpdatedPayload {
    count: f64 = 0.0,
    timestamp: f64? = null,
}
/// Issued when console is cleared. This happens either upon <code>clearMessages</code> command or after page navigation.
pub struct Console_messagesClearedPayload {
    reason: Console_ClearReason = "",
}
/// Issued from console.takeHeapSnapshot.
pub struct Console_heapSnapshotPayload {
    timestamp: f64 = 0.0,
    snapshotData: Heap_HeapSnapshotData = "",
    title: ([]const u8)? = null,
}
/// Enables console domain, sends the messages collected so far to the client by means of the <code>messageAdded</code> notification.
pub struct Console_enableParameters {}
pub struct Console_enableReturnValue {}
/// Disables console domain, prevents further console messages from being reported to the client.
pub struct Console_disableParameters {}
pub struct Console_disableReturnValue {}
/// Clears console messages collected in the browser.
pub struct Console_clearMessagesParameters {}
pub struct Console_clearMessagesReturnValue {}
/// Control whether calling <code>console.clear()</code> has an effect in Web Inspector. Defaults to true.
pub struct Console_setConsoleClearAPIEnabledParameters {
    enable: bool = false,
}
pub struct Console_setConsoleClearAPIEnabledReturnValue {}
/// List of the different message sources that are non-default logging channels.
pub struct Console_getLoggingChannelsParameters {}
pub struct Console_getLoggingChannelsReturnValue {
    channels: []const Console_Channel = &.{},
}
/// Modify the level of a channel.
pub struct Console_setLoggingChannelLevelParameters {
    source: Console_ChannelSource = "",
    level: Console_ChannelLevel = "",
}
pub struct Console_setLoggingChannelLevelReturnValue {}

// ============================================================================
// Domain: DOM
// This domain exposes DOM read/write operations. Each DOM Node is represented with its mirror object that has an <code>id</code>. This <code>id</code> can be used to get additional information on the Node, resolve it into the JavaScript object wrapper, etc. It is important that client receives DOM events only for the nodes that are known to the client. Backend keeps track of the nodes that were sent to the client and never sends the same node twice. It is client's responsibility to collect information about the nodes that were sent to the client.<p>Note that <code>iframe</code> owner elements will return corresponding document elements as their child nodes.</p>
// ============================================================================

/// Unique DOM node identifier.
pub type DOM_NodeId = f64;
/// Unique event listener identifier.
pub type DOM_EventListenerId = f64;
/// Pseudo element type.
/// enum: "before"|"after"
pub type DOM_PseudoType = []const u8;
/// Shadow root type.
/// enum: "user-agent"|"open"|"closed"
pub type DOM_ShadowRootType = []const u8;
/// Custom element state.
/// enum: "builtin"|"custom"|"waiting"|"failed"
pub type DOM_CustomElementState = []const u8;
/// Token values of @aria-relevant attribute.
/// enum: "additions"|"removals"|"text"
pub type DOM_LiveRegionRelevant = []const u8;
/// DOM interaction is implemented in terms of mirror objects that represent the actual DOM nodes. DOMNode is a base node mirror type.
pub struct DOM_Node {
    nodeId: DOM_NodeId = 0.0,
    nodeType: f64 = 0.0,
    nodeName: []const u8 = "",
    localName: []const u8 = "",
    nodeValue: []const u8 = "",
    frameId: Network_FrameId? = null,
    contentFrameId: Network_FrameId? = null,
    childNodeCount: f64? = null,
    children: ([]const DOM_Node)? = null,
    attributes: ([]const []const u8)? = null,
    documentURL: ([]const u8)? = null,
    baseURL: ([]const u8)? = null,
    publicId: ([]const u8)? = null,
    systemId: ([]const u8)? = null,
    xmlVersion: ([]const u8)? = null,
    name: ([]const u8)? = null,
    value: ([]const u8)? = null,
    pseudoType: DOM_PseudoType? = null,
    shadowRootType: DOM_ShadowRootType? = null,
    customElementState: DOM_CustomElementState? = null,
    contentDocument: (*const DOM_Node)? = null,
    shadowRoots: ([]const DOM_Node)? = null,
    templateContent: (*const DOM_Node)? = null,
    pseudoElements: ([]const DOM_Node)? = null,
    contentSecurityPolicyHash: ([]const u8)? = null,
    layoutFlags: ([]const []const u8)? = null,
}
/// Relationship between data that is associated with a node and the node itself.
pub struct DOM_DataBinding {
    binding: []const u8 = "",
    type_: ([]const u8)? = null, // protocol field: "type"
    value: []const u8 = "",
}
pub struct DOM_Rect {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
}
/// A structure holding event listener properties.
pub struct DOM_EventListener {
    eventListenerId: DOM_EventListenerId = 0.0,
    type_: []const u8 = "", // protocol field: "type"
    useCapture: bool = false,
    isAttribute: bool = false,
    nodeId: DOM_NodeId? = null,
    onWindow: bool? = null,
    location: Debugger_Location? = null,
    handlerName: ([]const u8)? = null,
    passive: bool? = null,
    once: bool? = null,
    disabled: bool? = null,
    hasBreakpoint: bool? = null,
}
/// A structure holding accessibility properties.
pub struct DOM_AccessibilityProperties {
    activeDescendantNodeId: DOM_NodeId? = null,
    busy: bool? = null,
    checked: ([]const u8)? = null, // enum: "true"|"false"|"mixed"
    childNodeIds: ([]const DOM_NodeId)? = null,
    controlledNodeIds: ([]const DOM_NodeId)? = null,
    current: ([]const u8)? = null, // enum: "true"|"false"|"page"|"step"|"location"|"date"|"time"
    disabled: bool? = null,
    headingLevel: f64? = null,
    hierarchyLevel: f64? = null,
    isPopUpButton: bool? = null,
    exists: bool = false,
    expanded: bool? = null,
    flowedNodeIds: ([]const DOM_NodeId)? = null,
    focused: bool? = null,
    ignored: bool? = null,
    ignoredByDefault: bool? = null,
    invalid: ([]const u8)? = null, // enum: "true"|"false"|"grammar"|"spelling"
    hidden: bool? = null,
    label: []const u8 = "",
    liveRegionAtomic: bool? = null,
    liveRegionRelevant: ([]const []const u8)? = null,
    liveRegionStatus: ([]const u8)? = null, // enum: "assertive"|"polite"|"off"
    mouseEventNodeId: DOM_NodeId? = null,
    nodeId: DOM_NodeId = 0.0,
    ownedNodeIds: ([]const DOM_NodeId)? = null,
    parentNodeId: DOM_NodeId? = null,
    pressed: bool? = null,
    readonly: bool? = null,
    required: bool? = null,
    role: []const u8 = "",
    selected: bool? = null,
    selectedChildNodeIds: ([]const DOM_NodeId)? = null,
    switchState: ([]const u8)? = null, // enum: "off"|"on"
}
/// A structure holding an RGBA color.
pub struct DOM_RGBAColor {
    r: f64 = 0.0,
    g: f64 = 0.0,
    b: f64 = 0.0,
    a: f64? = null,
}
/// An array of quad vertices, x immediately followed by y for each point, points clock-wise.
pub type DOM_Quad = []const f64;
/// Configuration data for the highlighting of page elements.
pub struct DOM_HighlightConfig {
    showInfo: bool? = null,
    contentColor: DOM_RGBAColor? = null,
    paddingColor: DOM_RGBAColor? = null,
    borderColor: DOM_RGBAColor? = null,
    marginColor: DOM_RGBAColor? = null,
}
/// Configuration data for grid overlays.
pub struct DOM_GridOverlayConfig {
    gridColor: DOM_RGBAColor = .{},
    showLineNames: bool? = null,
    showLineNumbers: bool? = null,
    showExtendedGridLines: bool? = null,
    showTrackSizes: bool? = null,
    showAreaNames: bool? = null,
    showOrderNumbers: bool? = null,
}
/// Configuration data for flex overlays.
pub struct DOM_FlexOverlayConfig {
    flexColor: DOM_RGBAColor = .{},
    showOrderNumbers: bool? = null,
}
/// An object referencing a node and a pseudo-element, primarily used to identify an animation effect target.
pub struct DOM_Styleable {
    nodeId: DOM_NodeId = 0.0,
    pseudoId: CSS_PseudoId? = null,
}
/// A structure holding media element statistics and configurations.
pub struct DOM_MediaStats {
    audio: DOM_AudioMediaStats? = null,
    video: DOM_VideoMediaStats? = null,
    devicePixelRatio: f64? = null,
    viewport: DOM_ViewportSize? = null,
    quality: DOM_VideoPlaybackQuality? = null,
    source: ([]const u8)? = null,
}
/// A structure holding media element's audio-specific statistics and configurations.
pub struct DOM_AudioMediaStats {
    bitrate: f64 = 0.0,
    codec: []const u8 = "",
    humanReadableCodecString: []const u8 = "",
    numberOfChannels: f64 = 0.0,
    sampleRate: f64 = 0.0,
    isProtected: bool? = null,
}
/// A structure holding media element's audio-specific statistics and configurations.
pub struct DOM_VideoMediaStats {
    bitrate: f64 = 0.0,
    codec: []const u8 = "",
    humanReadableCodecString: []const u8 = "",
    colorSpace: DOM_VideoColorSpace = .{},
    framerate: f64 = 0.0,
    height: f64 = 0.0,
    width: f64 = 0.0,
    immersiveVideoMetadata: DOM_ImmersiveVideoMetadata? = null,
    isProtected: bool? = null,
}
/// WebCodecs VideoColorSpace
pub struct DOM_VideoColorSpace {
    fullRange: bool? = null,
    matrix: ([]const u8)? = null,
    primaries: ([]const u8)? = null,
    transfer: ([]const u8)? = null,
}
/// A count of frames enqueued for display by the media element, and a subset count of dropped and display composited frames.
pub struct DOM_VideoPlaybackQuality {
    displayCompositedVideoFrames: f64 = 0.0,
    droppedVideoFrames: f64 = 0.0,
    totalVideoFrames: f64 = 0.0,
}
/// A structure containing metadata describing spatial video properties.
pub struct DOM_ImmersiveVideoMetadata {
    kind: DOM_VideoProjectionMetadataKind = "",
    width: f64 = 0.0,
    height: f64 = 0.0,
    horizontalFieldOfView: f64? = null,
    stereoCameraBaseline: f64? = null,
    horizontalDisparityAdjustment: f64? = null,
}
/// Video Projection Metadata Kind.
/// enum: "unknown"|"rectilinear"|"equirectangular"|"half-equirectangular"|"equi-angular-cubemap"|"parametric"|"pyramid"|"apple...
pub type DOM_VideoProjectionMetadataKind = []const u8;
pub struct DOM_ViewportSize {
    width: f64 = 0.0,
    height: f64 = 0.0,
}
/// Fired when <code>Document</code> has been totally updated. Node ids are no longer valid.
pub type DOM_documentUpdatedPayload = ProtocolJsonValue;
/// Inspect a particular node.
pub struct DOM_inspectPayload {
    nodeId: DOM_NodeId = 0.0,
}
/// Fired when backend wants to provide client with the missing DOM structure. This happens upon most of the calls requesting node ids.
pub struct DOM_setChildNodesPayload {
    parentId: DOM_NodeId = 0.0,
    nodes: []const DOM_Node = &.{},
}
/// Fired when <code>Element</code>'s attribute is modified.
pub struct DOM_attributeModifiedPayload {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Fired when <code>Element</code>'s attribute is removed.
pub struct DOM_attributeRemovedPayload {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
}
/// Fired when <code>Element</code>'s inline style is modified via a CSS property modification.
pub struct DOM_inlineStyleInvalidatedPayload {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Mirrors <code>DOMCharacterDataModified</code> event.
pub struct DOM_characterDataModifiedPayload {
    nodeId: DOM_NodeId = 0.0,
    characterData: []const u8 = "",
}
/// Fired when <code>Container</code>'s child node count has changed.
pub struct DOM_childNodeCountUpdatedPayload {
    nodeId: DOM_NodeId = 0.0,
    childNodeCount: f64 = 0.0,
}
/// Mirrors <code>DOMNodeInserted</code> event.
pub struct DOM_childNodeInsertedPayload {
    parentNodeId: DOM_NodeId = 0.0,
    previousNodeId: DOM_NodeId = 0.0,
    node: DOM_Node = .{},
}
/// Mirrors <code>DOMNodeRemoved</code> event.
pub struct DOM_childNodeRemovedPayload {
    parentNodeId: DOM_NodeId = 0.0,
    nodeId: DOM_NodeId = 0.0,
}
/// Fired when a detached DOM node is about to be destroyed. Currently, this event will only be fired when a DOM node that is detached is about to be destructed.
pub struct DOM_willDestroyDOMNodePayload {
    nodeId: DOM_NodeId = 0.0,
}
/// Called when shadow root is pushed into the element.
pub struct DOM_shadowRootPushedPayload {
    hostId: DOM_NodeId = 0.0,
    root: DOM_Node = .{},
}
/// Called when shadow root is popped from the element.
pub struct DOM_shadowRootPoppedPayload {
    hostId: DOM_NodeId = 0.0,
    rootId: DOM_NodeId = 0.0,
}
/// Called when the custom element state is changed.
pub struct DOM_customElementStateChangedPayload {
    nodeId: DOM_NodeId = 0.0,
    customElementState: DOM_CustomElementState = "",
}
/// Called when a pseudo element is added to an element.
pub struct DOM_pseudoElementAddedPayload {
    parentId: DOM_NodeId = 0.0,
    pseudoElement: DOM_Node = .{},
}
/// Called when a pseudo element is removed from an element.
pub struct DOM_pseudoElementRemovedPayload {
    parentId: DOM_NodeId = 0.0,
    pseudoElementId: DOM_NodeId = 0.0,
}
/// Called when an event listener is added to a node.
pub struct DOM_didAddEventListenerPayload {
    nodeId: DOM_NodeId = 0.0,
}
/// Called after a request has been made to remove an event listener from a node.
pub struct DOM_willRemoveEventListenerPayload {
    nodeId: DOM_NodeId = 0.0,
}
/// Called when an event is fired on a node.
pub struct DOM_didFireEventPayload {
    nodeId: DOM_NodeId = 0.0,
    eventName: []const u8 = "",
    timestamp: Network_Timestamp = 0.0,
    data: ProtocolStringMap? = null,
}
/// Called when an element enters/exits a power efficient playback state.
pub struct DOM_powerEfficientPlaybackStateChangedPayload {
    nodeId: DOM_NodeId = 0.0,
    timestamp: Network_Timestamp = 0.0,
    isPowerEfficient: bool = false,
}
/// Returns the root DOM node to the caller.
pub struct DOM_getDocumentParameters {}
pub struct DOM_getDocumentReturnValue {
    root: DOM_Node = .{},
}
/// Requests that children of the node with given id are returned to the caller in form of <code>setChildNodes</code> events where not only immediate children are retrieved, but all children down to the specified depth.
pub struct DOM_requestChildNodesParameters {
    nodeId: DOM_NodeId = 0.0,
    depth: f64? = null,
}
pub struct DOM_requestChildNodesReturnValue {}
/// Requests the <code>HTMLSlotElement</code> that the node with the given id is assigned to.
pub struct DOM_requestAssignedSlotParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_requestAssignedSlotReturnValue {
    slotElementId: DOM_NodeId? = null,
}
/// Requests the list of assigned nodes for the <code>HTMLSlotElement</code> with the given id.
pub struct DOM_requestAssignedNodesParameters {
    slotElementId: DOM_NodeId = 0.0,
}
pub struct DOM_requestAssignedNodesReturnValue {
    assignedNodeIds: []const DOM_NodeId = &.{},
}
/// Executes <code>querySelector</code> on a given node.
pub struct DOM_querySelectorParameters {
    nodeId: DOM_NodeId = 0.0,
    selector: []const u8 = "",
}
pub struct DOM_querySelectorReturnValue {
    nodeId: DOM_NodeId? = null,
}
/// Executes <code>querySelectorAll</code> on a given node.
pub struct DOM_querySelectorAllParameters {
    nodeId: DOM_NodeId = 0.0,
    selector: []const u8 = "",
}
pub struct DOM_querySelectorAllReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
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
/// Removes node with given id.
pub struct DOM_removeNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_removeNodeReturnValue {}
/// Sets attribute for an element with given id.
pub struct DOM_setAttributeValueParameters {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
    value: []const u8 = "",
}
pub struct DOM_setAttributeValueReturnValue {}
/// Sets attributes on element with given id. This method is useful when user edits some existing attribute value and types in several attribute name/value pairs.
pub struct DOM_setAttributesAsTextParameters {
    nodeId: DOM_NodeId = 0.0,
    text: []const u8 = "",
    name: ([]const u8)? = null,
}
pub struct DOM_setAttributesAsTextReturnValue {}
/// Removes attribute with given name from an element with given id.
pub struct DOM_removeAttributeParameters {
    nodeId: DOM_NodeId = 0.0,
    name: []const u8 = "",
}
pub struct DOM_removeAttributeReturnValue {}
/// Gets the list of builtin DOM event names.
pub struct DOM_getSupportedEventNamesParameters {}
pub struct DOM_getSupportedEventNamesReturnValue {
    eventNames: []const []const u8 = &.{},
}
/// Returns all data binding relationships between data that is associated with the node and the node itself.
pub struct DOM_getDataBindingsForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getDataBindingsForNodeReturnValue {
    dataBindings: []const DOM_DataBinding = &.{},
}
/// Returns all data that has been associated with the node and is available for data binding.
pub struct DOM_getAssociatedDataForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getAssociatedDataForNodeReturnValue {
    associatedData: ([]const u8)? = null,
}
/// Returns event listeners relevant to the node.
pub struct DOM_getEventListenersForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
    includeAncestors: bool? = null,
}
pub struct DOM_getEventListenersForNodeReturnValue {
    listeners: []const DOM_EventListener = &.{},
}
/// Enable/disable the given event listener. A disabled event listener will not fire.
pub struct DOM_setEventListenerDisabledParameters {
    eventListenerId: DOM_EventListenerId = 0.0,
    disabled: bool = false,
}
pub struct DOM_setEventListenerDisabledReturnValue {}
/// Set a breakpoint on the given event listener.
pub struct DOM_setBreakpointForEventListenerParameters {
    eventListenerId: DOM_EventListenerId = 0.0,
    options: Debugger_BreakpointOptions? = null,
}
pub struct DOM_setBreakpointForEventListenerReturnValue {}
/// Remove any breakpoints on the given event listener.
pub struct DOM_removeBreakpointForEventListenerParameters {
    eventListenerId: DOM_EventListenerId = 0.0,
}
pub struct DOM_removeBreakpointForEventListenerReturnValue {}
/// Returns a dictionary of accessibility properties for the node.
pub struct DOM_getAccessibilityPropertiesForNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getAccessibilityPropertiesForNodeReturnValue {
    properties: DOM_AccessibilityProperties = .{},
}
/// Returns node's HTML markup.
pub struct DOM_getOuterHTMLParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getOuterHTMLReturnValue {
    outerHTML: []const u8 = "",
}
/// Sets node HTML markup, returns new node id.
pub struct DOM_setOuterHTMLParameters {
    nodeId: DOM_NodeId = 0.0,
    outerHTML: []const u8 = "",
}
pub struct DOM_setOuterHTMLReturnValue {}
pub struct DOM_insertAdjacentHTMLParameters {
    nodeId: DOM_NodeId = 0.0,
    position: []const u8 = "",
    html: []const u8 = "",
}
pub struct DOM_insertAdjacentHTMLReturnValue {}
/// Searches for a given string in the DOM tree. Use <code>getSearchResults</code> to access search results or <code>cancelSearch</code> to end this search session.
pub struct DOM_performSearchParameters {
    query: []const u8 = "",
    nodeIds: ([]const DOM_NodeId)? = null,
    caseSensitive: bool? = null,
}
pub struct DOM_performSearchReturnValue {
    searchId: []const u8 = "",
    resultCount: f64 = 0.0,
}
/// Returns search results from given <code>fromIndex</code> to given <code>toIndex</code> from the sarch with the given identifier.
pub struct DOM_getSearchResultsParameters {
    searchId: []const u8 = "",
    fromIndex: f64 = 0.0,
    toIndex: f64 = 0.0,
}
pub struct DOM_getSearchResultsReturnValue {
    nodeIds: []const DOM_NodeId = &.{},
}
/// Discards search results from the session with the given id. <code>getSearchResults</code> should no longer be called for that search.
pub struct DOM_discardSearchResultsParameters {
    searchId: []const u8 = "",
}
pub struct DOM_discardSearchResultsReturnValue {}
/// Requests that the node is sent to the caller given the JavaScript node object reference. All nodes that form the path from the node to the root are also sent to the client as a series of <code>setChildNodes</code> notifications.
pub struct DOM_requestNodeParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct DOM_requestNodeReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Requests that the frame owner element (such as <code>iframe</code>) of a child frame is sent to the caller. The child frame may be hosted by another process. All nodes that form the path from the owner element to the root are also sent to the client as a series of <code>setChildNodes</code> notifications.
pub struct DOM_requestFrameOwnerParameters {
    frameId: Network_FrameId = "",
}
pub struct DOM_requestFrameOwnerReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Enters the 'inspect' mode. In this mode, elements that user is hovering over are highlighted. Backend then generates 'inspect' command upon element selection.
pub struct DOM_setInspectModeEnabledParameters {
    enabled: bool = false,
    highlightConfig: DOM_HighlightConfig? = null,
    gridOverlayConfig: DOM_GridOverlayConfig? = null,
    flexOverlayConfig: DOM_FlexOverlayConfig? = null,
    showRulers: bool? = null,
}
pub struct DOM_setInspectModeEnabledReturnValue {}
/// Highlights given rectangle. Coordinates are absolute with respect to the target's viewport: the main frame viewport for a page target, or the frame's own viewport for a frame target.
pub struct DOM_highlightRectParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    color: DOM_RGBAColor? = null,
    outlineColor: DOM_RGBAColor? = null,
    usePageCoordinates: bool? = null,
}
pub struct DOM_highlightRectReturnValue {}
/// Highlights given quad. Coordinates are absolute with respect to the target's viewport: the main frame viewport for a page target, or the frame's own viewport for a frame target.
pub struct DOM_highlightQuadParameters {
    quad: DOM_Quad = &.{},
    color: DOM_RGBAColor? = null,
    outlineColor: DOM_RGBAColor? = null,
    usePageCoordinates: bool? = null,
}
pub struct DOM_highlightQuadReturnValue {}
/// Highlights all DOM nodes that match a given selector. A string containing a CSS selector must be specified.
pub struct DOM_highlightSelectorParameters {
    selectorString: []const u8 = "",
    frameId: ([]const u8)? = null,
    highlightConfig: DOM_HighlightConfig = .{},
    gridOverlayConfig: DOM_GridOverlayConfig? = null,
    flexOverlayConfig: DOM_FlexOverlayConfig? = null,
    showRulers: bool? = null,
}
pub struct DOM_highlightSelectorReturnValue {}
/// Highlights DOM node with given id or with the given JavaScript object wrapper. Either nodeId or objectId must be specified.
pub struct DOM_highlightNodeParameters {
    nodeId: DOM_NodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    highlightConfig: DOM_HighlightConfig = .{},
    gridOverlayConfig: DOM_GridOverlayConfig? = null,
    flexOverlayConfig: DOM_FlexOverlayConfig? = null,
    showRulers: bool? = null,
}
pub struct DOM_highlightNodeReturnValue {}
/// Highlights each DOM node in the given list.
pub struct DOM_highlightNodeListParameters {
    nodeIds: []const DOM_NodeId = &.{},
    highlightConfig: DOM_HighlightConfig = .{},
    gridOverlayConfig: DOM_GridOverlayConfig? = null,
    flexOverlayConfig: DOM_FlexOverlayConfig? = null,
    showRulers: bool? = null,
}
pub struct DOM_highlightNodeListReturnValue {}
/// Hides DOM node highlight.
pub struct DOM_hideHighlightParameters {}
pub struct DOM_hideHighlightReturnValue {}
/// Highlights owner element of the frame with given id.
pub struct DOM_highlightFrameParameters {
    frameId: Network_FrameId = "",
    contentColor: DOM_RGBAColor? = null,
    contentOutlineColor: DOM_RGBAColor? = null,
}
pub struct DOM_highlightFrameReturnValue {}
/// Shows a grid overlay for a node that begins a 'grid' layout context. The command has no effect if <code>nodeId</code> is invalid or the associated node does not begin a 'grid' layout context. A node can only have one grid overlay at a time; subsequent calls with the same <code>nodeId</code> will override earlier calls.
pub struct DOM_showGridOverlayParameters {
    nodeId: DOM_NodeId = 0.0,
    gridOverlayConfig: DOM_GridOverlayConfig = .{},
}
pub struct DOM_showGridOverlayReturnValue {}
/// Hides a grid overlay for a node that begins a 'grid' layout context. The command has no effect if <code>nodeId</code> is specified and invalid, or if there is not currently an overlay set for the <code>nodeId</code>.
pub struct DOM_hideGridOverlayParameters {
    nodeId: DOM_NodeId? = null,
}
pub struct DOM_hideGridOverlayReturnValue {}
/// Shows a flex overlay for a node that begins a 'flex' layout context. The command has no effect if <code>nodeId</code> is invalid or the associated node does not begin a 'flex' layout context. A node can only have one flex overlay at a time; subsequent calls with the same <code>nodeId</code> will override earlier calls.
pub struct DOM_showFlexOverlayParameters {
    nodeId: DOM_NodeId = 0.0,
    flexOverlayConfig: DOM_FlexOverlayConfig = .{},
}
pub struct DOM_showFlexOverlayReturnValue {}
/// Hides a flex overlay for a node that begins a 'flex' layout context. The command has no effect if <code>nodeId</code> is specified and invalid, or if there is not currently an overlay set for the <code>nodeId</code>.
pub struct DOM_hideFlexOverlayParameters {
    nodeId: DOM_NodeId? = null,
}
pub struct DOM_hideFlexOverlayReturnValue {}
/// Requests that the node is sent to the caller given its path.
pub struct DOM_pushNodeByPathToFrontendParameters {
    path: []const u8 = "",
}
pub struct DOM_pushNodeByPathToFrontendReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Resolves JavaScript node object for given node id.
pub struct DOM_resolveNodeParameters {
    nodeId: DOM_NodeId? = null,
    objectId: Runtime_RemoteObjectId? = null,
    frameId: Network_FrameId? = null,
    executionContextId: Runtime_ExecutionContextId? = null,
    objectGroup: ([]const u8)? = null,
}
pub struct DOM_resolveNodeReturnValue {
    object: Runtime_RemoteObject = .{},
}
/// Returns attributes for the specified node.
pub struct DOM_getAttributesParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getAttributesReturnValue {
    attributes: []const []const u8 = &.{},
}
/// Moves node into the new container, places it before the given anchor.
pub struct DOM_moveToParameters {
    nodeId: DOM_NodeId = 0.0,
    targetNodeId: DOM_NodeId = 0.0,
    insertBeforeNodeId: DOM_NodeId? = null,
}
pub struct DOM_moveToReturnValue {
    nodeId: DOM_NodeId = 0.0,
}
/// Undoes the last performed action.
pub struct DOM_undoParameters {}
pub struct DOM_undoReturnValue {}
/// Re-does the last undone action.
pub struct DOM_redoParameters {}
pub struct DOM_redoReturnValue {}
/// Marks last undoable state.
pub struct DOM_markUndoableStateParameters {}
pub struct DOM_markUndoableStateReturnValue {}
/// Focuses the given element.
pub struct DOM_focusParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_focusReturnValue {}
/// Enables console to refer to the node with given id via $0 (see Command Line API for more details).
pub struct DOM_setInspectedNodeParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_setInspectedNodeReturnValue {}
/// Controls whether any DOM commands work for nodes inside a UserAgent shadow tree.
pub struct DOM_setAllowEditingUserAgentShadowTreesParameters {
    allow: bool = false,
}
pub struct DOM_setAllowEditingUserAgentShadowTreesReturnValue {}
/// Returns media stats for the selected node.
pub struct DOM_getMediaStatsParameters {
    nodeId: DOM_NodeId = 0.0,
}
pub struct DOM_getMediaStatsReturnValue {
    mediaStats: DOM_MediaStats = .{},
}
/// Returns node description.
pub struct DOM_describeNodeParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct DOM_describeNodeReturnValue {
    contentFrameId: Network_FrameId? = null,
    ownerFrameId: Network_FrameId? = null,
}
/// Scrolls the given rect into view if not already in the viewport.
pub struct DOM_scrollIntoViewIfNeededParameters {
    objectId: Runtime_RemoteObjectId = "",
    rect: DOM_Rect? = null,
}
pub struct DOM_scrollIntoViewIfNeededReturnValue {}
/// Returns quads that describe node position on the page. This method
pub struct DOM_getContentQuadsParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct DOM_getContentQuadsReturnValue {
    quads: []const DOM_Quad = &.{},
}
/// Sets input files for given <input type=file>
pub struct DOM_setInputFilesParameters {
    objectId: Runtime_RemoteObjectId = "",
    paths: []const []const u8 = &.{},
}
pub struct DOM_setInputFilesReturnValue {}
