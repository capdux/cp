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
} from "./working_mem.cp";

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
} from "./entity_graph.cp";
