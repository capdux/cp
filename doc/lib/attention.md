# `cap:attention` — Transformer Attention, KV-Cache & Token Sampler

## 1. 概述

`cap:attention`（`cap.attention`）是 CP 语言纯原生实现的工业级大语言模型推理核心算子库，提供数值稳定的 Scaled Dot-Product Attention (MHA)、Grouped-Query Attention (GQA)、支持 Speculative Decoding 回滚的自回归 `KvCache`，以及支持 Temperature、Repetition Penalty、Top-K 与 Top-P (Nucleus) 的确定性 Token 采样器。

## 2. 核心类型

- **`AttnError`**: 统一错误集合（`InvalidDimension`, `HeadMismatch`, `InvalidStartPos`, `EmptyInput`, `CacheFull`, `InvalidLayer`, `InvalidTemperature`, `InvalidTopK`, `InvalidTopP`, `OutOfMemory`）。
- **`PagedKvSlot`**: 单步追加返回的槽位元信息（`layer`, `seq_offset`, `num_tokens`）。
- **`KvCache`**: 多层自回归 KV 缓存容器，按 `[layer][seq_pos][kv_head][head_dim]` 布局存储，支持 `appendStep`、`truncateTo` 回滚与 `reset`。
- **`KV_INVALID_BLOCK` / `KvBlockMeta` / `KvAllocStats` / `PagedKvAllocator` / `BlockTable`** (`kv_cache.cp`)：零堆分配 PagedAttention 物理块管理器，支持系统提示词前缀哈希零拷贝共享、释放后缓存复用与 LRU 驱逐、Beam Search `retainBlock` + 引用计数 CoW（共享态分配新块，独占态零拷贝原地返回）以及 `BlockTable` 逻辑到物理槽位映射。
- **`SamplingConfig`**: 采样配置（`temperature`, `top_k`, `top_p`, `repetition_penalty`, `seed`）。
- **`RopeConfig` / RoPE / RMSNorm / LayerNorm / SwiGLU / GeLU** (`rope.cp`)：零堆分配 Transformer 基础推理核，支持 NTK-Aware 长上下文缩放的 RoPE 旋转位置编码、RMSNorm/LayerNorm 归一化与 SiLU/GeLU/SwiGLU 激活函数。
- **`QUANT_BLOCK_SIZE` / `BlockQ8_0` / `BlockQ4_0` / GEMV** (`quant_matmul.cp`)：零堆分配 GGML 风格 32 元素分块量化与整型累加矩阵向量乘法（`W_q8 * x_q8` 与经典 llama.cpp `W_q4 * x_q8` 混合量化推理核）。
- **`OnlineSoftmaxState` / `FlashAttnConfig` / `flashAttentionForward` / `flashAttentionSingleQuery`** (`flash.cp`)：零堆分配 O(1) 额外内存 FlashAttention-2 分块在线 Softmax 前向核，绝不物化 $N \times N$ 分数矩阵，原生支持因果掩码、Mistral 滑动窗口注意力与单 token 自回归解码热路径。
- **`MoeGateKind` / `MoeRouteEntry` / `MoeDispatchStats` / `moeRouteToken` / `moeDispatchBatch` / `moeLoadBalanceLoss` / `moeCombineOutputs`** (`moe.cp`)：零堆分配稀疏混合专家（Sparse MoE）推理调度核，对标 Mixtral / Switch Transformer / Qwen3-MoE / DeepSeek-V3，支持 Softmax 与 Sigmoid 门控、无辅助损失 `expert_bias` 均衡偏置、容量受限 CSR 分桶派发（含溢出丢弃统计）、负载均衡辅助损失 $L_{\text{aux}}$ 与多专家加权输出合并。
- **`LoraAdapter` / `loraApplyAdd` / `loraApplyWeightedSum` / `loraMergeIntoBase` / `loraUnmergeFromBase`** (`lora.cp`)：零堆分配多租户 LoRA 低秩适配器前向推理与权重热融合核，支持零拷贝借用 `A/B` 低秩矩阵、$O(r(d_{\text{in}} + d_{\text{out}}))$ 动态增量叠加、多 LoRA 加权混合以及零推理开销的静态热合并与在线热卸载。
- **`softplus` / `ssmSelectiveStep` / `ssmSelectiveScan` / `causalConv1dStep`** (`ssm.cp`)：零堆分配 Mamba / S6 选择性状态空间模型（Selective SSM）$O(N)$ 线性复杂度长上下文推理核与深度可分离因果一维卷积，支持 ZOH 离散化（$A = -\exp(a_{\log}) < 0$ 严格稳定）、分块预填充无缝续接自回归单步生成以及固定环形历史缓存因果卷积。
- **`GGUF_MAGIC` / `GgufValueType` / `GgmlType` / `GgufHeader` / `GgufKvEntry` / `GgufTensorInfo` / `parseGgufHeader` / `parseGgufFile` / `ggufFindKv` / `ggufFindTensor` / `ggufTensorByteSize`** (`gguf.cp`)：零堆分配零拷贝 GGUF v2/v3 大模型权重文件解析器，直接面向 mmap 缓冲区解析头、元数据 KV（自动提取 `general.alignment`）、张量描述符与对齐 `data_offset`，并精确计算 F32/F16/BF16/Q4_0/Q8_0/K-Quants 张量物理字节数。
- **`SafeDtype` / `SafeTensorDesc` / `SafeTensorsInfo` / `parseSafeTensors` / `safeTensorFind` / `safeTensorBytes`** (`safetensors.cp`)：零堆分配零拷贝 HuggingFace Safetensors 模型权重解析器，直接面向 mmap 缓冲区解析 `[8B LE header_len][JSON header][payload]`，跳过 `__metadata__`，严格校验 `shape * dtype.byteSize() == data_end - data_start` 与载荷边界，并返回只读零拷贝权重切片。
- **FP16 / BF16 / FP8 E4M3FN 低精度浮点编解码核** (`fp16.cp`)：零堆分配位级精确 AI 浮点转换与批量小端解包/打包核，完整覆盖 IEEE-754 Binary16（含次正规与 RNE 舍入）、Brain Float16（RNE 防截断偏差）与 OCP/NVIDIA Hopper FP8 E4M3FN（偏置 7、饱和至 `±448.0`、张量级 `scale` 批量反量化）。

## 3. API 速查

| 符号 | 签名概要 | 说明 |
| --- | --- | --- |
| `scaledDotProductAttention` | `(alloc, q, k, v, seq_q, seq_k, head_dim, causal, start_pos): AttnError![]f32` | 数值稳定 Softmax 注意力，支持因果掩码与 `start_pos` 偏移 |
| `groupedQueryAttention` | `(alloc, q, k_cache, v_cache, num_q_heads, num_kv_heads, seq_k, head_dim): AttnError![]f32` | 分组查询注意力，校验 `num_q_heads % num_kv_heads == 0` |
| `KvCache.init` | `(alloc, num_layers, num_kv_heads, head_dim, max_seq_len): AttnError!KvCache` | 初始化多层 KV-Cache |
| `KvCache.deinit` | `(*_)` | 释放内部缓冲区 |
| `KvCache.appendStep` | `(*_, layer, k_vec, v_vec): AttnError!PagedKvSlot` | 追加单步 KV 向量 |
| `KvCache.seqLen` | `(layer): usize` | 查询指定层当前序列长度 |
| `KvCache.keysSlice` / `valuesSlice` | `(layer): []const f32` | 获取指定层当前有效 K/V 切片 |
| `KvCache.truncateTo` | `(*_, new_len)` | 回滚所有层至 `new_len`（用于 Speculative Decoding） |
| `KvCache.reset` | `(*_)` | 清空所有层缓存长度 |
| `sampleGreedy` | `(logits): AttnError!usize` | 贪心 Argmax 采样 |
| `sampleToken` | `(alloc, logits, recent_tokens, config): AttnError!usize` | 支持 RepPenalty + Temp + Top-K + Top-P + SplitMix64 采样 |
| `PagedKvAllocator.init` | `(blocks: *[]KvBlockMeta, block_size: u16): PagedKvAllocator?` | 零堆分配初始化固定物理块池（校验 `blocks.len > 0` 与 `block_size > 0`） |
| `PagedKvAllocator.allocateOrSharePrefix` | `(*_, prefix_hash: u64, tick: u64): u32?` | 命中相同 `prefix_hash` 直接 `ref_count += 1` 零拷贝共享；否则分配空闲块或 LRU 驱逐 `ref_count == 0` 缓存块 |
| `PagedKvAllocator.allocateFresh` | `(*_, tick: u64): u32?` | 分配独占无哈希新块（`ref_count = 1`） |
| `PagedKvAllocator.retainBlock` / `releaseBlock` | `(*_, block_id: u32, tick: u64): bool` | 引用计数递增（Beam Search 共享）与递减（归零后带哈希块自动保留为可复用缓存） |
| `PagedKvAllocator.copyOnWrite` | `(*_, block_id: u32, tick: u64): u32?` | `ref_count > 1` 时分配新块并递增 `cow_copies`；`ref_count == 1` 时零拷贝原地返回 `block_id` |
| `PagedKvAllocator.stats` | `(): KvAllocStats` | 返回总块数、空闲块、可驱逐缓存块、前缀命中/未命中与 CoW 次数 |
| `BlockTable.init` / `appendBlock` / `mapTokenToPhys` | `(phys_blocks, block_size)` / `(phys_id)` / `(logical_pos): ([2]u32)?` | 序列逻辑块表与 `[phys_block_id, slot_offset]` 映射 |
| `RopeConfig.default` | `(head_dim: usize): RopeConfig` | 构造默认 RoPE 配置（`base_theta = 10000.0`, `ntk_scale = 1.0`） |
| `ropePrecomputeTables` | `(cfg, max_seq_len, cos_out, sin_out): bool` | 预计算 `[max_seq_len][head_dim / 2]` RoPE cos/sin 查找表（支持 NTK 缩放） |
| `ropeApplyInPlace` | `(vec_inout, pos, head_dim, cos_table, sin_table): bool` | 对单头 Q/K 向量执行严格保范数的半切分旋转位置编码 |
| `rmsNorm` / `layerNorm` | `(x, weight, eps, out): bool` / `(x, gamma, beta, eps, out): bool` | 零堆分配 RMSNorm（支持原地与空权重）与 LayerNorm 归一化核 |
| `silu` / `geluTanh` / `swigluForward` | `(x): f64` / `(x): f64` / `(gate, up, out): bool` | 现代 LLM FFN 激活核（`swigluForward`: `out[i] = silu(gate[i]) * up[i]`） |
| `quantizeRowQ8_0` / `dequantizeRowQ8_0` | `(src, blocks_out): bool` / `(blocks, dst): bool` | Q8_0 32 元素分块量化/反量化（要求 `src.len % 32 == 0`，~4x 压缩） |
| `dotQ8_0` / `gemvQ8_0` | `(a, b): f64?` / `(w_q8, x_q8, rows, bpr, y_out): bool` | 块内 `i32` 纯整型累加点积与 Q8_0 GEMV（`y = W * x`） |
| `quantizeRowQ4_0` / `dequantizeRowQ4_0` | `(src, blocks_out): bool` / `(blocks, dst): bool` | Q4_0 32 元素 4-bit 打包/解包（16B/块，~8x 压缩） |
| `gemvQ4_0_Q8_0` | `(w_q4, x_q8, rows, bpr, y_out): bool` | 经典 llama.cpp `W_q4 * x_q8` 混合量化推理 GEMV（块内 `i32` 乘加） |
| `OnlineSoftmaxState.init` / `update` | `(): OnlineSoftmaxState` / `(*_, score: f64): f64` | 在线 Softmax 状态累加原语，返回旧累加器重缩放系数 `exp(old_max - new_max)` |
| `FlashAttnConfig.default` | `(): FlashAttnConfig` | 默认配置（`scale = auto`、`causal = false`、`window_size = 0`、`block_kv = 32`） |
| `flashAttentionForward` | `(q, k, v, seq_q, seq_kv, head_dim, cfg, out): bool` | O(1) 额外内存分块在线 Softmax 前向核（支持因果掩码与 Mistral 滑动窗口） |
| `flashAttentionSingleQuery` | `(q_vec, k, v, seq_kv, head_dim, scale, out_vec): bool` | 自回归 Decode 单 token 生成热路径 |
| `moeRouteToken` | `(router_logits, expert_bias, top_k, kind, renorm_topk, out_routes): bool` | 稀疏 MoE Top-K 专家门控路由（支持 `Softmax`/`Sigmoid`、可选 `expert_bias` 均衡偏置与 Top-K 权重重归一化） |
| `moeDispatchBatch` | `(token_routes, n_tokens, top_k, n_experts, expert_capacity, expert_offsets_out, expert_tokens_out, expert_weights_out, counts_scratch): MoeDispatchStats?` | 容量受限批量 Token-to-Expert CSR 分桶派发，按到达顺序截断溢出并统计 `dispatched`/`dropped` |
| `moeLoadBalanceLoss` | `(router_probs_flat, chosen_experts_flat, n_tokens, n_experts, top_k, frac_scratch, prob_scratch): f64?` | Switch/Mixtral 负载均衡辅助损失 $L_{\text{aux}} = E \sum f_e P_e$（完美均匀为 `1.0`，单专家坍缩为 `E`） |
| `moeCombineOutputs` | `(expert_outs_flat, routes, hidden_dim, out_vec): bool` | 按 Top-K 路由权重加权合并多专家隐藏层输出 |
| `LoraAdapter.init` / `scaling` | `(id, in_dim, out_dim, rank, alpha, a_mat, b_mat): LoraAdapter?` / `(): f64` | 零拷贝构造并校验 LoRA 适配器描述符，返回缩放因子 `alpha / rank` |
| `loraApplyAdd` | `(adapter, x, y_inout, hidden_scratch): bool` | 单适配器增量前向 `y += (alpha / r) * B * (A * x)`，延迟 $O(r(d_{\text{in}} + d_{\text{out}}))$ |
| `loraApplyWeightedSum` | `(adapters, weights, x, y_inout, hidden_scratch): bool` | 多租户多 LoRA 加权组合叠加（原子校验，支持零权重跳过） |
| `loraMergeIntoBase` / `loraUnmergeFromBase` | `(base_weight_inout, adapter): bool` | 静态权重热合并 `W += (alpha / r) * B * A` 与精确逆操作热卸载 `W -= (alpha / r) * B * A` |
| `softplus` | `(x: f64): f64` | 数值稳定 `ln(1 + exp(x))`，`x > 35.0` 线性直通防溢出 |
| `ssmSelectiveStep` | `(x, dt, a_log, b_vec, c_vec, d_skip, d_inner, d_state, state_inout, y_out): bool` | Mamba S6 单步 ZOH 选择性状态空间递推（$A = -\exp(a_{\log}) < 0$） |
| `ssmSelectiveScan` | `(x_seq, dt_seq, a_log, b_seq, c_seq, d_skip, seq_len, d_inner, d_state, state_inout, y_seq_out): bool` | 整序列 $O(N)$ 选择性扫描，支持分块预填充后无缝续接单步解码 |
| `causalConv1dStep` | `(x, weight, bias, d_inner, kernel_size, conv_state_inout, y_out): bool` | Mamba 深度可分离因果 1D 卷积单步核（带 `[d_inner, kernel_size - 1]` 历史状态） |
| `parseGgufHeader` | `(data: []const u8): GgufHeader?` | 快速校验 GGUF 24B 文件头、`GGUF_MAGIC` 与 `version ∈ {2, 3}` |
| `parseGgufFile` | `(data, kvs_out, tensors_out): GgufHeader?` | 单遍零拷贝解析全部元数据 KV（自动应用 `general.alignment`）与张量描述符并计算对齐 `data_offset` |
| `ggufFindKv` / `ggufFindTensor` | `(kvs, key): usize?` / `(tensors, name): usize?` | 按键名或张量名检索元数据与张量描述符索引 |
| `ggufTensorByteSize` | `(dtype, dims, n_dims): u64?` | 精确计算 `F32`/`F16`/`BF16`/`Q4_0`/`Q8_0`/`K-Quants` 张量物理字节数并校验分块对齐 |
| `parseSafeTensors` | `(data, tensors_out): SafeTensorsInfo?` | 零堆分配单遍解析 Safetensors 头与张量描述符（跳过 `__metadata__`，严格校验长度与边界） |
| `safeTensorFind` | `(tensors, name): usize?` | 按张量名检索 `SafeTensorDesc` 索引 |
| `safeTensorBytes` | `(data, info, desc): ([]const u8)?` | 零拷贝返回张量原始权重字节切片（支持 0D 标量至 6D 张量） |
| `f16BitsToF32` / `f32ToF16Bits` | `(bits: u16): f32` / `(val: f32): u16` | IEEE-754 FP16 精确位级互转（支持次正规、`±0`/`±Inf`/NaN 与 RNE 舍入） |
| `decodeF16SliceLe` / `encodeF16SliceLe` | `(src_bytes, dst): bool` / `(src, dst_bytes): bool` | 批量小端 FP16 权重字节流与 `f32` 切片互转 |
| `bf16BitsToF32` / `f32ToBf16Bits` | `(bits: u16): f32` / `(val: f32): u16` | Brain Float16 精确互转（带 Round-to-Nearest-Even 舍入防系统偏差） |
| `decodeBf16SliceLe` / `encodeBf16SliceLe` | `(src_bytes, dst): bool` / `(src, dst_bytes): bool` | 批量小端 BF16 权重字节流与 `f32` 切片互转 |
| `fp8E4m3ToF32` / `f32ToFp8E4m3` / `decodeFp8E4m3Slice` | `(bits: u8): f32` / `(val: f32): u8` / `(src_bytes, scale, dst): bool` | OCP FP8 E4M3FN 精确互转（最大 `448.0`、饱和钳位、`0x7F`/`0xFF`=NaN）与带缩放批量反量化 |

## 4. 示例

```cp
import { KvCache, groupedQueryAttention, SamplingConfig, sampleToken } from "cap:attention";

let cache = try KvCache.init(alloc, 2, 2, 4, 16);
defer cache.deinit();

_ = try cache.appendStep(0, k_step, v_step);
const ctx = try groupedQueryAttention(
    alloc,
    q_step,
    cache.keysSlice(0),
    cache.valuesSlice(0),
    8,
    2,
    cache.seqLen(0),
    4,
);
defer alloc.free(ctx);

const tok = try sampleToken(alloc, logits, &[_]usize{1, 4}, SamplingConfig{
    .temperature = 0.8,
    .top_k = 40,
    .top_p = 0.9,
    .repetition_penalty = 1.1,
    .seed = 12345,
});
```

### PagedAttention 前缀共享、CoW 与逻辑块表映射示例 (`kv_cache.cp`)

```cp
import {
    KV_INVALID_BLOCK,
    KvBlockMeta,
    PagedKvAllocator,
    BlockTable,
} from "cap:attention";

pub fn demoPagedKvPrefixSharing() {
    let pool_buf: [16]KvBlockMeta = [KvBlockMeta{}; 16];
    let pool: []KvBlockMeta = pool_buf[0..];
    if (alloc_init := PagedKvAllocator.init(pool, 16)) {
        let kv_alloc = alloc_init;
        const sys_prefix_hash: u64 = 0xDEADBEEF0001;

        // 请求 1 与请求 2 零拷贝共享系统提示词前缀块
        const b1 = kv_alloc.allocateOrSharePrefix(sys_prefix_hash, 10) ?? KV_INVALID_BLOCK;
        const b2 = kv_alloc.allocateOrSharePrefix(sys_prefix_hash, 12) ?? KV_INVALID_BLOCK; // prefix_hits += 1

        // 请求 2 修改块内容触发 CoW，自动分配独占新块
        const b2_cow = kv_alloc.copyOnWrite(b2, 15) ?? KV_INVALID_BLOCK;

        let phys_buf: [8]u32 = [0; 8];
        let phys: []u32 = phys_buf[0..];
        if (bt_init := BlockTable.init(phys, 16)) {
            let bt = bt_init;
            * = bt.appendBlock(b1);
            * = bt.appendBlock(b2_cow);
            if (mapped := bt.mapTokenToPhys(19)) {
                // mapped == [b2_cow, 3]
                * = mapped;
            }
        }
    }
}
```

### RoPE（含 NTK 缩放）、RMSNorm 与 SwiGLU 推理核示例 (`rope.cp`)

```cp
import {
    RopeConfig,
    ropePrecomputeTables,
    ropeApplyInPlace,
    rmsNorm,
    swigluForward,
} from "cap:attention";

pub fn demoTransformerKernels() {
    // 1. 预计算 NTK-Aware RoPE 表并对 Q 向量应用位置编码
    const cfg = RopeConfig{ head_dim: 4, base_theta: 10000.0, ntk_scale: 2.0 };
    let cos_buf: [16]f64 = [0.0; 16]; // max_seq_len=8 * (head_dim/2)=2
    let sin_buf: [16]f64 = [0.0; 16];
    let cos_s: []f64 = cos_buf[0..];
    let sin_s: []f64 = sin_buf[0..];
    * = ropePrecomputeTables(cfg, 8, cos_s, sin_s);

    let q_buf: [4]f64 = [1.0, -0.5, 0.25, 0.75];
    let q_s: []f64 = q_buf[0..];
    * = ropeApplyInPlace(q_s, 5, 4, cos_buf[0..], sin_buf[0..]);

    // 2. RMSNorm 与 SwiGLU FFN
    let norm_buf: [4]f64 = [0.0; 4];
    let ffn_buf: [4]f64 = [0.0; 4];
    let norm_s: []f64 = norm_buf[0..];
    let ffn_s: []f64 = ffn_buf[0..];
    const empty_w: []const f64 = q_buf[0..0];
    * = rmsNorm(q_buf[0..], empty_w, 0.00001, norm_s);
    * = swigluForward(norm_buf[0..], q_buf[0..], ffn_s);
}
```

### GGML 风格 Q8_0 / Q4_0 分块量化与混合推理 GEMV 示例 (`quant_matmul.cp`)

```cp
import {
    BlockQ8_0,
    BlockQ4_0,
    quantizeRowQ8_0,
    quantizeRowQ4_0,
    gemvQ4_0_Q8_0,
} from "cap:attention";

pub fn demoQuantizedGemv(w_row_32: []const f64, x_32: []const f64) {
    let w_q4_buf: [1]BlockQ4_0 = [BlockQ4_0{}; 1];
    let x_q8_buf: [1]BlockQ8_0 = [BlockQ8_0{}; 1];
    let w_q4: []BlockQ4_0 = w_q4_buf[0..];
    let x_q8: []BlockQ8_0 = x_q8_buf[0..];

    * = quantizeRowQ4_0(w_row_32, w_q4);
    * = quantizeRowQ8_0(x_32, x_q8);

    let y_buf: [1]f64 = [0.0];
    let y: []f64 = y_buf[0..];
    * = gemvQ4_0_Q8_0(w_q4_buf[0..], x_q8_buf[0..], 1, 1, y);
}
```

### FlashAttention-2 分块在线 Softmax 与滑动窗口示例 (`flash.cp`)

```cp
import {
    FlashAttnConfig,
    flashAttentionForward,
    flashAttentionSingleQuery,
} from "cap:attention";

pub fn demoFlashAttention2(q: []const f64, k: []const f64, v: []const f64) {
    // 1. Mistral 风格滑动窗口因果注意力（0 临时 N x N 矩阵分配）
    let out_buf: [32]f64 = [0.0; 32]; // seq_q=8 * head_dim=4
    let out: []f64 = out_buf[0..];
    const cfg = FlashAttnConfig{
        scale: 0.0,       // 自动取 1 / sqrt(head_dim)
        causal: true,
        window_size: 4,   // 仅关注最近 4 个 token [i - 3, i]
        block_kv: 16,
    };
    * = flashAttentionForward(q, k, v, 8, 8, 4, cfg, out);

    // 2. 自回归单 token Decode 热路径
    let step_out_buf: [4]f64 = [0.0; 4];
    let step_out: []f64 = step_out_buf[0..];
    * = flashAttentionSingleQuery(q[28..32], k, v, 8, 4, 0.0, step_out);
}
```

### 稀疏混合专家（MoE）Top-K 路由、CSR 容量派发与加权合并示例 (`moe.cp`)

```cp
import {
    MoeGateKind,
    MoeRouteEntry,
    moeRouteToken,
    moeDispatchBatch,
    moeCombineOutputs,
} from "cap:attention";

pub fn demoSparseMoeRouting() {
    // 1. DeepSeek-V3 风格 Sigmoid Top-2 路由（带专家均衡偏置 + Top-K 重归一化）
    const logits: [4]f64 = [1.2, 0.9, 0.3, -0.4];
    const bias: [4]f64 = [-0.25, 0.0, 0.30, 0.0];
    let routes_buf: [2]MoeRouteEntry = [MoeRouteEntry{}; 2];
    let routes: []MoeRouteEntry = routes_buf[0..];
    * = moeRouteToken(logits[0..], bias[0..], 2, MoeGateKind.Sigmoid, true, routes);

    // 2. 批量容量受限 CSR 派发（此处演示 1 token x Top-2 到 4 专家，单专家容量 2）
    let offsets_buf: [5]usize = [0; 5];
    let tokens_buf: [2]usize = [0; 2];
    let weights_buf: [2]f64 = [0.0; 2];
    let scratch_buf: [4]usize = [0; 4];
    let offsets: []usize = offsets_buf[0..];
    let tokens: []usize = tokens_buf[0..];
    let weights: []f64 = weights_buf[0..];
    let scratch: []usize = scratch_buf[0..];
    if (stats := moeDispatchBatch(routes_buf[0..], 1, 2, 4, 2, offsets, tokens, weights, scratch)) {
        * = stats; // stats.dispatched == 2, stats.dropped == 0
    }

    // 3. 加权合并选中专家的隐藏层输出（hidden_dim = 4）
    const expert_outs: [8]f64 = [
        1.0, 2.0, 3.0, 4.0,
        4.0, 3.0, 2.0, 1.0,
    ];
    let combined_buf: [4]f64 = [0.0; 4];
    let combined: []f64 = combined_buf[0..];
    * = moeCombineOutputs(expert_outs[0..], routes_buf[0..], 4, combined);
}
```

### 多租户 LoRA 低秩适配器增量推理与权重热融合示例 (`lora.cp`)

```cp
import {
    LoraAdapter,
    loraApplyAdd,
    loraMergeIntoBase,
    loraUnmergeFromBase,
} from "cap:attention";

pub fn demoMultiTenantLora(
    base_w: *[]f64,
    a_mat: []const f64,
    b_mat: []const f64,
    x: []const f64,
    y_inout: *[]f64,
) {
    // 1. 零拷贝绑定 LoRA 适配器（in_dim=4, out_dim=3, rank=2, alpha=4.0）
    if (ad := LoraAdapter.init(101, 4, 3, 2, 4.0, a_mat, b_mat)) {
        // 2. 多租户动态增量前向：y += (alpha / r) * B * (A * x)
        let scratch_buf: [2]f64 = [0.0; 2];
        let scratch: []f64 = scratch_buf[0..];
        * = loraApplyAdd(ad, x, y_inout, scratch);

        // 3. 单租户部署热合并到基础权重，切换适配器时热卸载还原
        * = loraMergeIntoBase(base_w, ad);
        * = loraUnmergeFromBase(base_w, ad);
    }
}
```

### Mamba / S6 选择性状态空间模型与因果 1D 卷积示例 (`ssm.cp`)

```cp
import {
    softplus,
    ssmSelectiveStep,
    ssmSelectiveScan,
    causalConv1dStep,
} from "cap:attention";

pub fn demoMambaS6Step(
    x_in: []const f64,
    conv_w: []const f64,
    conv_b: []const f64,
    a_log: []const f64,
    b_vec: []const f64,
    c_vec: []const f64,
    d_skip: []const f64,
    conv_state: *[]f64,
    ssm_state: *[]f64,
) {
    // 1. 因果深度可分离 1D 卷积 (d_inner = 2, kernel_size = 4)
    let conv_out_buf: [2]f64 = [0.0; 2];
    let conv_out: []f64 = conv_out_buf[0..];
    * = causalConv1dStep(x_in, conv_w, conv_b, 2, 4, conv_state, conv_out);

    // 2. 计算正步长 dt = softplus(dt_raw) 并执行单步 S6 递推 (d_state = 4)
    const dt_buf: [2]f64 = [softplus(0.25), softplus(-0.5)];
    let y_buf: [2]f64 = [0.0; 2];
    let y_out: []f64 = y_buf[0..];
    * = ssmSelectiveStep(conv_out_buf[0..], dt_buf[0..], a_log, b_vec, c_vec, d_skip, 2, 4, ssm_state, y_out);
}
```

### GGUF v2/v3 大模型权重零拷贝解析示例 (`gguf.cp`)

```cp
import {
    GgufKvEntry,
    GgufTensorInfo,
    parseGgufFile,
    ggufFindKv,
    ggufFindTensor,
    ggufTensorByteSize,
} from "cap:attention";

pub fn demoParseGgufWeights(mmap_bytes: []const u8) {
    let kvs_buf: [64]GgufKvEntry = [GgufKvEntry{}; 64];
    let tensors_buf: [128]GgufTensorInfo = [GgufTensorInfo{}; 128];
    let kvs: []GgufKvEntry = kvs_buf[0..];
    let tensors: []GgufTensorInfo = tensors_buf[0..];

    if (hdr := parseGgufFile(mmap_bytes, kvs, tensors)) {
        const kv_slice = kvs_buf[0 .. (hdr.kv_count as usize)];
        const tensor_slice = tensors_buf[0 .. (hdr.tensor_count as usize)];
        if (arch_i := ggufFindKv(kv_slice, "general.architecture")) {
            * = kv_slice[arch_i].str_val; // e.g. "llama" / "qwen2"
        }
        if (t_i := ggufFindTensor(tensor_slice, "blk.0.attn_q.weight")) {
            const info = tensor_slice[t_i];
            if (n_bytes := ggufTensorByteSize(info.dtype, info.dims, info.n_dims)) {
                const payload_start = hdr.data_offset + (info.offset as usize);
                * = payload_start;
                * = n_bytes;
            }
        }
    }
}
```

### HuggingFace Safetensors 权重零拷贝加载示例 (`safetensors.cp`)

```cp
import {
    SafeTensorDesc,
    parseSafeTensors,
    safeTensorFind,
    safeTensorBytes,
} from "cap:attention";

pub fn demoLoadSafeTensors(mmap_bytes: []const u8) {
    let descs_buf: [128]SafeTensorDesc = [SafeTensorDesc{}; 128];
    let descs: []SafeTensorDesc = descs_buf[0..];

    if (info := parseSafeTensors(mmap_bytes, descs)) {
        const active = descs_buf[0..info.tensor_count];
        if (idx := safeTensorFind(active, "model.embed_tokens.weight")) {
            if (raw_weights := safeTensorBytes(mmap_bytes, info, active[idx])) {
                * = raw_weights; // 零拷贝只读原始权重切片
            }
        }
    }
}
```

### 低精度权重解包（FP16 / BF16 / FP8 E4M3FN）示例 (`fp16.cp`)

```cp
import {
    decodeF16SliceLe,
    decodeBf16SliceLe,
    decodeFp8E4m3Slice,
} from "cap:attention";

pub fn demoDecodeLowPrecisionWeights(
    f16_bytes: []const u8,
    bf16_bytes: []const u8,
    fp8_bytes: []const u8,
) {
    let out_buf: [64]f32 = [0.0; 64];
    let out: []f32 = out_buf[0..];
    * = decodeF16SliceLe(f16_bytes, out);
    * = decodeBf16SliceLe(bf16_bytes, out);
    * = decodeFp8E4m3Slice(fp8_bytes, 0.015625, out); // 张量级 scale 反量化
}
```
