// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts | Synced: 2026-11-28

import { ProtocolJsonValue, ProtocolStringMap } from "./common";
import { Debugger_ScriptId } from "./domains_03_domdebugger_to_inspector";
import { Network_FrameId, Network_Walltime } from "./domains_04_layertree_to_network";

// ============================================================================
// Domain: Runtime
// Runtime domain exposes JavaScript runtime by means of remote evaluation and mirror objects. Evaluation results are returned as mirror object that expose object type, string representation and unique identifier that can be used for further object reference. Original objects are maintained in memory unless they are either explicitly released or are released along with the other objects in their object group.
// ============================================================================

/// Unique object identifier.
pub type Runtime_RemoteObjectId = []const u8;
/// Mirror object referencing original JavaScript object.
pub struct Runtime_RemoteObject {
    type_: []const u8 = "", // protocol field: "type"; enum: "object"|"function"|"undefined"|"string"|"number"|"boolean"|"symbol"|"bigint"
    subtype: ([]const u8)? = null, // enum: "array"|"null"|"node"|"regexp"|"date"|"error"|"map"|"set"|"weakmap"|"weakset"|"iterator"|"class"|"proxy"|"weakref"
    className: ([]const u8)? = null,
    value: ProtocolJsonValue? = null,
    description: ([]const u8)? = null,
    objectId: Runtime_RemoteObjectId? = null,
    size: f64? = null,
    classPrototype: (*const Runtime_RemoteObject)? = null,
    preview: Runtime_ObjectPreview? = null,
}
/// Object containing abbreviated remote object value.
pub struct Runtime_ObjectPreview {
    type_: []const u8 = "", // protocol field: "type"; enum: "object"|"function"|"undefined"|"string"|"number"|"boolean"|"symbol"|"bigint"
    subtype: ([]const u8)? = null, // enum: "array"|"null"|"node"|"regexp"|"date"|"error"|"map"|"set"|"weakmap"|"weakset"|"iterator"|"class"|"proxy"|"weakref"
    description: ([]const u8)? = null,
    lossless: bool = false,
    overflow: bool? = null,
    properties: ([]const Runtime_PropertyPreview)? = null,
    entries: ([]const Runtime_EntryPreview)? = null,
    size: f64? = null,
}
pub struct Runtime_PropertyPreview {
    name: []const u8 = "",
    type_: []const u8 = "", // protocol field: "type"; enum: "object"|"function"|"undefined"|"string"|"number"|"boolean"|"symbol"|"bigint"|"accessor"
    subtype: ([]const u8)? = null, // enum: "array"|"null"|"node"|"regexp"|"date"|"error"|"map"|"set"|"weakmap"|"weakset"|"iterator"|"class"|"proxy"|"weakref"
    value: ([]const u8)? = null,
    valuePreview: Runtime_ObjectPreview? = null,
    isPrivate: bool? = null,
    internal: bool? = null,
}
pub struct Runtime_EntryPreview {
    key: Runtime_ObjectPreview? = null,
    value: Runtime_ObjectPreview = .{},
}
pub struct Runtime_CollectionEntry {
    key: Runtime_RemoteObject? = null,
    value: Runtime_RemoteObject = .{},
}
/// Object property descriptor.
pub struct Runtime_PropertyDescriptor {
    name: []const u8 = "",
    value: Runtime_RemoteObject? = null,
    writable: bool? = null,
    get: Runtime_RemoteObject? = null,
    set: Runtime_RemoteObject? = null,
    wasThrown: bool? = null,
    configurable: bool? = null,
    enumerable: bool? = null,
    isOwn: bool? = null,
    symbol: Runtime_RemoteObject? = null,
    isPrivate: bool? = null,
    nativeGetter: bool? = null,
}
/// Object internal property descriptor. This property isn't normally visible in JavaScript code.
pub struct Runtime_InternalPropertyDescriptor {
    name: []const u8 = "",
    value: Runtime_RemoteObject? = null,
}
/// Represents function call argument. Either remote object id <code>objectId</code> or primitive <code>value</code> or neither of (for undefined) them should be specified.
pub struct Runtime_CallArgument {
    value: ProtocolJsonValue? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
/// Id of an execution context.
pub type Runtime_ExecutionContextId = f64;
/// Type of the execution context.
/// enum: "normal"|"user"|"internal"
pub type Runtime_ExecutionContextType = []const u8;
/// Description of an isolated world.
pub struct Runtime_ExecutionContextDescription {
    id: Runtime_ExecutionContextId = 0.0,
    type_: Runtime_ExecutionContextType = "", // protocol field: "type"
    name: []const u8 = "",
    frameId: Network_FrameId = "",
}
/// Syntax error type: "none" for no error, "irrecoverable" for unrecoverable errors, "unterminated-literal" for when there is an unterminated literal, "recoverable" for when the expression is unfinished but valid so far.
/// enum: "none"|"irrecoverable"|"unterminated-literal"|"recoverable"
pub type Runtime_SyntaxErrorType = []const u8;
/// Range of an error in source code.
pub struct Runtime_ErrorRange {
    startOffset: f64 = 0.0,
    endOffset: f64 = 0.0,
}
pub struct Runtime_StructureDescription {
    fields: ([]const []const u8)? = null,
    optionalFields: ([]const []const u8)? = null,
    constructorName: ([]const u8)? = null,
    prototypeStructure: (*const Runtime_StructureDescription)? = null,
    isImprecise: bool? = null,
}
pub struct Runtime_TypeSet {
    isFunction: bool = false,
    isUndefined: bool = false,
    isNull: bool = false,
    isBoolean: bool = false,
    isInteger: bool = false,
    isNumber: bool = false,
    isString: bool = false,
    isObject: bool = false,
    isSymbol: bool = false,
    isBigInt: bool = false,
}
/// Container for type information that has been gathered.
pub struct Runtime_TypeDescription {
    isValid: bool = false,
    leastCommonAncestor: ([]const u8)? = null,
    typeSet: Runtime_TypeSet? = null,
    structures: ([]const Runtime_StructureDescription)? = null,
    isTruncated: bool? = null,
}
/// Describes the location of an expression we want type information for.
pub struct Runtime_TypeLocation {
    typeInformationDescriptor: f64 = 0.0,
    sourceID: []const u8 = "",
    divot: f64 = 0.0,
}
/// From Wikipedia: a basic block is a portion of the code within a program with only one entry point and only one exit point. This type gives the location of a basic block and if that basic block has executed.
pub struct Runtime_BasicBlock {
    startOffset: f64 = 0.0,
    endOffset: f64 = 0.0,
    hasExecuted: bool = false,
    executionCount: f64 = 0.0,
}
/// Issued when new execution context is created.
pub struct Runtime_executionContextCreatedPayload {
    context: Runtime_ExecutionContextDescription = .{},
}
/// Issued when new execution context is created.
pub struct Runtime_bindingCalledPayload {
    contextId: Runtime_ExecutionContextId = 0.0,
    name: []const u8 = "",
    argument: []const u8 = "",
}
/// Parses JavaScript source code for errors.
pub struct Runtime_parseParameters {
    source: []const u8 = "",
}
pub struct Runtime_parseReturnValue {
    result: Runtime_SyntaxErrorType = "",
    message: ([]const u8)? = null,
    range: Runtime_ErrorRange? = null,
}
/// Evaluates expression on global object.
pub struct Runtime_evaluateParameters {
    expression: []const u8 = "",
    objectGroup: ([]const u8)? = null,
    includeCommandLineAPI: bool? = null,
    doNotPauseOnExceptionsAndMuteConsole: bool? = null,
    contextId: Runtime_ExecutionContextId? = null,
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    saveResult: bool? = null,
    emulateUserGesture: bool? = null,
}
pub struct Runtime_evaluateReturnValue {
    result: Runtime_RemoteObject = .{},
    wasThrown: bool? = null,
    savedResultIndex: f64? = null,
}
/// Calls the async callback when the promise with the given ID gets settled.
pub struct Runtime_awaitPromiseParameters {
    promiseObjectId: Runtime_RemoteObjectId = "",
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    saveResult: bool? = null,
}
pub struct Runtime_awaitPromiseReturnValue {
    result: Runtime_RemoteObject = .{},
    wasThrown: bool? = null,
    savedResultIndex: f64? = null,
}
/// Calls function with given declaration on the given object. Object group of the result is inherited from the target object.
pub struct Runtime_callFunctionOnParameters {
    objectId: Runtime_RemoteObjectId = "",
    functionDeclaration: []const u8 = "",
    arguments: ([]const Runtime_CallArgument)? = null,
    doNotPauseOnExceptionsAndMuteConsole: bool? = null,
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    emulateUserGesture: bool? = null,
    awaitPromise: bool? = null,
}
pub struct Runtime_callFunctionOnReturnValue {
    result: Runtime_RemoteObject = .{},
    wasThrown: bool? = null,
}
/// Adds binding with the given name on the global objects of all inspected contexts. Each binding function call produces Runtime.bindingCalled event.
pub struct Runtime_addBindingParameters {
    name: []const u8 = "",
}
pub struct Runtime_addBindingReturnValue {}
/// Returns a preview for the given object.
pub struct Runtime_getPreviewParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct Runtime_getPreviewReturnValue {
    preview: Runtime_ObjectPreview = .{},
}
/// Returns properties of a given object. Object group of the result is inherited from the target object.
pub struct Runtime_getPropertiesParameters {
    objectId: Runtime_RemoteObjectId = "",
    ownProperties: bool? = null,
    fetchStart: f64? = null,
    fetchCount: f64? = null,
    generatePreview: bool? = null,
}
pub struct Runtime_getPropertiesReturnValue {
    properties: []const Runtime_PropertyDescriptor = &.{},
    internalProperties: ([]const Runtime_InternalPropertyDescriptor)? = null,
}
/// Returns displayable properties of a given object. Object group of the result is inherited from the target object. Displayable properties are own properties, internal properties, and native getters in the prototype chain (assumed to be bindings and treated like own properties for the frontend).
pub struct Runtime_getDisplayablePropertiesParameters {
    objectId: Runtime_RemoteObjectId = "",
    fetchStart: f64? = null,
    fetchCount: f64? = null,
    generatePreview: bool? = null,
}
pub struct Runtime_getDisplayablePropertiesReturnValue {
    properties: []const Runtime_PropertyDescriptor = &.{},
    internalProperties: ([]const Runtime_InternalPropertyDescriptor)? = null,
}
/// Returns entries of given Map / Set collection.
pub struct Runtime_getCollectionEntriesParameters {
    objectId: Runtime_RemoteObjectId = "",
    objectGroup: ([]const u8)? = null,
    fetchStart: f64? = null,
    fetchCount: f64? = null,
}
pub struct Runtime_getCollectionEntriesReturnValue {
    entries: []const Runtime_CollectionEntry = &.{},
}
/// Assign a saved result index to this value.
pub struct Runtime_saveResultParameters {
    value: Runtime_CallArgument = .{},
    contextId: Runtime_ExecutionContextId? = null,
}
pub struct Runtime_saveResultReturnValue {
    savedResultIndex: f64? = null,
}
/// Creates an additional reference to all saved values in the Console using the the given string as a prefix instead of $.
pub struct Runtime_setSavedResultAliasParameters {
    alias: ([]const u8)? = null,
}
pub struct Runtime_setSavedResultAliasReturnValue {}
/// Releases remote object with given id.
pub struct Runtime_releaseObjectParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct Runtime_releaseObjectReturnValue {}
/// Releases all remote objects that belong to a given group.
pub struct Runtime_releaseObjectGroupParameters {
    objectGroup: []const u8 = "",
}
pub struct Runtime_releaseObjectGroupReturnValue {}
/// Enables reporting of execution contexts creation by means of <code>executionContextCreated</code> event. When the reporting gets enabled the event will be sent immediately for each existing execution context.
pub struct Runtime_enableParameters {}
pub struct Runtime_enableReturnValue {}
/// Disables reporting of execution contexts creation.
pub struct Runtime_disableParameters {}
pub struct Runtime_disableReturnValue {}
/// Returns detailed information on the given function.
pub struct Runtime_getRuntimeTypesForVariablesAtOffsetsParameters {
    locations: []const Runtime_TypeLocation = &.{},
}
pub struct Runtime_getRuntimeTypesForVariablesAtOffsetsReturnValue {
    types: []const Runtime_TypeDescription = &.{},
}
/// Enables type profiling on the VM.
pub struct Runtime_enableTypeProfilerParameters {}
pub struct Runtime_enableTypeProfilerReturnValue {}
/// Disables type profiling on the VM.
pub struct Runtime_disableTypeProfilerParameters {}
pub struct Runtime_disableTypeProfilerReturnValue {}
/// Enables control flow profiling on the VM.
pub struct Runtime_enableControlFlowProfilerParameters {}
pub struct Runtime_enableControlFlowProfilerReturnValue {}
/// Disables control flow profiling on the VM.
pub struct Runtime_disableControlFlowProfilerParameters {}
pub struct Runtime_disableControlFlowProfilerReturnValue {}
/// Returns a list of basic blocks for the given sourceID with information about their text ranges and whether or not they have executed.
pub struct Runtime_getBasicBlocksParameters {
    sourceID: []const u8 = "",
}
pub struct Runtime_getBasicBlocksReturnValue {
    basicBlocks: []const Runtime_BasicBlock = &.{},
}

// ============================================================================
// Domain: Screencast
// ============================================================================

/// Unique identifier of the screencast.
pub type Screencast_ScreencastId = []const u8;
pub struct Screencast_screencastFramePayload {
    data: []const u8 = "",
    timestamp: f64 = 0.0,
    deviceWidth: f64 = 0.0,
    deviceHeight: f64 = 0.0,
}
/// Starts screencast.
pub struct Screencast_startScreencastParameters {
    width: f64 = 0.0,
    height: f64 = 0.0,
    toolbarHeight: f64 = 0.0,
    quality: f64 = 0.0,
}
pub struct Screencast_startScreencastReturnValue {
    generation: f64 = 0.0,
}
/// Stops screencast.
pub struct Screencast_stopScreencastParameters {}
pub struct Screencast_stopScreencastReturnValue {}
pub struct Screencast_screencastFrameAckParameters {
    generation: f64 = 0.0,
}
pub struct Screencast_screencastFrameAckReturnValue {}

// ============================================================================
// Domain: ScriptProfiler
// Profiler domain exposes JavaScript evaluation timing and profiling.
// ============================================================================

/// enum: "API"|"Microtask"|"Other"
pub type ScriptProfiler_EventType = []const u8;
pub struct ScriptProfiler_Event {
    startTime: f64 = 0.0,
    endTime: f64 = 0.0,
    type_: ScriptProfiler_EventType = "", // protocol field: "type"
}
pub struct ScriptProfiler_ExpressionLocation {
    line: f64 = 0.0,
    column: f64 = 0.0,
}
pub struct ScriptProfiler_StackFrame {
    sourceID: Debugger_ScriptId = "",
    name: []const u8 = "",
    line: f64 = 0.0,
    column: f64 = 0.0,
    url: []const u8 = "",
    expressionLocation: ScriptProfiler_ExpressionLocation? = null,
}
pub struct ScriptProfiler_StackTrace {
    timestamp: f64 = 0.0,
    stackFrames: []const ScriptProfiler_StackFrame = &.{},
}
pub struct ScriptProfiler_Samples {
    stackTraces: []const ScriptProfiler_StackTrace = &.{},
}
/// Tracking started.
pub struct ScriptProfiler_trackingStartPayload {
    timestamp: f64 = 0.0,
}
/// Periodic tracking updates with event data.
pub struct ScriptProfiler_trackingUpdatePayload {
    event: ScriptProfiler_Event = .{},
}
/// Tracking stopped. Includes any buffered data during tracking, such as profiling information.
pub struct ScriptProfiler_trackingCompletePayload {
    timestamp: f64 = 0.0,
    samples: ScriptProfiler_Samples? = null,
}
/// Start tracking script evaluations.
pub struct ScriptProfiler_startTrackingParameters {
    includeSamples: bool? = null,
}
pub struct ScriptProfiler_startTrackingReturnValue {}
/// Stop tracking script evaluations. This will produce a `trackingComplete` event.
pub struct ScriptProfiler_stopTrackingParameters {}
pub struct ScriptProfiler_stopTrackingReturnValue {}

// ============================================================================
// Domain: Security
// Security domain allows the frontend to query for information relating to the security of the page (e.g. HTTPS info, TLS info, user activity, etc.).
// ============================================================================

/// Information about a SSL connection to display in the frontend.
pub struct Security_Connection {
    protocol: ([]const u8)? = null,
    cipher: ([]const u8)? = null,
}
/// Information about a SSL certificate to display in the frontend.
pub struct Security_Certificate {
    subject: ([]const u8)? = null,
    validFrom: Network_Walltime? = null,
    validUntil: Network_Walltime? = null,
    dnsNames: ([]const []const u8)? = null,
    ipAddresses: ([]const []const u8)? = null,
}
/// Security information for a given Network.Response.
pub struct Security_Security {
    connection: Security_Connection? = null,
    certificate: Security_Certificate? = null,
}

// ============================================================================
// Domain: ServiceWorker
// Actions and events related to the inspected service worker.
// ============================================================================

/// ServiceWorker metadata and initial state.
pub struct ServiceWorker_Configuration {
    targetId: []const u8 = "",
    securityOrigin: []const u8 = "",
    url: []const u8 = "",
    content: []const u8 = "",
}
/// Returns the initialization information for this target.
pub struct ServiceWorker_getInitializationInfoParameters {}
pub struct ServiceWorker_getInitializationInfoReturnValue {
    info: ServiceWorker_Configuration = .{},
}

// ============================================================================
// Domain: Storage
// Query and modify storage (cookies for now) under Site Isolation. Reads and deletes are scoped to the cookies the inspected page's frames can access, as Page.getCookies does without Site Isolation: for each frame, including cross-origin iframes in other processes, the cookies its document would send to its own URL and to its loaded same-site subresources' URLs, each labeled with its partitionKey. setCookie is not scoped.
// ============================================================================

/// Same-Site policy of a cookie.
/// enum: "None"|"Lax"|"Strict"
pub type Storage_CookieSameSitePolicy = []const u8;
/// Cookie object.
pub struct Storage_Cookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "",
    expires: f64 = 0.0,
    session: bool = false,
    httpOnly: bool = false,
    secure: bool = false,
    sameSite: Storage_CookieSameSitePolicy = "",
    partitionKey: ([]const u8)? = null,
}
/// Filter parameters for cookie retrieval and deletion. Every provided field narrows the result set.
pub struct Storage_CookieFilter {
    name: ([]const u8)? = null,
    value: ([]const u8)? = null,
    domain: ([]const u8)? = null,
    path: ([]const u8)? = null,
    httpOnly: bool? = null,
    secure: bool? = null,
    partitionKey: ([]const u8)? = null,
}
/// Identifies the storage partition a cookie belongs to. At least one of 'userContext' or 'sourceOrigin' is present.
pub struct Storage_PartitionKey {
    userContext: ([]const u8)? = null,
    sourceOrigin: ([]const u8)? = null,
}
/// The type of storage partition descriptor.
/// enum: "context"
pub type Storage_PartitionDescriptorType = []const u8;
/// Describes a storage partition. Omit to target the inspected page's default data store. Type 'context' targets the inspected page's main-frame origin.
pub struct Storage_PartitionDescriptor {
    type_: Storage_PartitionDescriptorType = "", // protocol field: "type"
}
/// Marks the Storage domain enabled for this target. No tracking is started; cookies are read on demand from the authoritative store, so this is effectively a no-op kept for domain-lifecycle symmetry.
pub struct Storage_enableParameters {}
pub struct Storage_enableReturnValue {}
/// Marks the Storage domain disabled for this target. Counterpart to enable; a no-op beyond lifecycle bookkeeping.
pub struct Storage_disableParameters {}
pub struct Storage_disableReturnValue {}
/// Retrieves zero or more cookies which match the provided filter, within the given partition.
pub struct Storage_getCookiesParameters {
    filter: Storage_CookieFilter? = null,
    partition: Storage_PartitionDescriptor? = null,
}
pub struct Storage_getCookiesReturnValue {
    cookies: []const Storage_Cookie = &.{},
    partitionKey: Storage_PartitionKey = .{},
}
/// Creates a new cookie, replacing any cookie in the partition which matches.
pub struct Storage_setCookieParameters {
    cookie: Storage_Cookie = .{},
    partition: Storage_PartitionDescriptor? = null,
}
pub struct Storage_setCookieReturnValue {
    partitionKey: Storage_PartitionKey = .{},
}
/// Removes zero or more cookies which match the provided filter, within the given partition. A filter is required; omitting it fails rather than deleting every cookie.
pub struct Storage_deleteCookiesParameters {
    filter: Storage_CookieFilter? = null,
    partition: Storage_PartitionDescriptor? = null,
}
pub struct Storage_deleteCookiesReturnValue {
    partitionKey: Storage_PartitionKey = .{},
}

// ============================================================================
// Domain: Target
// ============================================================================

/// Description of a target.
pub struct Target_TargetInfo {
    targetId: []const u8 = "",
    type_: []const u8 = "", // protocol field: "type"; enum: "page"|"frame"|"service-worker"|"worker"
    isProvisional: bool? = null,
    isPaused: bool? = null,
    processIdentifier: ([]const u8)? = null,
}
/// State of a debugger pause session. While it is paused, the UI process holds every page of the browsing context group: each frame is either stopped or held. Closed means the hold was released, not that no frame is stopped: a frame can stay in the debugger, so Debugger.paused and Debugger.resumed remain the source of truth for which frames are stopped.
/// enum: "paused"|"closed"
pub type Target_DebuggerPauseSessionState = []const u8;
pub struct Target_targetCreatedPayload {
    targetInfo: Target_TargetInfo = .{},
}
pub struct Target_targetDestroyedPayload {
    targetId: []const u8 = "",
    crashed: bool = false,
}
/// This event is fired when provisional load is committed. Provisional target swaps with the current target.
pub struct Target_didCommitProvisionalTargetPayload {
    oldTargetId: []const u8 = "",
    newTargetId: []const u8 = "",
}
/// Fired when a debugger pause session opens, when the set of frames whose pause has not ended changes, and when the hold is released.
pub struct Target_debuggerPauseSessionChangedPayload {
    sessionId: []const u8 = "",
    state: Target_DebuggerPauseSessionState = "",
    stoppingTargetIds: []const []const u8 = &.{},
}
pub struct Target_dispatchMessageFromTargetPayload {
    targetId: []const u8 = "",
    message: []const u8 = "",
}
/// If set to true, new targets will be paused on start waiting for resume command. Other commands can be dispatched on the target before it is resumed.
pub struct Target_setPauseOnStartParameters {
    pauseOnStart: bool = false,
}
pub struct Target_setPauseOnStartReturnValue {}
/// Will resume target if it was paused on start.
pub struct Target_resumeParameters {
    targetId: []const u8 = "",
}
pub struct Target_resumeReturnValue {}
/// Send an Inspector Protocol message to be dispatched to a Target's agents.
pub struct Target_sendMessageToTargetParameters {
    targetId: []const u8 = "",
    message: []const u8 = "",
}
pub struct Target_sendMessageToTargetReturnValue {}
/// Reveals the target on screen.
pub struct Target_activateParameters {
    targetId: []const u8 = "",
}
pub struct Target_activateReturnValue {}

// ============================================================================
// Domain: Timeline
// Timeline provides its clients with instrumentation records that are generated during the page runtime. Timeline instrumentation can be started and stopped using corresponding commands. While timeline is started, it is generating timeline event records.
// ============================================================================

/// Timeline record type.
/// enum: "EventDispatch"|"ScheduleStyleRecalculation"|"RecalculateStyles"|"InvalidateLayout"|"ScheduleLayout"|"Layout"|"Paint"...
pub type Timeline_EventType = []const u8;
/// Instrument types.
/// enum: "ScriptProfiler"|"Timeline"|"CPU"|"Memory"|"Heap"|"Animation"|"Screenshot"
pub type Timeline_Instrument = []const u8;
/// Timeline record contains information about the recorded activity.
pub struct Timeline_TimelineEvent {
    type_: Timeline_EventType = "", // protocol field: "type"
    data: ProtocolStringMap = "",
    children: ([]const Timeline_TimelineEvent)? = null,
}
/// Fired for every instrumentation event while timeline is started.
pub struct Timeline_eventRecordedPayload {
    record: Timeline_TimelineEvent = .{},
}
/// Fired when recording has started.
pub struct Timeline_recordingStartedPayload {
    startTime: f64 = 0.0,
}
/// Fired when recording has stopped.
pub struct Timeline_recordingStoppedPayload {
    endTime: f64 = 0.0,
}
/// Fired when auto capture started.
pub type Timeline_autoCaptureStartedPayload = ProtocolJsonValue;
/// Enables Timeline domain events.
pub struct Timeline_enableParameters {}
pub struct Timeline_enableReturnValue {}
/// Disables Timeline domain events.
pub struct Timeline_disableParameters {}
pub struct Timeline_disableReturnValue {}
/// Starts capturing instrumentation events.
pub struct Timeline_startParameters {
    maxCallStackDepth: f64? = null,
}
pub struct Timeline_startReturnValue {}
/// Stops capturing instrumentation events.
pub struct Timeline_stopParameters {}
pub struct Timeline_stopReturnValue {}
/// Toggle auto capture state. If <code>true</code> the backend will disable breakpoints and start capturing on navigation. The backend will fire the <code>autoCaptureStarted</code> event when an auto capture starts. The frontend should stop the auto capture when appropriate and re-enable breakpoints.
pub struct Timeline_setAutoCaptureEnabledParameters {
    enabled: bool = false,
}
pub struct Timeline_setAutoCaptureEnabledReturnValue {}
/// Instruments to enable when capture starts on the backend (e.g. auto capture or programmatic capture).
pub struct Timeline_setInstrumentsParameters {
    instruments: []const Timeline_Instrument = &.{},
}
pub struct Timeline_setInstrumentsReturnValue {}

// ============================================================================
// Domain: Worker
// ============================================================================

pub struct Worker_workerCreatedPayload {
    workerId: []const u8 = "",
    url: []const u8 = "",
    name: []const u8 = "",
}
pub struct Worker_workerTerminatedPayload {
    workerId: []const u8 = "",
}
pub struct Worker_dispatchMessageFromWorkerPayload {
    workerId: []const u8 = "",
    message: []const u8 = "",
}
/// Enable Worker domain events.
pub struct Worker_enableParameters {}
pub struct Worker_enableReturnValue {}
/// Disable Worker domain events.
pub struct Worker_disableParameters {}
pub struct Worker_disableReturnValue {}
/// Sent after the frontend has sent all initialization messages and can resume this worker. This command is required to allow execution in the worker.
pub struct Worker_initializedParameters {
    workerId: []const u8 = "",
}
pub struct Worker_initializedReturnValue {}
/// Send an Inspector Protocol message to be dispatched to a Worker's agents.
pub struct Worker_sendMessageToWorkerParameters {
    workerId: []const u8 = "",
    message: []const u8 = "",
}
pub struct Worker_sendMessageToWorkerReturnValue {}
