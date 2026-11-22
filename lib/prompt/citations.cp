// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc RAG Citation Parser & Grounding Verifier) | Created: 2026-11-22
// Pure CP zero-heap-allocation Perplexity / Enterprise RAG Inline Citation Grounding & Faithfulness Auditor:
//   - Zero-alloc inline citation extraction (`[1]`, `[2, 3]`, `[1][4]`) ignoring non-numeric brackets (`[todo]`, `[note]`)
//   - Strict 1-based bounds verification flagging `[0]` and `> source_count` hallucinated citations
//   - Zero-alloc ASCII case-folding lexical overlap scorer (`computeClaimSupportBps`, 0..=10000 bps)
//   - Sentence-level multi-source union grounding audit (`auditRagCitations`) with configurable faithfulness gate

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum inline citation references buffered per sentence during audit.
pub const CITATION_MAX_REFS: usize = 64;

/// Recommended maximum sentence audit records per RAG answer.
pub const CITATION_MAX_SENTENCES: usize = 32;

/// Single inline numeric citation reference parsed from an LLM answer.
pub struct CitationRefSpan {
    source_id_1based: u32 = 0;
    start_byte: usize = 0;
    end_byte: usize = 0;
    in_bounds: bool = false;
}

/// Per-sentence citation & source-support audit record.
pub struct CitationSentenceAudit {
    start_byte: usize = 0;
    end_byte: usize = 0;
    citation_count: usize = 0;
    valid_citations: usize = 0;
    invalid_citations: usize = 0;
    support_overlap_bps: u16 = 0;
    is_grounded: bool = false;
}

/// Faithfulness & citation gate policy configuration.
pub struct CitationGateConfig {
    min_support_overlap_bps: u16 = 3500;
    min_token_len: usize = 3;
    allow_uncited_sentences: bool = false;
    reject_out_of_bounds: bool = true;

    /// Returns production default configuration (35% overlap, min token len 3, strict citation & bounds).
    static fn defaults(): CitationGateConfig {
        return CitationGateConfig{
            min_support_overlap_bps: 3500,
            min_token_len: 3,
            allow_uncited_sentences: false,
            reject_out_of_bounds: true,
        };
    }

    /// Validates that `min_support_overlap_bps <= 10000` and `min_token_len >= 1`.
    fn isValid(): bool {
        return _.min_support_overlap_bps <= 10000 && _.min_token_len >= 1;
    }
}

/// Global RAG citation faithfulness & grounding audit report.
pub struct CitationAuditReport {
    total_sentences: usize = 0;
    cited_sentences: usize = 0;
    grounded_sentences: usize = 0;
    weak_grounding_sentences: usize = 0;
    valid_citation_refs: usize = 0;
    out_of_bounds_refs: usize = 0;
    avg_support_bps: u16 = 0;
    passed_gate: bool = false;
}

// ============================================================================
// 2. Internal Zero-Alloc ASCII & Lexical Helpers
// ============================================================================

fn isAsciiSpace(b: u8): bool {
    return b == 32 || b == 9 || b == 10 || b == 13;
}

fn isAsciiDigit(b: u8): bool {
    return b >= 48 && b <= 57;
}

fn isTokenChar(b: u8): bool {
    if (b >= 48 && b <= 57) return true;  // '0'..'9'
    if (b >= 65 && b <= 90) return true;  // 'A'..'Z'
    if (b >= 97 && b <= 122) return true; // 'a'..'z'
    if (b >= 128) return true;            // UTF-8 multibyte token bytes
    return false;
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 65 && b <= 90) return b + 32;
    return b;
}

fn tokensEqualIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

/// Validates whether `inner` (contents strictly between `[` and `]`) is a valid comma-separated
/// non-empty list of unsigned decimal integers (e.g. `1`, `2, 3`, `1, 4`).
fn isValidNumericBracketInner(inner: []const u8): bool {
    if (inner.len == 0) return false;
    let pos: usize = 0;
    let num_count: usize = 0;

    while (pos < inner.len) {
        // Skip optional leading spaces/tabs before a number
        while (pos < inner.len && (inner[pos] == 32 || inner[pos] == 9)) : (pos += 1) {}
        if (pos >= inner.len) {
            // Trailing comma without following digit (e.g. "[1, ]") is invalid
            return false;
        }
        if (!isAsciiDigit(inner[pos])) return false;

        // Consume digits of one integer
        while (pos < inner.len && isAsciiDigit(inner[pos])) : (pos += 1) {}
        num_count += 1;

        // Skip optional trailing spaces/tabs after the number
        while (pos < inner.len && (inner[pos] == 32 || inner[pos] == 9)) : (pos += 1) {}
        if (pos >= inner.len) break;

        // Must be a comma separator before next number
        if (inner[pos] != 44) return false; // ','
        pos += 1;

        // Comma at very end (e.g. "[1,]") is invalid
        if (pos >= inner.len) return false;
    }

    return num_count > 0;
}

/// Checks whether a numeric citation bracket starts at `text[start] == '['`,
/// returning the exclusive end byte index (`']' + 1`) if valid, or `0` otherwise.
fn matchNumericBracketEnd(text: []const u8, start: usize): usize {
    if (start >= text.len || text[start] != 91) return 0; // '['
    let j: usize = start + 1;
    while (j < text.len) : (j += 1) {
        const ch: u8 = text[j];
        if (ch == 93) { // ']'
            if (isValidNumericBracketInner(text[(start + 1)..j])) {
                return j + 1;
            }
            return 0;
        }
        if (ch == 91 || ch == 10 || ch == 13) {
            return 0;
        }
    }
    return 0;
}

/// Checks if `needle` (already length >= min_token_len) appears as a whole token
/// (case-insensitive ASCII) anywhere in `haystack`.
fn sourceContainsToken(haystack: []const u8, needle: []const u8): bool {
    let i: usize = 0;
    while (i < haystack.len) {
        while (i < haystack.len && !isTokenChar(haystack[i])) : (i += 1) {}
        const tok_start: usize = i;
        while (i < haystack.len && isTokenChar(haystack[i])) : (i += 1) {}
        const tok_len: usize = i - tok_start;
        if (tok_len == needle.len) {
            if (tokensEqualIgnoreCase(haystack[tok_start..i], needle)) {
                return true;
            }
        }
    }
    return false;
}

/// Checks if `needle` already appeared earlier in `prefix` (skipping numeric citation brackets)
/// to deduplicate repeated claim tokens with zero extra memory.
fn claimPrefixHasToken(prefix: []const u8, needle: []const u8): bool {
    let i: usize = 0;
    while (i < prefix.len) {
        if (prefix[i] == 91) {
            const br_end: usize = matchNumericBracketEnd(prefix, i);
            if (br_end > 0) {
                i = br_end;
                continue;
            }
        }
        if (!isTokenChar(prefix[i])) {
            i += 1;
            continue;
        }
        const tok_start: usize = i;
        while (i < prefix.len && isTokenChar(prefix[i])) : (i += 1) {}
        const tok_len: usize = i - tok_start;
        if (tok_len == needle.len) {
            if (tokensEqualIgnoreCase(prefix[tok_start..i], needle)) {
                return true;
            }
        }
    }
    return false;
}

/// Computes union lexical support coverage in basis points `[0, 10000]` of `claim`
/// across all cited valid 1-based source IDs in `valid_ids`.
fn computeClaimUnionSupportBps(
    claim: []const u8,
    sources: []const []const u8,
    valid_ids: []const u32,
    min_token_len: usize,
): u16 {
    if (claim.len == 0 || valid_ids.len == 0 || min_token_len == 0) return 0;

    let total_tokens: usize = 0;
    let supported_tokens: usize = 0;

    let i: usize = 0;
    while (i < claim.len) {
        // Skip inline numeric citation brackets so citation IDs do not skew claim lexical overlap
        if (claim[i] == 91) {
            const br_end: usize = matchNumericBracketEnd(claim, i);
            if (br_end > 0) {
                i = br_end;
                continue;
            }
        }

        if (!isTokenChar(claim[i])) {
            i += 1;
            continue;
        }

        const tok_start: usize = i;
        while (i < claim.len && isTokenChar(claim[i])) : (i += 1) {}
        const tok_len: usize = i - tok_start;
        if (tok_len < min_token_len) continue;

        const tok: []const u8 = claim[tok_start..i];
        // Deduplicate repeated tokens within the same claim
        if (claimPrefixHasToken(claim[0..tok_start], tok)) {
            continue;
        }

        total_tokens += 1;

        let found: bool = false;
        let s_idx: usize = 0;
        while (s_idx < valid_ids.len) : (s_idx += 1) {
            const sid_1based: u32 = valid_ids[s_idx];
            if (sid_1based >= 1 && (sid_1based as usize) <= sources.len) {
                const src_text: []const u8 = sources[(sid_1based as usize) - 1];
                if (sourceContainsToken(src_text, tok)) {
                    found = true;
                    break;
                }
            }
        }

        if (found) {
            supported_tokens += 1;
        }
    }

    if (total_tokens == 0) return 0;
    const bps_u64: u64 = ((supported_tokens as u64) * 10000) / (total_tokens as u64);
    if (bps_u64 > 10000) return 10000;
    return @intCast(bps_u64);
}

// ============================================================================
// 3. Public Zero-Alloc RAG Citation Grounding APIs
// ============================================================================

/// Extracts inline 1-based numeric citations (`[1]`, `[2, 3]`, `[1][4]`) from `answer`.
/// - Ignores non-numeric brackets such as `[note]`, `[todo]`, `[]`, `[1a]`, or `[1 2]`.
/// - Parses `[0]` and IDs `> source_count` with `in_bounds = false` for hallucination detection.
/// - Writes up to `out_refs.len` spans into `out_refs` and returns the number of spans written.
pub fn extractInlineCitations(
    answer: []const u8,
    source_count: usize,
    out_refs: *[]CitationRefSpan,
): usize {
    let written: usize = 0;
    let i: usize = 0;

    while (i < answer.len) {
        if (answer[i] != 91) { // '['
            i += 1;
            continue;
        }

        const br_end: usize = matchNumericBracketEnd(answer, i);
        if (br_end == 0) {
            i += 1;
            continue;
        }

        // Parse each comma-separated integer inside answer[(i+1)..(br_end-1)]
        let p: usize = i + 1;
        const inner_end: usize = br_end - 1;
        while (p < inner_end) {
            while (p < inner_end && (answer[p] == 32 || answer[p] == 9 || answer[p] == 44)) : (p += 1) {}
            if (p >= inner_end) break;

            let val: u64 = 0;
            while (p < inner_end && isAsciiDigit(answer[p])) : (p += 1) {
                const digit: u64 = (answer[p] - 48) as u64;
                if (val <= 0xFFFFFFFF) {
                    val = val * 10 + digit;
                }
            }

            const id_u32: u32 = if (val > 0xFFFFFFFF) 0xFFFFFFFF else @intCast(val);
            const in_b: bool = (id_u32 >= 1) && ((id_u32 as usize) <= source_count);

            if (written < out_refs.len) {
                out_refs[written] = CitationRefSpan{
                    source_id_1based: id_u32,
                    start_byte: i,
                    end_byte: br_end,
                    in_bounds: in_b,
                };
                written += 1;
            }
        }

        i = br_end;
    }

    return written;
}

/// Computes the zero-alloc ASCII case-folding lexical token coverage of `claim` in `source`
/// expressed in basis points `[0, 10000]` (where `10000` = 100.00% of unique claim tokens of
/// length `>= min_token_len` appear in `source`).
/// Inline numeric citation markers (`[1]`, `[2, 3]`) are excluded from claim tokens.
pub fn computeClaimSupportBps(
    claim: []const u8,
    source: []const u8,
    min_token_len: usize,
): u16 {
    if (claim.len == 0 || source.len == 0 || min_token_len == 0) return 0;

    let total_tokens: usize = 0;
    let supported_tokens: usize = 0;

    let i: usize = 0;
    while (i < claim.len) {
        if (claim[i] == 91) {
            const br_end: usize = matchNumericBracketEnd(claim, i);
            if (br_end > 0) {
                i = br_end;
                continue;
            }
        }

        if (!isTokenChar(claim[i])) {
            i += 1;
            continue;
        }

        const tok_start: usize = i;
        while (i < claim.len && isTokenChar(claim[i])) : (i += 1) {}
        const tok_len: usize = i - tok_start;
        if (tok_len < min_token_len) continue;

        const tok: []const u8 = claim[tok_start..i];
        if (claimPrefixHasToken(claim[0..tok_start], tok)) {
            continue;
        }

        total_tokens += 1;
        if (sourceContainsToken(source, tok)) {
            supported_tokens += 1;
        }
    }

    if (total_tokens == 0) return 0;
    const bps_u64: u64 = ((supported_tokens as u64) * 10000) / (total_tokens as u64);
    if (bps_u64 > 10000) return 10000;
    return @intCast(bps_u64);
}

/// Audits an entire RAG answer sentence-by-sentence against retrieved `sources`.
/// - Splits `answer` on `.`, `!`, `?`, and `\n` (preserving decimals like `3.14` and trailing `.[1]` markers).
/// - Extracts inline citations per sentence, validates 1-based source bounds, and computes multi-source
///   union lexical support coverage (`support_overlap_bps`).
/// - Writes up to `out_sentences.len` per-sentence audit records into `out_sentences` while accurately
///   aggregating all sentences into the returned `CitationAuditReport`.
pub fn auditRagCitations(
    answer: []const u8,
    sources: []const []const u8,
    cfg: CitationGateConfig,
    out_sentences: *[]CitationSentenceAudit,
): CitationAuditReport {
    let report = CitationAuditReport{};
    if (!cfg.isValid() || answer.len == 0) {
        report.passed_gate = false;
        return report;
    }

    let written_sentences: usize = 0;
    let support_sum_u64: u64 = 0;

    let pos: usize = 0;
    while (pos < answer.len) {
        // Skip leading whitespace before sentence
        while (pos < answer.len && isAsciiSpace(answer[pos])) : (pos += 1) {}
        if (pos >= answer.len) break;

        const raw_start: usize = pos;
        while (pos < answer.len) {
            const ch: u8 = answer[pos];
            if (ch == 10) { // '\n'
                pos += 1;
                break;
            }
            if (ch == 46 || ch == 33 || ch == 63) { // '.', '!', '?'
                // Do not split decimal numbers like 3.14
                if (ch == 46 && pos > raw_start && (pos + 1) < answer.len &&
                    isAsciiDigit(answer[pos - 1]) && isAsciiDigit(answer[pos + 1]))
                {
                    pos += 1;
                    continue;
                }
                // Consume consecutive punctuation (e.g. "..." or "?!")
                while (pos < answer.len && (answer[pos] == 46 || answer[pos] == 33 || answer[pos] == 63)) : (pos += 1) {}
                // Consume any immediately attached trailing citation brackets like ".[1][2]"
                while (pos < answer.len && answer[pos] == 91) {
                    const br_end: usize = matchNumericBracketEnd(answer, pos);
                    if (br_end == 0) break;
                    pos = br_end;
                }
                break;
            }
            pos += 1;
        }

        // Trim trailing whitespace from sentence span
        let s_end: usize = pos;
        while (s_end > raw_start && isAsciiSpace(answer[s_end - 1])) : (s_end -= 1) {}
        if (s_end <= raw_start) continue;

        const sent: []const u8 = answer[raw_start..s_end];
        report.total_sentences += 1;

        let refs_buf: [CITATION_MAX_REFS]CitationRefSpan = [CitationRefSpan{}; CITATION_MAX_REFS];
        let refs_s: []CitationRefSpan = refs_buf[0..];
        const ref_count: usize = extractInlineCitations(sent, sources.len, refs_s);

        let valid_ids_buf: [CITATION_MAX_REFS]u32 = [0; CITATION_MAX_REFS];
        let valid_count: usize = 0;
        let invalid_count: usize = 0;

        let r: usize = 0;
        while (r < ref_count) : (r += 1) {
            if (refs_buf[r].in_bounds) {
                valid_ids_buf[valid_count] = refs_buf[r].source_id_1based;
                valid_count += 1;
            } else {
                invalid_count += 1;
            }
        }

        report.valid_citation_refs += valid_count;
        report.out_of_bounds_refs += invalid_count;

        let overlap_bps: u16 = 0;
        let is_grounded: bool = false;

        if (ref_count > 0) {
            report.cited_sentences += 1;
            if (valid_count > 0) {
                overlap_bps = computeClaimUnionSupportBps(
                    sent,
                    sources,
                    valid_ids_buf[0..valid_count],
                    cfg.min_token_len,
                );
            }
            const oob_fail: bool = cfg.reject_out_of_bounds && (invalid_count > 0);
            is_grounded = (valid_count > 0) && !oob_fail && (overlap_bps >= cfg.min_support_overlap_bps);

            if (is_grounded) {
                report.grounded_sentences += 1;
            } else {
                report.weak_grounding_sentences += 1;
            }
            support_sum_u64 += overlap_bps as u64;
        }

        if (written_sentences < out_sentences.len) {
            out_sentences[written_sentences] = CitationSentenceAudit{
                start_byte: raw_start,
                end_byte: s_end,
                citation_count: ref_count,
                valid_citations: valid_count,
                invalid_citations: invalid_count,
                support_overlap_bps: overlap_bps,
                is_grounded: is_grounded,
            };
            written_sentences += 1;
        }
    }

    if (report.total_sentences == 0) {
        report.passed_gate = false;
        return report;
    }

    if (report.cited_sentences > 0) {
        report.avg_support_bps = @intCast(support_sum_u64 / (report.cited_sentences as u64));
    }

    const bounds_ok: bool = (!cfg.reject_out_of_bounds) || (report.out_of_bounds_refs == 0);
    const uncited_ok: bool = cfg.allow_uncited_sentences || (report.cited_sentences == report.total_sentences);
    const grounding_ok: bool = (report.weak_grounding_sentences == 0);
    const sources_ok: bool = (sources.len > 0);

    report.passed_gate = sources_ok && bounds_ok && uncited_ok && grounding_ok;
    return report;
}
