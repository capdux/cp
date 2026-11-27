[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lunr

纯 CP 实现的零分配内存倒排索引与 BM25 全文检索引擎，支持前缀自动补全检索。

- **源包对齐**：对标 `npm` `lunr`（MIT，olivernn/lunr.js）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，采用栈上固定容量 `MiniSearch` 索引（最多 `64` 篇文档、`256` 个唯一词项、`512` 条倒排记录、单词最大 `24` 字节）。

## 安装与导入

```bash
cap add lunr
```

```cp
import {
    SearchHit,
    DocMeta,
    TermEntry,
    Posting,
    MiniSearch,
    fnv1aLower,
    lnApprox,
    bm25Idf,
    bm25TermScore,
    sortHitsDesc,
} from "lunr";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `MiniSearch` | `class MiniSearch { docs: [64]DocMeta, terms: [256]TermEntry, postings: [512]Posting, ... }` | 固定容量栈上倒排索引，内置 BM25 相关性评分（`k1 = 1.2`，`b = 0.75`）。 |
| `MiniSearch.init` | `static fn init(): MiniSearch` | 初始化空倒排索引。 |
| `MiniSearch.addDocument` | `fn addDocument(*self, doc_id: u32, text: &[]u8): bool` | 按 ASCII 字母数字分词转小写并建索引；文档数达到上限 `64` 时返回 `false`。 |
| `MiniSearch.search` | `fn search(query: &[]u8, out: []SearchHit): usize` | 执行全文 BM25 检索，按得分降序（同分按 `doc_id` 升序）写入 `out`，返回命中数。 |
| `MiniSearch.searchPrefix` | `fn searchPrefix(prefix: &[]u8, out: []SearchHit): usize` | 执行前缀匹配检索（自动补全）并按 BM25 排序写入 `out`，返回命中数。 |
| `MiniSearch.docCount` / `termCount` / `clear` | `fn docCount(): usize` / `fn termCount(): usize` / `fn clear(*self)` | 查询文档数、词项数或原地清空索引。 |
| `SearchHit` | `class SearchHit { doc_id: u32, score: f64 }` | 单条排序检索结果。 |
| `DocMeta` / `TermEntry` / `Posting` | 索引数据结构 | `MiniSearch` 内部公开的文档、词项与倒排链表节点类型。 |
| `fnv1aLower` / `lnApprox` / `bm25Idf` / `bm25TermScore` / `sortHitsDesc` | 评分与工具函数 | 忽略大小写 FNV-1a 64 位哈希、快速自然对数近似、BM25 IDF/词频打分与降序排序。 |

## 使用示例

### 1. 文档建索引与 BM25 全文检索

```cp
import { MiniSearch, SearchHit } from "lunr";

fn searchDemo(): void {
    let mut idx = MiniSearch.init();
    idx.addDocument(1, "CP language zero allocation compiler");
    idx.addDocument(2, "Fast inverted index search engine");
    idx.addDocument(3, "CP search engine with BM25 ranking");

    let mut hits: [8]SearchHit = [SearchHit { doc_id: 0, score: 0.0 }; 8];
    const n = idx.search("CP search", hits[0..]);
    // hits[0].doc_id == 3（BM25 得分最高）
}
```

### 2. 前缀自动补全检索

```cp
import { MiniSearch, SearchHit } from "lunr";

fn prefixDemo(idx: MiniSearch): void {
    let mut hits: [8]SearchHit = [SearchHit { doc_id: 0, score: 0.0 }; 8];
    const n = idx.searchPrefix("comp", hits[0..]);
    // 命中 "compiler" -> doc_id 1
}
```

## 与源包差异与安全亮点

- **零堆分配**：文档表、词典与倒排链表全部内联在 `MiniSearch` 栈结构中（上限 `64` 文档 / `256` 词项 / `512` 倒排），彻底免除堆对象开销与 GC 停顿。
- **确定性排序**：`sortHitsDesc` 按 BM25 `score` 降序排列，得分相同时按 `doc_id` 升序稳定决胜。
- **无外部数学依赖**：内置 `lnApprox`、`bm25Idf`、`bm25TermScore`，纯 CP 独立编译。
