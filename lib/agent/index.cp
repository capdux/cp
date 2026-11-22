// Origin: Pure-CP Stdlib (Unified AI Agent Runtime Exports for cap:agent) | Created: 2026-11-22

export {
    BudgetStatus,
    ModelPricing,
    BudgetLimits,
    AgentBudget,
    LoopAction,
    CallSignature,
    LoopGuard,
    CircuitState,
    ToolCircuitBreaker,
    ContextStatus,
    ContextReport,
    ContextWindowAdvisor,
} from "./budget.cp";

export {
    TaskState,
    DagTask,
    DagEdge,
    DagSummary,
    AgentDag,
} from "./dag.cp";

export {
    passAtK,
    exactMatchNormalized,
    TextF1Result,
    tokenOverlapF1,
    TrajectoryScore,
    scoreToolTrajectory,
    EvalSummary,
    summarizeEvalRun,
} from "./eval.cp";

export {
    CHECKPOINT_MAGIC,
    ReactPhase,
    StepKind,
    TraceEntry,
    ReactSession,
    saveCheckpoint,
    loadCheckpoint,
} from "./react.cp";

export {
    SseEvent,
    SseFeedResult,
    SseStreamParser,
    StreamMetrics,
    StreamTelemetry,
    extractJsonStringField,
} from "./sse.cp";

export {
    ModelRoutePolicy,
    ModelEndpoint,
    ModelRouter,
} from "./model_router.cp";

export {
    GuardAction,
    GuardReason,
    GuardRule,
    GuardVerdict,
    GuardConfig,
    evaluateGuardrails,
    JsonDepthReport,
    inspectJsonPayloadDepth,
} from "./guardrail.cp";

export {
    ReqState,
    InferRequest,
    BatchLimits,
    ScheduleStep,
    BatchScheduler,
} from "./scheduler.cp";

export {
    FieldType,
    FieldSpec,
    SchemaErrorKind,
    ValidationReport,
    validateToolArgs,
    formatValidationHint,
} from "./tool_schema.cp";

export {
    SamplerConfig,
    SampledToken,
    applyRepetitionPenalties,
    greedyArgmax,
    sampleToken,
} from "./sampler.cp";

export {
    SpecVerifyResult,
    verifySpeculativeStep,
    verifySpeculativeGreedy,
    SpecStats,
    AdaptiveDraftController,
} from "./speculative.cp";

export {
    gnmtLengthPenalty,
    normalizedBeamScore,
    wouldRepeatNgram,
    BeamConfig,
    BeamHyp,
    BeamCandidate,
    BeamSearch,
} from "./beam.cp";

export {
    AgentVote,
    ConsensusStatus,
    ConsensusResult,
    resolveConsensus,
    BordaResult,
    resolveBordaRanking,
} from "./consensus.cp";

export {
    ApprovalState,
    ActionRisk,
    ApprovalRequest,
    HitlPolicy,
    HitlQueue,
} from "./hitl.cp";

export {
    PathAccessMode,
    SandboxDecision,
    PathRule,
    CommandPolicy,
    checkPathSandbox,
    checkCommandSandbox,
    isSensitiveEnvKey,
    filterEnvIndices,
} from "./sandbox.cp";

export {
    NO_PARENT_SPAN,
    AgentSpanKind,
    AgentSpanStatus,
    AgentSpan,
    TraceRollup,
    AgentTracer,
} from "./tracer.cp";

export {
    SKILL_NO_DEP,
    SkillStatus,
    SkillDescriptor,
    SkillQuery,
    SkillMatch,
    SkillRegistryStats,
    skillTagHash,
    SkillRegistry,
} from "./skill_registry.cp";

export {
    LlmErrorClass,
    RetryDecisionKind,
    RetryDecision,
    RateLimitSnapshot,
    LlmRetryPolicy,
    classifyHttpStatus,
    parseRetryAfterMs,
    decideLlmRetry,
} from "./retry_policy.cp";

export {
    ToolCacheKey,
    computeToolCacheKey,
    ToolSideEffectMode,
    ToolCacheEntry,
    ToolCacheStats,
    ToolCallCache,
} from "./tool_cache.cp";

export {
    QuotaRejectReason,
    TenantQuotaConfig,
    QuotaDecision,
    TenantQuotaStats,
    TenantQuotaSlot,
    LlmQuotaLimiter,
} from "./quota_limiter.cp";

export {
    ToolStreamState,
    ToolStreamAssembler,
    closePartialJson,
    peekStreamingStringField,
} from "./tool_stream.cp";

export {
    WorkflowNodeKind,
    WorkflowNode,
    WorkflowEdge,
    TransitionRecord,
    WorkflowStepResult,
    ReachabilityReport,
    AgentWorkflowFsm,
} from "./workflow_fsm.cp";

export {
    ModelRateCard,
    UsageEvent,
    AgentCostSummary,
    TopSpender,
    computeUsageCostMicroUsd,
    computeCacheSavingsMicroUsd,
    CostLedger,
} from "./cost_ledger.cp";

export {
    FailureCategory,
    ReflectionAction,
    AttemptRecord,
    ReflexionConfig,
    ReflexionStats,
    ReflexionController,
} from "./reflection.cp";

export {
    ResourceLockMode,
    ToolCallRequest,
    WaveAssignment,
    ParallelPlanStats,
    callsConflict,
    planToolWaves,
} from "./tool_parallel.cp";

export {
    PromptBlockKind,
    PromptBlockDesc,
    CachePlanConfig,
    CachePlanResult,
    planPromptCacheBreakpoints,
    computePrefixMatchTokens,
} from "./prompt_cache_plan.cp";

export {
    ContextSectionRequest,
    ContextSectionAlloc,
    ContextBudgetSummary,
    allocateContextBudget,
} from "./context_budget.cp";

export {
    AgentHealthState,
    LivenessConfig,
    AgentLivenessSlot,
    LivenessSummary,
    AgentLivenessMonitor,
} from "./liveness.cp";

export {
    MailPriority,
    BackpressurePolicy,
    MailEnvelope,
    SendOutcome,
    MailboxStats,
    AgentMailbox,
} from "./mailbox.cp";

export {
    MAX_WORK_STEAL_WORKERS,
    WorkTask,
    WorkerQueue,
    WorkStealStats,
    WorkStealPool,
} from "./work_steal.cp";

export {
    HandoffStatus,
    HandoffRecord,
    HandoffRouter,
    BlackboardWriteStatus,
    BlackboardEntry,
    AgentBlackboard,
    FanOutTaskState,
    FanOutStatus,
    FanOutSlot,
    FanOutSummary,
    FanOutAggregator,
} from "./handoff.cp";

export {
    TurnRunnerConfig,
    TurnStopReason,
    TurnStepKind,
    TurnStepEntry,
    TurnOutcome,
    AgentTurnRunner,
} from "./turn_runner.cp";

export {
    EvalGateThresholds,
    EvalCaseResult,
    EvalGateVerdict,
    EvalGateReport,
    evaluateAgentGate,
    compareWithBaselineGate,
    renderEvalGateSummary,
} from "./eval_gate.cp";

export {
    LlmProviderWire,
    LlmDeltaKind,
    LlmDeltaEvent,
    LlmStreamStats,
    parseOpenAiSseData,
    parseAnthropicSseData,
    LlmDeltaParser,
} from "./llm_stream.cp";

export {
    computeReplayArgsHash,
    ReplayEntryKind,
    ReplayMatchMode,
    ReplayFrame,
    ReplayCassette,
    ReplayStats,
    ReplayPlayer,
    ReplayDiffKind,
    ReplayStepDiff,
    ReplayDiffSummary,
    diffReplayTrajectories,
} from "./replay.cp";

export {
    RoutableToolEntry,
    ToolRouteHit,
    ToolRouteReport,
    SemanticToolRouter,
} from "./tool_router.cp";

export {
    MODEL_CAP_TOOLS,
    MODEL_CAP_VISION,
    MODEL_CAP_THINKING,
    MODEL_CAP_STRUCTURED_JSON,
    MODEL_CAP_PROMPT_CACHE,
    ModelSpec,
    ModelCostEstimate,
    estimateCallCost,
    ModelCatalog,
} from "./model_catalog.cp";

export {
    AGENT_SNAPSHOT_MAGIC,
    AGENT_SNAPSHOT_VERSION,
    SnapshotDecodeError,
    AgentSessionSnapshot,
    encodedSnapshotSize,
    encodeSessionSnapshot,
    decodeSessionSnapshot,
} from "./session_codec.cp";
