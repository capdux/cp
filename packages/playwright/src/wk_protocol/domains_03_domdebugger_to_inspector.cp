// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue, ProtocolStringMap } from "./common";
import { Console_StackTrace, DOM_NodeId } from "./domains_02_console_to_dom";
import { Network_RequestId } from "./domains_04_layertree_to_network";
import { Runtime_ExecutionContextId, Runtime_ObjectPreview, Runtime_RemoteObject, Runtime_RemoteObjectId } from "./domains_06_runtime_to_worker";

// ============================================================================
// Domain: DOMDebugger
// DOM debugging allows setting breakpoints on particular DOM operations and events. JavaScript execution will stop on these operations as if there was a regular breakpoint set.
// ============================================================================

/// DOM breakpoint type.
/// enum: "subtree-modified"|"attribute-modified"|"node-removed"
pub type DOMDebugger_DOMBreakpointType = []const u8;
/// Event breakpoint type.
/// enum: "animation-frame"|"interval"|"listener"|"timeout"
pub type DOMDebugger_EventBreakpointType = []const u8;
/// Sets breakpoint on particular operation with DOM.
pub struct DOMDebugger_setDOMBreakpointParameters {
    nodeId: DOM_NodeId = 0.0,
    type_: DOMDebugger_DOMBreakpointType = "", // protocol field: "type"
    options: Debugger_BreakpointOptions? = null,
}
pub struct DOMDebugger_setDOMBreakpointReturnValue {}
/// Removes DOM breakpoint that was set using <code>setDOMBreakpoint</code>.
pub struct DOMDebugger_removeDOMBreakpointParameters {
    nodeId: DOM_NodeId = 0.0,
    type_: DOMDebugger_DOMBreakpointType = "", // protocol field: "type"
}
pub struct DOMDebugger_removeDOMBreakpointReturnValue {}
/// Sets breakpoint on particular event of given type.
pub struct DOMDebugger_setEventBreakpointParameters {
    breakpointType: DOMDebugger_EventBreakpointType = "",
    eventName: ([]const u8)? = null,
    caseSensitive: bool? = null,
    isRegex: bool? = null,
    options: Debugger_BreakpointOptions? = null,
}
pub struct DOMDebugger_setEventBreakpointReturnValue {}
/// Removes breakpoint on particular event of given type.
pub struct DOMDebugger_removeEventBreakpointParameters {
    breakpointType: DOMDebugger_EventBreakpointType = "",
    eventName: ([]const u8)? = null,
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct DOMDebugger_removeEventBreakpointReturnValue {}
/// Sets breakpoint on network activity for the given URL.
pub struct DOMDebugger_setURLBreakpointParameters {
    url: []const u8 = "",
    isRegex: bool? = null,
    options: Debugger_BreakpointOptions? = null,
}
pub struct DOMDebugger_setURLBreakpointReturnValue {}
/// Removes breakpoint from network activity for the given URL.
pub struct DOMDebugger_removeURLBreakpointParameters {
    url: []const u8 = "",
    isRegex: bool? = null,
}
pub struct DOMDebugger_removeURLBreakpointReturnValue {}

// ============================================================================
// Domain: DOMStorage
// Query and modify DOM storage.
// ============================================================================

/// DOM Storage identifier.
pub struct DOMStorage_StorageId {
    securityOrigin: []const u8 = "",
    isLocalStorage: bool = false,
}
/// DOM Storage item.
pub type DOMStorage_Item = []const []const u8;
pub struct DOMStorage_domStorageItemsClearedPayload {
    storageId: DOMStorage_StorageId = .{},
}
pub struct DOMStorage_domStorageItemRemovedPayload {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
}
pub struct DOMStorage_domStorageItemAddedPayload {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
    newValue: []const u8 = "",
}
pub struct DOMStorage_domStorageItemUpdatedPayload {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
    oldValue: []const u8 = "",
    newValue: []const u8 = "",
}
/// Enables storage tracking, storage events will now be delivered to the client.
pub struct DOMStorage_enableParameters {}
pub struct DOMStorage_enableReturnValue {}
/// Disables storage tracking, prevents storage events from being sent to the client.
pub struct DOMStorage_disableParameters {}
pub struct DOMStorage_disableReturnValue {}
pub struct DOMStorage_getDOMStorageItemsParameters {
    storageId: DOMStorage_StorageId = .{},
}
pub struct DOMStorage_getDOMStorageItemsReturnValue {
    entries: []const DOMStorage_Item = &.{},
}
pub struct DOMStorage_setDOMStorageItemParameters {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
    value: []const u8 = "",
}
pub struct DOMStorage_setDOMStorageItemReturnValue {}
pub struct DOMStorage_removeDOMStorageItemParameters {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
}
pub struct DOMStorage_removeDOMStorageItemReturnValue {}
pub struct DOMStorage_clearDOMStorageItemsParameters {
    storageId: DOMStorage_StorageId = .{},
}
pub struct DOMStorage_clearDOMStorageItemsReturnValue {}

// ============================================================================
// Domain: Debugger
// Debugger domain exposes JavaScript debugging capabilities. It allows setting and removing breakpoints, stepping through execution, exploring stack traces, etc.
// ============================================================================

/// Breakpoint identifier.
pub type Debugger_BreakpointId = []const u8;
/// Breakpoint action identifier.
pub type Debugger_BreakpointActionIdentifier = f64;
/// Unique script identifier.
pub type Debugger_ScriptId = []const u8;
/// enum: "program"|"module"|"webassembly"
pub type Debugger_ScriptType = []const u8;
/// Call frame identifier.
pub type Debugger_CallFrameId = []const u8;
/// Location in the source code.
pub struct Debugger_Location {
    scriptId: Debugger_ScriptId = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64? = null,
}
/// Action to perform when a breakpoint is triggered.
pub struct Debugger_BreakpointAction {
    type_: []const u8 = "", // protocol field: "type"; enum: "log"|"evaluate"|"sound"|"probe"
    data: ([]const u8)? = null,
    id: Debugger_BreakpointActionIdentifier? = null,
    emulateUserGesture: bool? = null,
}
/// Extra options that modify breakpoint behavior.
pub struct Debugger_BreakpointOptions {
    condition: ([]const u8)? = null,
    actions: ([]const Debugger_BreakpointAction)? = null,
    autoContinue: bool? = null,
    ignoreCount: f64? = null,
}
/// Information about the function.
pub struct Debugger_FunctionDetails {
    location: Debugger_Location = .{},
    name: ([]const u8)? = null,
    displayName: ([]const u8)? = null,
    scopeChain: ([]const Debugger_Scope)? = null,
}
/// JavaScript call frame. Array of call frames form the call stack.
pub struct Debugger_CallFrame {
    callFrameId: Debugger_CallFrameId = "",
    functionName: []const u8 = "",
    location: Debugger_Location = .{},
    scopeChain: []const Debugger_Scope = &.{},
    this: Runtime_RemoteObject = .{},
    isTailDeleted: bool = false,
}
/// Scope description.
pub struct Debugger_Scope {
    object: Runtime_RemoteObject = .{},
    type_: []const u8 = "", // protocol field: "type"; enum: "global"|"with"|"closure"|"catch"|"functionName"|"globalLexicalEnvironment"|"nestedLexical"
    name: ([]const u8)? = null,
    location: Debugger_Location? = null,
    empty: bool? = null,
}
/// A sample collected by evaluating a probe breakpoint action.
pub struct Debugger_ProbeSample {
    probeId: Debugger_BreakpointActionIdentifier = 0.0,
    sampleId: f64 = 0.0,
    batchId: f64 = 0.0,
    timestamp: f64 = 0.0,
    payload: Runtime_RemoteObject = .{},
}
/// The pause reason auxiliary data when paused because of an assertion.
pub struct Debugger_AssertPauseReason {
    message: ([]const u8)? = null,
}
/// The pause reason auxiliary data when paused because of hitting a breakpoint.
pub struct Debugger_BreakpointPauseReason {
    breakpointId: []const u8 = "",
}
/// The pause reason auxiliary data when paused because of a Content Security Policy directive.
pub struct Debugger_CSPViolationPauseReason {
    directive: []const u8 = "",
}
/// Called when global has been cleared and debugger client should reset its state. Happens upon navigation or reload.
pub type Debugger_globalObjectClearedPayload = ProtocolJsonValue;
/// Fired when virtual machine parses script. This event is also fired for all known and uncollected scripts upon enabling debugger.
pub struct Debugger_scriptParsedPayload {
    scriptId: Debugger_ScriptId = "",
    url: []const u8 = "",
    startLine: f64 = 0.0,
    startColumn: f64 = 0.0,
    endLine: f64 = 0.0,
    endColumn: f64 = 0.0,
    executionContextId: Runtime_ExecutionContextId = 0.0,
    scriptType: Debugger_ScriptType = "",
    isContentScript: bool? = null,
    sourceURL: ([]const u8)? = null,
    sourceMapURL: ([]const u8)? = null,
    displayName: ([]const u8)? = null,
    requestId: Network_RequestId? = null,
}
/// Fired when virtual machine fails to parse the script.
pub struct Debugger_scriptFailedToParsePayload {
    url: []const u8 = "",
    scriptSource: []const u8 = "",
    startLine: f64 = 0.0,
    errorLine: f64 = 0.0,
    errorMessage: []const u8 = "",
}
/// Fired when breakpoint is resolved to an actual script and location.
pub struct Debugger_breakpointResolvedPayload {
    breakpointId: Debugger_BreakpointId = "",
    location: Debugger_Location = .{},
}
/// Fired when the virtual machine stopped on breakpoint or exception or any other stop criteria.
pub struct Debugger_pausedPayload {
    callFrames: []const Debugger_CallFrame = &.{},
    reason: []const u8 = "", // enum: "URL"|"DOM"|"AnimationFrame"|"Interval"|"Listener"|"Timeout"|"exception"|"assert"|"CSPViolation"|"DebuggerStatement"|...
    data: ProtocolStringMap? = null,
    asyncStackTrace: Console_StackTrace? = null,
}
/// Fired when the virtual machine resumed execution.
pub type Debugger_resumedPayload = ProtocolJsonValue;
/// Fires when a new probe sample is collected.
pub struct Debugger_didSampleProbePayload {
    sample: Debugger_ProbeSample = .{},
}
/// Fired when a "sound" breakpoint action is triggered on a breakpoint.
pub struct Debugger_playBreakpointActionSoundPayload {
    breakpointActionId: Debugger_BreakpointActionIdentifier = 0.0,
}
/// Enables debugger for the given page. Clients should not assume that the debugging has been enabled until the result for this command is received.
pub struct Debugger_enableParameters {}
pub struct Debugger_enableReturnValue {}
/// Disables debugger for given page.
pub struct Debugger_disableParameters {}
pub struct Debugger_disableReturnValue {}
/// Set the async stack trace depth for the page. A value of zero disables recording of async stack traces.
pub struct Debugger_setAsyncStackTraceDepthParameters {
    depth: f64 = 0.0,
}
pub struct Debugger_setAsyncStackTraceDepthReturnValue {}
/// Activates / deactivates all breakpoints on the page.
pub struct Debugger_setBreakpointsActiveParameters {
    active: bool = false,
}
pub struct Debugger_setBreakpointsActiveReturnValue {}
/// Sets JavaScript breakpoint at given location specified either by URL or URL regex. Once this command is issued, all existing parsed scripts will have breakpoints resolved and returned in <code>locations</code> property. Further matching script parsing will result in subsequent <code>breakpointResolved</code> events issued. This logical breakpoint will survive page reloads.
pub struct Debugger_setBreakpointByUrlParameters {
    lineNumber: f64 = 0.0,
    url: ([]const u8)? = null,
    urlRegex: ([]const u8)? = null,
    columnNumber: f64? = null,
    options: Debugger_BreakpointOptions? = null,
}
pub struct Debugger_setBreakpointByUrlReturnValue {
    breakpointId: Debugger_BreakpointId = "",
    locations: []const Debugger_Location = &.{},
}
/// Sets JavaScript breakpoint at a given location.
pub struct Debugger_setBreakpointParameters {
    location: Debugger_Location = .{},
    options: Debugger_BreakpointOptions? = null,
}
pub struct Debugger_setBreakpointReturnValue {
    breakpointId: Debugger_BreakpointId = "",
    actualLocation: Debugger_Location = .{},
}
/// Removes JavaScript breakpoint.
pub struct Debugger_removeBreakpointParameters {
    breakpointId: Debugger_BreakpointId = "",
}
pub struct Debugger_removeBreakpointReturnValue {}
/// Adds a JavaScript breakpoint that pauses execution whenever a function with the given name is about to be called.
pub struct Debugger_addSymbolicBreakpointParameters {
    symbol: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
    options: Debugger_BreakpointOptions? = null,
}
pub struct Debugger_addSymbolicBreakpointReturnValue {}
/// Removes a previously added symbolic breakpoint.
pub struct Debugger_removeSymbolicBreakpointParameters {
    symbol: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Debugger_removeSymbolicBreakpointReturnValue {}
/// Continues execution until the current evaluation completes. This will trigger either a Debugger.paused or Debugger.resumed event.
pub struct Debugger_continueUntilNextRunLoopParameters {}
pub struct Debugger_continueUntilNextRunLoopReturnValue {}
/// Continues execution until specific location is reached. This will trigger either a Debugger.paused or Debugger.resumed event.
pub struct Debugger_continueToLocationParameters {
    location: Debugger_Location = .{},
}
pub struct Debugger_continueToLocationReturnValue {}
/// Steps over the expression. This will trigger either a Debugger.paused or Debugger.resumed event.
pub struct Debugger_stepNextParameters {}
pub struct Debugger_stepNextReturnValue {}
/// Steps over the statement. This will trigger either a Debugger.paused or Debugger.resumed event.
pub struct Debugger_stepOverParameters {}
pub struct Debugger_stepOverReturnValue {}
/// Steps into the function call. This will trigger either a Debugger.paused or Debugger.resumed event.
pub struct Debugger_stepIntoParameters {}
pub struct Debugger_stepIntoReturnValue {}
/// Steps out of the function call. This will trigger either a Debugger.paused or Debugger.resumed event.
pub struct Debugger_stepOutParameters {}
pub struct Debugger_stepOutReturnValue {}
/// Stops on the next JavaScript statement.
pub struct Debugger_pauseParameters {}
pub struct Debugger_pauseReturnValue {}
/// Resumes JavaScript execution. This will trigger a Debugger.resumed event.
pub struct Debugger_resumeParameters {}
pub struct Debugger_resumeReturnValue {}
/// Searches for given string in script content.
pub struct Debugger_searchInContentParameters {
    scriptId: Debugger_ScriptId = "",
    query: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Debugger_searchInContentReturnValue {
    result: []const GenericTypes_SearchMatch = &.{},
}
/// Returns source for the script with given id.
pub struct Debugger_getScriptSourceParameters {
    scriptId: Debugger_ScriptId = "",
}
pub struct Debugger_getScriptSourceReturnValue {
    scriptSource: []const u8 = "",
}
/// Returns detailed information on given function.
pub struct Debugger_getFunctionDetailsParameters {
    functionId: Runtime_RemoteObjectId = "",
}
pub struct Debugger_getFunctionDetailsReturnValue {
    details: Debugger_FunctionDetails = .{},
}
/// Returns a list of valid breakpoint locations within the given location range.
pub struct Debugger_getBreakpointLocationsParameters {
    start: Debugger_Location = .{},
    end: Debugger_Location = .{},
}
pub struct Debugger_getBreakpointLocationsReturnValue {
    locations: []const Debugger_Location = &.{},
}
/// Control whether the debugger pauses execution before `debugger` statements.
pub struct Debugger_setPauseOnDebuggerStatementsParameters {
    enabled: bool = false,
    options: Debugger_BreakpointOptions? = null,
}
pub struct Debugger_setPauseOnDebuggerStatementsReturnValue {}
/// Defines pause on exceptions state. Can be set to stop on all exceptions, uncaught exceptions or no exceptions. Initial pause on exceptions state is <code>none</code>.
pub struct Debugger_setPauseOnExceptionsParameters {
    state: []const u8 = "", // enum: "none"|"uncaught"|"all"
    options: Debugger_BreakpointOptions? = null,
}
pub struct Debugger_setPauseOnExceptionsReturnValue {}
/// Set pause on assertions state. Assertions are console.assert assertions.
pub struct Debugger_setPauseOnAssertionsParameters {
    enabled: bool = false,
    options: Debugger_BreakpointOptions? = null,
}
pub struct Debugger_setPauseOnAssertionsReturnValue {}
/// Pause when running the next JavaScript microtask.
pub struct Debugger_setPauseOnMicrotasksParameters {
    enabled: bool = false,
    options: Debugger_BreakpointOptions? = null,
}
pub struct Debugger_setPauseOnMicrotasksReturnValue {}
/// Change whether to pause in the debugger for internal scripts. The default value is false.
pub struct Debugger_setPauseForInternalScriptsParameters {
    shouldPause: bool = false,
}
pub struct Debugger_setPauseForInternalScriptsReturnValue {}
/// Evaluates expression on a given call frame.
pub struct Debugger_evaluateOnCallFrameParameters {
    callFrameId: Debugger_CallFrameId = "",
    expression: []const u8 = "",
    objectGroup: ([]const u8)? = null,
    includeCommandLineAPI: bool? = null,
    doNotPauseOnExceptionsAndMuteConsole: bool? = null,
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    saveResult: bool? = null,
    emulateUserGesture: bool? = null,
}
pub struct Debugger_evaluateOnCallFrameReturnValue {
    result: Runtime_RemoteObject = .{},
    wasThrown: bool? = null,
    savedResultIndex: f64? = null,
}
/// Sets whether the given URL should be in the list of blackboxed scripts, which are ignored when pausing.
pub struct Debugger_setShouldBlackboxURLParameters {
    url: []const u8 = "",
    shouldBlackbox: bool = false,
    caseSensitive: bool? = null,
    isRegex: bool? = null,
    sourceRanges: ([]const f64)? = null,
}
pub struct Debugger_setShouldBlackboxURLReturnValue {}
/// Sets whether evaluation of breakpoint conditions, ignore counts, and actions happen at the location of the breakpoint or are deferred due to blackboxing.
pub struct Debugger_setBlackboxBreakpointEvaluationsParameters {
    blackboxBreakpointEvaluations: bool = false,
}
pub struct Debugger_setBlackboxBreakpointEvaluationsReturnValue {}

// ============================================================================
// Domain: Dialog
// Actions and events related to alert boxes.
// ============================================================================

/// Fired when a JavaScript initiated dialog (alert, confirm, prompt, or onbeforeunload) is about to open.
pub struct Dialog_javascriptDialogOpeningPayload {
    type_: []const u8 = "", // protocol field: "type"
    message: []const u8 = "",
    defaultPrompt: ([]const u8)? = null,
}
/// Fired when a JavaScript initiated dialog (alert, confirm, prompt, or onbeforeunload) has been closed.
pub type Dialog_javascriptDialogClosedPayload = ProtocolJsonValue;
/// Enables dialog domain notifications.
pub struct Dialog_enableParameters {}
pub struct Dialog_enableReturnValue {}
/// Disables dialog domain notifications.
pub struct Dialog_disableParameters {}
pub struct Dialog_disableReturnValue {}
/// Accepts or dismisses a JavaScript initiated dialog (alert, confirm, prompt, or onbeforeunload).
pub struct Dialog_handleJavaScriptDialogParameters {
    accept: bool = false,
    promptText: ([]const u8)? = null,
}
pub struct Dialog_handleJavaScriptDialogReturnValue {}

// ============================================================================
// Domain: Emulation
// ============================================================================

/// Credentials for HTTP authentication.
pub struct Emulation_AuthCredentials {
    username: []const u8 = "",
    password: []const u8 = "",
    origin: ([]const u8)? = null,
}
/// Credentials for proxy authentication.
pub struct Emulation_ProxyCredentials {
    username: []const u8 = "",
    password: []const u8 = "",
}
/// Overrides device metrics with provided values.
pub struct Emulation_setDeviceMetricsOverrideParameters {
    width: f64 = 0.0,
    height: f64 = 0.0,
    fixedLayout: bool = false,
    deviceScaleFactor: f64? = null,
}
pub struct Emulation_setDeviceMetricsOverrideReturnValue {}
/// Clears device metrics overrides without resizing the browser window.
pub struct Emulation_clearDeviceMetricsOverrideParameters {}
pub struct Emulation_clearDeviceMetricsOverrideReturnValue {}
/// Allows to disable script execution for the page.
pub struct Emulation_setJavaScriptEnabledParameters {
    enabled: bool = false,
}
pub struct Emulation_setJavaScriptEnabledReturnValue {}
/// Credentials to use during HTTP authentication. The first credentials with matching origin are used.
pub struct Emulation_setAuthCredentialsParameters {
    credentials: []const Emulation_AuthCredentials = &.{},
    proxyCredentials: Emulation_ProxyCredentials? = null,
}
pub struct Emulation_setAuthCredentialsReturnValue {}
/// Makes page focused for test.
pub struct Emulation_setActiveAndFocusedParameters {
    active: bool? = null,
}
pub struct Emulation_setActiveAndFocusedReturnValue {}
/// Overrides the permissions.
pub struct Emulation_grantPermissionsParameters {
    origin: []const u8 = "",
    permissions: []const []const u8 = &.{},
}
pub struct Emulation_grantPermissionsReturnValue {}
/// Clears permission overrides.
pub struct Emulation_resetPermissionsParameters {}
pub struct Emulation_resetPermissionsReturnValue {}
/// Overrides window.orientation with provided value.
pub struct Emulation_setOrientationOverrideParameters {
    angle: f64? = null,
}
pub struct Emulation_setOrientationOverrideReturnValue {}

// ============================================================================
// Domain: GenericTypes
// Exposes generic types to be used by any domain.
// ============================================================================

/// Search match in a resource.
pub struct GenericTypes_SearchMatch {
    lineNumber: f64 = 0.0,
    lineContent: []const u8 = "",
}
/// A two-dimensional size.
pub struct GenericTypes_Size {
    width: f64 = 0.0,
    height: f64 = 0.0,
}

// ============================================================================
// Domain: Heap
// Heap domain exposes JavaScript heap attributes and capabilities.
// ============================================================================

/// Information about a garbage collection.
pub struct Heap_GarbageCollection {
    type_: []const u8 = "", // protocol field: "type"; enum: "full"|"partial"
    startTime: f64 = 0.0,
    endTime: f64 = 0.0,
}
/// JavaScriptCore HeapSnapshot JSON data.
pub type Heap_HeapSnapshotData = []const u8;
/// Information about the garbage collection.
pub struct Heap_garbageCollectedPayload {
    collection: Heap_GarbageCollection = .{},
}
/// Tracking started.
pub struct Heap_trackingStartPayload {
    timestamp: f64 = 0.0,
    snapshotData: Heap_HeapSnapshotData = "",
}
/// Tracking stopped.
pub struct Heap_trackingCompletePayload {
    timestamp: f64 = 0.0,
    snapshotData: Heap_HeapSnapshotData = "",
}
/// Enables Heap domain events.
pub struct Heap_enableParameters {}
pub struct Heap_enableReturnValue {}
/// Disables Heap domain events.
pub struct Heap_disableParameters {}
pub struct Heap_disableReturnValue {}
/// Trigger a full garbage collection.
pub struct Heap_gcParameters {}
pub struct Heap_gcReturnValue {}
/// Take a heap snapshot.
pub struct Heap_snapshotParameters {}
pub struct Heap_snapshotReturnValue {
    timestamp: f64 = 0.0,
    snapshotData: Heap_HeapSnapshotData = "",
}
/// Start tracking heap changes. This will produce a `trackingStart` event.
pub struct Heap_startTrackingParameters {}
pub struct Heap_startTrackingReturnValue {}
/// Stop tracking heap changes. This will produce a `trackingComplete` event.
pub struct Heap_stopTrackingParameters {}
pub struct Heap_stopTrackingReturnValue {}
/// Returns a preview (string, Debugger.FunctionDetails, or Runtime.ObjectPreview) for a Heap.HeapObjectId.
pub struct Heap_getPreviewParameters {
    heapObjectId: f64 = 0.0,
}
pub struct Heap_getPreviewReturnValue {
    string_: ([]const u8)? = null, // protocol field: "string"
    functionDetails: Debugger_FunctionDetails? = null,
    preview: Runtime_ObjectPreview? = null,
}
/// Returns the strongly referenced Runtime.RemoteObject for a Heap.HeapObjectId.
pub struct Heap_getRemoteObjectParameters {
    heapObjectId: f64 = 0.0,
    objectGroup: ([]const u8)? = null,
}
pub struct Heap_getRemoteObjectReturnValue {
    result: Runtime_RemoteObject = .{},
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
/// Enables events from backend.
pub struct IndexedDB_enableParameters {}
pub struct IndexedDB_enableReturnValue {}
/// Disables events from backend.
pub struct IndexedDB_disableParameters {}
pub struct IndexedDB_disableReturnValue {}
/// Requests database names for given security origin.
pub struct IndexedDB_requestDatabaseNamesParameters {
    securityOrigin: []const u8 = "",
}
pub struct IndexedDB_requestDatabaseNamesReturnValue {
    databaseNames: []const []const u8 = &.{},
}
/// Requests database with given name in given frame.
pub struct IndexedDB_requestDatabaseParameters {
    securityOrigin: []const u8 = "",
    databaseName: []const u8 = "",
}
pub struct IndexedDB_requestDatabaseReturnValue {
    databaseWithObjectStores: IndexedDB_DatabaseWithObjectStores = .{},
}
/// Requests data from object store or index.
pub struct IndexedDB_requestDataParameters {
    securityOrigin: []const u8 = "",
    databaseName: []const u8 = "",
    objectStoreName: []const u8 = "",
    indexName: []const u8 = "",
    skipCount: f64 = 0.0,
    pageSize: f64 = 0.0,
    keyRange: IndexedDB_KeyRange? = null,
}
pub struct IndexedDB_requestDataReturnValue {
    objectStoreDataEntries: []const IndexedDB_DataEntry = &.{},
    hasMore: bool = false,
}
/// Clears all entries from an object store.
pub struct IndexedDB_clearObjectStoreParameters {
    securityOrigin: []const u8 = "",
    databaseName: []const u8 = "",
    objectStoreName: []const u8 = "",
}
pub struct IndexedDB_clearObjectStoreReturnValue {}

// ============================================================================
// Domain: Input
// ============================================================================

/// UTC time in seconds, counted from January 1, 1970.
pub type Input_TimeSinceEpoch = f64;
/// Touch point.
pub struct Input_TouchPoint {
    x: f64 = 0.0,
    y: f64 = 0.0,
    id: f64 = 0.0,
}
/// Dispatches a key event to the page.
pub struct Input_dispatchKeyEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "keyDown"|"keyUp"
    modifiers: f64? = null,
    text: ([]const u8)? = null,
    unmodifiedText: ([]const u8)? = null,
    code: ([]const u8)? = null,
    key: ([]const u8)? = null,
    windowsVirtualKeyCode: f64? = null,
    nativeVirtualKeyCode: f64? = null,
    autoRepeat: bool? = null,
    isKeypad: bool? = null,
    isSystemKey: bool? = null,
    macCommands: ([]const []const u8)? = null,
}
pub struct Input_dispatchKeyEventReturnValue {}
/// Dispatches a mouse event to the page.
pub struct Input_dispatchMouseEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "move"|"down"|"up"|"wheel"
    x: f64 = 0.0,
    y: f64 = 0.0,
    modifiers: f64? = null,
    button: ([]const u8)? = null, // enum: "none"|"left"|"middle"|"right"|"back"|"forward"
    buttons: f64? = null,
    clickCount: f64? = null,
}
pub struct Input_dispatchMouseEventReturnValue {}
/// Dispatches a wheel event to the page.
pub struct Input_dispatchWheelEventParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    modifiers: f64? = null,
    deltaX: f64? = null,
    deltaY: f64? = null,
}
pub struct Input_dispatchWheelEventReturnValue {}
/// Dispatches a tap event to the page.
pub struct Input_dispatchTapEventParameters {
    x: f64 = 0.0,
    y: f64 = 0.0,
    modifiers: f64? = null,
}
pub struct Input_dispatchTapEventReturnValue {}
/// Dispatches a touch event to the page.
pub struct Input_dispatchTouchEventParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "touchStart"|"touchMove"|"touchEnd"|"touchCancel"
    modifiers: f64? = null,
    touchPoints: ([]const Input_TouchPoint)? = null,
}
pub struct Input_dispatchTouchEventReturnValue {}

// ============================================================================
// Domain: Inspector
// ============================================================================

pub struct Inspector_evaluateForTestInFrontendPayload {
    script: []const u8 = "",
}
pub struct Inspector_inspectPayload {
    object: Runtime_RemoteObject = .{},
    hints: ProtocolStringMap = "",
}
/// Enables inspector domain notifications.
pub struct Inspector_enableParameters {}
pub struct Inspector_enableReturnValue {}
/// Disables inspector domain notifications.
pub struct Inspector_disableParameters {}
pub struct Inspector_disableReturnValue {}
/// Sent by the frontend after all initialization messages have been sent.
pub struct Inspector_initializedParameters {}
pub struct Inspector_initializedReturnValue {}
