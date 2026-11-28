// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue, ProtocolStringMap } from "./common";
import { Browser_BrowserContextID } from "./domains_02_browser_to_cachestorage";
import { DOM_BackendNodeId } from "./domains_03_cast_to_domsnapshot";
import { IO_StreamHandle } from "./domains_05_io_to_memory";
import { Page_FrameId } from "./domains_08_page_to_performance";
import { Runtime_RemoteObject, Runtime_StackTrace } from "./domains_11_debugger_to_schema";

// ============================================================================
// Domain: Target
// Supports additional targets discovery and allows to attach to them.
// ============================================================================

pub type Target_TargetID = []const u8;
/// Unique identifier of attached debugging session.
pub type Target_SessionID = []const u8;
pub struct Target_TargetInfo {
    targetId: Target_TargetID = "",
    type_: []const u8 = "", // protocol field: "type"
    title: []const u8 = "",
    url: []const u8 = "",
    attached: bool = false,
    parentId: Target_TargetID? = null,
    openerId: Target_TargetID? = null,
    canAccessOpener: bool = false,
    openerFrameId: Page_FrameId? = null,
    parentFrameId: Page_FrameId? = null,
    browserContextId: Browser_BrowserContextID? = null,
    subtype: ([]const u8)? = null,
    embedderData: ProtocolStringMap? = null,
}
/// A filter used by target query/discovery/auto-attach operations.
pub struct Target_FilterEntry {
    exclude: bool? = null,
    type_: ([]const u8)? = null, // protocol field: "type"
}
/// The entries in TargetFilter are matched sequentially against targets and
pub type Target_TargetFilter = []const Target_FilterEntry;
pub struct Target_RemoteLocation {
    host: []const u8 = "",
    port: f64 = 0.0,
}
/// The state of the target window.
/// enum: "normal"|"minimized"|"maximized"|"fullscreen"
pub type Target_WindowState = []const u8;
/// Issued when attached to target because of auto-attach or `attachToTarget` command.
pub struct Target_attachedToTargetPayload {
    sessionId: Target_SessionID = "",
    targetInfo: Target_TargetInfo = .{},
    waitingForDebugger: bool = false,
}
/// Issued when detached from target for any reason (including `detachFromTarget` command). Can be
pub struct Target_detachedFromTargetPayload {
    sessionId: Target_SessionID = "",
    targetId: Target_TargetID? = null,
}
/// Notifies about a new protocol message received from the session (as reported in
pub struct Target_receivedMessageFromTargetPayload {
    sessionId: Target_SessionID = "",
    message: []const u8 = "",
    targetId: Target_TargetID? = null,
}
/// Issued when a possible inspection target is created.
pub struct Target_targetCreatedPayload {
    targetInfo: Target_TargetInfo = .{},
}
/// Issued when a target is destroyed.
pub struct Target_targetDestroyedPayload {
    targetId: Target_TargetID = "",
}
/// Issued when a target has crashed.
pub struct Target_targetCrashedPayload {
    targetId: Target_TargetID = "",
    status: []const u8 = "",
    errorCode: f64 = 0.0,
}
/// Issued when some information about a target has changed. This only happens between
pub struct Target_targetInfoChangedPayload {
    targetInfo: Target_TargetInfo = .{},
}
/// Activates (focuses) the target.
pub struct Target_activateTargetParameters {
    targetId: Target_TargetID = "",
}
pub struct Target_activateTargetReturnValue {}
/// Attaches to the target with given id.
pub struct Target_attachToTargetParameters {
    targetId: Target_TargetID = "",
    flatten: bool? = null,
}
pub struct Target_attachToTargetReturnValue {
    sessionId: Target_SessionID = "",
}
/// Attaches to the browser target, only uses flat sessionId mode.
pub struct Target_attachToBrowserTargetParameters {}
pub struct Target_attachToBrowserTargetReturnValue {
    sessionId: Target_SessionID = "",
}
/// Closes the target. If the target is a page that gets closed too.
pub struct Target_closeTargetParameters {
    targetId: Target_TargetID = "",
}
pub struct Target_closeTargetReturnValue {
    success: bool = false,
}
/// Inject object to the target's main frame that provides a communication
pub struct Target_exposeDevToolsProtocolParameters {
    targetId: Target_TargetID = "",
    bindingName: ([]const u8)? = null,
    inheritPermissions: bool? = null,
}
pub struct Target_exposeDevToolsProtocolReturnValue {}
/// Creates a new empty BrowserContext. Similar to an incognito profile but you can have more than
pub struct Target_createBrowserContextParameters {
    disposeOnDetach: bool? = null,
    proxyServer: ([]const u8)? = null,
    proxyBypassList: ([]const u8)? = null,
    originsWithUniversalNetworkAccess: ([]const []const u8)? = null,
}
pub struct Target_createBrowserContextReturnValue {
    browserContextId: Browser_BrowserContextID = "",
}
/// Returns all browser contexts created with `Target.createBrowserContext` method.
pub struct Target_getBrowserContextsParameters {}
pub struct Target_getBrowserContextsReturnValue {
    browserContextIds: []const Browser_BrowserContextID = &.{},
    defaultBrowserContextId: Browser_BrowserContextID? = null,
}
/// Creates a new page.
pub struct Target_createTargetParameters {
    url: []const u8 = "",
    left: f64? = null,
    top: f64? = null,
    width: f64? = null,
    height: f64? = null,
    windowState: Target_WindowState? = null,
    browserContextId: Browser_BrowserContextID? = null,
    enableBeginFrameControl: bool? = null,
    newWindow: bool? = null,
    background: bool? = null,
    forTab: bool? = null,
    hidden: bool? = null,
    focus: bool? = null,
}
pub struct Target_createTargetReturnValue {
    targetId: Target_TargetID = "",
}
/// Detaches session with given id.
pub struct Target_detachFromTargetParameters {
    sessionId: Target_SessionID? = null,
    targetId: Target_TargetID? = null,
}
pub struct Target_detachFromTargetReturnValue {}
/// Deletes a BrowserContext. All the belonging pages will be closed without calling their
pub struct Target_disposeBrowserContextParameters {
    browserContextId: Browser_BrowserContextID = "",
}
pub struct Target_disposeBrowserContextReturnValue {}
/// Returns information about a target.
pub struct Target_getTargetInfoParameters {
    targetId: Target_TargetID? = null,
}
pub struct Target_getTargetInfoReturnValue {
    targetInfo: Target_TargetInfo = .{},
}
/// Retrieves a list of available targets.
pub struct Target_getTargetsParameters {
    filter: Target_TargetFilter? = null,
}
pub struct Target_getTargetsReturnValue {
    targetInfos: []const Target_TargetInfo = &.{},
}
/// Sends protocol message over session with given id.
pub struct Target_sendMessageToTargetParameters {
    message: []const u8 = "",
    sessionId: Target_SessionID? = null,
    targetId: Target_TargetID? = null,
}
pub struct Target_sendMessageToTargetReturnValue {}
/// Controls whether to automatically attach to new targets which are considered
pub struct Target_setAutoAttachParameters {
    autoAttach: bool = false,
    waitForDebuggerOnStart: bool = false,
    flatten: bool? = null,
    filter: Target_TargetFilter? = null,
}
pub struct Target_setAutoAttachReturnValue {}
/// Adds the specified target to the list of targets that will be monitored for any related target
pub struct Target_autoAttachRelatedParameters {
    targetId: Target_TargetID = "",
    waitForDebuggerOnStart: bool = false,
    filter: Target_TargetFilter? = null,
}
pub struct Target_autoAttachRelatedReturnValue {}
/// Controls whether to discover available targets and notify via
pub struct Target_setDiscoverTargetsParameters {
    discover: bool = false,
    filter: Target_TargetFilter? = null,
}
pub struct Target_setDiscoverTargetsReturnValue {}
/// Enables target discovery for the specified locations, when `setDiscoverTargets` was set to
pub struct Target_setRemoteLocationsParameters {
    locations: []const Target_RemoteLocation = &.{},
}
pub struct Target_setRemoteLocationsReturnValue {}
/// Gets the targetId of the DevTools page target opened for the given target
pub struct Target_getDevToolsTargetParameters {
    targetId: Target_TargetID = "",
}
pub struct Target_getDevToolsTargetReturnValue {
    targetId: Target_TargetID? = null,
}
/// Opens a DevTools window for the target.
pub struct Target_openDevToolsParameters {
    targetId: Target_TargetID = "",
    panelId: ([]const u8)? = null,
}
pub struct Target_openDevToolsReturnValue {
    targetId: Target_TargetID = "",
}

// ============================================================================
// Domain: Tethering
// The Tethering domain defines methods and events for browser port binding.
// ============================================================================

/// Informs that port was successfully bound and got a specified connection id.
pub struct Tethering_acceptedPayload {
    port: f64 = 0.0,
    connectionId: []const u8 = "",
}
/// Request browser port binding.
pub struct Tethering_bindParameters {
    port: f64 = 0.0,
}
pub struct Tethering_bindReturnValue {}
/// Request browser port unbinding.
pub struct Tethering_unbindParameters {
    port: f64 = 0.0,
}
pub struct Tethering_unbindReturnValue {}

// ============================================================================
// Domain: Tracing
// ============================================================================

/// Configuration for memory dump. Used only when "memory-infra" category is enabled.
pub type Tracing_MemoryDumpConfig = ProtocolStringMap;
pub struct Tracing_TraceConfig {
    recordMode: ([]const u8)? = null, // enum: "recordUntilFull"|"recordContinuously"|"recordAsMuchAsPossible"|"echoToConsole"
    traceBufferSizeInKb: f64? = null,
    enableSampling: bool? = null,
    enableSystrace: bool? = null,
    enableArgumentFilter: bool? = null,
    includedCategories: ([]const []const u8)? = null,
    excludedCategories: ([]const []const u8)? = null,
    syntheticDelays: ([]const []const u8)? = null,
    memoryDumpConfig: Tracing_MemoryDumpConfig? = null,
}
/// Data format of a trace. Can be either the legacy JSON format or the
/// enum: "json"|"proto"
pub type Tracing_StreamFormat = []const u8;
/// Compression type to use for traces returned via streams.
/// enum: "none"|"gzip"
pub type Tracing_StreamCompression = []const u8;
/// Details exposed when memory request explicitly declared.
/// enum: "background"|"light"|"detailed"
pub type Tracing_MemoryDumpLevelOfDetail = []const u8;
/// Backend type to use for tracing. `chrome` uses the Chrome-integrated
/// enum: "auto"|"chrome"|"system"
pub type Tracing_TracingBackend = []const u8;
pub struct Tracing_bufferUsagePayload {
    percentFull: f64? = null,
    eventCount: f64? = null,
    value: f64? = null,
}
/// Contains a bucket of collected trace events. When tracing is stopped collected events will be
pub struct Tracing_dataCollectedPayload {
    value: []const ProtocolStringMap = &.{},
}
/// Signals that tracing is stopped and there is no trace buffers pending flush, all data were
pub struct Tracing_tracingCompletePayload {
    dataLossOccurred: bool = false,
    stream: IO_StreamHandle? = null,
    traceFormat: Tracing_StreamFormat? = null,
    streamCompression: Tracing_StreamCompression? = null,
}
/// Stop trace events collection.
pub struct Tracing_endParameters {}
pub struct Tracing_endReturnValue {}
/// Gets supported tracing categories.
pub struct Tracing_getCategoriesParameters {}
pub struct Tracing_getCategoriesReturnValue {
    categories: []const []const u8 = &.{},
}
/// Return a descriptor for all available tracing categories.
pub struct Tracing_getTrackEventDescriptorParameters {}
pub struct Tracing_getTrackEventDescriptorReturnValue {
    descriptor: ProtocolBinary = "",
}
/// Record a clock sync marker in the trace.
pub struct Tracing_recordClockSyncMarkerParameters {
    syncId: []const u8 = "",
}
pub struct Tracing_recordClockSyncMarkerReturnValue {}
/// Request a global memory dump.
pub struct Tracing_requestMemoryDumpParameters {
    deterministic: bool? = null,
    levelOfDetail: Tracing_MemoryDumpLevelOfDetail? = null,
}
pub struct Tracing_requestMemoryDumpReturnValue {
    dumpGuid: []const u8 = "",
    success: bool = false,
}
/// Start trace events collection.
pub struct Tracing_startParameters {
    categories: ([]const u8)? = null,
    options: ([]const u8)? = null,
    bufferUsageReportingInterval: f64? = null,
    transferMode: ([]const u8)? = null, // enum: "ReportEvents"|"ReturnAsStream"
    streamFormat: Tracing_StreamFormat? = null,
    streamCompression: Tracing_StreamCompression? = null,
    traceConfig: Tracing_TraceConfig? = null,
    perfettoConfig: ProtocolBinary? = null,
    tracingBackend: Tracing_TracingBackend? = null,
    screenshotMaxSize: f64? = null,
    screenshotMaxCount: f64? = null,
}
pub struct Tracing_startReturnValue {}

// ============================================================================
// Domain: WebAudio
// This domain allows inspection of Web Audio API.
// https://webaudio.github.io/web-audio-api/
// ============================================================================

/// An unique ID for a graph object (AudioContext, AudioNode, AudioParam) in Web Audio API
pub type WebAudio_GraphObjectId = []const u8;
/// Enum of BaseAudioContext types
/// enum: "realtime"|"offline"
pub type WebAudio_ContextType = []const u8;
/// Enum of AudioContextState from the spec
/// enum: "suspended"|"running"|"closed"|"interrupted"
pub type WebAudio_ContextState = []const u8;
/// Enum of AudioNode types
pub type WebAudio_NodeType = []const u8;
/// Enum of AudioNode::ChannelCountMode from the spec
/// enum: "clamped-max"|"explicit"|"max"
pub type WebAudio_ChannelCountMode = []const u8;
/// Enum of AudioNode::ChannelInterpretation from the spec
/// enum: "discrete"|"speakers"
pub type WebAudio_ChannelInterpretation = []const u8;
/// Enum of AudioParam types
pub type WebAudio_ParamType = []const u8;
/// Enum of AudioParam::AutomationRate from the spec
/// enum: "a-rate"|"k-rate"
pub type WebAudio_AutomationRate = []const u8;
/// Fields in AudioContext that change in real-time.
pub struct WebAudio_ContextRealtimeData {
    currentTime: f64 = 0.0,
    renderCapacity: f64 = 0.0,
    callbackIntervalMean: f64 = 0.0,
    callbackIntervalVariance: f64 = 0.0,
}
/// Protocol object for BaseAudioContext
pub struct WebAudio_BaseAudioContext {
    contextId: WebAudio_GraphObjectId = "",
    contextType: WebAudio_ContextType = "",
    contextState: WebAudio_ContextState = "",
    realtimeData: WebAudio_ContextRealtimeData? = null,
    callbackBufferSize: f64 = 0.0,
    maxOutputChannelCount: f64 = 0.0,
    sampleRate: f64 = 0.0,
    renderQuantumSize: f64 = 0.0,
}
/// Protocol object for AudioListener
pub struct WebAudio_AudioListener {
    listenerId: WebAudio_GraphObjectId = "",
    contextId: WebAudio_GraphObjectId = "",
}
/// Protocol object for AudioNode
pub struct WebAudio_AudioNode {
    nodeId: WebAudio_GraphObjectId = "",
    contextId: WebAudio_GraphObjectId = "",
    nodeType: WebAudio_NodeType = "",
    numberOfInputs: f64 = 0.0,
    numberOfOutputs: f64 = 0.0,
    channelCount: f64 = 0.0,
    channelCountMode: WebAudio_ChannelCountMode = "",
    channelInterpretation: WebAudio_ChannelInterpretation = "",
}
/// Protocol object for AudioParam
pub struct WebAudio_AudioParam {
    paramId: WebAudio_GraphObjectId = "",
    nodeId: WebAudio_GraphObjectId = "",
    contextId: WebAudio_GraphObjectId = "",
    paramType: WebAudio_ParamType = "",
    rate: WebAudio_AutomationRate = "",
    defaultValue: f64 = 0.0,
    minValue: f64 = 0.0,
    maxValue: f64 = 0.0,
}
/// Notifies that a new BaseAudioContext has been created.
pub struct WebAudio_contextCreatedPayload {
    context: WebAudio_BaseAudioContext = .{},
}
/// Notifies that an existing BaseAudioContext will be destroyed.
pub struct WebAudio_contextWillBeDestroyedPayload {
    contextId: WebAudio_GraphObjectId = "",
}
/// Notifies that existing BaseAudioContext has changed some properties (id stays the same)..
pub struct WebAudio_contextChangedPayload {
    context: WebAudio_BaseAudioContext = .{},
}
/// Notifies that the construction of an AudioListener has finished.
pub struct WebAudio_audioListenerCreatedPayload {
    listener: WebAudio_AudioListener = .{},
}
/// Notifies that a new AudioListener has been created.
pub struct WebAudio_audioListenerWillBeDestroyedPayload {
    contextId: WebAudio_GraphObjectId = "",
    listenerId: WebAudio_GraphObjectId = "",
}
/// Notifies that a new AudioNode has been created.
pub struct WebAudio_audioNodeCreatedPayload {
    node: WebAudio_AudioNode = .{},
}
/// Notifies that an existing AudioNode has been destroyed.
pub struct WebAudio_audioNodeWillBeDestroyedPayload {
    contextId: WebAudio_GraphObjectId = "",
    nodeId: WebAudio_GraphObjectId = "",
}
/// Notifies that a new AudioParam has been created.
pub struct WebAudio_audioParamCreatedPayload {
    param: WebAudio_AudioParam = .{},
}
/// Notifies that an existing AudioParam has been destroyed.
pub struct WebAudio_audioParamWillBeDestroyedPayload {
    contextId: WebAudio_GraphObjectId = "",
    nodeId: WebAudio_GraphObjectId = "",
    paramId: WebAudio_GraphObjectId = "",
}
/// Notifies that two AudioNodes are connected.
pub struct WebAudio_nodesConnectedPayload {
    contextId: WebAudio_GraphObjectId = "",
    sourceId: WebAudio_GraphObjectId = "",
    destinationId: WebAudio_GraphObjectId = "",
    sourceOutputIndex: f64? = null,
    destinationInputIndex: f64? = null,
}
/// Notifies that AudioNodes are disconnected. The destination can be null, and it means all the outgoing connections from the source are disconnected.
pub struct WebAudio_nodesDisconnectedPayload {
    contextId: WebAudio_GraphObjectId = "",
    sourceId: WebAudio_GraphObjectId = "",
    destinationId: WebAudio_GraphObjectId = "",
    sourceOutputIndex: f64? = null,
    destinationInputIndex: f64? = null,
}
/// Notifies that an AudioNode is connected to an AudioParam.
pub struct WebAudio_nodeParamConnectedPayload {
    contextId: WebAudio_GraphObjectId = "",
    sourceId: WebAudio_GraphObjectId = "",
    destinationId: WebAudio_GraphObjectId = "",
    sourceOutputIndex: f64? = null,
}
/// Notifies that an AudioNode is disconnected to an AudioParam.
pub struct WebAudio_nodeParamDisconnectedPayload {
    contextId: WebAudio_GraphObjectId = "",
    sourceId: WebAudio_GraphObjectId = "",
    destinationId: WebAudio_GraphObjectId = "",
    sourceOutputIndex: f64? = null,
}
/// Enables the WebAudio domain and starts sending context lifetime events.
pub struct WebAudio_enableParameters {}
pub struct WebAudio_enableReturnValue {}
/// Disables the WebAudio domain.
pub struct WebAudio_disableParameters {}
pub struct WebAudio_disableReturnValue {}
/// Fetch the realtime data from the registered contexts.
pub struct WebAudio_getRealtimeDataParameters {
    contextId: WebAudio_GraphObjectId = "",
}
pub struct WebAudio_getRealtimeDataReturnValue {
    realtimeData: WebAudio_ContextRealtimeData = .{},
}

// ============================================================================
// Domain: WebAuthn
// This domain allows configuring virtual authenticators to test the WebAuthn
// API.
// ============================================================================

pub type WebAuthn_AuthenticatorId = []const u8;
/// enum: "u2f"|"ctap2"
pub type WebAuthn_AuthenticatorProtocol = []const u8;
/// enum: "ctap2_0"|"ctap2_1"|"ctap2_2"
pub type WebAuthn_Ctap2Version = []const u8;
/// LINT.IfChange(AuthenticatorTransport)
/// enum: "usb"|"nfc"|"ble"|"cable"|"hybrid"|"smart-card"|"internal"
pub type WebAuthn_AuthenticatorTransport = []const u8;
pub struct WebAuthn_VirtualAuthenticatorOptions {
    protocol: WebAuthn_AuthenticatorProtocol = "",
    ctap2Version: WebAuthn_Ctap2Version? = null,
    transport: WebAuthn_AuthenticatorTransport = "",
    hasResidentKey: bool? = null,
    hasUserVerification: bool? = null,
    hasLargeBlob: bool? = null,
    hasCredBlob: bool? = null,
    hasMinPinLength: bool? = null,
    hasPrf: bool? = null,
    hasHmacSecret: bool? = null,
    hasHmacSecretMc: bool? = null,
    hasCmtgKey: bool? = null,
    automaticPresenceSimulation: bool? = null,
    isUserVerified: bool? = null,
    defaultBackupEligibility: bool? = null,
    defaultBackupState: bool? = null,
}
pub struct WebAuthn_Credential {
    credentialId: ProtocolBinary = "",
    isResidentCredential: bool = false,
    rpId: ([]const u8)? = null,
    privateKey: ProtocolBinary = "",
    userHandle: ProtocolBinary? = null,
    signCount: f64 = 0.0,
    largeBlob: ProtocolBinary? = null,
    backupEligibility: bool? = null,
    backupState: bool? = null,
    userName: ([]const u8)? = null,
    userDisplayName: ([]const u8)? = null,
    cmtgKeys: ([]const ProtocolBinary)? = null,
    activeCmtgKeyIndex: f64? = null,
    generateCmtgKeyOnNextOperation: bool? = null,
}
/// Triggered when a credential is added to an authenticator.
pub struct WebAuthn_credentialAddedPayload {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credential: WebAuthn_Credential = .{},
}
/// Triggered when a credential is deleted, e.g. through
pub struct WebAuthn_credentialDeletedPayload {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credentialId: ProtocolBinary = "",
}
/// Triggered when a credential is updated, e.g. through
pub struct WebAuthn_credentialUpdatedPayload {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credential: WebAuthn_Credential = .{},
}
/// Triggered when a credential is used in a webauthn assertion.
pub struct WebAuthn_credentialAssertedPayload {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credential: WebAuthn_Credential = .{},
}
/// Enable the WebAuthn domain and start intercepting credential storage and
pub struct WebAuthn_enableParameters {
    enableUI: bool? = null,
}
pub struct WebAuthn_enableReturnValue {}
/// Disable the WebAuthn domain.
pub struct WebAuthn_disableParameters {}
pub struct WebAuthn_disableReturnValue {}
/// Creates and adds a virtual authenticator.
pub struct WebAuthn_addVirtualAuthenticatorParameters {
    options: WebAuthn_VirtualAuthenticatorOptions = .{},
}
pub struct WebAuthn_addVirtualAuthenticatorReturnValue {
    authenticatorId: WebAuthn_AuthenticatorId = "",
}
/// Resets parameters isBogusSignature, isBadUV, isBadUP to false if they are not present.
pub struct WebAuthn_setResponseOverrideBitsParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    isBogusSignature: bool? = null,
    isBadUV: bool? = null,
    isBadUP: bool? = null,
}
pub struct WebAuthn_setResponseOverrideBitsReturnValue {}
/// Removes the given authenticator.
pub struct WebAuthn_removeVirtualAuthenticatorParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
}
pub struct WebAuthn_removeVirtualAuthenticatorReturnValue {}
/// Adds the credential to the specified authenticator.
pub struct WebAuthn_addCredentialParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credential: WebAuthn_Credential = .{},
}
pub struct WebAuthn_addCredentialReturnValue {}
/// Returns a single credential stored in the given virtual authenticator that
pub struct WebAuthn_getCredentialParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credentialId: ProtocolBinary = "",
}
pub struct WebAuthn_getCredentialReturnValue {
    credential: WebAuthn_Credential = .{},
}
/// Returns all the credentials stored in the given virtual authenticator.
pub struct WebAuthn_getCredentialsParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
}
pub struct WebAuthn_getCredentialsReturnValue {
    credentials: []const WebAuthn_Credential = &.{},
}
/// Removes a credential from the authenticator.
pub struct WebAuthn_removeCredentialParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credentialId: ProtocolBinary = "",
}
pub struct WebAuthn_removeCredentialReturnValue {}
/// Clears all the credentials from the specified device.
pub struct WebAuthn_clearCredentialsParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
}
pub struct WebAuthn_clearCredentialsReturnValue {}
/// Sets whether User Verification succeeds or fails for an authenticator.
pub struct WebAuthn_setUserVerifiedParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    isUserVerified: bool = false,
}
pub struct WebAuthn_setUserVerifiedReturnValue {}
/// Sets whether tests of user presence will succeed immediately (if true) or fail to resolve (if false) for an authenticator.
pub struct WebAuthn_setAutomaticPresenceSimulationParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    enabled: bool = false,
}
pub struct WebAuthn_setAutomaticPresenceSimulationReturnValue {}
/// Allows setting credential properties.
pub struct WebAuthn_setCredentialPropertiesParameters {
    authenticatorId: WebAuthn_AuthenticatorId = "",
    credentialId: ProtocolBinary = "",
    backupEligibility: bool? = null,
    backupState: bool? = null,
    activeCmtgKeyIndex: f64? = null,
    generateCmtgKeyOnNextOperation: bool? = null,
    signCount: f64? = null,
}
pub struct WebAuthn_setCredentialPropertiesReturnValue {}

// ============================================================================
// Domain: WebMCP
// ============================================================================

/// Tool annotations
pub struct WebMCP_Annotation {
    readOnly: bool? = null,
    untrustedContent: bool? = null,
    consequential: bool? = null,
    debugging: bool? = null,
    autosubmit: bool? = null,
}
/// Represents the status of a tool invocation.
/// enum: "Completed"|"Canceled"|"Error"
pub type WebMCP_InvocationStatus = []const u8;
/// Unique identifier of a registered tool. A new identifier is created every time a tool is registered, so a tool that
pub type WebMCP_ToolId = []const u8;
/// Definition of a tool that can be invoked.
pub struct WebMCP_Tool {
    toolId: WebMCP_ToolId = "",
    name: []const u8 = "",
    description: []const u8 = "",
    inputSchema: ProtocolStringMap? = null,
    outputSchema: ProtocolStringMap? = null,
    annotations: WebMCP_Annotation? = null,
    frameId: Page_FrameId = "",
    backendNodeId: DOM_BackendNodeId? = null,
    stackTrace: Runtime_StackTrace? = null,
}
/// Definition of a tool that was removed.
pub struct WebMCP_RemovedTool {
    toolId: WebMCP_ToolId = "",
    name: []const u8 = "",
    frameId: Page_FrameId = "",
}
/// Event fired when new tools are added.
pub struct WebMCP_toolsAddedPayload {
    tools: []const WebMCP_Tool = &.{},
}
/// Event fired when tools are removed.
pub struct WebMCP_toolsRemovedPayload {
    tools: []const WebMCP_RemovedTool = &.{},
}
/// Event fired when a tool invocation starts.
pub struct WebMCP_toolInvokedPayload {
    frameId: Page_FrameId = "",
    invocationId: []const u8 = "",
    input: []const u8 = "",
    toolName: ([]const u8)? = null,
    toolId: WebMCP_ToolId? = null,
}
/// Event fired when a tool invocation completes or fails.
pub struct WebMCP_toolRespondedPayload {
    invocationId: []const u8 = "",
    status: WebMCP_InvocationStatus = "",
    output: ProtocolJsonValue? = null,
    errorText: ([]const u8)? = null,
    exception: Runtime_RemoteObject? = null,
}
/// Enables the WebMCP domain, allowing events to be sent. Enabling the domain will trigger a toolsAdded event for
pub struct WebMCP_enableParameters {}
pub struct WebMCP_enableReturnValue {}
/// Disables the WebMCP domain.
pub struct WebMCP_disableParameters {}
pub struct WebMCP_disableReturnValue {}
/// Invokes a registered tool. Exactly one of `toolId` and `toolName` must be specified; the command fails if both or
pub struct WebMCP_invokeToolParameters {
    frameId: Page_FrameId = "",
    toolId: WebMCP_ToolId? = null,
    toolName: ([]const u8)? = null,
    input: ProtocolStringMap = "",
}
pub struct WebMCP_invokeToolReturnValue {
    invocationId: []const u8 = "",
}
/// Cancels a pending tool invocation.
pub struct WebMCP_cancelInvocationParameters {
    invocationId: []const u8 = "",
}
pub struct WebMCP_cancelInvocationReturnValue {}

// ============================================================================
// Domain: Console
// This domain is deprecated - use Runtime or Log instead.
// ============================================================================

/// Console message.
pub struct Console_ConsoleMessage {
    source: []const u8 = "", // enum: "xml"|"javascript"|"network"|"console-api"|"storage"|"appcache"|"rendering"|"security"|"other"|"deprecation"|"worker"
    level: []const u8 = "", // enum: "log"|"warning"|"error"|"debug"|"info"
    text: []const u8 = "",
    url: ([]const u8)? = null,
    line: f64? = null,
    column: f64? = null,
}
/// Issued when new console message is added.
pub struct Console_messageAddedPayload {
    message: Console_ConsoleMessage = .{},
}
/// Does nothing.
pub struct Console_clearMessagesParameters {}
pub struct Console_clearMessagesReturnValue {}
/// Disables console domain, prevents further console messages from being reported to the client.
pub struct Console_disableParameters {}
pub struct Console_disableReturnValue {}
/// Enables console domain, sends the messages collected so far to the client by means of the
pub struct Console_enableParameters {}
pub struct Console_enableReturnValue {}
