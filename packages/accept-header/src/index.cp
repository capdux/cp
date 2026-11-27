// @cap/accept-header — Zero-heap RFC 7231 HTTP Accept header parser & media-type negotiator.
// Clean-room MIT implementation modeled on crates.io `accept-header`,
// reusing `@cap/negotiator` for q-value parsing.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry lowercase LUT, and undefined stack buffers.

import { QualitySpec, parseWeightedList } from "../../negotiator/src/index.cp";

pub const ACCEPT_MAX_ENTRIES: usize = 16;

type AcphVec16 = @Vector(16, u8);

fn acphBuildLowerLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let c: usize = 0;
    while (c < 256) {
        if (c >= 65 && c <= 90) {
            lut[c] = (c + 32) as u8;
        } else {
            lut[c] = c as u8;
        }
        c += 1;
    }
    return lut;
}

const ACPH_LOWER_LUT: [256]u8 = acphBuildLowerLut();

fn acphEqAscii(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    const len = a.len;
    while (i + 16 <= len) {
        const va: AcphVec16 = a[i .. i + 16][0..16].*;
        const vb: AcphVec16 = b[i .. i + 16][0..16].*;
        if (!@reduce(.And, va == vb)) {
            let k: usize = 0;
            while (k < 16) {
                if (ACPH_LOWER_LUT[a[i + k] as usize] != ACPH_LOWER_LUT[b[i + k] as usize]) {
                    return false;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (ACPH_LOWER_LUT[a[i] as usize] != ACPH_LOWER_LUT[b[i] as usize]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn acphFindSlash(s: string): usize {
    let i: usize = 0;
    const len = s.len;
    const v_sl: AcphVec16 = @splat(47);
    while (i + 16 <= len) {
        const v: AcphVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 47) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (s[i] == 47) { return i; }
        i = i + 1;
    }
    return len;
}

fn acphIsStar(s: string): bool {
    return s.len == 1 && s[0] == 42;
}

// Returns specificity: 3 = exact (type/sub), 2 = type/*, 1 = */*, 0 = invalid
fn acphMediaSpecificity(range: string): u8 {
    const slash: usize = acphFindSlash(range);
    if (slash == 0 || slash + 1 >= range.len) { return 0; }
    const main_type: string = range[0..slash];
    const sub_type: string = range[slash + 1..range.len];
    if (acphFindSlash(sub_type) < sub_type.len) { return 0; }
    const main_star: bool = acphIsStar(main_type);
    const sub_star: bool = acphIsStar(sub_type);
    if (main_star && sub_star) { return 1; }
    if (main_star && !sub_star) { return 0; }
    if (sub_star) { return 2; }
    return 3;
}

fn acphMatchesRange(range: string, mime: string): bool {
    const r_slash: usize = acphFindSlash(range);
    const m_slash: usize = acphFindSlash(mime);
    if (r_slash == 0 || r_slash + 1 >= range.len) { return false; }
    if (m_slash == 0 || m_slash + 1 >= mime.len) { return false; }

    const r_type: string = range[0..r_slash];
    const r_sub: string = range[r_slash + 1..range.len];
    const m_type: string = mime[0..m_slash];
    const m_sub: string = mime[m_slash + 1..mime.len];

    if (acphIsStar(r_type) && acphIsStar(r_sub)) { return true; }
    if (!acphEqAscii(r_type, m_type)) { return false; }
    if (acphIsStar(r_sub)) { return true; }
    return acphEqAscii(r_sub, m_sub);
}

/// Single parsed media-range entry from an HTTP `Accept` header.
/// `specificity`: `3` = exact `type/sub`, `2` = `type/*`, `1` = `*/*`.
pub struct AcceptEntry {
    pub media_range: string,
    pub quality_milli: u16,
    pub specificity: u8,
}

/// Zero-heap parsed HTTP `Accept` header holding up to 16 prioritized media-range entries.
pub struct AcceptHeader {
    pub entries: [16]AcceptEntry,
    pub count: usize,

    /// Parses an HTTP `Accept` header string, filtering `q=0` entries and stably sorting
    /// by descending `quality_milli`, then descending `specificity`.
    pub static fn parse(header: string): AcceptHeader {
        let specs: [16]QualitySpec = undefined;
        const raw_count: usize = parseWeightedList(header, specs[0..16]);

        let out_entries: [16]AcceptEntry = undefined;
        let n: usize = 0;
        let i: usize = 0;
        while (i < raw_count && n < ACCEPT_MAX_ENTRIES) {
            const sp: QualitySpec = specs[i];
            if (sp.q_milli > 0) {
                const spec: u8 = acphMediaSpecificity(sp.value);
                if (spec > 0) {
                    out_entries[n] = AcceptEntry {
                        media_range: sp.value,
                        quality_milli: sp.q_milli as u16,
                        specificity: spec,
                    };
                    n = n + 1;
                }
            }
            i = i + 1;
        }

        // Stable insertion sort: descending quality_milli, then descending specificity
        let k: usize = 1;
        while (k < n) {
            const cur: AcceptEntry = out_entries[k];
            let j: usize = k;
            while (j > 0) {
                const prev: AcceptEntry = out_entries[j - 1];
                const should_swap: bool = (prev.quality_milli < cur.quality_milli) ||
                    (prev.quality_milli == cur.quality_milli && prev.specificity < cur.specificity);
                if (!should_swap) { break; }
                out_entries[j] = prev;
                j = j - 1;
            }
            out_entries[j] = cur;
            k = k + 1;
        }

        return AcceptHeader {
            entries: out_entries,
            count: n,
        };
    }

    /// Returns the number of accepted (`q > 0`) media-range entries.
    pub fn len(): usize {
        return _.count;
    }

    /// Returns `true` if `mime` matches any accepted media-range entry.
    pub fn accepts(mime: string): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (acphMatchesRange(_.entries[i].media_range, mime)) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    /// Selects the highest-priority MIME type from `available` matching this `AcceptHeader`,
    /// or returns `null` if none match.
    pub fn bestMatch(available: []string): string? {
        let best_idx: usize = available.len;
        let best_q: u16 = 0;
        let best_spec: u8 = 0;

        let ci: usize = 0;
        while (ci < available.len) {
            const cand: string = available[ci];
            let cand_spec: u8 = 0;
            let cand_q: u16 = 0;
            let ei: usize = 0;
            while (ei < _.count) {
                const e: AcceptEntry = _.entries[ei];
                if (acphMatchesRange(e.media_range, cand)) {
                    if (e.specificity > cand_spec || (e.specificity == cand_spec && e.quality_milli > cand_q)) {
                        cand_spec = e.specificity;
                        cand_q = e.quality_milli;
                    }
                }
                ei = ei + 1;
            }

            if (cand_spec > 0) {
                const better: bool = (best_idx >= available.len) ||
                    (cand_q > best_q) ||
                    (cand_q == best_q && cand_spec > best_spec);
                if (better) {
                    best_idx = ci;
                    best_q = cand_q;
                    best_spec = cand_spec;
                }
            }
            ci = ci + 1;
        }

        if (best_idx >= available.len) { return null; }
        return available[best_idx];
    }
}

/// Convenience one-shot media-type negotiation against an HTTP `Accept` header string.
pub fn negotiateMediaType(header: string, available: []string): string? {
    const parsed: AcceptHeader = AcceptHeader.parse(header);
    return parsed.bestMatch(available);
}
