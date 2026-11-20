// Source: cp/lib/tensor/index.cp (Unified Exports for cap:tensor)
// Unified Public Exports for Multi-dimensional Strided Tensor & AI/LLM Operators (cap:tensor & cap.tensor)

export {
    MAX_TENSOR_DIMS,
    TensorError,
    Tensor,
} from "./core.cp";

export {
    add,
    mul,
    scale,
    matmul,
} from "./ops.cp";

export {
    softmaxRows,
    rmsNorm,
    layerNorm,
    silu,
    gelu,
    applyRope,
} from "./nn.cp";
