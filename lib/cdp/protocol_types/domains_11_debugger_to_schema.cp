// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue, ProtocolStringMap } from "./common";

// ============================================================================
// Domain: Debugger
// Debugger domain exposes JavaScript debugging capabilities. It allows setting and removing
// breakpoints, stepping through execution, exploring stack traces, etc.
// ============================================================================

/// Breakpoint identifier.
pub type Debugger_BreakpointId = []const u8;
/// Call frame identifier.
pub type Debugger_CallFrameId = []const u8;
/// Location in the source code.
pub struct Debugger_Location {
    scriptId: Runtime_ScriptId = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64? = null,
}
/// Location in the source code.
pub struct Debugger_ScriptPosition {
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
}
/// Location range within one script.
pub struct Debugger_LocationRange {
    scriptId: Runtime_ScriptId = "",
    start: Debugger_ScriptPosition = .{},
    end: Debugger_ScriptPosition = .{},
}
/// JavaScript call frame. Array of call frames form the call stack.
pub struct Debugger_CallFrame {
    callFrameId: Debugger_CallFrameId = "",
    functionName: []const u8 = "",
    functionLocation: Debugger_Location? = null,
    location: Debugger_Location = .{},
    url: []const u8 = "",
    scopeChain: []const Debugger_Scope = &.{},
    this: Runtime_RemoteObject = .{},
    returnValue: Runtime_RemoteObject? = null,
    canBeRestarted: bool? = null,
}
/// Scope description.
pub struct Debugger_Scope {
    type_: []const u8 = "", // protocol field: "type"; enum: "global"|"local"|"with"|"closure"|"catch"|"block"|"script"|"eval"|"module"|"wasm-expression-stack"
    object: Runtime_RemoteObject = .{},
    name: ([]const u8)? = null,
    startLocation: Debugger_Location? = null,
    endLocation: Debugger_Location? = null,
    emptyReason: ([]const u8)? = null, // enum: "no-variables"|"all-unavailable"
}
/// Search match for resource.
pub struct Debugger_SearchMatch {
    lineNumber: f64 = 0.0,
    lineContent: []const u8 = "",
}
pub struct Debugger_BreakLocation {
    scriptId: Runtime_ScriptId = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64? = null,
    type_: ([]const u8)? = null, // protocol field: "type"; enum: "debuggerStatement"|"call"|"return"
}
pub struct Debugger_WasmDisassemblyChunk {
    lines: []const []const u8 = &.{},
    bytecodeOffsets: []const f64 = &.{},
}
/// Enum of possible script languages.
/// enum: "JavaScript"|"WebAssembly"
pub type Debugger_ScriptLanguage = []const u8;
/// Debug symbols available for a wasm script.
pub struct Debugger_DebugSymbols {
    type_: []const u8 = "", // protocol field: "type"; enum: "SourceMap"|"EmbeddedDWARF"|"ExternalDWARF"
    externalURL: ([]const u8)? = null,
}
pub struct Debugger_ResolvedBreakpoint {
    breakpointId: Debugger_BreakpointId = "",
    location: Debugger_Location = .{},
}
/// Fired when breakpoint is resolved to an actual script and location.
pub struct Debugger_breakpointResolvedPayload {
    breakpointId: Debugger_BreakpointId = "",
    location: Debugger_Location = .{},
}
/// Fired when the virtual machine stopped on breakpoint or exception or any other stop criteria.
pub struct Debugger_pausedPayload {
    callFrames: []const Debugger_CallFrame = &.{},
    reason: []const u8 = "", // enum: "ambiguous"|"assert"|"CSPViolation"|"debugCommand"|"DOM"|"EventListener"|"exception"|"instrumentation"|"OOM"|"other"|...
    data: ProtocolStringMap? = null,
    hitBreakpoints: ([]const []const u8)? = null,
    asyncStackTrace: Runtime_StackTrace? = null,
    asyncStackTraceId: Runtime_StackTraceId? = null,
    asyncCallStackTraceId: Runtime_StackTraceId? = null,
}
/// Fired when the virtual machine resumed execution.
pub type Debugger_resumedPayload = ProtocolJsonValue;
/// Fired when virtual machine fails to parse the script.
pub struct Debugger_scriptFailedToParsePayload {
    scriptId: Runtime_ScriptId = "",
    url: []const u8 = "",
    startLine: f64 = 0.0,
    startColumn: f64 = 0.0,
    endLine: f64 = 0.0,
    endColumn: f64 = 0.0,
    executionContextId: Runtime_ExecutionContextId = 0.0,
    hash: []const u8 = "",
    buildId: []const u8 = "",
    executionContextAuxData: ProtocolStringMap? = null,
    sourceMapURL: ([]const u8)? = null,
    hasSourceURL: bool? = null,
    isModule: bool? = null,
    length: f64? = null,
    stackTrace: Runtime_StackTrace? = null,
    codeOffset: f64? = null,
    scriptLanguage: Debugger_ScriptLanguage? = null,
    embedderName: ([]const u8)? = null,
}
/// Fired when virtual machine parses script. This event is also fired for all known and uncollected
pub struct Debugger_scriptParsedPayload {
    scriptId: Runtime_ScriptId = "",
    url: []const u8 = "",
    startLine: f64 = 0.0,
    startColumn: f64 = 0.0,
    endLine: f64 = 0.0,
    endColumn: f64 = 0.0,
    executionContextId: Runtime_ExecutionContextId = 0.0,
    hash: []const u8 = "",
    buildId: []const u8 = "",
    executionContextAuxData: ProtocolStringMap? = null,
    isLiveEdit: bool? = null,
    sourceMapURL: ([]const u8)? = null,
    hasSourceURL: bool? = null,
    isModule: bool? = null,
    length: f64? = null,
    stackTrace: Runtime_StackTrace? = null,
    codeOffset: f64? = null,
    scriptLanguage: Debugger_ScriptLanguage? = null,
    debugSymbols: ([]const Debugger_DebugSymbols)? = null,
    embedderName: ([]const u8)? = null,
    resolvedBreakpoints: ([]const Debugger_ResolvedBreakpoint)? = null,
}
/// Continues execution until specific location is reached.
pub struct Debugger_continueToLocationParameters {
    location: Debugger_Location = .{},
    targetCallFrames: ([]const u8)? = null, // enum: "any"|"current"
}
pub struct Debugger_continueToLocationReturnValue {}
/// Disables debugger for given page.
pub struct Debugger_disableParameters {}
pub struct Debugger_disableReturnValue {}
/// Enables debugger for the given page. Clients should not assume that the debugging has been
pub struct Debugger_enableParameters {
    maxScriptsCacheSize: f64? = null,
}
pub struct Debugger_enableReturnValue {
    debuggerId: Runtime_UniqueDebuggerId = "",
}
/// Evaluates expression on a given call frame.
pub struct Debugger_evaluateOnCallFrameParameters {
    callFrameId: Debugger_CallFrameId = "",
    expression: []const u8 = "",
    objectGroup: ([]const u8)? = null,
    includeCommandLineAPI: bool? = null,
    silent: bool? = null,
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    throwOnSideEffect: bool? = null,
    timeout: Runtime_TimeDelta? = null,
    scopeNumber: f64? = null,
}
pub struct Debugger_evaluateOnCallFrameReturnValue {
    result: Runtime_RemoteObject = .{},
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Returns possible locations for breakpoint. scriptId in start and end range locations should be
pub struct Debugger_getPossibleBreakpointsParameters {
    start: Debugger_Location = .{},
    end: Debugger_Location? = null,
    restrictToFunction: bool? = null,
}
pub struct Debugger_getPossibleBreakpointsReturnValue {
    locations: []const Debugger_BreakLocation = &.{},
}
/// Returns source for the script with given id.
pub struct Debugger_getScriptSourceParameters {
    scriptId: Runtime_ScriptId = "",
}
pub struct Debugger_getScriptSourceReturnValue {
    scriptSource: []const u8 = "",
    bytecode: ProtocolBinary? = null,
}
pub struct Debugger_disassembleWasmModuleParameters {
    scriptId: Runtime_ScriptId = "",
}
pub struct Debugger_disassembleWasmModuleReturnValue {
    streamId: ([]const u8)? = null,
    totalNumberOfLines: f64 = 0.0,
    functionBodyOffsets: []const f64 = &.{},
    chunk: Debugger_WasmDisassemblyChunk = .{},
}
/// Disassemble the next chunk of lines for the module corresponding to the
pub struct Debugger_nextWasmDisassemblyChunkParameters {
    streamId: []const u8 = "",
}
pub struct Debugger_nextWasmDisassemblyChunkReturnValue {
    chunk: Debugger_WasmDisassemblyChunk = .{},
}
/// This command is deprecated. Use getScriptSource instead.
pub struct Debugger_getWasmBytecodeParameters {
    scriptId: Runtime_ScriptId = "",
}
pub struct Debugger_getWasmBytecodeReturnValue {
    bytecode: ProtocolBinary = "",
}
/// Returns stack trace with given `stackTraceId`.
pub struct Debugger_getStackTraceParameters {
    stackTraceId: Runtime_StackTraceId = .{},
}
pub struct Debugger_getStackTraceReturnValue {
    stackTrace: Runtime_StackTrace = .{},
}
/// Stops on the next JavaScript statement.
pub struct Debugger_pauseParameters {}
pub struct Debugger_pauseReturnValue {}
pub struct Debugger_pauseOnAsyncCallParameters {
    parentStackTraceId: Runtime_StackTraceId = .{},
}
pub struct Debugger_pauseOnAsyncCallReturnValue {}
/// Removes JavaScript breakpoint.
pub struct Debugger_removeBreakpointParameters {
    breakpointId: Debugger_BreakpointId = "",
}
pub struct Debugger_removeBreakpointReturnValue {}
/// Restarts particular call frame from the beginning. The old, deprecated
pub struct Debugger_restartFrameParameters {
    callFrameId: Debugger_CallFrameId = "",
    mode: ([]const u8)? = null, // enum: "StepInto"
}
pub struct Debugger_restartFrameReturnValue {
    callFrames: []const Debugger_CallFrame = &.{},
    asyncStackTrace: Runtime_StackTrace? = null,
    asyncStackTraceId: Runtime_StackTraceId? = null,
}
/// Resumes JavaScript execution.
pub struct Debugger_resumeParameters {
    terminateOnResume: bool? = null,
}
pub struct Debugger_resumeReturnValue {}
/// Searches for given string in script content.
pub struct Debugger_searchInContentParameters {
    scriptId: Runtime_ScriptId = "",
    query: []const u8 = "",
    caseSensitive: bool? = null,
    isRegex: bool? = null,
}
pub struct Debugger_searchInContentReturnValue {
    result: []const Debugger_SearchMatch = &.{},
}
/// Enables or disables async call stacks tracking.
pub struct Debugger_setAsyncCallStackDepthParameters {
    maxDepth: f64 = 0.0,
}
pub struct Debugger_setAsyncCallStackDepthReturnValue {}
/// Replace previous blackbox execution contexts with passed ones. Forces backend to skip
pub struct Debugger_setBlackboxExecutionContextsParameters {
    uniqueIds: []const []const u8 = &.{},
}
pub struct Debugger_setBlackboxExecutionContextsReturnValue {}
/// Replace previous blackbox patterns with passed ones. Forces backend to skip stepping/pausing in
pub struct Debugger_setBlackboxPatternsParameters {
    patterns: []const []const u8 = &.{},
    skipAnonymous: bool? = null,
}
pub struct Debugger_setBlackboxPatternsReturnValue {}
/// Makes backend skip steps in the script in blackboxed ranges. VM will try leave blacklisted
pub struct Debugger_setBlackboxedRangesParameters {
    scriptId: Runtime_ScriptId = "",
    positions: []const Debugger_ScriptPosition = &.{},
}
pub struct Debugger_setBlackboxedRangesReturnValue {}
/// Sets JavaScript breakpoint at a given location.
pub struct Debugger_setBreakpointParameters {
    location: Debugger_Location = .{},
    condition: ([]const u8)? = null,
}
pub struct Debugger_setBreakpointReturnValue {
    breakpointId: Debugger_BreakpointId = "",
    actualLocation: Debugger_Location = .{},
}
/// Sets instrumentation breakpoint.
pub struct Debugger_setInstrumentationBreakpointParameters {
    instrumentation: []const u8 = "", // enum: "beforeScriptExecution"|"beforeScriptWithSourceMapExecution"
}
pub struct Debugger_setInstrumentationBreakpointReturnValue {
    breakpointId: Debugger_BreakpointId = "",
}
/// Sets JavaScript breakpoint at given location specified either by URL or URL regex. Once this
pub struct Debugger_setBreakpointByUrlParameters {
    lineNumber: f64 = 0.0,
    url: ([]const u8)? = null,
    urlRegex: ([]const u8)? = null,
    scriptHash: ([]const u8)? = null,
    columnNumber: f64? = null,
    condition: ([]const u8)? = null,
}
pub struct Debugger_setBreakpointByUrlReturnValue {
    breakpointId: Debugger_BreakpointId = "",
    locations: []const Debugger_Location = &.{},
}
/// Sets JavaScript breakpoint before each call to the given function.
pub struct Debugger_setBreakpointOnFunctionCallParameters {
    objectId: Runtime_RemoteObjectId = "",
    condition: ([]const u8)? = null,
}
pub struct Debugger_setBreakpointOnFunctionCallReturnValue {
    breakpointId: Debugger_BreakpointId = "",
}
/// Activates / deactivates all breakpoints on the page.
pub struct Debugger_setBreakpointsActiveParameters {
    active: bool = false,
}
pub struct Debugger_setBreakpointsActiveReturnValue {}
/// Defines pause on exceptions state. Can be set to stop on all exceptions, uncaught exceptions,
pub struct Debugger_setPauseOnExceptionsParameters {
    state: []const u8 = "", // enum: "none"|"caught"|"uncaught"|"all"
}
pub struct Debugger_setPauseOnExceptionsReturnValue {}
/// Changes return value in top frame. Available only at return break position.
pub struct Debugger_setReturnValueParameters {
    newValue: Runtime_CallArgument = .{},
}
pub struct Debugger_setReturnValueReturnValue {}
/// Live edit is no longer supported and this command always fails with a "no longer available" error.
pub struct Debugger_setScriptSourceParameters {
    scriptId: Runtime_ScriptId = "",
    scriptSource: []const u8 = "",
    dryRun: bool? = null,
    allowTopFrameEditing: bool? = null,
}
pub struct Debugger_setScriptSourceReturnValue {
    callFrames: ([]const Debugger_CallFrame)? = null,
    stackChanged: bool? = null,
    asyncStackTrace: Runtime_StackTrace? = null,
    asyncStackTraceId: Runtime_StackTraceId? = null,
    status: []const u8 = "", // enum: "Ok"|"CompileError"|"BlockedByActiveGenerator"|"BlockedByActiveFunction"|"BlockedByTopLevelEsModuleChange"
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Makes page not interrupt on any pauses (breakpoint, exception, dom exception etc).
pub struct Debugger_setSkipAllPausesParameters {
    skip: bool = false,
}
pub struct Debugger_setSkipAllPausesReturnValue {}
/// Changes value of variable in a callframe. Object-based scopes are not supported and must be
pub struct Debugger_setVariableValueParameters {
    scopeNumber: f64 = 0.0,
    variableName: []const u8 = "",
    newValue: Runtime_CallArgument = .{},
    callFrameId: Debugger_CallFrameId = "",
}
pub struct Debugger_setVariableValueReturnValue {}
/// Steps into the function call.
pub struct Debugger_stepIntoParameters {
    breakOnAsyncCall: bool? = null,
    skipList: ([]const Debugger_LocationRange)? = null,
}
pub struct Debugger_stepIntoReturnValue {}
/// Steps out of the function call.
pub struct Debugger_stepOutParameters {}
pub struct Debugger_stepOutReturnValue {}
/// Steps over the statement.
pub struct Debugger_stepOverParameters {
    skipList: ([]const Debugger_LocationRange)? = null,
    enterRanges: ([]const Debugger_LocationRange)? = null,
}
pub struct Debugger_stepOverReturnValue {}

// ============================================================================
// Domain: HeapProfiler
// ============================================================================

/// Heap snapshot object id.
pub type HeapProfiler_HeapSnapshotObjectId = []const u8;
/// Sampling Heap Profile node. Holds callsite information, allocation statistics and child nodes.
pub struct HeapProfiler_SamplingHeapProfileNode {
    callFrame: Runtime_CallFrame = .{},
    selfSize: f64 = 0.0,
    id: f64 = 0.0,
    children: []const HeapProfiler_SamplingHeapProfileNode = &.{},
}
/// A single sample from a sampling profile.
pub struct HeapProfiler_SamplingHeapProfileSample {
    size: f64 = 0.0,
    nodeId: f64 = 0.0,
    ordinal: f64 = 0.0,
}
/// Sampling profile.
pub struct HeapProfiler_SamplingHeapProfile {
    head: HeapProfiler_SamplingHeapProfileNode = .{},
    samples: []const HeapProfiler_SamplingHeapProfileSample = &.{},
}
pub struct HeapProfiler_addHeapSnapshotChunkPayload {
    chunk: []const u8 = "",
}
/// If heap objects tracking has been started then backend may send update for one or more fragments
pub struct HeapProfiler_heapStatsUpdatePayload {
    statsUpdate: []const f64 = &.{},
}
/// If heap objects tracking has been started then backend regularly sends a current value for last
pub struct HeapProfiler_lastSeenObjectIdPayload {
    lastSeenObjectId: f64 = 0.0,
    timestamp: f64 = 0.0,
}
pub struct HeapProfiler_reportHeapSnapshotProgressPayload {
    done: f64 = 0.0,
    total: f64 = 0.0,
    finished: bool? = null,
}
pub type HeapProfiler_resetProfilesPayload = ProtocolJsonValue;
/// Enables console to refer to the node with given id via $x (see Command Line API for more details
pub struct HeapProfiler_addInspectedHeapObjectParameters {
    heapObjectId: HeapProfiler_HeapSnapshotObjectId = "",
}
pub struct HeapProfiler_addInspectedHeapObjectReturnValue {}
pub struct HeapProfiler_collectGarbageParameters {}
pub struct HeapProfiler_collectGarbageReturnValue {}
pub struct HeapProfiler_disableParameters {}
pub struct HeapProfiler_disableReturnValue {}
pub struct HeapProfiler_enableParameters {}
pub struct HeapProfiler_enableReturnValue {}
pub struct HeapProfiler_getHeapObjectIdParameters {
    objectId: Runtime_RemoteObjectId = "",
}
pub struct HeapProfiler_getHeapObjectIdReturnValue {
    heapSnapshotObjectId: HeapProfiler_HeapSnapshotObjectId = "",
}
pub struct HeapProfiler_getObjectByHeapObjectIdParameters {
    objectId: HeapProfiler_HeapSnapshotObjectId = "",
    objectGroup: ([]const u8)? = null,
}
pub struct HeapProfiler_getObjectByHeapObjectIdReturnValue {
    result: Runtime_RemoteObject = .{},
}
pub struct HeapProfiler_getSamplingProfileParameters {}
pub struct HeapProfiler_getSamplingProfileReturnValue {
    profile: HeapProfiler_SamplingHeapProfile = .{},
}
pub struct HeapProfiler_startSamplingParameters {
    samplingInterval: f64? = null,
    stackDepth: f64? = null,
    includeObjectsCollectedByMajorGC: bool? = null,
    includeObjectsCollectedByMinorGC: bool? = null,
}
pub struct HeapProfiler_startSamplingReturnValue {}
pub struct HeapProfiler_startTrackingHeapObjectsParameters {
    trackAllocations: bool? = null,
}
pub struct HeapProfiler_startTrackingHeapObjectsReturnValue {}
pub struct HeapProfiler_stopSamplingParameters {}
pub struct HeapProfiler_stopSamplingReturnValue {
    profile: HeapProfiler_SamplingHeapProfile = .{},
}
pub struct HeapProfiler_stopTrackingHeapObjectsParameters {
    reportProgress: bool? = null,
    treatGlobalObjectsAsRoots: bool? = null,
    captureNumericValue: bool? = null,
    exposeInternals: bool? = null,
}
pub struct HeapProfiler_stopTrackingHeapObjectsReturnValue {}
pub struct HeapProfiler_takeHeapSnapshotParameters {
    reportProgress: bool? = null,
    treatGlobalObjectsAsRoots: bool? = null,
    captureNumericValue: bool? = null,
    exposeInternals: bool? = null,
}
pub struct HeapProfiler_takeHeapSnapshotReturnValue {}

// ============================================================================
// Domain: Profiler
// ============================================================================

/// Profile node. Holds callsite information, execution statistics and child nodes.
pub struct Profiler_ProfileNode {
    id: f64 = 0.0,
    callFrame: Runtime_CallFrame = .{},
    hitCount: f64? = null,
    children: ([]const f64)? = null,
    deoptReason: ([]const u8)? = null,
    positionTicks: ([]const Profiler_PositionTickInfo)? = null,
}
/// Profile.
pub struct Profiler_Profile {
    nodes: []const Profiler_ProfileNode = &.{},
    startTime: f64 = 0.0,
    endTime: f64 = 0.0,
    samples: ([]const f64)? = null,
    timeDeltas: ([]const f64)? = null,
}
/// Specifies a number of samples attributed to a certain source position.
pub struct Profiler_PositionTickInfo {
    line: f64 = 0.0,
    ticks: f64 = 0.0,
}
/// Coverage data for a source range.
pub struct Profiler_CoverageRange {
    startOffset: f64 = 0.0,
    endOffset: f64 = 0.0,
    count: f64 = 0.0,
}
/// Coverage data for a JavaScript function.
pub struct Profiler_FunctionCoverage {
    functionName: []const u8 = "",
    ranges: []const Profiler_CoverageRange = &.{},
    isBlockCoverage: bool = false,
}
/// Coverage data for a JavaScript script.
pub struct Profiler_ScriptCoverage {
    scriptId: Runtime_ScriptId = "",
    url: []const u8 = "",
    functions: []const Profiler_FunctionCoverage = &.{},
}
pub struct Profiler_consoleProfileFinishedPayload {
    id: []const u8 = "",
    location: Debugger_Location = .{},
    profile: Profiler_Profile = .{},
    title: ([]const u8)? = null,
}
/// Sent when new profile recording is started using console.profile() call.
pub struct Profiler_consoleProfileStartedPayload {
    id: []const u8 = "",
    location: Debugger_Location = .{},
    title: ([]const u8)? = null,
}
/// Reports coverage delta since the last poll (either from an event like this, or from
pub struct Profiler_preciseCoverageDeltaUpdatePayload {
    timestamp: f64 = 0.0,
    occasion: []const u8 = "",
    result: []const Profiler_ScriptCoverage = &.{},
}
pub struct Profiler_disableParameters {}
pub struct Profiler_disableReturnValue {}
pub struct Profiler_enableParameters {}
pub struct Profiler_enableReturnValue {}
/// Collect coverage data for the current isolate. The coverage data may be incomplete due to
pub struct Profiler_getBestEffortCoverageParameters {}
pub struct Profiler_getBestEffortCoverageReturnValue {
    result: []const Profiler_ScriptCoverage = &.{},
}
/// Changes CPU profiler sampling interval. Must be called before CPU profiles recording started.
pub struct Profiler_setSamplingIntervalParameters {
    interval: f64 = 0.0,
}
pub struct Profiler_setSamplingIntervalReturnValue {}
pub struct Profiler_startParameters {}
pub struct Profiler_startReturnValue {}
/// Enable precise code coverage. Coverage data for JavaScript executed before enabling precise code
pub struct Profiler_startPreciseCoverageParameters {
    callCount: bool? = null,
    detailed: bool? = null,
    allowTriggeredUpdates: bool? = null,
}
pub struct Profiler_startPreciseCoverageReturnValue {
    timestamp: f64 = 0.0,
}
pub struct Profiler_stopParameters {}
pub struct Profiler_stopReturnValue {
    profile: Profiler_Profile = .{},
}
/// Disable precise code coverage. Disabling releases unnecessary execution count records and allows
pub struct Profiler_stopPreciseCoverageParameters {}
pub struct Profiler_stopPreciseCoverageReturnValue {}
/// Collect coverage data for the current isolate, and resets execution counters. Precise code
pub struct Profiler_takePreciseCoverageParameters {}
pub struct Profiler_takePreciseCoverageReturnValue {
    result: []const Profiler_ScriptCoverage = &.{},
    timestamp: f64 = 0.0,
}

// ============================================================================
// Domain: Runtime
// Runtime domain exposes JavaScript runtime by means of remote evaluation and mirror objects.
// Evaluation results are returned as mirror object that expose object type, string representation
// ============================================================================

/// Unique script identifier.
pub type Runtime_ScriptId = []const u8;
/// Represents options for serialization. Overrides `generatePreview` and `returnByValue`.
pub struct Runtime_SerializationOptions {
    serialization: []const u8 = "", // enum: "deep"|"json"|"idOnly"
    maxDepth: f64? = null,
    additionalParameters: ProtocolStringMap? = null,
}
/// Represents deep serialized value.
pub struct Runtime_DeepSerializedValue {
    type_: []const u8 = "", // protocol field: "type"; enum: "undefined"|"null"|"string"|"number"|"boolean"|"bigint"|"regexp"|"date"|"symbol"|"array"|"object"|"function"|"map"|"s...
    value: ProtocolJsonValue? = null,
    objectId: ([]const u8)? = null,
    weakLocalObjectReference: f64? = null,
}
/// Unique object identifier.
pub type Runtime_RemoteObjectId = []const u8;
/// Primitive value which cannot be JSON-stringified. Includes values `-0`, `NaN`, `Infinity`,
pub type Runtime_UnserializableValue = []const u8;
/// Mirror object referencing original JavaScript object.
pub struct Runtime_RemoteObject {
    type_: []const u8 = "", // protocol field: "type"; enum: "object"|"function"|"undefined"|"string"|"number"|"boolean"|"symbol"|"bigint"
    subtype: ([]const u8)? = null, // enum: "array"|"null"|"node"|"regexp"|"date"|"map"|"set"|"weakmap"|"weakset"|"iterator"|"generator"|"error"|"proxy"|"promise...
    className: ([]const u8)? = null,
    value: ProtocolJsonValue? = null,
    unserializableValue: Runtime_UnserializableValue? = null,
    description: ([]const u8)? = null,
    deepSerializedValue: Runtime_DeepSerializedValue? = null,
    objectId: Runtime_RemoteObjectId? = null,
    preview: Runtime_ObjectPreview? = null,
    customPreview: Runtime_CustomPreview? = null,
}
pub struct Runtime_CustomPreview {
    header: []const u8 = "",
    bodyGetterId: Runtime_RemoteObjectId? = null,
}
/// Object containing abbreviated remote object value.
pub struct Runtime_ObjectPreview {
    type_: []const u8 = "", // protocol field: "type"; enum: "object"|"function"|"undefined"|"string"|"number"|"boolean"|"symbol"|"bigint"
    subtype: ([]const u8)? = null, // enum: "array"|"null"|"node"|"regexp"|"date"|"map"|"set"|"weakmap"|"weakset"|"iterator"|"generator"|"error"|"proxy"|"promise...
    description: ([]const u8)? = null,
    overflow: bool = false,
    properties: []const Runtime_PropertyPreview = &.{},
    entries: ([]const Runtime_EntryPreview)? = null,
}
pub struct Runtime_PropertyPreview {
    name: []const u8 = "",
    type_: []const u8 = "", // protocol field: "type"; enum: "object"|"function"|"undefined"|"string"|"number"|"boolean"|"symbol"|"accessor"|"bigint"
    value: ([]const u8)? = null,
    valuePreview: Runtime_ObjectPreview? = null,
    subtype: ([]const u8)? = null, // enum: "array"|"null"|"node"|"regexp"|"date"|"map"|"set"|"weakmap"|"weakset"|"iterator"|"generator"|"error"|"proxy"|"promise...
}
pub struct Runtime_EntryPreview {
    key: Runtime_ObjectPreview? = null,
    value: Runtime_ObjectPreview = .{},
}
/// Object property descriptor.
pub struct Runtime_PropertyDescriptor {
    name: []const u8 = "",
    value: Runtime_RemoteObject? = null,
    writable: bool? = null,
    get: Runtime_RemoteObject? = null,
    set: Runtime_RemoteObject? = null,
    configurable: bool = false,
    enumerable: bool = false,
    wasThrown: bool? = null,
    isOwn: bool? = null,
    symbol: Runtime_RemoteObject? = null,
}
/// Object internal property descriptor. This property isn't normally visible in JavaScript code.
pub struct Runtime_InternalPropertyDescriptor {
    name: []const u8 = "",
    value: Runtime_RemoteObject? = null,
}
/// Object private field descriptor.
pub struct Runtime_PrivatePropertyDescriptor {
    name: []const u8 = "",
    value: Runtime_RemoteObject? = null,
    get: Runtime_RemoteObject? = null,
    set: Runtime_RemoteObject? = null,
}
/// Represents function call argument. Either remote object id `objectId`, primitive `value`,
pub struct Runtime_CallArgument {
    value: ProtocolJsonValue? = null,
    unserializableValue: Runtime_UnserializableValue? = null,
    objectId: Runtime_RemoteObjectId? = null,
}
/// Id of an execution context.
pub type Runtime_ExecutionContextId = f64;
/// Description of an isolated world.
pub struct Runtime_ExecutionContextDescription {
    id: Runtime_ExecutionContextId = 0.0,
    origin: []const u8 = "",
    name: []const u8 = "",
    uniqueId: []const u8 = "",
    auxData: ProtocolStringMap? = null,
}
/// Detailed information about exception (or error) that was thrown during script compilation or
pub struct Runtime_ExceptionDetails {
    exceptionId: f64 = 0.0,
    text: []const u8 = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
    scriptId: Runtime_ScriptId? = null,
    url: ([]const u8)? = null,
    stackTrace: Runtime_StackTrace? = null,
    exception: Runtime_RemoteObject? = null,
    executionContextId: Runtime_ExecutionContextId? = null,
    exceptionMetaData: ProtocolStringMap? = null,
}
/// Number of milliseconds since epoch.
pub type Runtime_Timestamp = f64;
/// Number of milliseconds.
pub type Runtime_TimeDelta = f64;
/// Stack entry for runtime errors and assertions.
pub struct Runtime_CallFrame {
    functionName: []const u8 = "",
    scriptId: Runtime_ScriptId = "",
    url: []const u8 = "",
    lineNumber: f64 = 0.0,
    columnNumber: f64 = 0.0,
}
/// Call frames for assertions or error messages.
pub struct Runtime_StackTrace {
    description: ([]const u8)? = null,
    callFrames: []const Runtime_CallFrame = &.{},
    parent: (*const Runtime_StackTrace)? = null,
    parentId: Runtime_StackTraceId? = null,
}
/// Unique identifier of current debugger.
pub type Runtime_UniqueDebuggerId = []const u8;
/// If `debuggerId` is set stack trace comes from another debugger and can be resolved there. This
pub struct Runtime_StackTraceId {
    id: []const u8 = "",
    debuggerId: Runtime_UniqueDebuggerId? = null,
}
/// Notification is issued every time when binding is called.
pub struct Runtime_bindingCalledPayload {
    name: []const u8 = "",
    payload: []const u8 = "",
    executionContextId: Runtime_ExecutionContextId = 0.0,
}
/// Issued when console API was called.
pub struct Runtime_consoleAPICalledPayload {
    type_: []const u8 = "", // protocol field: "type"; enum: "log"|"debug"|"info"|"error"|"warning"|"dir"|"dirxml"|"table"|"trace"|"clear"|"startGroup"|"startGroupCollapsed"|"end...
    args: []const Runtime_RemoteObject = &.{},
    executionContextId: Runtime_ExecutionContextId = 0.0,
    timestamp: Runtime_Timestamp = 0.0,
    stackTrace: Runtime_StackTrace? = null,
    context: ([]const u8)? = null,
}
/// Issued when unhandled exception was revoked.
pub struct Runtime_exceptionRevokedPayload {
    reason: []const u8 = "",
    exceptionId: f64 = 0.0,
}
/// Issued when exception was thrown and unhandled.
pub struct Runtime_exceptionThrownPayload {
    timestamp: Runtime_Timestamp = 0.0,
    exceptionDetails: Runtime_ExceptionDetails = .{},
}
/// Issued when new execution context is created.
pub struct Runtime_executionContextCreatedPayload {
    context: Runtime_ExecutionContextDescription = .{},
}
/// Issued when execution context is destroyed.
pub struct Runtime_executionContextDestroyedPayload {
    executionContextId: Runtime_ExecutionContextId = 0.0,
    executionContextUniqueId: []const u8 = "",
}
/// Issued when all executionContexts were cleared in browser
pub type Runtime_executionContextsClearedPayload = ProtocolJsonValue;
/// Issued when object should be inspected (for example, as a result of inspect() command line API
pub struct Runtime_inspectRequestedPayload {
    object: Runtime_RemoteObject = .{},
    hints: ProtocolStringMap = "",
    executionContextId: Runtime_ExecutionContextId? = null,
}
/// Add handler to promise with given promise object id.
pub struct Runtime_awaitPromiseParameters {
    promiseObjectId: Runtime_RemoteObjectId = "",
    returnByValue: bool? = null,
    generatePreview: bool? = null,
}
pub struct Runtime_awaitPromiseReturnValue {
    result: Runtime_RemoteObject = .{},
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Calls function with given declaration on the given object. Object group of the result is
pub struct Runtime_callFunctionOnParameters {
    functionDeclaration: []const u8 = "",
    objectId: Runtime_RemoteObjectId? = null,
    arguments: ([]const Runtime_CallArgument)? = null,
    silent: bool? = null,
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    userGesture: bool? = null,
    awaitPromise: bool? = null,
    executionContextId: Runtime_ExecutionContextId? = null,
    objectGroup: ([]const u8)? = null,
    throwOnSideEffect: bool? = null,
    uniqueContextId: ([]const u8)? = null,
    serializationOptions: Runtime_SerializationOptions? = null,
}
pub struct Runtime_callFunctionOnReturnValue {
    result: Runtime_RemoteObject = .{},
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Compiles expression.
pub struct Runtime_compileScriptParameters {
    expression: []const u8 = "",
    sourceURL: []const u8 = "",
    persistScript: bool = false,
    executionContextId: Runtime_ExecutionContextId? = null,
}
pub struct Runtime_compileScriptReturnValue {
    scriptId: Runtime_ScriptId? = null,
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Disables reporting of execution contexts creation.
pub struct Runtime_disableParameters {}
pub struct Runtime_disableReturnValue {}
/// Discards collected exceptions and console API calls.
pub struct Runtime_discardConsoleEntriesParameters {}
pub struct Runtime_discardConsoleEntriesReturnValue {}
/// Enables reporting of execution contexts creation by means of `executionContextCreated` event.
pub struct Runtime_enableParameters {}
pub struct Runtime_enableReturnValue {}
/// Evaluates expression on global object.
pub struct Runtime_evaluateParameters {
    expression: []const u8 = "",
    objectGroup: ([]const u8)? = null,
    includeCommandLineAPI: bool? = null,
    silent: bool? = null,
    contextId: Runtime_ExecutionContextId? = null,
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    userGesture: bool? = null,
    awaitPromise: bool? = null,
    throwOnSideEffect: bool? = null,
    timeout: Runtime_TimeDelta? = null,
    disableBreaks: bool? = null,
    replMode: bool? = null,
    allowUnsafeEvalBlockedByCSP: bool? = null,
    uniqueContextId: ([]const u8)? = null,
    serializationOptions: Runtime_SerializationOptions? = null,
}
pub struct Runtime_evaluateReturnValue {
    result: Runtime_RemoteObject = .{},
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Returns the isolate id.
pub struct Runtime_getIsolateIdParameters {}
pub struct Runtime_getIsolateIdReturnValue {
    id: []const u8 = "",
}
/// Returns the JavaScript heap usage.
pub struct Runtime_getHeapUsageParameters {}
pub struct Runtime_getHeapUsageReturnValue {
    usedSize: f64 = 0.0,
    totalSize: f64 = 0.0,
    embedderHeapUsedSize: f64 = 0.0,
    backingStorageSize: f64 = 0.0,
}
/// Returns properties of a given object. Object group of the result is inherited from the target
pub struct Runtime_getPropertiesParameters {
    objectId: Runtime_RemoteObjectId = "",
    ownProperties: bool? = null,
    accessorPropertiesOnly: bool? = null,
    generatePreview: bool? = null,
    nonIndexedPropertiesOnly: bool? = null,
}
pub struct Runtime_getPropertiesReturnValue {
    result: []const Runtime_PropertyDescriptor = &.{},
    internalProperties: ([]const Runtime_InternalPropertyDescriptor)? = null,
    privateProperties: ([]const Runtime_PrivatePropertyDescriptor)? = null,
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Returns all let, const and class variables from global scope.
pub struct Runtime_globalLexicalScopeNamesParameters {
    executionContextId: Runtime_ExecutionContextId? = null,
}
pub struct Runtime_globalLexicalScopeNamesReturnValue {
    names: []const []const u8 = &.{},
}
pub struct Runtime_queryObjectsParameters {
    prototypeObjectId: Runtime_RemoteObjectId = "",
    objectGroup: ([]const u8)? = null,
}
pub struct Runtime_queryObjectsReturnValue {
    objects: Runtime_RemoteObject = .{},
}
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
/// Tells inspected instance to run if it was waiting for debugger to attach.
pub struct Runtime_runIfWaitingForDebuggerParameters {}
pub struct Runtime_runIfWaitingForDebuggerReturnValue {}
/// Runs script with given id in a given context.
pub struct Runtime_runScriptParameters {
    scriptId: Runtime_ScriptId = "",
    executionContextId: Runtime_ExecutionContextId? = null,
    objectGroup: ([]const u8)? = null,
    silent: bool? = null,
    includeCommandLineAPI: bool? = null,
    returnByValue: bool? = null,
    generatePreview: bool? = null,
    awaitPromise: bool? = null,
}
pub struct Runtime_runScriptReturnValue {
    result: Runtime_RemoteObject = .{},
    exceptionDetails: Runtime_ExceptionDetails? = null,
}
/// Enables or disables async call stacks tracking.
pub struct Runtime_setAsyncCallStackDepthParameters {
    maxDepth: f64 = 0.0,
}
pub struct Runtime_setAsyncCallStackDepthReturnValue {}
pub struct Runtime_setCustomObjectFormatterEnabledParameters {
    enabled: bool = false,
}
pub struct Runtime_setCustomObjectFormatterEnabledReturnValue {}
pub struct Runtime_setMaxCallStackSizeToCaptureParameters {
    size: f64 = 0.0,
}
pub struct Runtime_setMaxCallStackSizeToCaptureReturnValue {}
/// Terminate current or next JavaScript execution.
pub struct Runtime_terminateExecutionParameters {}
pub struct Runtime_terminateExecutionReturnValue {}
/// If executionContextId is empty, adds binding with the given name on the
pub struct Runtime_addBindingParameters {
    name: []const u8 = "",
    executionContextId: Runtime_ExecutionContextId? = null,
    executionContextName: ([]const u8)? = null,
}
pub struct Runtime_addBindingReturnValue {}
/// This method does not remove binding function from global object but
pub struct Runtime_removeBindingParameters {
    name: []const u8 = "",
}
pub struct Runtime_removeBindingReturnValue {}
/// This method tries to lookup and populate exception details for a
pub struct Runtime_getExceptionDetailsParameters {
    errorObjectId: Runtime_RemoteObjectId = "",
}
pub struct Runtime_getExceptionDetailsReturnValue {
    exceptionDetails: Runtime_ExceptionDetails? = null,
}

// ============================================================================
// Domain: Schema
// This domain is deprecated.
// ============================================================================

/// Description of the protocol domain.
pub struct Schema_Domain {
    name: []const u8 = "",
    version: []const u8 = "",
}
/// Returns supported domains.
pub struct Schema_getDomainsParameters {}
pub struct Schema_getDomainsReturnValue {
    domains: []const Schema_Domain = &.{},
}
