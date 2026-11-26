// @cap/shortid — URL-friendly non-sequential short unique ID generator & validator
// Clean-room MIT CP implementation of npm `shortid` (64-char URL-safe alphabet, cluster worker ID,
// monotonic counter per tick, deterministic seeding & format validation). Zero-heap.

pub const SHORTID_ALPHABET: string = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-";
pub const SHORTID_MIN_LEN: usize = 7;
pub const SHORTID_MAX_LEN: usize = 14;

fn sidU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn sidCharIndex(c: u8, alphabet: string): i32 {
    let i: usize = 0;
    while (i < alphabet.len) {
        if (alphabet[i] == c) { return i as i32; }
        i = i + 1;
    }
    return -1;
}

/// Deterministic, zero-heap ShortId generator with cluster worker ID (`0..15`),
/// format version (`0..15`), and per-tick collision-free sequence counter.
pub struct ShortIdGenerator {
    alphabet: [64]u8,
    worker_id: u8,
    fmt_version: u8,
    last_tick: u64,
    counter: u32,
    rng_state: u64,

    pub static fn new(seed: u64, worker_id: u8): ShortIdGenerator {
        let g = ShortIdGenerator {
            alphabet: undefined,
            worker_id: worker_id & 15,
            fmt_version: 6,
            last_tick: 0,
            counter: 0,
            rng_state: if (seed == 0) { sidU64(0x9E3779B9, 0x7F4A7C15) } else { seed },
        };
        let i: usize = 0;
        while (i < 64) {
            g.alphabet[i] = SHORTID_ALPHABET[i];
            i = i + 1;
        }
        // Fisher-Yates shuffle alphabet deterministically from seed
        let k: usize = 63;
        while (k > 0) {
            let r = g.nextRand();
            let j = (r as usize) % (k + 1);
            let tmp = g.alphabet[k];
            g.alphabet[k] = g.alphabet[j];
            g.alphabet[j] = tmp;
            k = k - 1;
        }
        return g;
    }

    pub fn workerId(): u8 {
        return _.worker_id;
    }

    pub fn version(): u8 {
        return _.fmt_version;
    }

    fn nextRand(*_): u64 {
        // SplitMix64 step
        _.rng_state = _.rng_state +% sidU64(0x9E3779B9, 0x7F4A7C15);
        let z = _.rng_state;
        z = (z ^ (z >> 30)) *% sidU64(0xBF58476D, 0x1CE4E5B9);
        z = (z ^ (z >> 27)) *% sidU64(0x94D049BB, 0x133111EB);
        return z ^ (z >> 31);
    }

    fn encodeValue(*_, val: u64, out: []u8, pos: usize): usize {
        let p = pos;
        let v = val;
        let done = false;
        while (!done && p < out.len) {
            let low4 = (v & 15) as usize;
            let rand2 = ((_.nextRand() & 3) as usize) << 4;
            let idx = (rand2 | low4) & 63;
            out[p] = _.alphabet[idx];
            p = p + 1;
            v = v >> 4;
            if (v == 0) { done = true; }
        }
        return p;
    }

    /// Generates a URL-safe non-sequential short ID (7..14 chars) for the given timestamp `tick_ms`.
    /// Writes into `out` and returns written byte count.
    pub fn generate(*_, tick_ms: u64, out: []u8): usize {
        let sec = tick_ms / 1000;
        if (sec == _.last_tick) {
            _.counter = _.counter + 1;
        } else {
            _.counter = 0;
            _.last_tick = sec;
        }

        let pos: usize = 0;
        pos = _.encodeValue(_.fmt_version as u64, out, pos);
        pos = _.encodeValue(_.worker_id as u64, out, pos);
        if (_.counter > 0) {
            pos = _.encodeValue(_.counter as u64, out, pos);
        }
        // Mix entropy + timestamp delta for 7..11 char non-sequential output
        let payload = (sec ^ 0x5A5A5A5A) | ((_.nextRand() & 0xFFFF) << 24);
        pos = _.encodeValue(payload, out, pos);
        return pos;
    }

    /// Validates that `id` only contains characters from this generator's alphabet and has valid length.
    pub fn isValid(id: string): bool {
        if (id.len < SHORTID_MIN_LEN || id.len > SHORTID_MAX_LEN) { return false; }
        let i: usize = 0;
        while (i < id.len) {
            if (sidCharIndex(id[i], _.alphabet[0..64]) < 0) { return false; }
            i = i + 1;
        }
        return true;
    }

    /// Decodes the embedded format version (`0..15`) from the first character of `id`, or `-1` if invalid.
    pub fn decodeVersion(id: string): i32 {
        if (id.len < 2) { return -1; }
        let idx = sidCharIndex(id[0], _.alphabet[0..64]);
        if (idx < 0) { return -1; }
        return idx & 15;
    }

    /// Decodes the embedded cluster worker ID (`0..15`) from the second character of `id`, or `-1` if invalid.
    pub fn decodeWorker(id: string): i32 {
        if (id.len < 2) { return -1; }
        let idx = sidCharIndex(id[1], _.alphabet[0..64]);
        if (idx < 0) { return -1; }
        return idx & 15;
    }
}

/// Validates whether `id` is a syntactically valid URL-safe ShortId (`7..14` chars in `[0-9a-zA-Z_-]`).
pub fn isValidShortId(id: string): bool {
    if (id.len < SHORTID_MIN_LEN || id.len > SHORTID_MAX_LEN) { return false; }
    let i: usize = 0;
    while (i < id.len) {
        if (sidCharIndex(id[i], SHORTID_ALPHABET) < 0) { return false; }
        i = i + 1;
    }
    return true;
}
