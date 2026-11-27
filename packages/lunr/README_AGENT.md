# lunr — AI Agent Quick Ref

Zero-allocation in-memory inverted index and BM25 full-text search engine with prefix matching (npm `lunr` parity).

## Import

```cp
import {
    SearchHit, DocMeta, TermEntry, Posting, MiniSearch,
    fnv1aLower, lnApprox, bm25Idf, bm25TermScore, sortHitsDesc,
} from "lunr";
```

## API Signatures

```cp
export class SearchHit { doc_id: u32, score: f64 }
export class DocMeta { id: u32, length: u32 }
export class TermEntry { hash: u64, text: [24]u8, text_len: u8, df: u16, head_posting: i32 }
export class Posting { doc_slot: u16, tf: u16, next: i32 }

export fn fnv1aLower(data: &[]u8, start: usize, end: usize): u64;
export fn lnApprox(x_in: f64): f64;
export fn bm25Idf(n_docs: usize, df: u16): f64;
export fn bm25TermScore(idf: f64, tf: u16, dl: u32, avgdl: f64): f64;
export fn sortHitsDesc(hits: []SearchHit, count: usize);

export class MiniSearch {
    static fn init(): MiniSearch;
    fn docCount(): usize;
    fn termCount(): usize;
    fn clear(*self);
    fn addDocument(*self, doc_id: u32, text: &[]u8): bool;
    fn search(query: &[]u8, out: []SearchHit): usize;
    fn searchPrefix(prefix: &[]u8, out: []SearchHit): usize;
}
```

## Minimal Example

```cp
import { MiniSearch, SearchHit } from "lunr";

fn demo(): void {
    let mut idx = MiniSearch.init();
    idx.addDocument(1, "hello cp world");
    idx.addDocument(2, "cp full text search");
    let mut hits: [4]SearchHit = [SearchHit { doc_id: 0, score: 0.0 }; 4];
    const n = idx.search("cp", hits[0..]);
    const p = idx.searchPrefix("sea", hits[0..]);
}
```

## Caveats & Borrowing Rules

- **Mutable receiver**: Declare `let mut idx = MiniSearch.init()` for `addDocument` and `clear`; `search` and `searchPrefix` are read-only on `idx` and write into mutable `out: []SearchHit`.
- **Fixed capacities**: Max `64` documents (`addDocument` returns `false` when full), `256` unique terms, `512` postings; tokens longer than `24` bytes are truncated to 24 bytes for prefix storage.
- **ASCII tokenizer**: Splits on non-alphanumeric ASCII bytes and folds ASCII letters to lowercase; pass `&[]u8` (or string literal/slice).
- **Output buffer**: `search`/`searchPrefix` write at most `out.len` top-ranked hits sorted by descending `score` (ties broken by ascending `doc_id`).
