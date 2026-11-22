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
