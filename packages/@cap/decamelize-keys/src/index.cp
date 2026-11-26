// @cap/decamelize-keys — Zero-heap object key decamelizer.
// Clean-room MIT port of npm `decamelize-keys` v2, composing @cap/map-obj & @cap/decamelize.

import { MapObjResult, MAP_OBJ_MAX_ENTRIES } from "../../map-obj/src/index.cp";
import { decamelize } from "../../decamelize/src/index.cp";

pub const DCK_MAX_ENTRIES: usize = 32;
pub const DCK_KEY_BUF_CAP: usize = 1024;

/// Zero-heap 32-slot decamelized object result backed by a fixed 1024-byte key buffer pool.
/// Storing offsets into `key_buf` prevents dangling slices when returned by value.
pub struct DecamelizeKeysResult {
    pub key_buf: [1024]u8,
    pub key_buf_len: usize,
    pub key_offs: [32]usize,
    pub key_lens: [32]usize,
    pub values: [32]string,
    pub active: [32]bool,
    pub count: usize,

    pub static fn empty(): DecamelizeKeysResult {
        let kb: [1024]u8 = [0; 1024];
        let offs: [32]usize = [0; 32];
        let lens: [32]usize = [0; 32];
        let vals: [32]string = [""; 32];
        let act: [32]bool = [false; 32];
        return DecamelizeKeysResult {
            key_buf: kb,
            key_buf_len: 0,
            key_offs: offs,
            key_lens: lens,
            values: vals,
            active: act,
            count: 0,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn get(k: string): string? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.active[i]) {
                let off = _.key_offs[i];
                let klen = _.key_lens[i];
                if (klen == k.len) {
                    let matched = true;
                    let j: usize = 0;
                    while (j < klen) {
                        if (_.key_buf[off + j] != k[j]) {
                            matched = false;
                            break;
                        }
                        j = j + 1;
                    }
                    if (matched) {
                        return _.values[i];
                    }
                }
            }
            i = i + 1;
        }
        return null;
    }

    pub fn pushRaw(*_, k: string, v: string): bool {
        if (_.count >= DCK_MAX_ENTRIES) { return false; }
        if (_.key_buf_len + k.len > DCK_KEY_BUF_CAP) { return false; }
        let start = _.key_buf_len;
        let i: usize = 0;
        while (i < k.len) {
            _.key_buf[start + i] = k[i];
            i = i + 1;
        }
        _.key_offs[_.count] = start;
        _.key_lens[_.count] = k.len;
        _.values[_.count] = v;
        _.active[_.count] = true;
        _.key_buf_len = start + k.len;
        _.count = _.count + 1;
        return true;
    }

    pub fn pushDecamelized(*_, k: string, sep: string, v: string): bool {
        if (_.count >= DCK_MAX_ENTRIES) { return false; }
        let start = _.key_buf_len;
        if (start >= DCK_KEY_BUF_CAP) { return false; }
        let written = decamelize(k, sep, _.key_buf[start..DCK_KEY_BUF_CAP]);
        _.key_offs[_.count] = start;
        _.key_lens[_.count] = written;
        _.values[_.count] = v;
        _.active[_.count] = true;
        _.key_buf_len = start + written;
        _.count = _.count + 1;
        return true;
    }
}

fn dckStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn dckIsExcluded(k: string, exclude: []string): bool {
    let i: usize = 0;
    while (i < exclude.len) {
        if (dckStrEq(k, exclude[i])) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Converts all active keys in `src` (`MapObjResult`) using `@cap/decamelize` with `sep`
/// (defaults to `"_"` when `sep` is empty), keeping any key in `exclude` unchanged.
pub fn decamelizeKeys(src: MapObjResult, sep: string, exclude: []string): DecamelizeKeysResult {
    let effective_sep = if (sep.len == 0) { "_" } else { sep };
    let res = DecamelizeKeysResult.empty();
    let limit = if (src.count < MAP_OBJ_MAX_ENTRIES) { src.count } else { MAP_OBJ_MAX_ENTRIES };
    let i: usize = 0;
    while (i < limit) {
        if (src.entries[i].active) {
            let k = src.entries[i].key;
            let v = src.entries[i].val;
            if (dckIsExcluded(k, exclude)) {
                * = res.pushRaw(k, v);
            } else {
                * = res.pushDecamelized(k, effective_sep, v);
            }
        }
        i = i + 1;
    }
    return res;
}
