// Source: cp/lib/a2a/index.cp (A2A multi-agent collaboration protocol standard library barrel export)
export {
    A2A_PROTOCOL_VERSION,
    JSONRPC_PARSE_ERROR,
    JSONRPC_INVALID_REQUEST,
    JSONRPC_METHOD_NOT_FOUND,
    JSONRPC_INVALID_PARAMS,
    JSONRPC_INTERNAL_ERROR,
    A2A_ERR_TASK_NOT_FOUND,
    A2A_ERR_TASK_NOT_CANCELABLE,
    A2A_ERR_PUSH_NOT_SUPPORTED,
    A2A_ERR_UNSUPPORTED_OPERATION,
    A2A_ERR_INVALID_STATE_TRANSITION,
    A2aError,
    AgentSkill,
    AgentCapabilities,
    AgentCard,
    AgentCardSummary,
    TaskState,
    MessageRole,
    PartKind,
    Part,
    Message,
    Artifact,
    SseEventKind,
    TaskStatusUpdateEvent,
    TaskArtifactUpdateEvent,
    SseTaskEvent,
    ParsedSseTaskEvent,
    appendJsonString,
    extractJsonRawField,
    extractJsonStringField,
    encodePartJson,
    encodeMessageJson,
    encodeArtifactJson,
    encodeAgentCardJson,
    parseAgentCardSummary,
    encodeTaskEventSse,
    parseSseTaskEvent,
} from "./protocol.cp";

export {
    A2aServerConfig,
    A2aTask,
    A2aServer,
} from "./server.cp";

export {
    PeerHealth,
    AgentPeer,
    AgentRouter,
    HandoffStatus,
    HandoffTrace,
} from "./router.cp";

export {
    A2aTaskState,
    canTransition,
    A2aTaskItem,
    A2aQueueStats,
    A2aTaskQueue,
} from "./task_queue.cp";

export {
    A2aEventKind,
    A2aStreamEvent,
    ReplayResult,
    A2aEventStreamRing,
} from "./event_stream.cp";

export {
    A2aModalityMask,
    A2aFeatureMask,
    A2aRequirement,
    A2aPeerOffer,
    NegotiationRejectReason,
    NegotiationMatch,
    checkPeerCompatibility,
    scoreCompatiblePeer,
    negotiateBestPeers,
} from "./negotiation.cp";
