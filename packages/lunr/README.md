[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lunr

Zero-allocation in-memory inverted index and BM25 full-text search engine with prefix autocomplete in pure CP.

- **Upstream Alignment**: `npm` `lunr` (MIT, olivernn/lunr.js)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — fixed-capacity stack `MiniSearch` index (`64` docs, `256` unique terms, `512` postings, max `24`-byte tokens).

## Installation & Import

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

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `MiniSearch` | `class MiniSearch { docs: [64]DocMeta, terms: [256]TermEntry, postings: [512]Posting, ... }` | Fixed-capacity stack inverted index with BM25 scoring (`k1 = 1.2`, `b = 0.75`). |
| `MiniSearch.init` | `static fn init(): MiniSearch` | Creates an empty inverted index. |
| `MiniSearch.addDocument` | `fn addDocument(*self, doc_id: u32, text: &[]u8): bool` | Tokenizes ASCII alphanumeric lowercase terms and indexes `doc_id`; returns `false` if document capacity (`64`) is full. |
| `MiniSearch.search` | `fn search(query: &[]u8, out: []SearchHit): usize` | Runs full-text BM25 query, sorts hits by descending `score` (ties broken by ascending `doc_id`), and writes into `out`; returns hit count. |
| `MiniSearch.searchPrefix` | `fn searchPrefix(prefix: &[]u8, out: []SearchHit): usize` | Matches all indexed terms starting with `prefix` (autocomplete) with BM25 ranking; writes into `out` and returns hit count. |
| `MiniSearch.docCount` / `termCount` / `clear` | `fn docCount(): usize` / `fn termCount(): usize` / `fn clear(*self)` | Inspects document/term counts or resets the index in place. |
| `SearchHit` | `class SearchHit { doc_id: u32, score: f64 }` | Ranked search result entry. |
| `DocMeta` / `TermEntry` / `Posting` | Index storage structs | Public metadata structs stored inside `MiniSearch`. |
| `fnv1aLower` / `lnApprox` / `bm25Idf` / `bm25TermScore` / `sortHitsDesc` | Scoring & utility functions | Lowercase FNV-1a 64-bit hash, fast `ln(x)` approximation, BM25 IDF/term score helpers, and descending hit sorter. |

## Examples

### 1. Index Documents & Full-Text BM25 Search

```cp
import { MiniSearch, SearchHit } from "lunr";

fn searchDemo(): void {
    let mut idx = MiniSearch.init();
    idx.addDocument(1, "CP language zero allocation compiler");
    idx.addDocument(2, "Fast inverted index search engine");
    idx.addDocument(3, "CP search engine with BM25 ranking");

    let mut hits: [8]SearchHit = [SearchHit { doc_id: 0, score: 0.0 }; 8];
    const n = idx.search("CP search", hits[0..]);
    // hits[0].doc_id == 3 (highest BM25 score)
}
```

### 2. Prefix Autocomplete Search

```cp
import { MiniSearch, SearchHit } from "lunr";

fn prefixDemo(idx: MiniSearch): void {
    let mut hits: [8]SearchHit = [SearchHit { doc_id: 0, score: 0.0 }; 8];
    const n = idx.searchPrefix("comp", hits[0..]);
    // Matches "compiler" -> doc_id 1
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: All document tables, inverted term dictionaries, and posting lists live in fixed-size arrays inside `MiniSearch` (up to `64` docs, `256` terms, `512` postings), avoiding JS object-graph memory overhead.
- **Deterministic Tie-Breaking**: `sortHitsDesc` sorts by descending BM25 `score`, breaking equal scores deterministically by ascending `doc_id`.
- **Self-Contained Math**: Includes `lnApprox`, `bm25Idf`, and `bm25TermScore` with zero external math library dependencies.
