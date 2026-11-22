# `cap:attention` — Transformer Attention, KV-Cache & Token Sampler

## 1. 概述

`cap:attention`（`cap.attention`）是 CP 语言纯原生实现的工业级大语言模型推理核心算子库，提供数值稳定的 Scaled Dot-Product Attention (MHA)、Grouped-Query Attention (GQA)、支持 Speculative Decoding 回滚的自回归 `KvCache`，以及支持 Temperature、Repetition Penalty、Top-K 与 Top-P (Nucleus) 的确定性 Token 采样器。

## 2. 核心类型

- **`AttnError`**: 统一错误集合（`InvalidDimension`, `HeadMismatch`, `InvalidStartPos`, `EmptyInput`, `CacheFull`, `InvalidLayer`, `InvalidTemperature`, `InvalidTopK`, `InvalidTopP`, `OutOfMemory`）。
- **`PagedKvSlot`**: 单步追加返回的槽位元信息（`layer`, `seq_offset`, `num_tokens`）。
- **`KvCache`**: 多层自回归 KV 缓存容器，按 `[layer][seq_pos][kv_head][head_dim]` 布局存储，支持 `appendStep`、`truncateTo` 回滚与 `reset`。
- **`KV_INVALID_BLOCK` / `KvBlockMeta` / `KvAllocStats` / `PagedKvAllocator` / `BlockTable`** (`kvCache.cp`)：零堆分配 PagedAttention 物理块管理器，支持系统提示词前缀哈希零拷贝共享、释放后缓存复用与 LRU 驱逐、Beam Search `retainBlock` + 引用计数 CoW（共享态分配新块，独占态零拷贝原地返回）以及 `BlockTable` 逻辑到物理槽位映射。
- **`SamplingConfig`**: 采样配置（`temperature`, `top_k`, `top_p`, `repetition_penalty`, `seed`）。
- **`RopeConfig` / RoPE / RMSNorm / LayerNorm / SwiGLU / GeLU** (`rope.cp`)：零堆分配 Transformer 基础推理核，支持 NTK-Aware 长上下文缩放的 RoPE 旋转位置编码、RMSNorm/LayerNorm 归一化与 SiLU/GeLU/SwiGLU 激活函数。
- **`QUANT_BLOCK_SIZE` / `BlockQ8_0` / `BlockQ4_0` / GEMV** (`quantMatmul.cp`)：零堆分配 GGML 风格 32 元素分块量化与整型累加矩阵向量乘法（`W_q8 * x_q8` 与经典 llama.cpp `W_q4 * x_q8` 混合量化推理核）。
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
- **`Int4QuantMode` / `Int4GroupParams` / `packInt4Pair` / `unpackInt4Lo` / `unpackInt4Hi` / `quantizeGroupInt4Sym` / `quantizeGroupInt4Asym` / `dequantizeGroupInt4` / `gemvInt4Group`** (`quantInt4.cp`)：零堆分配 GPTQ / AWQ 风格 Per-Group INT4 权重压缩与混合精度融合 GEMV 引擎，支持对称 `[-8, 7]` 与非对称 `[0, 15] + zero_point` 分组量化、4-bit nibble 紧致打包以及尺度外提（Scale Hoisting）零临时反量化矩阵乘加。
- **`KvQuantParams` / `quantizeTokenKvI8` / `dequantizeTokenKvI8` / `dotQueryQuantKeyI8` / `quantizedAttentionSingleQuery`** (`kvQuant.cp`)：零堆分配长上下文 KV-Cache 在线 INT8 逐 Token 对称量化压缩（4x 显存压缩，对标 vLLM/KIVI）、尺度外提 Query-Key 点积以及 O(1) 额外内存 Online Softmax 融合单 Query 注意力核。
- **`SwaConfig` / `RingKvCache` / `swaAttentionSingleQuery` / `ringKvAttentionSingleQuery`** (`slidingWin.cp`)：零堆分配滑动窗口注意力（SWA，对标 Mistral / Gemma 2）与 StreamingLLM Attention Sink 固定容量环形 KV-Cache，支持前 `sink_tokens` 永久锚定、窗口环形覆盖与 $O((S+W)\cdot d)$ 无界流式解码。
- **`PREFIX_INVALID_BLOCK` / `computePrefixBlockHash` / `PrefixBlockEntry` / `PrefixMatchResult` / `PrefixCacheStats` / `KvPrefixCache`** (`prefixCache.cp`)：零堆分配多智能体共享提示词块级 KV 前缀缓存（对标 SGLang RadixAttention / vLLM Automatic Prefix Caching），支持链式 FNV-1a 64-bit 哈希防分叉误命中、最长前缀匹配、在途引用计数 `pinBlock`/`unpinBlock` 保护与未 pin 槽位 LRU 驱逐。
- **`gemmTransposedBF32` / `gemmF32` / `batchedGemmTransposedBF32` / `splitGemmRowsF32`** (`tiledGemm.cp`)：零堆分配 Cache-Friendly 分块矩阵乘法 GEMM 与多头批量 QK^T MatMul 核，基于 CP 原生 8-lane SIMD `@dot`、`@fma` 与零拷贝互斥分片 `@splitAtMut` 实现。
- **`conv1dCausalF32` / `depthwiseConv1dF32` / `hzToMel` / `melToHz` / `buildMelFilterbankF32` / `applyMelFilterbankF32`** (`conv1d.cp`)：零堆分配 AI 语音/状态空间 1D 因果空洞卷积、深度可分离 1D 卷积、HTK 梅尔频率变换、三角梅尔滤波器组构建与 `@dot` SIMD 对数梅尔谱投影核（对标 Whisper / Mamba / ConvNeXt-1D）。
- **`ImageNormParams` / `normalizeRgbU8ToF32` / `patchifyHwcF32` / `patchEmbedLinearF32` / `build2dSinCosPosEmbF32` / `applyRope2dInPlaceF32`** (`vision.cp`)：零堆分配多模态视觉大模型（ViT / CLIP / Qwen2-VL / Flux）前端核，覆盖 `@fma` RGB 归一化、HWC Patchify、`@dot` 线性补丁投影、MAE/ViT 2D 正弦位置编码与严格保范数的 2D 空间 RoPE。
- **`FP8_E4M3_MAX_FINITE` / `Fp8BlockScale` / `computeBlockAmaxF32` / `quantizeBlockFp8E4m3` / `dequantizeBlockFp8E4m3` / `gemvBlockFp8E4m3`** (`quantFp8.cp`)：零堆分配 DeepSeek-V3 / H100 风格块级动态缩放 FP8 E4M3FN 量化与尺度外提融合混合精度 GEMV 核（支持任意非整除尾部块与全零块零除防护）。
- **`VocabBitset` / `DfaEdge` / `ConstrainedDfa` / `applyVocabMaskInPlace` / `applyTemperatureAndTopKMaskInPlace`** (`constrain.cp`)：零堆分配 LLM 结构化受限解码 Logits 位图掩码与 DFA 状态引导词表过滤器（对标 xgrammar / Outlines，支持硬件 `@popCount`、尾位自动清零、`-inf` 掩码与零堆分配温度 + Top-K 过滤）。
- **`RopeScalingKind` / `YarnConfig` / `yarnAttentionMscale` / `computeRopeInvFreqs` / `applyScaledRopeInPlaceF32`** (`ropeYarn.cp`)：零堆分配长上下文大模型 RoPE 扩展引擎，统一支持标准 RoPE、Linear PI、NTK-Aware 与 YaRN（Peng et al. 2023）波长频段斜坡插值以及 `mscale` 注意力温度缩放。
- **`SPEC_TREE_NO_PARENT` / `DraftTreeNode` / `TreeVerifyResult` / `validateDraftTree` / `buildTreeAttentionMask` / `verifyDraftTreeGreedy`** (`specTree.cp`)：零堆分配 Medusa / Eagle 树状推测解码注意力拓扑掩码构造与贪心最长接受路径验证器，支持单次前向并行验证整棵多分支草稿树并提取奖励 Token。
- **`IterSchedulerConfig` / `SeqRunState` / `SeqSchedEntry` / `ScheduledStepItem` / `IterStepPlan` / `IterSchedulerStats` / `ContinuousIterScheduler`** (`iterSched.cp`)：零堆分配 vLLM / Sarathi-Serve 风格 Chunked-Prefill + Decode 连续批处理迭代调度器，优先保障在途解码低延迟（TPOT）、KV 块紧张时按低优先级/最新入队抢占释放防止死锁，并利用剩余 Token 预算对等待/预填充请求执行分块预填充。

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
| `ImageNormParams.clipDefault` / `imagenetDefault` / `normalizeRgbU8ToF32` | `(rgb_u8, params, out_f32): bool` | CLIP / ImageNet RGB 均值方差预设与单 `@fma` 硬件融合归一化 |
| `patchifyHwcF32` / `patchEmbedLinearF32` | `(img_hwc, h, w, c, ps, out): usize?` / `(patches, w, b, np, pdim, edim, out): bool` | 零堆分配 HWC 图像分块切分与 `@dot` 8-lane SIMD 线性补丁嵌入投影 |
| `build2dSinCosPosEmbF32` | `(grid_h, grid_w, embed_dim, out_pos): bool` | MAE / ViT 2D 正弦余弦位置编码（前一半编 `y`、后一半编 `x`，要求 `embed_dim % 4 == 0`） |
| `applyRope2dInPlaceF32` | `(vec: *[]f32, row, col, theta_base): bool` | Qwen2-VL / Flux 原位 2D 空间旋转位置编码（严格保 L2 范数不变） |
| `computeBlockAmaxF32` / `quantizeBlockFp8E4m3` / `dequantizeBlockFp8E4m3` | `(src, block_size, out_q, out_scales): bool` | DeepSeek-V3 / H100 风格块级动态缩放（`amax / 448.0`）FP8 E4M3FN 量化与反量化（支持非整除尾块与全零块防护） |
| `gemvBlockFp8E4m3` | `(x, w_q, w_scales, rows, cols, block_size, out_y): bool` | 零临时反量化大矩阵的块级 FP8 尺度外提融合 GEMV（块内 `@fma` 累加、块外乘加 `w_scale`） |
| `VocabBitset` | `init` / `allowAll` / `clearAll` / `allowToken` / `forbidToken` / `isAllowed` / `allowedCount` / `intersectWith` | 零堆分配词表位图掩码（自动清零尾位、硬件 `@popCount`） |
| `applyVocabMaskInPlace` | `(logits: *[]f32, mask_words: []const u64): usize?` | 将未置位词表 token 置 `-inf`，返回保留合法 token 数（全禁止返回 `0` 供上层回退） |
| `ConstrainedDfa` | `init` / `buildAllowedMask` / `step` / `isAccepting` | 零堆分配固定转移表 DFA 状态引导词表掩码生成器与终态判定 |
| `applyTemperatureAndTopKMaskInPlace` | `(logits: *[]f32, temperature: f32, top_k: usize): usize?` | 温度缩放 + 零堆分配确定性 Top-K Logits 掩码（其余置 `-inf`） |
| `yarnAttentionMscale` | `(scale_factor: f64, mscale_coeff: f64): f64` | YaRN 注意力温度缩放系数 `0.1 * mscale_coeff * ln(scale_factor) + 1.0` |
| `computeRopeInvFreqs` | `(kind: RopeScalingKind, cfg: YarnConfig, out_inv_freqs: *[]f64): bool` | 计算 `None` / `Linear` / `NtkAware` / `Yarn`（高频保留、低频线性插值、中频斜坡混合）逆频率表 |
| `applyScaledRopeInPlaceF32` | `(vec: *[]f32, pos: usize, inv_freqs: []const f64, mscale: f32): bool` | 支持 `mscale` 的原位半切分 RoPE 旋转（严格保范数 `mscale * ||vec||`） |
| `validateDraftTree` | `(nodes: []const DraftTreeNode): bool` | 校验 Medusa/Eagle 草稿树单根、严格拓扑序（`parent < i`）、深度一致与无环 |
| `buildTreeAttentionMask` | `(nodes, out_mask_u8: *[]u8): bool` | 构造 `N×N` 树状因果注意力掩码（仅祖先与自身可见、兄弟分支互不可见） |
| `verifyDraftTreeGreedy` | `(nodes, target_argmax, out_accepted_tokens: *[]u32): TreeVerifyResult?` | 沿草稿树贪心匹配最长连续接受路径并返回末节点奖励 Token |
| `ContinuousIterScheduler.init` | `(entries_buf: *[]SeqSchedEntry, config: IterSchedulerConfig): ContinuousIterScheduler?` | 零堆分配初始化连续批处理迭代调度器（校验所有容量与块大小 > 0） |
| `ContinuousIterScheduler.enqueue` | `(*_, req_id: u64, prompt_tokens: usize, max_new_tokens: usize, priority: i32): bool` | 入队新请求（优先使用空槽，满时自动复用最旧 `Finished` 槽位；拒绝重复活跃 ID） |
| `ContinuousIterScheduler.scheduleStep` | `(*_, out_items: *[]ScheduledStepItem): IterStepPlan` | 单步迭代调度：Decode 优先 + 低优/最新入队 KV 抢占防死锁 + 剩余预算 Chunked Prefill |
| `ContinuousIterScheduler.commitStep` | `(*_, req_id: u64, generated_tokens: usize, hit_eos: bool): bool` | 提交单步执行结果，推进预填充/解码进度，命中 EOS 或达上限自动释放 KV 块 |
| `ContinuousIterScheduler.cancelRequest` / `getEntry` / `stats` | `(*_, req_id): bool` / `(req_id): SeqSchedEntry?` / `(): IterSchedulerStats` | 主动取消并回收 KV 块、按 ID 查询序列状态与全量遥测统计 |

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

### PagedAttention 前缀共享、CoW 与逻辑块表映射示例 (`kvCache.cp`)

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

### GGML 风格 Q8_0 / Q4_0 分块量化与混合推理 GEMV 示例 (`quantMatmul.cp`)

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

### Per-Group INT4 分组量化与融合 GEMV 示例 (`quantInt4.cp`)

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

### 在线 INT8 KV-Cache 量化与融合单 Query 注意力示例 (`kvQuant.cp`)

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

### 滑动窗口注意力与 StreamingLLM Attention Sink 环形缓存示例 (`slidingWin.cp`)

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

### 块级 KV 前缀缓存与多 Agent 共享 System Prompt 示例 (`prefixCache.cp`)

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

### 分块 SIMD GEMM、多头 Batched QK^T 与 `@splitAtMut` 分片示例 (`tiledGemm.cp`)

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

### 多模态 ViT 图像分块、2D 位置编码与 2D-RoPE 示例 (`vision.cp`)

```cp
import {
    ImageNormParams,
    normalizeRgbU8ToF32,
    patchifyHwcF32,
    build2dSinCosPosEmbF32,
    applyRope2dInPlaceF32,
} from "cap:attention";

pub fn demoVisionPatchFrontend() {
    // 4x4x3 RGB 图像按 patch_size=2 切分为 4 个 12D 补丁
    let rgb: [48]u8 = [128; 48];
    let img_f32: [48]f32 = undefined;
    let patches: [48]f32 = undefined;
    let pos_emb: [32]f32 = undefined; // 4 patches * embed_dim=8

    let img_s: []f32 = img_f32[0..];
    let patch_s: []f32 = patches[0..];
    let pos_s: []f32 = pos_emb[0..];

    if (normalizeRgbU8ToF32(rgb[0..], ImageNormParams.clipDefault(), img_s)) {
        if (n_patches := patchifyHwcF32(img_f32[0..], 4, 4, 3, 2, patch_s)) {
            * = n_patches;
            * = build2dSinCosPosEmbF32(2, 2, 8, pos_s);
        }
    }

    let q_tok: [8]f32 = [1.0, 0.5, -0.5, 0.25, 0.8, -0.4, 0.2, 0.6];
    let q_s: []f32 = q_tok[0..];
    * = applyRope2dInPlaceF32(q_s, 1, 2, 10000.0);
}
```

### DeepSeek-V3 / H100 块级 FP8 E4M3 量化与融合 GEMV 示例 (`quantFp8.cp`)

```cp
import {
    quantizeBlockFp8E4m3,
    gemvBlockFp8E4m3,
} from "cap:attention";

pub fn demoBlockFp8Gemv(w_row0: []const f32, x: []const f32) {
    let w_q: [64]u8 = [0; 64];
    let w_scales: [2]f32 = [0.0; 2]; // cols=64, block_size=32 -> 2 blocks
    let w_q_s: []u8 = w_q[0..];
    let w_s: []f32 = w_scales[0..];
    * = quantizeBlockFp8E4m3(w_row0, 32, w_q_s, w_s);

    let y: [1]f32 = [0.0];
    let y_s: []f32 = y[0..];
    * = gemvBlockFp8E4m3(x, w_q[0..], w_scales[0..], 1, 64, 32, y_s);
}
```

### 结构化受限解码 Logits 掩码与 DFA 状态引导示例 (`constrain.cp`)

```cp
import {
    DfaEdge,
    ConstrainedDfa,
    applyVocabMaskInPlace,
    applyTemperatureAndTopKMaskInPlace,
} from "cap:attention";

pub fn demoConstrainedDfaDecode(logits_in: *[]f32) {
    const edges: [2]DfaEdge = [
        DfaEdge{ from_state: 0, to_state: 1, token_id: 2 },
        DfaEdge{ from_state: 1, to_state: 2, token_id: 5 },
    ];
    const accept: [1]u64 = [4]; // bit 2 -> state 2 accepting
    if (dfa := ConstrainedDfa.init(edges[0..], accept[0..], 3, 16)) {
        let mask_buf: [1]u64 = [0];
        let mask_s: []u64 = mask_buf[0..];
        * = dfa.buildAllowedMask(0, mask_s);
        * = applyVocabMaskInPlace(logits_in, mask_buf[0..]);
        * = applyTemperatureAndTopKMaskInPlace(logits_in, 0.8, 1);
    }
}
```

### 长上下文 YaRN / NTK-Aware / Linear PI RoPE 扩展示例 (`ropeYarn.cp`)

```cp
import {
    RopeScalingKind,
    YarnConfig,
    yarnAttentionMscale,
    computeRopeInvFreqs,
    applyScaledRopeInPlaceF32,
} from "cap:attention";

pub fn demoYarnLongContextRope(q_head: *[]f32, pos: usize) {
    // 将 4K 预训练上下文 4x 扩展至 16K
    const cfg = YarnConfig.default(64, 4096, 4.0);
    let inv_freqs: [32]f64 = [0.0; 32];
    let inv_s: []f64 = inv_freqs[0..];
    * = computeRopeInvFreqs(RopeScalingKind.Yarn, cfg, inv_s);

    const mscale: f32 = yarnAttentionMscale(cfg.scale_factor, 1.0) as f32;
    * = applyScaledRopeInPlaceF32(q_head, pos, inv_freqs[0..], mscale);
}
```

### Medusa / Eagle 树状推测解码掩码与贪心验证示例 (`specTree.cp`)

```cp
import {
    SPEC_TREE_NO_PARENT,
    DraftTreeNode,
    buildTreeAttentionMask,
    verifyDraftTreeGreedy,
} from "cap:attention";

pub fn demoSpeculativeTreeVerify() {
    const nodes: [4]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 10, parent_idx: 0, depth: 1 },
        DraftTreeNode{ token_id: 20, parent_idx: 0, depth: 1 },
        DraftTreeNode{ token_id: 30, parent_idx: 2, depth: 2 },
    ];
    let mask: [16]u8 = [0; 16];
    let mask_s: []u8 = mask[0..];
    * = buildTreeAttentionMask(nodes[0..], mask_s);

    const target_argmax: [4]u32 = [20, 99, 30, 42];
    let accepted: [4]u32 = [0; 4];
    let accepted_s: []u32 = accepted[0..];
    if (res := verifyDraftTreeGreedy(nodes[0..], target_argmax[0..], accepted_s)) {
        // accepted == [20, 30], res.accepted_len == 2, res.bonus_token_id == 42
        * = res;
    }
}
```

### vLLM 风格 PagedAttention 物理块池、Copy-on-Write 分叉与 SIMD 解码核 (`pagedKv.cp`)

面向高吞吐 LLM 推理引擎的 100% 零堆分配 Paged KV-Cache 管理与 SIMD `@dot`/`@fma` 注意力解码核：

- **`PAGED_KV_INVALID_BLOCK` (`0xFFFFFFFF`)** / **`PagedKvPoolStats`** / **`PagedKvBlockPool`**：
  - `init(ref_counts: *[]u32): PagedKvBlockPool?` / `allocBlock(*_): u32?` / `retainBlock(*_, block_id): bool` / `releaseBlock(*_, block_id): bool` / `refCount(block_id)` / `freeBlocks()` / `allocatedBlocks()` / `stats()`（含高水位 `high_watermark`、利用率与 CoW 次数统计）。
- **`PagedTokenSlot`** / **`SeqBlockTable`**：
  - `init(block_ids: *[]u32, block_size: usize): SeqBlockTable?` / `physicalBlockAt(logical_idx)`
  - `appendTokenSlot(*_, pool, k_cache_flat, v_cache_flat, head_dim): PagedTokenSlot?`（跨块自动分配新物理块；向共享尾块追加时自动触发 Copy-on-Write 分离）
  - `forkSequence(pool, child_block_ids): SeqBlockTable?`（零拷贝分叉，共享前缀物理块并递增引用计数）
  - `ensureWritableBlock(*_, logical_idx, pool, k_cache_flat, v_cache_flat, head_dim): PagedTokenSlot?`（写时复制分离，绝不污染父序列）
  - `writeKvToken(slot, k_vec, v_vec, k_cache_flat, v_cache_flat, head_dim)` / `releaseAll(*_, pool)`（零泄漏回收）
- **`pagedAttentionDecodeF32(query, seq_table, k_cache_flat, v_cache_flat, head_dim, scale, out_vec): bool`**：按逻辑块表非连续索引物理 K/V 块，使用 8-lane SIMD `@dot` 计算缩放点积、单遍数值稳定 online Softmax 与 `@fma` 加权累加 V 输出。

```cp
import {
    PagedKvBlockPool,
    SeqBlockTable,
    pagedAttentionDecodeF32,
} from "cap:attention";

pub fn demoPagedAttentionCow() {
    // 4 个物理块，每块 block_size = 2，head_dim = 4
    let ref_counts: [4]u32 = [0; 4];
    let k_cache: [32]f32 = [0.0; 32];
    let v_cache: [32]f32 = [0.0; 32];
    let pool = PagedKvBlockPool.init(ref_counts[0..]) ?? return;

    let parent_blks: [4]u32 = [0; 4];
    let parent = SeqBlockTable.init(parent_blks[0..], 2) ?? return;

    // 追加 3 个前缀 token（占用 2 个物理块，尾块半满）
    const k0: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v0: [4]f32 = [10.0, 0.0, 0.0, 0.0];
    if (s0 := parent.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4)) {
        * = parent.writeKvToken(s0, k0[0..], v0[0..], k_cache[0..], v_cache[0..], 4);
    }

    // 零拷贝 CoW 分叉子序列（共享物理块，refcount 递增为 2）
    let child_blks: [4]u32 = [0; 4];
    let child = parent.forkSequence(&pool, child_blks[0..]) ?? return;

    // 子序列向共享尾块追加 token 触发 Copy-on-Write，不污染 parent
    const k1: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const v1: [4]f32 = [0.0, 20.0, 0.0, 0.0];
    if (s1 := child.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4)) {
        * = child.writeKvToken(s1, k1[0..], v1[0..], k_cache[0..], v_cache[0..], 4);
    }

    // SIMD PagedAttention 单头解码
    let out: [4]f32 = [0.0; 4];
    * = pagedAttentionDecodeF32(k0[0..], child, k_cache[0..], v_cache[0..], 4, 0.0, out[0..]);

    child.releaseAll(&pool);
    parent.releaseAll(&pool); // 全部物理块零泄漏归还
}
```

### vLLM / Sarathi 风格 Chunked-Prefill + Decode 连续批处理迭代调度器 (`iterSched.cp`)

面向高吞吐 LLM 推理引擎的 100% 零堆分配迭代级连续批处理调度器：

- **`IterSchedulerConfig`**：配置 `max_batch_seqs`、`max_token_budget_per_step`、`max_kv_blocks` 与 `block_size`。
- **`SeqRunState`**：完整序列生命周期状态机（`Waiting` / `Prefilling` / `Decoding` / `Preempted` / `Finished`）。
- **`ContinuousIterScheduler`**：
  - `init(entries_buf, config): ContinuousIterScheduler?`：在调用方栈/固定缓冲上零堆初始化。
  - `enqueue(*_, req_id, prompt_tokens, max_new_tokens, priority): bool`：入队请求（支持自动回收最旧 `Finished` 槽位）。
  - `scheduleStep(*_, out_items): IterStepPlan`：Phase 1 优先推进在途 `Decoding` 请求（若 KV 块不足，按最低优先级/最新入队抢占 `Preempted` 释放 KV 块防死锁）；Phase 2 将剩余 `token_budget` 分配给 `Prefilling`/`Preempted`/`Waiting` 请求做 **Chunked Prefill 分块预填充**。
  - `commitStep(*_, req_id, generated_tokens, hit_eos): bool`：提交本步执行结果，自动推进 `Prefilling -> Decoding` 并在命中 EOS 或达到 `max_new_tokens` 时释放全部 KV 块。
  - `cancelRequest(*_, req_id)` / `getEntry(req_id)` / `kvBlocksFree()` / `kvBlocksUsed()` / `activeCount()` / `stats()`。

```cp
import {
    IterSchedulerConfig,
    SeqSchedEntry,
    ScheduledStepItem,
    ContinuousIterScheduler,
} from "cap:attention";

pub fn demoContinuousIterScheduler() {
    let entries_buf: [8]SeqSchedEntry = [SeqSchedEntry{}; 8];
    let entries_s: []SeqSchedEntry = entries_buf[0..];
    const cfg = IterSchedulerConfig{
        max_batch_seqs: 4,
        max_token_budget_per_step: 8,
        max_kv_blocks: 16,
        block_size: 4,
    };
    let sched = ContinuousIterScheduler.init(entries_s, cfg) ?? return;

    // 入队 1 个长 Prompt 请求（14 tokens -> 跨 2 步 Chunked Prefill）与 1 个短高优请求
    * = sched.enqueue(1001, 14, 4, 1);
    * = sched.enqueue(1002, 4, 2, 10);

    let items_buf: [4]ScheduledStepItem = [ScheduledStepItem{}; 4];
    let items_s: []ScheduledStepItem = items_buf[0..];

    // Step 1: 高优 1002 预填充 4 tokens，剩余预算 4 tokens 分配给 1001 做首块 Chunked Prefill
    const plan1 = sched.scheduleStep(items_s);
    let i: usize = 0;
    while (i < plan1.num_items) : (i += 1) {
        const it = items_buf[i];
        * = sched.commitStep(it.req_id, if (it.is_prefill) 0 else 1, false);
    }

    // Step 2: 1002 进入 Decode (1 token)，剩余预算 7 tokens 继续推进 1001 预填充 (Prefill+Decode 混合步)
    const plan2 = sched.scheduleStep(items_s);
    * = plan2;
}
```

### 零分配长上下文 KV-Cache 驱逐引擎：StreamingLLM Attention Sink + H2O Heavy-Hitter Oracle (`kvEvict.cp`)

面向长上下文 LLM 推理与多轮 AI Agent 对话的 100% 纯 CP 零堆分配 KV-Cache 淘汰策略内核（内联固定容量 `MAX_KV_EVICTION_SLOTS = 256`）：

- **策略与角色枚举**：`KvEvictionPolicyKind`（`streaming_sink_window | h2o_heavy_hitter | hybrid_sink_h2o_window`）、`KvTokenRole`（`empty | attention_sink | heavy_hitter | recent_window`）
- **配置与数据结构**：`KvEvictionConfig`（`sink_budget`, `heavy_budget`, `recent_budget`, `score_decay_f32`, `policy`，提供 `totalCapacity()` 与 `isValid()`）、`KvSlotMeta`、`KvEvictionResult`、`KvEvictionStats`（含实时压缩比 `compression_ratio_f32`）
- **核心引擎（`KvEvictionManager`）**：
  - `init(cfg): KvEvictionManager?`
  - `admit_token(*_, token_pos, kv_block_idx, init_score): KvEvictionResult`：前 `sink_budget` 个 token 永久锚定为 `attention_sink`；满预算时将滑出 `recent_window` 的高分 token 自动晋升为 `heavy_hitter`，并精准驱逐最低分（LRU 破平局）的非 sink 槽位；严格保证三区配额守恒。
  - `update_attention_scores(*_, step_scores): bool`：基于硬件 `@fma` 执行 `score = @fma(score, decay, step_score)` 并刷新活跃槽位 LRU 步号。
  - `lookup_by_token_pos(token_pos): usize?` / `collect_active_positions(out_pos): usize`（按 `token_pos` 升序零堆输出驻留 RoPE 位置列表）/ `stats()` / `reset()`。

```cp
import {
    KvEvictionPolicyKind,
    KvEvictionConfig,
    KvEvictionManager,
} from "cap:attention";

pub fn demoKvEvictionHybrid() {
    const cfg = KvEvictionConfig{
        sink_budget: 2,
        heavy_budget: 2,
        recent_budget: 2,
        score_decay_f32: 0.9,
        policy: KvEvictionPolicyKind.hybrid_sink_h2o_window,
    };
    let mgr = KvEvictionManager.init(cfg) ?? return;

    // 接入 10 个 token：pos 0..1 永久锁定为 Sink，高分 token 晋升 Heavy-Hitter，低分淘汰
    let pos: u64 = 0;
    while (pos < 10) : (pos += 1) {
        const init_s: f32 = if (pos == 3) 5.0 else 0.1;
        * = mgr.admit_token(pos, pos as u32, init_s);
    }

    let active_pos: [6]u64 = [0; 6];
    let active_s: []u64 = active_pos[0..];
    const n = mgr.collect_active_positions(active_s); // 升序 RoPE 位置列表，必含 0, 1, 3, 8, 9
    * = n;
    * = mgr.stats().compression_ratio_f32; // 10 / 6 = 1.6667x
}
```

### 零分配多租户 Multi-LoRA 适配器缓存路由与 SGMV 批处理编排引擎 (`loraRouter.cp`)

面向 S-LoRA / Punica / vLLM 多租户 LLM 推理服务的 100% 纯 CP 零动态堆分配 Multi-LoRA 适配器驻留缓存、引用计数 LRU 驱逐、混合批 SGMV 编排与 SIMD 增量核：

- **常量与数据模型**：`LORA_ROUTER_MAX_ADAPTERS` (`32`)、`LORA_MAX_BATCH_GROUPS` (`32`)、`LoraRouterConfig`（`max_loaded_adapters`, `memory_budget_bytes`, `max_rank`, `isValid()`）、`LoraAdapterSlot`、`LoraAcquireOutcome`、`LoraBatchGroup`、`LoraRouterStats`。
- **多租户适配器驻留路由器（`MultiLoraRouter`）**：
  - `init(cfg): MultiLoraRouter?`
  - `acquire_adapter(*_, adapter_id, rank, alpha_f32, mem_bytes, step, pin): LoraAcquireOutcome`：命中则递增 `active_refs` 并刷新 `last_used_step`；未命中时原子预检冷槽位可回收容量，按 LRU 驱逐 `active_refs == 0 && !pinned` 的冷适配器；若全被锁定/在途占用则返回 `oom_blocked = true` 且不破坏现有缓存。
  - `release_adapter(*_, adapter_id): bool`（递减引用计数防下溢）、`unpin_adapter(*_, adapter_id): bool`、`get_slot(slot_idx)`、`stats()`。
- **零堆 SGMV 批编排与 SIMD 增量算子**：
  - `planLoraSgmvBatch(req_adapter_ids, out_groups, out_sorted_req_indices): usize`：零辅助内存将混合多租户请求按 `adapter_id` 稳定聚合为连续 SGMV 分组（`adapter_id == 0` 的 base-only 请求独立成组）。
  - `applyLoraDeltaSimdF32(x, a_mat, b_mat, in_dim, rank, out_dim, scale, scratch_mid, y_inout): bool`：利用 CP 原生 8-lane SIMD `@dot` 与硬件 `@fma` 原位计算 `y += scale * (B * (A * x))`。

```cp
import {
    LoraRouterConfig,
    LoraBatchGroup,
    MultiLoraRouter,
    planLoraSgmvBatch,
    applyLoraDeltaSimdF32,
} from "cap:attention";

pub fn demoMultiLoraSgmv() {
    let router = MultiLoraRouter.init(LoraRouterConfig{
        max_loaded_adapters: 4,
        memory_budget_bytes: 64 * 1024 * 1024,
        max_rank: 32,
    }) ?? return;

    * = router.acquire_adapter(101, 16, 32.0, 8 * 1024 * 1024, 1, false);
    * = router.acquire_adapter(202, 8, 16.0, 4 * 1024 * 1024, 2, true);

    // 混合多租户 batch 稳定分组（0 为 base-only）
    const reqs: [5]u64 = [101, 0, 202, 101, 0];
    let groups: [4]LoraBatchGroup = [LoraBatchGroup{}; 4];
    let sorted: [5]usize = [0; 5];
    let groups_s: []LoraBatchGroup = groups[0..];
    let sorted_s: []usize = sorted[0..];
    const ng = planLoraSgmvBatch(reqs[0..], groups_s, sorted_s);
    * = ng; // 3 组: [101: 0,3], [0: 1,4], [202: 2]
}
```

### 零分配受限解码前缀树（Prefix-Trie）与 Logit 掩码处理器 (`guidedTrie.cp`)

面向生产级 LLM 受限解码（工具名白名单、枚举值约束、JSON 键名约束、vLLM / SGLang Guided Decoding）的 100% 纯 CP 零动态堆分配 Token-ID 前缀树与 $O(V + B)$ Logit 掩码引擎：

- **常量与数据模型**：
  - `GUIDED_TRIE_MAX_NODES` (`512`)、`GUIDED_TRIE_NO_NODE` (`0xFFFF`)
  - `GuidedTrieNode`（`token_id`, `is_terminal`, `first_child`, `next_sibling`, `parent`, `depth`）：Left-Child Right-Sibling（LCRS）紧凑节点布局，零堆分配。
  - `GuidedTrieCursor`（`node_idx`, `step_count`, `is_complete`, `is_dead`）：轻量值类型解码光标，支持推测解码多分支回溯与非法 Token 死状态拦截。
  - `GuidedTrieStats`（`node_count`, `sequence_count`, `max_depth`）：前缀树规模与深度统计快照。
- **核心引擎（`GuidedTokenTrie`）**：
  - `init(): GuidedTokenTrie`：初始化含虚拟根节点（`index = 0`）的空前缀树。
  - `insert_sequence(*_, tokens: []const u32): bool`：两阶段原子插入——自动共享公共前缀、重复序列幂等不重复计数、拒绝空序列、容量不足时零副作用返回 `false`。
  - `new_cursor(): GuidedTrieCursor` / `advance(cursor, token_id): GuidedTrieCursor`：从根节点或当前光标沿合法子边推进；未命中时返回 `is_dead = true` 的死光标。
  - `allowed_next_tokens(cursor, eos_token_id, out_tokens): usize`：枚举当前光标所有合法下一 Token；当 `cursor.is_complete` 时自动将 `eos_token_id` 纳入合法集合（自动去重）。
  - `apply_logit_mask_f32(cursor, eos_token_id, logits): usize`：零堆分配 $O(V + B)$ Logit 掩码处理器，将所有非法词表项置为 `-1.0e30`，精确保留所有合法下一 Token（及终态 `eos_token_id`）的原始 logit 并返回合法数量；越界 `token_id` 安全忽略，死光标屏蔽全部词表返回 `0`。
  - `stats(): GuidedTrieStats`：返回当前节点数、序列数与最大深度。

```cp
import {
    GuidedTokenTrie,
} from "cap:attention";

pub fn demoGuidedTokenTrie() {
    let trie = GuidedTokenTrie.init();
    const eos_id: u32 = 2;

    // 注册合法工具调用名称 Token 序列（自动共享前缀 [10, 20]）
    const tool_get: [2]u32 = [10, 20];
    const tool_get_user: [3]u32 = [10, 20, 31];
    const tool_set: [2]u32 = [10, 25];
    * = trie.insert_sequence(tool_get[0..]);
    * = trie.insert_sequence(tool_get_user[0..]);
    * = trie.insert_sequence(tool_set[0..]);

    // 沿 [10, 20] 推进：既是终态又是中间节点，允许继续走 31 或生成 eos_id (2)
    let cur = trie.new_cursor();
    cur = trie.advance(cur, 10);
    cur = trie.advance(cur, 20);

    let logits_buf: [64]f32 = [0.5; 64];
    logits_buf[2] = 1.2;
    logits_buf[31] = 2.8;
    logits_buf[50] = 99.0; // 非法高分 token 将被掩码为 -1.0e30
    let logits: []f32 = logits_buf[0..];
    const allowed = trie.apply_logit_mask_f32(cur, eos_id, logits);
    * = allowed; // 2 (token 31 与 eos_id 2)
}
```

### 零分配生产级 LLM Logit 采样管线：Penalties + Top-K + Min-P + Top-P Nucleus (`nucleus.cp`)

面向 llama.cpp / vLLM / SGLang 级推理引擎的 100% 纯 CP 零动态堆分配全栈 Logit 后处理与随机采样管线（栈上固定候选池 `SAMPLER_MAX_CANDIDATES = 256`）：

- **常量与数据模型**：
  - `SAMPLER_MAX_CANDIDATES = 256`
  - `NucleusSamplerConfig`（`temperature` [`0.0` 贪心], `top_k` [`0` 不限], `top_p` [`(0, 1]`], `min_p` [`[0, 1]`], `repeat_penalty` [`>= 1.0`], `frequency_penalty`, `presence_penalty`；提供 `defaults()` 与 `isValid()`）
  - `SamplerCandidate`（`token_id`, `logit`, `prob`）、`NucleusSampleOutcome`（`ok`, `token_id`, `selected_prob`, `kept_candidates`, `entropy_nats`）
- **核心纯函数**：
  - `applyTokenPenaltiesInPlace(logits, recent_tokens, repeat_penalty, freq_penalty, presence_penalty)`：Keskar 正/负 logit 双向重复惩罚 + OpenAI `freq_penalty * count + presence_penalty`（自动去重统计频次，安全跳过越界 ID 与非有限 logit）。
  - `selectTopKCandidates(logits, top_k, out_cands): usize`：原地最小堆 $O(V \log K)$ 提取降序 Top-K 有限候选，自动过滤 `NaN`/`±Inf`，平局按 `token_id` 升序决胜。
  - `softmaxAndTruncateNucleus(cands, count, temperature, top_p, min_p): usize`：数值稳定温度 Softmax -> **Min-P** 自适应长尾截断（`prob >= min_p * max_prob`）-> **Top-P (Nucleus)** 累计概率截断（至少保留 1 候选）-> 重归一化概率和为 `1.0`。
  - `sampleFromLogits(logits, recent_tokens, cfg, rng_seed): NucleusSampleOutcome`：端到端零堆管线（含 `temperature == 0` 贪心快路径、53-bit SplitMix64 确定性采样与香农熵 `entropy_nats` 遥测）。

```cp
import {
    NucleusSamplerConfig,
    sampleFromLogits,
} from "cap:attention";

pub fn demoNucleusSamplerPipeline() {
    let logits_buf: [6]f32 = [3.5, 3.2, 1.0, -0.5, -2.0, -5.0];
    const recent: [2]u32 = [0, 0]; // token 0 重复出现 2 次
    let logits: []f32 = logits_buf[0..];

    let cfg = NucleusSamplerConfig.defaults();
    cfg.temperature = 0.8;
    cfg.top_k = 4;
    cfg.min_p = 0.05;
    cfg.top_p = 0.90;
    cfg.repeat_penalty = 1.25;
    cfg.frequency_penalty = 0.2;

    const outcome = sampleFromLogits(logits, recent[0..], cfg, 2026);
    * = outcome; // outcome.ok == true, 附带 selected_prob, kept_candidates, entropy_nats
}
```

### 零分配 SGLang 风格 RadixAttention 前缀共享 KV-Cache 树与叶优先 LRU 驱逐引擎 (`radixKv.cp`)

面向多轮 AI Agent 对话与多租户共享系统提示词（System Prompt / Few-Shot / Tool Schema）的 100% 纯 CP 零动态堆分配 **SGLang RadixAttention** 前缀压缩树（LCRS 布局）与叶优先 LRU 驱逐引擎：

- **常量与数据模型**：
  - `RADIX_KV_MAX_NODES` (`128`，槽位 `0` 为永久虚拟根，槽位 `1..127` 为可复用压缩边节点)、`RADIX_KV_MAX_EDGE_TOKENS` (`16`，单边最多压缩 16 个 token 与 KV 块 ID)、`RADIX_KV_NO_NODE` (`0xFFFF`)
  - `RadixKvNode`（`tokens: [16]u32`, `kv_blocks: [16]u32`, `token_len: u8`, `parent`, `first_child`, `next_sibling`, `lock_ref: u32`, `last_access_step: u64`, `occupied: bool`）
  - `RadixMatchResult`（`matched_tokens`, `last_node_idx`, `out_blocks_written`）
  - `RadixKvStats`（`active_nodes`, `cached_tokens`, `locked_nodes`, `total_hits`, `total_splits`, `total_evicted_tokens`）
- **核心引擎（`RadixKvCacheTree`）**：
  - `init(): RadixKvCacheTree`：初始化含虚拟根节点（`index = 0`）的空前缀共享树（零堆分配）。
  - `match_prefix(*_, query_tokens: []const u32, now_step: u64, out_kv_blocks: *[]u32): RadixMatchResult`：自根向下执行最长公共前缀匹配（完整边遍历 + 边内部分前缀匹配），刷新命中路径 `last_access_step`，写出对应 KV 块 ID 并累计 `total_hits`。
  - `insert_prefix(*_, tokens: []const u32, kv_blocks: []const u32, now_step: u64): usize`：共享已有公共前缀；在边中间分叉时自动执行 Radix Edge Split（`total_splits += 1`），并由后缀节点完整继承原节点的 `lock_ref` 与 `last_access_step`；长序列（`> 16` tokens）自动切分链式存储；原子容量预检确保满池零副作用返回 `0`；重复序列幂等返回 `0`。
  - `lock_path(*_, leaf_node_idx: u16): bool` / `unlock_path(*_, leaf_node_idx: u16): bool`：沿 `parent` 链对整条前缀路径递增/两阶段原子防下溢递减 `lock_ref`，保护在途推理分支不被 LRU 驱逐。
  - `evict_lru_tokens(*_, needed_tokens: usize, out_freed_blocks: *[]u32): usize`：严格叶优先 LRU 驱逐——每轮仅淘汰 `first_child == RADIX_KV_NO_NODE && lock_ref == 0` 且 `last_access_step` 最小的冷叶子节点；摘除后若父节点变为无子且未锁定则自动进入下一轮级联回收，并释放槽位供后续复用。
  - `stats(): RadixKvStats`：返回当前活跃数据节点数、已缓存 Token 总数、已锁定节点数以及累计命中/分裂/驱逐统计。

```cp
import {
    RadixKvCacheTree,
} from "cap:attention";

pub fn demoRadixAttentionKvTree() {
    let tree = RadixKvCacheTree.init();

    // 1. 写入多租户共享系统提示词 + 对话分支 A
    const seq_a: [6]u32 = [101, 102, 103, 104, 201, 202];
    const kv_a: [6]u32 = [10, 11, 12, 13, 20, 21];
    * = tree.insert_prefix(seq_a[0..], kv_a[0..], 1);

    // 2. 分支 B 共享 [101..104] 并在边中间分叉 -> 自动触发 Radix Edge Split
    const seq_b: [6]u32 = [101, 102, 103, 104, 301, 302];
    const kv_b: [6]u32 = [10, 11, 12, 13, 30, 31];
    * = tree.insert_prefix(seq_b[0..], kv_b[0..], 2);

    // 3. 匹配分支 A 并锁定在途路径，防止被 LRU 驱逐
    let matched_blocks: [8]u32 = [0; 8];
    let matched_s: []u32 = matched_blocks[0..];
    const m = tree.match_prefix(seq_a[0..], 5, matched_s);
    * = tree.lock_path(m.last_node_idx);

    // 4. 内存压力下按叶优先 LRU 驱逐：仅淘汰未锁定的冷分支 B，保护在途分支 A
    let freed_buf: [8]u32 = [0; 8];
    let freed_s: []u32 = freed_buf[0..];
    const freed = tree.evict_lru_tokens(2, freed_s);
    * = freed; // 2 (释放 kv_b 尾部块 30, 31)
    * = tree.unlock_path(m.last_node_idx);
}
```

### 零分配 Leviathan 无损推测解码拒绝采样与残差重采样引擎 (`specReject.cp`)

面向生产级 LLM 推测解码（Leviathan et al. 2023 / DeepMind / vLLM Lossless Speculative Decoding）的 100% 纯 CP 零动态堆分配精确拒绝采样、两遍 O(V) 残差分布重采样与 `@fma` EWMA 自适应草稿步长控制器：

- **常量与数据模型**：
  - `SPEC_REJECT_MAX_DRAFT_K` (`16`，单步最多校验 16 个草稿 Token，全接受时含 Bonus Token 最多产出 `17` 个 Token)
  - `SpecRejectionOutcome`（`ok`, `accepted_draft_count`, `emitted_tokens: [17]u32`, `emitted_count`, `rejected_at_idx`, `used_bonus_token`, `acceptance_rate_f32`）
  - `SpecStepSchedulerConfig`（`min_k`, `max_k`, `initial_k`, `ewma_alpha`, `grow_threshold`, `shrink_threshold`；提供 `defaults()` 与 `isValid()`）
- **核心纯函数与自适应控制器**：
  - `sampleResidualDistributionF32(target_probs, draft_probs, rng_u01): u32?`：两遍 O(V) 零堆分配扫描，从归一化残差分布 `norm(max(0, p(x) - q(x)))` 精确采样修正 Token；若残差质量近似为 `0`（如 `p == q`），安全退化为直接从 `target_probs` 采样。
  - `verifySpeculativeRejectionF32(draft_tokens, draft_probs_flat, target_probs_flat, vocab_size, rng_seed): SpecRejectionOutcome`：逐位置以接受概率 `min(1, p_i(x_i) / q_i(x_i))` 执行无损拒绝采样；首次拒绝立即从残差分布输出修正 Token 并终止；若全部 `k` 个草稿均被接受，则从第 `k + 1` 组目标分布采样额外 Bonus Token（`used_bonus_token = true`，单步产出 `k + 1` 个 Token）。
  - `SpecStepScheduler`（`init`, `observe_round`, `current_k`, `ewma_acceptance_rate`）：基于硬件 `@fma` 追踪 EWMA 接受率，高接受率自动扩容草稿步长 `K`、低接受率自动缩容并严格夹紧在 `[min_k, max_k]` 内。

```cp
import {
    SpecStepSchedulerConfig,
    SpecStepScheduler,
    verifySpeculativeRejectionF32,
} from "cap:attention";

pub fn demoSpeculativeRejection() {
    // 2 个草稿 token (vocab_size = 4)，target 含第 k+1 组 bonus 分布
    const draft_tokens: [2]u32 = [1, 2];
    const q_flat: [8]f32 = [
        0.1, 0.7, 0.1, 0.1,
        0.1, 0.1, 0.7, 0.1,
    ];
    const p_flat: [12]f32 = [
        0.1, 0.7, 0.1, 0.1, // 与 q_0 一致 -> 100% 接受 token 1
        0.1, 0.1, 0.7, 0.1, // 与 q_1 一致 -> 100% 接受 token 2
        0.0, 0.0, 0.0, 1.0, // bonus 分布 -> 必产出 token 3
    ];

    const out = verifySpeculativeRejectionF32(draft_tokens[0..], q_flat[0..], p_flat[0..], 4, 42);
    // out.ok == true, out.emitted_count == 3 ([1, 2, 3]), out.used_bonus_token == true
    * = out;

    if (sched := SpecStepScheduler.init(SpecStepSchedulerConfig.defaults())) {
        let s = sched;
        const next_k = s.observe_round(out.accepted_draft_count, 2);
        * = next_k;
    }
}
```

### 零分配 SGLang RadixAttention 前缀 KV-Cache 压缩前缀树与 LRU 驱逐引擎 (`radixCache.cp`)

面向 SGLang RadixAttention 风格长前缀共享 KV-Cache 复用的 100% 纯 CP 零动态堆分配压缩前缀树（Radix Tree）与自底向上 LRU 驱逐引擎：

- **常量与数据模型**：
  - `PRC_MAX_NODES = 128`（节点 0 为虚拟根）、`PRC_MAX_POOL_SLOTS = 2048`（连续 Token / KV-Page 槽位池，带零堆原地碎片紧缩）、`PRC_NO_NODE = 0xFFFF`、`PRC_ROOT_NODE = 0`
  - `PrefixRadixNode`（`node_id`, `parent_idx`, `first_child_idx`, `next_sibling_idx`, `child_count`, `token_offset`, `token_len`, `ref_count`, `last_access_tick`, `active`）
  - `PrefixRadixMatch`（`matched_tokens`, `matched_pages`, `leaf_node_id`, `split_offset`）、`PrefixRadixInsertResult`、`PrefixRadixEvictReport`、`PrefixRadixStats`
- **核心引擎 `PrefixRadixCache`**：
  - `init(): PrefixRadixCache` / `getNode(node_id)` / `stats()`
  - `matchPrefix(*_, tokens, now_tick, out_matched_pages: *[]u32): PrefixRadixMatch`：纯迭代最长前缀匹配，支持边中间部分命中（`split_offset > 0`），自动刷新命中路径 `last_access_tick`。
  - `insertPrefix(*_, tokens, kv_pages, now_tick): PrefixRadixInsertResult`：纯迭代前缀插入与 **O(1) 零拷贝偏移边分裂**；池尾不足时自动原地紧缩回收已驱逐碎片。
  - `pinNode(*_, node_id)` / `unpinNode(*_, node_id)`：沿祖先链原子递增/递减 `ref_count`，保护在途请求共享前缀不被驱逐。
  - `evictPages(*_, needed_pages, out_freed_pages: *[]u32): PrefixRadixEvictReport`：自底向上 LRU 叶子驱逐（仅淘汰 `ref_count == 0` 的最冷叶子，级联回收空父节点）。

```cp
import { PrefixRadixCache } from "cap:attention";

pub fn demoPrefixRadixCache() {
    let cache = PrefixRadixCache.init();
    const t1: [4]u32 = [10, 20, 30, 40];
    const p1: [4]u32 = [100, 101, 102, 103];
    const r1 = cache.insertPrefix(t1[0..], p1[0..], 1);

    // 共享前缀 [10, 20] 分叉 -> 自动 O(1) 偏移边分裂
    const t2: [4]u32 = [10, 20, 99, 100];
    const p2: [4]u32 = [100, 101, 202, 203];
    * = cache.insertPrefix(t2[0..], p2[0..], 2);

    // Pin 保护分支 1，驱逐未锁定的冷分支
    * = cache.pinNode(r1.leaf_node_id);
    let freed: [4]u32 = [0; 4];
    let freed_s: []u32 = freed[0..];
    const ev = cache.evictPages(2, *freed_s);
    * = ev.freed_pages; // 2 (202, 203)
    * = cache.unpinNode(r1.leaf_node_id);
}
```


### 零分配 LLM Prompt 边界 Token Healing 与字节前缀 Logits 掩码引擎 (`tokenHeal.cp`)

面向大语言模型推理（Guidance / llama.cpp 风格）的 100% 纯 CP 零动态堆分配 Prompt 末尾边界 Token Healing 与字节前缀 Logits `-Inf` 掩码引擎，彻底消除 BPE 切分边界伪影（如 `"http:"` 切分阻断生成 `"http://"`）：

- **常量与数据模型**：
  - `HEAL_MAX_VOCAB_TOKENS = 512`、`HEAL_MASK_WORDS = 8`、`HEAL_MAX_BYTE_POOL = 8192`、`HEAL_MAX_PREFIX_BYTES = 64`、`HEAL_MAX_BACKTRACK_TOKENS = 8`
  - `HealMatchMode`（`StrictSuperset` 严格超集匹配、`MultiStepPrefix` 多步连续前缀匹配）、`HealSessionState`（`Inactive` / `Active` / `Completed` / `Failed`）
  - `TokenHealConfig`（`max_backtrack_tokens`, `mode`, `require_strict_extension`；提供 `defaults()` 与 `multiStep(max_backtrack)`）
  - `HealVocabTable`（`init`, `addToken`, `setToken`, `tokenBytes`, `isValidToken`, `vocabSize`, `poolUsed`）：紧凑固定字节池词表，支持顺序追加、稀疏注册与零拷贝字节切片查询。
  - `HealCandidateMask`（`empty`, `allowAll`, `clearAll`, `allowToken`, `forbidToken`, `isAllowed`, `allowedCount`, `firstAllowedToken`）：64-bit 位图候选集，使用硬件 `@popCount` 精确统计有效候选数。
  - `TokenHealPlan`（`no_heal_needed`, `trimmed_prompt_len`, `backtrack_count`, `heal_prefix`, `heal_prefix_len`, `candidate_count`, `strict_extension_count`, `prefixSlice()`）
  - `HealStepOutcome`（`accepted`, `completed`, `consumed_prefix_bytes`, `remaining_prefix_len`, `extra_healed_bytes`）
- **核心纯函数与多步修复状态机**：
  - `planTokenHealing(prompt_tokens, vocab): TokenHealPlan` / `planTokenHealingWithConfig(prompt_tokens, vocab, cfg): TokenHealPlan`：支持从 Prompt 末尾回退 1 个或多个不完整边界 Token，提取拼接字节前缀 `heal_prefix` 与回退后前缀长度 `trimmed_prompt_len`；空 prompt、未知 token、无匹配候选时安全降级为 `no_heal_needed = true`。
  - `buildHealingCandidateMask(vocab, prefix, mode): HealCandidateMask`：基于紧凑词表字节池生成匹配位图。
  - `applyHealingLogitsMaskInPlace(logits: *[]f32, mask): usize?`：原地对非候选及 `NaN`/`Inf` 槽位置 `-inf` 并返回保留有效候选数；`logits.len < mask.vocab_size` 时无副作用返回 `null`。
  - `TokenHealingSession`（`init`, `fromPlan`, `isActive`, `isCompleted`, `trimmedPromptLen`, `remainingPrefix`, `buildStepMask`, `applyStepLogitsMaskInPlace`, `advance`）：支持单步超集与多步部分前缀连续推进，完全覆盖 `heal_prefix` 后自动转入 `Completed` 并解除 Logits 掩码。

```cp
import {
    HealMatchMode,
    TokenHealConfig,
    HealVocabTable,
    TokenHealingSession,
    planTokenHealing,
} from "cap:attention";

pub fn demoTokenHealing() {
    let vocab = HealVocabTable.init();
    const id_visit = vocab.addToken("Visit ") ?? 0;
    const id_http_c = vocab.addToken("http:") ?? 0;
    const id_http_url = vocab.addToken("http://") ?? 0;
    * = vocab.addToken("https://");

    const prompt: [2]u32 = [id_visit, id_http_c];
    const plan = planTokenHealing(prompt[0..], vocab);
    // plan.no_heal_needed == false, plan.trimmed_prompt_len == 1, plan.prefixSlice() == "http:"

    let session = TokenHealingSession.fromPlan(plan, HealMatchMode.StrictSuperset);
    let logits: [4]f32 = [1.0, 3.5, 4.2, 9.9];
    let logits_s: []f32 = logits[0..];
    * = session.applyStepLogitsMaskInPlace(vocab, *logits_s); // 屏蔽 "Visit " 与 "https://" 为 -inf

    // 采样得到 "http://" -> 单步完成修复并自动解除后续掩码
    const step = session.advance(id_http_url, vocab);
    * = step.completed; // true
}
```


### 零分配 StreamingLLM + H2O KV-Cache 压缩器 (`kvCompactor.cp`)

面向长上下文大模型自回归推理的 100% 纯 CP 零动态堆分配 **StreamingLLM + H2O (Heavy-Hitter Oracle) KV-Cache Compactor**：

- **常量与数据模型**：
  - `KV_COMPACTOR_MAX_SLOTS = 256`：固定容量内联槽位表上限，零动态堆分配。
  - `KvSlotRole`：`sink`（初始 attention-sink 永久保留）、`heavy_hitter`（中间区 Top-H 重击者）、`recent_window`（尾部滑动窗口）、`evictable`（待压缩淘汰）。
  - `KvCompactorSlot`：单槽元数据（`pos_id`, `attn_score_ema`, `last_step`, `role`）。
  - `KvCompactorConfig`：三区配额与 EMA 衰减配置（`sink_budget`, `heavy_budget`, `recent_budget`, `ema_decay`, `auto_compact_on_full`；提供 `defaults()`, `totalBudget()`, `isValid()`）。
  - `KvCompactReport`：压缩统计报告（`before_len`, `after_len`, `evicted_count`, `written_evicted_pos`, `sink_kept`, `heavy_kept`, `recent_kept`, `step`）。
  - `KvCompactorStats`：全局占用、四类角色分布与累计追加/驱逐/压缩计数器。
- **核心引擎 `KvCacheCompactor` 方法**：
  - `init(cfg): KvCacheCompactor?`：校验总预算 `1 <= totalBudget() <= 256` 且 `ema_decay ∈ (0.0, 1.0]`，非法配置返回 `null`。
  - `appendToken(*_, pos_id, initial_attn): bool`：清洗 NaN/Inf/负权重后追加新 token 并自动刷新三区角色；物理满池时依据 `auto_compact_on_full` 自动压缩或驱逐最弱非 sink 槽位（全 sink 满池时拒绝驱逐返回 `false`）。
  - `updateAttentionStep(*_, attn_weights): bool`：每 decode 步使用硬件 `@fma` 原地更新各槽位 EMA 注意力重要度（`score = @fma(decay, score, clean_w)`），自动清洗 NaN/Inf/负权重；权重长度不匹配时原子拒绝并返回 `false`。
  - `compactToBudget(*_, out_evicted_pos: *[]u64): KvCompactReport`：一键原地双指针紧缩保留槽位（严格保持 `pos_id` 时序递增），将淘汰位置写入 `out_evicted_pos` 并返回 `KvCompactReport`（空池与未超预算均安全无副作用）。
  - `len()` / `capacity()` / `budget()` / `needsCompaction()` / `slotAt(i)` / `activeSlots()` / `findByPosId(pos_id)` / `stats()` / `reset(*_)`。

```cp
import {
    KvCompactorConfig,
    KvCacheCompactor,
} from "cap:attention";

pub fn demoKvCompactor() {
    const cfg = KvCompactorConfig {
        sink_budget: 2,
        heavy_budget: 2,
        recent_budget: 2,
        ema_decay: 0.9,
        auto_compact_on_full: true,
    };
    if (let comp := KvCacheCompactor.init(cfg)) {
        let p: u64 = 0;
        while (p < 10) : (p += 1) {
            * = comp.appendToken(p, 0.05);
        }
        const weights: [10]f32 = [0.2, 0.2, 0.01, 0.95, 0.02, 0.85, 0.01, 0.01, 0.1, 0.1];
        * = comp.updateAttentionStep(weights[0..]);

        let evicted: [8]u64 = [0; 8];
        let evicted_s: []u64 = evicted[0..];
        const rep = comp.compactToBudget(*evicted_s);
        // rep.before_len == 10, rep.after_len == 6, rep.evicted_count == 4
        // 保留 sink(0,1) + heavy_hitter(3,5) + recent_window(8,9)
        * = rep.after_len;
    }
}
```


### 零分配 AWQ 激活感知 INT4 分组量化与显著通道缩放 GEMV 内核 (`quantAwq.cp`)

面向大模型边缘端侧 W4A16/W4A32 推理的 100% 纯 CP 零动态堆分配 **AWQ (Activation-Aware Weight Quantization)** 量化与融合反量化 GEMV 内核，原生 8-lane SIMD `@dot` 与硬件 `@fma` 驱动：

- **常量与数据模型**：
  - `AWQ_MIN_CHANNEL_SCALE = 1.0e-4`、`AWQ_MAX_CHANNEL_SCALE = 1.0e4`
  - `AwqGroupMeta`（`scale: f32`, `zero_point: u8`）
  - `AwqQuantErrorStats`（`rows`, `cols`, `mse`, `max_abs_err`, `min_row_cosine`, `mean_row_cosine`）
  - 几何辅助：`awqGroupsPerRow(cols, group_size)`、`awqPackedBytesPerRow(cols)`
- **核心量化与推理内核**：
  - `computeAwqChannelScales(activations, num_samples, cols, alpha, *out_scales): bool`：按通道统计 `s_j = mean(|x_j|)^alpha` 识别显著权重通道，自动清洗 `NaN`/`Inf` 并将零激活通道置为中性缩放 `1.0`。
  - `awqQuantizeMatrix(weights, channel_scales, rows, cols, group_size, *out_packed, *out_groups): bool`：先执行通道预缩放 `w' = w * s_j` 再进行分组非对称 INT4 打包（每字节 2 个 4-bit 无符号值 `0..15`），全零/常量组零除零风险。
  - `awqDequantizeRow(packed_weights, groups, channel_scales, row_idx, rows, cols, group_size, *out_row): bool`：逐行反量化并还原至原始权重空间。
  - `awqGemvF32(packed_weights, groups, channel_scales, x, rows, cols, group_size, *scaled_x_scratch, *out_y): bool`：零堆分配融合 GEMV，先将输入激活按 `1 / s_j` 一次性预缩放，再通过 32 元素栈瓦片调用 `@dot` + `@fma` 累积各分组点积。
  - `evaluateAwqQuantError(orig_weights, packed_weights, groups, channel_scales, rows, cols, group_size, *row_scratch): AwqQuantErrorStats?`：诊断量化前后 MSE、最大绝对误差与逐行余弦保真度。

```cp
import {
    AwqGroupMeta,
    computeAwqChannelScales,
    awqQuantizeMatrix,
    awqGemvF32,
} from "cap:attention";

pub fn demoAwqGemv(calib_acts: []const f32, weights: []const f32, x: []const f32) {
    let scales: [16]f32 = [1.0; 16];
    let scales_s: []f32 = scales[0..];
    * = computeAwqChannelScales(calib_acts, 1, 16, 0.5, *scales_s);

    let q_packed: [16]u8 = [0; 16]; // 2 rows x 16 cols -> 16 bytes
    let packed_s: []u8 = q_packed[0..];
    let groups: [2]AwqGroupMeta = [AwqGroupMeta{}; 2];
    let groups_s: []AwqGroupMeta = groups[0..];
    * = awqQuantizeMatrix(weights, scales[0..], 2, 16, 16, *packed_s, *groups_s);

    let scratch: [16]f32 = [0.0; 16];
    let scratch_s: []f32 = scratch[0..];
    let out_y: [2]f32 = [0.0; 2];
    let out_y_s: []f32 = out_y[0..];
    * = awqGemvF32(q_packed[0..], groups[0..], scales[0..], x, 2, 16, 16, *scratch_s, *out_y_s);
}
```


### 零分配 FlashDecoding Split-K KV 分区注意力与 Log-Sum-Exp 无损归约内核 (`flashDecode.cp`)

面向超长上下文自回归解码（Decode 阶段）的 100% 纯 CP 零动态堆分配 **Dao-AILab FlashDecoding Split-K** 并行注意力核心算子，基于原生 8-lane SIMD `@dot` 与硬件 `@fma` 实现：

- **常量与数据模型**：
  - `FLASH_DECODE_MAX_HEAD_DIM = 256`、`FLASH_DECODE_MAX_SPLITS = 64`
  - `FlashDecodePartial`：单分区在线 Softmax 累加结果（含归一化部分输出 `partial_out: [256]f32`、精确 `lse = max_logit + ln(sum_exp)`、`max_logit`、`sum_exp`、`head_dim`、`num_kv`、`valid`；提供 `empty()`、`isValid()`、`outputSlice()`）。
  - `FlashDecodeMergeStats`：归约诊断摘要（`global_lse`、`active_partitions`、`skipped_empty`、`total_kv`）。
- **核心 Split-K 与归约算子**：
  - `flashDecodePartition(q, k_part, v_part, num_kv, head_dim, scale, *out_acc): bool`：对单个 KV 分区独立执行在线 Softmax 注意力（`scale <= 0.0` 默认 `1 / sqrt(head_dim)`），输出 `partial_out` 与精确 `lse`；当 `num_kv == 0` 时输出安全空分区（`valid = false`）供后续归约自动跳过。
  - `mergeFlashDecodePartialsWithStats(partials, head_dim, *out_vec): FlashDecodeMergeStats?` / `mergeFlashDecodePartials(partials, head_dim, *out_vec): bool`：基于全局最大 LSE 偏移的无溢出 Log-Sum-Exp 归约（权重 `exp(lse_s - global_lse) ∈ (0, 1]`），自动跳过空分区，支持任意切分点与任意分区顺序，与全量单遍 SDPA 数学等价（误差 `< 1e-5`）。
  - `flashDecodeSplitK(q, k_all, v_all, num_kv, head_dim, num_splits, scale, *out_vec): bool`：端到端 Split-K 解码入口，将 KV 序列均匀划分为 `num_splits` 个分区（支持 `num_splits > num_kv`）并无损合并。
  - `flashDecodeSplitKGqa(q_heads, k_heads, v_heads, num_q_heads, num_kv_heads, num_kv, head_dim, num_splits, scale, *out_heads): bool`：多 Query 头 Grouped-Query Attention (GQA) 批量 Split-K 解码归约。

```cp
import {
    FlashDecodePartial,
    flashDecodePartition,
    mergeFlashDecodePartials,
    flashDecodeSplitK,
} from "cap:attention";

pub fn demoFlashDecode(q: []const f32, k_all: []const f32, v_all: []const f32, num_kv: usize, head_dim: usize) {
    // 方式 1：手动 Split-K 分区 + LSE 无损归约
    const mid: usize = num_kv / 2;
    let parts: [2]FlashDecodePartial = [FlashDecodePartial.empty(); 2];
    * = flashDecodePartition(q, k_all[0 .. mid * head_dim], v_all[0 .. mid * head_dim], mid, head_dim, 0.0, *parts[0]);
    * = flashDecodePartition(q, k_all[mid * head_dim .. num_kv * head_dim], v_all[mid * head_dim .. num_kv * head_dim], num_kv - mid, head_dim, 0.0, *parts[1]);

    let out_a: [64]f32 = [0.0; 64];
    let out_a_s: []f32 = out_a[0..head_dim];
    * = mergeFlashDecodePartials(parts[0..], head_dim, *out_a_s);

    // 方式 2：端到端 4 路 Split-K 解码
    let out_b: [64]f32 = [0.0; 64];
    let out_b_s: []f32 = out_b[0..head_dim];
    * = flashDecodeSplitK(q, k_all, v_all, num_kv, head_dim, 4, 0.0, *out_b_s);
}
```


### 零分配束搜索与多样性束解码引擎 (`beamSearch.cp`)

面向大模型高质量序列生成的 100% 纯 CP 零动态堆分配 **Beam Search & Diverse Beam Search (Vijayakumar et al. 2016)** 解码引擎：

- **常量与数据模型**：
  - `BEAM_MAX_WIDTH = 16`、`BEAM_MAX_SEQ_LEN = 128`、`BEAM_MAX_GROUPS = 8`、`BEAM_MAX_FINISHED = 32`
  - `BeamCandidate`：单步候选 `(token_id: u32, log_prob: f32)`。
  - `BeamSearchConfig`：束解码配置（`beam_width`, `max_seq_len`, `num_groups`, `diversity_penalty`, `length_penalty_alpha`, `no_repeat_ngram_size`, `early_stopping`, `eos_token_id`, `min_len`；提供 `defaults()` 与 `isValid()`）。
  - `BeamHypothesis`：定长序列假设（`tokens: [128]u32`, `len`, `log_prob`, `normalized_score`, `group_id`, `finished`；提供 `empty()` 与 `tokenSlice()`）。
  - `BeamStepOutcome` / `BeamSearchStats`：单步扩展结果与全链路诊断统计快照（步数、活跃束数、已完成假设数、拦截重复 N-gram 次数、多样性惩罚触发次数、早停/达到最大长度状态）。
- **核心评分、拦截与解码 API**：
  - `beamLengthPenalty(seq_len, alpha): f32` / `beamNormalizedScore(log_prob, seq_len, alpha): f32`：Google NMT 长度归一化 `((5 + len) / 6)^alpha` 与归一化得分。
  - `beamHasRepeatedNgram(tokens, next_token, ngram_size): bool`：精确 N-gram 禁止重复阻断检测（支持 unigram/bigram/trigram 及任意 N）。
  - `BeamSearchDecoder.init(cfg, prompt_tokens): BeamSearchDecoder?` / `reset(*_, prompt_tokens): bool`：零堆分配初始化与会话复用。
  - `step(*_, beam_candidates, candidates_per_beam, eos_token_id): BeamStepOutcome?`：从每条活跃束的 Top-K log-prob 候选中无堆选出下一轮活跃束，支持多组组间 Hamming 多样性惩罚、N-gram 阻断、`min_len` 护栏、EOS 自动归档与 `early_stopping` 收敛判定。
  - `stepFromLogits(*_, beam_logits, vocab_size, eos_token_id): BeamStepOutcome?`：直接输入稠密词表原始 logits，自动执行数值稳定 Log-Softmax + 栈上 Top-K 筛选并推进束搜索。
  - `bestHypothesis(): BeamHypothesis?` / `topHypotheses(*out_hyps): usize` / `finishedHypotheses(*out_hyps): usize`：按长度归一化得分降序提取最佳假设与 Top-N 假设。

```cp
import {
    BeamCandidate,
    BeamSearchConfig,
    BeamHypothesis,
    BeamSearchDecoder,
} from "cap:attention";

pub fn demoBeamSearch() {
    let cfg = BeamSearchConfig.defaults();
    cfg.beam_width = 4;
    cfg.num_groups = 2;
    cfg.diversity_penalty = 1.5;
    cfg.no_repeat_ngram_size = 3;
    cfg.eos_token_id = 2;

    const prompt: [1]u32 = [1];
    if (d := BeamSearchDecoder.init(cfg, prompt[0..])) {
        let decoder = d;
        const cands: [8]BeamCandidate = [
            BeamCandidate{ token_id: 10, log_prob: -0.1 },
            BeamCandidate{ token_id: 11, log_prob: -0.2 },
            BeamCandidate{ token_id: 12, log_prob: -0.3 },
            BeamCandidate{ token_id: 13, log_prob: -0.4 },
            BeamCandidate{ token_id: 10, log_prob: -0.1 },
            BeamCandidate{ token_id: 11, log_prob: -0.2 },
            BeamCandidate{ token_id: 12, log_prob: -0.3 },
            BeamCandidate{ token_id: 13, log_prob: -0.4 },
        ];
        * = decoder.step(cands[0..], 4, 2);
        let top: [4]BeamHypothesis = [BeamHypothesis.empty(); 4];
        let top_s: []BeamHypothesis = top[0..];
        * = decoder.topHypotheses(*top_s);
    }
}
```

---

### 零分配 LLM 分块 Prefill + 优先级抢占连续批调度器 (`batchSched.cp`)

面向 Sarathi / vLLM 风格高吞吐低尾延迟推理引擎的 100% 纯 CP 零动态堆分配连续批调度器（Token 预算 + KV 块池联合调度）：

- **常量与枚举**：
  - `BATCH_SCHED_MAX_REQS = 64`、`BATCH_SCHED_MAX_STEP_ITEMS = 64`
  - `BatchReqState`（`waiting | running | preempted | finished | aborted`）
  - `BatchExecPhase`（`prefill_chunk | decode`）
- **配置与数据模型**：
  - `BatchSchedConfig`（`max_batch_tokens`, `chunk_size`, `total_kv_blocks`, `kv_block_size`, `max_running_seqs`, `ttft_urgency_window_ms`；提供 `defaults()` 与 `isValid()`）
  - `BatchRequestSlot`（含 `isPrefillComplete()`, `committedTokens()`, `kv_blocks_held`, `preempt_count`, `first_token_ms`）
  - `BatchStepEntry` / `BatchStepPlan`（单步调度的 Decode/Prefill 项列表、总 Token 数、新分配 KV 块、被抢占 `preempted_ids` 与剩余空闲 KV 块）
  - `BatchSchedStats`（状态分布、KV 池使用/空闲块、累计步数/Prefill/Decode Token、抢占次数、完成数与平均 TTFT）
- **核心引擎 `ChunkedBatchScheduler`**：
  - `init(cfg)` / `reset(*_)` / `submitRequest(*_, req_id, prompt_tokens, max_output_tokens, priority, arrival_ms, ttft_deadline_ms)` / `abortRequest(*_, req_id)` / `reapTerminal(*_)`
  - `scheduleStep(*_, now_ms, *out_plan)`：三阶段联合调度——① 优先保障运行中 Decode（1 token/步），KV 块不足时按最低优先级 + 最晚到达原则抢占并归还 KV 块；② 推进在途分块 Prefill；③ 在剩余 Token 预算与并发槽位内优先唤醒 `preempted` 请求，再按 TTFT 截止期紧迫度 + 优先级 + 到达顺序接纳 `waiting` 请求并按 `chunk_size` 切片。
  - `commitStepResult(*_, req_id, tokens_processed, reached_eos, now_ms)`：推进 Prefill/Decode 进度、记录首次 Token 延迟（TTFT），并在达到 EOS 或 `max_output_tokens` 时自动释放全部 KV 块。

```cp
import {
    BatchSchedConfig,
    BatchStepPlan,
    ChunkedBatchScheduler,
} from "cap:attention";

pub fn demoChunkedBatchScheduler() {
    let cfg = BatchSchedConfig.defaults();
    cfg.max_batch_tokens = 256;
    cfg.chunk_size = 128;
    cfg.total_kv_blocks = 64;
    cfg.kv_block_size = 16;

    if (s := ChunkedBatchScheduler.init(cfg)) {
        let sched = s;
        * = sched.submitRequest(1001, 300, 32, 10, 1000, 2000);
        let plan = BatchStepPlan{};
        sched.scheduleStep(1050, *plan);
        // 300-token Prompt 按 chunk_size=128 自动切片，首步调度 128 tokens (8 KV blocks)
        * = sched.commitStepResult(1001, 128, false, 1100);
        * = sched.stats();
    }
}
```

---

### 零分配 LLM 前缀缓存感知多副本推理路由器 (`cacheAware.cp`)

面向 SGLang Router / Mooncake 风格分布式 LLM 推理网关的 100% 纯 CP 零动态堆分配前缀亲和度 + 负载联合路由器：

- **常量与枚举**：
  - `CACHE_ROUTE_MAX_REPLICAS = 16`、`CACHE_ROUTE_LRU_BLOCKS = 64`（每副本内联 64 槽近似前缀块 LRU 缓存）、`CACHE_ROUTE_BPS_SCALE = 10000`
  - `CacheRouteStatus`（`routed | no_healthy_replica | all_replicas_saturated | invalid_input`）
- **配置与数据模型**：
  - `CacheRouteConfig`（`cache_affinity_weight`, `kv_load_weight`, `queue_load_weight`, `overload_kv_bps`, `overload_penalty`；提供 `defaults()` 与 `isValid()`）
  - `ReplicaBlockSlot` / `ReplicaNodeState`（`isAvailable()`, `queueLoadRatio()`, `kvLoadRatio()`, `hasBlock`, `matchedPrefixBlocks`, `touchBlock`）
  - `CacheRouteDecision`（`status`, `replica_id`, `matched_blocks`, `total_blocks`, `hit_ratio`, `score`, `diverted_by_overload`, `hotspot_fallback`）
  - `CacheRouterStats`（集群健康副本数、总路由数、缓存命中路由数、累计命中块数、过载分流次数、热点回退次数与拒绝次数）
- **核心引擎 `CacheAwareRouter`**：
  - `init()` / `initWithConfig(config)` / `registerReplica(*_, replica_id, max_requests)` / `removeReplica` / `setReplicaHealth` / `updateReplicaLoad` / `completeRequest(*_, replica_id, new_kv_util_bps)`
  - `routePrompt(*_, prompt_block_hashes, now_ms): CacheRouteDecision`：联合最长连续共享前缀命中率、KV 缓存余量与队列余量打分，自动对 `kv_util_bps >= overload_kv_bps` 的热点副本施加降权惩罚分流防雪崩，选中后自动刷新副本 LRU 前缀块并递增 `active_requests`。

```cp
import { CacheAwareRouter } from "cap:attention";

pub fn demoCacheAwareRouter() {
    let router = CacheAwareRouter.init();
    * = router.registerReplica(1, 32);
    * = router.registerReplica(2, 32);

    const sys_prompt: [3]u64 = [0x101, 0x102, 0x103];
    const r1 = router.routePrompt(sys_prompt[0..], 1000); // 写入副本 1 前缀缓存
    * = router.completeRequest(r1.replica_id, 2000);

    // 后续同前缀请求自动路由回副本 1 (matched_blocks == 3)
    const follow_up: [4]u64 = [0x101, 0x102, 0x103, 0x204];
    const r2 = router.routePrompt(follow_up[0..], 2000);
    * = (r2.replica_id == r1.replica_id && r2.matched_blocks == 3);
}
```

---

### 零分配多模态/长序列二分软匹配 Token 合并引擎 (`tokenMerge.cp`)

面向 ViT / 多模态 VLM 与长上下文 Transformer 推理加速的 100% 纯 CP 零动态堆分配 ToMe (Token Merging) 引擎：

- **常量与枚举**：
  - `TOME_MAX_TOKENS = 256`、`TOME_MAX_DIM = 64`、`TOME_PERMILLE_SCALE = 1000`
  - `TokenMergeStatus`（`ok | invalid_input | capacity_exceeded | buffer_too_small`）
- **配置与报告**：
  - `TokenMergeConfig`（`r`, `protect_prefix`, `min_similarity`；`make(...)`, `isValid()`）
  - `TokenMergeReport`（`status`, `input_tokens`, `output_tokens`, `merged_pairs`, `protected_tokens`, `compression_permille`, `avg_merge_sim`, `min_merge_sim`）
- **核心算子**：
  - `mergeTokensF32(tokens, sizes, seq_len, dim, cfg, out_tokens, out_sizes, out_merge_map): TokenMergeReport`：保留 `protect_prefix` 个系统/CLS 前缀 Token，对剩余序列做偶/奇二分软匹配（原生 SIMD `@dot` 余弦相似度），贪心选取 Top-`r`（满足 `>= min_similarity`）冗余对执行加权平均融合并累加权重 `s_new = sum s_i`。
  - `computeProportionalLogBias(sizes, out_bias): bool`：计算 Proportional Attention 偏置 `log(size)` 供 SDPA 保持合并后注意力质量。
  - `unmergeTokensF32(merged_tokens, merge_map, orig_seq_len, merged_seq_len, dim, out_tokens)`：按 `merge_map` 零堆投影还原至原序列长度。

```cp
import {
    TokenMergeConfig,
    mergeTokensF32,
    computeProportionalLogBias,
    unmergeTokensF32,
} from "cap:attention";

pub fn demoTokenMerge() {
    // 5 tokens (dim=2): token 0 is protected CLS; tokens (1,2) and (3,4) are near-duplicates
    const toks: [10]f32 = [
        0.0, 1.0,  // CLS
        1.0, 0.0,  1.0, 0.05,
        0.0, -1.0, 0.05, -1.0,
    ];
    const empty_sizes: [0]f32 = [];
    let out_toks: [10]f32 = [0.0; 10];
    let out_sizes: [5]f32 = [0.0; 5];
    let merge_map: [5]usize = [0; 5];
    let ot_s: []f32 = out_toks[0..];
    let os_s: []f32 = out_sizes[0..];
    let mm_s: []usize = merge_map[0..];

    const rep = mergeTokensF32(toks[0..], empty_sizes[0..], 5, 2, TokenMergeConfig.make(2, 1, 0.9), *ot_s, *os_s, *mm_s);
    let bias: [5]f32 = [0.0; 5];
    let b_s: []f32 = bias[0..];
    * = computeProportionalLogBias(out_sizes[0..rep.output_tokens], *b_s);

    let restored: [10]f32 = [0.0; 10];
    let rs_s: []f32 = restored[0..];
    * = unmergeTokensF32(out_toks[0 .. rep.output_tokens * 2], merge_map[0..], 5, rep.output_tokens, 2, *rs_s);
}
```
