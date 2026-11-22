// Origin: Pure-CP Stdlib Test Suite (lib/prompt/citation_grounder.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc RAG Citation Parser & Faithfulness Grounder:
//   1. Inline citation parsing ([1], [2, 3], [1][4]) & ignoring non-numeric brackets ([todo], [note], [], [1a], [1 2])
//   2. Out-of-bounds hallucinated citation detection ([0], [99], huge u32 overflow) & gate rejection
//   3. computeClaimSupportBps case-folding, min_token_len filtering, dedup & citation-marker exclusion
//   4. Multi-source union grounding ([1, 2] & [1][2]) passing gate with high combined overlap
//   5. Misattributed / unrelated source citation triggering weak_grounding_sentences & gate rejection
//   6. Uncited sentence policy toggle (allow_uncited_sentences = false vs true)
//   7. Decimal preservation (3.14) & post-period trailing citations (.[1]) sentence segmentation
//   8. Empty answer, empty sources, invalid CitationGateConfig & small output buffer truncation guards

import { test, expect_eq } from "cap:test";
import {
    CITATION_MAX_REFS,
    CITATION_MAX_SENTENCES,
    CitationRefSpan,
    CitationSentenceAudit,
    CitationGateConfig,
    CitationAuditReport,
    extractInlineCitations,
    computeClaimSupportBps,
    auditRagCitations,
} from "cap:prompt";

// ============================================================================
// Test 1: Inline Citation Parsing & Non-Numeric Bracket Filtering
// ============================================================================
test("citation_grounder: parse [1], [2, 3], [1][4] and ignore non-numeric brackets", () => {
    expect_eq(CITATION_MAX_REFS, 64);
    expect_eq(CITATION_MAX_SENTENCES, 32);

    const text = "Check [todo] and [note] then [1] plus [2, 3] and [1][4] skip [] [1a] [1 2] [1,].";
    let refs: [16]CitationRefSpan = [CitationRefSpan{}; 16];
    let refs_s: []CitationRefSpan = refs[0..];

    const n = extractInlineCitations(text, 4, refs_s);
    expect_eq(n, 5);

    expect_eq(refs[0].source_id_1based, 1);
    expect_eq(refs[0].in_bounds, true);
    expect_eq(refs[0].end_byte > refs[0].start_byte, true);

    expect_eq(refs[1].source_id_1based, 2);
    expect_eq(refs[1].in_bounds, true);
    expect_eq(refs[2].source_id_1based, 3);
    expect_eq(refs[2].in_bounds, true);
    // [2, 3] share the same enclosing bracket span
    expect_eq(refs[1].start_byte, refs[2].start_byte);
    expect_eq(refs[1].end_byte, refs[2].end_byte);

    expect_eq(refs[3].source_id_1based, 1);
    expect_eq(refs[3].in_bounds, true);
    expect_eq(refs[4].source_id_1based, 4);
    expect_eq(refs[4].in_bounds, true);
    // [1][4] are adjacent distinct bracket spans
    expect_eq(refs[3].end_byte, refs[4].start_byte);
});

// ============================================================================
// Test 2: Out-of-Bounds Citations [0], [99] & Overflow Guard
// ============================================================================
test("citation_grounder: detect [0] and [99] out-of-bounds citations and fail gate", () => {
    const text = "Zero [0] and valid [1] and oob [99] and huge [99999999999999].";
    let refs: [8]CitationRefSpan = [CitationRefSpan{}; 8];
    let refs_s: []CitationRefSpan = refs[0..];

    const n = extractInlineCitations(text, 2, refs_s);
    expect_eq(n, 4);
    expect_eq(refs[0].source_id_1based, 0);
    expect_eq(refs[0].in_bounds, false);
    expect_eq(refs[1].source_id_1based, 1);
    expect_eq(refs[1].in_bounds, true);
    expect_eq(refs[2].source_id_1based, 99);
    expect_eq(refs[2].in_bounds, false);
    expect_eq(refs[3].in_bounds, false);

    // Full audit must reject out-of-bounds citations
    const sources: [2][]const u8 = [
        "RadixAttention caches shared prompt prefixes in a radix tree.",
        "PagedAttention manages physical KV blocks without fragmentation.",
    ];
    const answer = "RadixAttention caches shared prompt prefixes [0]. PagedAttention manages physical blocks [99].";
    let audits: [4]CitationSentenceAudit = [CitationSentenceAudit{}; 4];
    let audits_s: []CitationSentenceAudit = audits[0..];

    const rep: CitationAuditReport = auditRagCitations(answer, sources[0..], CitationGateConfig.defaults(), audits_s);
    expect_eq(rep.total_sentences, 2);
    expect_eq(rep.cited_sentences, 2);
    expect_eq(rep.valid_citation_refs, 0);
    expect_eq(rep.out_of_bounds_refs, 2);
    expect_eq(rep.grounded_sentences, 0);
    expect_eq(rep.weak_grounding_sentences, 2);
    expect_eq(rep.passed_gate, false);
    expect_eq(audits[0].invalid_citations, 1);
    expect_eq(audits[1].invalid_citations, 1);
});

// ============================================================================
// Test 3: computeClaimSupportBps Case-Folding, Min Token Len & Dedup
// ============================================================================
test("citation_grounder: computeClaimSupportBps case-folding, filtering & dedup", () => {
    const src = "The BEAM scheduler runs native Zig state machines with zero garbage collection.";

    // Case-insensitive exact match (tokens len >= 3: "BEAM", "scheduler", "runs", "native", "Zig")
    // "in" (len 2) ignored; "[123]" skipped as numeric citation
    const claim_full = "BEAM SCHEDULER runs native zig in [123]!";
    expect_eq(computeClaimSupportBps(claim_full, src, 3), 10000);

    // 2 of 4 unique tokens >= 3 match ("native", "zero" match; "python", "runtime" miss) -> 5000 bps
    // Repeated "python" is deduplicated so denominator stays 4
    const claim_half = "native zero python python runtime";
    expect_eq(computeClaimSupportBps(claim_half, src, 3), 5000);

    // Disjoint tokens -> 0 bps
    expect_eq(computeClaimSupportBps("banana mango orange", src, 3), 0);

    // Empty / invalid min_token_len -> 0 bps
    expect_eq(computeClaimSupportBps("", src, 3), 0);
    expect_eq(computeClaimSupportBps("native", "", 3), 0);
    expect_eq(computeClaimSupportBps("native", src, 0), 0);
});

// ============================================================================
// Test 4: Multi-Source Union Grounding ([1, 2] and [1][2])
// ============================================================================
test("citation_grounder: multi-source union support passes gate", () => {
    const sources: [2][]const u8 = [
        "CP compiles directly into native Zig state machines.",
        "BEAM preemptive scheduler orchestrates concurrent AI agents.",
    ];
    // Sentence 1 synthesizes from [1, 2]; Sentence 2 synthesizes from [1][2]
    const answer = "CP compiles native Zig state machines with BEAM preemptive scheduler [1, 2]. Concurrent agents run inside native state machines [1][2].";

    let audits: [4]CitationSentenceAudit = [CitationSentenceAudit{}; 4];
    let audits_s: []CitationSentenceAudit = audits[0..];
    const rep = auditRagCitations(answer, sources[0..], CitationGateConfig.defaults(), audits_s);

    expect_eq(rep.total_sentences, 2);
    expect_eq(rep.cited_sentences, 2);
    expect_eq(rep.grounded_sentences, 2);
    expect_eq(rep.weak_grounding_sentences, 0);
    expect_eq(rep.valid_citation_refs, 4);
    expect_eq(rep.out_of_bounds_refs, 0);
    expect_eq(rep.avg_support_bps >= 7000, true);
    expect_eq(rep.passed_gate, true);

    expect_eq(audits[0].is_grounded, true);
    expect_eq(audits[0].valid_citations, 2);
    expect_eq(audits[0].support_overlap_bps >= 8000, true);
    expect_eq(audits[1].is_grounded, true);
    expect_eq(audits[1].valid_citations, 2);
});

// ============================================================================
// Test 5: Wrong / Unrelated Source Citation Triggers Weak Grounding Gate Failure
// ============================================================================
test("citation_grounder: citing unrelated source flags weak_grounding_sentences", () => {
    const sources: [2][]const u8 = [
        "FlashAttention computes exact attention using online softmax tiling.",
        "BoringSSL provides AES-GCM authenticated encryption and SHA-256 hashing.",
    ];
    // Sentence 1 cites [1] accurately; Sentence 2 cites [2] for an unrelated claim about Kubernetes!
    const answer = "FlashAttention computes exact attention via online softmax tiling [1]. Kubernetes schedules containerized pods across worker nodes [2].";

    let audits: [4]CitationSentenceAudit = [CitationSentenceAudit{}; 4];
    let audits_s: []CitationSentenceAudit = audits[0..];
    const rep = auditRagCitations(answer, sources[0..], CitationGateConfig.defaults(), audits_s);

    expect_eq(rep.total_sentences, 2);
    expect_eq(rep.cited_sentences, 2);
    expect_eq(rep.grounded_sentences, 1);
    expect_eq(rep.weak_grounding_sentences, 1);
    expect_eq(rep.out_of_bounds_refs, 0);
    expect_eq(rep.passed_gate, false);

    expect_eq(audits[0].is_grounded, true);
    expect_eq(audits[1].is_grounded, false);
    expect_eq(audits[1].support_overlap_bps < 3500, true);
});

// ============================================================================
// Test 6: Uncited Sentences Policy Toggle (false vs true)
// ============================================================================
test("citation_grounder: allow_uncited_sentences false vs true gate behavior", () => {
    const sources: [1][]const u8 = [
        "SIMD instructions accelerate dot product reductions across eight float lanes.",
    ];
    const answer = "Here is the summary. SIMD instructions accelerate dot product reductions [1].";

    let audits: [4]CitationSentenceAudit = [CitationSentenceAudit{}; 4];
    let audits_s: []CitationSentenceAudit = audits[0..];

    // Strict mode: allow_uncited_sentences = false -> MUST fail
    const strict_cfg = CitationGateConfig.defaults();
    expect_eq(strict_cfg.allow_uncited_sentences, false);
    const rep_strict = auditRagCitations(answer, sources[0..], strict_cfg, audits_s);
    expect_eq(rep_strict.total_sentences, 2);
    expect_eq(rep_strict.cited_sentences, 1);
    expect_eq(rep_strict.grounded_sentences, 1);
    expect_eq(rep_strict.weak_grounding_sentences, 0);
    expect_eq(rep_strict.passed_gate, false);
    expect_eq(audits[0].citation_count, 0);
    expect_eq(audits[1].is_grounded, true);

    // Relaxed mode: allow_uncited_sentences = true -> MUST pass
    let relaxed_cfg = CitationGateConfig.defaults();
    relaxed_cfg.allow_uncited_sentences = true;
    const rep_relaxed = auditRagCitations(answer, sources[0..], relaxed_cfg, audits_s);
    expect_eq(rep_relaxed.total_sentences, 2);
    expect_eq(rep_relaxed.cited_sentences, 1);
    expect_eq(rep_relaxed.grounded_sentences, 1);
    expect_eq(rep_relaxed.passed_gate, true);
});

// ============================================================================
// Test 7: Decimal Preservation (3.14) & Trailing Post-Period Citations (.[1])
// ============================================================================
test("citation_grounder: decimal numbers and trailing post-period citations", () => {
    const sources: [2][]const u8 = [
        "Approximation constant equals 3.14159 for circular geometry calculations.",
        "Latency dropped to 1.25 milliseconds after enabling prefix caching.",
    ];
    const answer = "Constant equals 3.14159 for circular geometry.[1] Latency dropped to 1.25 milliseconds with prefix caching.[2]";

    let audits: [4]CitationSentenceAudit = [CitationSentenceAudit{}; 4];
    let audits_s: []CitationSentenceAudit = audits[0..];
    const rep = auditRagCitations(answer, sources[0..], CitationGateConfig.defaults(), audits_s);

    expect_eq(rep.total_sentences, 2);
    expect_eq(rep.cited_sentences, 2);
    expect_eq(rep.grounded_sentences, 2);
    expect_eq(rep.passed_gate, true);
    expect_eq(audits[0].valid_citations, 1);
    expect_eq(audits[1].valid_citations, 1);
});

// ============================================================================
// Test 8: Empty Answer, Empty Sources, Invalid Config & Small Buffer Truncation
// ============================================================================
test("citation_grounder: boundary guards for empty inputs, invalid cfg & buffer cap", () => {
    const sources: [1][]const u8 = ["Zero allocation buffers prevent heap fragmentation."];
    const empty_sources: [0][]const u8 = [];
    let audits: [4]CitationSentenceAudit = [CitationSentenceAudit{}; 4];
    let audits_s: []CitationSentenceAudit = audits[0..];

    // Empty & whitespace-only answers fail gate cleanly
    const r_empty = auditRagCitations("", sources[0..], CitationGateConfig.defaults(), audits_s);
    expect_eq(r_empty.total_sentences, 0);
    expect_eq(r_empty.passed_gate, false);

    const r_ws = auditRagCitations("   \n\t  ", sources[0..], CitationGateConfig.defaults(), audits_s);
    expect_eq(r_ws.total_sentences, 0);
    expect_eq(r_ws.passed_gate, false);

    // Empty sources fail gate even with allow_uncited_sentences = true
    let relaxed = CitationGateConfig.defaults();
    relaxed.allow_uncited_sentences = true;
    const r_nosrc = auditRagCitations("Zero allocation buffers work.", empty_sources[0..], relaxed, audits_s);
    expect_eq(r_nosrc.passed_gate, false);

    // Invalid configs fail isValid() and gate
    let bad_bps = CitationGateConfig.defaults();
    bad_bps.min_support_overlap_bps = 10001;
    expect_eq(bad_bps.isValid(), false);
    expect_eq(auditRagCitations("Zero allocation [1].", sources[0..], bad_bps, audits_s).passed_gate, false);

    let bad_tok = CitationGateConfig.defaults();
    bad_tok.min_token_len = 0;
    expect_eq(bad_tok.isValid(), false);
    expect_eq(auditRagCitations("Zero allocation [1].", sources[0..], bad_tok, audits_s).passed_gate, false);

    // Small output buffer truncation: out_refs len 2 on 4 citations, out_sentences len 1 on 3 sentences
    let tiny_refs: [2]CitationRefSpan = [CitationRefSpan{}; 2];
    let tiny_refs_s: []CitationRefSpan = tiny_refs[0..];
    expect_eq(extractInlineCitations("[1][2][3][4]", 4, tiny_refs_s), 2);
    expect_eq(tiny_refs[0].source_id_1based, 1);
    expect_eq(tiny_refs[1].source_id_1based, 2);

    const three_sents = "Zero allocation buffers [1]. Prevent heap fragmentation [1]. Zero heap buffers [1].";
    let tiny_audits: [1]CitationSentenceAudit = [CitationSentenceAudit{}; 1];
    let tiny_audits_s: []CitationSentenceAudit = tiny_audits[0..];
    const r_trunc = auditRagCitations(three_sents, sources[0..], CitationGateConfig.defaults(), tiny_audits_s);
    expect_eq(r_trunc.total_sentences, 3);
    expect_eq(r_trunc.grounded_sentences, 3);
    expect_eq(r_trunc.passed_gate, true);
    expect_eq(tiny_audits[0].is_grounded, true);
});
