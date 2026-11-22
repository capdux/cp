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
} from "./kv_cache.cp";
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
} from "./quant_matmul.cp";

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
} from "./quant_int4.cp";

export {
    KvQuantParams,
    quantizeTokenKvI8,
    dequantizeTokenKvI8,
    dotQueryQuantKeyI8,
    quantizedAttentionSingleQuery,
} from "./kv_quant.cp";

export {
    SwaConfig,
    RingKvCache,
    swaAttentionSingleQuery,
    ringKvAttentionSingleQuery,
} from "./sliding_window.cp";

export {
    PREFIX_INVALID_BLOCK,
    computePrefixBlockHash,
    PrefixBlockEntry,
    PrefixMatchResult,
    PrefixCacheStats,
    KvPrefixCache,
} from "./prefix_cache.cp";

export {
    gemmTransposedBF32,
    gemmF32,
    batchedGemmTransposedBF32,
    splitGemmRowsF32,
} from "./tiled_gemm.cp";

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
} from "./vision_patch.cp";

export {
    FP8_E4M3_MAX_FINITE,
    Fp8BlockScale,
    computeBlockAmaxF32,
    quantizeBlockFp8E4m3,
    dequantizeBlockFp8E4m3,
    gemvBlockFp8E4m3,
} from "./quant_fp8.cp";

export {
    VocabBitset,
    DfaEdge,
    ConstrainedDfa,
    applyVocabMaskInPlace,
    applyTemperatureAndTopKMaskInPlace,
} from "./constrained_mask.cp";

export {
    RopeScalingKind,
    YarnConfig,
    yarnAttentionMscale,
    computeRopeInvFreqs,
    applyScaledRopeInPlaceF32,
} from "./rope_yarn.cp";

export {
    SPEC_TREE_NO_PARENT,
    DraftTreeNode,
    TreeVerifyResult,
    validateDraftTree,
    buildTreeAttentionMask,
    verifyDraftTreeGreedy,
} from "./spec_tree.cp";

export {
    PAGED_KV_INVALID_BLOCK,
    PagedKvPoolStats,
    PagedKvBlockPool,
    PagedTokenSlot,
    SeqBlockTable,
    pagedAttentionDecodeF32,
} from "./paged_kv.cp";

export {
    IterSchedulerConfig,
    SeqRunState,
    SeqSchedEntry,
    ScheduledStepItem,
    IterStepPlan,
    IterSchedulerStats,
    ContinuousIterScheduler,
} from "./iter_scheduler.cp";

export {
    MAX_KV_EVICTION_SLOTS,
    KvEvictionPolicyKind,
    KvTokenRole,
    KvEvictionConfig,
    KvSlotMeta,
    KvEvictionResult,
    KvEvictionStats,
    KvEvictionManager,
} from "./kv_eviction.cp";

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
} from "./lora_router.cp";

export {
    GUIDED_TRIE_MAX_NODES,
    GUIDED_TRIE_NO_NODE,
    GuidedTrieNode,
    GuidedTrieCursor,
    GuidedTrieStats,
    GuidedTokenTrie,
} from "./guided_trie.cp";

export {
    SAMPLER_MAX_CANDIDATES,
    NucleusSamplerConfig,
    SamplerCandidate,
    NucleusSampleOutcome,
    applyTokenPenaltiesInPlace,
    selectTopKCandidates,
    softmaxAndTruncateNucleus,
    sampleFromLogits,
} from "./nucleus_sampler.cp";

export {
    RADIX_KV_MAX_NODES,
    RADIX_KV_MAX_EDGE_TOKENS,
    RADIX_KV_NO_NODE,
    RadixKvNode,
    RadixMatchResult,
    RadixKvStats,
    RadixKvCacheTree,
} from "./radix_kv_tree.cp";

export {
    SPEC_REJECT_MAX_DRAFT_K,
    SpecRejectionOutcome,
    SpecStepSchedulerConfig,
    SpecStepScheduler,
    sampleResidualDistributionF32,
    verifySpeculativeRejectionF32,
} from "./spec_rejection.cp";
