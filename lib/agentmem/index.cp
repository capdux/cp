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
