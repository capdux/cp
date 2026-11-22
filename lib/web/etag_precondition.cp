// Origin: Pure-CP Stdlib (RFC 9110 §13 Conditional Request & ETag Precondition Evaluator) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 9110 §8.8.3 & §13 HTTP Conditional Request & ETag Evaluation Engine:
//   - Strict RFC 9110 ETag parser (`parseEntityTag`): distinguishes strong `"tag"` vs weak `W/"tag"`,
//     rejects lowercase `w/`, unclosed quotes, and illegal non-etagc bytes
//   - Strong & weak comparison (`etagStrongEqual`, `etagWeakEqual`) and quote-aware comma-separated
//     ETag list / `*` wildcard matching (`etagListMatchesStrong`, `etagListMatchesWeak`, `validateEtagList`)
//   - Exact RFC 9110 §13.2.2 5-step precedence evaluator (`evaluatePreconditions`):
//     If-Match -> If-Unmodified-Since -> If-None-Match -> If-Modified-Since -> If-Range

// ============================================================================
// 1. Public Enums & Data Models
// ============================================================================

/// HTTP method subset governing conditional request semantics (RFC 9110 §13.2.2).
pub enum PreconditionMethod {
    get,
    head,
    put,
    post,
    delete,
    patch,
    options,
}

/// Final disposition produced by `evaluatePreconditions`.
pub enum PreconditionVerdict {
    proceed_200,
    serve_range_206,
    fallback_full_200,
    not_modified_304,
    precondition_failed_412,
}

/// Specific header rule that produced the `PreconditionResult`.
pub enum PreconditionTrigger {
    none,
    if_match,
    if_unmodified_since,
    if_none_match,
    if_modified_since,
    if_range_matched,
    if_range_mismatched,
}

/// Parsed zero-copy RFC 9110 Entity-Tag (`tag` points into the original header slice without quotes).
pub struct EntityTag {
    weak: bool = false;
    tag: []const u8 = "";
}

/// Conditional request headers and method inputs for `evaluatePreconditions`.
pub struct PreconditionRequest {
    method: PreconditionMethod = PreconditionMethod.get;
    if_match: []const u8 = "";
    if_unmodified_since_sec: i64? = null;
    if_none_match: []const u8 = "";
    if_modified_since_sec: i64? = null;
    has_range: bool = false;
    if_range_etag: []const u8 = "";
    if_range_date_sec: i64? = null;
}

/// Current target resource state and representation validators.
pub struct ResourceValidators {
    resource_exists: bool = true;
    etag: []const u8 = "";
    last_modified_sec: i64? = null;
}

/// Evaluation verdict returned by `evaluatePreconditions`.
pub struct PreconditionResult {
    verdict: PreconditionVerdict = PreconditionVerdict.proceed_200;
    http_status: u16 = 200;
    trigger: PreconditionTrigger = PreconditionTrigger.none;
    serve_range: bool = false;
}

// ============================================================================
// 2. Internal Byte & Slice Helpers
// ============================================================================

fn isOws(b: u8): bool {
    return b == ' ' || b == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) : (start += 1) {}
    while (end > start && isOws(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

/// RFC 9110 §8.8.3: `etagc = %x21 / %x23-7E / obs-text (%x80-FF)`.
fn isEtagc(b: u8): bool {
    return b == 0x21 || (b >= 0x23 && b <= 0x7E) || b >= 0x80;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isReadMethod(m: PreconditionMethod): bool {
    return m == PreconditionMethod.get || m == PreconditionMethod.head;
}

// ============================================================================
// 3. RFC 9110 §8.8.3 Entity-Tag Parser & Comparators
// ============================================================================

/// Parses a single RFC 9110 Entity-Tag (`"abc"` or `W/"abc"`).
/// Rejects lowercase `w/`, unclosed quotes, and any non-`etagc` bytes inside quotes.
pub fn parseEntityTag(raw: []const u8): EntityTag? {
    const s = trimOws(raw);
    if (s.len < 2) {
        return null;
    }

    let weak: bool = false;
    let body: []const u8 = s;

    if (s[0] == 'W') {
        if (s.len < 4 || s[1] != '/') {
            return null;
        }
        weak = true;
        body = s[2..];
    } else if (s[0] == 'w') {
        // RFC 9110 §8.8.3: weak indicator is case-sensitive `W/`
        return null;
    }

    if (body.len < 2 || body[0] != '"' || body[body.len - 1] != '"') {
        return null;
    }

    const inner = body[1..(body.len - 1)];
    let i: usize = 0;
    while (i < inner.len) : (i += 1) {
        if (!isEtagc(inner[i])) {
            return null;
        }
    }

    return EntityTag{
        weak: weak,
        tag: inner,
    };
}

/// RFC 9110 §8.8.3.2 Strong Comparison: both tags must be strong and byte-identical.
pub fn etagStrongEqual(a: EntityTag, b: EntityTag): bool {
    if (a.weak || b.weak) {
        return false;
    }
    return bytesEqual(a.tag, b.tag);
}

/// RFC 9110 §8.8.3.2 Weak Comparison: opaque tags must be byte-identical (ignores weak flag).
pub fn etagWeakEqual(a: EntityTag, b: EntityTag): bool {
    return bytesEqual(a.tag, b.tag);
}

// ============================================================================
// 4. Quote-Aware Comma-Separated ETag List & Wildcard `*` Matcher
// ============================================================================

fn matchEtagListInternal(
    header_val: []const u8,
    current: EntityTag?,
    resource_exists: bool,
    strong: bool,
): bool {
    const trimmed = trimOws(header_val);
    if (trimmed.len == 0) {
        return false;
    }
    // Wildcard `*` matches any existing representation, and fails when resource does not exist.
    if (trimmed.len == 1 && trimmed[0] == '*') {
        return resource_exists;
    }
    if (!resource_exists) {
        return false;
    }
    const cur_tag = current ?? return false;

    let pos: usize = 0;
    while (pos < trimmed.len) {
        // Skip leading OWS and empty comma items
        while (pos < trimmed.len && (isOws(trimmed[pos]) || trimmed[pos] == ',')) : (pos += 1) {}
        if (pos >= trimmed.len) break;

        const item_start: usize = pos;
        let in_quotes: bool = false;
        while (pos < trimmed.len) : (pos += 1) {
            const c = trimmed[pos];
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                break;
            }
        }
        if (in_quotes) {
            // Unclosed quote in header list -> malformed, abort matching
            return false;
        }

        const item = trimOws(trimmed[item_start..pos]);
        if (item.len > 0) {
            if (cand := parseEntityTag(item)) {
                if (strong) {
                    if (etagStrongEqual(cand, cur_tag)) return true;
                } else {
                    if (etagWeakEqual(cand, cur_tag)) return true;
                }
            }
        }
        if (pos < trimmed.len && trimmed[pos] == ',') {
            pos += 1;
        }
    }

    return false;
}

/// Evaluates an ETag list header (`If-Match`) using strong comparison and `*` wildcard rules.
pub fn etagListMatchesStrong(
    header_val: []const u8,
    current: EntityTag?,
    resource_exists: bool,
): bool {
    return matchEtagListInternal(header_val, current, resource_exists, true);
}

/// Evaluates an ETag list header (`If-None-Match`) using weak comparison and `*` wildcard rules.
pub fn etagListMatchesWeak(
    header_val: []const u8,
    current: EntityTag?,
    resource_exists: bool,
): bool {
    return matchEtagListInternal(header_val, current, resource_exists, false);
}

/// Strictly validates that `header_val` is either `*` or a well-formed comma-separated ETag list.
pub fn validateEtagList(header_val: []const u8): bool {
    const trimmed = trimOws(header_val);
    if (trimmed.len == 0) {
        return false;
    }
    if (trimmed.len == 1 && trimmed[0] == '*') {
        return true;
    }

    let valid_items: usize = 0;
    let pos: usize = 0;
    while (pos < trimmed.len) {
        while (pos < trimmed.len && (isOws(trimmed[pos]) || trimmed[pos] == ',')) : (pos += 1) {}
        if (pos >= trimmed.len) break;

        const item_start: usize = pos;
        let in_quotes: bool = false;
        while (pos < trimmed.len) : (pos += 1) {
            const c = trimmed[pos];
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                break;
            }
        }
        if (in_quotes) {
            return false;
        }
        const item = trimOws(trimmed[item_start..pos]);
        if (item.len > 0) {
            if (parseEntityTag(item) == null) {
                return false;
            }
            valid_items += 1;
        }
        if (pos < trimmed.len && trimmed[pos] == ',') {
            pos += 1;
        }
    }

    return valid_items > 0;
}

// ============================================================================
// 5. RFC 9110 §13.2.2 Exact Precedence Precondition Evaluator
// ============================================================================

/// Evaluates HTTP conditional request headers strictly following RFC 9110 §13.2.2 precedence:
///   1. `If-Match` (strong comparison -> 412 on mismatch)
///   2. `If-Unmodified-Since` (only when `If-Match` absent -> 412 if modified after timestamp)
///   3. `If-None-Match` (weak comparison -> 304 on GET/HEAD match, 412 on write methods)
///   4. `If-Modified-Since` (only when `If-None-Match` absent & GET/HEAD -> 304 if not modified)
///   5. `If-Range` (when GET + `has_range` -> 206 `serve_range` on strong match, else 200 `fallback_full_200`)
pub fn evaluatePreconditions(
    req: PreconditionRequest,
    res: ResourceValidators,
): PreconditionResult {
    const cur_etag: EntityTag? = if (res.resource_exists && res.etag.len > 0) parseEntityTag(res.etag) else null;

    // Step 1: If-Match (strong comparison)
    const has_if_match: bool = trimOws(req.if_match).len > 0;
    if (has_if_match) {
        if (!etagListMatchesStrong(req.if_match, cur_etag, res.resource_exists)) {
            return PreconditionResult{
                verdict: PreconditionVerdict.precondition_failed_412,
                http_status: 412,
                trigger: PreconditionTrigger.if_match,
                serve_range: false,
            };
        }
    } else if (ius := req.if_unmodified_since_sec) {
        // Step 2: If-Unmodified-Since (ignored when If-Match is present)
        if (res.resource_exists) {
            if (lm := res.last_modified_sec) {
                if (lm > ius) {
                    return PreconditionResult{
                        verdict: PreconditionVerdict.precondition_failed_412,
                        http_status: 412,
                        trigger: PreconditionTrigger.if_unmodified_since,
                        serve_range: false,
                    };
                }
            }
        }
    }

    // Step 3: If-None-Match (weak comparison)
    const has_if_none_match: bool = trimOws(req.if_none_match).len > 0;
    if (has_if_none_match) {
        if (etagListMatchesWeak(req.if_none_match, cur_etag, res.resource_exists)) {
            if (isReadMethod(req.method)) {
                return PreconditionResult{
                    verdict: PreconditionVerdict.not_modified_304,
                    http_status: 304,
                    trigger: PreconditionTrigger.if_none_match,
                    serve_range: false,
                };
            }
            return PreconditionResult{
                verdict: PreconditionVerdict.precondition_failed_412,
                http_status: 412,
                trigger: PreconditionTrigger.if_none_match,
                serve_range: false,
            };
        }
    } else if (isReadMethod(req.method)) {
        // Step 4: If-Modified-Since (ignored when If-None-Match is present, or method != GET/HEAD)
        if (ims := req.if_modified_since_sec) {
            if (res.resource_exists) {
                if (lm := res.last_modified_sec) {
                    if (lm <= ims) {
                        return PreconditionResult{
                            verdict: PreconditionVerdict.not_modified_304,
                            http_status: 304,
                            trigger: PreconditionTrigger.if_modified_since,
                            serve_range: false,
                        };
                    }
                }
            }
        }
    }

    // Step 5: If-Range (only applies to GET requests carrying a Range header)
    if (req.method == PreconditionMethod.get && req.has_range) {
        const ir_etag_trimmed = trimOws(req.if_range_etag);
        if (ir_etag_trimmed.len > 0) {
            if (ir_tag := parseEntityTag(ir_etag_trimmed)) {
                if (ct := cur_etag) {
                    if (etagStrongEqual(ir_tag, ct)) {
                        return PreconditionResult{
                            verdict: PreconditionVerdict.serve_range_206,
                            http_status: 206,
                            trigger: PreconditionTrigger.if_range_matched,
                            serve_range: true,
                        };
                    }
                }
            }
            return PreconditionResult{
                verdict: PreconditionVerdict.fallback_full_200,
                http_status: 200,
                trigger: PreconditionTrigger.if_range_mismatched,
                serve_range: false,
            };
        } else if (ir_date := req.if_range_date_sec) {
            if (res.resource_exists) {
                if (lm := res.last_modified_sec) {
                    if (lm == ir_date) {
                        return PreconditionResult{
                            verdict: PreconditionVerdict.serve_range_206,
                            http_status: 206,
                            trigger: PreconditionTrigger.if_range_matched,
                            serve_range: true,
                        };
                    }
                }
            }
            return PreconditionResult{
                verdict: PreconditionVerdict.fallback_full_200,
                http_status: 200,
                trigger: PreconditionTrigger.if_range_mismatched,
                serve_range: false,
            };
        }

        // Range present without If-Range -> serve partial content 206
        return PreconditionResult{
            verdict: PreconditionVerdict.serve_range_206,
            http_status: 206,
            trigger: PreconditionTrigger.none,
            serve_range: true,
        };
    }

    // Step 6: All preconditions passed -> proceed normally
    return PreconditionResult{
        verdict: PreconditionVerdict.proceed_200,
        http_status: 200,
        trigger: PreconditionTrigger.none,
        serve_range: false,
    };
}
