# CP 标准库：本地向量检索与混合 RAG 搜索引擎手册 (`cap:vector` / `cap.vector`)

## 一、 架构总览与设计准则

`cap:vector` 是 CP 语言官方内置的纯原生本地语义向量检索与混合 RAG（Retrieval-Augmented Generation）搜索引擎库，专为百万级 AI Agent 本地记忆检索、知识库召回与混合重排场景打造：

- **稠密向量近邻索引 (`VectorIndex`)**：基于 `cap:math` SIMD 加速算子（`cosineSimilarityF32`、`dotF32`、`euclideanDistanceF32`），支持 `Cosine` / `Dot` / `Euclidean` 三大度量、`upsert` 原位更新、`O(1)` Swap-Remove 删除与 `min_score` 阈值过滤。
- **稀疏全文倒排索引 (`Bm25Index`)**：内置 ASCII 小写字母数字分词器、英语停用词跳过与词项去重统计，精确实现 Okapi BM25 词频饱和与平滑 IDF 评分，支持文档级更新与删除后倒排表自动清理。
- **混合重排器 (`RrfFusion`)**：实现工业标准 Reciprocal Rank Fusion（`score = sum(1.0 / (rrf_k + rank))`），自动去重并提权双路命中文档。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:vector"` 与 `import cap from "cap"; cap.vector.*` 编译期自动动态挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/vector/flat_index.cp` | `// Source: cp/lib/vector/flat_index.cp (In-Memory Vector Index with Cosine/Dot/L2 Metrics)` | `DistanceMetric`、`SearchHit`、`VectorError`、`VectorIndex` |
| `lib/vector/bm25.cp` | `// Source: cp/lib/vector/bm25.cp (Okapi BM25 Full-Text Inverted Index)` | `Bm25Config`、`Bm25Index` 倒排索引与分词器 |
| `lib/vector/hybrid.cp` | `// Origin: Pure-CP Stdlib (RAG Hybrid Retrieval: RRF, MMR Rerank & Int8 Scalar Quantization) \| Created: 2026-11-22` | `RrfFusion` + 零堆分配多路 RRF (`rrfFuse`)、线性归一化融合 (`normalizeScoresMinMax`/`normalizeScoresZScore`/`weightedScoreFuse`)、MMR 多样性重排 (`mmrRerank`) 与 U8 标量量化 (`quantizeVectorU8`/`dequantizeVectorU8`/`quantizedDotU8`) |
| `lib/vector/semantic_cache.cp` | `// Origin: Pure-CP Stdlib (AI Agent Semantic Cache: Cosine Threshold Lookup, TTL & LRU Eviction) \| Created: 2026-11-22` | `SemanticCache` 零堆分配 LLM 语义缓存（余弦阈值命中、TTL 过期与过期优先 + LRU 淘汰） |
| `lib/vector/binary_quant.cp` | `// Origin: Pure-CP Stdlib (RAG 1-Bit Binary Vector Quantization, Hamming Search & Two-Stage Rescore) \| Created: 2026-11-22` | 1-bit 二值量化（32x 压缩）、`@popCount` 汉明距离、Charikar 余弦估计与两阶段粗排+全精度精排 (`twoStageSearch`) |
| `lib/vector/ivf.cp` | `// Origin: Pure-CP Stdlib (RAG IVF-Flat Coarse-Quantized Inverted File ANN Index) \| Created: 2026-11-22` | `IvfFlatIndex` 零堆分配 CSR 倒排文件近似最近邻索引（L2 / Cosine 多探针 `nprobe` 亚线性检索） |
| `lib/vector/pq.cp` | `// Origin: Pure-CP Stdlib (RAG Product Quantization & Asymmetric Distance Computation ADC) \| Created: 2026-11-22` | `PqCodebook` 乘积量化编码/解码与 `O(m)` 查表非对称距离（ADC）Top-K 检索 |
| `lib/vector/reranker.cp` | `// Origin: Pure-CP Stdlib (AI RAG Reranker: MMR, ColBERT MaxSim & IR Metrics) \| Created: 2026-11-22` | 零辅助缓冲 MMR (`mmrSelect`)、ColBERT Late-Interaction (`colbertMaxSim`)、原位 MinMax/ZScore 归一化与 `NDCG@K` / `MRR` / `Recall@K` IR 评测 |
| `lib/vector/filter.cp` | `// Origin: Pure-CP Stdlib (AI Vector Search Metadata Bitset Filter & Selectivity Planner) \| Created: 2026-11-22` | 1024-bit 零堆分配位图 (`FilterBitset`)、数值/范围元数据谓词过滤、选择度自适应 Filtered ANN 路由规划器与位跳跃掩码 Top-K 余弦检索 |
| `lib/vector/simd_ops.cp` | `// Origin: Pure-CP Stdlib (AI-Native SIMD Vector Distance & Batch Retrieval Kernels using @dot/@l2sq/@fma) \| Created: 2026-11-22` | 原生 8-lane SIMD 内建原语 (`@dot`/`@l2sq`/`@fma`) 驱动的向量距离、单舍入 AXPY 与零堆分配批量 Top-K Cosine / L2 检索核 |
| `lib/vector/kmeans.cp` | `// Origin: Pure-CP Stdlib (AI-Native SIMD K-Means++ Clustering & IVF/PQ Codebook Trainer) \| Created: 2026-11-22` | 基于 `@l2sq`/`@fma` 原生 SIMD 与确定性 SplitMix64 的零堆分配 K-Means++ 聚类与 IVF/PQ 码本训练器 |
| `lib/vector/index.cp` | `// Source: cp/lib/vector/index.cp (Unified Exports for cap:vector)` | `cap:vector` 与 `cap.vector` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 内存稠密向量索引 (`flat_index.cp`)

- **`VectorError`**：`DimensionMismatch`、`InvalidDimension`、`OutOfMemory`
- **`DistanceMetric`**：`Cosine` | `Dot` | `Euclidean`
- **`SearchHit`**：`{ id: string, score: f32, meta: string }`
  - `id` 与 `meta` 切片借用自底层索引（或输入列表）；调用方只需对 `search`/`fuse` 返回的 `[]SearchHit` 切片执行 `alloc.free(hits)`。
- **`VectorIndex`**：
  - `VectorIndex.init(alloc, dim, metric): VectorIndex ! VectorError`（`dim == 0` 返回 `VectorError.InvalidDimension`）
  - `upsert(*_, id: string, vec: []const f32, meta: string = ""): void ! VectorError`（维度不匹配返回 `VectorError.DimensionMismatch`；已存在 `id` 原位覆盖向量与元数据）
  - `remove(*_, id: string): bool`（`O(1)` Swap-Remove 删除，返回是否存在并移除）
  - `len(): usize`
  - `search(alloc, query: []const f32, top_k: usize, min_score: f32 = -3.4028234e38): ([]SearchHit) ! VectorError`
    - 按相关性降序返回最多 `top_k` 条 `score >= min_score` 的结果。
    - `Euclidean` 模式下将欧氏距离 `d` 单调映射为相似度分数 `1.0 / (1.0 + d)`（`d = 0` 得 `1.0`），确保高分代表高相关性。
  - `deinit(*_)`

### 2. Okapi BM25 倒排索引 (`bm25.cp`)

- **`Bm25Config`**：`{ k1: f64 = 1.5, b: f64 = 0.75, use_stopwords: bool = true }`
- **`Bm25Index`**：
  - `Bm25Index.init(alloc, config: Bm25Config = Bm25Config{}): Bm25Index`
  - `addDocument(*_, id: string, text: string): !`（使用 ASCII 小写字母数字分词器，跳过停用词并去重统计词频；同 `id` 再次写入自动覆盖更新）
  - `removeDocument(*_, id: string): bool`（移除文档及其倒排记录并清理空倒排链）
  - `docCount(): usize`
  - `search(alloc, query: string, top_k: usize): ([]SearchHit)!`（计算精确 Okapi BM25 得分并按降序返回 `[]SearchHit`）
  - `deinit(*_)`

### 3. Reciprocal Rank Fusion 混合重排与零堆分配 RAG 原语 (`hybrid.cp`)

#### 3.1 Allocator 版字符串 ID 融合 (`RrfFusion`)
- **`RrfFusion.fuse(alloc, vector_hits, text_hits, rrf_k: f32 = 60.0, top_k: usize = 10): ([]SearchHit)!`**
  - 采用 1-based 排名（`rank = 1, 2, ...`）累加融合分数：`score(doc) = sum(1.0 / (rrf_k + rank))`。
  - 自动按 `id` 去重（单路内部重复亦仅计首次最高排名），保留非空 `meta`（优先向量路元信息），按融合分数降序输出最多 `top_k` 条结果。

#### 3.2 零堆分配多路 RRF 融合
- **`RankedDoc`**：`{ doc_id: u32 = 0, score: f64 = 0.0 }`
- **`rrfFuse(lists_doc_ids: []const u32, list_offsets: []const usize, k_const: f64, out_docs: *[]RankedDoc, work_ids: *[]u32, work_scores: *[]f64): usize?`**
  - 支持任意 `M` 路召回列表（CSR 偏移 `list_offsets`，长度 `M + 1`），例如 BM25 + Dense + Sparse 三路融合。
  - 按 $\sum_m \frac{1}{k + \text{rank}_m(d)}$（1-based 排名，单路内部重复只计首次最高排名）融合去重，按 `score` 降序、同分按 `doc_id` 升序确定性输出到 `out_docs`。
  - 零堆分配，所有输出与工作区缓冲由调用方提供；参数非法（负/`NaN` `k_const`、非法偏移、缓冲不足）返回 `null`。

#### 3.3 分数归一化与线性加权融合
- **`normalizeScoresMinMax(scores: []const f64, out: *[]f64): bool`**
  - 将任意有限浮点分数线性映射到 `[0.0, 1.0]`（常数输入映射为 `0.0`）；含 `NaN`/`Inf` 或输出缓冲不足返回 `false`。
- **`normalizeScoresZScore(scores: []const f64, out: *[]f64): bool`**
  - 零均值单位方差标准化（常数输入映射为 `0.0`）；含 `NaN`/`Inf` 或输出缓冲不足返回 `false`。
- **`weightedScoreFuse(dense_docs: []const RankedDoc, sparse_docs: []const RankedDoc, alpha: f64, out_docs: *[]RankedDoc): usize?`**
  - 计算 `alpha * dense + (1.0 - alpha) * sparse`，自动去重合并双路文档并按得分降序（同分 `doc_id` 升序）写入 `out_docs`。`alpha` 超出 `[0.0, 1.0]`、含非有限分数或缓冲不足返回 `null`。

#### 3.4 Maximal Marginal Relevance 多样性去冗余重排 (`mmrRerank`)
- **`mmrRerank(query_sims: []const f64, embeddings: []const f64, n_docs: usize, dim: usize, top_k: usize, lambda: f64, out_indices: *[]usize, selected_mask: *[]u8): usize?`**
  - 贪心迭代选择 $\arg\max_{d \notin S} [\lambda \cdot \text{sim}(q, d) - (1 - \lambda) \max_{s \in S} \cos(d, s)]$，在 AI Agent RAG 上下文拼装阶段自动剔除近重复片段、保留多主题覆盖。
  - `lambda = 1.0` 时退化为纯相关性排序；`lambda = 0.5` 兼顾相关性与多样性；`selected_mask` 内部自动清零。

#### 3.5 U8/Int8 非对称标量量化与快速量化内积
- **`ScalarQuantParams`**：`{ min_val: f64 = 0.0, scale: f64 = 0.0, inv_scale: f64 = 0.0 }`
- **`quantizeVectorU8(vec: []const f64, q_out: *[]u8): ScalarQuantParams?`**
  - 将浮点向量非对称均匀量化到 `[0, 255]`（`f32` 4x / `f64` 8x 压缩），常数向量安全返回 `scale = 0.0`。
- **`dequantizeVectorU8(q: []const u8, params: ScalarQuantParams, out: *[]f64): bool`**
  - 按 `min_val + q[i] * scale` 重构浮点向量，典型 128D 嵌入相对 L2 重构误差 `< 1%`。
- **`quantizedDotU8(qa: []const u8, pa: ScalarQuantParams, qb: []const u8, pb: ScalarQuantParams): f64?`**
  - 基于纯整型内循环累计 `sum(qa)`、`sum(qb)`、`sum(qa * qb)` 直接计算近似内积，避免逐维反量化开销且数值等价于反量化内积。

### 4. 零堆分配 LLM 语义缓存 (`semantic_cache.cp`)

- **`SemanticEntry`**：`{ occupied, key_id, payload_id, created_ms, last_hit_ms, expires_ms, hit_count, norm }`
- **`SemanticLookupHit`**：`{ slot: usize, key_id: u64, payload_id: u64, similarity: f64 }`
- **`SemanticCacheStats`**：`{ active_entries, capacity, hits, misses, evictions, expired_evictions, hit_rate }`
- **`SemanticCache`**：
  - `SemanticCache.init(entries: *[]SemanticEntry, embeddings_flat: *[]f64, dim: usize, similarity_threshold: f64, default_ttl_ms: i64): SemanticCache?`（校验 `dim > 0`、`embeddings_flat.len >= entries.len * dim`、`similarity_threshold ∈ (-1.0, 1.0]` 与 `default_ttl_ms >= 0`）
  - `put(*_, key_id, payload_id, embedding, now_ms, ttl_ms_override): usize?`（先原地更新同 `key_id` 槽位；满池优先驱逐已过期条目，再按 LRU 最久未命中 `last_hit_ms` 淘汰；自动拒绝零向量与 `NaN`/`Inf`）
  - `lookup(*_, query_embedding, now_ms): SemanticLookupHit?`（跳过过期条目，返回最高余弦相似度 `>= similarity_threshold` 的命中项并原子刷新 `last_hit_ms` 与 `hit_count`）
  - `invalidate(*_, key_id: u64): bool` / `purgeExpired(*_, now_ms: i64): usize` / `stats(): SemanticCacheStats`

### 5. 1-Bit 二值向量量化与两阶段粗排+精排检索 (`binary_quant.cp`)

- **`BinaryCandidate`**：`{ index: usize, hamming: u32 }`
- **`RescoredHit`**：`{ index: usize, cosine: f64, hamming: u32 }`
- **`binaryWordsForDim(dim: usize): usize`**：返回打包 `dim` 位所需 `u64` 字数 `ceil(dim / 64)`。
- **`quantizeBinary(vec: []const f64, threshold: f64, words_out: *[]u64): bool`**：1-bit 符号量化（`vec[i] >= threshold` 置位 1，`f32` 32x / `f64` 64x 压缩），校验无 `NaN`/`Inf` 且输出缓冲足够。
- **`hammingDistanceWords(a: []const u64, b: []const u64): u32`**：基于硬件 `@popCount` 的极速位异或汉明距离。
- **`binaryCosineApprox(hamming: u32, dim: usize): f64`**：Charikar 无偏余弦估计 $\cos(\pi \cdot \text{hamming} / d)$。
- **`searchBinaryTopK(query_words, corpus_words, n_docs, words_per_vec, top_k, out_candidates): usize?`**：基于堆的零堆分配 1-bit 汉明 Top-K 召回（按 `hamming` 升序、同分按 `index` 升序）。
- **`twoStageSearch(query, query_words, corpus_words, corpus_fp, n_docs, dim, oversample_m, top_k, coarse_scratch, out_hits): usize?`**：两阶段粗排（1-bit Hamming Top-M）+ 精排（全精度 Cosine Top-K），零堆分配支持超大规模向量语料。

### 6. 倒排文件近似最近邻索引 (`ivf.cp`)

- **`IvfHit`**：`{ doc_id: usize, dist_sq: f64, cluster_id: usize }`（`searchCosine` 中 `dist_sq` 存储余弦距离 `1 - cos(q, x)`）
- **`IvfBuildStats`**：`{ n_clusters: usize, n_docs: usize, min_cluster_size: usize, max_cluster_size: usize }`
- **`IvfFlatIndex`**（借用调用方 `centroids`、`list_offsets`、`list_doc_ids`）：
  - `IvfFlatIndex.buildFromCentroids(corpus, n_docs, dim, centroids, n_clusters, list_offsets_out, list_doc_ids_out, counts_scratch): IvfBuildStats?`（单遍计数 + 前缀和 + 写入，零堆分配构建紧凑 CSR 倒排表）
  - `IvfFlatIndex.searchL2(index, corpus, query, nprobe, top_k, probe_ids_s, probe_dists_s, out_hits): usize?`（先选 `nprobe` 个最近质心，再扫描对应倒排桶返回 L2 平方距离升序 Top-K）
  - `IvfFlatIndex.searchCosine(index, corpus, query, nprobe, top_k, probe_ids_s, probe_dists_s, out_hits): usize?`（按余弦距离 `1 - cos(q, x)` 多探针检索 Top-K）
  - `IvfFlatIndex.clusterSize(index, cluster_id): usize`（查询指定倒排桶大小，越界安全返回 `0`）

### 7. 乘积量化与非对称距离查找表 (`pq.cp`)

- **`PqCodebook`**：`{ dim, m, sub_dim, ks, centroids }`，提供 `PqCodebook.init(dim, m, ks, centroids): PqCodebook?`（校验 `dim % m == 0`、`1 <= ks <= 256` 及有限质心）
- **`PqHit`**：`{ doc_id: usize, dist_sq: f64 }`
- **`pqEncodeVector(cb, vec, code_out: *[]u8): bool`**：将 `dim` 维向量切分为 `m` 个子空间编码为 `m` 字节 `u8` 码字
- **`pqDecodeVector(cb, code, vec_out: *[]f64): bool`**：从 `m` 字节码字重建近似向量（越界码字返回 `false`）
- **`pqBuildAdcTable(cb, query, lut_out: *[]f64): bool`**：预计算 `m * ks` 子空间平方距离查找表 `lut[s * ks + c]`
- **`pqAdcDistanceSq(lut, code, m, ks): f64`**：纯 `m` 次查表累加（零浮点乘法），与 `L2Sq(query, decode(code))` 精确等价
- **`pqSearchAdcTopK(cb, codes_flat, n_docs, query, top_k, lut_scratch, out_hits): usize?`**：`O(n * m)` 极速 ADC 检索，按 `(dist_sq 升序, doc_id 升序)` 输出 Top-K

### 8. RAG 检索重排与信息检索评测引擎 (`reranker.cp`)

- **`mmrSelect(query_sims, doc_embeddings, n_docs, dim, k, lambda, out_indices: *[]usize): usize?`**：无需额外 visited mask 的零堆分配 Maximal Marginal Relevance 多样性选择（`lambda ∈ [0, 1]` 平衡相关性与去重多样性，同分按文档下标升序）
- **`colbertMaxSim(query_tokens, q_len, doc_tokens, d_len, dim): f64?`**：ColBERT Late-Interaction 逐 query token 最大余弦相似度求和（单位化嵌入上精确等价点积 MaxSim）
- **`minMaxNormalizeInPlace(scores: *[]f64): bool` / `zScoreNormalizeInPlace(scores: *[]f64): bool`**：原位多路检索分数归一化（零方差/常数序列安全归零，不触发除零 NaN）
- **`ndcgAtK(relevances, k): f64?`**：零堆分配 `O(1)` 栈空间精确计算 `NDCG@K`（标准 $2^{\text{rel}} - 1$ 增益，完整处理重复相关性等级）
- **`mrrScore(ranked_ids, relevant_ids): f64?` / `recallAtK(ranked_ids, relevant_ids, k): f64?`**：生产级 RAG 召回与排序质量评测指标（`recallAtK` 自动对 ground-truth 去重防止 Recall > 1.0）

### 9. 元数据位图过滤与选择度自适应路由引擎 (`filter.cp`)

- **`FILTER_BITSET_WORDS` (`16`) / `FILTER_BITSET_MAX_BITS` (`1024`) / `FilterBitset`**：固定 1024-bit（16×u64）零堆分配位图，提供 `empty`/`full`/`set`/`clear`/`testBit`/`popcount`（硬件 `@popCount`）/`andWith`/`orWith`/`notInPlace`/`andNotWith`/`firstSetBit`（硬件 `@ctz`）
- **`FilterOp` (`Eq | Neq | Gt | Gte | Lt | Lte | Range`) / `NumericFilter`**：数值与闭区间范围谓词描述符
- **`evalInt64Filter(values, filter, *out_mask): usize?` / `evalFloat64Filter(values, filter, *out_mask): usize?`**：单遍将数值列谓词编译为 `FilterBitset` 位图并返回命中数
- **`FilterStrategy` (`PreFilterBrute | InlineGraphFilter | PostFilterExpand`) / `FilterPlan` / `planFilterStrategy(total_docs, matching_docs, top_k): FilterPlan?`**：基于选择度 `selectivity = matching / total` 与 `top_k` 自动规划最优过滤执行路径与 `oversample_factor`
- **`maskedTopKCosine(query, corpus_flat, n_docs, dim, mask, k, *out_ids, *out_scores): usize?`**：使用 `@ctz` 位跳跃仅扫描位图置位子集的零堆分配 Top-K 余弦检索

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { DistanceMetric, VectorIndex, Bm25Index, RrfFusion } from "cap:vector";

pub fn demoHybridRag(): !void {
    const alloc = std.heap.page_allocator;

    let vec_idx = VectorIndex.init(alloc, 3, .Cosine)?;
    defer vec_idx.deinit();
    vec_idx.upsert("doc1", &[1.0, 0.0, 0.0], "{\"src\":\"kb\"}")?;
    vec_idx.upsert("doc2", &[0.0, 1.0, 0.0], "{\"src\":\"log\"}")?;

    let bm25 = Bm25Index.init(alloc);
    defer bm25.deinit();
    bm25.addDocument("doc1", "rust zig memory safety")?;
    bm25.addDocument("doc2", "vector search rag hybrid")?;

    const v_hits = vec_idx.search(alloc, &[0.9, 0.1, 0.0], 5, 0.0)?;
    defer alloc.free(v_hits);

    const t_hits = bm25.search(alloc, "vector memory", 5)?;
    defer alloc.free(t_hits);

    const fused = RrfFusion.fuse(alloc, v_hits, t_hits, 60.0, 5)?;
    defer alloc.free(fused);

    // 等价动态懒加载挂载用法：
    const fused2 = cap.vector.RrfFusion.fuse(alloc, v_hits, t_hits, 60.0, 5)?;
    defer alloc.free(fused2);
}
```

### 零堆分配 RAG 混合重排与标量量化示例

```typescript
import {
    RankedDoc,
    rrfFuse,
    mmrRerank,
    quantizeVectorU8,
    quantizedDotU8,
} from "cap:vector";

pub fn demoZeroHeapRagPipeline() {
    // 1. 3 路召回（BM25 + Dense + Sparse）零堆分配 RRF 融合
    const lists_ids: [9]u32 = [100, 42, 200, 300, 42, 100, 42, 400, 300];
    const offsets: [4]usize = [0, 3, 6, 9];
    let out_docs_buf: [8]RankedDoc = [RankedDoc{}; 8];
    let w_ids_buf: [8]u32 = [0; 8];
    let w_scores_buf: [8]f64 = [0.0; 8];
    let out_docs: []RankedDoc = out_docs_buf[0..];
    let w_ids: []u32 = w_ids_buf[0..];
    let w_scores: []f64 = w_scores_buf[0..];
    const n_fused = rrfFuse(lists_ids[0..], offsets[0..], 60.0, out_docs, w_ids, w_scores) ?? 0;

    // 2. MMR 多样性重排（lambda = 0.5 去冗余）
    const q_sims: [4]f64 = [0.95, 0.94, 0.85, 0.80];
    const embs: [12]f64 = [
        1.0, 0.0, 0.0,
        0.999, 0.01, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    ];
    let sel_idx_buf: [3]usize = [0; 3];
    let mask_buf: [4]u8 = [0; 4];
    let sel_idx: []usize = sel_idx_buf[0..];
    let mask: []u8 = mask_buf[0..];
    const k = mmrRerank(q_sims[0..], embs[0..], 4, 3, 3, 0.5, sel_idx, mask) ?? 0;

    // 3. U8 非对称标量量化与快速内积
    const va: [4]f64 = [-0.5, 0.25, 0.75, 1.25];
    const vb: [4]f64 = [-0.4, 0.30, 0.80, 1.10];
    let qa_buf: [4]u8 = [0; 4];
    let qb_buf: [4]u8 = [0; 4];
    let qa: []u8 = qa_buf[0..];
    let qb: []u8 = qb_buf[0..];
    if (pa := quantizeVectorU8(va[0..], qa)) {
        if (pb := quantizeVectorU8(vb[0..], qb)) {
            const approx_dot = quantizedDotU8(qa_buf[0..], pa, qb_buf[0..], pb) ?? 0.0;
            * = approx_dot;
        }
    }
    * = n_fused;
    * = k;
}
```

### LLM 语义缓存（余弦阈值命中 + TTL + LRU）示例

```typescript
import {
    SemanticEntry,
    SemanticCache,
} from "cap:vector";

pub fn demoSemanticCache() {
    let entries_buf: [16]SemanticEntry = [SemanticEntry{}; 16];
    let embs_buf: [64]f64 = [0.0; 64]; // 16 slots * 4 dims
    let entries: []SemanticEntry = entries_buf[0..];
    let embs: []f64 = embs_buf[0..];

    if (cache_init := SemanticCache.init(entries, embs, 4, 0.92, 60000)) {
        let cache = cache_init;
        const prompt_emb: [4]f64 = [1.0, 0.0, 0.0, 0.0];
        * = cache.put(1001, 9001, prompt_emb[0..], 1000, 0);

        // 近义提问命中缓存，直接复用 payload_id = 9001 省去 LLM 推理开销
        const near_query: [4]f64 = [0.99, 0.05, 0.0, 0.0];
        if (hit := cache.lookup(near_query[0..], 2500)) {
            * = hit.payload_id;
        }
        * = cache.purgeExpired(70000);
        * = cache.stats();
    }
}
```

### 1-Bit 二值量化与两阶段粗排+精排示例 (`binary_quant.cp`)

```typescript
import {
    BinaryCandidate,
    RescoredHit,
    quantizeBinary,
    twoStageSearch,
} from "cap:vector";

pub fn demoTwoStageBinarySearch() {
    const query: [4]f64 = [0.9, -0.2, 0.5, -0.8];
    const corpus_fp: [12]f64 = [
        0.88, -0.18, 0.52, -0.79, // doc 0: closest
        -0.9, 0.2, -0.5, 0.8,     // doc 1: opposite
        0.5, 0.5, 0.5, -0.5,      // doc 2: partial match
    ];

    let q_words_buf: [1]u64 = [0];
    let c_words_buf: [3]u64 = [0; 3];
    let q_words: []u64 = q_words_buf[0..];
    * = quantizeBinary(query[0..], 0.0, q_words);

    let d: usize = 0;
    while (d < 3) : (d += 1) {
        let cw: []u64 = c_words_buf[d .. d + 1];
        * = quantizeBinary(corpus_fp[d * 4 .. (d + 1) * 4], 0.0, cw);
    }

    let scratch_buf: [2]BinaryCandidate = [BinaryCandidate{}; 2];
    let hits_buf: [2]RescoredHit = [RescoredHit{}; 2];
    let scratch: []BinaryCandidate = scratch_buf[0..];
    let hits: []RescoredHit = hits_buf[0..];
    const k = twoStageSearch(
        query[0..], q_words_buf[0..], c_words_buf[0..], corpus_fp[0..],
        3, 4, 2, 2, scratch, hits,
    ) ?? 0;
    * = k;
}
```

### IVF-Flat 多探针倒排索引检索示例 (`ivf.cp`)

```typescript
import {
    IvfHit,
    IvfFlatIndex,
} from "cap:vector";

pub fn demoIvfFlatSearch() {
    const centroids: [4]f64 = [0.0, 0.0, 10.0, 10.0];
    const corpus: [8]f64 = [0.1, 0.2, -0.1, 0.0, 9.8, 10.1, 10.2, 9.9];

    let offsets_buf: [3]usize = [0; 3];
    let doc_ids_buf: [4]usize = [0; 4];
    let counts_buf: [2]usize = [0; 2];
    let offsets: []usize = offsets_buf[0..];
    let doc_ids: []usize = doc_ids_buf[0..];
    let counts: []usize = counts_buf[0..];
    * = IvfFlatIndex.buildFromCentroids(corpus[0..], 4, 2, centroids[0..], 2, offsets, doc_ids, counts);

    const idx = IvfFlatIndex{
        centroids: centroids[0..],
        list_offsets: offsets,
        list_doc_ids: doc_ids,
        n_clusters: 2,
        dim: 2,
    };

    const query: [2]f64 = [9.9, 10.0];
    let p_ids_buf: [2]usize = [0; 2];
    let p_dists_buf: [2]f64 = [0.0; 2];
    let hits_buf: [2]IvfHit = [IvfHit{}; 2];
    let p_ids: []usize = p_ids_buf[0..];
    let p_dists: []f64 = p_dists_buf[0..];
    let hits: []IvfHit = hits_buf[0..];
    const n = IvfFlatIndex.searchL2(idx, corpus[0..], query[0..], 1, 2, p_ids, p_dists, hits) ?? 0;
    * = n;
}
```

### 乘积量化（PQ）与 ADC 查找表极速检索示例 (`pq.cp`)

```typescript
import {
    PqCodebook,
    PqHit,
    pqEncodeVector,
    pqSearchAdcTopK,
} from "cap:vector";

pub fn demoPqAdcSearch(centroids_128: []const f64, query_16: []const f64, vec_a: []const f64) {
    if (cb := PqCodebook.init(16, 4, 8, centroids_128)) {
        let codes_buf: [4]u8 = [0; 4];
        let code_slice: []u8 = codes_buf[0..];
        * = pqEncodeVector(cb, vec_a, code_slice);

        let lut_buf: [32]f64 = [0.0; 32];
        let hits_buf: [1]PqHit = [PqHit{}; 1];
        let lut: []f64 = lut_buf[0..];
        let hits: []PqHit = hits_buf[0..];
        const k = pqSearchAdcTopK(cb, codes_buf[0..], 1, query_16, 1, lut, hits) ?? 0;
        * = k;
    }
}
```

### RAG 重排（MMR / ColBERT MaxSim）与 IR 指标评测示例 (`reranker.cp`)

```typescript
import {
    mmrSelect,
    colbertMaxSim,
    ndcgAtK,
    mrrScore,
    recallAtK,
} from "cap:vector";

pub fn demoRerankerAndIrMetrics() {
    // 1. MMR 平衡相关性与多样性（无需额外 mask 缓冲）
    const q_sims: [3]f64 = [0.95, 0.94, 0.85];
    const embs: [6]f64 = [1.0, 0.0, 0.999, 0.01, 0.0, 1.0];
    let out_buf: [2]usize = [0; 2];
    let out_idx: []usize = out_buf[0..];
    * = mmrSelect(q_sims[0..], embs[0..], 3, 2, 2, 0.5, out_idx); // 选中 [0, 2]

    // 2. ColBERT Late-Interaction MaxSim
    const q_toks: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    const d_toks: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    const maxsim = colbertMaxSim(q_toks[0..], 2, d_toks[0..], 2, 2) ?? 0.0; // 2.0

    // 3. IR 评测指标
    const rels: [4]f64 = [3.0, 2.0, 1.0, 0.0];
    const ranked: [4]u32 = [10, 20, 30, 40];
    const truth: [2]u32 = [20, 40];
    * = ndcgAtK(rels[0..], 4);             // 1.0
    * = mrrScore(ranked[0..], truth[0..]); // 0.5
    * = recallAtK(ranked[0..], truth[0..], 2); // 0.5
    * = maxsim;
}
```

### 元数据位图过滤与选择度自适应 Filtered ANN 示例 (`filter.cp`)

```typescript
import {
    FilterBitset,
    FilterOp,
    NumericFilter,
    evalInt64Filter,
    planFilterStrategy,
    maskedTopKCosine,
} from "cap:vector";

pub fn demoMetadataBitsetFilter(corpus: []const f64, query: []const f64) {
    const years: [4]i64 = [2021, 2023, 2024, 2025];
    let mask = FilterBitset.empty();
    const matched = evalInt64Filter(
        years[0..],
        NumericFilter{ op: FilterOp.Range, i64_lo: 2023, i64_hi: 2025 },
        &mask,
    ) ?? 0;

    if (plan := planFilterStrategy(4, matched, 2)) {
        * = plan.strategy;
        let ids_buf: [2]usize = [0; 2];
        let scores_buf: [2]f64 = [0.0; 2];
        let ids: []usize = ids_buf[0..];
        let scores: []f64 = scores_buf[0..];
        * = maskedTopKCosine(query, corpus, 4, 2, mask, 2, ids, scores);
    }
}
```

### 10. 原生 SIMD 内建原语向量距离与批量 Top-K 检索 (`simd_ops.cp`)

直接基于 CP 编译器原生 8-lane SIMD 内建原语 `@dot`、`@l2sq`、`@fma` 的零堆分配向量算子与批量检索核：

- **`simdDotF32(a: []const f32, b: []const f32): f32?`** / **`simdDotF64(a: []const f64, b: []const f64): f64?`**：校验非空等长后直调 `@dot(a, b)`。
- **`simdL2SqF32(a: []const f32, b: []const f32): f32?`** / **`simdL2DistF32(a: []const f32, b: []const f32): f32?`**：基于 `@l2sq(a, b)` 计算平方欧氏距离与欧氏距离。
- **`simdCosineF32(a: []const f32, b: []const f32): f32?`**：基于 `@dot` 计算余弦相似度并钳位至 `[-1.0, 1.0]`，零向量安全返回 `0.0`。
- **`simdAxpyInPlaceF32(alpha: f32, x: []const f32, y: *[]f32): bool`**：原位单舍入硬件融合乘加 `y[i] = @fma(alpha, x[i], y[i])`。
- **`simdBatchTopKCosineF32(query, corpus_flat, n_docs, dim, k, out_ids: *[]usize, out_scores: *[]f32): usize?`**：零堆分配批量 SIMD 余弦 Top-K 降序检索（同分按 `doc_id` 升序）。
- **`simdBatchTopKL2F32(query, corpus_flat, n_docs, dim, k, out_ids: *[]usize, out_dists: *[]f32): usize?`**：零堆分配批量 SIMD L2 最近邻 Top-K 升序检索（排序阶段用 `@l2sq`，仅对最终 Top-K 胜出者执行 `@sqrt`）。

```typescript
import {
    simdCosineF32,
    simdAxpyInPlaceF32,
    simdBatchTopKCosineF32,
    simdBatchTopKL2F32,
} from "cap:vector";

pub fn demoSimdVectorOps() {
    const query: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const corpus: [12]f32 = [
        0.9, 0.1, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        1.0, 0.0, 0.0, 0.0,
    ];
    let ids: [2]usize = [0, 0];
    let scores: [2]f32 = [0.0, 0.0];
    let ids_s: []usize = ids[0..];
    let scores_s: []f32 = scores[0..];

    * = simdBatchTopKCosineF32(query[0..], corpus[0..], 3, 4, 2, ids_s, scores_s);
    * = simdBatchTopKL2F32(query[0..], corpus[0..], 3, 4, 2, ids_s, scores_s);

    let y: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    let y_s: []f32 = y[0..];
    * = simdAxpyInPlaceF32(0.5, query[0..], y_s);
    * = simdCosineF32(query[0..], y[0..]);
}
```

### 11. 原生 SIMD K-Means++ 向量聚类与码本训练 (`kmeans.cp`)

面向 IVF 粗量化质心与 PQ 子空间码本训练的零堆分配 SIMD K-Means++ 聚类器，全程调用原生 `@l2sq` 与 `@fma` 内建原语：

- **`KMeansConfig`**：`{ k: usize = 1, max_iters: u32 = 50, tol: f32 = 0.0001, seed: u64 = 42 }`
- **`KMeansResult`**：`{ iters: u32, inertia: f64, converged: bool, empty_reseeded: u32 }`
- **`kmeansInitPlusPlusF32(data, n, dim, k, seed, centroids: *[]f32, min_dists_buf: *[]f32): bool`**：确定性 SplitMix64 + `@l2sq` 距离加权 K-Means++ 初始质心采样。
- **`kmeansAssignF32(data, n, dim, centroids, k, assignments: *[]usize): f64?`**：基于 `@l2sq` 分配最近质心并返回总 inertia。
- **`kmeansFitF32(data, n, dim, cfg, centroids: *[]f32, assignments: *[]usize, counts_buf: *[]usize, scratch_dists: *[]f32): KMeansResult?`**：完整 Lloyd 迭代 + `@fma` 原位质心更新 + 空簇最远点重采样防退化 + 收敛早停。

```typescript
import { KMeansConfig, kmeansFitF32 } from "cap:vector";

pub fn demoSimdKMeans(data: []const f32, n: usize, dim: usize) {
    let centroids: [16]f32 = undefined; // k=4, dim=4
    let assignments: [64]usize = undefined;
    let counts: [4]usize = undefined;
    let scratch: [64]f32 = undefined;

    let cent_s: []f32 = centroids[0..];
    let assign_s: []usize = assignments[0..];
    let counts_s: []usize = counts[0..];
    let scratch_s: []f32 = scratch[0..];

    const cfg = KMeansConfig{ k: 4, max_iters: 30, tol: 0.0001, seed: 2026 };
    if (res := kmeansFitF32(data, n, dim, cfg, cent_s, assign_s, counts_s, scratch_s)) {
        * = res;
    }
}
```
