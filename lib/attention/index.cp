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
