// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue, ProtocolStringMap } from "./common";
import { Network_RequestId } from "./domain_06_network";
import { DOM_BackendNodeId, DOM_Rect } from "./domains_03_cast_to_domsnapshot";
import { Storage_StorageBucket } from "./domains_09_performancetimeline_to_systeminfo";
import { Runtime_RemoteObject, Runtime_RemoteObjectId, Runtime_StackTrace, Runtime_Timestamp } from "./domains_11_debugger_to_schema";

// ============================================================================
// Domain: IO
// Input/Output operations for streams produced by DevTools.
// ============================================================================

/// This is either obtained from another method or specified as `blob:<uuid>` where
pub type IO_StreamHandle = []const u8;
/// Close the stream, discard any temporary backing storage.
pub struct IO_closeParameters {
    handle: IO_StreamHandle = "",
}
pub struct IO_closeReturnValue {}
/// Read a chunk of the stream
pub struct IO_readParameters {
    handle: IO_StreamHandle = "",
    offset: f64? = null,
    size: f64? = null,
}
pub struct IO_readReturnValue {
    base64Encoded: bool? = null,
    data: []const u8 = "",
    eof: bool = false,
}
/// Return UUID of Blob object specified by a remote object id.
pub struct IO_resolveBlobParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct IO_resolveBlobReturnValue {
    uuid: []const u8 = "",
}

// ============================================================================
// Domain: IndexedDB
// ============================================================================

/// Database with an array of object stores.
pub struct IndexedDB_DatabaseWithObjectStores {
    name: []const u8 = "",
    version: f64 = 0.0,
    objectStores: []const IndexedDB_ObjectStore = &.{},
}
/// Object store.
pub struct IndexedDB_ObjectStore {
    name: []const u8 = "",
    keyPath: IndexedDB_KeyPath = .{},
    autoIncrement: bool = false,
    indexes: []const IndexedDB_ObjectStoreIndex = &.{},
}
/// Object store index.
pub struct IndexedDB_ObjectStoreIndex {
    name: []const u8 = "",
    keyPath: IndexedDB_KeyPath = .{},
    unique: bool = false,
    multiEntry: bool = false,
}
/// Key.
pub struct IndexedDB_Key {
    type_: []const u8 = "", // protocol field: "type"; enum: "number"|"string"|"date"|"array"
    number: f64? = null,
    string_: ([]const u8)? = null, // protocol field: "string"
    date: f64? = null,
    array: ([]const IndexedDB_Key)? = null,
}
/// Key range.
pub struct IndexedDB_KeyRange {
    lower: IndexedDB_Key? = null,
    upper: IndexedDB_Key? = null,
    lowerOpen: bool = false,
    upperOpen: bool = false,
}
/// Data entry.
pub struct IndexedDB_DataEntry {
    key: Runtime_RemoteObject = .{},
    primaryKey: Runtime_RemoteObject = .{},
    value: Runtime_RemoteObject = .{},
}
/// Key path.
pub struct IndexedDB_KeyPath {
    type_: []const u8 = "", // protocol field: "type"; enum: "null"|"string"|"array"
    string_: ([]const u8)? = null, // protocol field: "string"
    array: ([]const []const u8)? = null,
}
/// Clears all entries from an object store.
pub struct IndexedDB_clearObjectStoreParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
    databaseName: []const u8 = "",
    objectStoreName: []const u8 = "",
}
pub struct IndexedDB_clearObjectStoreReturnValue {}
/// Deletes a database.
pub struct IndexedDB_deleteDatabaseParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
    databaseName: []const u8 = "",
}
pub struct IndexedDB_deleteDatabaseReturnValue {}
/// Delete a range of entries from an object store
pub struct IndexedDB_deleteObjectStoreEntriesParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
    databaseName: []const u8 = "",
    objectStoreName: []const u8 = "",
    keyRange: IndexedDB_KeyRange = .{},
}
pub struct IndexedDB_deleteObjectStoreEntriesReturnValue {}
/// Disables events from backend.
pub struct IndexedDB_disableParameters {}
pub struct IndexedDB_disableReturnValue {}
/// Enables events from backend.
pub struct IndexedDB_enableParameters {}
pub struct IndexedDB_enableReturnValue {}
/// Requests data from object store or index.
pub struct IndexedDB_requestDataParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
    databaseName: []const u8 = "",
    objectStoreName: []const u8 = "",
    indexName: ([]const u8)? = null,
    skipCount: f64 = 0.0,
    pageSize: f64 = 0.0,
    keyRange: IndexedDB_KeyRange? = null,
}
pub struct IndexedDB_requestDataReturnValue {
    objectStoreDataEntries: []const IndexedDB_DataEntry = &.{},
    hasMore: bool = false,
}
/// Gets metadata of an object store.
pub struct IndexedDB_getMetadataParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
    databaseName: []const u8 = "",
    objectStoreName: []const u8 = "",
}
pub struct IndexedDB_getMetadataReturnValue {
    entriesCount: f64 = 0.0,
    keyGeneratorValue: f64 = 0.0,
}
/// Requests database with given name in given frame.
pub struct IndexedDB_requestDatabaseParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
    databaseName: []const u8 = "",
}
pub struct IndexedDB_requestDatabaseReturnValue {
    databaseWithObjectStores: IndexedDB_DatabaseWithObjectStores = .{},
}
/// Requests database names for given security origin.
pub struct IndexedDB_requestDatabaseNamesParameters {
    securityOrigin: ([]const u8)? = null,
    storageKey: ([]const u8)? = null,
    storageBucket: Storage_StorageBucket? = null,
}
pub struct IndexedDB_requestDatabaseNamesReturnValue {
    databaseNames: []const []const u8 = &.{},
}

// ============================================================================
// Domain: Input
// ============================================================================

pub struct Input_TouchPoint {
    x: f64 = 0.0,
    y: f64 = 0.0,
    radiusX: f64? = null,
    radiusY: f64? = null,
    rotationAngle: f64? = null,
    force: f64? = null,
    tangentialPressure: f64? = null,
    tiltX: f64? = null,
    tiltY: f64? = null,
    twist: f64? = null,
    id: f64? = null,
}
/// enum: "default"|"touch"|"mouse"
pub type Input_GestureSourceType = []const u8;
/// enum: "none"|"left"|"middle"|"right"|"back"|"forward"
pub type Input_MouseButton = []const u8;
/// UTC time in seconds, counted from January 1, 1970.
pub type Input_TimeSinceEpoch = f64;
pub struct Input_DragDataItem {
    mimeType: []const u8 = "",
    data: []const u8 = "",
    title: ([]const u8)? = null,
    baseURL: ([]const u8)? = null,
}
pub struct Input_DragData {
    items: []const Input_DragDataItem = &.{},
    files: ([]const []const u8)? = null,
    dragOperationsMask: f64 = 0.0,
}
/// Emitted only when `Input.setInterceptDrags` is enabled. Use this data with `Input.dispatchDragEvent` to
pub struct Input_dragInterceptedPayload {
    data: Input_DragData = .{},
}
/// Dispatches a drag event into the page.
pub struct Input_dispatchDragEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "dragEnter"|"dragOver"|"drop"|"dragCancel"
    x: f64 = 0.0,
    y: f64 = 0.0,
    data: Input_DragData = .{},
    modifiers: f64? = null,
}
pub struct Input_dispatchDragEventReturnValue {}
/// Dispatches a key event to the page.
pub struct Input_dispatchKeyEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "keyDown"|"keyUp"|"rawKeyDown"|"char"
    modifiers: f64? = null,
    timestamp: Input_TimeSinceEpoch? = null,
    text: ([]const u8)? = null,
    unmodifiedText: ([]const u8)? = null,
    keyIdentifier: ([]const u8)? = null,
    code: ([]const u8)? = null,
    key: ([]const u8)? = null,
    windowsVirtualKeyCode: f64? = null,
    nativeVirtualKeyCode: f64? = null,
    autoRepeat: bool? = null,
    isKeypad: bool? = null,
    isSystemKey: bool? = null,
    location: f64? = null,
    commands: ([]const []const u8)? = null,
}
pub struct Input_dispatchKeyEventReturnValue {}
/// This method emulates inserting text that doesn't come from a key press,
pub struct Input_insertTextParameters {
    text: []const u8 = "",
}
pub struct Input_insertTextReturnValue {}
/// This method sets the current candidate text for IME.
pub struct Input_imeSetCompositionParameters {
    text: []const u8 = "",
    selectionStart: f64 = 0.0,
    selectionEnd: f64 = 0.0,
    replacementStart: f64? = null,
    replacementEnd: f64? = null,
}
pub struct Input_imeSetCompositionReturnValue {}
/// Dispatches a mouse event to the page.
pub struct Input_dispatchMouseEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "mousePressed"|"mouseReleased"|"mouseMoved"|"mouseWheel"
    x: f64 = 0.0,
    y: f64 = 0.0,
    modifiers: f64? = null,
    timestamp: Input_TimeSinceEpoch? = null,
    button: Input_MouseButton? = null,
    buttons: f64? = null,
    clickCount: f64? = null,
    force: f64? = null,
    tangentialPressure: f64? = null,
    tiltX: f64? = null,
    tiltY: f64? = null,
    twist: f64? = null,
    deltaX: f64? = null,
    deltaY: f64? = null,
    pointerType: ([]const u8)? = null, // enum: "mouse"|"pen"
}
pub struct Input_dispatchMouseEventReturnValue {}
/// Dispatches a touch event to the page.
pub struct Input_dispatchTouchEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "touchStart"|"touchEnd"|"touchMove"|"touchCancel"
    touchPoints: []const Input_TouchPoint = &.{},
    modifiers: f64? = null,
    timestamp: Input_TimeSinceEpoch? = null,
}
pub struct Input_dispatchTouchEventReturnValue {}
/// Cancels any active dragging in the page.
pub struct Input_cancelDraggingParameters {}
pub struct Input_cancelDraggingReturnValue {}
/// Emulates touch event from the mouse event parameters.
pub struct Input_emulateTouchFromMouseEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "mousePressed"|"mouseReleased"|"mouseMoved"|"mouseWheel"
    x: f64 = 0.0,
    y: f64 = 0.0,
    button: Input_MouseButton = "",
    timestamp: Input_TimeSinceEpoch? = null,
    deltaX: f64? = null,
    deltaY: f64? = null,
    modifiers: f64? = null,
    clickCount: f64? = null,
}
pub struct Input_emulateTouchFromMouseEventReturnValue {}
/// Ignores input events (useful while auditing page).
pub struct Input_setIgnoreInputEventsParameters {
    ignore: bool = false,
}
pub struct Input_setIgnoreInputEventsReturnValue {}
/// Prevents default drag and drop behavior and instead emits `Input.dragIntercepted` events.
pub struct Input_setInterceptDragsParameters {
    enabled: bool = false,
}
pub struct Input_setInterceptDragsReturnValue {}
/// Synthesizes a pinch gesture over a time period by issuing appropriate touch events.
pub struct Input_synthesizePinchGestureParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    scaleFactor: f64 = 0.0,
    relativeSpeed: f64? = null,
    gestureSourceType: Input_GestureSourceType? = null,
}
pub struct Input_synthesizePinchGestureReturnValue {}
/// Synthesizes a scroll gesture over a time period by issuing appropriate touch events.
pub struct Input_synthesizeScrollGestureParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    xDistance: f64? = null,
    yDistance: f64? = null,
    xOverscroll: f64? = null,
    yOverscroll: f64? = null,
    preventFling: bool? = null,
    speed: f64? = null,
    gestureSourceType: Input_GestureSourceType? = null,
    repeatCount: f64? = null,
    repeatDelayMs: f64? = null,
    interactionMarkerName: ([]const u8)? = null,
}
pub struct Input_synthesizeScrollGestureReturnValue {}
/// Synthesizes a tap gesture over a time period by issuing appropriate touch events.
pub struct Input_synthesizeTapGestureParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    duration: f64? = null,
    tapCount: f64? = null,
    gestureSourceType: Input_GestureSourceType? = null,
}
pub struct Input_synthesizeTapGestureReturnValue {}

// ============================================================================
// Domain: Inspector
// ============================================================================

/// Fired when remote debugging connection is about to be terminated. Contains detach reason.
pub struct Inspector_detachedPayload {
    reason: []const u8 = "",
}
/// Fired when debugging target has crashed
pub type Inspector_targetCrashedPayload = ProtocolJsonValue;
/// Fired when debugging target has reloaded after crash
pub type Inspector_targetReloadedAfterCrashPayload = ProtocolJsonValue;
/// Fired on worker targets when main worker script and any imported scripts have been evaluated.
pub type Inspector_workerScriptLoadedPayload = ProtocolJsonValue;
/// Disables inspector domain notifications.
pub struct Inspector_disableParameters {}
pub struct Inspector_disableReturnValue {}
/// Enables inspector domain notifications.
pub struct Inspector_enableParameters {}
pub struct Inspector_enableReturnValue {}

// ============================================================================
// Domain: LayerTree
// ============================================================================

/// Unique Layer identifier.
pub type LayerTree_LayerId = []const u8;
/// Unique snapshot identifier.
pub type LayerTree_SnapshotId = []const u8;
/// Rectangle where scrolling happens on the main thread.
pub struct LayerTree_ScrollRect {
    rect: DOM_Rect = .{},
    type_: []const u8 = "", // protocol field: "type"; enum: "RepaintsOnScroll"|"TouchEventHandler"|"WheelEventHandler"
}
/// Sticky position constraints.
pub struct LayerTree_StickyPositionConstraint {
    stickyBoxRect: DOM_Rect = .{},
    containingBlockRect: DOM_Rect = .{},
    nearestLayerShiftingStickyBox: LayerTree_LayerId? = null,
    nearestLayerShiftingContainingBlock: LayerTree_LayerId? = null,
}
/// Serialized fragment of layer picture along with its offset within the layer.
pub struct LayerTree_PictureTile {
    x: f64 = 0.0,
    y: f64 = 0.0,
    picture: ProtocolBinary = "",
}
/// Information about a compositing layer.
pub struct LayerTree_Layer {
    layerId: LayerTree_LayerId = "",
    parentLayerId: LayerTree_LayerId? = null,
    backendNodeId: DOM_BackendNodeId? = null,
    offsetX: f64 = 0.0,
    offsetY: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    transform: ([]const f64)? = null,
    anchorX: f64? = null,
    anchorY: f64? = null,
    anchorZ: f64? = null,
    paintCount: f64 = 0.0,
    drawsContent: bool = false,
    invisible: bool? = null,
    scrollRects: ([]const LayerTree_ScrollRect)? = null,
    stickyPositionConstraint: LayerTree_StickyPositionConstraint? = null,
}
/// Array of timings, one per paint step.
pub type LayerTree_PaintProfile = []const f64;
pub struct LayerTree_layerPaintedPayload {
    layerId: LayerTree_LayerId = "",
    clip: DOM_Rect = .{},
}
pub struct LayerTree_layerTreeDidChangePayload {
    layers: ([]const LayerTree_Layer)? = null,
}
/// Provides the reasons why the given layer was composited.
pub struct LayerTree_compositingReasonsParameters {
    layerId: LayerTree_LayerId = "",
}
pub struct LayerTree_compositingReasonsReturnValue {
    compositingReasons: []const []const u8 = &.{},
    compositingReasonIds: []const []const u8 = &.{},
}
/// Disables compositing tree inspection.
pub struct LayerTree_disableParameters {}
pub struct LayerTree_disableReturnValue {}
/// Enables compositing tree inspection.
pub struct LayerTree_enableParameters {}
pub struct LayerTree_enableReturnValue {}
/// Returns the snapshot identifier.
pub struct LayerTree_loadSnapshotParameters {
    tiles: []const LayerTree_PictureTile = &.{},
}
pub struct LayerTree_loadSnapshotReturnValue {
    snapshotId: LayerTree_SnapshotId = "",
}
/// Returns the layer snapshot identifier.
pub struct LayerTree_makeSnapshotParameters {
    layerId: LayerTree_LayerId = "",
}
pub struct LayerTree_makeSnapshotReturnValue {
    snapshotId: LayerTree_SnapshotId = "",
}
pub struct LayerTree_profileSnapshotParameters {
    snapshotId: LayerTree_SnapshotId = "",
    minRepeatCount: f64? = null,
    minDuration: f64? = null,
    clipRect: DOM_Rect? = null,
}
pub struct LayerTree_profileSnapshotReturnValue {
    timings: []const LayerTree_PaintProfile = &.{},
}
/// Releases layer snapshot captured by the back-end.
pub struct LayerTree_releaseSnapshotParameters {
    snapshotId: LayerTree_SnapshotId = "",
}
pub struct LayerTree_releaseSnapshotReturnValue {}
/// Replays the layer snapshot and returns the resulting bitmap.
pub struct LayerTree_replaySnapshotParameters {
    snapshotId: LayerTree_SnapshotId = "",
    fromStep: f64? = null,
    toStep: f64? = null,
    scale: f64? = null,
}
pub struct LayerTree_replaySnapshotReturnValue {
    dataURL: []const u8 = "",
}
/// Replays the layer snapshot and returns canvas log.
pub struct LayerTree_snapshotCommandLogParameters {
    snapshotId: LayerTree_SnapshotId = "",
}
pub struct LayerTree_snapshotCommandLogReturnValue {
    commandLog: []const ProtocolStringMap = &.{},
}

// ============================================================================
// Domain: Log
// Provides access to log entries.
// ============================================================================

/// Log entry.
pub struct Log_LogEntry {
    source: []const u8 = "", // enum: "xml"|"javascript"|"network"|"storage"|"appcache"|"rendering"|"security"|"deprecation"|"worker"|"violation"|"interven...
    level: []const u8 = "", // enum: "verbose"|"info"|"warning"|"error"
    text: []const u8 = "",
    category: ([]const u8)? = null, // enum: "cors"
    timestamp: Runtime_Timestamp = 0.0,
    url: ([]const u8)? = null,
    lineNumber: f64? = null,
    stackTrace: Runtime_StackTrace? = null,
    networkRequestId: Network_RequestId? = null,
    workerId: ([]const u8)? = null,
    args: ([]const Runtime_RemoteObject)? = null,
}
/// Violation configuration setting.
pub struct Log_ViolationSetting {
    name: []const u8 = "", // enum: "longTask"|"longLayout"|"blockedEvent"|"blockedParser"|"discouragedAPIUse"|"handler"|"recurringHandler"
    threshold: f64 = 0.0,
}
/// Issued when new message was logged.
pub struct Log_entryAddedPayload {
    entry: Log_LogEntry = .{},
}
/// Clears the log.
pub struct Log_clearParameters {}
pub struct Log_clearReturnValue {}
/// Disables log domain, prevents further log entries from being reported to the client.
pub struct Log_disableParameters {}
pub struct Log_disableReturnValue {}
/// Enables log domain, sends the entries collected so far to the client by means of the
pub struct Log_enableParameters {}
pub struct Log_enableReturnValue {}
/// start violation reporting.
pub struct Log_startViolationsReportParameters {
    config: []const Log_ViolationSetting = &.{},
}
pub struct Log_startViolationsReportReturnValue {}
/// Stop violation reporting.
pub struct Log_stopViolationsReportParameters {}
pub struct Log_stopViolationsReportReturnValue {}

// ============================================================================
// Domain: Media
// This domain allows detailed inspection of media elements.
// ============================================================================

/// Players will get an ID that is unique within the agent context.
pub type Media_PlayerId = []const u8;
pub type Media_Timestamp = f64;
/// Have one type per entry in MediaLogRecord::Type
pub struct Media_PlayerMessage {
    level: []const u8 = "", // enum: "error"|"warning"|"info"|"debug"
    message: []const u8 = "",
}
/// Corresponds to kMediaPropertyChange
pub struct Media_PlayerProperty {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Corresponds to kMediaEventTriggered
pub struct Media_PlayerEvent {
    timestamp: Media_Timestamp = 0.0,
    value: []const u8 = "",
}
/// Represents logged source line numbers reported in an error.
pub struct Media_PlayerErrorSourceLocation {
    file: []const u8 = "",
    line: f64 = 0.0,
}
/// Corresponds to kMediaError
pub struct Media_PlayerError {
    errorType: []const u8 = "",
    code: f64 = 0.0,
    stack: []const Media_PlayerErrorSourceLocation = &.{},
    cause: []const Media_PlayerError = &.{},
    data: ProtocolStringMap = "",
}
pub struct Media_Player {
    playerId: Media_PlayerId = "",
    domNodeId: DOM_BackendNodeId? = null,
}
/// This can be called multiple times, and can be used to set / override /
pub struct Media_playerPropertiesChangedPayload {
    playerId: Media_PlayerId = "",
    properties: []const Media_PlayerProperty = &.{},
}
/// Send events as a list, allowing them to be batched on the browser for less
pub struct Media_playerEventsAddedPayload {
    playerId: Media_PlayerId = "",
    events: []const Media_PlayerEvent = &.{},
}
/// Send a list of any messages that need to be delivered.
pub struct Media_playerMessagesLoggedPayload {
    playerId: Media_PlayerId = "",
    messages: []const Media_PlayerMessage = &.{},
}
/// Send a list of any errors that need to be delivered.
pub struct Media_playerErrorsRaisedPayload {
    playerId: Media_PlayerId = "",
    errors: []const Media_PlayerError = &.{},
}
/// Called whenever a player is created, or when a new agent joins and receives
pub struct Media_playerCreatedPayload {
    player: Media_Player = .{},
}
/// Enables the Media domain
pub struct Media_enableParameters {}
pub struct Media_enableReturnValue {}
/// Disables the Media domain.
pub struct Media_disableParameters {}
pub struct Media_disableReturnValue {}

// ============================================================================
// Domain: Memory
// ============================================================================

/// Memory pressure level.
/// enum: "moderate"|"critical"
pub type Memory_PressureLevel = []const u8;
/// Heap profile sample.
pub struct Memory_SamplingProfileNode {
    size: f64 = 0.0,
    total: f64 = 0.0,
    stack: []const []const u8 = &.{},
}
/// Array of heap profile samples.
pub struct Memory_SamplingProfile {
    samples: []const Memory_SamplingProfileNode = &.{},
    modules: []const Memory_Module = &.{},
}
/// Executable module information
pub struct Memory_Module {
    name: []const u8 = "",
    uuid: []const u8 = "",
    baseAddress: []const u8 = "",
    size: f64 = 0.0,
}
/// DOM object counter data.
pub struct Memory_DOMCounter {
    name: []const u8 = "",
    count: f64 = 0.0,
}
/// Retruns current DOM object counters.
pub struct Memory_getDOMCountersParameters {}
pub struct Memory_getDOMCountersReturnValue {
    documents: f64 = 0.0,
    nodes: f64 = 0.0,
    jsEventListeners: f64 = 0.0,
}
/// Retruns DOM object counters after preparing renderer for leak detection.
pub struct Memory_getDOMCountersForLeakDetectionParameters {}
pub struct Memory_getDOMCountersForLeakDetectionReturnValue {
    counters: []const Memory_DOMCounter = &.{},
}
/// Prepares for leak detection by terminating workers, stopping spellcheckers,
pub struct Memory_prepareForLeakDetectionParameters {}
pub struct Memory_prepareForLeakDetectionReturnValue {}
/// Simulate OomIntervention by purging V8 memory.
pub struct Memory_forciblyPurgeJavaScriptMemoryParameters {}
pub struct Memory_forciblyPurgeJavaScriptMemoryReturnValue {}
/// Enable/disable suppressing memory pressure notifications in all processes.
pub struct Memory_setPressureNotificationsSuppressedParameters {
    suppressed: bool = false,
}
pub struct Memory_setPressureNotificationsSuppressedReturnValue {}
/// Simulate a memory pressure notification in all processes.
pub struct Memory_simulatePressureNotificationParameters {
    level: Memory_PressureLevel = "",
}
pub struct Memory_simulatePressureNotificationReturnValue {}
/// Start collecting native memory profile.
pub struct Memory_startSamplingParameters {
    samplingInterval: f64? = null,
    suppressRandomness: bool? = null,
}
pub struct Memory_startSamplingReturnValue {}
/// Stop collecting native memory profile.
pub struct Memory_stopSamplingParameters {}
pub struct Memory_stopSamplingReturnValue {}
/// Retrieve native memory allocations profile
pub struct Memory_getAllTimeSamplingProfileParameters {}
pub struct Memory_getAllTimeSamplingProfileReturnValue {
    profile: Memory_SamplingProfile = .{},
}
/// Retrieve native memory allocations profile
pub struct Memory_getBrowserSamplingProfileParameters {}
pub struct Memory_getBrowserSamplingProfileReturnValue {
    profile: Memory_SamplingProfile = .{},
}
/// Retrieve native memory allocations profile collected since last
pub struct Memory_getSamplingProfileParameters {}
pub struct Memory_getSamplingProfileReturnValue {
    profile: Memory_SamplingProfile = .{},
}
