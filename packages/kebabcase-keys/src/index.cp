// @cap/kebabcase-keys — Zero-heap object key & string kebab-case converter.
// Clean-room MIT port of npm `kebabcase-keys` v1, composing `@cap/decamelize-keys`, `@cap/map-obj` & `cap:ascii`.
// Accelerated with 16-lane @Vector(16, u8) SIMD fast-paths, comptime 256-entry LUTs, and @memcpy.

import { strEq } from "cap:ascii";
import { MapObjResult, MAP_OBJ_MAX_ENTRIES } from "../../map-obj/src/index.cp";
import { DecamelizeKeysResult, decamelizeKeys, DCK_MAX_ENTRIES, DCK_KEY_BUF_CAP } from "../../decamelize-keys/src/index.cp";

type KckVec16 = @Vector(16, u8);

const KCK_FLAG_UPPER: u8 = 1;
const KCK_FLAG_LOWER: u8 = 2;
const KCK_FLAG_DIGIT: u8 = 4;
const KCK_FLAG_SEP: u8 = 8;

fn kckBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = KCK_FLAG_DIGIT;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = KCK_FLAG_UPPER;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = KCK_FLAG_LOWER;
        c += 1;
    }
    // Separators: '-', '_', ' ', '\t', '\n', '\r', '.'
    lut[45] = KCK_FLAG_SEP;
    lut[95] = KCK_FLAG_SEP;
    lut[32] = KCK_FLAG_SEP;
    lut[9] = KCK_FLAG_SEP;
    lut[10] = KCK_FLAG_SEP;
    lut[13] = KCK_FLAG_SEP;
    lut[46] = KCK_FLAG_SEP;
    return lut;
}

fn kckBuildLowerLut(): [256]u8 {
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

const KCK_CHAR_LUT: [256]u8 = kckBuildCharLut();
const KCK_LOWER_LUT: [256]u8 = kckBuildLowerLut();

fn kckIsUpper(c: u8): bool {
    return (KCK_CHAR_LUT[c as usize] & KCK_FLAG_UPPER) != 0;
}

fn kckIsLower(c: u8): bool {
    return (KCK_CHAR_LUT[c as usize] & KCK_FLAG_LOWER) != 0;
}

fn kckIsDigit(c: u8): bool {
    return (KCK_CHAR_LUT[c as usize] & KCK_FLAG_DIGIT) != 0;
}

fn kckIsSep(c: u8): bool {
    return (KCK_CHAR_LUT[c as usize] & KCK_FLAG_SEP) != 0;
}

fn kckIsExcluded(k: string, exclude: []const string): bool {
    let i: usize = 0;
    while (i < exclude.len) {
        if (strEq(k, exclude[i])) { return true; }
        i = i + 1;
    }
    return false;
}

/// Converts any string to canonical `kebab-case` into `out`, returning bytes written.
/// Handles camelCase (`fooBar` -> `foo-bar`), PascalCase (`UserAgent` -> `user-agent`),
/// consecutive uppercase acronyms (`HTMLParser` -> `html-parser`, `XMLHttpRequest` -> `xml-http-request`),
/// underscores/spaces/dots (`foo_bar baz` -> `foo-bar-baz`), collapses repeated hyphens,
/// and trims leading/trailing hyphens (`--foo_bar--` -> `foo-bar`).
pub fn kebabcaseString(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) { return 0; }
    let n = input.len;
    let w: usize = 0;
    let i: usize = 0;
    let prev_hyphen = true; // suppresses leading hyphens
    let prev_was_lower_or_digit = false;
    let prev_was_upper = false;

    const v_zero: KckVec16 = @splat(0 as u8);
    const v_one: KckVec16 = @splat(1 as u8);
    const v_lo_a: KckVec16 = @splat(97 as u8);
    const v_lo_z: KckVec16 = @splat(122 as u8);
    const v_d0: KckVec16 = @splat(48 as u8);
    const v_d9: KckVec16 = @splat(57 as u8);

    while (i < n && w < out.len) {
        // 16-lane SIMD fast-path: copy pure lowercase/digit chunks verbatim via @memcpy
        if (i + 16 <= n && w + 16 <= out.len) {
            const v: KckVec16 = input[i .. i + 16][0..16].*;
            const ok = (@select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero))
                | (@select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero));
            if (@reduce(.And, ok != v_zero)) {
                @memcpy(out[w .. w + 16], input[i .. i + 16]);
                i += 16;
                w += 16;
                prev_hyphen = false;
                prev_was_upper = false;
                prev_was_lower_or_digit = true;
                continue;
            }
        }

        let c = input[i];
        let flags = KCK_CHAR_LUT[c as usize];
        if ((flags & KCK_FLAG_SEP) != 0) {
            if (!prev_hyphen) {
                out[w] = 45; // '-'
                w = w + 1;
                prev_hyphen = true;
            }
            prev_was_lower_or_digit = false;
            prev_was_upper = false;
            i = i + 1;
            continue;
        }

        let is_up = (flags & KCK_FLAG_UPPER) != 0;
        let is_lo_or_dg = (flags & (KCK_FLAG_LOWER | KCK_FLAG_DIGIT)) != 0;

        if (is_up) {
            let next_is_lo = (i + 1 < n) && kckIsLower(input[i + 1]);
            if (!prev_hyphen && (prev_was_lower_or_digit || (prev_was_upper && next_is_lo))) {
                if (w < out.len) {
                    out[w] = 45;
                    w = w + 1;
                }
            }
            if (w < out.len) {
                out[w] = KCK_LOWER_LUT[c as usize];
                w = w + 1;
            }
            prev_hyphen = false;
            prev_was_upper = true;
            prev_was_lower_or_digit = false;
        } else {
            out[w] = c;
            w = w + 1;
            prev_hyphen = false;
            prev_was_upper = false;
            prev_was_lower_or_digit = is_lo_or_dg;
        }
        i = i + 1;
    }

    // Trim trailing hyphen(s)
    while (w > 0 && out[w - 1] == 45) {
        w = w - 1;
    }
    return w;
}

/// Returns true if `s` is already canonical `kebab-case` (lowercase alphanumeric + single internal `-`).
pub fn isKebabCase(s: string): bool {
    if (s.len == 0) { return false; }
    if (s[0] == 45 || s[s.len - 1] == 45) { return false; }
    let prev_dash = false;
    let i: usize = 0;

    const v_zero: KckVec16 = @splat(0 as u8);
    const v_one: KckVec16 = @splat(1 as u8);
    const v_lo_a: KckVec16 = @splat(97 as u8);
    const v_lo_z: KckVec16 = @splat(122 as u8);
    const v_d0: KckVec16 = @splat(48 as u8);
    const v_d9: KckVec16 = @splat(57 as u8);

    while (i < s.len) {
        while (i + 16 <= s.len) {
            const v: KckVec16 = s[i .. i + 16][0..16].*;
            const ok = (@select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero))
                | (@select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero));
            if (@reduce(.And, ok != v_zero)) {
                prev_dash = false;
                i += 16;
                continue;
            }
            break;
        }
        if (i >= s.len) { break; }
        let c = s[i];
        if (c == 45) {
            if (prev_dash) { return false; }
            prev_dash = true;
        } else if ((KCK_CHAR_LUT[c as usize] & (KCK_FLAG_LOWER | KCK_FLAG_DIGIT)) != 0) {
            prev_dash = false;
        } else {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Converts all active keys in `src` (`MapObjResult`) to `kebab-case` (`"-"` separator)
/// by delegating to `@cap/decamelize-keys`, keeping keys in `exclude` unchanged.
pub fn kebabcaseKeys(src: MapObjResult, exclude: []string): DecamelizeKeysResult {
    return decamelizeKeys(src, "-", exclude);
}

/// Convenience helper converting all active keys in `src` to `kebab-case` with no exclusions.
pub fn kebabcaseKeysAll(src: MapObjResult): DecamelizeKeysResult {
    let empty_excl: [0]string = undefined;
    return decamelizeKeys(src, "-", empty_excl[0..0]);
}

/// Converts parallel `keys` and `vals` slices directly to a `DecamelizeKeysResult` using
/// full `toKebabCase` (acronym splitting, underscore/space normalization & hyphen trimming),
/// keeping any key listed in `exclude` verbatim.
pub fn kebabcaseEntries(
    keys: []const string,
    vals: []const string,
    exclude: []const string,
): DecamelizeKeysResult {
    let res = DecamelizeKeysResult.empty();
    let n = if (keys.len < vals.len) { keys.len } else { vals.len };
    let limit = if (n < DCK_MAX_ENTRIES) { n } else { DCK_MAX_ENTRIES };
    let i: usize = 0;
    while (i < limit) {
        let k = keys[i];
        let v = vals[i];
        if (kckIsExcluded(k, exclude)) {
            * = res.pushRaw(k, v);
        } else {
            let start = res.key_buf_len;
            if (start < DCK_KEY_BUF_CAP) {
                let written = kebabcaseString(k, res.key_buf[start..DCK_KEY_BUF_CAP]);
                res.key_offs[res.count] = start;
                res.key_lens[res.count] = written;
                res.values[res.count] = v;
                res.active[res.count] = true;
                res.key_buf_len = start + written;
                res.count = res.count + 1;
            }
        }
        i = i + 1;
    }
    return res;
}

/// Converts all active keys in `src` using full `toKebabCase` with `exclude` and `stop_paths`
/// (dot-separated keys matching `stop_paths` are kept verbatim; other dot paths convert each segment).
pub fn kebabcaseKeysDeep(
    src: MapObjResult,
    exclude: []const string,
    stop_paths: []const string,
): DecamelizeKeysResult {
    let res = DecamelizeKeysResult.empty();
    let limit = if (src.count < MAP_OBJ_MAX_ENTRIES) { src.count } else { MAP_OBJ_MAX_ENTRIES };
    let i: usize = 0;
    while (i < limit) {
        if (src.entries[i].active) {
            let k = src.entries[i].key;
            let v = src.entries[i].val;
            if (kckIsExcluded(k, exclude) || kckIsExcluded(k, stop_paths)) {
                * = res.pushRaw(k, v);
            } else {
                let start = res.key_buf_len;
                if (start < DCK_KEY_BUF_CAP) {
                    let written = kebabcasePathInto(k, res.key_buf[start..DCK_KEY_BUF_CAP]);
                    res.key_offs[res.count] = start;
                    res.key_lens[res.count] = written;
                    res.values[res.count] = v;
                    res.active[res.count] = true;
                    res.key_buf_len = start + written;
                    res.count = res.count + 1;
                }
            }
        }
        i = i + 1;
    }
    return res;
}

/// Converts a single key string to `kebab-case` into `out`, returning bytes written.
pub fn kebabcaseKeyInto(key: string, out: []u8): usize {
    return kebabcaseString(key, out);
}

/// Recursively converts each `.`-delimited segment of a nested key path (e.g.
/// `"userProfile.billingAddress.postalCode"`) to `kebab-case` into `out`.
pub fn kebabcasePathInto(dot_path: string, out: []u8): usize {
    let n = dot_path.len;
    if (n == 0 || out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let seg_start: usize = 0;
    let wpos: usize = 0;

    while (pos <= n) {
        let is_end = pos == n;
        let is_dot = !is_end && dot_path[pos] == 46;
        if (is_end || is_dot) {
            if (pos > seg_start && wpos < out.len) {
                let seg = dot_path[seg_start..pos];
                let written = kebabcaseKeyInto(seg, out[wpos..out.len]);
                wpos = wpos + written;
            }
            if (is_dot && wpos < out.len) {
                out[wpos] = 46;
                wpos = wpos + 1;
            }
            seg_start = pos + 1;
        }
        pos = pos + 1;
    }
    return wpos;
}

/// Batch-converts a slice of `MapObjResult` objects into `out`, returning count converted.
pub fn kebabcaseKeysBatch(
    items: []MapObjResult,
    exclude: []string,
    out: []DecamelizeKeysResult,
): usize {
    let limit = if (items.len < out.len) { items.len } else { out.len };
    let i: usize = 0;
    while (i < limit) {
        out[i] = decamelizeKeys(items[i], "-", exclude);
        i = i + 1;
    }
    return limit;
}
