// @cap/regex-escape — Zero-heap regular expression literal & character-class escaper
// MIT License (clean-room CP implementation)
// Accelerated with comptime 256-entry bitmask LUT, 16-lane @Vector(16, u8) SIMD fast-paths, and @memcpy.

type ReVec16 = @Vector(16, u8);

const RE_FLAG_META: u8 = 1;
const RE_FLAG_ESCAPE: u8 = 2;
const RE_FLAG_CLASS: u8 = 4;

fn reBuildFlagsLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c == ('\\' as usize)
            || c == ('.' as usize)
            || c == ('+' as usize)
            || c == ('*' as usize)
            || c == ('?' as usize)
            || c == ('(' as usize)
            || c == (')' as usize)
            || c == ('|' as usize)
            || c == ('[' as usize)
            || c == (']' as usize)
            || c == ('{' as usize)
            || c == ('}' as usize)
            || c == ('^' as usize)
            || c == ('$' as usize)
            || c == ('-' as usize))
        {
            f = f | RE_FLAG_META | RE_FLAG_ESCAPE;
        }
        if (c == ('/' as usize)) {
            f = f | RE_FLAG_ESCAPE;
        }
        if (c == ('\\' as usize) || c == (']' as usize) || c == ('-' as usize) || c == ('^' as usize)) {
            f = f | RE_FLAG_CLASS;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const RE_FLAGS_LUT: [256]u8 = reBuildFlagsLut();

pub fn isMetaChar(c: u8) -> bool {
    return (RE_FLAGS_LUT[c as usize] & RE_FLAG_META) != 0;
}

fn isEscapeTarget(c: u8) -> bool {
    return (RE_FLAGS_LUT[c as usize] & RE_FLAG_ESCAPE) != 0;
}

fn isClassTarget(c: u8) -> bool {
    return (RE_FLAGS_LUT[c as usize] & RE_FLAG_CLASS) != 0;
}

// Fast 16-lane SIMD check: true if all 16 bytes are safe ASCII alphanumeric/underscore/space
// (none of them can be regex metacharacters or '/').
fn reChunkAllSafe(v: ReVec16) -> bool {
    const v_0: ReVec16 = @splat(48 as u8);
    const v_9: ReVec16 = @splat(57 as u8);
    const v_A: ReVec16 = @splat(65 as u8);
    const v_Z: ReVec16 = @splat(90 as u8);
    const v_a: ReVec16 = @splat(97 as u8);
    const v_z: ReVec16 = @splat(122 as u8);
    const v_und: ReVec16 = @splat(95 as u8);
    const v_sp: ReVec16 = @splat(32 as u8);
    const v_one: ReVec16 = @splat(1 as u8);
    const v_zero: ReVec16 = @splat(0 as u8);

    const is_dig = @select(u8, v >= v_0, v_one, v_zero) & @select(u8, v <= v_9, v_one, v_zero);
    const is_upp = @select(u8, v >= v_A, v_one, v_zero) & @select(u8, v <= v_Z, v_one, v_zero);
    const is_low = @select(u8, v >= v_a, v_one, v_zero) & @select(u8, v <= v_z, v_one, v_zero);
    const is_ok = is_dig | is_upp | is_low | @select(u8, v == v_und, v_one, v_zero) | @select(u8, v == v_sp, v_one, v_zero);
    return @reduce(.Min, is_ok) != 0;
}

pub fn needsEscape(input: string) -> bool {
    let i: usize = 0;
    while (i + 16 <= input.len) {
        const v: ReVec16 = input[i .. i + 16][0..16].*;
        if (reChunkAllSafe(v)) {
            i += 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) {
            if (isEscapeTarget(input[i + k])) {
                return true;
            }
            k += 1;
        }
        i += 16;
    }
    while (i < input.len) {
        if (isEscapeTarget(input[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn escapedLen(input: string) -> usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i + 16 <= input.len) {
        const v: ReVec16 = input[i .. i + 16][0..16].*;
        if (reChunkAllSafe(v)) {
            total += 16;
            i += 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) {
            total += if (isEscapeTarget(input[i + k])) (2 as usize) else (1 as usize);
            k += 1;
        }
        i += 16;
    }
    while (i < input.len) {
        total += if (isEscapeTarget(input[i])) (2 as usize) else (1 as usize);
        i += 1;
    }
    return total;
}

pub fn escapeRegex(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    const need: usize = escapedLen(input);
    if (need > out.len) {
        return 0;
    }
    if (need == input.len) {
        @memcpy(out[0..input.len], input);
        return input.len;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i + 16 <= input.len) {
        const v: ReVec16 = input[i .. i + 16][0..16].*;
        if (reChunkAllSafe(v)) {
            @memcpy(out[pos .. pos + 16], input[i .. i + 16]);
            pos += 16;
            i += 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) {
            const c: u8 = input[i + k];
            if (isEscapeTarget(c)) {
                out[pos] = '\\' as u8;
                out[pos + 1] = c;
                pos += 2;
            } else {
                out[pos] = c;
                pos += 1;
            }
            k += 1;
        }
        i += 16;
    }
    while (i < input.len) {
        const c: u8 = input[i];
        if (isEscapeTarget(c)) {
            out[pos] = '\\' as u8;
            out[pos + 1] = c;
            pos += 2;
        } else {
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

pub fn escapeCharClass(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    const v_bs: ReVec16 = @splat('\\' as u8);
    const v_rb: ReVec16 = @splat(']' as u8);
    const v_dash: ReVec16 = @splat('-' as u8);
    const v_caret: ReVec16 = @splat('^' as u8);

    let pos: usize = 0;
    let i: usize = 0;
    while (i + 16 <= input.len) {
        const v: ReVec16 = input[i .. i + 16][0..16].*;
        if (!@reduce(.Or, v == v_bs)
            && !@reduce(.Or, v == v_rb)
            && !@reduce(.Or, v == v_dash)
            && !@reduce(.Or, v == v_caret))
        {
            if (pos + 16 > out.len) {
                return 0;
            }
            @memcpy(out[pos .. pos + 16], input[i .. i + 16]);
            pos += 16;
            i += 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) {
            const c: u8 = input[i + k];
            if (isClassTarget(c)) {
                if (pos + 2 > out.len) {
                    return 0;
                }
                out[pos] = '\\' as u8;
                out[pos + 1] = c;
                pos += 2;
            } else {
                if (pos >= out.len) {
                    return 0;
                }
                out[pos] = c;
                pos += 1;
            }
            k += 1;
        }
        i += 16;
    }
    while (i < input.len) {
        const c: u8 = input[i];
        if (isClassTarget(c)) {
            if (pos + 2 > out.len) {
                return 0;
            }
            out[pos] = '\\' as u8;
            out[pos + 1] = c;
            pos += 2;
        } else {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

pub fn unescapeRegexLiteral(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    const v_bs: ReVec16 = @splat('\\' as u8);
    let pos: usize = 0;
    let i: usize = 0;
    while (i + 16 <= input.len) {
        const v: ReVec16 = input[i .. i + 16][0..16].*;
        if (!@reduce(.Or, v == v_bs)) {
            if (pos + 16 > out.len) {
                return 0;
            }
            @memcpy(out[pos .. pos + 16], input[i .. i + 16]);
            pos += 16;
            i += 16;
            continue;
        }
        break;
    }
    while (i < input.len) {
        if (input[i] == '\\' as u8 && i + 1 < input.len) {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = input[i + 1];
            pos += 1;
            i += 2;
        } else {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = input[i];
            pos += 1;
            i += 1;
        }
    }
    return pos;
}
