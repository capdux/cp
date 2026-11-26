// @cap/sqids — Zero-allocation YouTube-style Reversible Short ID Generator
// Clean-room pure-CP implementation of the Sqids specification (MIT License).
// Modeled after npm `sqids`/`hashids` and Rust `sqids`/`harsh`.

const std = @import("std");

pub const DEFAULT_ALPHABET: string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
pub const MIN_ALPHABET_LEN: usize = 3;
pub const MAX_ALPHABET_LEN: usize = 128;
const MAX_ENCODE_BUF: usize = 512;

/// Validation error for `SqidsOptions`.
pub enum SqidsError: u8 {
    None = 0,
    AlphabetTooShort = 1,
    AlphabetTooLong = 2,
    AlphabetNonAscii = 3,
    AlphabetDuplicateChar = 4,
}

/// Configuration options for `Sqids`.
pub struct SqidsOptions {
    alphabet: string = DEFAULT_ALPHABET,
    min_length: u8 = 0,
}

// Curated lowercase blocklist for profanity avoidance (Sqids matching rules).
const BLOCKLIST: [52]string = [
    "anal",
    "anus",
    "arse",
    "ass",
    "bastard",
    "bitch",
    "b1tch",
    "boob",
    "cock",
    "c0ck",
    "crap",
    "cunt",
    "damn",
    "dick",
    "d1ck",
    "dyke",
    "fag",
    "fuck",
    "fuk",
    "f4ck",
    "fck",
    "hell",
    "homo",
    "jerk",
    "jizz",
    "kike",
    "milf",
    "nazi",
    "nigg",
    "n1gg",
    "penis",
    "piss",
    "p1ss",
    "poop",
    "porn",
    "p0rn",
    "prick",
    "pussy",
    "rape",
    "scum",
    "sex",
    "shit",
    "sh1t",
    "slut",
    "spic",
    "tit",
    "tits",
    "turd",
    "twat",
    "wank",
    "whore",
    "wtf",
];

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn hasDigit(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] >= '0' && s[i] <= '9') return true;
    }
    return false;
}

fn containsSlice(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) : (i += 1) {
        if (std.mem.eql(u8, haystack[i .. i + needle.len], needle)) {
            return true;
        }
    }
    return false;
}

fn startsWithSlice(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) return false;
    return std.mem.eql(u8, haystack[0..prefix.len], prefix);
}

fn endsWithSlice(haystack: []const u8, suffix: []const u8): bool {
    if (suffix.len > haystack.len) return false;
    return std.mem.eql(u8, haystack[haystack.len - suffix.len .. haystack.len], suffix);
}

fn isBlockedId(id: []const u8): bool {
    if (id.len == 0 || id.len > MAX_ENCODE_BUF) return false;
    let lower_buf: [MAX_ENCODE_BUF]u8 = undefined;
    let i: usize = 0;
    while (i < id.len) : (i += 1) {
        lower_buf[i] = toLowerAscii(id[i]);
    }
    const lower = lower_buf[0..id.len];

    let b: usize = 0;
    while (b < BLOCKLIST.len) : (b += 1) {
        const word = BLOCKLIST[b];
        if (word.len <= lower.len) {
            if (lower.len <= 3 || word.len <= 3) {
                if (std.mem.eql(u8, lower, word)) return true;
            } else if (hasDigit(word)) {
                if (startsWithSlice(lower, word) || endsWithSlice(lower, word)) return true;
            } else {
                if (containsSlice(lower, word)) return true;
            }
        }
    }
    return false;
}

/// Deterministic Sqids Fisher-Yates-style alphabet shuffle.
fn shuffleAlphabet(buf: []u8) {
    const len = buf.len;
    if (len <= 1) return;
    let i: usize = 0;
    let j: usize = len - 1;
    while (j > 0) {
        const r: usize = (i * j + @as(usize, buf[i]) + @as(usize, buf[j])) % len;
        const tmp = buf[i];
        buf[i] = buf[r];
        buf[r] = tmp;
        i += 1;
        j -= 1;
    }
}

fn reverseBytes(buf: []u8) {
    if (buf.len <= 1) return;
    let l: usize = 0;
    let r: usize = buf.len - 1;
    while (l < r) {
        const tmp = buf[l];
        buf[l] = buf[r];
        buf[r] = tmp;
        l += 1;
        r -= 1;
    }
}

fn indexOfByte(haystack: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < haystack.len) : (i += 1) {
        if (haystack[i] == needle) return i;
    }
    return null;
}

/// Encodes a single `u64` into base-`alphabet.len` digits and appends to `dst`.
/// Returns the new length of `dst`, or `null` on buffer overflow.
fn appendToId(num: u64, alphabet: []const u8, dst: []u8, pos: usize): usize? {
    let rev: [64]u8 = undefined;
    let rev_len: usize = 0;
    const base: u64 = @intCast(alphabet.len);
    let v: u64 = num;

    while (true) {
        const rem: usize = @intCast(v % base);
        rev[rev_len] = alphabet[rem];
        rev_len += 1;
        v /= base;
        if (v == 0) break;
    }

    if (pos + rev_len > dst.len) return null;
    let k: usize = 0;
    while (k < rev_len) : (k += 1) {
        dst[pos + k] = rev[rev_len - 1 - k];
    }
    return pos + rev_len;
}

/// Decodes a base-`alphabet.len` chunk into a `u64`, returning `null` on invalid char or overflow.
fn chunkToNumber(chunk: []const u8, alphabet: []const u8): u64? {
    if (chunk.len == 0) return null;
    const base: u64 = @intCast(alphabet.len);
    const max_u64: u64 = std.math.maxInt(u64);
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < chunk.len) : (i += 1) {
        if (digit_idx := indexOfByte(alphabet, chunk[i])) {
            const digit: u64 = @intCast(digit_idx);
            if (acc > (max_u64 - digit) / base) return null;
            acc = acc * base + digit;
        } else {
            return null;
        }
    }
    return acc;
}

/// Stack-allocated, zero-heap Sqids encoder/decoder.
pub struct Sqids {
    alphabet: [MAX_ALPHABET_LEN]u8 = undefined,
    alphabet_len: usize = 0,
    min_length: u8 = 0,
    valid: bool = false,
    err: SqidsError = SqidsError.None,

    /// Initializes `Sqids` with the default 62-char alphanumeric alphabet and `min_length = 0`.
    pub static fn initDefault(): Sqids {
        return Sqids.init(SqidsOptions{});
    }

    /// Initializes `Sqids` with custom options, validating ASCII uniqueness and shuffling the alphabet.
    pub static fn init(opts: SqidsOptions): Sqids {
        let s = Sqids{
            alphabet: undefined,
            alphabet_len: 0,
            min_length: opts.min_length,
            valid: false,
            err: SqidsError.None,
        };

        const raw = opts.alphabet;
        if (raw.len < MIN_ALPHABET_LEN) {
            s.err = SqidsError.AlphabetTooShort;
            return s;
        }
        if (raw.len > MAX_ALPHABET_LEN) {
            s.err = SqidsError.AlphabetTooLong;
            return s;
        }

        let seen: [128]bool = undefined;
        let k: usize = 0;
        while (k < 128) : (k += 1) {
            seen[k] = false;
        }

        let i: usize = 0;
        while (i < raw.len) : (i += 1) {
            const c = raw[i];
            if (c >= 128) {
                s.err = SqidsError.AlphabetNonAscii;
                return s;
            }
            if (seen[c]) {
                s.err = SqidsError.AlphabetDuplicateChar;
                return s;
            }
            seen[c] = true;
            s.alphabet[i] = c;
        }

        s.alphabet_len = raw.len;
        shuffleAlphabet(s.alphabet[0..s.alphabet_len]);
        s.valid = true;
        return s;
    }

    /// Returns `true` if this `Sqids` instance was initialized with a valid alphabet.
    pub fn isValid(): bool {
        return _.valid;
    }

    /// Encodes a slice of `u64` numbers into caller-provided `out` buffer (zero heap allocations).
    /// Returns a slice of `out`, or `""` if invalid, empty input, or `out` is too small.
    pub fn encode(numbers: []const u64, out: []u8): string {
        if (!_.valid || numbers.len == 0) return "";
        const alen = _.alphabet_len;

        let base_offset: usize = numbers.len;
        let ni: usize = 0;
        while (ni < numbers.len) : (ni += 1) {
            const idx: usize = @intCast(numbers[ni] % @as(u64, alen));
            base_offset += @as(usize, _.alphabet[idx]) + ni;
        }
        base_offset %= alen;

        let work: [MAX_ALPHABET_LEN]u8 = undefined;
        let tmp: [MAX_ENCODE_BUF]u8 = undefined;
        const min_len: usize = @as(usize, _.min_length);

        let increment: usize = 0;
        while (increment <= alen) : (increment += 1) {
            const offset = (base_offset + increment) % alen;

            // Rotate alphabet by `offset`
            let w: usize = 0;
            while (w < alen) : (w += 1) {
                work[w] = _.alphabet[(offset + w) % alen];
            }

            const prefix = work[0];
            reverseBytes(work[0..alen]);

            let ret_len: usize = 0;
            tmp[ret_len] = prefix;
            ret_len += 1;

            let overflowed = false;
            let i: usize = 0;
            while (i < numbers.len) : (i += 1) {
                const separator = work[0];
                if (next_len := appendToId(numbers[i], work[1..alen], tmp[0..], ret_len)) {
                    ret_len = next_len;
                } else {
                    overflowed = true;
                    break;
                }

                if (i + 1 < numbers.len) {
                    if (ret_len >= MAX_ENCODE_BUF) {
                        overflowed = true;
                        break;
                    }
                    tmp[ret_len] = separator;
                    ret_len += 1;
                    shuffleAlphabet(work[0..alen]);
                }
            }
            if (overflowed) return "";

            // Pad up to `min_length` using trailing separator + shuffled alphabet chunks
            if (min_len > ret_len) {
                if (ret_len >= MAX_ENCODE_BUF) return "";
                tmp[ret_len] = work[0];
                ret_len += 1;

                while (min_len > ret_len) {
                    shuffleAlphabet(work[0..alen]);
                    const need = min_len - ret_len;
                    const take = if (need < alen) need else alen;
                    if (ret_len + take > MAX_ENCODE_BUF) return "";
                    let p: usize = 0;
                    while (p < take) : (p += 1) {
                        tmp[ret_len + p] = work[p];
                    }
                    ret_len += take;
                }
            }

            if (isBlockedId(tmp[0..ret_len])) {
                continue;
            }

            if (ret_len > out.len) return "";
            let c: usize = 0;
            while (c < ret_len) : (c += 1) {
                out[c] = tmp[c];
            }
            return out[0..ret_len];
        }

        return "";
    }

    /// Convenience helper to encode a single `u64` into `out`.
    pub fn encodeOne(n: u64, out: []u8): string {
        const single: [1]u64 = [n];
        return _.encode(single[0..], out);
    }

    /// Decodes a Sqids string into caller-provided `out` slice, returning the number of decoded elements.
    /// Returns `0` if `id` is empty, contains invalid characters, or overflows `out`.
    pub fn decode(id: string, out: []u64): usize {
        if (!_.valid || id.len == 0) return 0;
        const alen = _.alphabet_len;
        const alpha = _.alphabet[0..alen];

        // Validate every character belongs to the alphabet
        let v: usize = 0;
        while (v < id.len) : (v += 1) {
            if (indexOfByte(alpha, id[v]) == null) {
                return 0;
            }
        }

        const prefix = id[0];
        let offset: usize = 0;
        if (found := indexOfByte(alpha, prefix)) {
            offset = found;
        } else {
            return 0;
        }

        let work: [MAX_ALPHABET_LEN]u8 = undefined;
        let w: usize = 0;
        while (w < alen) : (w += 1) {
            work[w] = alpha[(offset + w) % alen];
        }
        reverseBytes(work[0..alen]);

        let rem = id[1..id.len];
        let count: usize = 0;

        while (rem.len > 0) {
            const separator = work[0];
            let sep_pos: usize = rem.len;
            if (found_sep := indexOfByte(rem, separator)) {
                sep_pos = found_sep;
            }

            const chunk = rem[0..sep_pos];
            if (chunk.len == 0) {
                // Reached min_length padding boundary
                break;
            }

            if (count >= out.len) return 0;
            if (val := chunkToNumber(chunk, work[1..alen])) {
                out[count] = val;
                count += 1;
            } else {
                return 0;
            }

            if (sep_pos < rem.len) {
                rem = rem[sep_pos + 1 .. rem.len];
                if (rem.len > 0) {
                    shuffleAlphabet(work[0..alen]);
                }
            } else {
                break;
            }
        }

        return count;
    }

    /// Convenience helper to decode an ID that represents exactly one `u64`.
    /// Returns `null` if decoding fails or produces != 1 numbers.
    pub fn decodeOne(id: string): u64? {
        let buf: [2]u64 = undefined;
        const n = _.decode(id, buf[0..]);
        if (n != 1) return null;
        return buf[0];
    }
}
