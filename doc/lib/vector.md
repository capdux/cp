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
| `lib/vector/flatIndex.cp` | `// Source: cp/lib/vector/flatIndex.cp (In-Memory Vector Index with Cosine/Dot/L2 Metrics)` | `DistanceMetric`、`SearchHit`、`VectorError`、`VectorIndex` |
| `lib/vector/bm25.cp` | `// Source: cp/lib/vector/bm25.cp (Okapi BM25 Full-Text Inverted Index)` | `Bm25Config`、`Bm25Index` 倒排索引与分词器 |
| `lib/vector/hybrid.cp` | `// Origin: Pure-CP Stdlib (RAG Hybrid Retrieval: RRF, MMR Rerank & Int8 Scalar Quantization) \| Created: 2026-11-22` | `RrfFusion` + 零堆分配多路 RRF (`rrfFuse`)、线性归一化融合 (`normalizeScoresMinMax`/`normalizeScoresZScore`/`weightedScoreFuse`)、MMR 多样性重排 (`mmrRerank`) 与 U8 标量量化 (`quantizeVectorU8`/`dequantizeVectorU8`/`quantizedDotU8`) |
| `lib/vector/semanticCache.cp` | `// Origin: Pure-CP Stdlib (AI Agent Semantic Cache: Cosine Threshold Lookup, TTL & LRU Eviction) \| Created: 2026-11-22` | `SemanticCache` 零堆分配 LLM 语义缓存（余弦阈值命中、TTL 过期与过期优先 + LRU 淘汰） |
| `lib/vector/binaryQuant.cp` | `// Origin: Pure-CP Stdlib (RAG 1-Bit Binary Vector Quantization, Hamming Search & Two-Stage Rescore) \| Created: 2026-11-22` | 1-bit 二值量化（32x 压缩）、`@popCount` 汉明距离、Charikar 余弦估计与两阶段粗排+全精度精排 (`twoStageSearch`) |
| `lib/vector/ivf.cp` | `// Origin: Pure-CP Stdlib (RAG IVF-Flat Coarse-Quantized Inverted File ANN Index) \| Created: 2026-11-22` | `IvfFlatIndex` 零堆分配 CSR 倒排文件近似最近邻索引（L2 / Cosine 多探针 `nprobe` 亚线性检索） |
| `lib/vector/pq.cp` | `// Origin: Pure-CP Stdlib (RAG Product Quantization & Asymmetric Distance Computation ADC) \| Created: 2026-11-22` | `PqCodebook` 乘积量化编码/解码与 `O(m)` 查表非对称距离（ADC）Top-K 检索 |
| `lib/vector/reranker.cp` | `// Origin: Pure-CP Stdlib (AI RAG Reranker: MMR, ColBERT MaxSim & IR Metrics) \| Created: 2026-11-22` | 零辅助缓冲 MMR (`mmrSelect`)、ColBERT Late-Interaction (`colbertMaxSim`)、原位 MinMax/ZScore 归一化与 `NDCG@K` / `MRR` / `Recall@K` IR 评测 |
| `lib/vector/filter.cp` | `// Origin: Pure-CP Stdlib (AI Vector Search Metadata Bitset Filter & Selectivity Planner) \| Created: 2026-11-22` | 1024-bit 零堆分配位图 (`FilterBitset`)、数值/范围元数据谓词过滤、选择度自适应 Filtered ANN 路由规划器与位跳跃掩码 Top-K 余弦检索 |
| `lib/vector/simdOps.cp` | `// Origin: Pure-CP Stdlib (AI-Native SIMD Vector Distance & Batch Retrieval Kernels using @dot/@l2sq/@fma) \| Created: 2026-11-22` | 原生 8-lane SIMD 内建原语 (`@dot`/`@l2sq`/`@fma`) 驱动的向量距离、单舍入 AXPY 与零堆分配批量 Top-K Cosine / L2 检索核 |
| `lib/vector/kmeans.cp` | `// Origin: Pure-CP Stdlib (AI-Native SIMD K-Means++ Clustering & IVF/PQ Codebook Trainer) \| Created: 2026-11-22` | 基于 `@l2sq`/`@fma` 原生 SIMD 与确定性 SplitMix64 的零堆分配 K-Means++ 聚类与 IVF/PQ 码本训练器 |
| `lib/vector/navGraph.cp` | `// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Vamana/NSW Navigable Graph ANN using @l2sq) \| Created: 2026-11-22` | 零堆分配 Vamana / NSW 单层近邻导航图近似检索索引 (`NavGraphIndex`)，基于 `@l2sq` 8-lane SIMD 贪心束搜索、双向建边与按距离剪枝 |
| `lib/vector/sparseVec.cp` | `// Origin: Pure-CP Stdlib (AI Learned Sparse Retrieval: SPLADE/BGE-M3 Sparse Vector & Merge-Join Scorer) \| Created: 2026-11-22` | SPLADE / BGE-M3 学习型稀疏词法向量表示、原位规范化、O(m+n) `@fma` 归并点积、Top-K 稀疏裁剪与零堆分配 CSR 批量 Top-K 检索核 |
| `lib/vector/evalHarness.cp` | `// Origin: Pure-CP Stdlib (AI Vector ANN Evaluation Harness: SIMD Ground-Truth, Recall@K & Latency Percentiles) \| Created: 2026-11-22` | ANN 向量索引离线评测与延迟分位数统计套件：原位堆排序 P50/P95/P99、原生 `@l2sq` SIMD 暴力 Ground-Truth、去重 Recall@K 与批量汇总 |
| `lib/vector/quantSq4.cp` | `// Origin: Pure-CP Stdlib (AI Vector Search 4-bit Asymmetric Scalar Quantization SQ4) \| Created: 2026-11-22` | 4-bit 非对称标量量化（SQ4）：`f32` 向量 8x 显存压缩、奇偶维 Nibble 打包、无临时解码缓冲 `@fma` 非对称 L2 距离与批量 Top-K 检索 |
| `lib/vector/rabitq.cp` | `// Origin: Pure-CP Stdlib (AI Vector Search RaBitQ Unbiased 1-bit Binary Quantization & Estimator) \| Created: 2026-11-22` | SIGMOD 2024 RaBitQ 无偏 1-bit 随机正交二值量化（32x 压缩）：$O(D \log D)$ 归一化 FWHT、确定性符号翻转、精确几何修正元数据与无偏内积/L2 距离估算核 |
| `lib/vector/projReduce.cp` | `// Origin: Pure-CP Stdlib (AI Embedding Dimensionality Reduction: Power-Iter PCA & JL Random Projection using @dot/@fma) \| Created: 2026-11-22` | 高维 Embedding 降维与随机投影核：均值中心化、无协方差矩阵幂迭代 Top-K 主成分求解、`@dot` 批量投影与 Achlioptas 稀疏 JL 随机投影 |
| `lib/vector/hybridRag.cp` | `// Origin: Pure-CP Stdlib (AI Hybrid RAG Pipeline: Weighted RRF Fusion, SIMD @dot/@l2sq MMR Rerank & Token Budget Pruning) \| Created: 2026-11-22` | 生产级零堆分配混合 RAG 管线：可配权重双路/多路 RRF (`HybridRrfFusion`)、`@dot`/`@fma` SIMD MMR 多样性重排 (`mmrRerankSimdF32`)、`@l2sq` 近重复去重 (`dedupNearDuplicatesL2F32`) 与 Prompt Token 预算裁剪 (`selectWithinTokenBudget`) |
| `lib/vector/ragStore.cp` | `// Origin: Pure-CP Stdlib (Zero-Heap Local RAG Store, 64-Bit Tag Filter & SIMD Cosine Top-K Facade) \| Created: 2026-11-22` | 零堆分配一站式本地 RAG 存储与检索门面 (`InMemoryRagStore`)：L2 归一化写入、墓碑槽位复用、64 位标签位图预过滤与 Token 预算上下文拼装 |
| `lib/vector/matryoshka.cp` | `// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Matryoshka Multi-Resolution Two-Stage SIMD Vector Retrieval) \| Created: 2026-11-22` | 零堆分配 Matryoshka 嵌套维多分辨率粗召回 + 精排重打分管线：前 `k` 维截断归一化 (`truncateAndNormalizeF32`)、紧凑粗向量库构建 (`buildCoarseEmbeddingsF32`)、8-lane SIMD `@dot` / `@l2sq` 两阶段检索与计算压缩比统计 |
| `lib/vector/colbert.cp` | `// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc ColBERT Multi-Vector Late-Interaction MaxSim Retrieval Index) \| Created: 2026-11-22` | 零堆分配 ColBERTv2 / ColPali 多向量 Token 级延迟交互（MaxSim）检索索引 (`ColbertLiteIndex`)：调用方扁平 Token 池、可选插入期 L2 归一化、8-lane SIMD `@dot` MaxSim 精确打分、确定性 Top-K 检索与 `@l2sq` 低范数噪声 Token 裁剪 (`prune_low_norm_tokens`) |
| `lib/vector/simhash.cp` | `// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc 64-bit Charikar SimHash & 4-Band LSH Near-Duplicate RAG Dedup) \| Created: 2026-11-22` | 零堆分配 64-bit Charikar SimHash 指纹与鸽巢 4x16-bit LSH 分带近重复 RAG 文档去重索引 (`SimHashDedupIndex`)：硬件 `@popCount` 汉明距离、零堆大小写折叠分词 + 2-shingle 哈希、256 槽位内联去重与槽位复用 |
| `lib/vector/bm25f.cp` | `// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc BM25F Multi-Field Weighted Inverted Index) \| Created: 2026-11-22` | 零堆分配 Robertson/Zaragoza **BM25F** 多字段加权倒排检索索引 (`Bm25fIndex`)：每字段独立长度归一化 `b_f` 与权重 `w_f` 聚合虚拟词频后统一 `k1` 饱和、平滑非负 IDF 与 4 字段代码检索预设 |
| `lib/vector/lateChunk.cp` | `// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Late Chunking & Contextual Token-Span Pooling Engine using @dot/@fma) \| Created: 2026-11-22` | 零堆分配长上下文 RAG **延迟分块与上下文 Token 区间池化引擎**（`detectSemanticBoundaries`、`poolLateChunkSpan`、`poolLateChunks`、`searchLateChunks`）：基于 `@dot`/`@fma` 的 3 策略池化（`mean_pool` / `weighted_attention_pool` / `max_norm_pool`）、相邻余弦突变自动断句、安全 L2 归一化与 NaN/Inf 清洗 |
| `lib/vector/index.cp` | `// Source: cp/lib/vector/index.cp (Unified Exports for cap:vector)` | `cap:vector` 与 `cap.vector` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 内存稠密向量索引 (`flatIndex.cp`)

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

### 4. 零堆分配 LLM 语义缓存 (`semanticCache.cp`)

- **`SemanticEntry`**：`{ occupied, key_id, payload_id, created_ms, last_hit_ms, expires_ms, hit_count, norm }`
- **`SemanticLookupHit`**：`{ slot: usize, key_id: u64, payload_id: u64, similarity: f64 }`
- **`SemanticCacheStats`**：`{ active_entries, capacity, hits, misses, evictions, expired_evictions, hit_rate }`
- **`SemanticCache`**：
  - `SemanticCache.init(entries: *[]SemanticEntry, embeddings_flat: *[]f64, dim: usize, similarity_threshold: f64, default_ttl_ms: i64): SemanticCache?`（校验 `dim > 0`、`embeddings_flat.len >= entries.len * dim`、`similarity_threshold ∈ (-1.0, 1.0]` 与 `default_ttl_ms >= 0`）
  - `put(*_, key_id, payload_id, embedding, now_ms, ttl_ms_override): usize?`（先原地更新同 `key_id` 槽位；满池优先驱逐已过期条目，再按 LRU 最久未命中 `last_hit_ms` 淘汰；自动拒绝零向量与 `NaN`/`Inf`）
  - `lookup(*_, query_embedding, now_ms): SemanticLookupHit?`（跳过过期条目，返回最高余弦相似度 `>= similarity_threshold` 的命中项并原子刷新 `last_hit_ms` 与 `hit_count`）
  - `invalidate(*_, key_id: u64): bool` / `purgeExpired(*_, now_ms: i64): usize` / `stats(): SemanticCacheStats`

### 5. 1-Bit 二值向量量化与两阶段粗排+精排检索 (`binaryQuant.cp`)

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

### 1-Bit 二值量化与两阶段粗排+精排示例 (`binaryQuant.cp`)

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

### 10. 原生 SIMD 内建原语向量距离与批量 Top-K 检索 (`simdOps.cp`)

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

### 12. 零堆分配 Vamana / NSW 近邻导航图近似检索索引 (`navGraph.cp`)

面向大规模高维嵌入向量亚线性近似最近邻（ANN）检索的单层可导航小世界图索引，对标 DiskANN / HNSW Layer-0 贪心束搜索，零堆分配并直接调用原生 8-lane SIMD `@l2sq` 内建原语：

- **`NAV_MAX_DEGREE` (`16`)** / **`NAV_INVALID_NODE` (`0xFFFFFFFF`)**：单节点固定邻接表最大度数与无效节点哨兵。
- **`NavGraphHit`**：`{ node_id: u32, dist_sq: f32 }`
- **`NavNodeAdj`**：`{ neighbors: [16]u32, degree: u8, occupied: bool }`
- **`NavGraphIndex`**（调用方提供扁平 `vectors_buf` 与 `adj_buf` 槽位池）：
  - `NavGraphIndex.init(vectors_buf: *[]f32, adj_buf: *[]NavNodeAdj, dim: usize, max_degree: usize): NavGraphIndex?`（校验 `dim > 0`、`1 <= max_degree <= 16` 与缓冲容量）
  - `insert(*_, vec: []const f32, ef_construct: usize): u32?`（基于 `@l2sq` 贪心束搜索候选近邻、双向建边、超度时按距离裁剪保留最近邻）
  - `search(query: []const f32, k: usize, ef_search: usize, out_hits: *[]NavGraphHit): usize?`（基于固定栈候选束与 visited 位图执行贪心束搜索，按 `(dist_sq 升序, node_id 升序)` 输出 Top-K）
  - `nodeCount(): usize` / `edgeCount(): usize` / `entryPoint(): u32`

```typescript
import {
    NavGraphHit,
    NavNodeAdj,
    NavGraphIndex,
} from "cap:vector";

pub fn demoNavGraphAnn() {
    let vecs: [128]f32 = [0.0; 128]; // 16 nodes * 8 dims
    let adjs: [16]NavNodeAdj = [NavNodeAdj{}; 16];
    let vecs_s: []f32 = vecs[0..];
    let adjs_s: []NavNodeAdj = adjs[0..];

    if (idx_init := NavGraphIndex.init(vecs_s, adjs_s, 8, 8)) {
        let idx = idx_init;
        const v0: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
        const v1: [8]f32 = [0.9, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
        * = idx.insert(v0[0..], 16);
        * = idx.insert(v1[0..], 16);

        let hits: [2]NavGraphHit = [NavGraphHit{}; 2];
        let hits_s: []NavGraphHit = hits[0..];
        const n = idx.search(v0[0..], 2, 16, hits_s) ?? 0;
        * = n;
    }
}
```

### 13. 学习型稀疏词法向量与 CSR 批量检索 (`sparseVec.cp`)

面向 SPLADE / BGE-M3 混合检索的零堆分配稀疏词法向量算子与 CSR 语料库批量 Top-K 检索核：

- **`SparseEntry`**：`{ term_id: u32, weight: f32 }`
- **`SparseHit`**：`{ doc_id: u32, score: f32 }`
- **`sparseNormalizeInPlace(entries: *[]SparseEntry): usize`**：过滤 `NaN`/`Inf`，原位 $O(n \log n)$ 堆排序按 `term_id` 升序排列、累加合并重复 `term_id` 权重并剔除 `<= 0` 项，返回有效规范化长度。
- **`sparseDot(a: []const SparseEntry, b: []const SparseEntry): f32`**：严格升序稀疏向量的 $O(|a| + |b|)$ 双指针归并点积，使用硬件 `@fma` 累加。
- **`sparseCosine(a: []const SparseEntry, b: []const SparseEntry): f32`**：稀疏余弦相似度（钳位至 `[-1.0, 1.0]`，空向量或零范数安全返回 `0.0`）。
- **`sparsePruneTopK(src: []const SparseEntry, k: usize, out_dst: *[]SparseEntry): usize`**：按 `|weight|` 保留 Top-K 词项（同权按 `term_id` 升序决胜）并重新按 `term_id` 升序输出，用于 SPLADE 稀疏度控制。
- **`sparseBatchTopK(query: []const SparseEntry, corpus_entries: []const SparseEntry, doc_offsets: []const usize, k: usize, out_hits: *[]SparseHit): usize?`**：基于 CSR 紧凑偏移数组零堆分配批量检索 Top-K（跳过零重叠文档，按 `score` 降序、同分 `doc_id` 升序输出；非法偏移或缓冲不足返回 `null`）。

```typescript
import {
    SparseEntry,
    SparseHit,
    sparseNormalizeInPlace,
    sparseDot,
    sparseCosine,
    sparsePruneTopK,
    sparseBatchTopK,
} from "cap:vector";

pub fn demoLearnedSparseRetrieval() {
    let raw_q: [4]SparseEntry = [
        SparseEntry{ term_id: 42, weight: 1.5 },
        SparseEntry{ term_id: 7, weight: 0.8 },
        SparseEntry{ term_id: 42, weight: 0.5 },
        SparseEntry{ term_id: 99, weight: 0.0 },
    ];
    let q_s: []SparseEntry = raw_q[0..];
    const q_len = sparseNormalizeInPlace(q_s); // [(7, 0.8), (42, 2.0)]

    const corpus: [4]SparseEntry = [
        SparseEntry{ term_id: 7, weight: 1.0 },
        SparseEntry{ term_id: 42, weight: 1.0 },
        SparseEntry{ term_id: 42, weight: 3.0 },
        SparseEntry{ term_id: 100, weight: 2.0 },
    ];
    const offsets: [3]usize = [0, 2, 4]; // doc 0: [0..2], doc 1: [2..4]
    let hits: [2]SparseHit = [SparseHit{}; 2];
    let hits_s: []SparseHit = hits[0..];

    const n = sparseBatchTopK(raw_q[0..q_len], corpus[0..], offsets[0..], 2, hits_s) ?? 0;
    * = sparseDot(raw_q[0..q_len], corpus[0..2]);
    * = sparseCosine(raw_q[0..q_len], corpus[0..2]);
    let pruned: [1]SparseEntry = [SparseEntry{}; 1];
    let pruned_s: []SparseEntry = pruned[0..];
    * = sparsePruneTopK(raw_q[0..q_len], 1, pruned_s);
    * = n;
}
```

### 14. ANN 向量索引离线评测与延迟分位数套件 (`evalHarness.cp`)

零堆分配的 ANN 召回评测与延迟分位数统计套件，内置基于原生 8-lane SIMD `@l2sq` 的暴力 Ground-Truth 生成器与切片 `slice.split(k)` 批量评测：

- **`LatencyPercentiles`**：`{ count, min_us, p50_us, p95_us, p99_us, max_us, mean_us }`
- **`computeLatencyPercentilesInPlace(samples_us: *[]u64): LatencyPercentiles?`**：原位 $O(n \log n)$ 堆排序并计算精确 Nearest-Rank P50/P95/P99 分位数与均值；空样本返回 `null`。
- **`AnnEvalSummary`**：`{ queries, k, mean_recall_at_k, min_recall_at_k, exact_top1_rate }`
- **`computeGroundTruthL2TopK(query, corpus_flat, n_docs, dim, k, out_gt_ids: *[]u32): usize?`**：基于 `@l2sq` 原生 SIMD 生成精确 Top-K L2 最近邻 Ground-Truth ID（按 `(dist_sq 升序, doc_id 升序)` 确定性打破平局）。
- **`annRecallAtK(predicted_ids, ground_truth_ids, k): f64?`**：自动去重预测 ID 与真值 ID 计算严格落在 `[0.0, 1.0]` 的精确 `Recall@K`。
- **`evaluateAnnBatch(pred_flat, gt_flat, num_queries, k): AnnEvalSummary?`**：批量汇总 `mean_recall_at_k`、`min_recall_at_k` 与 `exact_top1_rate`。

```typescript
import {
    computeLatencyPercentilesInPlace,
    computeGroundTruthL2TopK,
    evaluateAnnBatch,
} from "cap:vector";

pub fn demoAnnEvalHarness(query: []const f32, corpus: []const f32, n_docs: usize, dim: usize) {
    let gt_buf: [5]u32 = [0; 5];
    let gt_s: []u32 = gt_buf[0..];
    * = computeGroundTruthL2TopK(query, corpus, n_docs, dim, 5, gt_s);

    const pred: [5]u32 = [gt_buf[0], gt_buf[1], gt_buf[2], gt_buf[3], gt_buf[4]];
    if (summary := evaluateAnnBatch(pred[0..], gt_buf[0..], 1, 5)) {
        * = summary.mean_recall_at_k;
    }

    let lat: [5]u64 = [120, 45, 310, 80, 95];
    let lat_s: []u64 = lat[0..];
    if (pct := computeLatencyPercentilesInPlace(lat_s)) {
        * = pct.p99_us;
    }
}
```

### 15. 4-Bit 非对称标量量化（SQ4 — 8x 显存压缩）(`quantSq4.cp`)

对标 Qdrant / Faiss SQ4 的零堆分配 4-bit 标量量化与非对称 L2 距离检索核，将 `f32` 向量压缩 8 倍（每字节打包 2 个 4-bit Nibble `[0..15]`）：

- **`Sq4Codebook`**：`{ mins: []f32, scales: []f32, dim: usize }`，提供 `Sq4Codebook.init(mins, scales, dim): Sq4Codebook?`。
- **`sq4PackedBytes(dim: usize): usize`**：返回 `ceil(dim / 2)` 打包字节数。
- **`sq4FitCodebook(corpus_flat, n_docs, dim, out_mins: *[]f32, out_scales: *[]f32): bool`**：按维拟合 `[min, (max - min) / 15.0]`（常量维 `min == max` 自动置 `scale = 0.0` 防零除）。
- **`sq4EncodeVector(vec, mins, scales, out_packed: *[]u8): bool`** / **`sq4DecodeVector(packed_code, mins, scales, out_vec: *[]f32): bool`**：偶维低 4 位、奇维高 4 位打包编码与 `@fma` 反量化重建。
- **`sq4AsymmetricL2Sq(query, packed_code, mins, scales): f32?`**：无需临时解码向量，直接基于硬件 `@fma` 计算全精度查询与 4-bit 编码向量的非对称平方 L2 距离。
- **`sq4BatchTopK(query, packed_corpus, n_docs, mins, scales, k, out_ids: *[]usize, out_dists: *[]f32): usize?`**：8x 压缩语料库零堆分配 Top-K 近似检索（按 `(dist_sq 升序, doc_id 升序)` 输出）。

```typescript
import {
    sq4FitCodebook,
    sq4EncodeVector,
    sq4BatchTopK,
} from "cap:vector";

pub fn demoSq4Retrieval(corpus: []const f32, query: []const f32) {
    let mins: [4]f32 = [0.0; 4];
    let scales: [4]f32 = [0.0; 4];
    let mins_s: []f32 = mins[0..];
    let scales_s: []f32 = scales[0..];
    * = sq4FitCodebook(corpus, 2, 4, mins_s, scales_s);

    let packed_buf: [4]u8 = [0; 4]; // 2 docs * 2 bytes (8x smaller than 32 bytes f32)
    let p0: []u8 = packed_buf[0..2];
    let p1: []u8 = packed_buf[2..4];
    * = sq4EncodeVector(corpus[0..4], mins[0..], scales[0..], p0);
    * = sq4EncodeVector(corpus[4..8], mins[0..], scales[0..], p1);

    let ids: [2]usize = [0; 2];
    let dists: [2]f32 = [0.0; 2];
    let ids_s: []usize = ids[0..];
    let dists_s: []f32 = dists[0..];
    * = sq4BatchTopK(query, packed_buf[0..], 2, mins[0..], scales[0..], 2, ids_s, dists_s);
}
```

### 16. RaBitQ 无偏 1-Bit 随机正交二值量化（32x 显存压缩）(`rabitq.cp`)

对标 SIGMOD 2024 RaBitQ 的零堆分配无偏 1-bit 随机正交量化与内积/L2 估算核，将 `f32` 向量压缩 32 倍同时保持理论无偏距离估计：

- **`RabitqCodeMeta`**：`{ norm_l2: f32, dot_o_obar: f32, sum_bits: u32 }`
- **`rabitqWordsForDim(dim: usize): usize`**：返回打包 `dim` 比特所需 `u64` 字数 `ceil(dim / 64)`。
- **`rabitqSignFlipInPlace(vec: *[]f32, seed: u64): bool`**：确定性 Rademacher（`±1`）坐标符号翻转（对合可逆）。
- **`rabitqFastWalshHadamardInPlace(vec: *[]f32): bool`**：2 的幂维度 $O(D \log D)$ 归一化快速沃尔什-哈达玛正交变换（保范数、保内积、自逆 $H_D^2 = I$）。
- **`rabitqEncodeUnit(vec, centroid, out_words: *[]u64): RabitqCodeMeta?`**：相对质心残差编码为 1-bit 打包字并输出精确几何修正因子 `norm_l2`、`dot_o_obar = <o, o_bar>` 与 `@popCount` 置位数 `sum_bits`。
- **`rabitqEstimateDot(query_residual, code_words, meta): f32?`**：零临时解码缓冲计算无偏内积估计 $\hat{d} = \|x - c\| \cdot \langle q', \bar{o} \rangle / \langle o, \bar{o} \rangle$。
- **`rabitqEstimateL2Sq(query_residual, query_norm_sq, code_words, meta): f32?`**：基于 `@fma` 计算无偏平方 L2 距离估计。

```typescript
import {
    rabitqSignFlipInPlace,
    rabitqFastWalshHadamardInPlace,
    rabitqEncodeUnit,
    rabitqEstimateL2Sq,
} from "cap:vector";

pub fn demoRabitq(x_in: []const f32, q_in: []const f32) {
    let x: [64]f32 = [0.0; 64];
    let q: [64]f32 = [0.0; 64];
    const c: [64]f32 = [0.0; 64];
    let i: usize = 0;
    while (i < 64 && i < x_in.len && i < q_in.len) : (i += 1) {
        x[i] = x_in[i];
        q[i] = q_in[i];
    }
    let xs: []f32 = x[0..];
    let qs: []f32 = q[0..];
    * = rabitqSignFlipInPlace(xs, 2026);
    * = rabitqFastWalshHadamardInPlace(xs);
    * = rabitqSignFlipInPlace(qs, 2026);
    * = rabitqFastWalshHadamardInPlace(qs);

    let code: [1]u64 = [0];
    let code_s: []u64 = code[0..];
    if (m := rabitqEncodeUnit(x[0..], c[0..], code_s)) {
        const q_norm_sq: f32 = @dot(q[0..], q[0..]);
        * = rabitqEstimateL2Sq(q[0..], q_norm_sq, code[0..], m);
    }
}
```

### 17. 高维 Embedding 降维与随机投影 (`projReduce.cp`)

面向高维语义嵌入降维、去均值与保距稀疏投影的零堆分配算子，基于 `@dot`/`@fma` 8-lane SIMD 与 `slice.split` 实现：

- **`computeMeanF32(data, n, dim, out_mean: *[]f32): bool`** / **`subtractMeanInPlaceF32(data: *[]f32, n, dim, mean): bool`**：按维计算均值与原位零均值中心化。
- **`fitTopKPrincipalAxesF32(centered_data, n, dim, k, max_iters, seed, out_axes: *[]f32, out_eigenvals: *[]f32, scratch_vec: *[]f32): bool`**：零堆分配无协方差矩阵幂迭代（$w = X^T(X v)/n$）+ 两遍修正 Gram-Schmidt 正交消去，输出前 `k` 个单位正交主轴及降序样本方差。
- **`projectEmbeddingsF32(data, n, in_dim, axes, out_dim, out_proj: *[]f32): bool`**：基于 `@dot` 8-lane SIMD 批量将 `[n, in_dim]` 投影至 `[n, out_dim]`。
- **`jlRandomProjectF32(vec, in_dim, out_dim, seed, out_proj: *[]f32): bool`**：Achlioptas 稀疏 `{-1, 0, +1}`（2/3 稀疏度）确定性 Johnson-Lindenstrauss 随机投影，零矩阵存储且期望精确保距。

```typescript
import {
    computeMeanF32,
    subtractMeanInPlaceF32,
    fitTopKPrincipalAxesF32,
    projectEmbeddingsF32,
    jlRandomProjectF32,
} from "cap:vector";

pub fn demoProjReduce(data_in: *[]f32, n: usize) {
    let mean: [8]f32 = [0.0; 8];
    let mean_s: []f32 = mean[0..];
    * = computeMeanF32(data_in[0..], n, 8, mean_s);
    * = subtractMeanInPlaceF32(data_in, n, 8, mean[0..]);

    let axes: [16]f32 = [0.0; 16]; // k = 2, dim = 8
    let evals: [2]f32 = [0.0; 2];
    let scratch: [8]f32 = [0.0; 8];
    let axes_s: []f32 = axes[0..];
    let evals_s: []f32 = evals[0..];
    let scratch_s: []f32 = scratch[0..];
    * = fitTopKPrincipalAxesF32(data_in[0..], n, 8, 2, 40, 42, axes_s, evals_s, scratch_s);

    let proj_one: [2]f32 = [0.0; 2];
    let proj_s: []f32 = proj_one[0..];
    * = projectEmbeddingsF32(data_in[0..8], 1, 8, axes[0..], 2, proj_s);
    * = jlRandomProjectF32(data_in[0..8], 8, 2, 2026, proj_s);
}
```

### 18. 生产级零堆分配混合检索、SIMD MMR 多样性重排与 Token 预算裁剪 (`hybridRag.cp`)

面向 AI Agent RAG 上下文构造的端到端零堆分配、8-lane SIMD（`@dot` / `@l2sq` / `@fma`）加速混合检索与上下文裁剪管线：

- **`HybridRagHit`**：`{ doc_id: u32, score: f32, dense_rank: u32, sparse_rank: u32 }`（1-based 通道排名，`0` 表示未出现在该通道）
- **`HybridRrfFusion`**：可配置 `dense_weight`、`sparse_weight`、`rrf_k` 的零堆分配加权 RRF 融合器：
  - `HybridRrfFusion.init(dense_weight: f32, sparse_weight: f32, rrf_k: f32): HybridRrfFusion?`（校验权重与 `rrf_k` 有限非负，拒绝双权重同为 `0.0`）
  - `fuseDual(dense_ids, sparse_ids, top_k, out_hits: *[]HybridRagHit, work_hits: *[]HybridRagHit): usize?`：双路加权倒数排名融合 `dense_weight / (rrf_k + dense_rank) + sparse_weight / (rrf_k + sparse_rank)`，自动通道内去重并记录双路排名，按 `(score 降序, doc_id 升序)` 输出 Top-K。
  - `fuseMultiWeighted(lists_doc_ids, list_offsets, list_weights, top_k, out_hits: *[]HybridRagHit, work_hits: *[]HybridRagHit): usize?`：CSR 紧凑多路加权 RRF 融合（支持 Dense + BM25 + SPLADE + Graph 等任意 M 路召回）。
- **`mmrRerankSimdF32(relevance_scores, embeddings_flat, n_docs, dim, top_k, lambda, out_indices: *[]usize): usize?`**：基于 `@dot` 与 `@fma` 的 8-lane SIMD `f32` 最大边际相关性（MMR）多样性重排（`lambda ∈ [0.0, 1.0]`，`O(1)` 额外栈空间，无需外部 mask 缓冲，彻底消除 RAG 近重复片段）。
- **`dedupNearDuplicatesL2F32(embeddings_flat, n_docs, dim, min_l2_sq, max_keep, out_indices: *[]usize): usize?`**：基于 `@l2sq` SIMD 平方欧氏距离阈值的快速近重复片段去重。
- **`RagContextChunk`**（`{ doc_id: u32, score: f32, token_count: u32 }`）/ **`TokenBudgetResult`**（`{ selected_count, used_tokens, skipped_oversized, filtered_low_score, budget_limit }`）
- **`selectWithinTokenBudget(candidates, max_tokens, min_score, greedy_skip_oversized, out_selected: *[]RagContextChunk): TokenBudgetResult?`**：在硬性 LLM Prompt Token 预算内按最低分数过滤并执行贪心填充（`greedy_skip_oversized = true`）或严格前缀截断（`greedy_skip_oversized = false`）。

```typescript
import {
    HybridRagHit,
    HybridRrfFusion,
    RagContextChunk,
    mmrRerankSimdF32,
    dedupNearDuplicatesL2F32,
    selectWithinTokenBudget,
} from "cap:vector";

pub fn demoHybridRagPipeline() {
    // 1. 可配权重 Dense + Sparse RRF 融合（偏向 Dense 语义召回 0.7 : 0.3）
    if (rrf := HybridRrfFusion.init(0.7, 0.3, 60.0)) {
        const dense_ids: [4]u32 = [101, 102, 103, 104];
        const sparse_ids: [4]u32 = [103, 105, 101, 106];
        let work: [8]HybridRagHit = [HybridRagHit{}; 8];
        let fused: [4]HybridRagHit = [HybridRagHit{}; 4];
        let work_s: []HybridRagHit = work[0..];
        let fused_s: []HybridRagHit = fused[0..];
        const n_fused = rrf.fuseDual(dense_ids[0..], sparse_ids[0..], 4, fused_s, work_s) ?? 0;

        // 2. 基于 @dot 的 SIMD MMR 多样性重排（消除 101/102 近重复片段）
        const rels: [4]f32 = [0.95, 0.94, 0.85, 0.78];
        const embs: [16]f32 = [
            1.0, 0.0, 0.0, 0.0,
            0.998, 0.02, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
        ];
        let mmr_idx: [3]usize = [0; 3];
        let mmr_s: []usize = mmr_idx[0..];
        const n_mmr = mmrRerankSimdF32(rels[0..], embs[0..], 4, 4, 3, 0.5, mmr_s) ?? 0;

        // 3. 按 LLM Prompt Token 预算裁剪（max_tokens = 600，贪心跳过超大块）
        const chunks: [3]RagContextChunk = [
            RagContextChunk{ doc_id: 101, score: 0.95, token_count: 350 },
            RagContextChunk{ doc_id: 103, score: 0.85, token_count: 400 },
            RagContextChunk{ doc_id: 104, score: 0.78, token_count: 200 },
        ];
        let packed_buf: [3]RagContextChunk = [RagContextChunk{}; 3];
        let packed_s: []RagContextChunk = packed_buf[0..];
        if (budget := selectWithinTokenBudget(chunks[0..], 600, 0.5, true, packed_s)) {
            * = budget.used_tokens; // 550 tokens (selected doc 101 + doc 104, skipped doc 103)
        }
        * = n_fused;
        * = n_mmr;
    }
}
```

---

## 零堆分配一站式本地 RAG 向量库门面（`ragStore.cp`）

面向人类与 AI 程序员的 15 行开箱即用本地 RAG 存储与检索门面，100% 纯 CP 零堆分配：

- **`RagDocMeta` / `RagSearchHit` / `RagStoreStats`**：承载 `doc_id`、`namespace_hash`、64 位多标签位图 `tag_mask: u64`、`token_count`、`timestamp_ms`、`text` 与逻辑删除墓碑标记。
- **`InMemoryRagStore`**：
  - `init(metas_buf, vecs_flat_buf, dim): InMemoryRagStore?`：复用调用方栈/静态切片零堆承载元数据与扁平向量池；
  - `upsert(*_, doc_id, namespace_hash, tag_mask, token_count, timestamp_ms, text, embedding): bool`：自动 L2 归一化写入，同 `doc_id` 原地更新，新文档自动复用已删除墓碑槽位；
  - `remove(*_, doc_id): bool` / `get(doc_id): RagDocMeta?`；
  - `searchFiltered(query_vec, namespace_hash, required_tag_mask, min_score, top_k, out_hits): usize?`：命名空间 + 64 位标签位图预过滤 + `@dot` 8-lane SIMD 余弦相似度 Top-K 召回；
  - `packSearchContext(hits, max_tokens, out_buf): usize?`：在 `max_tokens` 预算内自动拼装带 `[doc:<id>]` 来源标记的 LLM 上下文块；
  - `len()` / `capacity()` / `embeddingDim()` / `stats()`。

### 15 行开箱即用 QuickStart 示例

```cp
import { RagDocMeta, RagSearchHit, InMemoryRagStore } from "cap:vector";

pub fn demoLocalRagQuickStart() {
    let metas: [8]RagDocMeta = [RagDocMeta{}; 8];
    let vecs: [32]f32 = [0.0; 32]; // 8 docs x dim 4
    let metas_s: []RagDocMeta = metas[0..];
    let vecs_s: []f32 = vecs[0..];
    let store = InMemoryRagStore.init(metas_s, vecs_s, 4) ?? return;

    const e1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    * = store.upsert(1, 100, 0b01, 12, 1000, "CP supports 8-lane SIMD @dot", e1[0..]);
    * = store.upsert(2, 100, 0b10, 15, 1001, "BEAM scheduler runs millions of actors", e2[0..]);

    let hits: [2]RagSearchHit = [RagSearchHit{}; 2];
    let hits_s: []RagSearchHit = hits[0..];
    const n = store.searchFiltered(e1[0..], 100, 0b01, 0.5, 2, hits_s) ?? 0;
    let ctx: [256]u8 = [0; 256];
    let ctx_s: []u8 = ctx[0..];
    * = store.packSearchContext(hits_s[0..n], 64, ctx_s);
}
```

---

## Matryoshka 嵌套维多分辨率两阶段 SIMD 向量检索管线（`matryoshka.cp`）

面向 OpenAI `text-embedding-3`、Nomic、Jina、Gemini 等现代 Matryoshka Representation Learning (MRL) 嵌套维度嵌入模型的 100% 纯 CP、零堆分配两阶段粗到精检索引擎，充分利用 CP 原生 8-lane SIMD `@dot`、`@l2sq` 与 `slice.split` 零开销行遍历：

- **`MatryoshkaConfig`**：`{ full_dim, coarse_dim, coarse_top_k, final_top_k }`
  - `MatryoshkaConfig.init(full_dim, coarse_dim, coarse_top_k, final_top_k): MatryoshkaConfig?`（校验 `0 < coarse_dim <= full_dim` 与 `0 < final_top_k <= coarse_top_k`）
  - `isValid(): bool`
- **`MatryoshkaHit`**：`{ doc_idx: usize, coarse_score: f32, fine_score: f32 }`
- **`MatryoshkaStats`**：`{ scanned_docs, coarse_candidates, returned_hits, coarse_dim, full_dim, coarse_macs, fine_macs, brute_force_macs, compute_speedup_ratio }`
  - 自动统计阶段一粗排 MACs、阶段二精排 MACs、全维暴力 MACs 及真实计算压缩加速比 `compute_speedup_ratio = brute_force_macs / (coarse_macs + fine_macs)`。
- **核心算子**：
  - `truncateAndNormalizeF32(full_vec: []const f32, target_dim: usize, out_vec: *[]f32): bool`：截取前 `target_dim` 维并用 8-lane SIMD `@dot` + `@sqrt` 单位化，拒绝越界、零向量与 `NaN`/`Inf`。
  - `buildCoarseEmbeddingsF32(full_vecs_flat: []const f32, n_docs: usize, full_dim: usize, coarse_dim: usize, out_coarse_flat: *[]f32): bool`：通过 `slice.split` 零开销批量构建 `[n_docs, coarse_dim]` 紧凑单位化粗检索库。
  - `searchMatryoshkaTwoStageF32(query_full, coarse_vecs_flat, full_vecs_flat, n_docs, cfg, scratch_coarse_hits, out_final_hits): MatryoshkaStats?`：阶段一仅用 `coarse_dim` 8-lane SIMD `@dot` 扫描全库取 `coarse_top_k` 候选；阶段二仅对候选集执行 `full_dim` 精确 `@dot` 余弦精排输出 `final_top_k`，支持任意高维且无栈维度上限。
  - `searchMatryoshkaTwoStageL2F32(query_full, coarse_vecs_flat, full_vecs_flat, n_docs, cfg, scratch_coarse_hits, out_final_hits): MatryoshkaStats?`：基于 8-lane SIMD `@l2sq` 的两阶段 L2 欧氏近邻检索（仅对最终 `final_top_k` 执行 `@sqrt`）。

### Matryoshka 两阶段检索 QuickStart 示例

```cp
import {
    MatryoshkaConfig,
    MatryoshkaHit,
    buildCoarseEmbeddingsF32,
    searchMatryoshkaTwoStageF32,
} from "cap:vector";

pub fn demoMatryoshkaTwoStage() {
    // 示例：4 条 16D 嵌套嵌入压缩至 4D 粗检索库，先粗召回 Top-3 再全维精排 Top-2
    const full_corpus: [64]f32 = [
        1.0, 0.0, 0.0, 0.0,  0.5, 0.5, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
        0.9, 0.1, 0.0, 0.0,  0.9, 0.9, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 1.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
    ];
    let coarse_bank: [16]f32 = [0.0; 16]; // 4 docs x 4D
    let coarse_s: []f32 = coarse_bank[0..];
    if (!buildCoarseEmbeddingsF32(full_corpus[0..], 4, 16, 4, coarse_s)) return;

    const query: [16]f32 = [
        0.9, 0.1, 0.0, 0.0,  0.9, 0.9, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
    ];
    const cfg = MatryoshkaConfig.init(16, 4, 3, 2) ?? return;
    let scratch: [3]MatryoshkaHit = [MatryoshkaHit{}; 3];
    let out_hits: [2]MatryoshkaHit = [MatryoshkaHit{}; 2];
    let scratch_s: []MatryoshkaHit = scratch[0..];
    let out_s: []MatryoshkaHit = out_hits[0..];

    if (stats := searchMatryoshkaTwoStageF32(query[0..], coarse_bank[0..], full_corpus[0..], 4, cfg, scratch_s, out_s)) {
        // out_hits[0].doc_idx == 1 (精排翻转粗排), stats.compute_speedup_ratio 记录计算压缩比
        * = stats.compute_speedup_ratio;
    }
}
```

---

## ColBERT 多向量延迟交互（MaxSim）检索索引（`colbert.cp`）

面向 ColBERTv2 / ColPali 等现代高精度代码、文档与表格多向量 RAG 场景的 100% 纯 CP 零动态堆分配 Token 级延迟交互（Late-Interaction MaxSim）检索引擎：

- **常量与数据结构**：
  - `COLBERT_MAX_DOCS = 128`：索引定长文档槽位上限（零堆分配）。
  - `ColbertConfig`：`{ dim: usize, l2_normalize_on_insert: bool = true }`，提供 `ColbertConfig.init(dim, l2_normalize_on_insert)` 与 `isValid(): bool`。
  - `ColbertDocSpan`：`{ doc_id: u64, token_offset: usize, token_count: usize, active: bool }`，记录单篇文档在调用方扁平 Token 池中的连续区间。
  - `ColbertSearchHit`：`{ doc_id: u64, maxsim_score_f32: f32, avg_maxsim_f32: f32, matched_tokens: usize }`。
  - `ColbertIndexStats`：`{ active_docs, total_stored_tokens, token_capacity, dim }`。
- **核心索引 `ColbertLiteIndex`**：
  - `ColbertLiteIndex.init(cfg: ColbertConfig, token_pool: *[]f32): ColbertLiteIndex?`：在调用方拥有的扁平 `f32` Token 池上零分配初始化索引。
  - `add_document(*_, doc_id: u64, doc_tokens: []const f32, num_tokens: usize): bool`：提供活跃 ID 去重、容量/槽位/维度/非有限值守卫，支持插入期按 token 粒度通过 `@dot` + `@sqrt` 自动 L2 归一化写入。
  - `remove_document(*_, doc_id: u64): bool`：逻辑摘除目标文档，使其不再参与 MaxSim 打分与 Top-K 检索。
  - `score_document_maxsim(query_tokens: []const f32, q_tokens: usize, doc_idx: usize): f32`：利用 CP 原生 8-lane SIMD `@dot` 计算精确延迟交互得分 $\sum_{i} \max_{j} \langle q_i, d_j \rangle$。
  - `search_topk(query_tokens: []const f32, q_tokens: usize, k: usize, out_hits: *[]ColbertSearchHit): usize`：精确 MaxSim Top-K 降序检索，平局按 `doc_id` 升序稳定决胜，同时输出 `avg_maxsim_f32` 与正相关匹配 token 数 `matched_tokens`。
  - `stats(): ColbertIndexStats`：返回当前活跃文档数、已存 Token 数、池容量与维度。
- **低范数噪声 Token 裁剪核**：
  - `prune_low_norm_tokens(tokens: []const f32, num_tokens: usize, dim: usize, min_l2_norm: f32, out_tokens: *[]f32): usize`：基于 CP 原生 `@l2sq` 分块 SIMD 裁剪 L2 范数低于阈值的停用词/噪声 Token 嵌入并紧凑写回 `out_tokens`，自动过滤 `NaN`/`Inf`，有效加速 2x~4x 检索。

### ColBERT MaxSim 多向量检索 QuickStart 示例

```cp
import {
    ColbertConfig,
    ColbertSearchHit,
    ColbertLiteIndex,
    prune_low_norm_tokens,
} from "cap:vector";

pub fn demoColbertMaxSim() {
    let pool: [128]f32 = [0.0; 128]; // 最多容纳 32 个 4D token
    let pool_s: []f32 = pool[0..];
    const cfg = ColbertConfig.init(4, true) ?? return;
    let idx = ColbertLiteIndex.init(cfg, pool_s) ?? return;

    // 文档 101 包含两个语义正交的 token（覆盖概念 A 与概念 B）
    const doc101: [8]f32 = [
        3.0, 0.0, 0.0, 0.0,
        0.0, 4.0, 0.0, 0.0,
    ];
    // 文档 102 仅覆盖概念 A
    const doc102: [4]f32 = [2.0, 0.0, 0.0, 0.0];
    * = idx.add_document(101, doc101[0..], 2);
    * = idx.add_document(102, doc102[0..], 1);

    // 裁剪查询中的低范数停用词噪声 token
    const raw_q: [12]f32 = [
        1.0, 0.0, 0.0, 0.0,   // 有效概念 A
        0.05, 0.02, 0.0, 0.0, // 低范数噪声 token -> 将被裁剪
        0.0, 1.0, 0.0, 0.0,   // 有效概念 B
    ];
    let clean_q: [12]f32 = [0.0; 12];
    let clean_q_s: []f32 = clean_q[0..];
    const kept = prune_low_norm_tokens(raw_q[0..], 3, 4, 0.5, clean_q_s);

    let hits: [2]ColbertSearchHit = [ColbertSearchHit{}; 2];
    let hits_s: []ColbertSearchHit = hits[0..];
    const n = idx.search_topk(clean_q[0 .. kept * 4], kept, 2, hits_s);
    // n == 2, hits[0].doc_id == 101 (MaxSim = 2.0), hits[1].doc_id == 102 (MaxSim = 1.0)
    * = n;
}
```

---

## 零分配 64-bit Charikar SimHash 与 4-Band LSH 近重复 RAG 文档去重索引（`simhash.cp`）

面向 RAG 语料摄取、AI Agent 网页抓取记忆与代码分块入库的 100% 纯 CP 零动态堆分配近重复文本检测与去重索引：

- **常量与数据结构**：
  - `SIMHASH_MAX_DOCS = 256`、`SIMHASH_NUM_BANDS = 4`
  - `SimHashDedupConfig`：`{ max_hamming_dist: u32 = 3, min_token_len: usize = 2 }`，提供 `isValid(): bool`（校验 `max_hamming_dist <= 16 && min_token_len >= 1`）。
  - `SimHashDocEntry`：`{ doc_id: u64, fingerprint: u64, active: bool }`
  - `SimHashMatch`：`{ is_duplicate: bool, matched_doc_id: u64, hamming_dist: u32, fingerprint: u64, inserted: bool }`
  - `SimHashDedupStats`：`{ active_docs, total_checked, duplicates_rejected, unique_accepted }`
- **纯函数算子**：
  - `hammingDistanceU64(a: u64, b: u64): u32`：调用硬件 `@popCount(a ^ b)` 计算精确 64 位汉明距离。
  - `computeSimHash64(text: []const u8, min_token_len: usize): u64`：零堆 ASCII 大小写折叠分词 + 单词与 2-shingle FNV-1a/SplitMix64 哈希 + 64 维符号权重累加（空或无有效词元文本返回 `0`）。
- **核心去重索引（`SimHashDedupIndex`）**：
  - `SimHashDedupIndex.init(cfg): SimHashDedupIndex?`
  - `query_nearest_fp(fp: u64): SimHashMatch`：`max_hamming_dist <= 3` 时启用鸽巢原理 4x16-bit 分带零漏报预筛；`> 3` 时自动全量 `@popCount` 精确扫描，平局按 `doc_id` 升序决胜。
  - `query_nearest(text: []const u8): SimHashMatch`
  - `check_and_insert(*_, doc_id: u64, text: []const u8): SimHashMatch`：近重复或活跃 `doc_id` 冲突时拒插并递增 `duplicates_rejected`；唯一文档入库并递增 `unique_accepted`；空文本与满 256 槽安全防护。
  - `remove_doc(*_, doc_id: u64): bool`：逻辑摘除并释放槽位供后续文档复用。
  - `stats(): SimHashDedupStats`

### 64-bit SimHash 与 4-Band LSH 近重复去重 QuickStart 示例

```cp
import {
    SimHashDedupConfig,
    SimHashDedupIndex,
} from "cap:vector";

pub fn demoSimHashRagDedup() {
    let idx = SimHashDedupIndex.init(SimHashDedupConfig{
        max_hamming_dist: 3,
        min_token_len: 2,
    }) ?? return;

    const doc1: []const u8 = "CP language compiles to native Zig state machines for million-scale AI agent concurrency.";
    const doc1_dup: []const u8 = "CP Language compiles to native Zig state machines, for million-scale AI agent concurrency!";
    const doc2: []const u8 = "Quantum entanglement enables teleportation of photonic qubits across optical fiber networks.";

    const r1 = idx.check_and_insert(101, doc1);     // inserted == true, is_duplicate == false
    const r2 = idx.check_and_insert(102, doc1_dup); // inserted == false, is_duplicate == true, matched_doc_id == 101
    const r3 = idx.check_and_insert(103, doc2);     // inserted == true, is_duplicate == false
    * = r1;
    * = r2;
    * = r3;
}
```

---

## 零分配 BM25F 多字段加权倒排检索索引（`bm25f.cp`）

面向代码检索、结构化文档 RAG 与多字段元数据搜索的 100% 纯 CP 零动态堆分配 Robertson/Zaragoza **BM25F** 多字段加权倒排检索内核：

- **核心原理**：先按各字段独立长度归一化参数 `b_f` 与字段权重 `w_f` 聚合虚拟词频 $\text{pseudo\_tf} = \sum_f \frac{w_f \cdot \text{tf}_f}{(1 - b_f) + b_f \cdot \text{len}_f / \text{avg\_len}_f}$，再统一经过 `k1` 饱和与 Lucene 非负平滑 IDF $\ln(1 + \frac{N - \text{df} + 0.5}{\text{df} + 0.5})$，彻底避免多字段线性叠加破坏词频饱和特性。
- **常量与数据模型**：
  - `BM25F_MAX_FIELDS = 4`、`BM25F_MAX_DOCS = 128`、`BM25F_MAX_POSTINGS = 2048`
  - `Bm25fFieldSpec`（`weight_f32`, `b_f32`, `isValid()`）、`Bm25fConfig`（`k1_f32`, `field_count`, `fields`, `defaultsCodeSearch()`, `isValid()`）、`Bm25fHit`（`doc_id`, `score_f32`, `matched_terms`）、`Bm25fIndexStats`（`active_docs`, `posting_count`, `field_count`, `avg_field_lens`）。
- **核心引擎（`Bm25fIndex`）**：
  - `init(cfg): Bm25fIndex?`
  - `add_document(*_, doc_id, field_texts): bool`（ASCII 大小写折叠分词 + FNV-1a 64-bit 哈希，ID 去重与倒排池溢出原子回滚）
  - `remove_document(*_, doc_id): bool`（逻辑摘除并同步维护 `active_docs`、`posting_count` 与 `avg_field_lens`）
  - `search_topk(query_text, k, out_hits): usize`（查询词去重、BM25F 打分降序 Top-K、平局按 `doc_id` 升序决胜）
  - `stats(): Bm25fIndexStats`

### BM25F 代码/文档多字段检索 QuickStart 示例

```cp
import {
    Bm25fConfig,
    Bm25fHit,
    Bm25fIndex,
} from "cap:vector";

pub fn demoBm25fCodeSearch() {
    let idx = Bm25fIndex.init(Bm25fConfig.defaultsCodeSearch()) ?? return;

    // 4 字段: [title, symbol, body, tags]
    const doc1: [4][]const u8 = ["Paged KV Cache", "PagedKvAllocator", "manages physical blocks", "attention kv"];
    const doc2: [4][]const u8 = ["Runtime Notes", "init_runtime", "mentions PagedKvAllocator in passing", "docs"];
    * = idx.add_document(101, doc1[0..]);
    * = idx.add_document(102, doc2[0..]);

    let hits: [2]Bm25fHit = [Bm25fHit{}; 2];
    let hits_s: []Bm25fHit = hits[0..];
    const n = idx.search_topk("PagedKvAllocator", 2, hits_s);
    // hits[0].doc_id == 101 (symbol 字段高权重显著领先)
    * = n;
}
```

---

## 零分配 1-Bit 二值量化向量检索索引（`@popCount` 汉明粗排 + `@dot` Float32 精排，`binIndex.cp`）

面向 OpenAI `text-embedding-3` / Cohere v3 / BGE-M3 **32x 显存/内存压缩**的 100% 纯 CP 零动态堆分配 1-Bit 二值量化向量索引与两阶段检索内核（固定 256 文档位图内联池，支持最高 1024 维）：

- **常量与数据模型**：
  - `BIN_IDX_MAX_DOCS = 256`、`BIN_IDX_MAX_WORDS = 16`（支持 `1..=1024` 维）
  - `BinaryIndexConfig`（`dim`, `threshold_f32`, `wordsPerVec()`, `isValid()`）、`BinaryIndexHit`（`doc_id`, `hamming_dist`, `approx_cosine_f32`, `exact_score_f32`）、`BinaryIndexStats`（`active_docs`, `dim`, `words_per_vec`, `bytes_per_vec`, `compression_ratio_x`）
- **核心引擎（`BinaryVectorIndex`）**：
  - `init(cfg): BinaryVectorIndex?`
  - `quantize_vec(vec, out_words): bool`：按 `vec[i] >= threshold_f32` 打包为小端 64 位字（拦截非有限值与维度不匹配）。
  - `add_document(*_, doc_id, vec): bool` / `remove_document(*_, doc_id): bool`：ID 去重写入、逻辑摘除与槽位复用。
  - `search_hamming_topk(query, k, out_hits): usize`：Stage 1 硬件 `@popCount(q_word ^ d_word)` 汉明粗排（同步计算 `approx_cosine_f32 = 1.0 - 2.0 * hamming / dim`，按 `(hamming_dist 升序, doc_id 升序)` 输出 Top-K）。
  - `search_two_stage_rescore(query, raw_doc_pool, k, oversample_k, out_hits): usize`：先汉明粗排取 `oversample_k` 候选，再利用 CP 原生 `@dot` SIMD 在调用方 `raw_doc_pool`（按槽位索引）上精排输出精确 Top-K。
  - `stats(): BinaryIndexStats`：返回活跃文档数、每向量字节数与精确压缩比（64 的倍数维精确 `32.0x`）。

### 1-Bit 二值量化两阶段检索 QuickStart 示例

```cp
import {
    BinaryIndexConfig,
    BinaryIndexHit,
    BinaryVectorIndex,
} from "cap:vector";

pub fn demoBinaryVectorIndex(query: []const f32, raw_pool: []const f32) {
    let idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 64, threshold_f32: 0.0 }) ?? return;
    * = idx.add_document(101, raw_pool[0..64]);
    * = idx.add_document(102, raw_pool[64..128]);

    let hits: [2]BinaryIndexHit = [BinaryIndexHit{}; 2];
    let hits_s: []BinaryIndexHit = hits[0..];
    // Stage 1 粗排 + Stage 2 @dot SIMD 精排
    const n = idx.search_two_stage_rescore(query, raw_pool, 2, 8, hits_s);
    * = n;
    * = idx.stats().compression_ratio_x; // 32.0
}
```

---

## 零分配 SIMD 语义意图路由器（Semantic Intent Router，`semRouter.cp`）

面向 AI Agent 微秒级前置意图分类的 100% 纯 CP 零动态堆分配 SIMD 语义路由器（无需等待 500ms LLM 推理即可完成高频工具/技能路由与歧义降级）：

- **常量与枚举**：
  - `SEM_ROUTER_MAX_ROUTES = 32`、`SEM_ROUTER_MAX_EXEMPLARS = 128`、`SEM_ROUTER_NAME_MAX_LEN = 48`
  - `SemanticAggMode`（`max_exemplar` 取同路由全部例句最大余弦 | `centroid_mean` 取同路由例句均值余弦）
  - `SemanticRouteStatus`（`matched | below_threshold | ambiguous_margin | no_routes | invalid_query`）
- **数据模型**：
  - `SemanticRouterConfig`（`dim`, `agg_mode`, `min_margin_f32`；提供 `isValid()`）
  - `SemanticRouteHit`（`route_id`, `score_f32`, `passed_threshold`）
  - `SemanticRouteDecision`（`status`, `route_id`, `best_score_f32`, `second_score_f32`, `margin_f32`）
  - `SemanticRouterStats`（`route_count`, `exemplar_count`, `dim`, `total_queries`, `matched_count`, `fallback_count`）
- **核心路由器（`SemanticIntentRouter`）**：
  - `init(cfg, vec_pool: *[]f32): SemanticIntentRouter?`：复用调用方扁平 `f32` 例句缓冲，零动态堆分配。
  - `register_route(*_, route_id, name, min_threshold_f32): bool`：唯一 ID 去重、独立置信度阈值 `[-1, 1]` 与 32 路由上限校验。
  - `add_exemplar(*_, route_id, embedding): bool`：自动 `@dot` + `@sqrt` L2 归一化写入扁平池，支持每路由多例句。
  - `route_query(*_, query_embedding, out_top_hits: *[]SemanticRouteHit): SemanticRouteDecision`：对查询向量缩放不变归一化，基于 8-lane SIMD `@dot` 计算每路由聚合分，按 `(score_f32 降序, route_id 升序)` 填充 `out_top_hits`，并严格执行每路由 `min_threshold_f32` 与 Top-1/Top-2 `min_margin_f32` 歧义防误路由守卫。

### SIMD 语义意图路由器 QuickStart 示例

```cp
import {
    SemanticAggMode,
    SemanticRouteStatus,
    SemanticRouterConfig,
    SemanticRouteHit,
    SemanticIntentRouter,
} from "cap:vector";

pub fn demoSemanticIntentRouter() {
    let pool: [64]f32 = [0.0; 64]; // 16 exemplars x 4D
    let pool_s: []f32 = pool[0..];
    const cfg = SemanticRouterConfig{ dim: 4, agg_mode: SemanticAggMode.max_exemplar, min_margin_f32: 0.08 };
    let router = SemanticIntentRouter.init(cfg, pool_s) ?? return;

    * = router.register_route(10, "billing_refund", 0.75);
    * = router.register_route(20, "code_review", 0.75);

    const ex_bill: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const ex_code: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    * = router.add_exemplar(10, ex_bill[0..]);
    * = router.add_exemplar(20, ex_code[0..]);

    const q: [4]f32 = [0.95, 0.10, 0.0, 0.0];
    let hits: [2]SemanticRouteHit = [SemanticRouteHit{}; 2];
    let hits_s: []SemanticRouteHit = hits[0..];
    const decision = router.route_query(q[0..], hits_s);
    if (decision.status == SemanticRouteStatus.matched) {
        * = decision.route_id; // 10 ("billing_refund")
    }
}
```

---

## 零分配 RAPTOR 分层摘要树与多层级联合检索器（RAPTOR Tree，`raptor.cp`）

面向长文档层次化 RAG 的 100% 纯 CP 零动态堆分配 RAPTOR（Recursive Abstractive Processing for Tree-Organized Retrieval）分层摘要树与双模式联合检索引擎：

- **常量与枚举**：
  - `RAPTOR_MAX_NODES = 256`、`RAPTOR_MAX_LEVELS = 8`、`RAPTOR_NO_NODE = 0xFFFF`
  - `RaptorNodeKind`（`leaf_chunk` | `cluster_summary` | `root_abstraction`）
  - `RaptorSearchMode`（`collapsed_tree` 全层级扁平联合检索 | `tree_traversal` 自顶向下逐层剪枝路由）
  - `RaptorScoreFusion`（`linear_weighted` 按层线性加权 | `reciprocal_rank` 按层权重倒数排名融合 RRF）
- **数据模型**：
  - `RaptorTreeConfig`（`dim`, `max_nodes`, `level_weights`, `rrf_k`, `cluster_sim_threshold`；提供 `isValid()`）
  - `RaptorNode`（`node_id`, `slot_idx`, `level`, `kind`, `parent_idx`, `first_child_idx`, `next_sibling_idx`, `child_count`, `token_count`, `cluster_size`, `active`）
  - `RaptorSearchHit`（`node_id`, `slot_idx`, `level`, `kind`, `raw_similarity`, `fused_score`, `l2_sq_dist`, `token_count`）
  - `RaptorClusterAssignment`（`leaf_slot`, `target_parent_slot`, `similarity`, `l2_sq_dist`, `matched_existing`）
  - `RaptorTreeStats`（`total_nodes`, `capacity`, `dim`, `max_level`, `level_counts`, `leaf_count`, `summary_count`, `root_count`, `total_tokens`）
- **核心引擎（`RaptorTree`）**：
  - `init(cfg, vecs_flat_buf: *[]f32): RaptorTree?`：复用调用方扁平 `f32` 向量缓冲，零动态堆分配。
  - `addLeaf(*_, node_id, embedding, token_count): u16?` / `addSummaryNode(...)` / `addSummaryFromChildrenMean(...)` / `attachChild(...)`：构建多层父子摘要树（采用 O(1) First-Child/Next-Sibling/Parent 链，自动 L2 归一化与跨层防环校验）。
  - `findBestClusterAtLevel(embedding, target_level)` / `assignLeafIncremental(*_, node_id, embedding, token_count, target_summary_level, update_centroid)`：基于原生 `@dot` / `@l2sq` 增量余弦相似度聚类归属，并可选通过 `@fma` 原地更新父质心。
  - `traverseBfs(start_slot, out_slots)` / `traverseDfs(start_slot, out_slots)` / `collectChildren(...)` / `collectAncestors(...)`：纯迭代零递归遍历与父子/祖先路径收集。
  - `searchCollapsed(...)` / `searchTreeTraversal(...)` / `search(...)`：支持 Collapsed Tree 全层级扁平 Top-K 与 Tree Traversal 自顶向下 beam 剪枝 Top-K 双模式检索，内置按层权重融合与防重复命中去重。

### RAPTOR 分层摘要树 QuickStart 示例

```cp
import {
    RaptorNodeKind,
    RaptorSearchMode,
    RaptorScoreFusion,
    RaptorTreeConfig,
    RaptorSearchHit,
    RaptorTree,
} from "cap:vector";

pub fn demoRaptorTree() {
    let vec_buf: [128]f32 = [0.0; 128]; // up to 32 nodes x 4D
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{ dim: 4, max_nodes: 32 };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    const e0: [4]f32 = [1.0, 0.1, 0.0, 0.0];
    const e1: [4]f32 = [0.9, 0.2, 0.0, 0.0];
    const l0 = tree.addLeaf(100, e0[0..], 120) ?? return;
    const l1 = tree.addLeaf(101, e1[0..], 110) ?? return;
    const ch: [2]u16 = [l0, l1];
    const s0 = tree.addSummaryFromChildrenMean(200, 1, RaptorNodeKind.cluster_summary, 64, ch[0..]) ?? return;
    const rch: [1]u16 = [s0];
    * = tree.addSummaryFromChildrenMean(300, 2, RaptorNodeKind.root_abstraction, 32, rch[0..]);

    const q: [4]f32 = [0.98, 0.12, 0.0, 0.0];
    let hits: [4]RaptorSearchHit = [RaptorSearchHit{}; 4];
    let hits_s: []RaptorSearchHit = hits[0..];
    const n = tree.search(q[0..], RaptorSearchMode.tree_traversal, RaptorScoreFusion.linear_weighted, 3, 2, *hits_s);
    * = n;
}
```


---

## 零分配单阶段谓词感知过滤图检索与自适应回退引擎（Filtered HNSW，`filtHnsw.cp`）

面向多租户、多标签 RAG 元数据过滤的 100% 纯 CP 零动态堆分配 ACORN / Filtered-DiskANN 风格图检索引擎（原生 `@dot`、`@l2sq` 与 `@popCount` 驱动）：

- **常量与枚举**：
  - `FHNSW_MAX_NODES = 256`、`FHNSW_MAX_DEGREE = 16`、`FHNSW_INVALID_SLOT = 0xFFFF`、`FHNSW_ANY_TENANT = 0`
  - `FilteredSearchPath`（`graph_filtered | exact_fallback | empty_result`）
  - `FilteredGraphStatus`（`ok | invalid_dimension | invalid_vector | index_full | duplicate_doc_id | node_not_found`）
- **数据模型**：
  - `FilteredGraphConfig`（`dim`, `max_nodes`, `max_degree`, `ef_construction`, `exact_fallback_bps`）
  - `FilteredPredicate`（`required_tags`, `forbidden_tags`, `tenant_id`；`matches(tag_mask, node_tenant)`）
  - `FilteredGraphNode`、`FilteredSearchHit`、`FilteredSearchReport`（执行路径、命中数、选择率 `selectivity_bps`、访问节点与距离计算数）、`FilteredGraphStats`
- **核心引擎（`FilteredHnswIndex`）**：
  - `init(cfg, vecs_flat_buf)` / `insert(*_, doc_id, embedding, tag_mask, tenant_id)`：贪心近邻构图与双向受限度数边（`M <= 16`）。
  - `updateNodeTags(*_, doc_id, new_tag_mask)` / `markDeleted(*_, doc_id)` / `stats()`：在线元数据热更新与保连通逻辑删除。
  - `search(query, pred, k, ef_search, out_hits)`：通过 `@popCount` 计算过滤选择率；高选择率走单阶段谓词感知图遍历（穿越非匹配/墓碑节点保持连通），极低选择率自动回退至精确掩码 SIMD `@dot` 扫描保证 100% Recall。

### Filtered HNSW QuickStart 示例

```cp
import {
    FilteredGraphConfig,
    FilteredPredicate,
    FilteredSearchHit,
    FilteredHnswIndex,
} from "cap:vector";

pub fn demoFilteredHnsw() {
    let vec_buf: [256]f32 = [0.0; 256]; // up to 64 nodes x 4D
    let vec_s: []f32 = vec_buf[0..];
    let idx = FilteredHnswIndex.init(FilteredGraphConfig{ dim: 4, max_nodes: 64, max_degree: 8 }, *vec_s) ?? return;

    const e0: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e1: [4]f32 = [0.9, 0.2, 0.0, 0.0];
    * = idx.insert(101, e0[0..], 0b0011, 1);
    * = idx.insert(102, e1[0..], 0b0001, 1);

    let hits: [4]FilteredSearchHit = [FilteredSearchHit{}; 4];
    let hits_s: []FilteredSearchHit = hits[0..];
    const pred = FilteredPredicate{ required_tags: 0b0001, forbidden_tags: 0b0010, tenant_id: 1 };
    const rep = idx.search(e0[0..], pred, 2, 16, *hits_s);
    * = rep.hit_count; // 精确命中 doc_id=102，过滤掉含禁止标签 0b0010 的 doc_id=101
}
```


---

## 零分配长上下文 RAG 延迟分块与 Token 区间池化引擎（Late Chunking，`lateChunk.cp`）

面向长上下文 RAG（保留跨段指代与篇章上下文语义）的 100% 纯 CP 零动态堆分配延迟分块池化引擎，全程基于原生 8-lane SIMD `@dot` 与单舍入硬件融合乘加 `@fma`：

- **数据模型与策略枚举**：
  - `LateChunkPoolMode`：`mean_pool` | `weighted_attention_pool` | `max_norm_pool`
  - `LateChunkSpan`：`{ start_token: usize, end_token: usize, boundary_sim: f32 }`，提供 `tokenCount(): usize` 与 `isValid(num_tokens: usize): bool`
  - `LateChunkHit`：`{ chunk_idx: usize, score: f32 }`
  - `LateChunkStats`：`{ num_tokens, num_chunks, dim, min_chunk_len, max_chunk_len, zero_norm_chunks }`
- **清洗与安全归一化原语**：
  - `sanitizeVectorF32(src, *dst): usize?`：将 `NaN`/`Inf` 清洗为 `0.0` 并返回清洗计数。
  - `normalizeVectorInPlaceF32(*vec): bool`：先清洗非有限值再基于 `@dot` + `@sqrt` 原位 L2 归一化；零向量安全保持全零并返回 `false`。
- **核心引擎 API**：
  - `detectSemanticBoundaries(token_embeddings, num_tokens, dim, min_chunk_tokens, max_chunk_tokens, drop_threshold, *out_spans): usize?`：通过 `@dot` 计算相邻 Token 余弦相似度，在 `cos < drop_threshold` 的语义低谷自动切分并严格遵守 `[min_chunk_tokens, max_chunk_tokens]` 长度上下界。
  - `poolLateChunkSpan(token_embeddings, num_tokens, dim, span, mode, token_weights, l2_normalize, *out_vec): bool` / `poolLateChunks(..., *out_chunk_vecs): usize?`：支持均池化、外部注意力权重或自适应 L2 范数显著度加权池化（`@fma` 累积）及最大范数池化，可选自动零向量安全 L2 归一化。
  - `searchLateChunks(query_vec, chunk_vecs, num_chunks, dim, *out_hits): usize?`：零堆分配文档内 Top-K 余弦检索，按 `score` 降序、同分按 `chunk_idx` 升序确定性输出。
  - `computeLateChunkStats(spans, num_tokens, dim, chunk_vecs): LateChunkStats?`：汇总块长分布与零范数块计数。

### Late Chunking QuickStart 示例

```cp
import {
    LateChunkPoolMode,
    LateChunkSpan,
    LateChunkHit,
    detectSemanticBoundaries,
    poolLateChunks,
    searchLateChunks,
} from "cap:vector";

pub fn demoLateChunking(token_embeddings: []const f32, num_tokens: usize, dim: usize, query: []const f32) {
    let spans_buf: [16]LateChunkSpan = [LateChunkSpan{}; 16];
    let spans_s: []LateChunkSpan = spans_buf[0..];
    const n_spans = detectSemanticBoundaries(token_embeddings, num_tokens, dim, 2, 8, 0.5, *spans_s) ?? return;

    let chunks_buf: [64]f32 = [0.0; 64]; // up to 16 chunks x 4D
    let chunks_s: []f32 = chunks_buf[0..];
    const empty_w: [0]f32 = [];
    const n_chunks = poolLateChunks(
        token_embeddings, num_tokens, dim, spans_buf[0..n_spans],
        LateChunkPoolMode.mean_pool, empty_w[0..], true, *chunks_s,
    ) ?? return;

    let hits_buf: [3]LateChunkHit = [LateChunkHit{}; 3];
    let hits_s: []LateChunkHit = hits_buf[0..];
    const n_hits = searchLateChunks(query, chunks_buf[0..], n_chunks, dim, *hits_s) ?? 0;
    * = n_hits;
}
```


---

## 零分配 8-bit 非对称标量量化向量索引（SQ8，`sq8Quant.cp`）

面向高性价比 4x 显存/内存压缩向量检索的 100% 纯 CP 零动态堆分配 **8-bit Asymmetric Scalar Quantization (SQ8)** 引擎：

- **常量与数据模型**：
  - `SQ8_MAX_DIM = 64`、`SQ8_MAX_DOCS = 256`（16 KiB 内联 `u8` 压缩池，4x 压缩对比 64 KiB `f32`）
  - `Sq8Metric`：`dot`（内积降序）| `l2_sq`（平方欧氏距离升序）
  - `Sq8Codebook`：逐维 `min_vals`、`scales`、`inv_scales`（零方差维度自动安全退化为 `scale = 0.0` 零除零）
  - `Sq8SearchHit`、`Sq8ErrorStats`（`mse`, `mae`, `max_abs_err`, `cosine_fidelity`, `compression_ratio`）、`Sq8IndexStats`
- **校准器与编解码核**：
  - `Sq8Calibrator`：`init(dim, clip_quantile)`、`observe(*_, vec)`、`finalize(): Sq8Codebook?`（增量流式 Min/Max + Welford 均值/方差可选极值截断）
  - `sq8CalibrateBatch(vecs_flat, n_vecs, dim, clip_quantile): Sq8Codebook?`：两遍 64-bin 零堆直方图 1%~99% 极值截断校准
  - `encodeF32ToU8` / `decodeU8ToF32`：4x 压缩量化与反量化
  - `sq8AsymDotF32` / `sq8AsymL2SqF32` / `sq8SymDotU8`：非对称（`f32` vs `u8`）与对称（`u8` vs `u8`）距离核
  - `evaluateSq8Error(codebook, orig_vecs, n_vecs): Sq8ErrorStats?`：批量量化重建保真度评估
- **核心索引 `Sq8VectorIndex`**：
  - `init(codebook)` / `addVector(*_, doc_id, vec)` / `addQuantized(*_, doc_id, codes)` / `clear(*_)` / `stats()`
  - `searchTopK(query, metric, top_k, *out_hits): usize?`：一阶段 SQ8 非对称粗排 Top-K
  - `rescoreTopK(query, raw_vecs_flat, metric, coarse_k, final_k, *out_hits): usize?`：两阶段 SQ8 粗排 + 外部原始 `f32` 精确重打分。

```cp
import {
    Sq8Metric,
    Sq8Calibrator,
    Sq8SearchHit,
    Sq8VectorIndex,
} from "cap:vector";

pub fn demoSq8VectorIndex() {
    let cal = Sq8Calibrator.init(4, 0.0) ?? return;
    const v1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    * = cal.observe(v1[0..]);
    * = cal.observe(v2[0..]);
    const cb = cal.finalize() ?? return;

    let idx = Sq8VectorIndex.init(cb) ?? return;
    * = idx.addVector(101, v1[0..]);
    * = idx.addVector(102, v2[0..]);

    let hits: [2]Sq8SearchHit = [Sq8SearchHit{}; 2];
    let hits_s: []Sq8SearchHit = hits[0..];
    * = idx.searchTopK(v1[0..], Sq8Metric.dot, 2, *hits_s);
}
```

---

### 零分配多向量延迟交互文档检索与 Token 对齐归因索引 (`multiVec.cp`)

面向 ColBERT / PLAID / 可解释 RAG 高亮重排的 100% 纯 CP 零动态堆分配多向量延迟交互（Chamfer / MaxSim）检索索引：

- **常量与枚举**：
  - `MULTIVEC_MAX_DOCS = 64`、`MULTIVEC_MAX_DIM = 32`、`MULTIVEC_ARENA_FLOATS = 16384`（64 KiB 连续 `f32` Token 嵌入 Arena）
  - `MultiVecScoreMode`（`weighted_maxsim` | `chamfer_symmetric`）
- **数据模型**：
  - `MultiVecHit`（`doc_id`, `score`, `coarse_score`, `q2d_maxsim`, `d2q_maxsim`, `token_count`）
  - `MultiVecTokenAlign`（`query_token_idx`, `best_doc_token_idx`, `dot_score`, `weighted_contribution`）
  - `MultiVecIndexStats`（`dim`, `doc_capacity`, `doc_count`, `total_tokens`, `arena_used_floats`, `total_searches`, `total_coarse_pruned`）
- **核心索引 `MultiVecIndex`**：
  - `init(dim)` / `clear(*_)` / `stats()`
  - `addDocument(*_, doc_id, doc_tokens_flat, normalize_tokens)`: 写入变长多向量文档（支持可选逐 Token L2 归一化），自动维护 L2 归一化文档质心 `centroid`，并内置 NaN/Inf、维度对齐、重复 ID 与 Arena 容量原子防护。
  - `searchTwoStage(*_, query_tokens_flat, query_weights, mode, coarse_k, top_k, *out_hits)`: 两阶段 SIMD `@dot` 检索——阶段 1 基于查询质心与文档质心快速粗排剪枝至 Top-`coarse_k`；阶段 2 执行带权重 MaxSim（`sum_i w_i * max_j @dot(q_i, d_j)`）或对称 Chamfer 双向打分并降序输出 Top-`top_k`。
  - `explainAlignment(doc_id, query_tokens_flat, query_weights, *out_pairs)`: 精确输出每个查询 Token 命中的最佳文档 Token 索引与加权点积贡献，支撑可解释 RAG 证据高亮。

```cp
import {
    MultiVecScoreMode,
    MultiVecHit,
    MultiVecTokenAlign,
    MultiVecIndex,
} from "cap:vector";

pub fn demoMultiVecIndex() {
    let idx = MultiVecIndex.init(4) ?? return;
    // Doc 1: 2 tokens ([1,0,0,0], [0,1,0,0])
    const d1: [8]f32 = [1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0];
    * = idx.addDocument(1, d1[0..], true);

    const q: [8]f32 = [0.0, 1.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0];
    const w: [2]f32 = [2.0, 1.0];
    let hits: [2]MultiVecHit = [MultiVecHit{}; 2];
    let hits_s: []MultiVecHit = hits[0..];
    * = idx.searchTwoStage(q[0..], w[0..], MultiVecScoreMode.weighted_maxsim, 4, 2, *hits_s);

    let align: [2]MultiVecTokenAlign = [MultiVecTokenAlign{}; 2];
    let align_s: []MultiVecTokenAlign = align[0..];
    * = idx.explainAlignment(1, q[0..], w[0..], *align_s); // q[0]->d[1] (2.0), q[1]->d[0] (1.0)
}
```

---

### 零分配流式语义近重复检测与在线质心聚类去重引擎 (`semanticDedup.cp`)

面向 RAG 语料入库清洗的 100% 纯 CP 零动态堆分配流式语义去重与在线质心聚类引擎：

- **常量与枚举**：
  - `SEM_DEDUP_MAX_CLUSTERS = 128`、`SEM_DEDUP_MAX_DIM = 64`、`SEM_DEDUP_BPS_SCALE = 10000`
  - `SemanticDedupStatus`（`novel | duplicate | capacity_full | invalid_vector`）
- **数据模型**：
  - `SemanticDedupConfig`（`make(dim, sim_threshold, update_centroid)`, `isValid()`）
  - `SemanticCluster`（归一化质心、`canonical_doc_id`、`member_count`、`min_member_sim`、`avgMemberSim()`）
  - `SemanticDedupDecision`、`SemanticClusterHit`、`SemanticDedupBatchReport`、`SemanticDedupStats`（含 `dedup_ratio_bps`）
- **核心引擎 `SemanticDeduplicator`**：
  - `init(cfg)` / `reset(*_)` / `getClusterById(cluster_id)` / `stats()`
  - `ingest(*_, doc_id, vec): SemanticDedupDecision`：单遍 L2 归一化（防零向量/NaN/Inf/维度不匹配）+ 原生 SIMD `@dot` 余弦相似度匹配，命中 `>= sim_threshold` 判为 `duplicate` 并按配置滑动更新质心，否则建新簇返回 `novel`。
  - `dedupBatch(*_, doc_ids, vecs_flat, out_kept_ids): SemanticDedupBatchReport`：批量入库清洗并输出保留文档 ID 与本批去重率万分比。
  - `findNearestClusters(vec, top_k, out_hits): usize`：降序召回 Top-K 最相近语义簇。
  - `mergeClusters(*_, merge_threshold): usize`：按成员数加权合并相近质心簇并释放槽位。

```cp
import {
    SemanticDedupConfig,
    SemanticDedupStatus,
    SemanticDeduplicator,
} from "cap:vector";

pub fn demoSemanticDedup() {
    let dedup = SemanticDeduplicator.init(SemanticDedupConfig.make(4, 0.92, true)) ?? return;
    const v1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v2: [4]f32 = [0.99, 0.14, 0.0, 0.0]; // cos ~= 0.990
    const v3: [4]f32 = [0.0, 1.0, 0.0, 0.0];

    const d1 = dedup.ingest(101, v1[0..]); // novel (cluster 1)
    const d2 = dedup.ingest(102, v2[0..]); // duplicate -> canonical_doc_id == 101
    const d3 = dedup.ingest(103, v3[0..]); // novel (cluster 2)
    * = (d1.status == SemanticDedupStatus.novel);
    * = (d2.status == SemanticDedupStatus.duplicate);
    * = (d3.status == SemanticDedupStatus.novel);
}
```
