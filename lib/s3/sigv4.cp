// Source: bun/src/s3_signing/credentials.cp -> cp/lib/s3_signing/s3_signing.cp (Pure CP AWS SigV4 S3 Request Signing & Presigned URL Engine)

const std = @import("std");

/// Unified error set for AWS Signature Version 4 S3 request signing and presigned URL generation.
pub err S3SignError {
    InvalidCredentials,
    InvalidTimestamp,
    InvalidRegion,
    InvalidService,
    InvalidMethod,
    InvalidPath,
    InvalidExpiration,
    OutOfMemory,
}

/// AWS/S3 credentials for SigV4 signing.
pub struct S3Credentials {
    access_key_id: []const u8 = "",
    secret_access_key: []const u8 = "",
    session_token: ([]const u8)? = null,
}

/// HTTP header key/value pair for SigV4 canonicalization.
pub struct SigV4Header {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Query parameter key/value pair for SigV4 canonicalization.
pub struct SigV4QueryParam {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Complete AWS SigV4 signed request output.
///
/// Caller owns heap-allocated buffers (`authorization`, `canonical_request`, `string_to_sign`)
/// and must release them via `deinit(allocator)` using the same allocator passed to `signRequest`.
pub struct SigV4SignedResult {
    authorization: []u8 = [],
    amz_date: [16]u8 = [0; 16],
    payload_sha256: [64]u8 = [0; 64],
    canonical_request: []u8 = [],
    string_to_sign: []u8 = [],
    signature: [64]u8 = [0; 64],

    /// Frees all heap-allocated buffers owned by this result.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.authorization.len > 0) {
            allocator.free(_.authorization);
        }
        if (_.canonical_request.len > 0) {
            allocator.free(_.canonical_request);
        }
        if (_.string_to_sign.len > 0) {
            allocator.free(_.string_to_sign);
        }
        _.authorization = [];
        _.canonical_request = [];
        _.string_to_sign = [];
    }
}

const HEX_LOWER: []const u8 = "0123456789abcdef";
const HEX_UPPER: []const u8 = "0123456789ABCDEF";
const MAX_PRESIGN_EXPIRES_SEC: u32 = 604800;

fn isUnreservedUriChar(c: u8): bool {
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return c == '-' || c == '_' || c == '.' || c == '~';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn containsCtlOrNewline(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '\r' || c == '\n' || c == 0) return true;
        i += 1;
    }
    return false;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toAsciiLower(a[i]) != toAsciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn lessThanBytes(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn bytesToLowerHex32(digest: [32]u8): [64]u8 {
    let out: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 32) {
        const b = digest[i];
        out[i * 2] = HEX_LOWER[(b >> 4) & 0x0F];
        out[i * 2 + 1] = HEX_LOWER[b & 0x0F];
        i += 1;
    }
    return out;
}

fn sha256Hex(data: []const u8): [64]u8 {
    let digest: [32]u8 = [0; 32];
    std.crypto.hash.sha2.Sha256.hash(data, &digest, .{});
    return bytesToLowerHex32(digest);
}

fn validateAmzDate(ts: []const u8): S3SignError!void {
    if (ts.len != 16) return S3SignError.InvalidTimestamp;
    if (ts[8] != 'T' || ts[15] != 'Z') return S3SignError.InvalidTimestamp;

    let i: usize = 0;
    while (i < 15) {
        if (i != 8 && !isDigit(ts[i])) {
            return S3SignError.InvalidTimestamp;
        }
        i += 1;
    }

    const month = (@as(u32, ts[4] - '0') * 10) + @as(u32, ts[5] - '0');
    const day = (@as(u32, ts[6] - '0') * 10) + @as(u32, ts[7] - '0');
    const hour = (@as(u32, ts[9] - '0') * 10) + @as(u32, ts[10] - '0');
    const minute = (@as(u32, ts[11] - '0') * 10) + @as(u32, ts[12] - '0');
    const second = (@as(u32, ts[13] - '0') * 10) + @as(u32, ts[14] - '0');

    if (month < 1 || month > 12) return S3SignError.InvalidTimestamp;
    if (day < 1 || day > 31) return S3SignError.InvalidTimestamp;
    if (hour > 23 || minute > 59 || second > 59) return S3SignError.InvalidTimestamp;
}

fn validateCredentials(creds: S3Credentials): S3SignError!void {
    if (creds.access_key_id.len == 0 || creds.secret_access_key.len == 0) {
        return S3SignError.InvalidCredentials;
    }
    if (containsCtlOrNewline(creds.access_key_id) || containsCtlOrNewline(creds.secret_access_key)) {
        return S3SignError.InvalidCredentials;
    }
    if (tok := creds.session_token) {
        if (containsCtlOrNewline(tok)) {
            return S3SignError.InvalidCredentials;
        }
    }
}

fn validateCommonInputs(
    method: []const u8,
    canonical_uri: []const u8,
    region: []const u8,
    service: []const u8,
    amz_date_iso8601: []const u8,
    creds: S3Credentials,
): S3SignError!void {
    validateCredentials(creds)?;
    validateAmzDate(amz_date_iso8601)?;

    if (region.len == 0 || containsCtlOrNewline(region)) {
        return S3SignError.InvalidRegion;
    }
    if (service.len == 0 || containsCtlOrNewline(service)) {
        return S3SignError.InvalidService;
    }
    if (method.len == 0 || containsCtlOrNewline(method)) {
        return S3SignError.InvalidMethod;
    }
    if (canonical_uri.len == 0 || canonical_uri[0] != '/' || containsCtlOrNewline(canonical_uri)) {
        return S3SignError.InvalidPath;
    }
}

/// Strictly encodes a URI component or path according to AWS SigV4 RFC 3986 rules.
///
/// Only unreserved characters (`A-Z`, `a-z`, `0-9`, `-`, `_`, `.`, `~`) are preserved;
/// `/` is preserved when `encode_slash == false` and percent-encoded as `%2F` when `true`.
/// Spaces are encoded as `%20` (never `+`). All hex digits are uppercase.
pub fn awsUriEncode(allocator: std.mem.Allocator, input: []const u8, encode_slash: bool): S3SignError![]u8 {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (isUnreservedUriChar(c) || (!encode_slash && c == '/')) {
            out_len += 1;
        } else {
            out_len += 3;
        }
        i += 1;
    }

    let buf = allocator.alloc(u8, out_len) catch return S3SignError.OutOfMemory;
    let pos: usize = 0;
    i = 0;
    while (i < input.len) {
        const c = input[i];
        if (isUnreservedUriChar(c) || (!encode_slash && c == '/')) {
            buf[pos] = c;
            pos += 1;
        } else {
            buf[pos] = '%';
            buf[pos + 1] = HEX_UPPER[(c >> 4) & 0x0F];
            buf[pos + 2] = HEX_UPPER[c & 0x0F];
            pos += 3;
        }
        i += 1;
    }
    return buf;
}

/// Derives the 32-byte AWS SigV4 signing key via 4-stage chained HMAC-SHA256:
/// `kDate -> kRegion -> kService -> kSigning`.
pub fn deriveSigningKey(
    secret_key: []const u8,
    date_yyyymmdd: []const u8,
    region: []const u8,
    service: []const u8,
): [32]u8 {
    let k_date: [32]u8 = [0; 32];
    if (secret_key.len <= 60) {
        let key_buf: [64]u8 = [0; 64];
        key_buf[0] = 'A';
        key_buf[1] = 'W';
        key_buf[2] = 'S';
        key_buf[3] = '4';
        if (secret_key.len > 0) {
            @memcpy(key_buf[4 .. 4 + secret_key.len], secret_key);
        }
        std.crypto.auth.hmac.sha2.HmacSha256.create(&k_date, date_yyyymmdd, key_buf[0 .. 4 + secret_key.len]);
    } else {
        // For keys longer than the 64-byte SHA-256 block size, HMAC hashes the key first.
        let hasher = std.crypto.hash.sha2.Sha256.init(.{});
        hasher.update("AWS4");
        hasher.update(secret_key);
        let hashed_key: [32]u8 = [0; 32];
        hasher.final(&hashed_key);
        std.crypto.auth.hmac.sha2.HmacSha256.create(&k_date, date_yyyymmdd, &hashed_key);
    }

    let k_region: [32]u8 = [0; 32];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&k_region, region, &k_date);

    let k_service: [32]u8 = [0; 32];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&k_service, service, &k_region);

    let k_signing: [32]u8 = [0; 32];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&k_signing, "aws4_request", &k_service);
    return k_signing;
}

struct OwnedPair {
    key: []u8 = [],
    val: []u8 = [],
}

fn freeOwnedPairs(allocator: std.mem.Allocator, pairs: []OwnedPair) {
    let i: usize = 0;
    while (i < pairs.len) {
        if (pairs[i].key.len > 0) allocator.free(pairs[i].key);
        if (pairs[i].val.len > 0) allocator.free(pairs[i].val);
        i += 1;
    }
    if (pairs.len > 0) {
        allocator.free(pairs);
    }
}

fn buildCanonicalQueryString(
    allocator: std.mem.Allocator,
    params: []const SigV4QueryParam,
): S3SignError![]u8 {
    if (params.len == 0) {
        return allocator.alloc(u8, 0) catch return S3SignError.OutOfMemory;
    }

    let pairs = allocator.alloc(OwnedPair, params.len) catch return S3SignError.OutOfMemory;
    let init_idx: usize = 0;
    while (init_idx < params.len) {
        pairs[init_idx] = OwnedPair{};
        init_idx += 1;
    }
    errdefer freeOwnedPairs(allocator, pairs);

    let i: usize = 0;
    while (i < params.len) {
        pairs[i].key = awsUriEncode(allocator, params[i].name, true)?;
        pairs[i].val = awsUriEncode(allocator, params[i].value, true)?;
        i += 1;
    }

    // Sort by encoded key ascending, then encoded value ascending.
    let a: usize = 1;
    while (a < pairs.len) {
        const cur = pairs[a];
        let b = a;
        while (b > 0) {
            const prev = pairs[b - 1];
            const should_swap = if (std.mem.eql(u8, prev.key, cur.key))
                lessThanBytes(cur.val, prev.val)
            else
                lessThanBytes(cur.key, prev.key);
            if (!should_swap) break;
            pairs[b] = prev;
            b -= 1;
        }
        pairs[b] = cur;
        a += 1;
    }

    let total_len: usize = params.len - 1; // '&' separators
    i = 0;
    while (i < pairs.len) {
        total_len += pairs[i].key.len + 1 + pairs[i].val.len;
        i += 1;
    }

    let out = allocator.alloc(u8, total_len) catch return S3SignError.OutOfMemory;
    let pos: usize = 0;
    i = 0;
    while (i < pairs.len) {
        if (i > 0) {
            out[pos] = '&';
            pos += 1;
        }
        if (pairs[i].key.len > 0) {
            @memcpy(out[pos .. pos + pairs[i].key.len], pairs[i].key);
            pos += pairs[i].key.len;
        }
        out[pos] = '=';
        pos += 1;
        if (pairs[i].val.len > 0) {
            @memcpy(out[pos .. pos + pairs[i].val.len], pairs[i].val);
            pos += pairs[i].val.len;
        }
        i += 1;
    }

    freeOwnedPairs(allocator, pairs);
    return out;
}

fn normalizeHeaderValue(allocator: std.mem.Allocator, raw: []const u8): S3SignError![]u8 {
    let start: usize = 0;
    while (start < raw.len && (raw[start] == ' ' || raw[start] == '\t')) {
        start += 1;
    }
    let end = raw.len;
    while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t')) {
        end -= 1;
    }

    // Collapse sequential spaces/tabs to a single space per AWS SigV4 Trimall.
    let out_len: usize = 0;
    let in_space = false;
    let i = start;
    while (i < end) {
        const c = raw[i];
        if (c == ' ' || c == '\t') {
            if (!in_space) {
                out_len += 1;
                in_space = true;
            }
        } else {
            out_len += 1;
            in_space = false;
        }
        i += 1;
    }

    let buf = allocator.alloc(u8, out_len) catch return S3SignError.OutOfMemory;
    let pos: usize = 0;
    in_space = false;
    i = start;
    while (i < end) {
        const c = raw[i];
        if (c == ' ' || c == '\t') {
            if (!in_space) {
                buf[pos] = ' ';
                pos += 1;
                in_space = true;
            }
        } else {
            buf[pos] = c;
            pos += 1;
            in_space = false;
        }
        i += 1;
    }
    return buf;
}

fn allocLowercase(allocator: std.mem.Allocator, s: []const u8): S3SignError![]u8 {
    let buf = allocator.alloc(u8, s.len) catch return S3SignError.OutOfMemory;
    let i: usize = 0;
    while (i < s.len) {
        buf[i] = toAsciiLower(s[i]);
        i += 1;
    }
    return buf;
}

struct CanonicalHeadersResult {
    canonical_headers: []u8 = [],
    signed_headers: []u8 = [],
}

fn buildCanonicalHeaders(
    allocator: std.mem.Allocator,
    headers: []const SigV4Header,
    creds: S3Credentials,
    amz_date_iso8601: []const u8,
): S3SignError!CanonicalHeadersResult {
    let has_amz_date = false;
    let has_security_token = false;
    let i: usize = 0;
    while (i < headers.len) {
        if (eqlIgnoreCase(headers[i].name, "x-amz-date")) has_amz_date = true;
        if (eqlIgnoreCase(headers[i].name, "x-amz-security-token")) has_security_token = true;
        i += 1;
    }

    let extra_count: usize = 0;
    if (!has_amz_date) extra_count += 1;
    let has_token_val = false;
    if (t := creds.session_token) {
        if (t.len > 0) has_token_val = true;
    }
    if (has_token_val && !has_security_token) extra_count += 1;

    const raw_count = headers.len + extra_count;
    let pairs = allocator.alloc(OwnedPair, raw_count) catch return S3SignError.OutOfMemory;
    i = 0;
    while (i < raw_count) {
        pairs[i] = OwnedPair{};
        i += 1;
    }
    errdefer freeOwnedPairs(allocator, pairs);

    i = 0;
    while (i < headers.len) {
        pairs[i].key = allocLowercase(allocator, headers[i].name)?;
        pairs[i].val = normalizeHeaderValue(allocator, headers[i].value)?;
        i += 1;
    }

    let idx = headers.len;
    if (!has_amz_date) {
        pairs[idx].key = allocLowercase(allocator, "x-amz-date")?;
        pairs[idx].val = normalizeHeaderValue(allocator, amz_date_iso8601)?;
        idx += 1;
    }
    if (has_token_val && !has_security_token) {
        if (tok := creds.session_token) {
            pairs[idx].key = allocLowercase(allocator, "x-amz-security-token")?;
            pairs[idx].val = normalizeHeaderValue(allocator, tok)?;
            idx += 1;
        }
    }

    // Sort by lowercase header name ascending.
    let a: usize = 1;
    while (a < raw_count) {
        const cur = pairs[a];
        let b = a;
        while (b > 0 && lessThanBytes(cur.key, pairs[b - 1].key)) {
            pairs[b] = pairs[b - 1];
            b -= 1;
        }
        pairs[b] = cur;
        a += 1;
    }

    // Merge duplicate header names with ','.
    let unique_count: usize = 0;
    i = 0;
    while (i < raw_count) {
        if (unique_count > 0 && std.mem.eql(u8, pairs[unique_count - 1].key, pairs[i].key)) {
            const prev_val = pairs[unique_count - 1].val;
            const add_val = pairs[i].val;
            let merged = allocator.alloc(u8, prev_val.len + 1 + add_val.len) catch return S3SignError.OutOfMemory;
            @memcpy(merged[0..prev_val.len], prev_val);
            merged[prev_val.len] = ',';
            @memcpy(merged[prev_val.len + 1 ..], add_val);
            allocator.free(prev_val);
            pairs[unique_count - 1].val = merged;
            allocator.free(pairs[i].key);
            allocator.free(pairs[i].val);
            pairs[i].key = [];
            pairs[i].val = [];
        } else {
            if (unique_count != i) {
                pairs[unique_count] = pairs[i];
                pairs[i].key = [];
                pairs[i].val = [];
            }
            unique_count += 1;
        }
        i += 1;
    }

    let canon_len: usize = 0;
    let signed_len: usize = if (unique_count > 0) unique_count - 1 else 0;
    i = 0;
    while (i < unique_count) {
        canon_len += pairs[i].key.len + 1 + pairs[i].val.len + 1; // name:val\n
        signed_len += pairs[i].key.len;
        i += 1;
    }

    let canon_buf = allocator.alloc(u8, canon_len) catch return S3SignError.OutOfMemory;
    errdefer allocator.free(canon_buf);
    let signed_buf = allocator.alloc(u8, signed_len) catch return S3SignError.OutOfMemory;

    let c_pos: usize = 0;
    let s_pos: usize = 0;
    i = 0;
    while (i < unique_count) {
        const k = pairs[i].key;
        const v = pairs[i].val;

        @memcpy(canon_buf[c_pos .. c_pos + k.len], k);
        c_pos += k.len;
        canon_buf[c_pos] = ':';
        c_pos += 1;
        @memcpy(canon_buf[c_pos .. c_pos + v.len], v);
        c_pos += v.len;
        canon_buf[c_pos] = '\n';
        c_pos += 1;

        if (i > 0) {
            signed_buf[s_pos] = ';';
            s_pos += 1;
        }
        @memcpy(signed_buf[s_pos .. s_pos + k.len], k);
        s_pos += k.len;

        i += 1;
    }

    freeOwnedPairs(allocator, pairs);
    return CanonicalHeadersResult{
        .canonical_headers = canon_buf,
        .signed_headers = signed_buf,
    };
}

/// Signs an HTTP/S3 request using AWS Signature Version 4.
///
/// Automatically canonicalizes URI, query parameters, and headers (injecting `x-amz-date`
/// and optional `x-amz-security-token` when not already present), computes `CanonicalRequest`,
/// `StringToSign`, 64-char hex `signature`, and full `Authorization` header.
pub fn signRequest(
    allocator: std.mem.Allocator,
    method: []const u8,
    canonical_uri: []const u8,
    query_params: []const SigV4QueryParam,
    headers: []const SigV4Header,
    payload_or_unsigned: []const u8,
    creds: S3Credentials,
    region: []const u8,
    service: []const u8,
    amz_date_iso8601: []const u8,
): S3SignError!SigV4SignedResult {
    validateCommonInputs(method, canonical_uri, region, service, amz_date_iso8601, creds)?;

    const is_unsigned = std.mem.eql(u8, payload_or_unsigned, "UNSIGNED-PAYLOAD");
    const payload_hex = if (is_unsigned) sha256Hex("") else sha256Hex(payload_or_unsigned);
    const hashed_payload_str: []const u8 = if (is_unsigned) "UNSIGNED-PAYLOAD" else &payload_hex;

    const encoded_uri = awsUriEncode(allocator, canonical_uri, false)?;
    defer allocator.free(encoded_uri);

    const canon_query = buildCanonicalQueryString(allocator, query_params)?;
    defer allocator.free(canon_query);

    const hdrs = buildCanonicalHeaders(allocator, headers, creds, amz_date_iso8601)?;
    defer allocator.free(hdrs.canonical_headers);
    defer allocator.free(hdrs.signed_headers);

    // CanonicalRequest: method\nuri\nquery\ncanon_headers\nsigned_headers\nhashed_payload
    const canon_req = std.fmt.allocPrint(
        allocator,
        "{s}\n{s}\n{s}\n{s}\n{s}\n{s}",
        .{ method, encoded_uri, canon_query, hdrs.canonical_headers, hdrs.signed_headers, hashed_payload_str },
    ) catch return S3SignError.OutOfMemory;
    errdefer allocator.free(canon_req);

    const canon_req_hex = sha256Hex(canon_req);
    const date_yyyymmdd = amz_date_iso8601[0..8];

    const string_to_sign = std.fmt.allocPrint(
        allocator,
        "AWS4-HMAC-SHA256\n{s}\n{s}/{s}/{s}/aws4_request\n{s}",
        .{ amz_date_iso8601, date_yyyymmdd, region, service, &canon_req_hex },
    ) catch return S3SignError.OutOfMemory;
    errdefer allocator.free(string_to_sign);

    const signing_key = deriveSigningKey(creds.secret_access_key, date_yyyymmdd, region, service);
    let sig_digest: [32]u8 = [0; 32];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&sig_digest, string_to_sign, &signing_key);
    const sig_hex = bytesToLowerHex32(sig_digest);

    const authorization = std.fmt.allocPrint(
        allocator,
        "AWS4-HMAC-SHA256 Credential={s}/{s}/{s}/{s}/aws4_request, SignedHeaders={s}, Signature={s}",
        .{ creds.access_key_id, date_yyyymmdd, region, service, hdrs.signed_headers, &sig_hex },
    ) catch return S3SignError.OutOfMemory;

    let amz_date_arr: [16]u8 = [0; 16];
    @memcpy(&amz_date_arr, amz_date_iso8601[0..16]);

    return SigV4SignedResult{
        .authorization = authorization,
        .amz_date = amz_date_arr,
        .payload_sha256 = payload_hex,
        .canonical_request = canon_req,
        .string_to_sign = string_to_sign,
        .signature = sig_hex,
    };
}

/// Generates an AWS SigV4 presigned URL using `UNSIGNED-PAYLOAD`.
///
/// Enforces `1 <= expires_sec <= 604800` (7-day AWS SigV4 maximum), includes
/// `X-Amz-Algorithm`, `X-Amz-Credential`, `X-Amz-Date`, `X-Amz-Expires`,
/// `X-Amz-SignedHeaders`, optional `X-Amz-Security-Token`, and `extra_query_params`.
pub fn presignUrl(
    allocator: std.mem.Allocator,
    scheme: []const u8,
    host: []const u8,
    canonical_uri: []const u8,
    method: []const u8,
    expires_sec: u32,
    extra_query_params: []const SigV4QueryParam,
    creds: S3Credentials,
    region: []const u8,
    service: []const u8,
    amz_date_iso8601: []const u8,
): S3SignError![]u8 {
    validateCommonInputs(method, canonical_uri, region, service, amz_date_iso8601, creds)?;
    if (scheme.len == 0 || containsCtlOrNewline(scheme)) {
        return S3SignError.InvalidPath;
    }
    if (host.len == 0 || containsCtlOrNewline(host)) {
        return S3SignError.InvalidPath;
    }
    if (expires_sec == 0 || expires_sec > MAX_PRESIGN_EXPIRES_SEC) {
        return S3SignError.InvalidExpiration;
    }

    const date_yyyymmdd = amz_date_iso8601[0..8];
    const cred_scope_val = std.fmt.allocPrint(
        allocator,
        "{s}/{s}/{s}/{s}/aws4_request",
        .{ creds.access_key_id, date_yyyymmdd, region, service },
    ) catch return S3SignError.OutOfMemory;
    defer allocator.free(cred_scope_val);

    let expires_buf: [16]u8 = [0; 16];
    const expires_str = std.fmt.bufPrint(&expires_buf, "{d}", .{expires_sec}) catch return S3SignError.InvalidExpiration;

    let has_token = false;
    if (t := creds.session_token) {
        if (t.len > 0) has_token = true;
    }
    const base_count: usize = if (has_token) 6 else 5;
    const total_params = base_count + extra_query_params.len;

    let params = allocator.alloc(SigV4QueryParam, total_params) catch return S3SignError.OutOfMemory;
    defer allocator.free(params);

    params[0] = SigV4QueryParam{ .name = "X-Amz-Algorithm", .value = "AWS4-HMAC-SHA256" };
    params[1] = SigV4QueryParam{ .name = "X-Amz-Credential", .value = cred_scope_val };
    params[2] = SigV4QueryParam{ .name = "X-Amz-Date", .value = amz_date_iso8601 };
    params[3] = SigV4QueryParam{ .name = "X-Amz-Expires", .value = expires_str };
    params[4] = SigV4QueryParam{ .name = "X-Amz-SignedHeaders", .value = "host" };

    let idx: usize = 5;
    if (has_token) {
        if (tok := creds.session_token) {
            params[idx] = SigV4QueryParam{ .name = "X-Amz-Security-Token", .value = tok };
            idx += 1;
        }
    }

    let i: usize = 0;
    while (i < extra_query_params.len) {
        params[idx + i] = extra_query_params[i];
        i += 1;
    }

    const encoded_uri = awsUriEncode(allocator, canonical_uri, false)?;
    defer allocator.free(encoded_uri);

    const canon_query = buildCanonicalQueryString(allocator, params)?;
    defer allocator.free(canon_query);

    const canon_req = std.fmt.allocPrint(
        allocator,
        "{s}\n{s}\n{s}\nhost:{s}\n\nhost\nUNSIGNED-PAYLOAD",
        .{ method, encoded_uri, canon_query, host },
    ) catch return S3SignError.OutOfMemory;
    defer allocator.free(canon_req);

    const canon_req_hex = sha256Hex(canon_req);
    const string_to_sign = std.fmt.allocPrint(
        allocator,
        "AWS4-HMAC-SHA256\n{s}\n{s}/{s}/{s}/aws4_request\n{s}",
        .{ amz_date_iso8601, date_yyyymmdd, region, service, &canon_req_hex },
    ) catch return S3SignError.OutOfMemory;
    defer allocator.free(string_to_sign);

    const signing_key = deriveSigningKey(creds.secret_access_key, date_yyyymmdd, region, service);
    let sig_digest: [32]u8 = [0; 32];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&sig_digest, string_to_sign, &signing_key);
    const sig_hex = bytesToLowerHex32(sig_digest);

    return std.fmt.allocPrint(
        allocator,
        "{s}://{s}{s}?{s}&X-Amz-Signature={s}",
        .{ scheme, host, encoded_uri, canon_query, &sig_hex },
    ) catch return S3SignError.OutOfMemory;
}
