# lunr@1.0.0 (Agent Quickref)

> Zero-allocation in-memory inverted index and BM25 full-text search engine with prefix matching in pure CP | License: MIT
> Import: `import { ... } from "lunr";` | Full types: `src/index.d.cp`

## Functions
- `export fn fnv1aLower(data: &[]u8, start: usize, end: usize): u64;`
- `export fn lnApprox(x_in: f64): f64;`
- `export fn bm25Idf(n_docs: usize, df: u16): f64;`
- `export fn bm25TermScore(idf: f64, tf: u16, dl: u32, avgdl: f64): f64;`
- `export fn sortHitsDesc(hits: []SearchHit, count: usize);`
