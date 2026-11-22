// Source: cp/lib/attention/index.cp (cap:attention public entry point)
export { AttnError, scaledDotProductAttention, groupedQueryAttention } from "./sdpa.cp";
export {
    PagedKvSlot,
    KvCache,
    KV_INVALID_BLOCK,
    KvBlockMeta,
    KvAllocStats,
    PagedKvAllocator,
    BlockTable,
} from "./kvCache.cp";
export { SamplingConfig, sampleGreedy, sampleToken } from "./sampler.cp";

export {
    RopeConfig,
    ropePrecomputeTables,
    ropeApplyInPlace,
    rmsNorm,
    layerNorm,
    silu,
    geluTanh,
    swigluForward,
} from "./rope.cp";

export {
    QUANT_BLOCK_SIZE,
    BlockQ8_0,
    BlockQ4_0,
    quantizeRowQ8_0,
    dequantizeRowQ8_0,
    dotQ8_0,
    gemvQ8_0,
    quantizeRowQ4_0,
    dequantizeRowQ4_0,
    gemvQ4_0_Q8_0,
} from "./quantMatmul.cp";

export {
    OnlineSoftmaxState,
    FlashAttnConfig,
    flashAttentionForward,
    flashAttentionSingleQuery,
} from "./flash.cp";

export {
    MoeGateKind,
    MoeRouteEntry,
    MoeDispatchStats,
    moeRouteToken,
    moeDispatchBatch,
    moeLoadBalanceLoss,
    moeCombineOutputs,
} from "./moe.cp";

export {
    LoraAdapter,
    loraApplyAdd,
    loraApplyWeightedSum,
    loraMergeIntoBase,
    loraUnmergeFromBase,
} from "./lora.cp";

export {
    softplus,
    ssmSelectiveStep,
    ssmSelectiveScan,
    causalConv1dStep,
} from "./ssm.cp";

export {
    GGUF_MAGIC,
    GGUF_DEFAULT_ALIGNMENT,
    GgufValueType,
    GgmlType,
    GgufHeader,
    GgufKvEntry,
    GgufTensorInfo,
    parseGgufHeader,
    parseGgufFile,
    ggufFindKv,
    ggufFindTensor,
    ggufTensorByteSize,
} from "./gguf.cp";

export {
    SafeDtype,
    SafeTensorDesc,
    SafeTensorsInfo,
    parseSafeTensors,
    safeTensorFind,
    safeTensorBytes,
} from "./safetensors.cp";

export {
    f16BitsToF32,
    f32ToF16Bits,
    decodeF16SliceLe,
    encodeF16SliceLe,
    bf16BitsToF32,
    f32ToBf16Bits,
    decodeBf16SliceLe,
    encodeBf16SliceLe,
    fp8E4m3ToF32,
    f32ToFp8E4m3,
    decodeFp8E4m3Slice,
} from "./fp16.cp";

export {
    MlaConfig,
    alibiSlopes,
    alibiAttentionSingleQuery,
    mlaCompressKv,
    mlaDecodeSingleHead,
} from "./mla.cp";

export {
    PoolKind,
    embeddingLookup,
    poolHiddenStates,
    l2NormalizeInPlace,
    matryoshkaProject,
    cosineSimilarityMatrix,
} from "./embedding.cp";

export {
    DpoResult,
    logSumExp,
    crossEntropyLoss,
    sequencePerplexity,
    klDivergenceFromLogits,
    dpoLoss,
    infoNceLoss,
} from "./loss.cp";

export {
    LrScheduleConfig,
    AdamWConfig,
    LionConfig,
    cosineWarmupLr,
    linearWarmupDecayLr,
    gradGlobalL2Norm,
    clipGradNormInPlace,
    adamWStepInPlace,
    lionStepInPlace,
} from "./optimizer.cp";

export {
    Int4QuantMode,
    Int4GroupParams,
    packInt4Pair,
    unpackInt4Lo,
    unpackInt4Hi,
    quantizeGroupInt4Sym,
    quantizeGroupInt4Asym,
    dequantizeGroupInt4,
    gemvInt4Group,
} from "./quantInt4.cp";

export {
    KvQuantParams,
    quantizeTokenKvI8,
    dequantizeTokenKvI8,
    dotQueryQuantKeyI8,
    quantizedAttentionSingleQuery,
} from "./kvQuant.cp";

export {
    SwaConfig,
    RingKvCache,
    swaAttentionSingleQuery,
    ringKvAttentionSingleQuery,
} from "./slidingWin.cp";

export {
    PREFIX_INVALID_BLOCK,
    computePrefixBlockHash,
    PrefixBlockEntry,
    PrefixMatchResult,
    PrefixCacheStats,
    KvPrefixCache,
} from "./prefixCache.cp";

export {
    gemmTransposedBF32,
    gemmF32,
    batchedGemmTransposedBF32,
    splitGemmRowsF32,
} from "./tiledGemm.cp";

export {
    conv1dCausalF32,
    depthwiseConv1dF32,
    hzToMel,
    melToHz,
    buildMelFilterbankF32,
    applyMelFilterbankF32,
} from "./conv1d.cp";

export {
    ImageNormParams,
    normalizeRgbU8ToF32,
    patchifyHwcF32,
    patchEmbedLinearF32,
    build2dSinCosPosEmbF32,
    applyRope2dInPlaceF32,
} from "./vision.cp";

export {
    FP8_E4M3_MAX_FINITE,
    Fp8BlockScale,
    computeBlockAmaxF32,
    quantizeBlockFp8E4m3,
    dequantizeBlockFp8E4m3,
    gemvBlockFp8E4m3,
} from "./quantFp8.cp";

export {
    VocabBitset,
    DfaEdge,
    ConstrainedDfa,
    applyVocabMaskInPlace,
    applyTemperatureAndTopKMaskInPlace,
} from "./constrain.cp";

export {
    RopeScalingKind,
    YarnConfig,
    yarnAttentionMscale,
    computeRopeInvFreqs,
    applyScaledRopeInPlaceF32,
} from "./ropeYarn.cp";

export {
    SPEC_TREE_NO_PARENT,
    DraftTreeNode,
    TreeVerifyResult,
    validateDraftTree,
    buildTreeAttentionMask,
    verifyDraftTreeGreedy,
} from "./specTree.cp";

export {
    PAGED_KV_INVALID_BLOCK,
    PagedKvPoolStats,
    PagedKvBlockPool,
    PagedTokenSlot,
    SeqBlockTable,
    pagedAttentionDecodeF32,
} from "./pagedKv.cp";

export {
    IterSchedulerConfig,
    SeqRunState,
    SeqSchedEntry,
    ScheduledStepItem,
    IterStepPlan,
    IterSchedulerStats,
    ContinuousIterScheduler,
} from "./iterSched.cp";

export {
    MAX_KV_EVICTION_SLOTS,
    KvEvictionPolicyKind,
    KvTokenRole,
    KvEvictionConfig,
    KvSlotMeta,
    KvEvictionResult,
    KvEvictionStats,
    KvEvictionManager,
} from "./kvEvict.cp";

export {
    LORA_ROUTER_MAX_ADAPTERS,
    LORA_MAX_BATCH_GROUPS,
    LoraRouterConfig,
    LoraAdapterSlot,
    LoraAcquireOutcome,
    LoraBatchGroup,
    LoraRouterStats,
    MultiLoraRouter,
    planLoraSgmvBatch,
    applyLoraDeltaSimdF32,
} from "./loraRouter.cp";

export {
    GUIDED_TRIE_MAX_NODES,
    GUIDED_TRIE_NO_NODE,
    GuidedTrieNode,
    GuidedTrieCursor,
    GuidedTrieStats,
    GuidedTokenTrie,
} from "./guidedTrie.cp";

export {
    SAMPLER_MAX_CANDIDATES,
    NucleusSamplerConfig,
    SamplerCandidate,
    NucleusSampleOutcome,
    applyTokenPenaltiesInPlace,
    selectTopKCandidates,
    softmaxAndTruncateNucleus,
    sampleFromLogits,
} from "./nucleus.cp";

export {
    RADIX_KV_MAX_NODES,
    RADIX_KV_MAX_EDGE_TOKENS,
    RADIX_KV_NO_NODE,
    RadixKvNode,
    RadixMatchResult,
    RadixKvStats,
    RadixKvCacheTree,
} from "./radixKv.cp";

export {
    SPEC_REJECT_MAX_DRAFT_K,
    SpecRejectionOutcome,
    SpecStepSchedulerConfig,
    SpecStepScheduler,
    sampleResidualDistributionF32,
    verifySpeculativeRejectionF32,
} from "./specReject.cp";

export {
    PRC_MAX_NODES,
    PRC_MAX_POOL_SLOTS,
    PRC_NO_NODE,
    PRC_ROOT_NODE,
    PrefixRadixNode,
    PrefixRadixMatch,
    PrefixRadixInsertResult,
    PrefixRadixEvictReport,
    PrefixRadixStats,
    PrefixRadixCache,
} from "./radixCache.cp";

export {
    HEAL_MAX_VOCAB_TOKENS,
    HEAL_MASK_WORDS,
    HEAL_MAX_BYTE_POOL,
    HEAL_MAX_PREFIX_BYTES,
    HEAL_MAX_BACKTRACK_TOKENS,
    HealMatchMode,
    HealSessionState,
    TokenHealConfig,
    HealVocabTable,
    HealCandidateMask,
    TokenHealPlan,
    HealStepOutcome,
    TokenHealingSession,
    buildHealingCandidateMask,
    applyHealingLogitsMaskInPlace,
    planTokenHealing,
    planTokenHealingWithConfig,
} from "./tokenHeal.cp";

export {
    KV_COMPACTOR_MAX_SLOTS,
    KvSlotRole,
    KvCompactorSlot,
    KvCompactorConfig,
    KvCompactReport,
    KvCompactorStats,
    KvCacheCompactor,
} from "./kvCompactor.cp";

export {
    AWQ_MIN_CHANNEL_SCALE,
    AWQ_MAX_CHANNEL_SCALE,
    AwqGroupMeta,
    AwqQuantErrorStats,
    awqGroupsPerRow,
    awqPackedBytesPerRow,
    computeAwqChannelScales,
    awqQuantizeMatrix,
    awqDequantizeRow,
    awqGemvF32,
    evaluateAwqQuantError,
} from "./quantAwq.cp";

export {
    FLASH_DECODE_MAX_HEAD_DIM,
    FLASH_DECODE_MAX_SPLITS,
    FlashDecodePartial,
    FlashDecodeMergeStats,
    flashDecodePartition,
    mergeFlashDecodePartialsWithStats,
    mergeFlashDecodePartials,
    flashDecodeSplitK,
    flashDecodeSplitKGqa,
} from "./flashDecode.cp";

export {
    BEAM_MAX_WIDTH,
    BEAM_MAX_SEQ_LEN,
    BEAM_MAX_GROUPS,
    BEAM_MAX_FINISHED,
    beamLengthPenalty,
    beamNormalizedScore,
    beamHasRepeatedNgram,
    BeamCandidate,
    BeamSearchConfig,
    BeamHypothesis,
    BeamStepOutcome,
    BeamSearchStats,
    BeamSearchDecoder,
} from "./beamSearch.cp";

export {
    BATCH_SCHED_MAX_REQS,
    BATCH_SCHED_MAX_STEP_ITEMS,
    BatchReqState,
    BatchExecPhase,
    BatchSchedConfig,
    BatchRequestSlot,
    BatchStepEntry,
    BatchStepPlan,
    BatchSchedStats,
    ChunkedBatchScheduler,
} from "./batchSched.cp";

export {
    CACHE_ROUTE_MAX_REPLICAS,
    CACHE_ROUTE_LRU_BLOCKS,
    CACHE_ROUTE_BPS_SCALE,
    CacheRouteStatus,
    CacheRouteConfig,
    ReplicaBlockSlot,
    ReplicaNodeState,
    CacheRouteDecision,
    CacheRouterStats,
    CacheAwareRouter,
} from "./cacheAware.cp";

export {
    TOME_MAX_TOKENS,
    TOME_MAX_DIM,
    TOME_PERMILLE_SCALE,
    TokenMergeStatus,
    TokenMergeConfig,
    TokenMergeReport,
    mergeTokensF32,
    computeProportionalLogBias,
    unmergeTokensF32,
} from "./tokenMerge.cp";

export {
    KV_FORK_MAX_PHYS_BLOCKS,
    KV_FORK_MAX_SEQS,
    KV_FORK_MAX_LOGICAL_BLOCKS,
    KV_FORK_INVALID_BLOCK,
    KvForkOpStatus,
    KvForkPhysBlock,
    KvForkSequence,
    KvForkAppendOutcome,
    KvForkWritePlan,
    KvForkStats,
    KvForkManager,
} from "./kvFork.cp";

export {
    RMSNORM_DEFAULT_EPS,
    NormKernelError,
    NormConfig,
    rmsNormF32,
    rmsNormInPlaceF32,
    rmsNormUnitOffsetF32,
    rmsNormWithConfigF32,
    fusedAddRmsNormF32,
    qkRmsNormHeadsInPlaceF32,
    batchRmsNormF32,
    layerNormF32,
    siluF32,
    geluTanhF32,
    swigluF32,
    gegluF32,
    swigluInterleavedF32,
    swigluSplitF32,
} from "./rmsNorm.cp";

export {
    MROPE_MAX_HEAD_DIM,
    MROPE_MAX_SEQ_LEN,
    MropeError,
    MropeConfig,
    MropePos3d,
    MropeSegmentKind,
    MropePosBuilder,
    computeMropeInvFreqs,
    applyMropeHeadInPlaceF32,
    applyMropeMultiHeadInPlaceF32,
    applyMropeSequenceInPlaceF32,
} from "./mrope.cp";
