# CP 标准库：近重复文档指纹去重引擎手册 (`cap:dedup`)

## 一、 架构总览与设计准则

`cap:dedup` 是 CP 语言官方内置的纯 CP **近重复文档指纹去重引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，为 AI RAG 语料清洗、网页爬虫去重（配合 `cap:feed` / `cap:sitemap` / `cap:text`）与代码相似度筛查提供工业级实现。既可通过 `import { ... } from "cap:dedup"` 导入，也支持 `import cap from "cap"; cap.dedup.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/dedup/dedup.cp` | `// Source: cp/lib/dedup/dedup.cp (...)` | `DedupError`、`SimHashHit`、`LshHit`、`SimHash`、`SimHashIndex`、`MinHash`、`LshIndex` |
| `lib/dedup/index.cp` | `// Source: cp/lib/dedup/index.cp (...)` | `cap:dedup` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与命中结构体

- **`DedupError`**：
  - `InvalidThreshold`：`SimHashIndex.queryFingerprint` 传入 `max_hamming_dist > 64`，或 `LshIndex.queryCandidates` 传入 `NaN` 或超出 `[0.0, 1.0]` 的 `min_jaccard`。
  - `InvalidParams`：`MinHash.computeShingles` 传入 `k_shingle == 0`、`num_hashes == 0` 或 `out_sig.len < num_hashes`；`MinHash.estimateJaccard` 传入空签名或长度不等的签名；`LshIndex.init` 传入 `bands == 0` 或 `rows_per_band == 0`。
  - `OutOfMemory`：底层文档表、签名缓冲或 LSH 分桶哈希表内存分配失败。
- **`SimHashHit`**：`{ doc_id: string, fingerprint: u64, hamming_dist: u8 }`
- **`LshHit`**：`{ doc_id: string, estimated_jaccard: f64 }`

### 2. `SimHash`（64-bit Charikar SimHash 与硬件位计数）

- `SimHash.computeText(text: string): u64`：按词切分（ASCII 大小写折叠 + UTF-8 多字节兼容）+ 64-bit FNV-1a + SplitMix64 终结混洗，累加 64 维符号权重向量并输出 64-bit 指纹。
- `SimHash.computeTokens(tokens: []const string): u64`：直接基于预分词切片计算 64-bit 指纹。
- `SimHash.hammingDistance(a: u64, b: u64): u8`：基于硬件指令 `@popCount(a ^ b)` 零循环计算海明距离。

### 3. `SimHashIndex`（4x16-bit 抽屉分片亚线性近邻索引）

- **鸽巢原理保证**：对于 $k \le 3$ 的海明距离阈值，将 64-bit 指纹切分为 4 个 16-bit 抽屉分片桶，至少有一个 16-bit 分片必然精确命中，实现 **100% 精确召回**而无需全表扫描；当 `max_hamming_dist > 3` 时平滑回退到全表精确扫描。
- `SimHashIndex.init(alloc: std.mem.Allocator): SimHashIndex` / `deinit(*_)` / `count(): usize`
- `add(*_, doc_id: string, text: string): u64 ! DedupError`
- `addFingerprint(*_, doc_id: string, fp: u64): void ! DedupError`
- `queryNearDuplicates(*_, alloc: std.mem.Allocator, text: string, max_hamming_dist: u8): []SimHashHit ! DedupError`（结果按 `hamming_dist` 升序排列；使用 `alloc.free(hits)` 释放结果切片）
- `queryFingerprint(*_, alloc: std.mem.Allocator, fp: u64, max_hamming_dist: u8): []SimHashHit ! DedupError`

### 4. `MinHash` & `LshIndex`（$k$-Shingle Jaccard 近似估算与局部敏感哈希）

- `MinHash.computeShingles(text: string, k_shingle: usize, num_hashes: usize, out_sig: []u64): void ! DedupError`：零堆分配滑动窗口提取字符 $k$-shingle，经 `num_hashes` 个独立 64-bit SplitMix64 种子取最小签名。
- `MinHash.estimateJaccard(sig_a: []const u64, sig_b: []const u64): f64 ! DedupError`：统计相同分量比例估算 Jaccard 相似度。
- `LshIndex.init(alloc: std.mem.Allocator, bands: usize, rows_per_band: usize): LshIndex ! DedupError`（默认 `k_shingle = 4`，`initWithShingle` 支持自定义 $k$ 值）
- `deinit(*_)` / `count(): usize`
- `insert(*_, doc_id: string, text: string): void ! DedupError`
- `queryCandidates(*_, alloc: std.mem.Allocator, text: string, min_jaccard: f64): []LshHit ! DedupError`（先经 LSH 分桶召回候选，再以 `estimateJaccard` 过滤低于 `min_jaccard` 的误报，按 `estimated_jaccard` 降序返回）

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SimHash, SimHashIndex, MinHash, LshIndex } from "cap:dedup";
import cap from "cap";

pub fn demoDedup(): ! {
    const alloc = std.heap.page_allocator;

    let sh_idx = SimHashIndex.init(alloc);
    defer sh_idx.deinit();
    * = sh_idx.add("doc-1", "CP language compiles to native Zig state machines for million agent concurrency")?;
    const hits = sh_idx.queryNearDuplicates(alloc, "CP language compiles to native Zig state machines for million actor concurrency", 3)?;
    defer alloc.free(hits);

    let lsh = cap.dedup.LshIndex.init(alloc, 16, 4)?;
    defer lsh.deinit();
    lsh.insert("rag-1", "Retrieval augmented generation pipeline deduplication in pure CP.")?;
    const cands = lsh.queryCandidates(alloc, "Retrieval augmented generation pipeline deduplication in pure CP!", 0.6)?;
    defer alloc.free(cands);
}
```
