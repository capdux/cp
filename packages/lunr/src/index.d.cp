// Type definitions for CP

import { isAlnum, toLowerByte } from "cap:ascii";
import { bytesEq } from "cap:bin";
export class SearchHit {
    doc_id: u32,
    score: f64,
}

export class DocMeta {
    id: u32,
    length: u32,
}

export class TermEntry {
    hash: u64,
    text: [24]u8,
    text_len: u8,
    df: u16,
    head_posting: i32,
}

export class Posting {
    doc_slot: u16,
    tf: u16,
    next: i32,
}

export fn fnv1aLower(data: &[]u8, start: usize, end: usize): u64;
export fn lnApprox(x_in: f64): f64;
export fn bm25Idf(n_docs: usize, df: u16): f64;
export fn bm25TermScore(idf: f64, tf: u16, dl: u32, avgdl: f64): f64;
export fn sortHitsDesc(hits: []SearchHit, count: usize);
export class MiniSearch {
    docs: [64]DocMeta,
    doc_count: usize,
    total_doc_len: u64,
    terms: [256]TermEntry,
    term_count: usize,
    postings: [512]Posting,
    posting_count: usize,
    fn init(): MiniSearch;
    fn docCount(): usize;
    fn termCount(): usize;
    fn clear(_: auto);
    fn addDocument(_: auto, doc_id: u32, text: &[]u8): bool;
    fn search(query: &[]u8, out: []SearchHit): usize;
    fn searchPrefix(prefix: &[]u8, out: []SearchHit): usize;
}

