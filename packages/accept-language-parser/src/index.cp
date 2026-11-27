// @cap/accept-lang — Zero-heap RFC 9110 / BCP-47 Accept-Language parser & negotiator
// MIT License (clean-room CP implementation)
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning and comptime 256-entry tag-char LUT.

import { isDigit, eqIgnoreCase } from "cap:ascii";

type AlVec16 = @Vector(16, u8);

fn alBuildTagCharLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = 1;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = 1;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = 1;
        c += 1;
    }
    lut['-' as usize] = 1;
    lut['_' as usize] = 1;
    return lut;
}

const AL_TAG_CHAR_LUT: [256]u8 = alBuildTagCharLut();

fn alFindByte(s: string, needle: u8, start: usize, end: usize) -> usize {
    let i: usize = start;
    const v_n: AlVec16 = @splat(needle);
    while (i + 16 <= end) {
        const v: AlVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_n)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == needle) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < end) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return end;
}

fn alFindSep(tag: string) -> usize {
    let i: usize = 0;
    const len = tag.len;
    const v_dash: AlVec16 = @splat('-' as u8);
    const v_under: AlVec16 = @splat('_' as u8);
    while (i + 16 <= len) {
        const v: AlVec16 = tag[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_dash) || @reduce(.Or, v == v_under)) {
            let k: usize = 0;
            while (k < 16) {
                const c = tag[i + k];
                if (c == '-' as u8 || c == '_' as u8) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = tag[i];
        if (c == '-' as u8 || c == '_' as u8) {
            return i;
        }
        i += 1;
    }
    return len;
}

pub struct LangRange {
    pub tag: string,
    pub primary: string,
    pub region: string,
    pub quality: u16,
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn isValidTag(tag: string) -> bool {
    if (tag.len == 0) {
        return false;
    }
    if (tag.len == 1 && tag[0] == '*' as u8) {
        return true;
    }
    let i: usize = 0;
    while (i < tag.len) {
        if (AL_TAG_CHAR_LUT[tag[i] as usize] == 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn parseQuality(s: string) -> i32 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    const len: usize = end - start;
    if (len == 0 || len > 5) {
        return -1;
    }

    const c0: u8 = s[start];
    if (c0 == '1' as u8) {
        if (len == 1) {
            return 1000;
        }
        if (s[start + 1] != '.' as u8) {
            return -1;
        }
        let i: usize = start + 2;
        while (i < end) {
            if (s[i] != '0' as u8) {
                return -1;
            }
            i += 1;
        }
        return 1000;
    }

    if (c0 == '0' as u8) {
        if (len == 1) {
            return 0;
        }
        if (s[start + 1] != '.' as u8) {
            return -1;
        }
        let val: u32 = 0;
        let mult: u32 = 100;
        let i: usize = start + 2;
        while (i < end) {
            const d: u8 = s[i];
            if (!isDigit(d)) {
                return -1;
            }
            val += ((d - ('0' as u8)) as u32) * mult;
            mult = mult / 10;
            i += 1;
        }
        return val as i32;
    }

    return -1;
}

pub fn parseAcceptLanguage(header: string, out: []LangRange) -> usize {
    let count: usize = 0;
    let pos: usize = 0;

    while (pos < header.len && count < out.len) {
        const seg_end: usize = alFindByte(header, ',' as u8, pos, header.len);

        // Split tag from optional ";q=..." parameters
        const semi: usize = alFindByte(header, ';' as u8, pos, seg_end);

        let t_s: usize = pos;
        let t_e: usize = semi;
        while (t_s < t_e && isWs(header[t_s])) {
            t_s += 1;
        }
        while (t_e > t_s && isWs(header[t_e - 1])) {
            t_e -= 1;
        }

        const tag: string = header[t_s..t_e];
        let valid_entry: bool = isValidTag(tag);
        let q_val: u16 = 1000;

        // Parse optional parameters after ';'
        let p_pos: usize = semi;
        while (valid_entry && p_pos < seg_end) {
            p_pos += 1; // skip ';'
            const p_end: usize = alFindByte(header, ';' as u8, p_pos, seg_end);
            const eq: usize = alFindByte(header, '=' as u8, p_pos, p_end);
            if (eq < p_end) {
                let k_s: usize = p_pos;
                let k_e: usize = eq;
                while (k_s < k_e && isWs(header[k_s])) {
                    k_s += 1;
                }
                while (k_e > k_s && isWs(header[k_e - 1])) {
                    k_e -= 1;
                }
                if (eqIgnoreCase(header[k_s..k_e], "q")) {
                    const pq: i32 = parseQuality(header[(eq + 1)..p_end]);
                    if (pq < 0) {
                        valid_entry = false;
                    } else {
                        q_val = pq as u16;
                    }
                }
            }
            p_pos = p_end;
        }

        if (valid_entry) {
            // Split primary & region subtags
            const first_sep: usize = alFindSep(tag);
            const primary: string = tag[0..first_sep];
            let region: string = "";
            if (first_sep < tag.len) {
                let last_sep: usize = tag.len;
                let k: usize = tag.len;
                while (k > 0) {
                    k -= 1;
                    if (tag[k] == '-' as u8 || tag[k] == '_' as u8) {
                        last_sep = k;
                        break;
                    }
                }
                if (last_sep + 1 < tag.len) {
                    region = tag[(last_sep + 1)..tag.len];
                }
            }

            out[count] = LangRange {
                tag: tag,
                primary: primary,
                region: region,
                quality: q_val,
            };
            count += 1;
        }

        pos = seg_end;
        if (pos < header.len && header[pos] == ',' as u8) {
            pos += 1;
        }
    }

    // Stable insertion sort by quality descending
    let i: usize = 1;
    while (i < count) {
        const key: LangRange = out[i];
        let j: usize = i;
        while (j > 0 && out[j - 1].quality < key.quality) {
            out[j] = out[j - 1];
            j -= 1;
        }
        out[j] = key;
        i += 1;
    }

    return count;
}

pub fn matchesTag(range_tag: string, supported_tag: string) -> bool {
    if (range_tag.len == 0 || supported_tag.len == 0) {
        return false;
    }
    if (range_tag.len == 1 && range_tag[0] == '*' as u8) {
        return true;
    }
    if (eqIgnoreCase(range_tag, supported_tag)) {
        return true;
    }
    if (supported_tag.len > range_tag.len && eqIgnoreCase(range_tag, supported_tag[0..range_tag.len])) {
        const next_c: u8 = supported_tag[range_tag.len];
        return next_c == '-' as u8 || next_c == '_' as u8;
    }
    return false;
}

fn isExcluded(ranges: []const LangRange, count: usize, candidate: string) -> bool {
    let i: usize = 0;
    while (i < count) {
        if (ranges[i].quality == 0 && matchesTag(ranges[i].tag, candidate)) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn pickLanguage(header: string, supported: []const string) -> string {
    if (header.len == 0 || supported.len == 0) {
        return "";
    }
    let ranges: [32]LangRange = undefined;
    const n: usize = parseAcceptLanguage(header, ranges[0..32]);
    if (n == 0) {
        return "";
    }

    let i: usize = 0;
    while (i < n) {
        const r: LangRange = ranges[i];
        if (r.quality == 0) {
            i += 1;
            continue;
        }
        // Pass 1: exact match preferred
        let j: usize = 0;
        while (j < supported.len) {
            if (!isExcluded(ranges[0..n], n, supported[j]) && eqIgnoreCase(r.tag, supported[j])) {
                return supported[j];
            }
            j += 1;
        }
        // Pass 2: prefix or wildcard match
        j = 0;
        while (j < supported.len) {
            if (!isExcluded(ranges[0..n], n, supported[j]) && matchesTag(r.tag, supported[j])) {
                return supported[j];
            }
            j += 1;
        }
        i += 1;
    }
    return "";
}
