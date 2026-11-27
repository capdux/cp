// @cap/bcp47 — Zero-allocation RFC 5646 / BCP-47 language tag parser, canonicalizer & matcher
// Clean-room MIT implementation inspired by npm `bcp-47`, `bcp-47-match`, and Rust `unic-langid`.

import { isAlpha, isDigit, isAlnum, toLowerByte, toUpperByte, trim, eqIgnoreCase } from "cap:ascii";

pub struct Bcp47Tag {
    pub valid: bool,
    pub language: string,
    pub script: string,
    pub region: string,
    pub variant: string,
    pub extension: string,
    pub private_use: string,

    pub static fn invalid() -> Bcp47Tag {
        return Bcp47Tag{
            valid: false,
            language: "",
            script: "",
            region: "",
            variant: "",
            extension: "",
            private_use: "",
        };
    }
}

struct SubtagSpan {
    start: usize,
    end: usize,
}

// ---------------------------------------------------------------------------
// Internal Character & Comparison Helpers
// ---------------------------------------------------------------------------

fn eqSubtagNormalized(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        let ca = toLowerByte(a[i]);
        let cb = toLowerByte(b[i]);
        if (ca == '_') ca = '-';
        if (cb == '_') cb = '-';
        if (ca != cb) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isAllAlpha(s: string) -> bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) {
        if (!isAlpha(s[i])) return false;
        i += 1;
    }
    return true;
}

fn isAllDigit(s: string) -> bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) return false;
        i += 1;
    }
    return true;
}

fn isAllAlphaNum(s: string) -> bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) {
        if (!isAlnum(s[i])) return false;
        i += 1;
    }
    return true;
}

fn isVariantSubtag(s: string) -> bool {
    if (!isAllAlphaNum(s)) return false;
    if (s.len >= 5 && s.len <= 8) return true;
    if (s.len == 4 && isDigit(s[0])) return true;
    return false;
}

fn isSingletonSubtag(s: string) -> bool {
    if (s.len != 1) return false;
    const c = toLowerByte(s[0]);
    if (c == 'x') return false;
    return isAlnum(c);
}

fn isPrivateIntro(s: string) -> bool {
    return s.len == 1 && (s[0] == 'x' || s[0] == 'X');
}

// ---------------------------------------------------------------------------
// 1. RFC 5646 Parser (Zero Heap Allocation)
// ---------------------------------------------------------------------------

/// Parses an RFC 5646 / BCP-47 language tag (accepts both `'-'` and `'_'` separators).
/// All returned string fields are zero-copy slices into `raw`.
pub fn parseBcp47(raw: string) -> Bcp47Tag {
    const s: string = trim(raw);
    if (s.len == 0) {
        return Bcp47Tag.invalid();
    }

    // Split into up to 32 subtags on '-' or '_'
    let spans: [32]SubtagSpan = undefined;
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) {
        if (i == s.len || s[i] == '-' || s[i] == '_') {
            const len = i - start;
            if (len == 0 || len > 8 || count >= 32) {
                return Bcp47Tag.invalid();
            }
            if (!isAllAlphaNum(s[start..i])) {
                return Bcp47Tag.invalid();
            }
            spans[count] = SubtagSpan{ start: start, end: i };
            count += 1;
            start = i + 1;
        }
        i += 1;
    }

    if (count == 0) {
        return Bcp47Tag.invalid();
    }

    const first = s[spans[0].start..spans[0].end];

    // Pure private-use tag: `x-...`
    if (isPrivateIntro(first)) {
        if (count < 2) {
            return Bcp47Tag.invalid();
        }
        return Bcp47Tag{
            valid: true,
            language: "",
            script: "",
            region: "",
            variant: "",
            extension: "",
            private_use: s[spans[0].start..spans[count - 1].end],
        };
    }

    // Primary language subtag: 2-3 ASCII letters
    if (!isAllAlpha(first) || first.len < 2 || first.len > 3) {
        return Bcp47Tag.invalid();
    }

    let language = first;
    let script: string = "";
    let region: string = "";
    let variant: string = "";
    let extension: string = "";
    let private_use: string = "";

    let idx: usize = 1;

    // Optional Script subtag: 4 ASCII letters (e.g. Hans, Latn, Cyrl)
    if (idx < count) {
        const sub = s[spans[idx].start..spans[idx].end];
        if (sub.len == 4 && isAllAlpha(sub)) {
            script = sub;
            idx += 1;
        }
    }

    // Optional Region subtag: 2 ASCII letters (ISO 3166) or 3 digits (UN M.49)
    if (idx < count) {
        const sub = s[spans[idx].start..spans[idx].end];
        if ((sub.len == 2 && isAllAlpha(sub)) || (sub.len == 3 && isAllDigit(sub))) {
            region = sub;
            idx += 1;
        }
    }

    // Optional Variant subtag(s): 5-8 alphanum or digit + 3 alphanum (e.g. 1996, rozaj)
    if (idx < count && isVariantSubtag(s[spans[idx].start..spans[idx].end])) {
        const var_start = spans[idx].start;
        let var_end = spans[idx].end;
        idx += 1;
        while (idx < count && isVariantSubtag(s[spans[idx].start..spans[idx].end])) {
            var_end = spans[idx].end;
            idx += 1;
        }
        variant = s[var_start..var_end];
    }

    // Optional Extension subtag(s): singleton (non-'x') followed by >= 1 subtag of len 2..8
    if (idx < count && isSingletonSubtag(s[spans[idx].start..spans[idx].end])) {
        const ext_start = spans[idx].start;
        let ext_end = spans[idx].end;
        while (idx < count && isSingletonSubtag(s[spans[idx].start..spans[idx].end])) {
            idx += 1;
            let ext_parts: usize = 0;
            while (idx < count) {
                const sub = s[spans[idx].start..spans[idx].end];
                if (sub.len < 2 || sub.len > 8) {
                    break;
                }
                ext_end = spans[idx].end;
                ext_parts += 1;
                idx += 1;
            }
            if (ext_parts == 0) {
                return Bcp47Tag.invalid();
            }
        }
        extension = s[ext_start..ext_end];
    }

    // Optional trailing Private-use subtag: `x-...`
    if (idx < count && isPrivateIntro(s[spans[idx].start..spans[idx].end])) {
        const priv_start = spans[idx].start;
        idx += 1;
        if (idx >= count) {
            return Bcp47Tag.invalid();
        }
        const priv_end = spans[count - 1].end;
        idx = count;
        private_use = s[priv_start..priv_end];
    }

    // Any leftover subtag means invalid ordering or syntax
    if (idx != count) {
        return Bcp47Tag.invalid();
    }

    return Bcp47Tag{
        valid: true,
        language: language,
        script: script,
        region: region,
        variant: variant,
        extension: extension,
        private_use: private_use,
    };
}

// ---------------------------------------------------------------------------
// 2. RFC 5646 Canonicalization (Zero Heap Allocation)
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendLowerHyphen(out: []u8, start_pos: usize, s: string) -> usize {
    let pos = start_pos;
    let i: usize = 0;
    while (i < s.len) {
        const c = if (s[i] == '_') '-' as u8 else toLowerByte(s[i]);
        pos = appendByte(out, pos, c);
        i += 1;
    }
    return pos;
}

fn appendUpper(out: []u8, start_pos: usize, s: string) -> usize {
    let pos = start_pos;
    let i: usize = 0;
    while (i < s.len) {
        pos = appendByte(out, pos, toUpperByte(s[i]));
        i += 1;
    }
    return pos;
}

fn appendTitle(out: []u8, start_pos: usize, s: string) -> usize {
    let pos = start_pos;
    let i: usize = 0;
    while (i < s.len) {
        const c = if (i == 0) toUpperByte(s[i]) else toLowerByte(s[i]);
        pos = appendByte(out, pos, c);
        i += 1;
    }
    return pos;
}

/// Formats `tag` with canonical RFC 5646 casing and `'-'` separators into `out`
/// (e.g. `"ZH_hans_cn"` -> `"zh-Hans-CN"`).
pub fn canonicalizeBcp47(tag: Bcp47Tag, out: []u8) -> string {
    if (!tag.valid) {
        return out[0..0];
    }
    let pos: usize = 0;
    let emitted = false;

    if (tag.language.len > 0) {
        pos = appendLowerHyphen(out, pos, tag.language);
        emitted = true;
    }
    if (tag.script.len > 0) {
        if (emitted) pos = appendByte(out, pos, '-');
        pos = appendTitle(out, pos, tag.script);
        emitted = true;
    }
    if (tag.region.len > 0) {
        if (emitted) pos = appendByte(out, pos, '-');
        pos = appendUpper(out, pos, tag.region);
        emitted = true;
    }
    if (tag.variant.len > 0) {
        if (emitted) pos = appendByte(out, pos, '-');
        pos = appendLowerHyphen(out, pos, tag.variant);
        emitted = true;
    }
    if (tag.extension.len > 0) {
        if (emitted) pos = appendByte(out, pos, '-');
        pos = appendLowerHyphen(out, pos, tag.extension);
        emitted = true;
    }
    if (tag.private_use.len > 0) {
        if (emitted) pos = appendByte(out, pos, '-');
        pos = appendLowerHyphen(out, pos, tag.private_use);
    }

    return out[0..pos];
}

// ---------------------------------------------------------------------------
// 3. Fallback Locale Matching
// ---------------------------------------------------------------------------

fn scoreLocaleMatch(req: Bcp47Tag, cand: Bcp47Tag) -> u32 {
    if (!req.valid || !cand.valid) {
        return 0;
    }

    // Pure private-use tags match only exact private_use
    if (req.language.len == 0) {
        if (cand.language.len == 0 && eqSubtagNormalized(req.private_use, cand.private_use)) {
            return 100;
        }
        return 0;
    }

    if (!eqIgnoreCase(req.language, cand.language)) {
        return 0;
    }

    const script_eq = eqIgnoreCase(req.script, cand.script);
    const region_eq = eqIgnoreCase(req.region, cand.region);
    const variant_eq = eqSubtagNormalized(req.variant, cand.variant);

    // 1. Exact match (language + script + region + variant)
    if (script_eq && region_eq && variant_eq) {
        return 100;
    }

    // 2. Language + Script + Region match
    if (req.script.len > 0 && req.region.len > 0 && script_eq && region_eq) {
        return 90;
    }

    // 3. Language + Script match
    if (req.script.len > 0 && script_eq) {
        return if (cand.region.len == 0) 75 else 70;
    }

    // 4. Language + Region match
    if (req.region.len > 0 && region_eq) {
        return if (cand.script.len == 0) 55 else 50;
    }

    // 5. Language-only fallback (prefer bare language tag over different region/script)
    if (cand.script.len == 0 && cand.region.len == 0) {
        return 30;
    }
    return 20;
}

/// Selects the best matching locale from `supported` for `requested`, prioritizing:
/// Exact match > Language+Script > Language+Region > Language-only fallback.
pub fn matchLocale(requested: Bcp47Tag, supported: []const string) -> string? {
    if (!requested.valid || supported.len == 0) {
        return null;
    }
    let best_score: u32 = 0;
    let best_idx: usize = 0;
    let i: usize = 0;
    while (i < supported.len) {
        const cand = parseBcp47(supported[i]);
        const score = scoreLocaleMatch(requested, cand);
        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
        i += 1;
    }
    if (best_score == 0) {
        return null;
    }
    return supported[best_idx];
}
