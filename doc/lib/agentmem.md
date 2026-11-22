# CP 标准库：分层长期记忆引擎手册 (`cap:agentmem` / `cap.agentmem`)

## 一、 架构总览与设计准则

`cap:agentmem` 是 CP 语言官方内置的纯原生、零 C 依赖的分层长期记忆引擎（Hierarchical Agent Memory），专为百万级 AI Agent 运行时的上下文记忆管理、经验巩固与记忆淘汰打造：

- **三层记忆模型 (`MemoryTier`)**：
  - `Working`：工作记忆，具备容量上限 (`max_working_items`)，写入超出上限时自动淘汰最久未访问（LRU / FIFO）的工作记忆条目。
  - `Episodic`：情景事件记忆，记录交互事件与轨迹，受时间衰减与遗忘淘汰 (`prune`) 管理。
  - `Semantic`：语义事实记忆，承载经巩固晋升或直接写入的长期核心知识，免疫 `prune` 淘汰。
- **三因子联合召回排序（Stanford Generative Agents 模型）**：
  - **Recency**：基于 `last_accessed_ms` 与半衰期 `decay_half_life_ms` 的指数时间衰减得分 $2^{-\Delta t / T_{1/2}}$；
  - **Importance**：人工或模型标注的重要度分值 `[0.0, 1.0]`；
  - **Relevance**：基于 ASCII 小写字母数字分词的关键词/标签 Jaccard 相似度、查询覆盖率、TF 饱和度与标签命中加权。
  - 每次 `recall` / `recallByTier` 命中返回后自动刷新命中记忆的 `last_accessed_ms = now_ms` 与 `access_count += 1`。
- **巩固与遗忘生命周期**：
  - `consolidate(min_access_count, min_importance)`：将高频且高重要性的 `Working` / `Episodic` 记忆晋升为 `Semantic`。
  - `prune(now_ms, min_recency_plus_importance)`：清理陈旧低分的 `Episodic` 记忆，保留 `Semantic` 核心事实。
- **JSONL 持久化往返**：`exportJsonl(alloc)` 与 `importJsonl(alloc, jsonl_text)` 支持无损序列化导出与重建。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:agentmem"` 与 `import cap from "cap"; cap.agentmem.*` 编译期自动动态挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/agentmem/memory.cp` | `// Source: cp/lib/agentmem/memory.cp (Hierarchical Agent Memory Engine)` | `AgentMemError`、`MemoryTier`、`RecallWeights`、`MemoryItem`、`RecallHit`、`AgentMemory` |
| `lib/agentmem/episodic.cp` | `// Origin: Pure-CP Stdlib (AI Agent Episodic Memory: Park 2023 Recency-Importance-Relevance & Ebbinghaus Decay)` | `MemoryWeights`、`EpisodeMeta`、`ScoredEpisode`、`ebbinghausRetention`、`reinforceStability`、`scoreEpisode`、`rankTopEpisodes`、`selectEvictionVictim`、`touchEpisode` |
| `lib/agentmem/kg.cp` | `// Origin: Pure-CP Stdlib (AI Agent Temporal Knowledge Graph Memory: SPO Triples & 2-Hop Recall)` | `KgEntity`、`KgTriple`、`KgStore`（时序三元组、事实版本失效与 1~2 跳 GraphRAG 邻域检索） |
| `lib/agentmem/workingMem.cp` | `// Origin: Pure-CP Stdlib (AI Agent Working Memory & Token-Budget Context Window Manager)` | `MemSlotRole`、`EvictionPolicy`、`WorkingMemItem`、`WorkingMemStats`、`WorkingMemory`（零堆分配工作记忆与 Token 预算滑动上下文窗） |
| `lib/agentmem/activation.cp` | `// Origin: Pure-CP Stdlib (AI GraphRAG Spreading Activation & Personalized PageRank Memory Recall)` | `MemGraphEdge`、`ActivatedNode`、`SpreadingConfig`、`spreadActivation`、`personalizedPageRank`（零堆分配 GraphRAG 扩散激活与个性化 PageRank 记忆召回） |
| `lib/agentmem/consolidation.cp` | `// Origin: Pure-CP Stdlib (AI Agent Memory Consolidation, Dedup & Retention Pruning Engine)` | `MemConsolidationTier`、`ConsolidatableRecord`、`ConsolidationPolicy`、`ConsolidationReport`、`halfLifeDecayFactor`、`consolidateMemoriesInPlace`（零堆分配睡眠期记忆巩固、去重合并、半衰期衰减、分级晋升与遗忘剪枝归档） |
| `lib/agentmem/retrieval.cp` | `// Origin: Pure-CP Stdlib (AI Agent Three-Factor Memory Retrieval, SIMD Dedup Merge & Prompt Packing)` | `MemoryRetrievalConfig`、`MemoryCandidate`、`MemoryScoredHit`、`MemoryMergeStats`、`MemoryPackResult`、`scoreAndRankMemoriesF32`、`mergeRedundantMemoriesF32`、`packMemoriesForPrompt` |
| `lib/agentmem/entityGraph.cp` | `// Origin: Pure-CP Stdlib (AI Agent Zero-Alloc Temporal Entity Knowledge Graph & Two-Hop Recall Engine)` | `TemporalFactEdge`、`GraphHopHit`、`EntityGraphStats`、`TemporalEntityGraph`（半开时序区间事实管理、原子新旧事实更替 `supersedeFact`、时间点历史快照回溯与 1~2 跳环路去重扩展） |
| `lib/agentmem/summaryTree.cp` | `// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Hierarchical Conversation Summary Tree & Context Compactor)` | `SUMMARY_TREE_MAX_NODES`、`SUMMARY_NO_NODE`、`SummaryNodeKind`、`SummaryTreeNode`、`ContextFrontierPlan`、`SummaryTreeStats`、`HierarchicalSummaryTree`（多级递归对话摘要折叠与零重叠 Token 预算上下文前沿规划） |
| `lib/agentmem/index.cp` | `// Source: cp/lib/agentmem/index.cp (Unified Exports for cap:agentmem)` | `cap:agentmem` 与 `cap.agentmem` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误、枚举与数据结构 (`memory.cp`)

- **`AgentMemError`**：`MemoryNotFound`、`InvalidImportance`、`InvalidCapacity`、`InvalidJsonl`、`OutOfMemory`
- **`MemoryTier`**：`Working` | `Episodic` | `Semantic`
- **`RecallWeights`**：`{ recency_weight: f64 = 1.0, importance_weight: f64 = 1.0, relevance_weight: f64 = 1.0, decay_half_life_ms: f64 = 3600000.0 }`
- **`MemoryItem`**：`{ id: u64, tier: MemoryTier, content: string, tags: []string, importance: f64, created_at_ms: u64, last_accessed_ms: u64, access_count: u32 }`
- **`RecallHit`**：`{ id: u64, tier: MemoryTier, content: string, total_score: f64, recency_score: f64, importance_score: f64, relevance_score: f64 }`
  - `content` 借用自 `AgentMemory` 内部存储；调用方需对 `recall` / `recallByTier` 返回的 `[]RecallHit` 执行 `alloc.free(hits)`。

### 2. `AgentMemory` 方法

- `AgentMemory.init(alloc, max_working_items: usize): AgentMemory ! AgentMemError`（`max_working_items == 0` 返回 `AgentMemError.InvalidCapacity`）
- `deinit(*_)`：释放全部内部记忆与标签内存。
- `len(): usize` / `countByTier(tier: MemoryTier): usize` / `get(id: u64): MemoryItem?` / `remove(*_, id: u64): bool`
- `add(*_, tier: MemoryTier, content: string, tags: []const string, importance: f64, now_ms: u64): u64 ! AgentMemError`
  - 校验 `0.0 <= importance <= 1.0`（否则返回 `AgentMemError.InvalidImportance`）。
  - `Working` 层达到 `max_working_items` 时自动淘汰 `last_accessed_ms` 最小的工作记忆。
- `recall(*_, alloc, query: string, now_ms: u64, limit: usize, weights: RecallWeights = RecallWeights{}): ([]RecallHit) ! AgentMemError`
- `recallByTier(*_, alloc, tier: MemoryTier, query: string, now_ms: u64, limit: usize, weights: RecallWeights = RecallWeights{}): ([]RecallHit) ! AgentMemError`
- `consolidate(*_, min_access_count: u32, min_importance: f64): usize`
- `prune(*_, now_ms: u64, min_recency_plus_importance: f64): usize` / `pruneWithHalfLife(*_, now_ms: u64, min_recency_plus_importance: f64, decay_half_life_ms: f64): usize`
- `exportJsonl(alloc): ([]u8) ! AgentMemError`
- `AgentMemory.importJsonl(alloc, jsonl_text: string): AgentMemory ! AgentMemError` / `AgentMemory.importJsonlWithCapacity(alloc, jsonl_text: string, max_working_items: usize): AgentMemory ! AgentMemError`

### 3. 零堆分配情景记忆衰减与三维检索打分 (`episodic.cp`)

面向无 GC / 固定池高并发 AI Agent 运行时，提供 Park et al. (2023) 三维检索模型与 Ebbinghaus 遗忘衰减的纯值类型零堆分配实现：

- **`MemoryWeights`**：`{ recency: f64 = 1.0, importance: f64 = 1.0, relevance: f64 = 1.0, decay_rate: f64 = 0.05 }`，提供 `MemoryWeights.default()`。有效记忆稳定性 $S = \text{strength} / \text{decay\_rate}$。
- **`EpisodeMeta`**：`{ id: u32, created_at_sec: i64, last_accessed_sec: i64, access_count: u32, importance: f64, strength: f64, pinned: bool }`。
- **`ScoredEpisode`**：`{ id: u32, total_score: f64, recency_score: f64, importance_score: f64, relevance_score: f64 }`。
- **`ebbinghausRetention(elapsed_hours: f64, stability_hours: f64): f64`**：计算 $R = e^{-t / S} \in [0, 1]$，内置 `NaN` / `Inf` / 非正稳定性防御。
- **`reinforceStability(stability: f64, access_count: u32): f64`**：按边际递减间隔重复增益强化记忆稳定性。
- **`scoreEpisode(ep: EpisodeMeta, now_sec: i64, relevance_01: f64, w: MemoryWeights): ScoredEpisode?`**：计算归一化三维联合得分；非法权重、越界分数或 `NaN` 返回 `null`。
- **`rankTopEpisodes(episodes, relevances, now_sec, w, top_k, out_scored: *[]ScoredEpisode): usize?`**：零堆分配按 `total_score` 降序输出 Top-K（同分按 `id` 升序确定性打破平局）。
- **`selectEvictionVictim(episodes, now_sec, w): usize?`**：按最低 `retention * importance` 挑选非 `pinned` 淘汰槽位（全 `pinned` 或空池返回 `null`）。
- **`touchEpisode(ep: *EpisodeMeta, now_sec: i64)`**：命中召回时刷新 `last_accessed_sec`、递增 `access_count` 并通过 `reinforceStability` 强化 `strength`。

### 4. 零堆分配时序知识图谱三元组记忆库 (`kg.cp`)

对标 Zep / Graphiti 时序事实失效模型，支持实体去重、版本化 SPO 三元组、时间点回溯与 1~2 跳 GraphRAG 上下文扩展：

- **`KgEntity`**：`{ id: u32, name: string, kind: string, name_hash: u64 }`
- **`KgTriple`**：`{ subject: u32, predicate_hash: u64, predicate: string, object: u32, confidence: f64, valid_from_ms: i64, valid_until_ms: i64, active: bool }`（`valid_until_ms == 0` 表示当前仍有效）
- **`KgStore`**：
  - `KgStore.init(entities: *[]KgEntity, triples: *[]KgTriple): KgStore`
  - `upsertEntity(*_, name, kind): u32?` / `findEntity(name): u32?`
  - `addFact(*_, subject, predicate, object, confidence, now_ms): usize?`
  - `supersedeFact(*_, subject, predicate, new_object, confidence, now_ms): usize?`（将同一 `(subject, predicate)` 旧事实标记 `valid_until_ms = now_ms, active = false` 并写入新事实，不丢失历史）
  - `queryFactsAt(subject, predicate, at_ms, out_indices: *[]usize): usize`（`predicate == ""` 匹配全部关系，`at_ms == 0` 查当前有效事实，非零查半开区间 `[valid_from_ms, valid_until_ms)` 历史快照）
  - `queryTwoHopNeighbors(start_entity, at_ms, out_entities: *[]u32, visited_s: *[]u8): usize`（零堆分配 1~2 跳去重邻域召回，排除起始点与失效边）

### 5. 零堆分配工作记忆与 Token 预算滑动上下文窗 (`workingMem.cp`)

面向 AI Agent 对话与工具调用上下文窗管理，在固定槽位池与硬性 `token_budget` 上限下提供 `pinned` 提示词保护、三种可切换驱逐策略及原子摘要压缩：

- **`MemSlotRole`**：`System` | `User` | `Assistant` | `Tool` | `Summary`
- **`EvictionPolicy`**：`FifoOldest`（按 `created_turn` 最早淘汰）| `Lru`（按 `last_accessed_turn` 最久未访问淘汰）| `LowestScore`（按 `0.65 * importance + 0.35 / (1 + 0.15 * age_turns)` 综合得分最低淘汰）
- **`WorkingMemItem`**：`{ id: u32, role: MemSlotRole, tokens: u32, importance: f32, created_turn: u32, last_accessed_turn: u32, pinned: bool, content_hash: u64, occupied: bool }`
- **`WorkingMemStats`**：`{ active_items: u32, pinned_items: u32, used_tokens: u32, budget_tokens: u32, evicted_total: u64, compacted_total: u64, utilization_pct: f32 }`
- **`WorkingMemory`**：
  - `WorkingMemory.init(slots: *[]WorkingMemItem, token_budget: u32, policy: EvictionPolicy): WorkingMemory`（重置并接管固定槽位池）
  - `push(*_, role, tokens, importance, turn, pinned, content_hash): u32?`（前置原子可行性预检：单条超预算、非法 `importance` 或 `pinned_tokens + tokens > token_budget` 时直接返回 `null` 且零副作用；否则按 `policy` 驱逐非 `pinned` 条目后写入）
  - `touch(*_, id, turn): bool` / `pin(*_, id): bool` / `unpin(*_, id): bool` / `remove(*_, id): bool`
  - `get(id): WorkingMemItem?` / `contains(id): bool` / `setPolicy(*_, policy)`
  - `compactOldestIntoSummary(*_, count, summary_tokens, turn, summary_hash): u32?`（将最早的至多 `count` 条非 `pinned` 条目原子折叠为一条 `.Summary` 槽位；若折叠后仍超预算则返回 `null` 且零副作用）
  - `stats(): WorkingMemStats`

### 6. 零堆分配 GraphRAG 扩散激活与个性化 PageRank (`activation.cp`)

面向多跳记忆联想召回与知识图谱实体重排，基于调用方栈上双缓冲实现零堆分配图传播：

- **`MemGraphEdge`**：`{ src: u32, dst: u32, weight: f32 }`
- **`ActivatedNode`**：`{ node_id: u32, score: f32 }`
- **`SpreadingConfig`**：`{ decay: f32 = 0.6, hops: u32 = 2, min_threshold: f32 = 0.01, bidirectional: bool = false }`
- **`spreadActivation(num_nodes, edges, seed_nodes, seed_weights, cfg, scores_a, scores_b, out_top_k): usize?`**：从种子节点出发沿加权边进行多跳扩散激活（每跳乘以 `decay` 并截断低于 `min_threshold` 的噪声，支持单/双向传播），按得分降序（同分按 `node_id` 升序）输出 Top-K 激活节点；非法配置或缓冲不足返回 `null`。
- **`personalizedPageRank(num_nodes, edges, seed_nodes, seed_weights, damping, max_iters, tol, rank_cur, rank_next, out_deg, out_top_k): usize?`**：加权个性化 PageRank，自动将悬挂节点（出度为 0）概率质量回收至个性化种子分布（严格保持总概率和为 `1.0`）并支持 L1 残差早停收敛。

### 7. 零堆分配长期记忆睡眠期巩固与剪枝压缩器 (`consolidation.cp`)

面向 AI Agent 后台空闲/睡眠期记忆整理，原地零堆分配完成四阶段记忆巩固周期：

- **`MemConsolidationTier`**：`ShortTerm` | `Episodic` | `Semantic` | `Archived`
- **`ConsolidatableRecord`**：`{ id: u32, content_hash: u64, topic_hash: u64, tier: MemConsolidationTier, strength: f32, recall_count: u32, last_recall_ms: u64, pinned: bool, alive: bool }`
- **`ConsolidationPolicy`**：`{ prune_strength_threshold: f32, promote_min_recalls: u32, promote_min_strength: f32, half_life_ms: u64, max_age_archive_ms: u64 }`（提供 `default()` 与 `isValid()`）
- **`ConsolidationReport`**：`{ merged_duplicates: u32, promoted_count: u32, archived_count: u32, pruned_count: u32, surviving_count: u32 }`
- **`halfLifeDecayFactor(age_ms: u64, half_life_ms: u64): f32`**：计算指数半衰期衰减系数 $2^{-\text{age} / T_{1/2}} \in [0, 1]$。
- **`consolidateMemoriesInPlace(records: *[]ConsolidatableRecord, now_ms: u64, policy: ConsolidationPolicy): ConsolidationReport?`**：
  1. **重复合并**：将相同非零 `content_hash` 的存活条目合并到首条，饱和累加 `recall_count`，取最大 `strength` 与 `last_recall_ms`，继承 `pinned` 与更高层级；
  2. **指数半衰期衰减**：按 $2^{-\Delta t / T_{1/2}}$ 衰减 `strength`（内置时钟回拨保护）；
  3. **分级晋升**：满足 `recall_count >= promote_min_recalls` 且 `strength >= promote_min_strength` 时执行 `ShortTerm -> Episodic -> Semantic` 晋升；
  4. **剪枝与归档**：剪枝低于 `prune_strength_threshold` 的非 `pinned` 非 `Semantic` 条目；将超龄（`age >= max_age_archive_ms`）的非 `pinned` 条目归档至 `Archived`；`pinned` 记忆严格免于剪枝与归档。非法策略返回 `null` 且零副作用。

---

## 三、 使用示例

### 1. 分层长期记忆引擎 (`AgentMemory`) 示例

```typescript
const std = @import("std");
import cap from "cap";
import { MemoryTier, RecallWeights, AgentMemory } from "cap:agentmem";

pub fn demoAgentMemory()! {
    const alloc = std.heap.page_allocator;

    let mem = AgentMemory.init(alloc, 4)?;
    defer mem.deinit();

    const tags_a: [2]string = ["zig", "compiler"];
    const id1 = mem.add(.Episodic, "User prefers zero-copy slices in Zig", &tags_a, 0.85, 1000)?;
    const id2 = mem.add(.Working, "Current task: optimize vector recall", ["rag"], 0.6, 2000)?;
    * = id1;
    * = id2;

    const hits = mem.recall(alloc, "zig zero-copy", 2500, 3, RecallWeights{})?;
    defer alloc.free(hits);

    * = mem.consolidate(1, 0.8);
    * = mem.prune(10000000, 0.5);

    const jsonl = mem.exportJsonl(alloc)?;
    defer alloc.free(jsonl);

    let restored = cap.agentmem.AgentMemory.importJsonl(alloc, jsonl)?;
    defer restored.deinit();
}
```

### 2. 零堆分配情景记忆检索与淘汰 (`episodic.cp`) 示例

```typescript
import {
    MemoryWeights,
    EpisodeMeta,
    ScoredEpisode,
    rankTopEpisodes,
    selectEvictionVictim,
    touchEpisode,
} from "cap:agentmem";

pub fn demoEpisodicPool() {
    const now_sec: i64 = 1700000000;
    const w = MemoryWeights.default();

    let pool: [3]EpisodeMeta = [
        EpisodeMeta{ id: 1, created_at_sec: now_sec - 3600, last_accessed_sec: now_sec - 3600, access_count: 1, importance: 0.9, strength: 1.0, pinned: true },
        EpisodeMeta{ id: 2, created_at_sec: now_sec - 7200, last_accessed_sec: now_sec - 7200, access_count: 0, importance: 0.6, strength: 1.0, pinned: false },
        EpisodeMeta{ id: 3, created_at_sec: now_sec - 86400, last_accessed_sec: now_sec - 86400, access_count: 0, importance: 0.2, strength: 1.0, pinned: false },
    ];
    const rels: [3]f64 = [0.7, 0.95, 0.1];

    // 1. 零堆分配 Top-2 检索
    let top_buf: [2]ScoredEpisode = undefined;
    let top_slice: []ScoredEpisode = top_buf[0..];
    if (n := rankTopEpisodes(pool[0..], rels[0..], now_sec, w, 2, top_slice)) {
        * = n;
        // 2. 命中召回后强化记忆稳定性
        touchEpisode(*pool[1], now_sec);
    }

    // 3. 池满时跳过 pinned 槽位挑选最该淘汰的旧记忆
    if (victim_idx := selectEvictionVictim(pool[0..], now_sec, w)) {
        * = victim_idx;
    }
}
```

### 3. 时序知识图谱事实更新与 2 跳 GraphRAG 召回 (`kg.cp`) 示例

```typescript
import { KgEntity, KgTriple, KgStore } from "cap:agentmem";

pub fn demoTemporalKg() {
    let ents: [8]KgEntity = undefined;
    let tris: [16]KgTriple = undefined;
    let ent_slice: []KgEntity = ents[0..];
    let tri_slice: []KgTriple = tris[0..];

    let kg = KgStore.init(ent_slice, tri_slice);
    const alice = kg.upsertEntity("Alice", "person") ?? 0;
    const org_a = kg.upsertEntity("OrgA", "org") ?? 0;
    const org_b = kg.upsertEntity("OrgB", "org") ?? 0;

    // t=100 写入旧事实，t=200 更新事实（旧事实自动失效但保留历史）
    * = kg.addFact(alice, "works_at", org_a, 0.9, 100);
    * = kg.supersedeFact(alice, "works_at", org_b, 0.95, 200);

    // 回溯 t=150 查到 OrgA，查当前 (at_ms=0) 查到 OrgB
    let idx_buf: [4]usize = undefined;
    let idx_slice: []usize = idx_buf[0..];
    * = kg.queryFactsAt(alice, "works_at", 150, idx_slice);
    * = kg.queryFactsAt(alice, "works_at", 0, idx_slice);

    // 2 跳 GraphRAG 关联实体召回
    let nb_buf: [8]u32 = undefined;
    let vis_buf: [8]u8 = undefined;
    let nb_slice: []u32 = nb_buf[0..];
    let vis_slice: []u8 = vis_buf[0..];
    * = kg.queryTwoHopNeighbors(alice, 0, nb_slice, vis_slice);
}
```

### 4. 工作记忆与 Token 预算滑动上下文窗 (`workingMem.cp`) 示例

```typescript
import {
    MemSlotRole,
    EvictionPolicy,
    WorkingMemItem,
    WorkingMemory,
} from "cap:agentmem";

pub fn demoWorkingMemory() {
    let pool: [8]WorkingMemItem = undefined;
    let pool_slice: []WorkingMemItem = pool[0..];

    let wm = WorkingMemory.init(pool_slice, 1000, .Lru);

    // 1. 写入 pinned System Prompt（永不被自动驱逐或折叠）
    * = wm.push(.System, 200, 1.0, 1, true, 0x1111);

    // 2. 写入对话与工具消息
    const usr_msg = wm.push(.User, 300, 0.6, 2, false, 0x2222) ?? 0;
    * = wm.push(.Assistant, 300, 0.7, 3, false, 0x3333);

    // 3. 刷新 usr_msg 访问轮次，使其在 LRU 下免于优先淘汰
    * = wm.touch(usr_msg, 4);

    // 4. 原子折叠最早的 2 条非 pinned 消息为一条 120 token 的 Summary
    * = wm.compactOldestIntoSummary(2, 120, 5, 0x9999);

    const st = wm.stats();
    * = st;
}
```

### 5. GraphRAG 扩散激活与个性化 PageRank 多跳记忆召回 (`activation.cp`) 示例

```cp
import {
    MemGraphEdge,
    ActivatedNode,
    SpreadingConfig,
    spreadActivation,
    personalizedPageRank,
} from "cap:agentmem";

pub fn demoGraphRagActivation() {
    const edges: [3]MemGraphEdge = [
        MemGraphEdge{ src: 0, dst: 1, weight: 0.9 },
        MemGraphEdge{ src: 1, dst: 2, weight: 0.8 },
        MemGraphEdge{ src: 0, dst: 3, weight: 0.3 },
    ];
    const seeds: [1]u32 = [0];
    const seed_w: [1]f32 = [1.0];

    let buf_a: [4]f32 = [0.0; 4];
    let buf_b: [4]f32 = [0.0; 4];
    let deg_buf: [4]f32 = [0.0; 4];
    let top_buf: [4]ActivatedNode = [ActivatedNode{}; 4];

    // 1. 2 跳扩散激活召回
    const cfg = SpreadingConfig{ decay: 0.5, hops: 2, min_threshold: 0.05, bidirectional: false };
    * = spreadActivation(4, edges[0..], seeds[0..], seed_w[0..], cfg, buf_a[0..], buf_b[0..], top_buf[0..]);

    // 2. Personalized PageRank 实体关联重排（含悬挂节点回收与早停）
    * = personalizedPageRank(4, edges[0..], seeds[0..], seed_w[0..], 0.85, 50, 0.0001, buf_a[0..], buf_b[0..], deg_buf[0..], top_buf[0..]);
}
```

### 6. 长期记忆睡眠期巩固、去重合并与遗忘剪枝 (`consolidation.cp`) 示例

```cp
import {
    MemConsolidationTier,
    ConsolidatableRecord,
    ConsolidationPolicy,
    consolidateMemoriesInPlace,
} from "cap:agentmem";

pub fn demoMemoryConsolidation() {
    let pool: [3]ConsolidatableRecord = [
        ConsolidatableRecord{
            id: 1, content_hash: 0xAA01, topic_hash: 10,
            tier: MemConsolidationTier.ShortTerm, strength: 0.75,
            recall_count: 2, last_recall_ms: 9000, pinned: false, alive: true,
        },
        ConsolidatableRecord{
            id: 2, content_hash: 0xAA01, topic_hash: 10,
            tier: MemConsolidationTier.ShortTerm, strength: 0.90,
            recall_count: 2, last_recall_ms: 10000, pinned: true, alive: true,
        },
        ConsolidatableRecord{
            id: 3, content_hash: 0xBB02, topic_hash: 20,
            tier: MemConsolidationTier.ShortTerm, strength: 0.20,
            recall_count: 1, last_recall_ms: 1000, pinned: false, alive: true,
        },
    ];

    const policy = ConsolidationPolicy{
        prune_strength_threshold: 0.15,
        promote_min_recalls: 3,
        promote_min_strength: 0.60,
        half_life_ms: 3000,
        max_age_archive_ms: 20000,
    };

    if (report := consolidateMemoriesInPlace(pool[0..], 10000, policy)) {
        // id=1 与 id=2 合并（recall=4, strength=0.90, pinned=true）并晋升至 Episodic；
        // id=3 衰减后低于阈值 0.15 被剪枝。
        * = report;
    }
}
```

### 7. 三因子记忆检索、SIMD 近重复归并与 Prompt 打包 (`retrieval.cp`)

融合 Stanford Generative Agents 经典 **Recency（指数时间衰减）+ Importance（重要度）+ SIMD `@dot` Relevance（语义余弦相关性）** 三因子打分、近重复情景记忆归并与 Token 预算上下文打包（100% 纯 CP、零堆分配）：

- **`MemoryRetrievalConfig`**：`{ w_recency, w_importance, w_relevance, decay_rate_per_hour, min_score, required_tag }`，提供 `init(w_recency, w_importance, w_relevance, decay_rate_per_hour): MemoryRetrievalConfig?`
- **`MemoryCandidate`** / **`MemoryScoredHit`** / **`scoreAndRankMemoriesF32(query_emb, candidates, embeddings_flat, dim, now_ms, cfg, top_k, out_hits): usize?`**：按 `exp(-decay_rate_per_hour * age_hours)`、归一化重要度与 8-lane SIMD `@dot` 余弦相似度加权融合，支持按 `required_tag` 与 `min_score` 过滤并按总分降序返回 Top-K。
- **`MemoryMergeStats`** / **`mergeRedundantMemoriesF32(candidates, embeddings_flat, dim, similarity_threshold): MemoryMergeStats?`**：基于 `@dot` 余弦相似度阈值原位合并高重复情景记忆，保留更高重要度与摘要、最新访问时间戳并饱和累加 `access_count`。
- **`MemoryPackResult`** / **`packMemoriesForPrompt(ranked_hits, candidates, max_tokens, out_buf): MemoryPackResult?`**：在 `max_tokens` 预算内贪心打包高分记忆并格式化为简洁的 `[mem#<id>] <summary>\n` LLM 提示块。

```cp
import {
    MemoryRetrievalConfig,
    MemoryCandidate,
    MemoryScoredHit,
    scoreAndRankMemoriesF32,
    mergeRedundantMemoriesF32,
    packMemoriesForPrompt,
} from "cap:agentmem";

pub fn demoThreeFactorMemoryRetrieval() {
    let mems: [3]MemoryCandidate = [
        MemoryCandidate{ mem_id: 101, importance: 0.9, last_access_ms: 7200000, access_count: 2, token_count: 40, tag: "pref", summary: "User prefers concise Rust/CP code" },
        MemoryCandidate{ mem_id: 102, importance: 0.7, last_access_ms: 7200000, access_count: 1, token_count: 35, tag: "pref", summary: "User likes concise code" },
        MemoryCandidate{ mem_id: 103, importance: 0.8, last_access_ms: 0,       access_count: 1, token_count: 50, tag: "fact", summary: "Workspace root is /data/dev/cp" },
    ];
    const embs: [12]f32 = [
        1.0,  0.0,  0.0, 0.0,
        0.99, 0.01, 0.0, 0.0, // 近重复记忆
        0.0,  1.0,  0.0, 0.0,
    ];
    let mems_s: []MemoryCandidate = mems[0..];
    * = mergeRedundantMemoriesF32(mems_s, embs[0..], 4, 0.95); // 合并 mem#102 到 mem#101

    const query: [4]f32 = [0.95, 0.1, 0.0, 0.0];
    if (cfg := MemoryRetrievalConfig.init(1.0, 1.0, 2.0, 0.1)) {
        let hits: [3]MemoryScoredHit = [MemoryScoredHit{}; 3];
        let hits_s: []MemoryScoredHit = hits[0..];
        const n = scoreAndRankMemoriesF32(query[0..], mems[0..], embs[0..], 4, 7200000, cfg, 3, hits_s) ?? 0;

        let prompt_buf: [256]u8 = [0; 256];
        let prompt_s: []u8 = prompt_buf[0..];
        if (pack_res := packMemoriesForPrompt(hits[0..n], mems[0..], 100, prompt_s)) {
            * = prompt_buf[0..pack_res.bytes_written];
        }
    }
}
```

### 8. 零堆分配时序实体关系图谱与 1~2 跳多跳召回器 (`entityGraph.cp`)

面向 GraphRAG 与长程事实记忆演化的 100% 纯 CP、零堆分配时序实体关系图谱引擎，通过半开时间区间 `[valid_from_ms, valid_to_ms)`（`valid_to_ms == 0` 表示至今有效）彻底解决 Agent 记忆“新旧事实冲突”痛点并保留完整可回溯历史：

- **`TemporalFactEdge`**：`{ edge_id, src_entity_id, rel_type_id, dst_entity_id, confidence: f32, valid_from_ms, valid_to_ms, active }`
- **`GraphHopHit`**：`{ entity_id, hop_depth, path_confidence: f32, via_edge_id }`
- **`EntityGraphStats`**：`{ capacity, total_edges, current_valid_edges, superseded_edges, total_assertions, total_supersedes }`
- **`TemporalEntityGraph`**：
  - `init(edges_buf: *[]TemporalFactEdge): TemporalEntityGraph?`（复用调用方栈/静态切片，零堆分配）
  - `assertFact(*_, src_id, rel_id, dst_id, confidence, valid_from_ms): u32?`（校验非零 ID、`confidence ∈ (0.0, 1.0]` 与 `valid_from_ms >= 0`）
  - `supersedeFact(*_, src_id, rel_id, new_dst_id, confidence, now_ms): u32?`（原子将同 `(src, rel)` 的旧有效事实设置 `valid_to_ms = now_ms` 并写入新事实；满槽或参数非法时保证不修改旧事实）
  - `invalidateEdge(*_, edge_id, valid_to_ms): bool`
  - `queryFactsAt(src_id, rel_id_opt: u32?, at_ms, out_edges): usize`（按时间点 `at_ms` 精确回溯历史快照，按 `confidence` 降序输出）
  - `traverseTwoHop(start_entity_id, at_ms, min_confidence, out_hits): usize`（零堆 1 跳 + 2 跳扩展，2 跳置信度为 `e1.confidence * e2.confidence`，自动规避环路、按实体去重保留最高置信度路径并按 `(path_confidence 降序, entity_id 升序)` 输出）
  - `stats(): EntityGraphStats` / `reset(*_)`

```cp
import { TemporalFactEdge, GraphHopHit, TemporalEntityGraph } from "cap:agentmem";

pub fn demoTemporalEntityGraph() {
    let edges: [16]TemporalFactEdge = [TemporalFactEdge{}; 16];
    let edges_s: []TemporalFactEdge = edges[0..];
    let g = TemporalEntityGraph.init(edges_s) ?? return;

    // t=1000: Alice(1) --lives_in(10)--> Tokyo(100), Tokyo(100) --in_country(11)--> Japan(200)
    * = g.assertFact(1, 10, 100, 0.90, 1000);
    * = g.assertFact(100, 11, 200, 0.95, 1000);
    // t=2000: Alice 搬到 Sydney(101)，自动使 Tokyo 事实失效
    * = g.supersedeFact(1, 10, 101, 0.98, 2000);

    let hits: [8]GraphHopHit = [GraphHopHit{}; 8];
    let hits_s: []GraphHopHit = hits[0..];
    const n_past = g.traverseTwoHop(1, 1500, 0.5, hits_s); // 命中 Tokyo(0.90) + Japan(0.855)
    const n_now  = g.traverseTwoHop(1, 2500, 0.5, hits_s); // 命中 Sydney(0.98)
    * = n_past;
    * = n_now;
}
```

### 9. 零堆分配情景记忆巩固、遗忘衰减与 `@dot` SIMD 去重压缩规划器 (`consolidation.cp` Part 2)

在原有 `consolidateMemoriesInPlace` 基础上，新增面向嵌入向量的零堆分配情景记忆巩固规划器：

- **导出符号**：`EpisodeMemoryTier`（`working | episodic | semantic_long_term | evicted`）、`ConsolidationConfig`（`decay_half_life_ms`, `merge_cosine_threshold`, `promote_access_count`, `promote_importance_min`, `evict_retention_threshold`, `max_retained_entries`，提供 `isValid()`）、`MemoryEpisodeMeta`、`ConsolidationDecisionKind`（`keep | promote_semantic | merged_into | evict_decay | evict_budget`）、`ConsolidationAction`、`ConsolidationSummary`。
- **核心纯函数与引擎**：
  - `computeRetentionScore(meta, now_ms, half_life_ms): f32`：融合半衰期指数衰减、对数访问频次增益与重要度计算 `[0.0, 1.0]` 确定性留存分。
  - `cosineSimEpisodeF32(a, b): f32`：直接调用 CP 原生 8-lane SIMD `@dot` 计算嵌入余弦相似度（防零向量/NaN/维度不匹配）。
  - `planMemoryConsolidation(episodes, embeddings, dim, now_ms, cfg, out_actions): ConsolidationSummary`：100% 零堆分配五阶段规划（`@dot` 近重复合并至最高留存分或 `pinned` 代表 -> 低留存分衰减驱逐 -> 高频高重要度 `episodic` 晋升 `semantic_long_term` -> 预算超限按留存分从低到高裁剪，严格保护 `pinned` 永不驱逐）。

```cp
import {
    EpisodeMemoryTier,
    ConsolidationConfig,
    MemoryEpisodeMeta,
    ConsolidationAction,
    planMemoryConsolidation,
} from "cap:agentmem";

pub fn demoSimdMemoryConsolidation() {
    const eps: [3]MemoryEpisodeMeta = [
        MemoryEpisodeMeta{ episode_id: 1, last_access_ms: 9000, access_count: 5, importance_f32: 0.9, tier: EpisodeMemoryTier.episodic },
        MemoryEpisodeMeta{ episode_id: 2, last_access_ms: 8000, access_count: 1, importance_f32: 0.5, tier: EpisodeMemoryTier.episodic },
        MemoryEpisodeMeta{ episode_id: 3, last_access_ms: 0,    access_count: 1, importance_f32: 0.2, tier: EpisodeMemoryTier.working },
    ];
    const embs: [12]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.99, 0.05, 0.0, 0.0, // 近重复 -> merged_into episode 0
        0.0, 1.0, 0.0, 0.0,   // 陈旧低分 -> evict_decay
    ];
    let actions: [3]ConsolidationAction = [ConsolidationAction{}; 3];
    let actions_s: []ConsolidationAction = actions[0..];
    const cfg = ConsolidationConfig{ decay_half_life_ms: 1000, merge_cosine_threshold: 0.95, max_retained_entries: 4 };
    const summary = planMemoryConsolidation(eps[0..], embs[0..], 4, 10000, cfg, actions_s);
    * = summary; // episode 0 promoted, episode 1 merged_into 0, episode 2 evicted_decay
}
```

### 10. 零分配分层对话摘要树与渐进式上下文压缩引擎 (`summaryTree.cp`)

面向 MemGPT / Letta 风格长周期对话 AI Agent 的 100% 纯 CP 零动态堆分配分层摘要树（Hierarchical Summary Tree）与 Token 预算前沿规划引擎：

- **常量与数据模型**：
  - `SUMMARY_TREE_MAX_NODES = 128`、`SUMMARY_NO_NODE = 0xFFFF`
  - `SummaryNodeKind`（`raw_turn | compacted_summary`）
  - `SummaryTreeNode`（`node_id`, `kind`, `level` [0 为原始叶子，1+ 为摘要层], `start_turn`, `end_turn`, `token_count`, `importance_bps` [0..=10000], `parent_idx`, `covered_by_summary`, `pinned`, `active`）
  - `ContextFrontierPlan`（`selected_node_indices`, `selected_count`, `total_tokens`, `raw_turns_kept`, `summaries_used`, `dropped_nodes`, `covered_turns`）
  - `SummaryTreeStats`（`total_nodes`, `raw_leaf_count`, `summary_node_count`, `max_level`, `raw_tokens_total`, `compacted_tokens_total`）
- **核心引擎（`HierarchicalSummaryTree`）**：
  - `init(): HierarchicalSummaryTree`
  - `append_raw_turn(*_, turn_seq, token_count, importance_bps, pinned): u16?`：严格校验单调递增 `turn_seq`、`importance_bps <= 10000` 与 128 节点容量。
  - `compact_range(*_, start_turn, end_turn, summary_tokens, importance_bps): u16?`：将覆盖 `[start_turn, end_turn]` 的现有顶层节点递归折叠为高层 `compacted_summary` 父节点，建立 `parent_idx` 关联并标记 `covered_by_summary = true`；拒绝跨界部分重叠。
  - `select_context_frontier(token_budget, min_recent_raw_turns): ContextFrontierPlan`：按「`pinned` 原始轮次 -> 最近 `min_recent_raw_turns` 原始轮次 -> 最高层不重叠摘要（自动下钻）-> 剩余高重要度原始轮次」四级策略在 `token_budget` 内规划零重复覆盖的上下文前沿，并按 `start_turn` 升序输出。
  - `stats(): SummaryTreeStats`

```cp
import { HierarchicalSummaryTree } from "cap:agentmem";

pub fn demoHierarchicalSummaryTree() {
    let tree = HierarchicalSummaryTree.init();
    // 追加 6 轮原始对话（每轮 100 tokens），并将前 4 轮压缩为 50 tokens 摘要
    let t: u32 = 1;
    while (t <= 6) : (t += 1) {
        * = tree.append_raw_turn(t, 100, 5000, t == 1);
    }
    * = tree.compact_range(2, 4, 50, 8000);

    // 预算 360 tokens、至少保留最近 2 轮原文：无缝拼装 [pinned turn 1, summary(2..4), turn 5, turn 6]
    const plan = tree.select_context_frontier(360, 2);
    * = plan.total_tokens;  // 350
    * = plan.covered_turns; // 6
}
```

### 11. 零分配多层级 AI Agent 记忆融合路由器 (`memRouter.cp`)

面向多层 AI Agent 记忆系统（Working Memory / Episodic Memory / Semantic Long-Term / Entity Graph）的 100% 纯 CP 零动态堆分配跨层加权 RRF 融合、跨层共识激励与 Token 预算打包内核：

- **常量与枚举**：
  - `MEM_ROUTER_MAX_CANDIDATES` (`64`)
  - `MemorySourceStore`（`working | episodic | semantic | entity_graph`）
  - `MemoryQueryIntent`（`recent_context | factual_lookup | deep_synthesis | custom_weights`）
- **配置与数据模型**：
  - `MemoryTierWeights`（`working_w`, `episodic_w`, `semantic_w`, `graph_w`, `rrf_k` [默认 `60.0`]；提供 `forIntent(intent)` 预设与 `isValid()`）
  - `MemoryRecallCandidate`（`memory_id`, `source`, `rank_in_source` [0-based], `token_cost`, `pinned`）
  - `FusedMemoryPick`（`memory_id`, `fused_score_f32`, `source_mask` [bit0..bit3 对应四层存储], `token_cost`, `pinned`）
  - `MemoryRouteSummary`（`input_candidates`, `unique_memories`, `selected_count`, `total_tokens`, `multi_store_consensus_count`）
- **核心纯函数**：
  - `fuseAndPackMemoryRecalls(candidates, weights, token_budget, out_picks): MemoryRouteSummary`：按 `memory_id` 聚合去重（同层重复候选防重复计分、仅保留最优 rank；跨层共识叠加各层加权 RRF 分数 `w_s / (rrf_k + rank + 1)` 并置位 `source_mask`），优先锁定 `pinned` 记忆，其余按 `(fused_score_f32 降序, memory_id 升序)` 在 `token_budget` 内贪心装入。

```cp
import {
    MemorySourceStore,
    MemoryQueryIntent,
    MemoryTierWeights,
    MemoryRecallCandidate,
    FusedMemoryPick,
    fuseAndPackMemoryRecalls,
} from "cap:agentmem";

pub fn demoMemoryFusionRouter() {
    const cands: [3]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 10, source: MemorySourceStore.working,      rank_in_source: 0, token_cost: 80, pinned: false },
        MemoryRecallCandidate{ memory_id: 20, source: MemorySourceStore.semantic,     rank_in_source: 1, token_cost: 90, pinned: false },
        MemoryRecallCandidate{ memory_id: 20, source: MemorySourceStore.entity_graph, rank_in_source: 0, token_cost: 90, pinned: false },
    ];
    let picks: [3]FusedMemoryPick = [FusedMemoryPick{}; 3];
    let picks_s: []FusedMemoryPick = picks[0..];

    const summary = fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.factual_lookup),
        200,
        picks_s,
    );
    // memory_id=20 被 semantic + entity_graph 双层共识命中，稳居榜首 (source_mask == 0b1100)
    * = summary;
}
```

### 12. 零分配长程 AI Agent 情景-语义记忆归并、冲突替代与衰减强化引擎 (`consolidator.cp`)

面向长程自主 AI Agent 记忆睡眠归并周期的 100% 纯 CP 零动态堆分配情景-语义记忆融合与衰减强化引擎：

- **常量与枚举**：
  - `MEM_CONSOL_MAX_EPISODIC = 64`、`MEM_CONSOL_MAX_SEMANTIC = 64`、`MEM_CONSOL_MAX_DIM = 32`、`MEM_CONSOL_NO_ID = 0`
  - `MemConsolStatus`（`active | merged | archived | evicted | superseded`）
- **数据模型与衰减评分**：
  - `MemConsolEntry`（含低维 L2 归一化向量 `vec`、`entity_key_hash`、`source_tag`、`importance`、`access_count`、`created_ms`/`last_access_ms`、`pinned`、`superseded_by_id`、`merged_into_semantic_id`、`cluster_members`）
  - `MemConsolPassConfig`（`defaults()` / `isValid()`）、`MemConsolPassReport`、`MemConsolEngineStats`
  - `computeEntryRetentionScore(entry, now_ms, half_life_ms): f32`：结合 Ebbinghaus 指数衰减（访问频次提升记忆稳定性）与重要度计算 `[0.0, 1.0]` 留存分，`pinned` 恒为 `1.0`。
- **核心引擎 `AgentMemoryConsolidator`**：
  - `init(dim): AgentMemoryConsolidator?` / `addEpisodic` / `addSemantic` / `touchEpisodic` / `touchSemantic` / `getEpisodic` / `getSemantic` / `stats()`
  - `runConsolidationPass(*_, now_ms, cfg): MemConsolPassReport`：三阶段一键归并：
    1. **实体冲突覆写检测**：同 `entity_key_hash` 且新条目与旧条目 `@dot` 余弦低于 `conflict_threshold` 时，将非锁定旧条目标记为 `superseded`。
    2. **近重复聚类与 `@fma` 质心融合晋升**：识别 `@dot >= merge_threshold` 的情景记忆簇，经 `@fma` 重要度加权融合质心并强化重要度，晋升或融入长期语义池，原情景成员标为 `merged`。
    3. **低分剪枝归档与硬 TTL 淘汰**：对超 TTL 或留存分 `< prune_threshold` 的非锁定条目执行 `evicted` / `archived`，`pinned` 条目永不淘汰。

```cp
import {
    MemConsolPassConfig,
    AgentMemoryConsolidator,
} from "cap:agentmem";

pub fn demoAgentMemoryConsolidator() {
    let eng = AgentMemoryConsolidator.init(4) ?? return;
    const e1: [4]f32 = [1.0, 0.05, 0.0, 0.0];
    const e2: [4]f32 = [0.99, 0.10, 0.0, 0.0];
    * = eng.addEpisodic(1, 0, 10, 0.70, 1000, false, e1[0..]);
    * = eng.addEpisodic(2, 0, 10, 0.75, 2000, false, e2[0..]);

    const rep = eng.runConsolidationPass(3000, MemConsolPassConfig.defaults());
    * = rep.clusters_merged;         // 1
    * = rep.semantic_promoted_count; // 1 (自动晋升为长期语义记忆)
}
```


### 13. 零分配时间窗口 + 实体位图 + `@dot` 向量三模联合情景记忆索引 (`episodes.cp`)

面向长程自主 AI Agent 情景记忆多维检索的 100% 纯 CP 零动态堆分配三模联合索引引擎，深度融合原生 `@dot`、`@popCount` 与 `@fma` 硬件原语：

- **常量与数据模型**：
  - `EP_INDEX_MAX_SLOTS = 128`、`EP_INDEX_MAX_DIM = 32`
  - `EpisodeRecord`（`episode_id`, `timestamp_ms`, `session_id`, `entity_mask: u64`, `importance_f32`, `pinned`, L2 归一化 `vec`）
  - `EpisodeQueryFilter`（`session_id`, `min_ts_ms..max_ts_ms`, `required_entities`, `any_entities`, `excluded_entities`, `target_entities`；提供 `all()` 与 `isValid()`）
  - `EpisodeHybridWeights`（`vector_weight`, `recency_weight`, `entity_weight`, `importance_weight`, `recency_half_life_ms`；提供 `defaults()` 与 `isValid()`）
  - `EpisodeHybridHit`（含 `total_score` 及分项拆解 `vector_score`, `recency_score`, `entity_score`, `importance_score`）、`EpisodeIndexStats`
- **核心引擎 `HybridEpisodeIndex`**：
  - `init(dim): HybridEpisodeIndex?`（校验 `1 <= dim <= 32`）
  - `insertEpisode(*_, episode_id, timestamp_ms, session_id, entity_mask, importance_f32, pinned, raw_vec): bool`（自动 L2 归一化，拦截零/NaN 向量、重复 ID 与满池）
  - `queryHybrid(*_, query_vec, filter, weights, now_ms, out_hits: *[]EpisodeHybridHit): usize`：联合时间窗口、会话与实体位图过滤，结合 `@dot` 余弦相似度、`@popCount` 实体重合度、指数时间近因衰减与 `@fma` 加权打分，输出确定性严格降序 Top-K。
  - `scanSessionTimeline(session_id, min_ts_ms, max_ts_ms, out_ids: *[]u64): usize`：按 `(timestamp_ms asc, episode_id asc)` 回放指定会话时间线切片。
  - `pruneBeforeTimestamp(*_, cutoff_ms): usize` / `setPinned(*_, episode_id, pinned)` / `getEpisode(episode_id)` / `stats()`：按时间水位线归档裁剪旧情景，严格保护 `pinned == true` 情景。

```cp
import {
    EpisodeQueryFilter,
    EpisodeHybridWeights,
    EpisodeHybridHit,
    HybridEpisodeIndex,
} from "cap:agentmem";

pub fn demoHybridEpisodeIndex() {
    let idx = HybridEpisodeIndex.init(4) ?? return;
    const v1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    * = idx.insertEpisode(1, 10_000, 100, 0b0111, 0.9, true, v1[0..]);

    let hits: [4]EpisodeHybridHit = [EpisodeHybridHit{}; 4];
    let hits_s: []EpisodeHybridHit = hits[0..];
    const n = idx.queryHybrid(v1[0..], EpisodeQueryFilter.all(), EpisodeHybridWeights.defaults(), 10_000, *hits_s);
    * = n; // 1
}
```


### 14. 零分配双时态事实账本 (`factLedger.cp`)

面向长程 AI Agent 的 100% 纯 CP 零动态堆分配 **Bi-Temporal Agent Fact Ledger**（Zep / Graphiti 双时态知识记忆架构），沿业务生效时间（Valid-Time `[valid_from_ms, valid_to_ms)`）与系统记账时间（Transaction-Time `[tx_from_ms, tx_to_ms)`）双正交时间轴追踪事实演化：

- **常量与模型**：
  - `FACT_LEDGER_MAX_RECORDS = 256`、`FACT_TIME_INF = i64::MAX`、`FACT_NO_ID = 0`、`FACT_MAX_CONFIDENCE_BPS = 10000`。
  - `FactStatus`（`active | superseded | retracted`）、`FactLedgerOpStatus`（`ok | invalid_time_range | invalid_confidence | capacity_full | fact_not_found | already_closed | invalid_tx_time`）。
  - `BiTemporalFact`（`fact_id`, `subject_id`, `predicate_id`, `object_hash`, `confidence_bps`, `valid_from_ms`, `valid_to_ms`, `tx_from_ms`, `tx_to_ms`, `source_turn_id`, `superseded_by`, `status`；提供 `isValidAt`, `isRecordedAt`, `isActiveAt`）、`FactAssertResult`、`FactLedgerStats`。
- **核心引擎 `BiTemporalFactLedger`**：
  - `assertFact(*_, subject, predicate, obj_hash, valid_from, tx_now, confidence_bps)` / `assertFactWindow(...)`：自动闭环冲突旧事实并记录 `superseded_by` 链，相同有效事实重叠写入自动强化置信度不浪费槽位。
  - `queryCurrentFacts(subject_id, now_ms, out_facts)`：查询当前系统认知在 `now_ms` 生效的真相事实。
  - `queryAsOf(subject_id, valid_at_ms, tx_at_ms, out_facts)`：任意历史双时态回溯查询（还原“在过去 `tx_at_ms` 时刻，Agent 认为 `valid_at_ms` 时刻的事实是什么”）。
  - `retractFact(*_, fact_id, tx_now_ms)`：显式撤回活跃事实并闭环记账区间。
  - `tracePredicateHistory(subject_id, predicate_id, out_facts)`：完整演化历史审计链。

```cp
import {
    BiTemporalFact,
    BiTemporalFactLedger,
} from "cap:agentmem";

pub fn demoBiTemporalFactLedger() {
    let ledger = BiTemporalFactLedger.init();
    // t=100: Agent 记录用户居住城市 = 北京 (hash=0x11)
    * = ledger.assertFact(1, 10, 0x11, 100, 100, 9000);
    // t=200: 用户搬家至上海 (hash=0x22) -> 自动失效旧事实并链接 superseded_by
    * = ledger.assertFact(1, 10, 0x22, 200, 200, 9500);

    let out: [4]BiTemporalFact = [BiTemporalFact{}; 4];
    let out_s: []BiTemporalFact = out[0..];
    * = ledger.queryCurrentFacts(1, 250, *out_s); // 命中 0x22
    * = ledger.queryAsOf(1, 150, 150, *out_s);    // 回溯命中 0x11
}
```


### 15. 零分配程序性技能记忆库 (`skillBank.cp`)

面向 Voyager / Reflexion 自主 AI Agent 架构的 100% 纯 CP 零动态堆分配 **Procedural Skill Memory Bank**，深度融合原生 `@dot`、`@fma` 与 `@popCount` 硬件原语：

- **常量与数据模型**：
  - `SKILL_BANK_MAX_SLOTS = 64`、`SKILL_BANK_MAX_DIM = 32`、`SKILL_BANK_MAX_BPS = 10000`
  - `SkillRecord`（`skill_id`、L2 归一化触发意图向量 `vec`、工具能力掩码 `tool_mask: u64`、`success_count`、`failure_count`、`avg_latency_ms`、`last_used_ms`、`pinned`；提供 `totalTrials()`, `reliability()`, `reliabilityBps()`, `isExecutableWith(mask)`）
  - `SkillRetrievalConfig`（`reliability_weight`, `tool_overlap_weight`, `min_cosine`；提供 `defaults()`, `isValid()`）
  - `SkillRecallHit`（含 `total_score` 及 `cosine_sim`, `reliability`, `tool_overlap`, `tool_mask`, `avg_latency_ms`）、`SkillBankStats`
- **核心引擎 `SkillBank`**：
  - `init(dim): SkillBank?`（校验 `1 <= dim <= 32`）
  - `registerSkill(*_, skill_id, intent_vec, tool_mask, pinned, now_ms): bool`：自动 L2 归一化触发向量，拦截 `skill_id == 0`、零/NaN 向量、重复 ID 与满池。
  - `retrieveSkills(query_vec, available_tools_mask, cfg, *out_hits): usize`：硬过滤不可执行技能 `(skill.tool_mask & ~available_tools_mask) == 0`，结合 `@dot` 余弦、拉普拉斯贝叶斯可靠性 `(success + 1) / (success + failure + 2)`、`@popCount` 工具重合度与 `@fma` 综合打分输出确定性 Top-K。
  - `recordExecution(*_, skill_id, succeeded, latency_ms, now_ms): bool`：记录执行成败、通过 `@fma` 更新增量平均延迟并维护单调 `last_used_ms`。
  - `pruneUnreliable(*_, min_trials, min_reliability_bps): usize`：自动淘汰达到最小试验次数且可靠性低于阈值的非 `pinned` 失效技能，严格保护 `pinned == true` 核心技能。
  - `setPinned` / `removeSkill` / `getSkill` / `len` / `clear` / `stats`。

```cp
import {
    SkillRetrievalConfig,
    SkillRecallHit,
    SkillBank,
} from "cap:agentmem";

pub fn demoSkillBank() {
    let bank = SkillBank.init(4) ?? return;
    const v_code: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    * = bank.registerSkill(101, v_code[0..], 0b0011, false, 1000);
    * = bank.recordExecution(101, true, 42.0, 1100);

    let hits: [4]SkillRecallHit = [SkillRecallHit{}; 4];
    let hits_s: []SkillRecallHit = hits[0..];
    const n = bank.retrieveSkills(v_code[0..], 0b0111, SkillRetrievalConfig.defaults(), *hits_s);
    * = n; // 1 (命中 skill 101)
}
```
