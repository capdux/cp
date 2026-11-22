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
