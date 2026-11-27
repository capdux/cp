// @cap/minisearch — Zero-allocation In-Memory Inverted Index & BM25 Search Engine
// Clean-room MIT implementation in pure CP.
// Supports ASCII tokenization, lowercase normalization, incremental indexing,
// multi-term BM25 (k1=1.2, b=0.75) ranking, and prefix autocomplete queries.

import { isAlnum, toLowerByte } from "cap:ascii";
import { bytesEq } from "cap:bin";

pub const MAX_DOCS: usize = 64;
pub const MAX_TERMS: usize = 256;
pub const MAX_POSTINGS: usize = 512;
const MAX_TERM_LEN: usize = 24;
const NONE_IDX: i32 = -1;

pub struct SearchHit {
    pub doc_id: u32,
    pub score: f64,
}

struct DocMeta {
    id: u32,
    length: u32,
}

struct TermEntry {
    hash: u64,
    text: [24]u8,
    text_len: u8,
    df: u16,
    head_posting: i32,
}

struct Posting {
    doc_slot: u16,
    tf: u16,
    next: i32,
}

fn fnv1aLower(data: []const u8, start: usize, end: usize) -> u64 {
    const offset: u64 = ((0xCBF29CE4 as u64) << 32) | (0x84222325 as u64);
    const prime: u64 = ((0x00000100 as u64) << 32) | (0x000001B3 as u64);
    let h: u64 = offset;
    let i = start;
    while (i < end) {
        h ^= toLowerByte(data[i]) as u64;
        h = h *% prime;
        i += 1;
    }
    return h;
}

fn lnApprox(x_in: f64) -> f64 {
    if (x_in <= 0.0) {
        return 0.0;
    }
    const ln2: f64 = 0.6931471805599453;
    let m = x_in;
    let k: i32 = 0;
    while (m > 1.5) {
        m *= 0.5;
        k += 1;
    }
    while (m < 0.75) {
        m *= 2.0;
        k -= 1;
    }
    const t = (m - 1.0) / (m + 1.0);
    const t2 = t * t;
    let term = t;
    let sum = t;
    term *= t2;
    sum += term / 3.0;
    term *= t2;
    sum += term / 5.0;
    term *= t2;
    sum += term / 7.0;
    term *= t2;
    sum += term / 9.0;
    term *= t2;
    sum += term / 11.0;
    return (k as f64) * ln2 + 2.0 * sum;
}

fn bm25Idf(n_docs: usize, df: u16) -> f64 {
    const n_f = n_docs as f64;
    const df_f = df as f64;
    return lnApprox(1.0 + (n_f - df_f + 0.5) / (df_f + 0.5));
}

fn bm25TermScore(idf: f64, tf: u16, dl: u32, avgdl: f64) -> f64 {
    const k1: f64 = 1.2;
    const b: f64 = 0.75;
    const tf_f = tf as f64;
    const dl_f = dl as f64;
    const denom = tf_f + k1 * (1.0 - b + b * (dl_f / avgdl));
    if (denom <= 0.0) {
        return 0.0;
    }
    return idf * (tf_f * (k1 + 1.0)) / denom;
}

fn sortHitsDesc(hits: []SearchHit, count: usize) {
    let i: usize = 1;
    while (i < count) {
        const key = hits[i];
        let j = i;
        while (j > 0) {
            const prev = hits[j - 1];
            const swap = (prev.score < key.score) || (prev.score == key.score && prev.doc_id > key.doc_id);
            if (!swap) {
                break;
            }
            hits[j] = prev;
            j -= 1;
        }
        hits[j] = key;
        i += 1;
    }
}

pub struct MiniSearch {
    docs: [64]DocMeta,
    doc_count: usize,
    total_doc_len: u64,
    terms: [256]TermEntry,
    term_count: usize,
    postings: [512]Posting,
    posting_count: usize,

    /// Initializes an empty MiniSearch inverted index.
    pub static fn init() -> MiniSearch {
        let docs: [64]DocMeta = undefined;
        let terms: [256]TermEntry = undefined;
        let postings: [512]Posting = undefined;
        return MiniSearch {
            docs: docs,
            doc_count: 0,
            total_doc_len: 0,
            terms: terms,
            term_count: 0,
            postings: postings,
            posting_count: 0,
        };
    }

    /// Returns the number of indexed documents.
    pub fn docCount() -> usize {
        return _.doc_count;
    }

    /// Returns the number of distinct indexed terms.
    pub fn termCount() -> usize {
        return _.term_count;
    }

    /// Clears all documents, terms, and postings.
    pub fn clear(*_) {
        _.doc_count = 0;
        _.total_doc_len = 0;
        _.term_count = 0;
        _.posting_count = 0;
    }

    /// Indexes a document with the given `doc_id` and ASCII text.
    /// Returns false if `MAX_DOCS` capacity is already full.
    pub fn addDocument(*_, doc_id: u32, text: []const u8) -> bool {
        if (_.doc_count >= MAX_DOCS) {
            return false;
        }
        const doc_slot = _.doc_count;
        _.docs[doc_slot] = DocMeta { id: doc_id, length: 0 };
        _.doc_count += 1;

        let doc_len: u32 = 0;
        let i: usize = 0;
        const n = text.len;

        while (i < n) {
            while (i < n && !isAlnum(text[i])) {
                i += 1;
            }
            if (i >= n) {
                break;
            }
            const start = i;
            while (i < n && isAlnum(text[i])) {
                i += 1;
            }
            const end = i;
            doc_len += 1;
            _.total_doc_len += 1;

            const h = fnv1aLower(text, start, end);

            // Locate existing term
            let term_idx: i32 = NONE_IDX;
            let ti: usize = 0;
            while (ti < _.term_count) {
                if (_.terms[ti].hash == h) {
                    term_idx = ti as i32;
                    break;
                }
                ti += 1;
            }

            if (term_idx == NONE_IDX) {
                if (_.term_count >= MAX_TERMS || _.posting_count >= MAX_POSTINGS) {
                    continue;
                }
                let tbuf: [24]u8 = undefined;
                let tlen = end - start;
                if (tlen > MAX_TERM_LEN) {
                    tlen = MAX_TERM_LEN;
                }
                let c: usize = 0;
                while (c < tlen) {
                    tbuf[c] = toLowerByte(text[start + c]);
                    c += 1;
                }
                const p_idx = _.posting_count;
                _.posting_count += 1;
                _.postings[p_idx] = Posting {
                    doc_slot: doc_slot as u16,
                    tf: 1,
                    next: NONE_IDX,
                };
                const new_ti = _.term_count;
                _.term_count += 1;
                _.terms[new_ti] = TermEntry {
                    hash: h,
                    text: tbuf,
                    text_len: tlen as u8,
                    df: 1,
                    head_posting: p_idx as i32,
                };
            } else {
                const uti = term_idx as usize;
                const head = _.terms[uti].head_posting;
                if (head != NONE_IDX && _.postings[head as usize].doc_slot == (doc_slot as u16)) {
                    _.postings[head as usize].tf += 1;
                } else if (_.posting_count < MAX_POSTINGS) {
                    const p_idx = _.posting_count;
                    _.posting_count += 1;
                    _.postings[p_idx] = Posting {
                        doc_slot: doc_slot as u16,
                        tf: 1,
                        next: head,
                    };
                    _.terms[uti].head_posting = p_idx as i32;
                    _.terms[uti].df += 1;
                }
            }
        }

        _.docs[doc_slot].length = doc_len;
        return true;
    }

    /// Executes a multi-term BM25 search (`k1 = 1.2, b = 0.75`).
    /// Writes up to `out.len` highest-scoring hits sorted descending by score,
    /// and returns the total number of matching documents.
    pub fn search(query: []const u8, out: []SearchHit) -> usize {
        if (_.doc_count == 0 || query.len == 0) {
            return 0;
        }

        let scores: [64]f64 = undefined;
        let matched: [64]bool = undefined;
        let d: usize = 0;
        while (d < _.doc_count) {
            scores[d] = 0.0;
            matched[d] = false;
            d += 1;
        }

        let avgdl = (_.total_doc_len as f64) / (_.doc_count as f64);
        if (avgdl <= 0.0) {
            avgdl = 1.0;
        }

        let i: usize = 0;
        const n = query.len;
        while (i < n) {
            while (i < n && !isAlnum(query[i])) {
                i += 1;
            }
            if (i >= n) {
                break;
            }
            const start = i;
            while (i < n && isAlnum(query[i])) {
                i += 1;
            }
            const h = fnv1aLower(query, start, i);

            let ti: usize = 0;
            while (ti < _.term_count) {
                if (_.terms[ti].hash == h) {
                    const idf = bm25Idf(_.doc_count, _.terms[ti].df);
                    let p_idx = _.terms[ti].head_posting;
                    while (p_idx != NONE_IDX) {
                        const post = _.postings[p_idx as usize];
                        const slot = post.doc_slot as usize;
                        scores[slot] += bm25TermScore(idf, post.tf, _.docs[slot].length, avgdl);
                        matched[slot] = true;
                        p_idx = post.next;
                    }
                    break;
                }
                ti += 1;
            }
        }

        let hits: [64]SearchHit = undefined;
        let hit_count: usize = 0;
        d = 0;
        while (d < _.doc_count) {
            if (matched[d]) {
                hits[hit_count] = SearchHit {
                    doc_id: _.docs[d].id,
                    score: scores[d],
                };
                hit_count += 1;
            }
            d += 1;
        }

        sortHitsDesc(hits[0..hit_count], hit_count);

        let w: usize = 0;
        while (w < hit_count && w < out.len) {
            out[w] = hits[w];
            w += 1;
        }
        return hit_count;
    }

    /// Executes a prefix autocomplete search matching all indexed terms starting with `prefix`.
    /// Returns the total number of matching documents and writes top hits into `out`.
    pub fn searchPrefix(prefix: []const u8, out: []SearchHit) -> usize {
        if (_.doc_count == 0 || prefix.len == 0) {
            return 0;
        }

        // Extract normalized lowercase alphanumeric prefix
        let pbuf: [24]u8 = undefined;
        let plen: usize = 0;
        let pi: usize = 0;
        while (pi < prefix.len && !isAlnum(prefix[pi])) {
            pi += 1;
        }
        while (pi < prefix.len && isAlnum(prefix[pi]) && plen < MAX_TERM_LEN) {
            pbuf[plen] = toLowerByte(prefix[pi]);
            plen += 1;
            pi += 1;
        }
        if (plen == 0) {
            return 0;
        }

        let scores: [64]f64 = undefined;
        let matched: [64]bool = undefined;
        let d: usize = 0;
        while (d < _.doc_count) {
            scores[d] = 0.0;
            matched[d] = false;
            d += 1;
        }

        let avgdl = (_.total_doc_len as f64) / (_.doc_count as f64);
        if (avgdl <= 0.0) {
            avgdl = 1.0;
        }

        let ti: usize = 0;
        while (ti < _.term_count) {
            const tlen = _.terms[ti].text_len as usize;
            if (tlen >= plen && bytesEq(_.terms[ti].text[0..plen], pbuf[0..plen])) {
                const idf = bm25Idf(_.doc_count, _.terms[ti].df);
                let p_idx = _.terms[ti].head_posting;
                while (p_idx != NONE_IDX) {
                    const post = _.postings[p_idx as usize];
                    const slot = post.doc_slot as usize;
                    scores[slot] += bm25TermScore(idf, post.tf, _.docs[slot].length, avgdl);
                    matched[slot] = true;
                    p_idx = post.next;
                }
            }
            ti += 1;
        }

        let hits: [64]SearchHit = undefined;
        let hit_count: usize = 0;
        d = 0;
        while (d < _.doc_count) {
            if (matched[d]) {
                hits[hit_count] = SearchHit {
                    doc_id: _.docs[d].id,
                    score: scores[d],
                };
                hit_count += 1;
            }
            d += 1;
        }

        sortHitsDesc(hits[0..hit_count], hit_count);

        let w: usize = 0;
        while (w < hit_count && w < out.len) {
            out[w] = hits[w];
            w += 1;
        }
        return hit_count;
    }
}
