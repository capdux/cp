// @cap/glob-parent — Zero-allocation glob parent extractor, glob detector & escaper for CP
// Clean-room MIT implementation inspired by npm `glob-parent`, reusing `@cap/is-extglob`.

import { rfindByte } from "cap:ascii";
import { findExtglobStart } from "../../is-extglob/src/index.cp";

pub struct GlobSplit {
    pub base: string,
    pub glob: string,
    pub is_glob: bool,
}

fn findMatchingBracket(s: string, openIdx: usize) -> usize? {
    let i = openIdx + 1;
    if (i >= s.len) {
        return null;
    }
    if (s[i] == '!' || s[i] == '^') {
        i += 1;
    }
    let contentCount: usize = 0;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            contentCount += 1;
            continue;
        }
        if (s[i] == ']' && contentCount > 0) {
            return i;
        }
        i += 1;
        contentCount += 1;
    }
    return null;
}

fn findMatchingBrace(s: string, openIdx: usize) -> usize? {
    let depth: usize = 1;
    let i = openIdx + 1;
    let hasComma = false;
    let hasRange = false;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == '{') {
            depth += 1;
        } else if (s[i] == '}') {
            depth -= 1;
            if (depth == 0) {
                if (hasComma || hasRange) {
                    return i;
                }
                return null;
            }
        } else if (depth == 1) {
            if (s[i] == ',') {
                hasComma = true;
            } else if (s[i] == '.' && i + 1 < s.len && s[i + 1] == '.') {
                hasRange = true;
            }
        }
        i += 1;
    }
    return null;
}

fn findFirstGlobPos(s: string) -> usize? {
    const extStart = findExtglobStart(s);
    let i: usize = 0;
    while (i < s.len) {
        if (extStart >= 0 && i == (extStart as usize)) {
            return i;
        }
        const c = s[i];
        if (c == '\\') {
            if (i + 1 < s.len) {
                i += 2;
                continue;
            }
            return null;
        }
        if (c == '*' || c == '?') {
            return i;
        }
        if (c == '[') {
            if (findMatchingBracket(s, i) != null) {
                return i;
            }
        }
        if (c == '{') {
            if (findMatchingBrace(s, i) != null) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn stripLeadingNegation(raw: string) -> string {
    if (raw.len > 0 && raw[0] == '!') {
        if (findExtglobStart(raw) == 0) {
            return raw;
        }
        return raw[1..raw.len];
    }
    return raw;
}

fn trimTrailingSlashes(s: string) -> string {
    let end = s.len;
    while (end > 1 && s[end - 1] == '/') {
        end -= 1;
    }
    return s[0..end];
}

pub fn isGlob(pattern: string) -> bool {
    if (pattern.len == 0) {
        return false;
    }
    return findFirstGlobPos(pattern) != null;
}

pub fn globParent(pattern: string) -> string {
    const clean = trimTrailingSlashes(stripLeadingNegation(pattern));
    if (clean.len == 0) {
        return ".";
    }
    if (clean.len == 1 && clean[0] == '/') {
        return "/";
    }

    const firstGlob = findFirstGlobPos(clean);
    if (firstGlob == null) {
        const slashOpt = rfindByte(clean, '/');
        if (slashOpt == null) {
            return ".";
        }
        const slashIdx = slashOpt ?? (0 as usize);
        if (slashIdx == 0) {
            return "/";
        }
        return trimTrailingSlashes(clean[0..slashIdx]);
    }

    const gPos = firstGlob ?? (0 as usize);
    const slashOpt = rfindByte(clean[0..gPos], '/');
    if (slashOpt == null) {
        return ".";
    }
    const slashIdx = slashOpt ?? (0 as usize);
    if (slashIdx == 0) {
        return "/";
    }
    return trimTrailingSlashes(clean[0..slashIdx]);
}

pub fn splitGlobBase(pattern: string) -> GlobSplit {
    const clean = stripLeadingNegation(pattern);
    if (clean.len == 0) {
        return GlobSplit{ base: ".", glob: "", is_glob: false };
    }
    if (clean.len == 1 && clean[0] == '/') {
        return GlobSplit{ base: "/", glob: "", is_glob: false };
    }

    const firstGlob = findFirstGlobPos(clean);
    if (firstGlob == null) {
        const trimmed = trimTrailingSlashes(clean);
        const slashOpt = rfindByte(trimmed, '/');
        if (slashOpt == null) {
            return GlobSplit{ base: ".", glob: trimmed, is_glob: false };
        }
        const slashIdx = slashOpt ?? (0 as usize);
        const base = if (slashIdx == 0) { "/" } else { trimTrailingSlashes(trimmed[0..slashIdx]) };
        const tail = trimmed[(slashIdx + 1)..trimmed.len];
        return GlobSplit{ base: base, glob: tail, is_glob: false };
    }

    const gPos = firstGlob ?? (0 as usize);
    const slashOpt = rfindByte(clean[0..gPos], '/');
    if (slashOpt == null) {
        return GlobSplit{ base: ".", glob: clean, is_glob: true };
    }
    const slashIdx = slashOpt ?? (0 as usize);
    const base = if (slashIdx == 0) { "/" } else { trimTrailingSlashes(clean[0..slashIdx]) };
    const rest = clean[(slashIdx + 1)..clean.len];
    return GlobSplit{ base: base, glob: rest, is_glob: true };
}

fn isGlobSpecial(c: u8) -> bool {
    return c == '*' || c == '?' || c == '[' || c == ']' || c == '{' || c == '}' || c == '(' || c == ')' || c == '!' || c == '\\';
}

pub fn escapeGlob(raw: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len && pos < out.len) {
        const c = raw[i];
        if (isGlobSpecial(c)) {
            if (pos + 2 > out.len) {
                break;
            }
            out[pos] = '\\';
            out[pos + 1] = c;
            pos += 2;
        } else {
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return out[0..pos];
}

export {
    GlobSplit,
    isGlob,
    globParent,
    splitGlobBase,
    escapeGlob,
};
