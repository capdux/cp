// @cap/cuid2 — Zero-heap collision-resistant CUID2 ID generator & validator.
// Clean-room MIT port of `@paralleldrive/cuid2`; reuses `@cap/fnv-hash` for mixing.

import { fnv1a64, fnv1a64Update } from "../../fnv-hash/src/index.cp";

pub const CUID2_DEFAULT_LEN: usize = 24;
pub const CUID2_MIN_LEN: usize = 2;
pub const CUID2_MAX_LEN: usize = 32;

fn c2Mix(x: u64): u64 {
    let s: u64 = if (x == 0) { 0x517cc1b7 } else { x };
    let z = s ^ (s >> 12);
    let z1 = z ^ (z << 25);
    return z1 ^ (z1 >> 27);
}

fn c2Base36Char(v: u64): u8 {
    let alphabet: string = "0123456789abcdefghijklmnopqrstuvwxyz";
    return alphabet[(v % 36) as usize];
}

fn c2LetterChar(v: u64): u8 {
    let letters: string = "abcdefghijklmnopqrstuvwxyz";
    return letters[(v % 26) as usize];
}

/// Validates whether `id` is a syntactically valid CUID2 (`[a-z][0-9a-z]{min-1..max-1}`).
pub fn isCuid2(id: string): bool {
    return isCuid2WithBounds(id, CUID2_MIN_LEN, CUID2_MAX_LEN);
}

/// Validates CUID2 with custom `[min_len, max_len]` bounds.
pub fn isCuid2WithBounds(id: string, min_len: usize, max_len: usize): bool {
    if (id.len < min_len || id.len > max_len) { return false; }
    // First character must be lowercase ASCII letter 'a'..'z' (CSS/HTML ID safe)
    if (id[0] < 97 || id[0] > 122) { return false; }
    let i: usize = 1;
    while (i < id.len) {
        let c = id[i];
        let ok = (c >= 97 && c <= 122) || (c >= 48 && c <= 57);
        if (!ok) { return false; }
        i = i + 1;
    }
    return true;
}

/// Stateful zero-heap CUID2 generator with monotonic counter and host fingerprint.
pub struct Cuid2Generator {
    pub state: u64,
    pub counter: u64,
    pub fingerprint_hash: u64,
    pub id_len: usize,

    pub static fn new(seed: u64, fingerprint: string, id_len: usize): Cuid2Generator {
        let clamped = if (id_len < CUID2_MIN_LEN) { CUID2_MIN_LEN }
            else if (id_len > CUID2_MAX_LEN) { CUID2_MAX_LEN }
            else { id_len };
        let fp_h = fnv1a64(fingerprint);
        let init_state = c2Mix(seed ^ fp_h ^ 0x1337c0de);
        return Cuid2Generator {
            state: init_state,
            counter: 0,
            fingerprint_hash: fp_h,
            id_len: clamped,
        };
    }

    pub fn length(): usize { return _.id_len; }

    /// Generates the next CUID2 string for `timestamp_ms` into `out`, returning bytes written.
    pub fn next(*_, timestamp_ms: u64, out: []u8): usize {
        if (out.len < _.id_len) { return 0; }
        _.counter = _.counter + 1;
        let ts_bytes: [16]u8 = undefined;
        let k: usize = 0;
        while (k < 8) {
            let sh: u6 = (k * 8) as u6;
            ts_bytes[k] = ((timestamp_ms >> sh) & 0xff) as u8;
            ts_bytes[k + 8] = ((_.counter >> sh) & 0xff) as u8;
            k = k + 1;
        }
        let h = fnv1a64Update(_.fingerprint_hash ^ _.state, ts_bytes[0..16]);
        _.state = c2Mix(_.state ^ h ^ _.counter);

        // First char: lowercase letter
        out[0] = c2LetterChar(h);
        let cur = c2Mix(h ^ _.state);
        let i: usize = 1;
        while (i < _.id_len) {
            if ((i & 7) == 0) {
                cur = c2Mix(cur ^ (i as u64) ^ _.state);
            }
            out[i] = c2Base36Char(cur);
            cur = c2Mix(cur + (i as u64));
            i = i + 1;
        }
        return _.id_len;
    }
}
