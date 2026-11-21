// Source: Absorbed from /data/dev/bun/src/js_parser/ast/CharFreq.cp
//! Identifier character frequency analyzer & frequency-optimal short-name generator
//! (used by JS/TS minifiers to assign the shortest/most compressible variable names).

export const CHAR_FREQ_COUNT: usize = 64;
export const HEAD_CHAR_COUNT: usize = 54; // a-z, A-Z, _, $
export const TAIL_CHAR_COUNT: usize = 64; // a-z, A-Z, 0-9, _, $

const DEFAULT_TAIL: [64]u8 = [
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '0','1','2','3','4','5','6','7','8','9','_','$',
];

const SCAN_BIG_THRESHOLD: usize = 32;

export struct CharAndCount {
    char: u8 = 0,
    count: i32 = 0,
    index: usize = 0,

    pub static fn comesBefore(a: CharAndCount, b: CharAndCount): bool {
        if (a.count != b.count) return a.count > b.count;
        if (a.index != b.index) return a.index < b.index;
        return a.char < b.char;
    }
}

/// Frequency-ordered base-54/base-64 identifier generator (zero heap allocation).
export struct NameMinifier {
    head: [54]u8,
    tail: [64]u8,

    pub static fn default(): NameMinifier {
        let m = NameMinifier{
            head: [0; 54],
            tail: DEFAULT_TAIL,
        };
        let hi: usize = 0;
        let i: usize = 0;
        while (i < TAIL_CHAR_COUNT) : (i += 1) {
            const c = DEFAULT_TAIL[i];
            if (c < '0' || c > '9') {
                m.head[hi] = c;
                hi += 1;
            }
        }
        return m;
    }

    /// Generate the `index`-th shortest valid identifier into `out` and return the written slice.
    pub fn numberToName(index: usize, out: []u8): []const u8 {
        if (out.len == 0) return out[0..0];
        let n = index;
        let len: usize = 0;
        out[len] = _.head[n % HEAD_CHAR_COUNT];
        len += 1;
        n = n / HEAD_CHAR_COUNT;
        while (n > 0 && len < out.len) {
            n -= 1;
            out[len] = _.tail[n % TAIL_CHAR_COUNT];
            len += 1;
            n = n / TAIL_CHAR_COUNT;
        }
        return out[0..len];
    }
}

/// 64-bucket ASCII identifier character frequency counter with SIMD-friendly merge.
export struct CharFreq {
    freqs: [64]i32 = [0; 64],

    pub static fn init(): CharFreq {
        return CharFreq{ freqs: [0; 64] };
    }

    pub fn scan(*_, text: []const u8, delta: i32) {
        if (delta == 0 || text.len == 0) return;
        if (text.len < SCAN_BIG_THRESHOLD) {
            let i: usize = 0;
            while (i < text.len) : (i += 1) {
                const c = text[i];
                if (c >= 'a' && c <= 'z') {
                    _.freqs[@as(usize, c - 'a')] += delta;
                } else if (c >= 'A' && c <= 'Z') {
                    _.freqs[@as(usize, c - 'A') + 26] += delta;
                } else if (c >= '0' && c <= '9') {
                    _.freqs[@as(usize, c - '0') + 52] += delta;
                } else if (c == '_') {
                    _.freqs[62] += delta;
                } else if (c == '$') {
                    _.freqs[63] += delta;
                }
            }
            return;
        }

        let deltas: [256]i32 = [0; 256];
        let i: usize = 0;
        while (i < text.len) : (i += 1) {
            deltas[@as(usize, text[i])] += delta;
        }
        let k: usize = 0;
        while (k < 26) : (k += 1) {
            _.freqs[k] += deltas['a' + k];
            _.freqs[26 + k] += deltas['A' + k];
        }
        let d: usize = 0;
        while (d < 10) : (d += 1) {
            _.freqs[52 + d] += deltas['0' + d];
        }
        _.freqs[62] += deltas['_'];
        _.freqs[63] += deltas['$'];
    }

    /// Merge another `CharFreq` into this one.
    pub fn include(*_, other: CharFreq) {
        let i: usize = 0;
        while (i < CHAR_FREQ_COUNT) : (i += 1) {
            _.freqs[i] += other.freqs[i];
        }
    }

    /// Query the recorded frequency of a specific identifier byte (returns 0 for non-identifier bytes).
    pub fn countOf(c: u8): i32 {
        if (c >= 'a' && c <= 'z') return _.freqs[@as(usize, c - 'a')];
        if (c >= 'A' && c <= 'Z') return _.freqs[@as(usize, c - 'A') + 26];
        if (c >= '0' && c <= '9') return _.freqs[@as(usize, c - '0') + 52];
        if (c == '_') return _.freqs[62];
        if (c == '$') return _.freqs[63];
        return 0;
    }

    /// Compile the frequency histogram into a sorted `NameMinifier` (most frequent chars first).
    pub fn compile(): NameMinifier {
        let arr: [64]CharAndCount = undefined;
        let i: usize = 0;
        while (i < CHAR_FREQ_COUNT) : (i += 1) {
            arr[i] = CharAndCount{
                char: DEFAULT_TAIL[i],
                count: _.freqs[i],
                index: i,
            };
        }

        // Insertion sort on 64 elements: fast, deterministic, zero imports.
        let a: usize = 1;
        while (a < CHAR_FREQ_COUNT) : (a += 1) {
            const key = arr[a];
            let b: usize = a;
            while (b > 0 && CharAndCount.comesBefore(key, arr[b - 1])) : (b -= 1) {
                arr[b] = arr[b - 1];
            }
            arr[b] = key;
        }

        let m = NameMinifier{
            head: [0; 54],
            tail: [0; 64],
        };
        let hi: usize = 0;
        let ti: usize = 0;
        while (ti < CHAR_FREQ_COUNT) : (ti += 1) {
            const ch = arr[ti].char;
            m.tail[ti] = ch;
            if (ch < '0' || ch > '9') {
                m.head[hi] = ch;
                hi += 1;
            }
        }
        return m;
    }
}
