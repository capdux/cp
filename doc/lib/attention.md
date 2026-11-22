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
- **`MlaConfig` / `alibiSlopes` / `alibiAttentionSingleQuery` / `mlaCompressKv` / `mlaDecodeSingleHead`** (`mla.cp`)：零堆分配 DeepSeek-V2/V3 多头潜在注意力（MLA：低秩 KV 潜空间压缩 + 矩阵吸收优化 + 解耦 RoPE）与 ALiBi 线性距离偏置注意力核，推理期无需重建完整 K/V 缓存，大幅降低 KV-Cache 显存占用。
- **`PoolKind` / `embeddingLookup` / `poolHiddenStates` / `l2NormalizeInPlace` / `matryoshkaProject` / `cosineSimilarityMatrix`** (`embedding.cp`)：零堆分配词嵌入查表、带 `attention_mask` 掩码的四流派序列池化（`Mean`/`Cls`/`LastToken`/`Max`，覆盖 BGE/E5/Nomic/Qwen3-Embedding）、Matryoshka 套娃维度截断重归一化与批量余弦相似度矩阵计算。
- **`DpoResult` / `logSumExp` / `crossEntropyLoss` / `sequencePerplexity` / `klDivergenceFromLogits` / `dpoLoss` / `infoNceLoss`** (`loss.cp`)：零堆分配数值稳定大模型训练与偏好对齐损失函数库，覆盖 Cross-Entropy（含 Label Smoothing）、带 `ignore_index` 序列困惑度、零概率缓冲 KL 散度、防溢出 DPO 偏好对齐损失（Rafailov et al. 2023）与 InfoNCE 对比学习损失。
- **`LrScheduleConfig` / `AdamWConfig` / `LionConfig` / `cosineWarmupLr` / `linearWarmupDecayLr` / `gradGlobalL2Norm` / `clipGradNormInPlace` / `adamWStepInPlace` / `lionStepInPlace`** (`optimizer.cp`)：零堆分配 AI 大模型训练优化器与学习率调度引擎，提供线性 Warmup + 余弦/线性退火、全局 L2 梯度范数裁剪、解耦权重衰减 AdamW（含偏差修正）与 Google Brain Lion 符号动量优化器。
- **`Int4QuantMode` / `Int4GroupParams` / `packInt4Pair` / `unpackInt4Lo` / `unpackInt4Hi` / `quantizeGroupInt4Sym` / `quantizeGroupInt4Asym` / `dequantizeGroupInt4` / `gemvInt4Group`** (`quant_int4.cp`)：零堆分配 GPTQ / AWQ 风格 Per-Group INT4 权重压缩与混合精度融合 GEMV 引擎，支持对称 `[-8, 7]` 与非对称 `[0, 15] + zero_point` 分组量化、4-bit nibble 紧致打包以及尺度外提（Scale Hoisting）零临时反量化矩阵乘加。
- **`KvQuantParams` / `quantizeTokenKvI8` / `dequantizeTokenKvI8` / `dotQueryQuantKeyI8` / `quantizedAttentionSingleQuery`** (`kv_quant.cp`)：零堆分配长上下文 KV-Cache 在线 INT8 逐 Token 对称量化压缩（4x 显存压缩，对标 vLLM/KIVI）、尺度外提 Query-Key 点积以及 O(1) 额外内存 Online Softmax 融合单 Query 注意力核。
- **`SwaConfig` / `RingKvCache` / `swaAttentionSingleQuery` / `ringKvAttentionSingleQuery`** (`sliding_window.cp`)：零堆分配滑动窗口注意力（SWA，对标 Mistral / Gemma 2）与 StreamingLLM Attention Sink 固定容量环形 KV-Cache，支持前 `sink_tokens` 永久锚定、窗口环形覆盖与 $O((S+W)\cdot d)$ 无界流式解码。
- **`PREFIX_INVALID_BLOCK` / `computePrefixBlockHash` / `PrefixBlockEntry` / `PrefixMatchResult` / `PrefixCacheStats` / `KvPrefixCache`** (`prefix_cache.cp`)：零堆分配多智能体共享提示词块级 KV 前缀缓存（对标 SGLang RadixAttention / vLLM Automatic Prefix Caching），支持链式 FNV-1a 64-bit 哈希防分叉误命中、最长前缀匹配、在途引用计数 `pinBlock`/`unpinBlock` 保护与未 pin 槽位 LRU 驱逐。
- **`gemmTransposedBF32` / `gemmF32` / `batchedGemmTransposedBF32` / `splitGemmRowsF32`** (`tiled_gemm.cp`)：零堆分配 Cache-Friendly 分块矩阵乘法 GEMM 与多头批量 QK^T MatMul 核，基于 CP 原生 8-lane SIMD `@dot`、`@fma` 与零拷贝互斥分片 `@splitAtMut` 实现。
- **`conv1dCausalF32` / `depthwiseConv1dF32` / `hzToMel` / `melToHz` / `buildMelFilterbankF32` / `applyMelFilterbankF32`** (`conv1d.cp`)：零堆分配 AI 语音/状态空间 1D 因果空洞卷积、深度可分离 1D 卷积、HTK 梅尔频率变换、三角梅尔滤波器组构建与 `@dot` SIMD 对数梅尔谱投影核（对标 Whisper / Mamba / ConvNeXt-1D）。

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
| `alibiSlopes` | `(n_heads, slopes_out): bool` | 生成 Press et al. 2022 ALiBi 几何斜率（支持 2 的幂与非 2 的幂头数） |
| `alibiAttentionSingleQuery` | `(q_heads, k_heads, v_heads, seq_kv, n_heads, head_dim, slopes, out_heads): bool` | O(1) 在线 Softmax 多头单查询 ALiBi 注意力，自动叠加 `-m_h * ((seq_kv - 1) - j)` 偏置 |
| `mlaCompressKv` | `(x, w_dkv, d_model, d_latent, latent_out): bool` | 将隐藏层向量压缩为 `d_latent` 维 KV 潜向量存入缓存 |
| `mlaDecodeSingleHead` | `(q_nope, q_rope, c_kv_cache, k_rope_cache, w_uk, w_uv, seq_kv, d_latent, d_nope, d_rope, d_v, scale, q_absorbed_s, out_latent_s, out_v): bool` | DeepSeek-V2/V3 MLA 矩阵吸收解码核（`q_abs = q_nope^T * W_UK` 直接点积潜缓存 + 解耦 RoPE + 单次 `W_UV` 上投影） |
| `embeddingLookup` | `(table, vocab_size, dim, token_ids, out_seq): bool` | Token ID 词表嵌入查表（原子校验每个 `token_id < vocab_size`） |
| `poolHiddenStates` | `(hidden_seq, attention_mask, seq_len, dim, kind, out_vec): bool` | 带 `attention_mask` 掩码的 `Mean`/`Cls`/`LastToken`/`Max` 序列池化（全 0 mask 返回 `false`） |
| `l2NormalizeInPlace` | `(vec, eps): f64?` | 原地 L2 单位化并返回归一化前原始范数（零向量返回 `null`） |
| `matryoshkaProject` | `(full_vec, target_dim, out_vec): bool` | Matryoshka (MRL) 套娃维度截断并重新 L2 归一化为单位向量 |
| `cosineSimilarityMatrix` | `(queries, n_q, docs, n_d, dim, out_mat): bool` | 批量计算 `[n_q, n_d]` 查询-文档余弦相似度矩阵 |
| `logSumExp` / `crossEntropyLoss` | `(logits): f64?` / `(logits, target_idx, label_smoothing): f64?` | 数值稳定 LogSumExp 与支持 `label_smoothing ∈ [0, 1)` 的 Cross-Entropy 损失 |
| `sequencePerplexity` | `(logits_flat, targets, seq_len, vocab_size, ignore_index): f64?` | 跳过 `ignore_index`（如 `-100`）计算序列困惑度 $\exp(\text{mean CE})$ |
| `klDivergenceFromLogits` | `(p_logits, q_logits): f64?` | 直接从 logits 计算非负 $D_{\text{KL}}(P \| Q)$（无需分配概率缓冲） |
| `dpoLoss` | `(pi_chosen, pi_rejected, ref_chosen, ref_rejected, beta): DpoResult?` | Direct Preference Optimization 对齐损失与隐式奖励/准确率诊断（极限大负 margin 防溢出） |
| `infoNceLoss` | `(similarities, positive_idx, temperature): f64?` | 对比学习 InfoNCE 损失（Embedding 微调与检索重排） |
| `cosineWarmupLr` / `linearWarmupDecayLr` | `(step, cfg): f64?` | 线性 Warmup + 余弦退火 / 线性衰减至 `min_lr` 学习率调度器 |
| `gradGlobalL2Norm` / `clipGradNormInPlace` | `(grads): f64?` / `(grads, max_norm): f64?` | 全局 L2 梯度范数与原位范数裁剪（返回裁剪前范数） |
| `adamWStepInPlace` | `(params, grads, m, v, step, lr, cfg): bool` | 解耦权重衰减 AdamW 单步更新（1-based `step` 偏差修正 `1 - beta^t`） |
| `lionStepInPlace` | `(params, grads, m, lr, cfg): bool` | Google Brain Lion 符号动量单步更新（仅需单份动量缓冲，显存减半） |
| `packInt4Pair` / `unpackInt4Lo` / `unpackInt4Hi` | `(lo: u8, hi: u8): u8` / `(byte: u8): u8` | 4-bit Nibble 打包/解包原语（低 4 位为偶索引，高 4 位为奇索引） |
| `quantizeGroupInt4Sym` / `quantizeGroupInt4Asym` | `(src, group_size, out_packed, out_params): bool` | Per-Group INT4 对称（`[-8, 7]`）与非对称（`[0, 15] + zero_point`）分组量化（要求偶数 `group_size >= 2`） |
| `dequantizeGroupInt4` | `(packed, params, mode, out_dst): bool` | 按分组参数反量化还原为 `f32` 切片 |
| `gemvInt4Group` | `(x, packed_weights, group_params, rows, cols, group_size, mode, out_y): bool` | 零临时反量化矩阵的 Per-Group INT4 尺度外提融合 GEMV（`y = W * x`，内部 `f64` 累加） |
| `quantizeTokenKvI8` / `dequantizeTokenKvI8` | `(vec, out_q): KvQuantParams?` / `(q, params, out_vec): bool` | 逐 Token 对称 INT8（`[-127, 127]`）在线 KV 量化与反量化（4x 显存压缩，记录 `min_val`/`max_val`） |
| `dotQueryQuantKeyI8` | `(q_vec, k_q, k_params): f64?` | 零反量化临时缓冲的尺度外提 Query-Key INT8 点积（内部 `f64` 累加） |
| `quantizedAttentionSingleQuery` | `(query, keys_q, k_params, vals_q, v_params, seq_len, head_dim, out): bool` | 基于 Online Softmax 的 O(1) 额外内存 INT8 KV-Cache 单 Query 融合注意力核 |
| `RingKvCache.init` / `appendToken` / `activeLen` / `totalSeen` / `clear` | `(cfg, keys_buf, vals_buf): RingKvCache?` / `(*_, k, v): bool` | 固定容量 `(sink_tokens + window_size) * head_dim` StreamingLLM 环形 KV-Cache，前 `sink_tokens` 永久锚定不被覆盖 |
| `swaAttentionSingleQuery` | `(query, keys, vals, seq_len, query_pos, cfg, out): bool` | $O((S+W)\cdot d)$ 滑动窗口 + Attention Sink 单 Query Online Softmax 融合注意力 |
| `ringKvAttentionSingleQuery` | `(query, cache, out): bool` | 直接在 `RingKvCache` 上按时间顺序执行无界流式 StreamingLLM 单 Query 注意力 |
| `computePrefixBlockHash` | `(parent_hash: u64, block_tokens: []const u32): u64` | 链式 FNV-1a 64-bit 前缀块哈希（保证完整前缀一致才命中，根 `parent_hash = 0`） |
| `KvPrefixCache.init` / `matchLongestPrefix` / `insertPrefixBlocks` | `(entries, block_size)` / `(*_, tokens, tick, out_block_ids): PrefixMatchResult` / `(*_, tokens, block_ids, tick): usize` | 零堆分配固定槽位块级 KV 前缀缓存、最长前缀匹配与链式块写入 |
| `KvPrefixCache.pinBlock` / `unpinBlock` / `evictLruUnpinned` / `stats` | `(*_, prefix_hash): bool` / `(*_): u32?` / `(): PrefixCacheStats` | 在途请求引用计数保护、仅驱逐 `ref_count == 0` 的最旧块与命中/节省 Token 统计 |
| `gemmTransposedBF32` | `(a, b_t, m, n, k, alpha, beta, c): bool` | 2D 分块 + `@dot` 8-lane SIMD + `@fma` 计算 `C = alpha * (A * B^T) + beta * C` |
| `gemmF32` | `(a, b, m, n, k, alpha, beta, c): bool` | 3D 分块 `ikj` 缓存友好遍历 + `@fma` 计算 `C = alpha * (A * B) + beta * C` |
| `batchedGemmTransposedBF32` | `(a, b_t, batch, m, n, k, c): bool` | 多头注意力 `QK^T` 批量矩阵乘法（`[batch, m, k] * [batch, n, k]^T`） |
| `splitGemmRowsF32` | `(a, b_t, m, n, k, split_row, c): bool` | 基于 `@splitAtMut` 将输出矩阵零拷贝拆分为上下互斥分区并分别计算 `A * B^T` |
| `conv1dCausalF32` | `(input, weight, bias, in_ch, out_ch, seq_len, kernel_size, dilation, out): bool` | 左因果填充 `(kernel_size - 1) * dilation` 的 1D 因果空洞卷积（`@dot`/`@fma`，零未来帧泄露） |
| `depthwiseConv1dF32` | `(input, weight, bias, channels, seq_len, kernel_size, stride, padding, out): bool` | 逐通道深度可分离 1D 卷积（支持任意 `stride`/`padding` 与可选偏置） |
| `hzToMel` / `melToHz` | `(hz: f64): f64` / `(mel: f64): f64` | HTK 梅尔刻度与赫兹频率精确互转 |
| `buildMelFilterbankF32` / `applyMelFilterbankF32` | `(n_mels, n_fft, sr, f_min, f_max, out_fb)` / `(power_spec, fb, n_mels, n_bins, log_floor, out_log_mel)` | 零堆分配三角梅尔滤波器组构建与 8-lane SIMD `@dot` 对数梅尔谱投影 |

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

### DeepSeek-V2/V3 MLA 矩阵吸收与 ALiBi 线性偏置示例 (`mla.cp`)

```cp
import {
    alibiSlopes,
    mlaCompressKv,
    mlaDecodeSingleHead,
} from "cap:attention";

pub fn demoMlaWeightAbsorption(
    q_nope: []const f64,
    q_rope: []const f64,
    c_kv_cache: []const f64,
    k_rope_cache: []const f64,
    w_uk: []const f64,
    w_uv: []const f64,
) {
    // 1. 生成 8 头 ALiBi 斜率 [0.5, 0.25, ..., 1/256]
    let slopes_buf: [8]f64 = [0.0; 8];
    let slopes: []f64 = slopes_buf[0..];
    * = alibiSlopes(8, slopes);

    // 2. MLA 矩阵吸收单头解码（d_latent=4, d_nope=6, d_rope=2, d_v=6, seq_kv=8）
    let q_abs_buf: [4]f64 = [0.0; 4];
    let out_lat_buf: [4]f64 = [0.0; 4];
    let out_v_buf: [6]f64 = [0.0; 6];
    let q_abs: []f64 = q_abs_buf[0..];
    let out_lat: []f64 = out_lat_buf[0..];
    let out_v: []f64 = out_v_buf[0..];
    * = mlaDecodeSingleHead(
        q_nope,
        q_rope,
        c_kv_cache,
        k_rope_cache,
        w_uk,
        w_uv,
        8,
        4,
        6,
        2,
        6,
        0.0,
        q_abs,
        out_lat,
        out_v,
    );
}
```

### 词表嵌入查表 -> 掩码池化 -> Matryoshka 套娃截断示例 (`embedding.cp`)

```cp
import {
    PoolKind,
    embeddingLookup,
    poolHiddenStates,
    matryoshkaProject,
} from "cap:attention";

pub fn demoEmbeddingPipeline(vocab_table: []const f64) {
    // 1. 查表：[token 1, token 3, PAD=0] (vocab_size=8, dim=4)
    const token_ids: [3]u32 = [1, 3, 0];
    let seq_buf: [12]f64 = [0.0; 12];
    let seq: []f64 = seq_buf[0..];
    * = embeddingLookup(vocab_table, 8, 4, token_ids[0..], seq);

    // 2. 带掩码的 LastToken 池化（跳过末尾 PAD）
    const mask: [3]u8 = [1, 1, 0];
    let pooled_buf: [4]f64 = [0.0; 4];
    let pooled: []f64 = pooled_buf[0..];
    * = poolHiddenStates(seq_buf[0..], mask[0..], 3, 4, PoolKind.LastToken, pooled);

    // 3. Matryoshka (MRL) 截断至前 2 维并重新 L2 单位化
    let mrl_buf: [2]f64 = [0.0; 2];
    let mrl: []f64 = mrl_buf[0..];
    * = matryoshkaProject(pooled_buf[0..], 2, mrl);
}
```

### 大模型对齐与评测损失（DPO / InfoNCE / Perplexity）示例 (`loss.cp`)

```cp
import {
    sequencePerplexity,
    dpoLoss,
    infoNceLoss,
} from "cap:attention";

pub fn demoAlignmentAndEvalLosses(logits_flat: []const f64, targets: []const i32) {
    // 1. 序列困惑度（忽略 prompt/PAD 位 -100）
    if (ppl := sequencePerplexity(logits_flat, targets, 4, 8, -100)) {
        * = ppl;
    }

    // 2. DPO 偏好对齐损失（beta = 0.1）
    if (dpo := dpoLoss(-12.5, -18.0, -14.0, -16.0, 0.1)) {
        * = dpo.loss;
        * = dpo.reward_margin;
        * = dpo.accuracy;
    }

    // 3. InfoNCE 对比学习损失（positive_idx = 0, tau = 0.05）
    const sims: [4]f64 = [0.92, 0.41, 0.15, -0.10];
    if (nce := infoNceLoss(sims[0..], 0, 0.05)) {
        * = nce;
    }
}
```

### 训练优化器（AdamW / Lion / Cosine Warmup / 梯度裁剪）示例 (`optimizer.cp`)

```cp
import {
    LrScheduleConfig,
    AdamWConfig,
    cosineWarmupLr,
    clipGradNormInPlace,
    adamWStepInPlace,
} from "cap:attention";

pub fn demoAdamWTrainingStep(
    params: *[]f64,
    grads: *[]f64,
    m: *[]f64,
    v: *[]f64,
    step: usize,
) {
    const sched = LrScheduleConfig{
        peak_lr: 0.0003,
        min_lr: 0.00003,
        warmup_steps: 100,
        total_steps: 10000,
    };
    if (lr := cosineWarmupLr(step, sched)) {
        * = clipGradNormInPlace(grads, 1.0);
        * = adamWStepInPlace(params, grads, m, v, step, lr, AdamWConfig.default());
    }
}
```

### Per-Group INT4 分组量化与融合 GEMV 示例 (`quant_int4.cp`)

```cp
import {
    Int4QuantMode,
    Int4GroupParams,
    quantizeGroupInt4Asym,
    gemvInt4Group,
} from "cap:attention";

pub fn demoPerGroupInt4Gemv() {
    // 2x8 权重矩阵按 group_size=4 分组非对称 INT4 量化（仅占 8 字节 + 4 组参数）
    const weights: [16]f32 = [
        -1.0, 2.0, 5.0, 14.0,   0.0, 0.5, 1.0, 1.5,
         3.0, 1.0, 0.0, -2.0,  -4.0, -2.0, 0.0, 2.0,
    ];
    const x: [8]f32 = [1.0, 0.5, -0.5, 0.25, 2.0, -1.0, 0.5, 1.0];

    let packed_buf: [8]u8 = undefined;
    let params: [4]Int4GroupParams = undefined;
    let y: [2]f32 = undefined;

    let packed_s: []u8 = packed_buf[0..];
    let params_s: []Int4GroupParams = params[0..];
    let y_s: []f32 = y[0..];

    if (quantizeGroupInt4Asym(weights[0..], 4, packed_s, params_s)) {
        * = gemvInt4Group(x[0..], packed_buf[0..], params[0..], 2, 8, 4, .Asymmetric, y_s);
    }
}
```

### 在线 INT8 KV-Cache 量化与融合单 Query 注意力示例 (`kv_quant.cp`)

```cp
import {
    KvQuantParams,
    quantizeTokenKvI8,
    quantizedAttentionSingleQuery,
} from "cap:attention";

pub fn demoQuantizedKvAttention() {
    const query: [4]f32 = [0.8, -0.4, 0.5, 0.2];
    const k0: [4]f32 = [0.9, -0.3, 0.4, 0.1];
    const v0: [4]f32 = [1.0, 2.0, -1.0, 0.5];
    const k1: [4]f32 = [-0.5, 0.7, -0.2, 0.6];
    const v1: [4]f32 = [-0.5, 0.25, 1.5, -1.0];

    let keys_q: [8]i8 = undefined;
    let vals_q: [8]i8 = undefined;
    let k_params: [2]KvQuantParams = undefined;
    let v_params: [2]KvQuantParams = undefined;
    let out: [4]f32 = undefined;

    k_params[0] = quantizeTokenKvI8(k0[0..], keys_q[0..4]) ?? KvQuantParams{};
    v_params[0] = quantizeTokenKvI8(v0[0..], vals_q[0..4]) ?? KvQuantParams{};
    k_params[1] = quantizeTokenKvI8(k1[0..], keys_q[4..8]) ?? KvQuantParams{};
    v_params[1] = quantizeTokenKvI8(v1[0..], vals_q[4..8]) ?? KvQuantParams{};

    let out_s: []f32 = out[0..];
    * = quantizedAttentionSingleQuery(
        query[0..],
        keys_q[0..],
        k_params[0..],
        vals_q[0..],
        v_params[0..],
        2,
        4,
        out_s,
    );
}
```

### 滑动窗口注意力与 StreamingLLM Attention Sink 环形缓存示例 (`sliding_window.cp`)

```cp
import {
    SwaConfig,
    RingKvCache,
    ringKvAttentionSingleQuery,
} from "cap:attention";

pub fn demoStreamingLlmRingKv() {
    const cfg = SwaConfig{
        sink_tokens: 2,
        window_size: 4,
        head_dim: 4,
        causal: true,
    };
    let k_buf: [24]f32 = undefined; // (2 + 4) * 4
    let v_buf: [24]f32 = undefined;
    let k_s: []f32 = k_buf[0..];
    let v_s: []f32 = v_buf[0..];

    if (mut_cache := RingKvCache.init(cfg, k_s, v_s)) {
        let cache = mut_cache;
        const tok_k: [4]f32 = [0.5, -0.2, 0.8, 0.1];
        const tok_v: [4]f32 = [1.0, 0.5, -0.5, 0.25];
        * = cache.appendToken(tok_k[0..], tok_v[0..]);

        let out: [4]f32 = undefined;
        let out_s: []f32 = out[0..];
        * = ringKvAttentionSingleQuery(tok_k[0..], cache, out_s);
    }
}
```

### 块级 KV 前缀缓存与多 Agent 共享 System Prompt 示例 (`prefix_cache.cp`)

```cp
import {
    PrefixBlockEntry,
    KvPrefixCache,
} from "cap:attention";

pub fn demoKvPrefixCache() {
    let entries: [8]PrefixBlockEntry = undefined;
    let entry_s: []PrefixBlockEntry = entries[0..];

    if (init_cache := KvPrefixCache.init(entry_s, 4)) {
        let cache = init_cache;
        const sys_and_user_a: [8]u32 = [10, 20, 30, 40, 101, 102, 103, 104];
        const block_ids_a: [2]u32 = [100, 101];
        * = cache.insertPrefixBlocks(sys_and_user_a[0..], block_ids_a[0..], 1);

        // Agent B 共享相同前 4 个 System Prompt token，第 2 块分叉
        const sys_and_user_b: [8]u32 = [10, 20, 30, 40, 201, 202, 203, 204];
        let matched_ids: [2]u32 = undefined;
        let matched_s: []u32 = matched_ids[0..];
        const res = cache.matchLongestPrefix(sys_and_user_b[0..], 2, matched_s);
        // res.matched_blocks == 1, res.matched_tokens == 4, matched_ids[0] == 100
        * = res;
    }
}
```

### 分块 SIMD GEMM、多头 Batched QK^T 与 `@splitAtMut` 分片示例 (`tiled_gemm.cp`)

```cp
import {
    gemmTransposedBF32,
    batchedGemmTransposedBF32,
    splitGemmRowsF32,
} from "cap:attention";

pub fn demoTiledGemm() {
    const a: [8]f32 = [1.0, 2.0, 3.0, 4.0,  5.0, 6.0, 7.0, 8.0]; // 2x4
    const b_t: [8]f32 = [1.0, 0.0, 1.0, 0.0,  0.0, 1.0, 0.0, 1.0]; // 2x4 (B^T)
    let c: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    let c_s: []f32 = c[0..];

    * = gemmTransposedBF32(a[0..], b_t[0..], 2, 2, 4, 1.0, 0.0, c_s);
    * = splitGemmRowsF32(a[0..], b_t[0..], 2, 2, 4, 1, c_s);
    * = batchedGemmTransposedBF32(a[0..], b_t[0..], 1, 2, 2, 4, c_s);
}
```

### 1D 因果卷积与 Whisper 梅尔滤波器组投影示例 (`conv1d.cp`)

```cp
import {
    conv1dCausalF32,
    buildMelFilterbankF32,
    applyMelFilterbankF32,
} from "cap:attention";

pub fn demoAudioMelAndCausalConv() {
    // 1. 构建 4-Mel, n_fft=16 (n_bins=9) 滤波器组并投影功率谱
    let fb: [36]f32 = undefined;
    let fb_s: []f32 = fb[0..];
    if (buildMelFilterbankF32(4, 16, 16000.0, 0.0, 8000.0, fb_s)) {
        const power: [9]f32 = [0.0, 1.0, 2.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.1];
        let log_mel: [4]f32 = undefined;
        let log_mel_s: []f32 = log_mel[0..];
        * = applyMelFilterbankF32(power[0..], fb[0..], 4, 9, 0.000001, log_mel_s);
    }

    // 2. 1D 因果卷积（kernel_size=3, dilation=1, 无偏置）
    const x: [5]f32 = [1.0, 2.0, 3.0, 4.0, 5.0];
    const w: [3]f32 = [0.2, 0.3, 0.5];
    const no_bias: []const f32 = x[0..0];
    let y: [5]f32 = undefined;
    let y_s: []f32 = y[0..];
    * = conv1dCausalF32(x[0..], w[0..], no_bias, 1, 1, 5, 3, 1, y_s);
}
```
