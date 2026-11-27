// @cap/kebabcase-keys — Zero-heap object key kebab-case converter.
// Clean-room MIT port of npm `kebabcase-keys` v1, composing @cap/decamelize-keys & @cap/map-obj.

import { MapObjResult } from "../../map-obj/src/index.cp";
import { DecamelizeKeysResult, decamelizeKeys } from "../../decamelize-keys/src/index.cp";

fn kckCopySlice(src_buf: [1024]u8, off: usize, len: usize, out: []u8, dst_off: usize): usize {
    let avail = if (dst_off < out.len) { out.len - dst_off } else { 0 };
    let n = if (len < avail) { len } else { avail };
    let i: usize = 0;
    while (i < n) {
        out[dst_off + i] = src_buf[off + i];
        i = i + 1;
    }
    return n;
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

/// Converts a single key string to `kebab-case` into `out`, returning bytes written.
pub fn kebabcaseKeyInto(key: string, out: []u8): usize {
    if (key.len == 0 || out.len == 0) {
        return 0;
    }
    let tmp = MapObjResult.empty();
    * = tmp.push(key, "");
    let empty_excl: [0]string = undefined;
    let res = decamelizeKeys(tmp, "-", empty_excl[0..0]);
    if (res.count == 0) {
        return 0;
    }
    return kckCopySlice(res.key_buf, res.key_offs[0], res.key_lens[0], out, 0);
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
