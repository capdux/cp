// @cap/num-rational — Zero-allocation exact rational number (`Ratio64`) engine for CP
// Clean-room MIT implementation inspired by Rust `num-rational`.

// ---------------------------------------------------------------------------
// Internal Math & String Helpers
// ---------------------------------------------------------------------------

fn absU64(n: i64) -> u64 {
    if (n >= 0) {
        return n as u64;
    }
    return ((0 - (n + 1)) as u64) + 1;
}

/// Signed truncating division `a / b` where `b > 0`, using unsigned division internally.
fn divTruncPos(a: i64, b_pos: i64) -> i64 {
    const bu = b_pos as u64;
    if (a >= 0) {
        return ((a as u64) / bu) as i64;
    }
    return -((absU64(a) / bu) as i64);
}

/// Remainder magnitude `|a| % b` where `b > 0`.
fn remMagPos(a: i64, b_pos: i64) -> u64 {
    return absU64(a) % (b_pos as u64);
}

fn gcdU64(a: u64, b: u64) -> u64 {
    let x = a;
    let y = b;
    while (y != 0) {
        const t = x % y;
        x = y;
        y = t;
    }
    return x;
}

fn isWs(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: string) -> string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isWs(s[a])) {
        a += 1;
    }
    while (b > a && isWs(s[b - 1])) {
        b -= 1;
    }
    return s[a..b];
}

fn parseI64Strict(raw: string) -> i64? {
    const s = trimWs(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: u64 = 0;
    const limit: u64 = if (neg) ((1 as u64) << 63) else (((1 as u64) << 63) - 1);
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit = (c - '0') as u64;
        if (acc > (limit - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    if (neg) {
        if (acc == ((1 as u64) << 63)) {
            return -9223372036854775807 - 1;
        }
        return -(acc as i64);
    }
    return acc as i64;
}

// ---------------------------------------------------------------------------
// Public Ratio64 Struct
// ---------------------------------------------------------------------------

pub struct Ratio64 {
    pub numer: i64,
    pub denom: i64,

    /// Constructs a reduced `Ratio64` with strictly positive `denom`.
    /// Returns `null` if `denom == 0`.
    pub static fn new(numer: i64, denom: i64) -> Ratio64? {
        if (denom == 0) {
            return null;
        }
        if (numer == 0) {
            return Ratio64{ numer: 0, denom: 1 };
        }
        let n = numer;
        let d = denom;
        if (d < 0) {
            n = -n;
            d = -d;
        }
        const g = gcdU64(absU64(n), d as u64) as i64;
        return Ratio64{
            numer: divTruncPos(n, g),
            denom: divTruncPos(d, g),
        };
    }

    pub static fn fromInt(n: i64) -> Ratio64 {
        return Ratio64{ numer: n, denom: 1 };
    }

    pub static fn zero() -> Ratio64 {
        return Ratio64{ numer: 0, denom: 1 };
    }

    pub static fn one() -> Ratio64 {
        return Ratio64{ numer: 1, denom: 1 };
    }

    pub fn neg() -> Ratio64 {
        return Ratio64{ numer: -_.numer, denom: _.denom };
    }

    pub fn abs() -> Ratio64 {
        return Ratio64{
            numer: if (_.numer < 0) -_.numer else _.numer,
            denom: _.denom,
        };
    }

    /// Returns the multiplicative inverse `denom / numer`, or `null` if zero.
    pub fn recip() -> Ratio64? {
        if (_.numer == 0) {
            return null;
        }
        return Ratio64.new(_.denom, _.numer);
    }

    /// Exact addition with pre-reduction by `gcd(d1, d2)` to avoid intermediate overflow.
    pub fn add(other: Ratio64) -> Ratio64 {
        const g = gcdU64(_.denom as u64, other.denom as u64) as i64;
        const d1_g = divTruncPos(_.denom, g);
        const d2_g = divTruncPos(other.denom, g);
        const n = _.numer * d2_g + other.numer * d1_g;
        const d = d1_g * other.denom;
        return Ratio64.new(n, d).?;
    }

    /// Exact subtraction `self - other`.
    pub fn sub(other: Ratio64) -> Ratio64 {
        return _.add(other.neg());
    }

    /// Exact multiplication with cross-cancellation to avoid intermediate overflow.
    pub fn mul(other: Ratio64) -> Ratio64 {
        if (_.numer == 0 || other.numer == 0) {
            return Ratio64.zero();
        }
        const g1 = gcdU64(absU64(_.numer), other.denom as u64) as i64;
        const g2 = gcdU64(absU64(other.numer), _.denom as u64) as i64;
        const n = divTruncPos(_.numer, g1) * divTruncPos(other.numer, g2);
        const d = divTruncPos(_.denom, g2) * divTruncPos(other.denom, g1);
        return Ratio64.new(n, d).?;
    }

    /// Exact division `self / other`, returning `null` on division by zero.
    pub fn div(other: Ratio64) -> Ratio64? {
        const inv = other.recip();
        if (inv == null) {
            return null;
        }
        return _.mul(inv.?);
    }

    pub fn equals(other: Ratio64) -> bool {
        return _.numer == other.numer && _.denom == other.denom;
    }

    pub fn isInteger() -> bool {
        return _.denom == 1;
    }

    /// Truncates toward zero.
    pub fn trunc() -> i64 {
        return divTruncPos(_.numer, _.denom);
    }

    /// Rounds toward negative infinity.
    pub fn floor() -> i64 {
        const q = divTruncPos(_.numer, _.denom);
        if (_.numer >= 0) {
            return q;
        }
        const r = remMagPos(_.numer, _.denom);
        return if (r == 0) q else (q - 1);
    }

    /// Rounds toward positive infinity.
    pub fn ceil() -> i64 {
        const q = divTruncPos(_.numer, _.denom);
        if (_.numer <= 0) {
            return q;
        }
        const r = remMagPos(_.numer, _.denom);
        return if (r == 0) q else (q + 1);
    }

    /// Exact ordering returning `-1`, `0`, or `1`.
    pub fn cmp(other: Ratio64) -> i32 {
        if (_.equals(other)) {
            return 0;
        }
        const q1 = _.floor();
        const q2 = other.floor();
        if (q1 != q2) {
            return if (q1 < q2) -1 else 1;
        }
        // Same floor: compare non-negative fractional remainders r1/d1 vs r2/d2
        const r1 = _.numer - q1 * _.denom;
        const r2 = other.numer - q2 * other.denom;
        const g = gcdU64(_.denom as u64, other.denom as u64) as i64;
        const lhs = r1 * divTruncPos(other.denom, g);
        const rhs = r2 * divTruncPos(_.denom, g);
        if (lhs < rhs) return -1;
        if (lhs > rhs) return 1;
        return 0;
    }

    pub fn toF64() -> f64 {
        return (_.numer as f64) / (_.denom as f64);
    }
}

// ---------------------------------------------------------------------------
// Parsing & Formatting (Zero Heap Allocation)
// ---------------------------------------------------------------------------

/// Parses a rational string such as `"3/4"`, `"-6/8"`, or `"42"` into a reduced `Ratio64`.
pub fn parseRatio(s: string) -> Ratio64? {
    const trimmed = trimWs(s);
    if (trimmed.len == 0) {
        return null;
    }
    let slash: usize = trimmed.len;
    let i: usize = 0;
    while (i < trimmed.len) {
        if (trimmed[i] == '/') {
            slash = i;
            break;
        }
        i += 1;
    }
    if (slash >= trimmed.len) {
        const n_opt = parseI64Strict(trimmed);
        if (n_opt == null) return null;
        return Ratio64.fromInt(n_opt.?);
    }
    const n_opt = parseI64Strict(trimmed[0..slash]);
    const d_opt = parseI64Strict(trimmed[(slash + 1)..trimmed.len]);
    if (n_opt == null || d_opt == null) {
        return null;
    }
    return Ratio64.new(n_opt.?, d_opt.?);
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendI64(out: []u8, start_pos: usize, val: i64) -> usize {
    let pos = start_pos;
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    if (val < 0) {
        pos = appendByte(out, pos, '-');
    }
    let v = absU64(val);
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    while (v > 0) {
        digits[len] = ('0' as u64 + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let k = len;
    while (k > 0) {
        k -= 1;
        pos = appendByte(out, pos, digits[k]);
    }
    return pos;
}

/// Formats `r` into `out` (`"5"` when `denom == 1`, otherwise `"2/3"`).
pub fn formatRatio(r: Ratio64, out: []u8) -> string {
    let pos = appendI64(out, 0 as usize, r.numer);
    if (r.denom != 1) {
        pos = appendByte(out, pos, '/');
        pos = appendI64(out, pos, r.denom);
    }
    return out[0..pos];
}
