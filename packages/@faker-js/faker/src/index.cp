// @cap/faker — Zero-allocation Deterministic Fake Data Generator
// Clean-room pure-CP implementation modeled after npm `@faker-js/faker` and Rust `fake` (MIT License).
// Powered by xoshiro256** PRNG seeded with SplitMix64 for fast, reproducible test fixtures.

const std = @import("std");

const SPLITMIX_GAMMA: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
const SPLITMIX_MUL1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
const SPLITMIX_MUL2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);

const HEX_LOWER: []const u8 = "0123456789abcdef";

const FIRST_NAMES: [32][]const u8 = [
    "Alice", "Benjamin", "Clara", "Daniel", "Elena", "Felix", "Grace", "Henry",
    "Iris", "Julian", "Klara", "Liam", "Maya", "Noah", "Olivia", "Parker",
    "Quinn", "Ruby", "Samuel", "Tessa", "Uriel", "Violet", "Wyatt", "Xena",
    "Yusuf", "Zoe", "Arthur", "Beatrice", "Caleb", "Diana", "Ethan", "Fiona",
];

const LAST_NAMES: [32][]const u8 = [
    "Anderson", "Bennett", "Carter", "Dawson", "Ellis", "Foster", "Graham", "Hayes",
    "Irwin", "Jenkins", "Keller", "Lawson", "Mitchell", "Nelson", "Owens", "Palmer",
    "Quincy", "Reynolds", "Sanders", "Turner", "Upton", "Vance", "Walker", "Young",
    "Zimmerman", "Brooks", "Collins", "Douglas", "Edwards", "Fisher", "Gibson", "Harper",
];

const EMAIL_DOMAINS: [8][]const u8 = [
    "example.com", "example.org", "example.net", "mail.test",
    "demo.io", "sandbox.dev", "acme.test", "cloud.example",
];

const TLDS: [8][]const u8 = [
    "com", "org", "net", "io", "dev", "app", "cloud", "tech",
];

const COUNTRY_CODES: [8][]const u8 = [
    "1", "44", "49", "33", "81", "61", "65", "86",
];

const WORDS: [48][]const u8 = [
    "alpha", "beacon", "cipher", "delta", "ember", "falcon", "granite", "harbor",
    "indigo", "jasper", "kernel", "lantern", "matrix", "nebula", "orbit", "prism",
    "quartz", "ripple", "signal", "timber", "umbra", "vector", "willow", "zenith",
    "bridge", "canvas", "drift", "engine", "fabric", "glacier", "horizon", "island",
    "journey", "kinetic", "ledger", "meadow", "nexus", "oracle", "packet", "quiver",
    "router", "stream", "tunnel", "uplink", "vertex", "window", "yonder", "zephyr",
];

const COMPANY_PREFIXES: [16][]const u8 = [
    "Apex", "Aether", "Cobalt", "Helios", "Hyperion", "Lumen", "Meridian", "Nimbus",
    "Nova", "Obsidian", "Orion", "Polaris", "Quantum", "Solstice", "Stratos", "Vanguard",
];

const COMPANY_NOUNS: [16][]const u8 = [
    "Analytics", "Cloud", "Compute", "Data", "Dynamics", "Energy", "Labs", "Logistics",
    "Networks", "Robotics", "Security", "Semiconductor", "Software", "Systems", "Tech", "Ventures",
];

const COMPANY_SUFFIXES: [8][]const u8 = [
    "Inc.", "LLC", "Corp.", "Ltd.", "Group", "Holdings", "Solutions", "Co.",
];

const CITIES: [24][]const u8 = [
    "San Francisco", "New York", "London", "Berlin", "Tokyo", "Singapore",
    "Sydney", "Toronto", "Zurich", "Amsterdam", "Paris", "Seoul",
    "Austin", "Seattle", "Boston", "Dublin", "Stockholm", "Munich",
    "Melbourne", "Vancouver", "Osaka", "Taipei", "Shanghai", "Shenzhen",
];

const COUNTRIES: [24][]const u8 = [
    "United States", "United Kingdom", "Germany", "Japan", "Singapore", "Australia",
    "Canada", "Switzerland", "Netherlands", "France", "South Korea", "Ireland",
    "Sweden", "Norway", "Finland", "Denmark", "Spain", "Italy",
    "New Zealand", "China", "Brazil", "Mexico", "India", "Poland",
];

fn splitmixStep(state: *u64): u64 {
    state.* = state.* +% SPLITMIX_GAMMA;
    let z = state.*;
    z = (z ^ (z >> 30)) *% SPLITMIX_MUL1;
    z = (z ^ (z >> 27)) *% SPLITMIX_MUL2;
    return z ^ (z >> 31);
}

fn rotl7(x: u64): u64 {
    return (x << 7) | (x >> 57);
}

fn rotl45(x: u64): u64 {
    return (x << 45) | (x >> 19);
}

fn appendByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* += 1;
    }
}

fn appendBytes(out: []u8, pos: *usize, data: []const u8) {
    let i: usize = 0;
    while (i < data.len && pos.* < out.len) : (i += 1) {
        out[pos.*] = data[i];
        pos.* += 1;
    }
}

fn appendLowerAscii(out: []u8, pos: *usize, data: []const u8) {
    let i: usize = 0;
    while (i < data.len && pos.* < out.len) : (i += 1) {
        const c = data[i];
        const lc = if (c >= 'A' && c <= 'Z') (c + 32) else c;
        out[pos.*] = lc;
        pos.* += 1;
    }
}

fn appendCapitalized(out: []u8, pos: *usize, data: []const u8) {
    if (data.len == 0) return;
    const first = data[0];
    const uc = if (first >= 'a' && first <= 'z') (first - 32) else first;
    appendByte(out, pos, uc);
    if (data.len > 1) {
        appendBytes(out, pos, data[1..]);
    }
}

fn appendU64Dec(out: []u8, pos: *usize, val: u64) {
    if (val == 0) {
        appendByte(out, pos, '0');
        return;
    }
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[len] = ('0' as u8) + ((v % 10) as u8);
        len += 1;
        v /= 10;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        appendByte(out, pos, tmp[len - 1 - i]);
    }
}

fn appendDecPadded(out: []u8, pos: *usize, val: u64, width: usize) {
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = ('0' as u8) + ((v % 10) as u8);
            len += 1;
            v /= 10;
        }
    }
    let pad = if (width > len) (width - len) else 0;
    while (pad > 0) : (pad -= 1) {
        appendByte(out, pos, '0');
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        appendByte(out, pos, tmp[len - 1 - i]);
    }
}

fn appendHexByte(out: []u8, pos: *usize, b: u8) {
    appendByte(out, pos, HEX_LOWER[((b >> 4) & 0x0f) as usize]);
    appendByte(out, pos, HEX_LOWER[(b & 0x0f) as usize]);
}

fn isLeapYear(year: u64): bool {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

fn daysInMonth(year: u64, month: u64): u64 {
    if (month == 2) return if (isLeapYear(year)) 29 else 28;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

/// Validates an ASCII numeric string against the Luhn (mod 10) checksum algorithm.
pub fn isValidLuhn(digits: []const u8): bool {
    if (digits.len < 2) return false;
    let sum: u64 = 0;
    let dbl = false;
    let i: usize = digits.len;
    while (i > 0) {
        i -= 1;
        const c = digits[i];
        if (c < '0' || c > '9') return false;
        let d: u64 = (c - '0') as u64;
        if (dbl) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        dbl = !dbl;
    }
    return (sum % 10) == 0;
}

/// Deterministic, zero-allocation fake data generator backed by xoshiro256**.
pub struct Faker {
    s0: u64,
    s1: u64,
    s2: u64,
    s3: u64,

    /// Initializes a deterministic `Faker` instance from a 64-bit seed.
    pub static fn init(seed: u64): Faker {
        let f = Faker{ s0: 0, s1: 0, s2: 0, s3: 0 };
        f.reseed(seed);
        return f;
    }

    /// Re-initializes the internal xoshiro256** state from a new 64-bit seed.
    pub fn reseed(*_, seed: u64) {
        let sm = seed;
        _.s0 = splitmixStep(&sm);
        _.s1 = splitmixStep(&sm);
        _.s2 = splitmixStep(&sm);
        _.s3 = splitmixStep(&sm);
        if (_.s0 == 0 && _.s1 == 0 && _.s2 == 0 && _.s3 == 0) {
            _.s0 = SPLITMIX_GAMMA;
        }
    }

    // -------------------------------------------------------------------------
    // Basic Random Primitives
    // -------------------------------------------------------------------------

    /// Generates the next 64-bit unsigned pseudo-random integer (xoshiro256**).
    pub fn nextU64(*_): u64 {
        const result = rotl7(_.s1 *% 5) *% 9;
        const t = _.s1 << 17;
        _.s2 ^= _.s0;
        _.s3 ^= _.s1;
        _.s1 ^= _.s2;
        _.s0 ^= _.s3;
        _.s2 ^= t;
        _.s3 = rotl45(_.s3);
        return result;
    }

    /// Generates a pseudo-random signed 64-bit integer in the inclusive range `[min, max]`.
    /// If `min >= max`, returns `min`.
    pub fn nextInt(*_, min: i64, max: i64): i64 {
        if (min >= max) return min;
        const lo_u: u64 = @bitCast(min);
        const hi_u: u64 = @bitCast(max);
        const diff: u64 = hi_u -% lo_u;
        if (diff == std.math.maxInt(u64)) {
            return @bitCast(_.nextU64());
        }
        const offset: u64 = _.nextU64() % (diff + 1);
        return @bitCast(lo_u +% offset);
    }

    /// Returns `true` with probability `prob_pct`% (`0` = always false, `>= 100` = always true).
    pub fn nextBool(*_, prob_pct: u8): bool {
        if (prob_pct == 0) return false;
        if (prob_pct >= 100) return true;
        return (_.nextU64() % 100) < (prob_pct as u64);
    }

    /// Picks a random string from `items`, or `""` if `items` is empty.
    pub fn pickStr(*_, items: []const []const u8): []const u8 {
        if (items.len == 0) return "";
        const idx: usize = (_.nextU64() % (items.len as u64)) as usize;
        return items[idx];
    }

    fn nextRangeU64(*_, lo: u64, hi: u64): u64 {
        if (lo >= hi) return lo;
        const span = hi - lo + 1;
        return lo + (_.nextU64() % span);
    }

    // -------------------------------------------------------------------------
    // Person & Identity
    // -------------------------------------------------------------------------

    pub fn firstName(*_): []const u8 {
        return _.pickStr(&FIRST_NAMES);
    }

    pub fn lastName(*_): []const u8 {
        return _.pickStr(&LAST_NAMES);
    }

    pub fn fullName(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        appendBytes(out, &pos, _.firstName());
        appendByte(out, &pos, ' ');
        appendBytes(out, &pos, _.lastName());
        return out[0..pos];
    }

    pub fn username(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        const sep: u8 = if (_.nextBool(50)) '.' else '_';
        const suffix = _.nextRangeU64(10, 99);
        appendLowerAscii(out, &pos, _.firstName());
        appendByte(out, &pos, sep);
        appendLowerAscii(out, &pos, _.lastName());
        appendU64Dec(out, &pos, suffix);
        return out[0..pos];
    }

    pub fn email(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        const sep: u8 = if (_.nextBool(50)) '.' else '_';
        const suffix = _.nextRangeU64(10, 99);
        appendLowerAscii(out, &pos, _.firstName());
        appendByte(out, &pos, sep);
        appendLowerAscii(out, &pos, _.lastName());
        appendU64Dec(out, &pos, suffix);
        appendByte(out, &pos, '@');
        appendBytes(out, &pos, _.pickStr(&EMAIL_DOMAINS));
        return out[0..pos];
    }

    /// Generates an E.164 formatted phone number (e.g. `+14155550192`).
    pub fn phone(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        const cc = _.pickStr(&COUNTRY_CODES);
        appendByte(out, &pos, '+');
        appendBytes(out, &pos, cc);
        // First subscriber digit non-zero (1..9), followed by 9 digits (10 subscriber digits total)
        appendByte(out, &pos, ('1' as u8) + ((_.nextU64() % 9) as u8));
        let i: usize = 0;
        while (i < 9) : (i += 1) {
            appendByte(out, &pos, ('0' as u8) + ((_.nextU64() % 10) as u8));
        }
        return out[0..pos];
    }

    // -------------------------------------------------------------------------
    // Network & Identifiers
    // -------------------------------------------------------------------------

    pub fn ipv4(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        const a = _.nextRangeU64(1, 254);
        const b = _.nextRangeU64(0, 255);
        const c = _.nextRangeU64(0, 255);
        const d = _.nextRangeU64(1, 254);
        appendU64Dec(out, &pos, a);
        appendByte(out, &pos, '.');
        appendU64Dec(out, &pos, b);
        appendByte(out, &pos, '.');
        appendU64Dec(out, &pos, c);
        appendByte(out, &pos, '.');
        appendU64Dec(out, &pos, d);
        return out[0..pos];
    }

    pub fn ipv6(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        const hi = _.nextU64();
        const lo = _.nextU64();
        let g: usize = 0;
        while (g < 8) : (g += 1) {
            if (g > 0) appendByte(out, &pos, ':');
            const word_val: u16 = if (g < 4)
                ((hi >> @as(u6, @intCast((3 - g) * 16))) & 0xffff) as u16
            else
                ((lo >> @as(u6, @intCast((7 - g) * 16))) & 0xffff) as u16;
            appendHexByte(out, &pos, ((word_val >> 8) & 0xff) as u8);
            appendHexByte(out, &pos, (word_val & 0xff) as u8);
        }
        return out[0..pos];
    }

    pub fn domain(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        appendBytes(out, &pos, _.word());
        appendByte(out, &pos, '-');
        appendBytes(out, &pos, _.word());
        appendByte(out, &pos, '.');
        appendBytes(out, &pos, _.pickStr(&TLDS));
        return out[0..pos];
    }

    pub fn url(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        appendBytes(out, &pos, "https://");
        appendBytes(out, &pos, _.word());
        appendByte(out, &pos, '-');
        appendBytes(out, &pos, _.word());
        appendByte(out, &pos, '.');
        appendBytes(out, &pos, _.pickStr(&TLDS));
        appendByte(out, &pos, '/');
        appendBytes(out, &pos, _.word());
        appendByte(out, &pos, '/');
        appendBytes(out, &pos, _.word());
        return out[0..pos];
    }

    /// Generates an RFC 4122 version 4 UUID (`xxxxxxxx-xxxx-4xxx-[89ab]xxx-xxxxxxxxxxxx`).
    pub fn uuidV4(*_, out: []u8): []const u8 {
        const hi = _.nextU64();
        const lo = _.nextU64();
        let bytes: [16]u8 = undefined;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            bytes[i] = ((hi >> @as(u6, @intCast((7 - i) * 8))) & 0xff) as u8;
            bytes[8 + i] = ((lo >> @as(u6, @intCast((7 - i) * 8))) & 0xff) as u8;
        }
        // RFC 4122 §4.4: version 4 (0100xxxx) and variant 1 (10xxxxxx)
        bytes[6] = (bytes[6] & 0x0f) | 0x40;
        bytes[8] = (bytes[8] & 0x3f) | 0x80;

        let pos: usize = 0;
        let b: usize = 0;
        while (b < 16) : (b += 1) {
            if (b == 4 || b == 6 || b == 8 || b == 10) {
                appendByte(out, &pos, '-');
            }
            appendHexByte(out, &pos, bytes[b]);
        }
        return out[0..pos];
    }

    /// Generates a CSS hex color string `#rrggbb`.
    pub fn hexColor(*_, out: []u8): []const u8 {
        const v = _.nextU64();
        let pos: usize = 0;
        appendByte(out, &pos, '#');
        appendHexByte(out, &pos, ((v >> 16) & 0xff) as u8);
        appendHexByte(out, &pos, ((v >> 8) & 0xff) as u8);
        appendHexByte(out, &pos, (v & 0xff) as u8);
        return out[0..pos];
    }

    // -------------------------------------------------------------------------
    // Text & Commerce
    // -------------------------------------------------------------------------

    pub fn word(*_): []const u8 {
        return _.pickStr(&WORDS);
    }

    pub fn sentence(*_, word_count: usize, out: []u8): []const u8 {
        if (word_count == 0) return out[0..0];
        let pos: usize = 0;
        let i: usize = 0;
        while (i < word_count) : (i += 1) {
            if (i > 0) appendByte(out, &pos, ' ');
            const w = _.word();
            if (i == 0) {
                appendCapitalized(out, &pos, w);
            } else {
                appendBytes(out, &pos, w);
            }
        }
        appendByte(out, &pos, '.');
        return out[0..pos];
    }

    pub fn paragraph(*_, sentence_count: usize, out: []u8): []const u8 {
        if (sentence_count == 0) return out[0..0];
        let pos: usize = 0;
        let s: usize = 0;
        while (s < sentence_count) : (s += 1) {
            if (s > 0) appendByte(out, &pos, ' ');
            const wc: usize = _.nextRangeU64(5, 10) as usize;
            let w: usize = 0;
            while (w < wc) : (w += 1) {
                if (w > 0) appendByte(out, &pos, ' ');
                const tok = _.word();
                if (w == 0) {
                    appendCapitalized(out, &pos, tok);
                } else {
                    appendBytes(out, &pos, tok);
                }
            }
            appendByte(out, &pos, '.');
        }
        return out[0..pos];
    }

    pub fn company(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        appendBytes(out, &pos, _.pickStr(&COMPANY_PREFIXES));
        appendByte(out, &pos, ' ');
        appendBytes(out, &pos, _.pickStr(&COMPANY_NOUNS));
        appendByte(out, &pos, ' ');
        appendBytes(out, &pos, _.pickStr(&COMPANY_SUFFIXES));
        return out[0..pos];
    }

    pub fn city(*_): []const u8 {
        return _.pickStr(&CITIES);
    }

    pub fn country(*_): []const u8 {
        return _.pickStr(&COUNTRIES);
    }

    /// Generates an ISO 8601 calendar date string (`YYYY-MM-DD`) between 1970 and 2035.
    pub fn isoDate(*_, out: []u8): []const u8 {
        let pos: usize = 0;
        const year = _.nextRangeU64(1970, 2035);
        const month = _.nextRangeU64(1, 12);
        const day = _.nextRangeU64(1, daysInMonth(year, month));
        appendDecPadded(out, &pos, year, 4);
        appendByte(out, &pos, '-');
        appendDecPadded(out, &pos, month, 2);
        appendByte(out, &pos, '-');
        appendDecPadded(out, &pos, day, 2);
        return out[0..pos];
    }

    // -------------------------------------------------------------------------
    // Financial Testing
    // -------------------------------------------------------------------------

    /// Generates a 16-digit synthetic test credit card number that passes Luhn validation.
    pub fn creditCardNumber(*_, out: []u8): []const u8 {
        let digits: [16]u8 = undefined;
        // Use standard test-card prefixes: 4xxx (Visa-like) or 51xx..55xx (Mastercard-like)
        if (_.nextBool(50)) {
            digits[0] = 4;
            digits[1] = (_.nextU64() % 10) as u8;
        } else {
            digits[0] = 5;
            digits[1] = (1 + (_.nextU64() % 5)) as u8;
        }
        let i: usize = 2;
        while (i < 15) : (i += 1) {
            digits[i] = (_.nextU64() % 10) as u8;
        }
        // Compute Luhn check digit at index 15 (positions 0,2,4,...,14 are doubled)
        let sum: u64 = 0;
        let k: usize = 0;
        while (k < 15) : (k += 1) {
            let d: u64 = digits[k] as u64;
            if ((k % 2) == 0) {
                d *= 2;
                if (d > 9) d -= 9;
            }
            sum += d;
        }
        digits[15] = ((10 - (sum % 10)) % 10) as u8;

        let pos: usize = 0;
        let p: usize = 0;
        while (p < 16) : (p += 1) {
            appendByte(out, &pos, ('0' as u8) + digits[p]);
        }
        return out[0..pos];
    }

    // -------------------------------------------------------------------------
    // Collection Helpers
    // -------------------------------------------------------------------------

    /// Performs an unbiased in-place Fisher-Yates shuffle on `items`.
    pub fn shuffleU64(*_, items: []u64) {
        if (items.len < 2) return;
        let i: usize = items.len - 1;
        while (i > 0) : (i -= 1) {
            const j: usize = (_.nextU64() % ((i + 1) as u64)) as usize;
            const tmp = items[i];
            items[i] = items[j];
            items[j] = tmp;
        }
    }

    /// Samples `min(population_len, out.len)` unique indices in `[0, population_len)` without
    /// replacement using Floyd's algorithm (O(1) extra memory, zero allocation).
    /// Returns the number of sampled indices written into `out`.
    pub fn sampleIndices(*_, population_len: usize, out: []usize): usize {
        const k = if (population_len < out.len) population_len else out.len;
        if (k == 0) return 0;
        let i: usize = 0;
        while (i < k) : (i += 1) {
            const limit = population_len - k + i + 1;
            const candidate: usize = (_.nextU64() % (limit as u64)) as usize;
            let exists = false;
            let j: usize = 0;
            while (j < i) : (j += 1) {
                if (out[j] == candidate) {
                    exists = true;
                    break;
                }
            }
            out[i] = if (exists) (limit - 1) else candidate;
        }
        return k;
    }
}
