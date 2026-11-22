// Source: cp/lib/agentmem/index.cp (Unified Exports for cap:agentmem)
// Unified Public Exports for Hierarchical Agent Memory Engine (cap:agentmem & cap.agentmem)

export {
    AgentMemError,
    MemoryTier,
    RecallWeights,
    MemoryItem,
    RecallHit,
    AgentMemory,
} from "./memory.cp";

export {
    MemoryWeights,
    EpisodeMeta,
    ScoredEpisode,
    ebbinghausRetention,
    reinforceStability,
    scoreEpisode,
    rankTopEpisodes,
    selectEvictionVictim,
    touchEpisode,
} from "./episodic.cp";

export {
    KgEntity,
    KgTriple,
    KgStore,
} from "./kg.cp";

export {
    MemSlotRole,
    EvictionPolicy,
    WorkingMemItem,
    WorkingMemStats,
    WorkingMemory,
} from "./workingMem.cp";

export {
    MemGraphEdge,
    ActivatedNode,
    SpreadingConfig,
    spreadActivation,
    personalizedPageRank,
} from "./activation.cp";

export {
    MemConsolidationTier,
    ConsolidatableRecord,
    ConsolidationPolicy,
    ConsolidationReport,
    halfLifeDecayFactor,
    consolidateMemoriesInPlace,
    EpisodeMemoryTier,
    ConsolidationConfig,
    MemoryEpisodeMeta,
    ConsolidationDecisionKind,
    ConsolidationAction,
    ConsolidationSummary,
    computeRetentionScore,
    cosineSimEpisodeF32,
    planMemoryConsolidation,
} from "./consolidation.cp";

export {
    MemoryRetrievalConfig,
    MemoryCandidate,
    MemoryScoredHit,
    MemoryMergeStats,
    MemoryPackResult,
    scoreAndRankMemoriesF32,
    mergeRedundantMemoriesF32,
    packMemoriesForPrompt,
} from "./retrieval.cp";

export {
    TemporalFactEdge,
    GraphHopHit,
    EntityGraphStats,
    TemporalEntityGraph,
} from "./entityGraph.cp";

export {
    SUMMARY_TREE_MAX_NODES,
    SUMMARY_NO_NODE,
    SummaryNodeKind,
    SummaryTreeNode,
    ContextFrontierPlan,
    SummaryTreeStats,
    HierarchicalSummaryTree,
} from "./summaryTree.cp";

export {
    MEM_ROUTER_MAX_CANDIDATES,
    MemorySourceStore,
    MemoryQueryIntent,
    MemoryTierWeights,
    MemoryRecallCandidate,
    FusedMemoryPick,
    MemoryRouteSummary,
    fuseAndPackMemoryRecalls,
} from "./memRouter.cp";

export {
    MEM_CONSOL_MAX_EPISODIC,
    MEM_CONSOL_MAX_SEMANTIC,
    MEM_CONSOL_MAX_DIM,
    MEM_CONSOL_NO_ID,
    MemConsolStatus,
    MemConsolEntry,
    MemConsolPassConfig,
    MemConsolPassReport,
    MemConsolEngineStats,
    computeEntryRetentionScore,
    AgentMemoryConsolidator,
} from "./consolidator.cp";

export {
    EP_INDEX_MAX_SLOTS,
    EP_INDEX_MAX_DIM,
    EpisodeRecord,
    EpisodeQueryFilter,
    EpisodeHybridWeights,
    EpisodeHybridHit,
    EpisodeIndexStats,
    HybridEpisodeIndex,
} from "./episodes.cp";

export {
    FACT_LEDGER_MAX_RECORDS,
    FACT_TIME_INF,
    FACT_NO_ID,
    FACT_MAX_CONFIDENCE_BPS,
    FactStatus,
    FactLedgerOpStatus,
    BiTemporalFact,
    FactAssertResult,
    FactLedgerStats,
    BiTemporalFactLedger,
} from "./factLedger.cp";

export {
    SKILL_BANK_MAX_SLOTS,
    SKILL_BANK_MAX_DIM,
    SKILL_BANK_MAX_BPS,
    SkillRecord,
    SkillRetrievalConfig,
    SkillRecallHit,
    SkillBankStats,
    SkillBank,
} from "./skillBank.cp";

export {
    BELIEF_MAX_RECORDS,
    BELIEF_MAX_AUDIT_ENTRIES,
    BELIEF_NO_ID,
    beliefLogOddsReinforce,
    beliefHalfLifeDecay,
    beliefEffectiveWeight,
    BeliefStatus,
    BeliefRevisionAction,
    BeliefRevisionConfig,
    BeliefRecord,
    BeliefRevisionResult,
    BeliefConflictAudit,
    BeliefStateStats,
    BeliefStateStore,
} from "./beliefState.cp";

export {
    GOAL_TREE_MAX_NODES,
    GOAL_TREE_MAX_DEPTH,
    GOAL_TREE_NAME_MAX_LEN,
    GOAL_NO_NODE,
    GOAL_PROGRESS_FULL_BPS,
    GoalOperator,
    GoalStatus,
    GoalNode,
    GoalTreeStats,
    GoalTree,
} from "./goalTree.cp";

export {
    REFLEXION_MAX_LESSONS,
    REFLEXION_TEXT_MAX_LEN,
    REFLEXION_NO_ID,
    REFLEXION_BPS_SCALE,
    ReflexionErrorKind,
    ReflexionStatus,
    ReflexionConfig,
    ReflexionLesson,
    ReflexionRecallHit,
    ReflexionBankStats,
    ReflexionBank,
    formatLessonsPromptBlock,
} from "./reflexion.cp";

export {
    LOOP_GUARD_MAX_STEPS,
    LOOP_GUARD_MAX_COOLDOWNS,
    LOOP_GUARD_DEFAULT_MAX_STEPS,
    LOOP_GUARD_DEFAULT_MAX_TOKENS,
    LoopAction,
    LoopTriggerReason,
    TrajectoryStep,
    LoopGuardConfig,
    LoopGuardDecision,
    ToolCooldownEntry,
    LoopGuardStats,
    hashTrajectoryBytes,
    formatLoopAction,
    formatLoopReason,
    AgentLoopGuard,
} from "./loopGuard.cp";

export {
    TOOL_HISTORY_MAX_ENTRIES,
    TOOL_HISTORY_NAME_MAX_LEN,
    TOOL_HISTORY_PREVIEW_MAX_LEN,
    TOOL_HISTORY_NO_REF,
    TOOL_HISTORY_DEFAULT_STUB_TOKENS,
    TOOL_HISTORY_BPS_SCALE,
    ToolOutputStatus,
    ToolOutputEntry,
    ToolHistoryConfig,
    ToolCompactReport,
    ToolHistoryStats,
    hashToolPayload,
    estimateToolTokens,
    formatToolStatus,
    ToolHistoryStore,
} from "./toolHistory.cp";

export {
    SCRATCHPAD_MAX_STEPS,
    SCRATCHPAD_MAX_CHECKPOINTS,
    SCRATCHPAD_TEXT_MAX_LEN,
    SCRATCHPAD_TAG_MAX_LEN,
    SCRATCHPAD_NO_STEP,
    ScratchStepKind,
    ScratchStep,
    ScratchCheckpoint,
    ScratchpadStats,
    ScratchRenderReport,
    formatScratchStepKind,
    AgentScratchpad,
} from "./scratchpad.cp";

export {
    PLAYBOOK_MAX_ENTRIES,
    PLAYBOOK_MAX_STEPS,
    PLAYBOOK_NAME_MAX_LEN,
    PLAYBOOK_STEP_MAX_LEN,
    PlaybookStatus,
    PlaybookStepSpec,
    PlaybookEntry,
    PlaybookQuery,
    PlaybookMatch,
    PlaybookStoreStats,
    PlaybookMemoryStore,
} from "./playbook.cp";
