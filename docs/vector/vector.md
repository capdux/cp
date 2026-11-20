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
| `lib/vector/hybrid.cp` | `// Source: cp/lib/vector/hybrid.cp (Reciprocal Rank Fusion Hybrid RAG Ranker)` | `RrfFusion.fuse` 混合检索 RRF 融合排序 |
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

### 3. Reciprocal Rank Fusion 混合重排 (`hybrid.cp`)

- **`RrfFusion.fuse(alloc, vector_hits, text_hits, rrf_k: f32 = 60.0, top_k: usize = 10): ([]SearchHit)!`**
  - 采用 1-based 排名（`rank = 1, 2, ...`）累加融合分数：`score(doc) = sum(1.0 / (rrf_k + rank))`。
  - 自动按 `id` 去重（单路内部重复亦仅计首次最高排名），保留非空 `meta`（优先向量路元信息），按融合分数降序输出最多 `top_k` 条结果。

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
