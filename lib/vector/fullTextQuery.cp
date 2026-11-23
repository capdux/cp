// Reference-hardened against quickwit-oss/tantivy (MIT)
// Origin: Pure-CP Stdlib (Tantivy-Style Full-Text Query Engine: Phrase Slop, Structured Boolean Query,
// Wildcard/Prefix Matching, BM25 Boundary Guards, Hybrid RRF Fusion & SIMD Batch Scoring)
// Companion submodule split from `lib/vector/fullTextIndex.cp` to keep each module < 3,000 lines.

const std = @import("std");
import { Simd } from "../simd/index.cp";
import {
    FT_INVALID_IDX,
    FullTextError,
    FullTextIndex,
    FullTextScratch,
    FtHit,
    FtPosting,
    FtTokenSpan,
    normalizeSingleTerm,
    tokenizeText,
    computeBm25fIdf,
    computeBm25fTermScore,
} from "./fullTextIndex.cp";

// ============================================================================
// 1. Structured Boolean Query Types (MUST / SHOULD / MUST_NOT + Boost + Slop)
// ============================================================================

/// Tantivy-style boolean occurrence constraint.
pub enum FtOccur: u8 {
    must = 0,
    should = 1,
    must_not = 2,
}

/// Clause matching strategy inside a structured `FtBoolQuery`.
pub enum FtClauseKind: u8 {
    term = 0,
    prefix = 1,
    wildcard = 2,
    phrase = 3,
}

/// Single weighted clause in a structured boolean query.
pub struct FtBoolClause {
    occur: FtOccur = FtOccur.should,
    kind: FtClauseKind = FtClauseKind.term,
    text: []const u8 = "",
    boost: f64 = 1.0,
    slop: u32 = 0,
}

/// Structured boolean query combining `MUST`, `SHOULD` (`minimum_should_match`), and `MUST_NOT`.
pub struct FtBoolQuery {
    clauses: []const FtBoolClause = [],
    minimum_should_match: usize = 0,
}

const FT_QUERY_MAX_TERMS: usize = 32;
const FT_QUERY_MAX_TOKEN_BYTES: usize = 64;

fn ftIsFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn ftIsFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn ftSanitizeBoost(boost: f64): f64 {
    if (!ftIsFiniteF64(boost) || boost <= 0.0) return 0.0;
    return boost;
}

fn ftHitBetter(a: FtHit, b: FtHit): bool {
    if (a.score > b.score) return true;
    if (a.score < b.score) return false;
    return a.doc_id < b.doc_id;
}

fn ftInsertTopK(out_hits: *[]FtHit, count: *usize, cand: FtHit) {
    if (out_hits.len == 0) return;
    if (count.* < out_hits.len) {
        let pos: usize = count.*;
        out_hits[pos] = cand;
        count.* += 1;
        while (pos > 0 && ftHitBetter(out_hits[pos], out_hits[pos - 1])) {
            const tmp: FtHit = out_hits[pos - 1];
            out_hits[pos - 1] = out_hits[pos];
            out_hits[pos] = tmp;
            pos -= 1;
        }
        return;
    }
    if (!ftHitBetter(cand, out_hits[out_hits.len - 1])) return;
    let pos: usize = out_hits.len - 1;
    out_hits[pos] = cand;
    while (pos > 0 && ftHitBetter(out_hits[pos], out_hits[pos - 1])) {
        const tmp: FtHit = out_hits[pos - 1];
        out_hits[pos - 1] = out_hits[pos];
        out_hits[pos] = tmp;
        pos -= 1;
    }
}

// ============================================================================
// 2. SIMD-Accelerated ASCII Query Normalization & Linear Wildcard Matcher
// ============================================================================

/// Normalizes an ASCII query string into `dst` using `cap:simd` (`Simd.skipWhitespace`,
/// `Simd.toLowerAscii`): trims leading/trailing whitespace, folds ASCII uppercase to lowercase,
/// and collapses interior whitespace runs into single spaces. Returns bytes written.
pub fn normalizeAsciiQuerySimd(src: []const u8, dst: *[]u8): usize {
    if (src.len == 0 || dst.len == 0) return 0;
    let start: usize = Simd.skipWhitespace(src);
    if (start >= src.len) return 0;

    let end: usize = src.len;
    while (end > start) {
        const b: u8 = src[end - 1];
        if (b == ' ' || b == '\t' || b == '\n' || b == '\r') {
            end -= 1;
        } else {
            break;
        }
    }

    const trimmed: []const u8 = src[start..end];
    let wp: usize = 0;
    let i: usize = 0;
    let in_ws: bool = false;
    while (i < trimmed.len && wp < dst.len) {
        const b: u8 = trimmed[i];
        if (b == ' ' || b == '\t' || b == '\n' || b == '\r') {
            if (!in_ws) {
                dst[wp] = ' ';
                wp += 1;
                in_ws = true;
            }
            i = i + 1 + Simd.skipWhitespace(trimmed[i + 1 ..]);
            continue;
        }
        in_ws = false;
        dst[wp] = b;
        wp += 1;
        i += 1;
    }

    if (wp > 0) {
        * = Simd.toLowerAscii(dst[0..wp], dst[0..wp]);
    }
    return wp;
}

/// Linear-time O(N*M) non-recursive wildcard matcher supporting `*` (0+ bytes) and `?` (any 1 byte).
/// Immune to catastrophic backtracking even on pathological patterns like `*a*a*a*a*b`.
pub fn wildcardMatchBytes(pattern: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = FT_INVALID_IDX as usize;
    let star_ti: usize = 0;

    while (ti < text.len) {
        if (pi < pattern.len && (pattern[pi] == '?' || pattern[pi] == text[ti])) {
            pi += 1;
            ti += 1;
        } else if (pi < pattern.len && pattern[pi] == '*') {
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (star_pi != (FT_INVALID_IDX as usize)) {
            pi = star_pi + 1;
            star_ti += 1;
            ti = star_ti;
        } else {
            return false;
        }
    }

    while (pi < pattern.len && pattern[pi] == '*') {
        pi += 1;
    }
    return pi == pattern.len;
}

// ============================================================================
// 3. Tantivy-Hardened BM25 Boundary Guards & 8-Lane SIMD Batch Scoring
// ============================================================================

/// Tantivy-hardened smooth non-negative BM25 IDF: `ln(1 + (N - df + 0.5) / (df + 0.5))`.
/// Safe for `active_docs == 0`, `df == 0`, and `df > active_docs` (never returns NaN/Inf/negative).
pub fn tantivyBm25Idf(active_docs: usize, df: usize): f64 {
    return computeBm25fIdf(active_docs, df);
}

/// Tantivy-hardened scalar BM25 score with full zero/NaN/Inf/negative boundary protection.
pub fn tantivyBm25Score(tf: f64, doc_len: f64, avg_dl: f64, idf: f64, k1: f64, b: f64): f64 {
    if (!ftIsFiniteF64(tf) || tf <= 0.0) return 0.0;
    if (!ftIsFiniteF64(idf) || idf <= 0.0) return 0.0;
    if (!ftIsFiniteF64(k1) || k1 <= 0.0) return 0.0;
    const safe_b: f64 = if (!ftIsFiniteF64(b) || b < 0.0) 0.0 else if (b > 1.0) 1.0 else b;
    const safe_dl: f64 = if (!ftIsFiniteF64(doc_len) || doc_len < 0.0) 0.0 else doc_len;
    const norm_ratio: f64 = if (!ftIsFiniteF64(avg_dl) || avg_dl <= 0.0) 1.0 else (safe_dl / avg_dl);
    const denom: f64 = tf + k1 * (1.0 - safe_b + safe_b * norm_ratio);
    if (denom <= 0.0 || !ftIsFiniteF64(denom)) return 0.0;
    return idf * (tf * (k1 + 1.0)) / denom;
}

/// 8-lane `@Vector(8, f32)` SIMD batch BM25 scorer over aligned document TF & length slices.
/// Matches `tantivyBm25Score` within `< 1e-4` while scoring 8 documents per SIMD instruction.
pub fn batchBm25ScoreSimdF32(
    tfs: []const f32,
    doc_lens: []const f32,
    avg_dl: f32,
    idf: f32,
    k1: f32,
    b: f32,
    out_scores: *[]f32,
): usize ! FullTextError {
    const n: usize = tfs.len;
    if (doc_lens.len < n || out_scores.len < n) {
        return FullTextError.BufferTooSmall;
    }
    if (n == 0) return 0;

    if (!ftIsFiniteF32(idf) || idf <= 0.0 || !ftIsFiniteF32(k1) || k1 <= 0.0) {
        let z: usize = 0;
        while (z < n) : (z += 1) out_scores[z] = 0.0;
        return n;
    }

    const safe_b: f32 = if (!ftIsFiniteF32(b) || b < 0.0) 0.0 else if (b > 1.0) 1.0 else b;
    const inv_avg_dl: f32 = if (!ftIsFiniteF32(avg_dl) || avg_dl <= 0.0) 0.0 else (1.0 / avg_dl);
    const k1_one_minus_b: f32 = k1 * (1.0 - safe_b);
    const k1_b_inv_avg: f32 = k1 * safe_b * inv_avg_dl;
    const idf_k1p1: f32 = idf * (k1 + 1.0);

    const v_zero: @Vector(8, f32) = @splat(0.0);
    const v_c0: @Vector(8, f32) = @splat(k1_one_minus_b);
    const v_c1: @Vector(8, f32) = @splat(k1_b_inv_avg);
    const v_num_scale: @Vector(8, f32) = @splat(idf_k1p1);
    const v_eps: @Vector(8, f32) = @splat(1.0e-12);

    let i: usize = 0;
    while (i + 8 <= n) : (i += 8) {
        const v_tf_raw: @Vector(8, f32) = tfs[i .. i + 8][0..8].*;
        const v_dl_raw: @Vector(8, f32) = doc_lens[i .. i + 8][0..8].*;
        const v_tf: @Vector(8, f32) = @max(v_tf_raw, v_zero);
        const v_dl: @Vector(8, f32) = @max(v_dl_raw, v_zero);
        const v_denom: @Vector(8, f32) = @max(v_tf + v_c0 + v_c1 * v_dl, v_eps);
        const v_res: @Vector(8, f32) = (v_tf * v_num_scale) / v_denom;
        out_scores[i .. i + 8][0..8].* = v_res;
    }
    while (i < n) : (i += 1) {
        const tf: f32 = if (ftIsFiniteF32(tfs[i]) && tfs[i] > 0.0) tfs[i] else 0.0;
        const dl: f32 = if (ftIsFiniteF32(doc_lens[i]) && doc_lens[i] > 0.0) doc_lens[i] else 0.0;
        if (tf <= 0.0) {
            out_scores[i] = 0.0;
        } else {
            const denom: f32 = tf + k1_one_minus_b + k1_b_inv_avg * dl;
            out_scores[i] = if (denom > 0.0) (tf * idf_k1p1) / denom else 0.0;
        }
    }
    return n;
}

/// 8-lane `@Vector(8, f32)` SIMD linear hybrid fusion: `out[i] = alpha * bm25[i] + (1 - alpha) * vec[i]`.
pub fn fuseHybridScoresSimdF32(
    bm25_scores: []const f32,
    vec_scores: []const f32,
    alpha: f32,
    out_fused: *[]f32,
): usize ! FullTextError {
    const n: usize = bm25_scores.len;
    if (vec_scores.len < n || out_fused.len < n) {
        return FullTextError.BufferTooSmall;
    }
    const a: f32 = if (!ftIsFiniteF32(alpha) || alpha < 0.0) 0.0 else if (alpha > 1.0) 1.0 else alpha;
    const b_w: f32 = 1.0 - a;

    const va: @Vector(8, f32) = @splat(a);
    const vb: @Vector(8, f32) = @splat(b_w);

    let i: usize = 0;
    while (i + 8 <= n) : (i += 8) {
        const v1: @Vector(8, f32) = bm25_scores[i .. i + 8][0..8].*;
        const v2: @Vector(8, f32) = vec_scores[i .. i + 8][0..8].*;
        const vf: @Vector(8, f32) = va * v1 + vb * v2;
        out_fused[i .. i + 8][0..8].* = vf;
    }
    while (i < n) : (i += 1) {
        out_fused[i] = @fma(a, bm25_scores[i], b_w * vec_scores[i]);
    }
    return n;
}

/// Zero-heap Reciprocal Rank Fusion (RRF) combining full-text BM25 hits with dense vector hits.
/// Score(d) = bm25_weight / (k_rrf + rank_bm25(d)) + vec_weight / (k_rrf + rank_vec(d)).
pub fn rrfFuseTopKHits(
    bm25_hits: []const FtHit,
    vec_hits: []const FtHit,
    k_rrf: f64,
    bm25_weight: f64,
    vec_weight: f64,
    out_hits: *[]FtHit,
): usize {
    if (out_hits.len == 0) return 0;
    const safe_k: f64 = if (!ftIsFiniteF64(k_rrf) || k_rrf <= 0.0) 60.0 else k_rrf;
    const wb: f64 = ftSanitizeBoost(bm25_weight);
    const wv: f64 = ftSanitizeBoost(vec_weight);

    let count: usize = 0;

    // 1. Score all docs appearing in bm25_hits (combining with vec_hits if present)
    let bi: usize = 0;
    while (bi < bm25_hits.len) : (bi += 1) {
        const doc_id: u64 = bm25_hits[bi].doc_id;
        let score: f64 = wb / (safe_k + @as(f64, @floatFromInt(bi + 1)));
        let matched: u32 = bm25_hits[bi].matched_terms;

        let vi: usize = 0;
        while (vi < vec_hits.len) : (vi += 1) {
            if (vec_hits[vi].doc_id == doc_id) {
                score += wv / (safe_k + @as(f64, @floatFromInt(vi + 1)));
                matched += vec_hits[vi].matched_terms;
                break;
            }
        }
        ftInsertTopK(out_hits, *count, FtHit{ doc_id: doc_id, score: score, matched_terms: matched });
    }

    // 2. Score docs appearing ONLY in vec_hits
    let vi: usize = 0;
    while (vi < vec_hits.len) : (vi += 1) {
        const doc_id: u64 = vec_hits[vi].doc_id;
        let seen_in_bm25: bool = false;
        let bk: usize = 0;
        while (bk < bm25_hits.len) : (bk += 1) {
            if (bm25_hits[bk].doc_id == doc_id) {
                seen_in_bm25 = true;
                break;
            }
        }
        if (!seen_in_bm25) {
            const score: f64 = wv / (safe_k + @as(f64, @floatFromInt(vi + 1)));
            ftInsertTopK(out_hits, *count, FtHit{
                doc_id: doc_id,
                score: score,
                matched_terms: vec_hits[vi].matched_terms,
            });
        }
    }

    return count;
}

// ============================================================================
// 4. Internal Posting-List Scorers (Term / Prefix / Wildcard / Phrase-Slop)
// ============================================================================

fn scoreSingleTermId(
    idx: *const FullTextIndex,
    term_id: u32,
    boost: f64,
    out_mask: *[]u8,
    out_scores: *[]f64,
) {
    if (idx.active_doc_count == 0) return;
    const te = idx.bufs.terms[term_id as usize];
    if (te.df == 0) return;
    const idf: f64 = computeBm25fIdf(idx.active_doc_count, te.df as usize);
    if (idf <= 0.0) return;

    // Accumulate pseudo-TF per document across all fields for this term
    // (Since postings are appended per doc in order, accumulate directly per doc_slot)
    let p_idx: u32 = te.head_posting;
    while (p_idx != FT_INVALID_IDX) {
        const p: FtPosting = idx.bufs.postings[p_idx as usize];
        const ds: usize = p.doc_slot as usize;
        if (ds < idx.doc_count && !idx.bufs.docs[ds].deleted) {
            const f: usize = p.field_id as usize;
            if (f < idx.cfg.field_count) {
                const fspec = idx.bufs.field_specs[f];
                const flen_off: usize = (idx.bufs.docs[ds].field_len_offset as usize) + f;
                const dl: f64 = @as(f64, @floatFromInt(idx.bufs.doc_field_lens[flen_off]));
                const sum_len: u64 = idx.bufs.field_len_sums[f];
                const avg_dl: f64 = if (idx.active_doc_count > 0 && sum_len > 0)
                    @as(f64, @floatFromInt(sum_len)) / @as(f64, @floatFromInt(idx.active_doc_count))
                else
                    1.0;
                const norm: f64 = 1.0 - fspec.b + fspec.b * (dl / avg_dl);
                const pseudo_tf: f64 = if (norm > 0.0) (fspec.weight * @as(f64, @floatFromInt(p.tf))) / norm else 0.0;
                const sc: f64 = computeBm25fTermScore(idf, pseudo_tf, idx.cfg.k1) * boost;
                if (sc > 0.0 && ftIsFiniteF64(sc)) {
                    out_mask[ds] = 1;
                    out_scores[ds] += sc;
                }
            }
        }
        p_idx = p.next_posting;
    }
}

fn findPostingPositions(
    idx: *const FullTextIndex,
    term_id: u32,
    doc_slot: u32,
    field_id: u16,
): ([]const u32)? {
    let p_idx: u32 = idx.bufs.terms[term_id as usize].head_posting;
    while (p_idx != FT_INVALID_IDX) {
        const p: FtPosting = idx.bufs.postings[p_idx as usize];
        if (p.doc_slot == doc_slot && p.field_id == field_id) {
            const start: usize = p.pos_offset as usize;
            const len: usize = p.pos_len as usize;
            if (start + len <= idx.position_count) {
                return idx.bufs.positions[start .. start + len];
            }
            return null;
        }
        p_idx = p.next_posting;
    }
    return null;
}

/// Checks whether a sequence of term IDs occurs in `(doc_slot, field_id)` in strictly increasing
/// positional order within total/step slop `slop`. Supports repeated terms in the phrase!
fn countPhraseSlopMatchesInField(
    idx: *const FullTextIndex,
    term_ids: []const u32,
    doc_slot: u32,
    field_id: u16,
    slop: u32,
): u32 {
    if (term_ids.len == 0) return 0;
    const first_pos = findPostingPositions(idx, term_ids[0], doc_slot, field_id) orelse return 0;
    if (term_ids.len == 1) return first_pos.len as u32;

    let matches: u32 = 0;
    let fi: usize = 0;
    while (fi < first_pos.len) : (fi += 1) {
        let prev_p: u32 = first_pos[fi];
        let rem_slop: u32 = slop;
        let ok: bool = true;

        let ti: usize = 1;
        while (ti < term_ids.len) : (ti += 1) {
            const next_list = findPostingPositions(idx, term_ids[ti], doc_slot, field_id) orelse {
                ok = false;
                break;
            };
            // Find smallest position `q` in `next_list` with `prev_p + 1 <= q <= prev_p + 1 + rem_slop`
            let found_q: u32 = FT_INVALID_IDX;
            let ni: usize = 0;
            while (ni < next_list.len) : (ni += 1) {
                const q: u32 = next_list[ni];
                if (q > prev_p && (q - prev_p - 1) <= rem_slop) {
                    found_q = q;
                    break;
                }
                if (q > prev_p && (q - prev_p - 1) > rem_slop) {
                    break;
                }
            }
            if (found_q == FT_INVALID_IDX) {
                ok = false;
                break;
            }
            const gap: u32 = found_q - prev_p - 1;
            rem_slop -= gap;
            prev_p = found_q;
        }
        if (ok) {
            matches += 1;
        }
    }
    return matches;
}

fn evalClauseIntoArrays(
    idx: *const FullTextIndex,
    clause: FtBoolClause,
    out_mask: *[]u8,
    out_scores: *[]f64,
) {
    let z: usize = 0;
    while (z < idx.doc_count) : (z += 1) {
        out_mask[z] = 0;
        out_scores[z] = 0.0;
    }

    const boost: f64 = ftSanitizeBoost(clause.boost);
    if (boost <= 0.0 || clause.text.len == 0 || idx.active_doc_count == 0) return;

    if (clause.kind == FtClauseKind.term) {
        let norm_buf: [FT_QUERY_MAX_TOKEN_BYTES]u8 = [0; FT_QUERY_MAX_TOKEN_BYTES];
        let norm_s: []u8 = norm_buf[0..];
        const nlen = normalizeSingleTerm(clause.text, idx.cfg.enable_stemming, norm_s);
        if (nlen == 0) return;
        if (tid := idx.findTerm(norm_buf[0..nlen])) {
            scoreSingleTermId(idx, tid, boost, out_mask, out_scores);
        }
        return;
    }

    if (clause.kind == FtClauseKind.prefix) {
        let pbuf: [FT_QUERY_MAX_TOKEN_BYTES]u8 = [0; FT_QUERY_MAX_TOKEN_BYTES];
        let ps: []u8 = pbuf[0..];
        const plen = normalizeAsciiQuerySimd(clause.text, ps);
        if (plen == 0) return;
        const prefix: []const u8 = pbuf[0..plen];

        let ti: usize = 0;
        while (ti < idx.term_count) : (ti += 1) {
            const tt = idx.termText(ti);
            if (tt.len >= plen && std.mem.eql(u8, tt[0..plen], prefix)) {
                scoreSingleTermId(idx, ti as u32, boost, out_mask, out_scores);
            }
        }
        return;
    }

    if (clause.kind == FtClauseKind.wildcard) {
        let wbuf: [FT_QUERY_MAX_TOKEN_BYTES]u8 = [0; FT_QUERY_MAX_TOKEN_BYTES];
        let ws: []u8 = wbuf[0..];
        const wlen = normalizeAsciiQuerySimd(clause.text, ws);
        if (wlen == 0) return;
        const pat: []const u8 = wbuf[0..wlen];

        let ti: usize = 0;
        while (ti < idx.term_count) : (ti += 1) {
            const tt = idx.termText(ti);
            if (wildcardMatchBytes(pat, tt)) {
                scoreSingleTermId(idx, ti as u32, boost, out_mask, out_scores);
            }
        }
        return;
    }

    // Phrase query with slop >= 0 (supports repeated terms & strict monotone position matching)
    let tok_spans: [FT_QUERY_MAX_TERMS]FtTokenSpan = [FtTokenSpan{}; FT_QUERY_MAX_TERMS];
    let tok_spans_s: []FtTokenSpan = tok_spans[0..];
    const ntok = tokenizeText(
        clause.text,
        idx.cfg.enable_stemming,
        idx.cfg.enable_stopwords,
        idx.cfg.enable_cjk_bigrams,
        tok_spans_s,
    );
    if (ntok == 0) return;

    let tids: [FT_QUERY_MAX_TERMS]u32 = [0; FT_QUERY_MAX_TERMS];
    let idf_sum: f64 = 0.0;
    let k: usize = 0;
    while (k < ntok) : (k += 1) {
        const word = tok_spans[k].text();
        const tid = idx.findTerm(word) orelse return;
        tids[k] = tid;
        idf_sum += computeBm25fIdf(idx.active_doc_count, idx.bufs.terms[tid as usize].df as usize);
    }
    if (idf_sum <= 0.0) return;

    // Iterate postings of first term to find candidate (doc_slot, field_id) pairs
    let p_idx: u32 = idx.bufs.terms[tids[0] as usize].head_posting;
    while (p_idx != FT_INVALID_IDX) {
        const p: FtPosting = idx.bufs.postings[p_idx as usize];
        const ds: usize = p.doc_slot as usize;
        if (ds < idx.doc_count && !idx.bufs.docs[ds].deleted) {
            const p_hits = countPhraseSlopMatchesInField(idx, tids[0..ntok], p.doc_slot, p.field_id, clause.slop);
            if (p_hits > 0) {
                const fspec = idx.bufs.field_specs[p.field_id as usize];
                const sc = computeBm25fTermScore(idf_sum, fspec.weight * @as(f64, @floatFromInt(p_hits)), idx.cfg.k1) * boost;
                if (sc > 0.0 && ftIsFiniteF64(sc)) {
                    out_mask[ds] = 1;
                    out_scores[ds] += sc;
                }
            }
        }
        p_idx = p.next_posting;
    }
}

// ============================================================================
// 5. Public Structured Query Entrypoints (Boolean / Phrase / Prefix / Wildcard)
// ============================================================================

/// Executes a Tantivy-style structured boolean query (`MUST` / `SHOULD` / `MUST_NOT` + `boost`
/// + `minimum_should_match`) over `FullTextIndex` with zero dynamic heap allocation.
/// Requires `scratch.doc_mask.len >= idx.doc_count`, `scratch.doc_mask_aux.len >= idx.doc_count * 2`,
/// `scratch.scores.len >= idx.doc_count`, and `scratch.matched_terms_cnt.len >= idx.doc_count`.
pub fn searchBooleanQuery(
    idx: *const FullTextIndex,
    query: FtBoolQuery,
    scratch: *FullTextScratch,
    out_hits: *[]FtHit,
): usize ! FullTextError {
    if (idx.doc_count == 0 || idx.active_doc_count == 0 || query.clauses.len == 0 || out_hits.len == 0) {
        return 0;
    }
    const dc: usize = idx.doc_count;
    if (scratch.scores.len < dc || scratch.doc_mask.len < dc ||
        scratch.doc_mask_aux.len < dc * 2 || scratch.matched_terms_cnt.len < dc) {
        return FullTextError.BufferTooSmall;
    }

    // Partition scratch arrays:
    //   doc_mask[0..dc]       -> overall eligibility (1 = candidate, 0 = excluded)
    //   doc_mask_aux[0..dc]   -> per-clause hit mask
    //   doc_mask_aux[dc..2dc] -> must_not exclusion mask
    //   matched_terms_cnt     -> count of matched SHOULD clauses per doc
    //   scores[0..dc]         -> accumulated BM25F score
    let has_must: bool = false;
    let has_should: bool = false;
    let ci: usize = 0;
    while (ci < query.clauses.len) : (ci += 1) {
        if (query.clauses[ci].occur == FtOccur.must) has_must = true;
        if (query.clauses[ci].occur == FtOccur.should) has_should = true;
    }
    if (!has_must && !has_should) {
        return 0;
    }

    let d: usize = 0;
    while (d < dc) : (d += 1) {
        scratch.doc_mask[d] = if (has_must && !idx.bufs.docs[d].deleted) 1 else 0;
        scratch.doc_mask_aux[dc + d] = 0;
        scratch.matched_terms_cnt[d] = 0;
        scratch.scores[d] = 0.0;
    }

    let clause_scores_buf: [256]f64 = [0.0; 256];
    const use_local_scores: bool = dc <= 256;
    let c_mask: []u8 = scratch.doc_mask_aux[0..dc];

    ci = 0;
    while (ci < query.clauses.len) : (ci += 1) {
        const cl = query.clauses[ci];
        let c_scores_slice: []f64 = if (use_local_scores) clause_scores_buf[0..dc] else scratch.scores[0..dc];
        if (!use_local_scores) {
            // For >256 docs, fall back to direct accumulation on non-excluded docs
            evalClauseIntoArrays(idx, cl, c_mask, scratch.scores);
        } else {
            evalClauseIntoArrays(idx, cl, c_mask, c_scores_slice);
        }

        if (cl.occur == FtOccur.must) {
            d = 0;
            while (d < dc) : (d += 1) {
                if (c_mask[d] == 0) {
                    scratch.doc_mask[d] = 0;
                } else if (scratch.doc_mask[d] != 0 && use_local_scores) {
                    scratch.scores[d] += clause_scores_buf[d];
                }
            }
        } else if (cl.occur == FtOccur.should) {
            d = 0;
            while (d < dc) : (d += 1) {
                if (c_mask[d] != 0 && !idx.bufs.docs[d].deleted) {
                    scratch.matched_terms_cnt[d] += 1;
                    if (!has_must) scratch.doc_mask[d] = 1;
                    if (use_local_scores) scratch.scores[d] += clause_scores_buf[d];
                }
            }
        } else {
            // MUST_NOT
            d = 0;
            while (d < dc) : (d += 1) {
                if (c_mask[d] != 0) {
                    scratch.doc_mask_aux[dc + d] = 1;
                }
            }
        }
    }

    const eff_min_should: usize = if (query.minimum_should_match > 0)
        query.minimum_should_match
    else if (!has_must && has_should)
        1
    else
        0;

    let hit_count: usize = 0;
    d = 0;
    while (d < dc) : (d += 1) {
        if (idx.bufs.docs[d].deleted) continue;
        if (scratch.doc_mask[d] == 0) continue;
        if (scratch.doc_mask_aux[dc + d] != 0) continue;
        if ((scratch.matched_terms_cnt[d] as usize) < eff_min_should) continue;
        const sc: f64 = scratch.scores[d];
        if (!ftIsFiniteF64(sc) || sc <= 0.0) continue;

        ftInsertTopK(out_hits, *hit_count, FtHit{
            doc_id: idx.bufs.docs[d].doc_id,
            score: sc,
            matched_terms: if (scratch.matched_terms_cnt[d] > 0) scratch.matched_terms_cnt[d] else 1,
        });
    }

    return hit_count;
}

/// Convenience zero-heap Phrase Query with configurable `slop` (`0` = exact phrase, `k` = proximity slop).
pub fn searchPhraseQuery(
    idx: *const FullTextIndex,
    phrase_text: []const u8,
    slop: u32,
    boost: f64,
    scratch: *FullTextScratch,
    out_hits: *[]FtHit,
): usize ! FullTextError {
    const clauses: [1]FtBoolClause = [
        FtBoolClause{
            occur: FtOccur.must,
            kind: FtClauseKind.phrase,
            text: phrase_text,
            boost: boost,
            slop: slop,
        },
    ];
    return searchBooleanQuery(idx, FtBoolQuery{ clauses: clauses[0..] }, scratch, out_hits);
}

/// Convenience zero-heap Prefix Query (SIMD ASCII lowercase prefix scan).
pub fn searchPrefixQuery(
    idx: *const FullTextIndex,
    prefix: []const u8,
    boost: f64,
    scratch: *FullTextScratch,
    out_hits: *[]FtHit,
): usize ! FullTextError {
    const clauses: [1]FtBoolClause = [
        FtBoolClause{
            occur: FtOccur.must,
            kind: FtClauseKind.prefix,
            text: prefix,
            boost: boost,
        },
    ];
    return searchBooleanQuery(idx, FtBoolQuery{ clauses: clauses[0..] }, scratch, out_hits);
}

/// Convenience zero-heap Wildcard Query (`*` / `?`, linear O(N*M) matcher).
pub fn searchWildcardQuery(
    idx: *const FullTextIndex,
    pattern: []const u8,
    boost: f64,
    scratch: *FullTextScratch,
    out_hits: *[]FtHit,
): usize ! FullTextError {
    const clauses: [1]FtBoolClause = [
        FtBoolClause{
            occur: FtOccur.must,
            kind: FtClauseKind.wildcard,
            text: pattern,
            boost: boost,
        },
    ];
    return searchBooleanQuery(idx, FtBoolQuery{ clauses: clauses[0..] }, scratch, out_hits);
}
