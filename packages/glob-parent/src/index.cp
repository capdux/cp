// @cap/glob-parent — Zero-allocation glob parent extractor, glob detector & escaper for CP
// Clean-room MIT implementation inspired by npm `glob-parent`, reusing `@cap/is-extglob`.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUT, and @memcpy.

import { rfindByte } from "cap:ascii";
import { findExtglobStart } from "../../is-extglob/src/index.cp";

type GpVec16 = @Vector(16, u8);

const GP_FLAG_SPECIAL: u8 = 1;

fn gpBuildSpecialLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut['*' as usize] = GP_FLAG_SPECIAL;
    lut['?' as usize] = GP_FLAG_SPECIAL;
    lut['[' as usize] = GP_FLAG_SPECIAL;
    lut[']' as usize] = GP_FLAG_SPECIAL;
    lut['{' as usize] = GP_FLAG_SPECIAL;
    lut['}' as usize] = GP_FLAG_SPECIAL;
    lut['(' as usize] = GP_FLAG_SPECIAL;
    lut[')' as usize] = GP_FLAG_SPECIAL;
    lut['!' as usize] = GP_FLAG_SPECIAL;
    lut['\\' as usize] = GP_FLAG_SPECIAL;
    return lut;
}

const GP_SPECIAL_LUT: [256]u8 = gpBuildSpecialLut();

pub struct GlobSplit {
    pub base: string,
    pub glob: string,
    pub is_glob: bool,
}

fn findMatchingBracket(s: string, openIdx: usize) -> usize? {
    let i = openIdx + 1;
    const len = s.len;
    if (i >= len) {
        return null;
    }
    if (s[i] == '!' || s[i] == '^') {
        i += 1;
    }
    let contentCount: usize = 0;
    const v_bs: GpVec16 = @splat('\\' as u8);
    const v_rb: GpVec16 = @splat(']' as u8);
    while (i < len) {
        if (i + 16 <= len) {
            const v: GpVec16 = s[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == v_bs) && !@reduce(.Or, v == v_rb)) {
                i += 16;
                contentCount += 16;
                continue;
            }
        }
        if (s[i] == '\\' && i + 1 < len) {
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
    const len = s.len;
    let hasComma = false;
    let hasRange = false;
    const v_bs: GpVec16 = @splat('\\' as u8);
    const v_lb: GpVec16 = @splat('{' as u8);
    const v_rb: GpVec16 = @splat('}' as u8);
    const v_cm: GpVec16 = @splat(',' as u8);
    const v_dt: GpVec16 = @splat('.' as u8);
    while (i < len) {
        if (i + 16 <= len) {
            const v: GpVec16 = s[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == v_bs) &&
                !@reduce(.Or, v == v_lb) &&
                !@reduce(.Or, v == v_rb) &&
                !@reduce(.Or, v == v_cm) &&
                !@reduce(.Or, v == v_dt)) {
                i += 16;
                continue;
            }
        }
        if (s[i] == '\\' && i + 1 < len) {
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
            } else if (s[i] == '.' && i + 1 < len && s[i + 1] == '.') {
                hasRange = true;
            }
        }
        i += 1;
    }
    return null;
}

fn findFirstGlobPos(s: string) -> usize? {
    const extStart = findExtglobStart(s);
    const scan_limit: usize = if (extStart >= 0) { extStart as usize } else { s.len };
    let i: usize = 0;
    const v_bs: GpVec16 = @splat('\\' as u8);
    const v_st: GpVec16 = @splat('*' as u8);
    const v_qm: GpVec16 = @splat('?' as u8);
    const v_lb: GpVec16 = @splat('[' as u8);
    const v_lc: GpVec16 = @splat('{' as u8);
    while (i < s.len) {
        if (extStart >= 0 && i == scan_limit) {
            return i;
        }
        if (i + 16 <= scan_limit) {
            const v: GpVec16 = s[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == v_bs) &&
                !@reduce(.Or, v == v_st) &&
                !@reduce(.Or, v == v_qm) &&
                !@reduce(.Or, v == v_lb) &&
                !@reduce(.Or, v == v_lc)) {
                i += 16;
                continue;
            }
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

fn gpFindSpecialSimd(s: string, start: usize) -> usize {
    let i = start;
    const len = s.len;
    const v_st: GpVec16 = @splat('*' as u8);
    const v_qm: GpVec16 = @splat('?' as u8);
    const v_lb: GpVec16 = @splat('[' as u8);
    const v_rb: GpVec16 = @splat(']' as u8);
    const v_lc: GpVec16 = @splat('{' as u8);
    const v_rc: GpVec16 = @splat('}' as u8);
    const v_lp: GpVec16 = @splat('(' as u8);
    const v_rp: GpVec16 = @splat(')' as u8);
    const v_ex: GpVec16 = @splat('!' as u8);
    const v_bs: GpVec16 = @splat('\\' as u8);
    while (i + 16 <= len) {
        const v: GpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_st) || @reduce(.Or, v == v_qm) ||
            @reduce(.Or, v == v_lb) || @reduce(.Or, v == v_rb) ||
            @reduce(.Or, v == v_lc) || @reduce(.Or, v == v_rc) ||
            @reduce(.Or, v == v_lp) || @reduce(.Or, v == v_rp) ||
            @reduce(.Or, v == v_ex) || @reduce(.Or, v == v_bs)) {
            break;
        }
        i += 16;
    }
    while (i < len && GP_SPECIAL_LUT[s[i] as usize] == 0) {
        i += 1;
    }
    return i;
}

pub fn escapeGlob(raw: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len && pos < out.len) {
        const next_sp = gpFindSpecialSimd(raw, i);
        if (next_sp > i) {
            const avail = out.len - pos;
            const run = next_sp - i;
            const copy_len = if (run < avail) { run } else { avail };
            @memcpy(out[pos .. pos + copy_len], raw[i .. i + copy_len]);
            pos += copy_len;
            i += copy_len;
            if (pos >= out.len) {
                break;
            }
        }
        if (i >= raw.len) {
            break;
        }
        if (pos + 2 > out.len) {
            break;
        }
        out[pos] = '\\';
        out[pos + 1] = raw[i];
        pos += 2;
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
