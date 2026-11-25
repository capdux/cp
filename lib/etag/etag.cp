// Source: Absorbed from /data/dev/bun/src/http_types/ETag.cp | Enhanced for RFC 9110 §8.8.3 / §13.1 / §13.2.2
// Pure-CP zero-allocation RFC 9110 HTTP ETag parser, strict validator, strong/weak comparison,
// content & stat ETag generators (FNV-1a 64-bit, 128-bit+len, SHA-256, file stat),
// If-Match / If-None-Match / If-Range header evaluation, and conditional request decision machine.

/// RFC 9110 §8.8.3 HTTP Entity-Tag (`"opaque-tag"` or `W/"opaque-tag"`).
/// Zero-allocation value type borrowing its unquoted `tag` slice from the source buffer.
pub struct EntityTag {
    /// Unquoted opaque-tag slice (`*etagc`).
    tag: []const u8,
    /// True if prefixed with `W/` (weak validator).
    is_weak: bool,

    /// Construct a validated strong `EntityTag` from an unquoted opaque-tag slice.
    /// Returns `null` if `tag` contains any byte outside RFC 9110 `etagc`.
    pub static fn strong(tag: []const u8): EntityTag? {
        if (!isValidOpaqueTag(tag)) return null;
        return EntityTag{ tag: tag, is_weak: false };
    }

    /// Construct a validated weak `EntityTag` from an unquoted opaque-tag slice.
    /// Returns `null` if `tag` contains any byte outside RFC 9110 `etagc`.
    pub static fn weak(tag: []const u8): EntityTag? {
        if (!isValidOpaqueTag(tag)) return null;
        return EntityTag{ tag: tag, is_weak: true };
    }

    /// Parse a single entity-tag (`"abc"` or `W/"abc"`), trimming surrounding OWS (`SP` / `HTAB`).
    /// Returns `null` if malformed (missing quotes, lowercase `w/`, or invalid `etagc` bytes).
    pub static fn parse(input: []const u8): EntityTag? {
        let s = etagTrimOws(input);
        if (s.len < 2) return null;

        let is_weak = false;
        if (s.len >= 2 && s[0] == 'W' && s[1] == '/') {
            is_weak = true;
            s = etagTrimOws(s[2..]);
        }

        if (s.len < 2 || s[0] != '"' || s[s.len - 1] != '"') return null;
        const inner = s[1 .. s.len - 1];
        if (!isValidOpaqueTag(inner)) return null;
        return EntityTag{ tag: inner, is_weak: is_weak };
    }

    /// Strictly parse a single RFC 9110 §8.8.3 `entity-tag = [ %s"W/" ] DQUOTE *etagc DQUOTE`.
    /// Unlike `parse`, rejects any leading/trailing OWS and any OWS between `W/` and `"`.
    pub static fn parseStrict(input: []const u8): EntityTag? {
        if (input.len < 2) return null;

        let s = input;
        let is_weak = false;
        if (s.len >= 2 && s[0] == 'W' && s[1] == '/') {
            is_weak = true;
            s = s[2..];
        }

        if (s.len < 2 || s[0] != '"' || s[s.len - 1] != '"') return null;
        const inner = s[1 .. s.len - 1];
        if (!isValidOpaqueTag(inner)) return null;
        return EntityTag{ tag: inner, is_weak: is_weak };
    }

    /// Returns `true` if this entity-tag is weak (`W/"..."`).
    pub fn isWeak(): bool {
        return _.is_weak;
    }

    /// Returns `true` if this entity-tag is strong (`"..."`).
    pub fn isStrong(): bool {
        return !_.is_weak;
    }

    /// Returns the unquoted opaque-tag slice.
    pub fn opaqueTag(): []const u8 {
        return _.tag;
    }

    /// Returns a weak copy of this entity-tag.
    pub fn toWeak(): EntityTag {
        return EntityTag{ tag: _.tag, is_weak: true };
    }

    /// Returns a strong copy of this entity-tag.
    pub fn toStrong(): EntityTag {
        return EntityTag{ tag: _.tag, is_weak: false };
    }

    /// Exact byte length required to format this entity-tag as an HTTP header value.
    pub fn formattedLen(): usize {
        const extra: usize = if (_.is_weak) 4 else 2;
        return _.tag.len + extra;
    }

    /// RFC 9110 §8.8.3.2 Weak Comparison: true iff opaque-tags match byte-for-byte.
    pub fn weakEq(other: EntityTag): bool {
        return etagStrEq(_.tag, other.tag);
    }

    /// RFC 9110 §8.8.3.2 Strong Comparison: true iff neither is weak and opaque-tags match.
    pub fn strongEq(other: EntityTag): bool {
        if (_.is_weak || other.is_weak) return false;
        return etagStrEq(_.tag, other.tag);
    }

    /// Backward-compatible alias for `weakEq`.
    pub fn weakMatch(other: EntityTag): bool {
        return _.weakEq(other);
    }

    /// Backward-compatible alias for `strongEq`.
    pub fn strongMatch(other: EntityTag): bool {
        return _.strongEq(other);
    }

    /// Format back to HTTP header syntax (`"tag"` or `W/"tag"`) into caller-supplied `out`.
    /// Returns `null` if `out` is too small.
    pub fn format(out: []u8): ([]const u8)? {
        const needed = _.formattedLen();
        if (needed > out.len) return null;
        let p: usize = 0;
        if (_.is_weak) {
            out[0] = 'W';
            out[1] = '/';
            p = 2;
        }
        out[p] = '"';
        p += 1;
        let i: usize = 0;
        while (i < _.tag.len) : (i += 1) {
            out[p] = _.tag[i];
            p += 1;
        }
        out[p] = '"';
        p += 1;
        return out[0..p];
    }

    /// Backward-compatible alias for `format`.
    pub fn toHeader(out: []u8): ([]const u8)? {
        return _.format(out);
    }
}

/// Backward-compatible RFC 9110 entity-tag struct (`"abc"` or `W/"abc"`).
/// Provides 100% API compatibility with earlier `cap:etag` consumers alongside `EntityTag`.
pub struct ParsedETag {
    /// Unquoted opaque-tag slice.
    tag: []const u8,
    /// True if prefixed with `W/`.
    is_weak: bool,

    /// Parse a single entity-tag (`"abc"` or `W/"abc"`).
    pub static fn parse(input: []const u8): ParsedETag? {
        const et = EntityTag.parse(input) ?? return null;
        return ParsedETag{ tag: et.tag, is_weak: et.is_weak };
    }

    /// Strictly parse a single RFC 9110 §8.8.3 entity-tag.
    pub static fn parseStrict(input: []const u8): ParsedETag? {
        const et = EntityTag.parseStrict(input) ?? return null;
        return ParsedETag{ tag: et.tag, is_weak: et.is_weak };
    }

    /// Convert to `EntityTag`.
    pub fn toEntityTag(): EntityTag {
        return EntityTag{ tag: _.tag, is_weak: _.is_weak };
    }

    /// RFC 9110 §8.8.3.2 Weak Comparison.
    pub fn weakMatch(other: ParsedETag): bool {
        return etagStrEq(_.tag, other.tag);
    }

    /// RFC 9110 §8.8.3.2 Strong Comparison.
    pub fn strongMatch(other: ParsedETag): bool {
        if (_.is_weak || other.is_weak) return false;
        return etagStrEq(_.tag, other.tag);
    }

    /// Format back to HTTP header syntax (`"tag"` or `W/"tag"`) into `out`.
    pub fn toHeader(out: []u8): ([]const u8)? {
        return _.toEntityTag().format(out);
    }
}

/// RFC 9110 §13.2.2 Conditional Request Precondition Evaluation Result.
pub enum PreconditionResult {
    /// All preconditions passed; proceed with normal request handling.
    Proceed,
    /// `If-None-Match` matched on `GET` or `HEAD`; respond with `304 Not Modified`.
    NotModified304,
    /// `If-Match` failed, or `If-None-Match` matched on a non-GET/HEAD method; respond with `412 Precondition Failed`.
    PreconditionFailed412,
}

/// Zero-allocation iterator over a comma-separated HTTP `If-Match` / `If-None-Match` header value.
/// Properly ignores commas inside double-quoted opaque-tags and trims OWS.
pub struct ETagListIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header_val: []const u8): ETagListIter {
        return ETagListIter{ src: etagTrimOws(header_val), pos: 0 };
    }

    /// Returns `true` if the trimmed header value is the wildcard `*`.
    pub fn isWildcard(): bool {
        return etagStrEq(_.src, "*");
    }

    /// Returns the next raw comma-separated trimmed item, respecting double quotes.
    pub fn nextRaw(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            let in_quote = false;
            while (_.pos < _.src.len) : (_.pos += 1) {
                const c = _.src[_.pos];
                if (c == '"') {
                    in_quote = !in_quote;
                } else if (c == ',' && !in_quote) {
                    break;
                }
            }
            const raw = etagTrimOws(_.src[start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (raw.len > 0) return raw;
        }
        return null;
    }

    /// Returns the next valid `EntityTag` in the header list, skipping empty or malformed entries.
    pub fn next(*_): EntityTag? {
        while (true) {
            const raw = _.nextRaw() ?? return null;
            if (parsed := EntityTag.parse(raw)) {
                return parsed;
            }
        }
    }
}

// ---------- RFC 9110 Character & Header Utilities ----------

/// RFC 9110 §8.8.3 `etagc = %x21 / %x23-7E / obs-text` (`%x80-FF`).
/// Rejects CTLs (`0x00..0x1F`, `0x7F`), space (`0x20`), and double-quote (`0x22`).
pub fn isValidEtagChar(c: u8): bool {
    return c == 0x21 || (c >= 0x23 && c <= 0x7E) || c >= 0x80;
}

/// Validates that every byte in `tag` is a legal RFC 9110 `etagc`.
pub fn isValidOpaqueTag(tag: []const u8): bool {
    let i: usize = 0;
    while (i < tag.len) : (i += 1) {
        if (!isValidEtagChar(tag[i])) return false;
    }
    return true;
}

/// Returns `true` if `header_val` (after trimming OWS) is the `*` wildcard.
pub fn isWildcardHeader(header_val: []const u8): bool {
    return etagStrEq(etagTrimOws(header_val), "*");
}

/// Parse all valid `EntityTag` entries from an `If-Match` / `If-None-Match` header list into `out`.
/// Returns the number of parsed tags written (capped at `out.len`).
pub fn parseHeaderList(header_val: []const u8, out: []EntityTag): usize {
    let iter = ETagListIter.init(header_val);
    if (iter.isWildcard()) return 0;
    let count: usize = 0;
    while (count < out.len) {
        const tag = iter.next() ?? break;
        out[count] = tag;
        count += 1;
    }
    return count;
}

// ---------- Conditional Header Matchers ----------

/// Evaluate HTTP `If-None-Match` header against a parsed `EntityTag` using RFC 9110 weak comparison.
/// Returns `true` if `header_val` matches `current` (i.e. `*` or any tag weak-equals `current`).
pub fn ifNoneMatchTag(current: EntityTag, header_val: []const u8): bool {
    const trimmed = etagTrimOws(header_val);
    if (trimmed.len == 0) return false;
    if (etagStrEq(trimmed, "*")) return true;
    return etagMatchHeaderList(current, trimmed, false);
}

/// Evaluate HTTP `If-Match` header against a parsed `EntityTag` using RFC 9110 strong comparison.
/// Returns `true` if `header_val` matches `current` (i.e. `*` or any tag strong-equals `current`).
pub fn ifMatchTag(current: EntityTag, header_val: []const u8): bool {
    const trimmed = etagTrimOws(header_val);
    if (trimmed.len == 0) return false;
    if (etagStrEq(trimmed, "*")) return true;
    return etagMatchHeaderList(current, trimmed, true);
}

/// Evaluate HTTP `If-None-Match` header against `etag` using RFC 9110 weak comparison.
/// Returns `true` if the header matches (e.g. server should reply `304 Not Modified` for GET/HEAD).
pub fn ifNoneMatch(etag: []const u8, header_val: []const u8): bool {
    const trimmed = etagTrimOws(header_val);
    if (trimmed.len == 0) return false;
    if (etagStrEq(trimmed, "*")) return true;

    const ours = EntityTag.parse(etag) ?? return false;
    return etagMatchHeaderList(ours, trimmed, false);
}

/// Evaluate HTTP `If-Match` header against `etag` using RFC 9110 strong comparison.
/// Returns `true` if the precondition succeeds.
pub fn ifMatch(etag: []const u8, header_val: []const u8): bool {
    const trimmed = etagTrimOws(header_val);
    if (trimmed.len == 0) return false;
    if (etagStrEq(trimmed, "*")) return true;

    const ours = EntityTag.parse(etag) ?? return false;
    return etagMatchHeaderList(ours, trimmed, true);
}

/// Evaluate HTTP `If-Range` header (entity-tag form) using RFC 9110 §13.1.5 strong comparison.
/// Unlike `If-Match`, `If-Range` does NOT accept `*` or comma-separated lists.
pub fn ifRangeMatch(etag: []const u8, header_val: []const u8): bool {
    const ours = EntityTag.parse(etag) ?? return false;
    const req = EntityTag.parse(header_val) ?? return false;
    return ours.strongEq(req);
}

/// RFC 9110 §13.2.2 Conditional Request Precondition Decision Machine.
/// Evaluates `If-Match` (Step 1) and `If-None-Match` (Step 3) in RFC precedence order:
/// - `method`: HTTP method (e.g. `"GET"`, `"HEAD"`, `"PUT"`, `"POST"`, `"DELETE"`, `"PATCH"`).
/// - `current_etag`: current representation's ETag header value, or `null` if resource does not exist.
/// - `if_match`: `If-Match` header value if present, or `null`.
/// - `if_none_match`: `If-None-Match` header value if present, or `null`.
pub fn evaluatePreconditions(
    method: []const u8,
    current_etag: ([]const u8)?,
    if_match: ([]const u8)?,
    if_none_match: ([]const u8)?,
): PreconditionResult {
    let has_rep = false;
    let cur_tag: EntityTag? = null;
    if (raw := current_etag) {
        const trimmed_cur = etagTrimOws(raw);
        if (trimmed_cur.len > 0) {
            has_rep = true;
            cur_tag = EntityTag.parse(trimmed_cur);
        }
    }

    // Step 1: If-Match (RFC 9110 §13.1.1 & §13.2.2)
    if (im_raw := if_match) {
        const im = etagTrimOws(im_raw);
        if (im.len > 0) {
            if (!has_rep) {
                return PreconditionResult.PreconditionFailed412;
            }
            if (!etagStrEq(im, "*")) {
                const cur = cur_tag ?? return PreconditionResult.PreconditionFailed412;
                if (!etagMatchHeaderList(cur, im, true)) {
                    return PreconditionResult.PreconditionFailed412;
                }
            }
        }
    }

    // Step 3: If-None-Match (RFC 9110 §13.1.2 & §13.2.2)
    if (inm_raw := if_none_match) {
        const inm = etagTrimOws(inm_raw);
        if (inm.len > 0) {
            let matched = false;
            if (etagStrEq(inm, "*")) {
                matched = has_rep;
            } else if (has_rep) {
                if (cur := cur_tag) {
                    matched = etagMatchHeaderList(cur, inm, false);
                }
            }

            if (matched) {
                if (etagIsSafeCacheMethod(method)) {
                    return PreconditionResult.NotModified304;
                }
                return PreconditionResult.PreconditionFailed412;
            }
        }
    }

    return PreconditionResult.Proceed;
}

// ---------- ETag Generators ----------

/// Generate a strong content ETag (`"<16-hex-fnv1a64>"`) into `out` (needs >= 18 bytes).
pub fn generateStrong(content: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 18) return null;
    const h = etagFnv1a64(content);
    out[0] = '"';
    etagWriteHexU64(h, out[1..17]);
    out[17] = '"';
    return out[0..18];
}

/// Generate a weak content ETag (`W/"<16-hex-fnv1a64>"`) into `out` (needs >= 20 bytes).
pub fn generateWeak(content: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 20) return null;
    const h = etagFnv1a64(content);
    out[0] = 'W';
    out[1] = '/';
    out[2] = '"';
    etagWriteHexU64(h, out[3..19]);
    out[19] = '"';
    return out[0..20];
}

/// Generate a strong content ETag with length + 128-bit dual hash (`"<hex-len>-<32-hex>"`).
/// Needs at most 51 bytes (2 quotes + up to 16 hex len + 1 dash + 32 hex hash).
pub fn generateContentStrong(content: []const u8, out: []u8): ([]const u8)? {
    return etagFormatContent128(content, false, out);
}

/// Generate a weak content ETag with length + 128-bit dual hash (`W/"<hex-len>-<32-hex>"`).
/// Needs at most 53 bytes.
pub fn generateContentWeak(content: []const u8, out: []u8): ([]const u8)? {
    return etagFormatContent128(content, true, out);
}

/// Generate a cryptographic SHA-256 strong content ETag (`"<64-hex-sha256>"`).
/// Needs >= 66 bytes in `out`.
pub fn generateSha256Strong(content: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 66) return null;
    let digest: [32]u8 = [0; 32];
    etagSha256(content, digest[0..]);
    out[0] = '"';
    etagWriteHexBytes(digest[0..], out[1..65]);
    out[65] = '"';
    return out[0..66];
}

/// Generate a cryptographic SHA-256 weak content ETag (`W/"<64-hex-sha256>"`).
/// Needs >= 68 bytes in `out`.
pub fn generateSha256Weak(content: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 68) return null;
    let digest: [32]u8 = [0; 32];
    etagSha256(content, digest[0..]);
    out[0] = 'W';
    out[1] = '/';
    out[2] = '"';
    etagWriteHexBytes(digest[0..], out[3..67]);
    out[67] = '"';
    return out[0..68];
}

/// Generate a weak ETag from file metadata (`W/"<hex-size>-<hex-mtime_ns>"`).
/// Follows RFC 9110 §8.8.3.1 (stat-based validators should be weak) and Rust `etag` crate conventions.
/// Handles negative `mtime_ns` (pre-epoch timestamps) cleanly as `W/"<hex-size>--<hex-abs>"`.
/// Needs at most 54 bytes in `out`.
pub fn fromStat(size: u64, mtime_ns: i128, out: []u8): ([]const u8)? {
    return etagFormatStat(size, mtime_ns, true, out);
}

/// Generate a strong ETag from file metadata (`"<hex-size>-<hex-mtime_ns>"`).
/// Use only when you can guarantee byte-for-byte identity across identical `(size, mtime_ns)`.
pub fn fromStatStrong(size: u64, mtime_ns: i128, out: []u8): ([]const u8)? {
    return etagFormatStat(size, mtime_ns, false, out);
}

// ---------- Internal Helpers (module-prefixed to avoid symbol collisions) ----------

fn etagFormatStat(size: u64, mtime_ns: i128, is_weak: bool, out: []u8): ([]const u8)? {
    let size_buf: [16]u8 = [0; 16];
    const size_hex = etagWriteCompactHexU64(size, size_buf[0..]);

    const is_neg = mtime_ns < 0;
    let mag: u128 = 0;
    if (is_neg) {
        // Safe conversion without signed overflow even for i128 min
        const shifted = -(mtime_ns + 1);
        mag = @as(u128, @intCast(shifted)) + 1;
    } else {
        mag = @as(u128, @intCast(mtime_ns));
    }

    let mtime_buf: [32]u8 = [0; 32];
    const mtime_hex = etagWriteCompactHexU128(mag, mtime_buf[0..]);

    const prefix_len: usize = if (is_weak) 3 else 1;
    const neg_len: usize = if (is_neg) 1 else 0;
    const total = prefix_len + size_hex.len + 1 + neg_len + mtime_hex.len + 1;
    if (total > out.len) return null;

    let p: usize = 0;
    if (is_weak) {
        out[0] = 'W';
        out[1] = '/';
        out[2] = '"';
        p = 3;
    } else {
        out[0] = '"';
        p = 1;
    }

    p = etagCopyBytes(out, p, size_hex);
    out[p] = '-';
    p += 1;
    if (is_neg) {
        out[p] = '-';
        p += 1;
    }
    p = etagCopyBytes(out, p, mtime_hex);
    out[p] = '"';
    p += 1;
    return out[0..p];
}

fn etagFormatContent128(content: []const u8, is_weak: bool, out: []u8): ([]const u8)? {
    let len_buf: [16]u8 = [0; 16];
    const len_hex = etagWriteCompactHexU64(@as(u64, @intCast(content.len)), len_buf[0..]);

    const prefix_len: usize = if (is_weak) 3 else 1;
    const total = prefix_len + len_hex.len + 1 + 32 + 1;
    if (total > out.len) return null;

    const h1 = etagFnv1a64(content);
    const h2 = etagMixHash64(content);

    let p: usize = 0;
    if (is_weak) {
        out[0] = 'W';
        out[1] = '/';
        out[2] = '"';
        p = 3;
    } else {
        out[0] = '"';
        p = 1;
    }

    p = etagCopyBytes(out, p, len_hex);
    out[p] = '-';
    p += 1;
    etagWriteHexU64(h1, out[p .. p + 16]);
    p += 16;
    etagWriteHexU64(h2, out[p .. p + 16]);
    p += 16;
    out[p] = '"';
    p += 1;
    return out[0..p];
}

fn etagMatchHeaderList(ours: EntityTag, list: []const u8, strong: bool): bool {
    let iter = ETagListIter.init(list);
    while (true) {
        const parsed = iter.next() ?? break;
        if (strong) {
            if (ours.strongEq(parsed)) return true;
        } else {
            if (ours.weakEq(parsed)) return true;
        }
    }
    return false;
}

fn etagIsSafeCacheMethod(method: []const u8): bool {
    return etagStrEq(method, "GET") || etagStrEq(method, "HEAD");
}

fn etagFnv1a64(data: []const u8): u64 {
    let h: u64 = (@as(u64, 0xcbf29ce4) << 32) | @as(u64, 0x84222325);
    const prime: u64 = 0x100000001b3;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h ^= @as(u64, data[i]);
        h = h *% prime;
    }
    return h;
}

/// Secondary independent 64-bit mixing hash (SplitMix64 + FNV-like avalanche) for 128-bit content tags.
fn etagMixHash64(data: []const u8): u64 {
    const c_golden: u64 = (@as(u64, 0x9e3779b9) << 32) | @as(u64, 0x7f4a7c15);
    const c_m1: u64 = (@as(u64, 0xbf58476d) << 32) | @as(u64, 0x1ce4e5b9);
    const c_m2: u64 = (@as(u64, 0x94d049bb) << 32) | @as(u64, 0x133111eb);
    let h: u64 = c_golden;
    h ^= @as(u64, @intCast(data.len)) *% c_m1;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = (h ^ (@as(u64, data[i]) +% c_golden)) *% c_m2;
        h ^= h >> 31;
    }
    h ^= h >> 30;
    h = h *% c_m1;
    h ^= h >> 27;
    h = h *% c_m2;
    h ^= h >> 31;
    return h;
}

// ---------- Pure-CP Zero-Alloc SHA-256 (FIPS 180-4) ----------

fn etagRotr32(x: u32, n: u5): u32 {
    const inv: u5 = @intCast(32 - @as(u6, n));
    return (x >> n) | (x << inv);
}

fn etagSha256Compress(state: []u32, block: []const u8) {
    const k: [64]u32 = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    ];

    let w: [64]u32 = [0; 64];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const j = i * 4;
        w[i] = (@as(u32, block[j]) << 24) |
            (@as(u32, block[j + 1]) << 16) |
            (@as(u32, block[j + 2]) << 8) |
            @as(u32, block[j + 3]);
    }
    i = 16;
    while (i < 64) : (i += 1) {
        const s0 = etagRotr32(w[i - 15], 7) ^ etagRotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const s1 = etagRotr32(w[i - 2], 17) ^ etagRotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] +% s0 +% w[i - 7] +% s1;
    }

    let a = state[0];
    let b = state[1];
    let c = state[2];
    let d = state[3];
    let e = state[4];
    let f = state[5];
    let g = state[6];
    let h = state[7];

    i = 0;
    while (i < 64) : (i += 1) {
        const S1 = etagRotr32(e, 6) ^ etagRotr32(e, 11) ^ etagRotr32(e, 25);
        const ch = (e & f) ^ ((~e) & g);
        const temp1 = h +% S1 +% ch +% k[i] +% w[i];
        const S0 = etagRotr32(a, 2) ^ etagRotr32(a, 13) ^ etagRotr32(a, 22);
        const maj = (a & b) ^ (a & c) ^ (b & c);
        const temp2 = S0 +% maj;

        h = g;
        g = f;
        f = e;
        e = d +% temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 +% temp2;
    }

    state[0] = state[0] +% a;
    state[1] = state[1] +% b;
    state[2] = state[2] +% c;
    state[3] = state[3] +% d;
    state[4] = state[4] +% e;
    state[5] = state[5] +% f;
    state[6] = state[6] +% g;
    state[7] = state[7] +% h;
}

fn etagSha256(data: []const u8, out: []u8) {
    let state: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];

    let offset: usize = 0;
    while (offset + 64 <= data.len) : (offset += 64) {
        etagSha256Compress(state[0..], data[offset .. offset + 64]);
    }

    let block: [64]u8 = [0; 64];
    const rem = data.len - offset;
    let i: usize = 0;
    while (i < rem) : (i += 1) {
        block[i] = data[offset + i];
    }
    block[rem] = 0x80;

    if (rem >= 56) {
        etagSha256Compress(state[0..], block[0..]);
        i = 0;
        while (i < 64) : (i += 1) {
            block[i] = 0;
        }
    }

    const bit_len: u64 = @as(u64, @intCast(data.len)) *% 8;
    block[56] = @intCast((bit_len >> 56) & 0xFF);
    block[57] = @intCast((bit_len >> 48) & 0xFF);
    block[58] = @intCast((bit_len >> 40) & 0xFF);
    block[59] = @intCast((bit_len >> 32) & 0xFF);
    block[60] = @intCast((bit_len >> 24) & 0xFF);
    block[61] = @intCast((bit_len >> 16) & 0xFF);
    block[62] = @intCast((bit_len >> 8) & 0xFF);
    block[63] = @intCast(bit_len & 0xFF);

    etagSha256Compress(state[0..], block[0..]);

    i = 0;
    while (i < 8) : (i += 1) {
        const v = state[i];
        out[i * 4] = @intCast((v >> 24) & 0xFF);
        out[i * 4 + 1] = @intCast((v >> 16) & 0xFF);
        out[i * 4 + 2] = @intCast((v >> 8) & 0xFF);
        out[i * 4 + 3] = @intCast(v & 0xFF);
    }
}

// ---------- Encoding & String Helpers ----------

fn etagWriteHexU64(v: u64, dst: []u8) {
    const hex = "0123456789abcdef";
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const shift: u6 = @intCast((15 - i) * 4);
        dst[i] = hex[@as(usize, @intCast((v >> shift) & 0xF))];
    }
}

fn etagWriteCompactHexU64(v: u64, dst: []u8): []const u8 {
    const hex = "0123456789abcdef";
    if (v == 0) {
        dst[0] = '0';
        return dst[0..1];
    }
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let cur = v;
    while (cur > 0) {
        tmp[n] = hex[@as(usize, @intCast(cur & 0xF))];
        cur = cur >> 4;
        n += 1;
    }
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = tmp[n - 1 - i];
    }
    return dst[0..n];
}

fn etagWriteCompactHexU128(v: u128, dst: []u8): []const u8 {
    const hex = "0123456789abcdef";
    if (v == 0) {
        dst[0] = '0';
        return dst[0..1];
    }
    let tmp: [32]u8 = [0; 32];
    let n: usize = 0;
    let cur = v;
    while (cur > 0) {
        tmp[n] = hex[@as(usize, @intCast(cur & 0xF))];
        cur = cur >> 4;
        n += 1;
    }
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = tmp[n - 1 - i];
    }
    return dst[0..n];
}

fn etagWriteHexBytes(src: []const u8, dst: []u8) {
    const hex = "0123456789abcdef";
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const b = src[i];
        dst[i * 2] = hex[@as(usize, b >> 4)];
        dst[i * 2 + 1] = hex[@as(usize, b & 0xF)];
    }
}

fn etagCopyBytes(dst: []u8, offset: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn etagTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn etagStrEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}
