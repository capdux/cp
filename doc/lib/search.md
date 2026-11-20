# CP 标准库：全文检索、BM25F、布尔/短语查询与自动补全引擎 (`cap:search` / `cap.search`)

## 一、 架构总览与设计准则

`cap:search` 是 CP 语言官方内置的纯原生全文检索与自动补全搜索引擎库，专为本地知识库、CLI 检索与 AI RAG 召回打造：

- **前缀自动补全树 (`PrefixTrie`)**：基于节点池化的 ASCII 大小写归一化前缀树，支持累计词频统计与按频次降序、字典序升序的 Top-K 补全建议。
- **多字段倒排索引 (`SearchIndex`)**：支持 `title` / `body` 双字段加权 BM25F 排序（默认 `title = 3.0`, `body = 1.0`）、可选英语停用词过滤、`DuplicateDocId` 检测与 `upsertDocument` / `removeDocument` 实时更新。
- **布尔检索与精确短语匹配**：支持默认 OR/BM25F 排序、显式布尔操作符（`+must` / `-exclude` / `AND` / `OR` / `NOT`）与 `"exact phrase"` 连续词序精确短语匹配。
- **稠密窗口高亮摘要 (`highlightSnippet`)**：自动定位命中关键词最密集的上下文窗口截取摘要，并保留原文大小写包裹自定义高亮标签（如 `<mark>...</mark>`）。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:search"` 与 `import cap from "cap"; cap.search.*` 动态挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/search/trie.cp` | `// Source: cp/lib/search/trie.cp (Prefix Autocomplete Trie & Search Error Types)` | `SearchError`、`PrefixSuggestion`、`PrefixTrie` |
| `lib/search/engine.cp` | `// Source: cp/lib/search/engine.cp (Multi-Field BM25F, Boolean/Phrase Query & Snippet Highlighting Engine)` | `SearchHit`、`SearchIndex`（BM25F、布尔/短语检索、高亮摘要） |
| `lib/search/index.cp` | `// Source: cp/lib/search/index.cp (Unified Exports for cap:search)` | `cap:search` 与 `cap.search` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与前缀自动补全 (`trie.cp`)

- **`SearchError`**：`InvalidQuery` | `DocumentNotFound` | `DuplicateDocId` | `OutOfMemory`
- **`PrefixSuggestion`**：`{ term: string, freq: u32 }`
  - `term` 借用自底层 Trie 内部存储；调用方仅需使用 `alloc.free(suggestions)` 释放返回切片。
- **`PrefixTrie`**：
  - `PrefixTrie.init(alloc): PrefixTrie`
  - `deinit(*_)`
  - `insert(*_, term: string, weight_delta: u32 = 1): !`（大小写归一化并累加词频）
  - `decrement(*_, term: string, weight_delta: u32 = 1)`（扣减词频，归零后自动移出 `termCount`）
  - `suggest(alloc, prefix: string, limit: usize): ([]PrefixSuggestion)!`（按 `freq` 降序、字典序升序返回 Top-K 候选）
  - `contains(term: string): bool`
  - `termCount(): usize`

### 2. 多字段全文检索引擎 (`engine.cp`)

- **`SearchHit`**：`{ doc_id: string, score: f64, snippet: string }`
  - `search` 返回的切片及其 `snippet` 字符串由调用方分配器拥有，需调用 `SearchIndex.freeHits(alloc, hits)` 统一释放。
- **`SearchIndex`**：
  - `SearchIndex.init(alloc): SearchIndex`
  - `deinit(*_)`
  - `freeHits(alloc, hits: []SearchHit)`
  - `setFieldWeights(*_, title_weight: f64, body_weight: f64)`（默认 `title = 3.0`, `body = 1.0`）
  - `setUseStopwords(*_, enabled: bool)`（默认 `false` 以保障精确短语完整性）
  - `docCount(): usize`
  - `addDocument(*_, doc_id: string, title: string, body: string): !`（重复 `doc_id` 返回 `SearchError.DuplicateDocId`）
  - `upsertDocument(*_, doc_id: string, title: string, body: string): !`（存在则覆盖更新，不存在则新增）
  - `removeDocument(*_, doc_id: string): bool`（移除文档、清理倒排链并同步扣减 `PrefixTrie` 词频）
  - `search(alloc, query: string, limit: usize): ([]SearchHit)!`（支持 `+term`、`-term`、`AND`、`OR`、`NOT`、`"exact phrase"`）
  - `highlightSnippet(alloc, text: string, terms: []const string, max_len: usize, open_tag: string, close_tag: string): ([]u8)!`
  - `autocomplete(alloc, prefix: string, limit: usize): ([]PrefixSuggestion)!`

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { SearchIndex } from "cap:search";

pub fn demoSearch()! {
    const alloc = std.heap.page_allocator;

    let idx = SearchIndex.init(alloc);
    defer idx.deinit();

    idx.setFieldWeights(3.0, 1.0);
    idx.addDocument("d1", "CP Language Guide", "Pure native systems programming with BEAM actors.")?;
    idx.addDocument("d2", "Vector Search", "BM25F ranking and exact phrase matching in CP.")?;

    const hits = idx.search(alloc, "\"exact phrase\" AND CP -actors", 5)?;
    defer SearchIndex.freeHits(alloc, hits);

    const sugs = idx.autocomplete(alloc, "sea", 3)?;
    defer alloc.free(sugs);
}
```
