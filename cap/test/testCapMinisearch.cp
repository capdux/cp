import { test, expect_eq } from "cap:test";
import { SearchHit, MiniSearch } from "../../packages/@cap/minisearch/src/index.cp";

fn main() {
    test("1. @cap/minisearch — Empty index queries return 0", () => {
        const idx = MiniSearch.init();
        expect_eq(idx.docCount(), 0 as usize);
        expect_eq(idx.termCount(), 0 as usize);

        let out: [8]SearchHit = undefined;
        expect_eq(idx.search("rust", out[0..8]), 0 as usize);
        expect_eq(idx.searchPrefix("ru", out[0..8]), 0 as usize);
    });

    test("2. @cap/minisearch — Single document exact search & case-insensitivity", () => {
        let idx = MiniSearch.init();
        expect_eq(idx.addDocument(101, "CP Language: Fast, Zero-Alloc Systems Compiler!"), true);
        expect_eq(idx.docCount(), 1 as usize);
        expect_eq(idx.termCount() >= 6, true);

        let out: [4]SearchHit = undefined;
        const n1 = idx.search("COMPILER", out[0..4]);
        expect_eq(n1, 1 as usize);
        expect_eq(out[0].doc_id, 101 as u32);
        expect_eq(out[0].score > 0.0, true);

        const n2 = idx.search("zero alloc", out[0..4]);
        expect_eq(n2, 1 as usize);
        expect_eq(out[0].doc_id, 101 as u32);
    });

    test("3. @cap/minisearch — BM25 TF/IDF ranking prioritizes high-TF & rare-term documents", () => {
        let idx = MiniSearch.init();
        * = idx.addDocument(1, "beam scheduler actor runtime concurrency");
        * = idx.addDocument(2, "zig native zig compiler zig codegen");
        * = idx.addDocument(3, "zig overview with many other words about networking and storage systems");
        * = idx.addDocument(4, "actor message mailbox beam runtime");

        let out: [4]SearchHit = undefined;
        // Doc 2 repeats "zig" 3 times in a short doc -> must outrank Doc 3
        const hits = idx.search("zig", out[0..4]);
        expect_eq(hits, 2 as usize);
        expect_eq(out[0].doc_id, 2 as u32);
        expect_eq(out[1].doc_id, 3 as u32);
        expect_eq(out[0].score > out[1].score, true);
    });

    test("4. @cap/minisearch — Multi-term joint query accumulates BM25 score", () => {
        let idx = MiniSearch.init();
        * = idx.addDocument(10, "fast search engine");
        * = idx.addDocument(20, "fast search engine with bm25 ranking");
        * = idx.addDocument(30, "bm25 ranking algorithm");

        let out: [4]SearchHit = undefined;
        // Query matches all 3 docs, but doc 20 matches BOTH "search" and "bm25" -> ranks #1
        const hits = idx.search("search bm25", out[0..4]);
        expect_eq(hits, 3 as usize);
        expect_eq(out[0].doc_id, 20 as u32);
        expect_eq(out[0].score > out[1].score, true);
    });

    test("5. @cap/minisearch — searchPrefix autocomplete recall", () => {
        let idx = MiniSearch.init();
        * = idx.addDocument(1, "compiler compilation compile");
        * = idx.addDocument(2, "compression algorithm");
        * = idx.addDocument(3, "network socket server");

        let out: [4]SearchHit = undefined;
        // "comp" matches both doc 1 (compiler/compilation/compile) and doc 2 (compression)
        const hits_comp = idx.searchPrefix("COMP", out[0..4]);
        expect_eq(hits_comp, 2 as usize);
        expect_eq(out[0].doc_id, 1 as u32); // 3 matching terms -> higher score

        // Narrower prefix "compil" matches only doc 1
        const hits_compil = idx.searchPrefix("compil", out[0..4]);
        expect_eq(hits_compil, 1 as usize);
        expect_eq(out[0].doc_id, 1 as u32);
    });

    test("6. @cap/minisearch — clear() reset & non-matching queries return 0", () => {
        let idx = MiniSearch.init();
        * = idx.addDocument(7, "hello world from cap");

        let out: [4]SearchHit = undefined;
        expect_eq(idx.search("nonexistent", out[0..4]), 0 as usize);
        expect_eq(idx.searchPrefix("xyz", out[0..4]), 0 as usize);
        expect_eq(idx.search("   !!!  ", out[0..4]), 0 as usize);

        idx.clear();
        expect_eq(idx.docCount(), 0 as usize);
        expect_eq(idx.termCount(), 0 as usize);
        expect_eq(idx.search("hello", out[0..4]), 0 as usize);
    });
}
