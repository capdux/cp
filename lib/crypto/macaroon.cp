// Origin: Pure-CP Stdlib (Zero-Alloc Macaroon Capability Tokens & Chained HMAC Attenuation Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation Macaroons (Birgisson et al. 2014) capability token engine:
//   - Offline one-way HMAC-SHA256 caveat attenuation: sig_0 = HMAC(root_key, id), sig_i = HMAC(sig_{i-1}, caveat_i)
//   - Constant-time 32-byte signature verification preventing timing side-channels
//   - Built-in fail-closed first-party caveat verifier: exp < / exp <=, scope =, method =, prefix =,
//     and custom key = value (with exact & wildcard '*' matching) reporting exact failed caveat index
//   - Zero-alloc binary wire codec (MAC1 format) with strict truncation, bounds & trailing-byte guards

const std = @import("std");

// ============================================================================
// 1. Constants & Internal Crypto / Parsing Helpers
// ============================================================================

/// HMAC-SHA256 chained signature byte length (256-bit).
pub const MACAROON_SIG_LEN: usize = 32;

/// Maximum inline byte length for Macaroon location hint.
pub const MACAROON_MAX_LOCATION_LEN: usize = 64;

/// Maximum inline byte length for Macaroon identifier.
pub const MACAROON_MAX_ID_LEN: usize = 64;

/// Maximum number of first-party caveats supported inline (zero heap).
pub const MACAROON_MAX_CAVEATS: usize = 16;

/// Maximum inline byte length per first-party caveat predicate.
pub const MACAROON_MAX_CAVEAT_LEN: usize = 96;

/// Maximum custom assertion claims attachable to `MacaroonVerifyContext`.
pub const MACAROON_MAX_CUSTOM_CLAIMS: usize = 8;

const MACAROON_WIRE_VERSION: u8 = 1;

/// Constant-time 32-byte equality check (never short-circuits).
pub fn macaroonConstantTimeEq(a: [32]u8, b: [32]u8): bool {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < MACAROON_SIG_LEN) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

fn hmacSha256(key: []const u8, msg: []const u8): [32]u8 {
    let out: [32]u8 = [0; 32];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&out, msg, key);
    return out;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    let lo: usize = 0;
    let hi: usize = s.len;
    while (lo < hi && (s[lo] == 0x20 || s[lo] == 0x09 || s[lo] == 0x0D || s[lo] == 0x0A)) {
        lo += 1;
    }
    while (hi > lo && (s[hi - 1] == 0x20 || s[hi - 1] == 0x09 || s[hi - 1] == 0x0D || s[hi - 1] == 0x0A)) {
        hi -= 1;
    }
    return s[lo..hi];
}

fn parseU64Decimal(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c < 0x30 || c > 0x39) return null;
        const digit: u64 = (c - 0x30) as u64;
        if (acc > (std.math.maxInt(u64) - digit) / 10) return null;
        acc = acc * 10 + digit;
    }
    return acc;
}

fn matchPattern(actual: []const u8, expected: []const u8): bool {
    if (expected.len == 1 && expected[0] == 0x2A) { // "*"
        return actual.len > 0;
    }
    if (expected.len > 1 && expected[expected.len - 1] == 0x2A) { // "prefix*"
        return startsWith(actual, expected[0 .. expected.len - 1]);
    }
    return bytesEq(actual, expected);
}

fn writeU16Le(buf: *[]u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn readU16Le(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn formatU64Into(val_in: u64, out: *[]u8): usize {
    if (val_in == 0) {
        if (out.len == 0) return 0;
        out[0] = 0x30;
        return 1;
    }
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val_in;
    while (v > 0) {
        tmp[n] = (0x30 as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    if (out.len < n) return 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[i] = tmp[n - 1 - i];
    }
    return n;
}

// ============================================================================
// 2. Public Status, Caveat & Verification Context Models
// ============================================================================

/// Status codes for Macaroon minting, attenuation, verification, and binary codec.
pub enum MacaroonStatus {
    ok,
    invalid_root_key,
    invalid_identifier,
    location_too_long,
    identifier_too_long,
    caveat_empty,
    caveat_too_long,
    caveats_full,
    signature_mismatch,
    caveat_expired,
    caveat_scope_denied,
    caveat_method_denied,
    caveat_prefix_denied,
    caveat_assertion_failed,
    caveat_malformed,
    buffer_too_small,
    invalid_format,
    truncated,
}

/// Fixed-capacity inline first-party caveat storage slot.
pub struct MacaroonCaveat {
    bytes: [96]u8 = [0; 96];
    len: usize = 0;

    /// Returns the valid caveat predicate slice.
    fn slice(): []const u8 {
        return _.bytes[0.._.len];
    }
}

/// Custom key-value assertion claim supplied in `MacaroonVerifyContext`.
pub struct MacaroonClaim {
    key: []const u8 = "";
    value: []const u8 = "";
}

/// Runtime request context evaluated against all first-party caveats during `verify`.
pub struct MacaroonVerifyContext {
    now_ts: u64 = 0;
    scope: []const u8 = "";
    method: []const u8 = "";
    path: []const u8 = "";
    claims: [8]MacaroonClaim = [MacaroonClaim{}; 8];
    claim_count: usize = 0;

    /// Constructs a verification context with current timestamp, scope, method, and resource path.
    static fn init(
        now_ts: u64,
        scope: []const u8,
        method: []const u8,
        path: []const u8,
    ): MacaroonVerifyContext {
        return MacaroonVerifyContext{
            now_ts: now_ts,
            scope: scope,
            method: method,
            path: path,
        };
    }

    /// Attaches a custom `key = value` claim to this verification context.
    /// Returns `false` if `key` is empty or the 8-claim inline buffer is full.
    fn addClaim(*_, key: []const u8, value: []const u8): bool {
        if (key.len == 0 || _.claim_count >= MACAROON_MAX_CUSTOM_CLAIMS) return false;
        _.claims[_.claim_count] = MacaroonClaim{
            key: key,
            value: value,
        };
        _.claim_count += 1;
        return true;
    }

    fn findClaim(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.claim_count) : (i += 1) {
            if (bytesEq(_.claims[i].key, key)) {
                return _.claims[i].value;
            }
        }
        return null;
    }
}

/// Detailed verification report returned by `Macaroon.verify`.
pub struct MacaroonVerifyResult {
    status: MacaroonStatus = MacaroonStatus.ok;
    signature_valid: bool = false;
    failed_caveat_index: usize = 0;
    has_failed_caveat: bool = false;
    caveats_checked: usize = 0;

    /// Returns `true` iff both the chained HMAC signature and every caveat passed.
    fn isOk(): bool {
        return _.status == MacaroonStatus.ok && _.signature_valid;
    }
}

// ============================================================================
// 3. Built-In Fail-Closed Caveat Evaluator
// ============================================================================

fn evaluateCaveat(raw_cav: []const u8, ctx: MacaroonVerifyContext): MacaroonStatus {
    const cav: []const u8 = trimAscii(raw_cav);
    if (cav.len == 0) return MacaroonStatus.caveat_malformed;

    // 1. Check `exp <= <ts>` and `exp < <ts>` expiration predicates.
    if (startsWith(cav, "exp")) {
        const rest = trimAscii(cav[3..]);
        if (startsWith(rest, "<=")) {
            const num_s = trimAscii(rest[2..]);
            const limit = parseU64Decimal(num_s) orelse return MacaroonStatus.caveat_malformed;
            return if (ctx.now_ts <= limit) MacaroonStatus.ok else MacaroonStatus.caveat_expired;
        }
        if (startsWith(rest, "<")) {
            const num_s = trimAscii(rest[1..]);
            const limit = parseU64Decimal(num_s) orelse return MacaroonStatus.caveat_malformed;
            return if (ctx.now_ts < limit) MacaroonStatus.ok else MacaroonStatus.caveat_expired;
        }
    }

    // 2. Parse `<key> = <expected>` predicate.
    let eq_idx: usize = cav.len;
    let i: usize = 0;
    while (i < cav.len) : (i += 1) {
        if (cav[i] == 0x3D) { // '='
            eq_idx = i;
            break;
        }
    }
    if (eq_idx == 0 || eq_idx >= cav.len - 1) {
        return MacaroonStatus.caveat_malformed;
    }

    const key = trimAscii(cav[0..eq_idx]);
    const val = trimAscii(cav[eq_idx + 1 ..]);
    if (key.len == 0 || val.len == 0) {
        return MacaroonStatus.caveat_malformed;
    }

    if (bytesEq(key, "scope")) {
        return if (matchPattern(ctx.scope, val)) MacaroonStatus.ok else MacaroonStatus.caveat_scope_denied;
    }
    if (bytesEq(key, "method")) {
        return if (matchPattern(ctx.method, val)) MacaroonStatus.ok else MacaroonStatus.caveat_method_denied;
    }
    if (bytesEq(key, "prefix")) {
        return if (startsWith(ctx.path, val)) MacaroonStatus.ok else MacaroonStatus.caveat_prefix_denied;
    }

    // 3. Custom key-value assertion against context claims (fail-closed if missing).
    const actual = ctx.findClaim(key) orelse return MacaroonStatus.caveat_assertion_failed;
    return if (matchPattern(actual, val)) MacaroonStatus.ok else MacaroonStatus.caveat_assertion_failed;
}

// ============================================================================
// 4. Zero-Alloc Macaroon Capability Token
// ============================================================================

/// Zero-heap-allocation Macaroon capability token with chained HMAC-SHA256 attenuation.
pub struct Macaroon {
    location: [64]u8 = [0; 64];
    location_len: usize = 0;
    identifier: [64]u8 = [0; 64];
    identifier_len: usize = 0;
    caveats: [16]MacaroonCaveat = [MacaroonCaveat{}; 16];
    caveat_count: usize = 0;
    sig: [32]u8 = [0; 32];

    /// Mints a new root Macaroon with `sig_0 = HMAC-SHA256(root_key, identifier)`.
    /// Returns `null` if `root_key` or `identifier` is empty or exceeds inline bounds.
    static fn mint(
        root_key: []const u8,
        location: []const u8,
        identifier: []const u8,
    ): Macaroon? {
        if (root_key.len == 0) return null;
        if (location.len > MACAROON_MAX_LOCATION_LEN) return null;
        if (identifier.len == 0 || identifier.len > MACAROON_MAX_ID_LEN) return null;

        let tok = Macaroon{};
        let i: usize = 0;
        while (i < location.len) : (i += 1) {
            tok.location[i] = location[i];
        }
        tok.location_len = location.len;

        i = 0;
        while (i < identifier.len) : (i += 1) {
            tok.identifier[i] = identifier[i];
        }
        tok.identifier_len = identifier.len;

        tok.sig = hmacSha256(root_key, identifier);
        return tok;
    }

    /// Returns the location hint slice.
    fn locationSlice(): []const u8 {
        return _.location[0.._.location_len];
    }

    /// Returns the token identifier slice.
    fn identifierSlice(): []const u8 {
        return _.identifier[0.._.identifier_len];
    }

    /// Returns the number of attached first-party caveats.
    fn caveatCount(): usize {
        return _.caveat_count;
    }

    /// Returns caveat `idx` as a read-only slice, or `null` if out of range.
    fn caveatAt(idx: usize): ([]const u8)? {
        if (idx >= _.caveat_count) return null;
        return _.caveats[idx].slice();
    }

    /// Returns a copy of the current 32-byte chained HMAC-SHA256 signature.
    fn signature(): [32]u8 {
        return _.sig;
    }

    /// Offline one-way capability attenuation (requires NO root key):
    /// appends `caveat` and replaces `sig = HMAC-SHA256(sig, caveat)`.
    fn addCaveat(*_, caveat: []const u8): MacaroonStatus {
        if (caveat.len == 0) return MacaroonStatus.caveat_empty;
        if (caveat.len > MACAROON_MAX_CAVEAT_LEN) return MacaroonStatus.caveat_too_long;
        if (_.caveat_count >= MACAROON_MAX_CAVEATS) return MacaroonStatus.caveats_full;

        let slot = MacaroonCaveat{};
        let i: usize = 0;
        while (i < caveat.len) : (i += 1) {
            slot.bytes[i] = caveat[i];
        }
        slot.len = caveat.len;

        _.caveats[_.caveat_count] = slot;
        _.caveat_count += 1;
        _.sig = hmacSha256(_.sig[0..], caveat);
        return MacaroonStatus.ok;
    }

    /// Convenience helper: appends `"exp < <exp_ts>"` expiration caveat.
    fn addExpCaveat(*_, exp_ts: u64): MacaroonStatus {
        let buf: [40]u8 = [0; 40];
        const prefix: []const u8 = "exp < ";
        let i: usize = 0;
        while (i < prefix.len) : (i += 1) {
            buf[i] = prefix[i];
        }
        let num_slice: []u8 = buf[prefix.len..];
        const digits: usize = formatU64Into(exp_ts, *num_slice);
        if (digits == 0) return MacaroonStatus.caveat_malformed;
        return _.addCaveat(buf[0 .. prefix.len + digits]);
    }

    /// Convenience helper: appends `"scope = <scope>"` caveat.
    fn addScopeCaveat(*_, scope: []const u8): MacaroonStatus {
        if (scope.len == 0) return MacaroonStatus.caveat_empty;
        const prefix: []const u8 = "scope = ";
        if (prefix.len + scope.len > MACAROON_MAX_CAVEAT_LEN) return MacaroonStatus.caveat_too_long;
        let buf: [96]u8 = [0; 96];
        let i: usize = 0;
        while (i < prefix.len) : (i += 1) {
            buf[i] = prefix[i];
        }
        let j: usize = 0;
        while (j < scope.len) : (j += 1) {
            buf[prefix.len + j] = scope[j];
        }
        return _.addCaveat(buf[0 .. prefix.len + scope.len]);
    }

    /// Convenience helper: appends `"method = <method>"` caveat.
    fn addMethodCaveat(*_, method: []const u8): MacaroonStatus {
        if (method.len == 0) return MacaroonStatus.caveat_empty;
        const prefix: []const u8 = "method = ";
        if (prefix.len + method.len > MACAROON_MAX_CAVEAT_LEN) return MacaroonStatus.caveat_too_long;
        let buf: [96]u8 = [0; 96];
        let i: usize = 0;
        while (i < prefix.len) : (i += 1) {
            buf[i] = prefix[i];
        }
        let j: usize = 0;
        while (j < method.len) : (j += 1) {
            buf[prefix.len + j] = method[j];
        }
        return _.addCaveat(buf[0 .. prefix.len + method.len]);
    }

    /// Convenience helper: appends `"prefix = <prefix>"` path-prefix caveat.
    fn addPrefixCaveat(*_, path_prefix: []const u8): MacaroonStatus {
        if (path_prefix.len == 0) return MacaroonStatus.caveat_empty;
        const prefix: []const u8 = "prefix = ";
        if (prefix.len + path_prefix.len > MACAROON_MAX_CAVEAT_LEN) return MacaroonStatus.caveat_too_long;
        let buf: [96]u8 = [0; 96];
        let i: usize = 0;
        while (i < prefix.len) : (i += 1) {
            buf[i] = prefix[i];
        }
        let j: usize = 0;
        while (j < path_prefix.len) : (j += 1) {
            buf[prefix.len + j] = path_prefix[j];
        }
        return _.addCaveat(buf[0 .. prefix.len + path_prefix.len]);
    }

    /// Recomputes the full HMAC-SHA256 chain from `root_key` and verifies against `_.sig`
    /// in constant time.
    fn verifySignature(root_key: []const u8): bool {
        if (root_key.len == 0 || _.identifier_len == 0) return false;
        let computed: [32]u8 = hmacSha256(root_key, _.identifier[0.._.identifier_len]);
        let i: usize = 0;
        while (i < _.caveat_count) : (i += 1) {
            computed = hmacSha256(computed[0..], _.caveats[i].slice());
        }
        return macaroonConstantTimeEq(computed, _.sig);
    }

    /// Verifies both the chained HMAC signature (constant-time) and every first-party caveat
    /// against `ctx`. Fails closed on any signature mismatch, expired timestamp, denied
    /// scope/method/prefix, missing claim, or malformed caveat, reporting exact `failed_caveat_index`.
    fn verify(root_key: []const u8, ctx: MacaroonVerifyContext): MacaroonVerifyResult {
        if (!_.verifySignature(root_key)) {
            return MacaroonVerifyResult{
                status: MacaroonStatus.signature_mismatch,
                signature_valid: false,
                failed_caveat_index: 0,
                has_failed_caveat: false,
                caveats_checked: 0,
            };
        }

        let i: usize = 0;
        while (i < _.caveat_count) : (i += 1) {
            const st = evaluateCaveat(_.caveats[i].slice(), ctx);
            if (st != MacaroonStatus.ok) {
                return MacaroonVerifyResult{
                    status: st,
                    signature_valid: true,
                    failed_caveat_index: i,
                    has_failed_caveat: true,
                    caveats_checked: i + 1,
                };
            }
        }

        return MacaroonVerifyResult{
            status: MacaroonStatus.ok,
            signature_valid: true,
            failed_caveat_index: 0,
            has_failed_caveat: false,
            caveats_checked: _.caveat_count,
        };
    }

    /// Returns exact binary wire byte length needed by `encodeBinary`.
    fn encodedSize(): usize {
        // 4B magic + 1B ver + 1B cav_count + 2B loc_len + loc + 2B id_len + id + caveats + 32B sig
        let total: usize = (4 as usize) + 1 + 1 + 2 + _.location_len + 2 + _.identifier_len + MACAROON_SIG_LEN;
        let i: usize = 0;
        while (i < _.caveat_count) : (i += 1) {
            total += (2 as usize) + _.caveats[i].len;
        }
        return total;
    }

    /// Encodes this Macaroon into zero-alloc binary wire format (`MAC1`).
    /// Returns total bytes written, or `null` if `out_buf` is too small.
    fn encodeBinary(out_buf: *[]u8): usize? {
        const needed: usize = _.encodedSize();
        if (out_buf.len < needed) return null;

        // Magic "MAC1" + version + caveat_count
        out_buf[0] = 0x4D;
        out_buf[1] = 0x41;
        out_buf[2] = 0x43;
        out_buf[3] = 0x31;
        out_buf[4] = MACAROON_WIRE_VERSION;
        out_buf[5] = _.caveat_count as u8;

        let pos: usize = 6;
        writeU16Le(out_buf, pos, _.location_len as u16);
        pos += 2;
        let i: usize = 0;
        while (i < _.location_len) : (i += 1) {
            out_buf[pos + i] = _.location[i];
        }
        pos += _.location_len;

        writeU16Le(out_buf, pos, _.identifier_len as u16);
        pos += 2;
        i = 0;
        while (i < _.identifier_len) : (i += 1) {
            out_buf[pos + i] = _.identifier[i];
        }
        pos += _.identifier_len;

        let c: usize = 0;
        while (c < _.caveat_count) : (c += 1) {
            const cav_len: usize = _.caveats[c].len;
            writeU16Le(out_buf, pos, cav_len as u16);
            pos += 2;
            let k: usize = 0;
            while (k < cav_len) : (k += 1) {
                out_buf[pos + k] = _.caveats[c].bytes[k];
            }
            pos += cav_len;
        }

        i = 0;
        while (i < MACAROON_SIG_LEN) : (i += 1) {
            out_buf[pos + i] = _.sig[i];
        }
        pos += MACAROON_SIG_LEN;
        return pos;
    }

    /// Decodes a Macaroon from binary wire format (`MAC1`).
    /// Strictly rejects bad magic/version, out-of-bounds lengths, truncation, or trailing junk bytes.
    static fn decodeBinary(data: []const u8): Macaroon? {
        // Minimum header (6B) + loc_len (2B) + id_len (2B) + 1B id + 32B sig = 43B
        if (data.len < 43) return null;
        if (data[0] != 0x4D || data[1] != 0x41 || data[2] != 0x43 || data[3] != 0x31) return null;
        if (data[4] != MACAROON_WIRE_VERSION) return null;

        const cav_count: usize = data[5] as usize;
        if (cav_count > MACAROON_MAX_CAVEATS) return null;

        let tok = Macaroon{};
        let pos: usize = 6;

        if (pos + 2 > data.len) return null;
        const loc_len: usize = readU16Le(data, pos) as usize;
        pos += 2;
        if (loc_len > MACAROON_MAX_LOCATION_LEN || loc_len > data.len - pos) return null;
        let i: usize = 0;
        while (i < loc_len) : (i += 1) {
            tok.location[i] = data[pos + i];
        }
        tok.location_len = loc_len;
        pos += loc_len;

        if (pos + 2 > data.len) return null;
        const id_len: usize = readU16Le(data, pos) as usize;
        pos += 2;
        if (id_len == 0 || id_len > MACAROON_MAX_ID_LEN || id_len > data.len - pos) return null;
        i = 0;
        while (i < id_len) : (i += 1) {
            tok.identifier[i] = data[pos + i];
        }
        tok.identifier_len = id_len;
        pos += id_len;

        let c: usize = 0;
        while (c < cav_count) : (c += 1) {
            if (pos + 2 > data.len) return null;
            const cav_len: usize = readU16Le(data, pos) as usize;
            pos += 2;
            if (cav_len == 0 || cav_len > MACAROON_MAX_CAVEAT_LEN || cav_len > data.len - pos) return null;
            let slot = MacaroonCaveat{};
            let k: usize = 0;
            while (k < cav_len) : (k += 1) {
                slot.bytes[k] = data[pos + k];
            }
            slot.len = cav_len;
            tok.caveats[c] = slot;
            pos += cav_len;
        }
        tok.caveat_count = cav_count;

        if (data.len - pos != MACAROON_SIG_LEN) return null;
        i = 0;
        while (i < MACAROON_SIG_LEN) : (i += 1) {
            tok.sig[i] = data[pos + i];
        }
        return tok;
    }
}
