// Source: cp/lib/oauth2/types.cp (OAuth 2.0 / PKCE / OIDC Core Types, Security & Percent-Encoding)

const std = @import("std");

/// Unified error set for `cap:oauth2` OAuth 2.0 / PKCE / OIDC protocol engine.
pub err OAuth2Error {
    InvalidVerifier,
    InvalidChallenge,
    InvalidConfig,
    InvalidState,
    StateMismatch,
    InvalidCallback,
    CallbackError,
    InvalidTokenResponse,
    OAuthErrorResponse,
    InvalidParameter,
    OutOfMemory,
}

/// PKCE code_challenge_method defined in RFC 7636 Section 4.2.
pub enum PkceMethod: u8 {
    S256,
    Plain,

    /// Returns the wire protocol string ("S256" or "plain").
    pub fn asString(): []const u8 {
        return switch (_) {
            .S256 => "S256",
            .Plain => "plain",
        };
    }
}

/// Owned PKCE code_verifier / code_challenge pair.
/// Caller must call `deinit(alloc)` with the same allocator used to create it.
pub struct PkcePair {
    code_verifier: []u8 = [],
    code_challenge: []u8 = [],
    method: PkceMethod = .S256,

    /// Frees owned `code_verifier` and `code_challenge` buffers.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.code_verifier.len > 0) {
            alloc.free(_.code_verifier);
            _.code_verifier = [];
        }
        if (_.code_challenge.len > 0) {
            alloc.free(_.code_challenge);
            _.code_challenge = [];
        }
    }
}

/// OAuth 2.0 / OIDC client configuration.
pub struct OAuth2Config {
    client_id: []const u8 = "",
    client_secret: ([]const u8)? = null,
    authorize_url: []const u8 = "",
    token_url: []const u8 = "",
    redirect_uri: ([]const u8)? = null,
    scopes: []const ([]const u8) = [],
}

/// Additional query parameter for authorization URL customization.
pub struct ExtraParam {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Parsed OAuth 2.0 authorization callback query result.
/// Caller must call `deinit(alloc)` to free owned decoded strings.
pub struct CallbackResult {
    is_error: bool = false,
    code: ([]u8)? = null,
    state: ([]u8)? = null,
    error_code: ([]u8)? = null,
    error_description: ([]u8)? = null,
    error_uri: ([]u8)? = null,

    /// Frees all heap-allocated fields owned by this callback result.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (v := _.code) {
            alloc.free(v);
            _.code = null;
        }
        if (v := _.state) {
            alloc.free(v);
            _.state = null;
        }
        if (v := _.error_code) {
            alloc.free(v);
            _.error_code = null;
        }
        if (v := _.error_description) {
            alloc.free(v);
            _.error_description = null;
        }
        if (v := _.error_uri) {
            alloc.free(v);
            _.error_uri = null;
        }
    }

    /// Returns true if the callback represents a successful authorization code response.
    pub fn isSuccess(*_): bool {
        return !_.is_error && _.code != null;
    }

    /// Constant-time comparison of callback `state` against `expected_state` to prevent CSRF.
    pub fn verifyState(*_, expected_state: []const u8): bool {
        if (expected_state.len == 0) return false;
        const actual = _.state orelse return false;
        return constantTimeEql(expected_state, actual);
    }
}

/// Parsed OAuth 2.0 / OIDC Token Endpoint JSON response (RFC 6749 Section 5.1 & 5.2).
/// Caller must call `deinit(alloc)` to free all owned string fields.
pub struct TokenResponse {
    access_token: []u8 = [],
    token_type: []u8 = [],
    expires_in: u64? = null,
    expires_at: u64? = null,
    refresh_token: ([]u8)? = null,
    scope: ([]u8)? = null,
    id_token: ([]u8)? = null,
    oauth_error: ([]u8)? = null,
    error_description: ([]u8)? = null,
    error_uri: ([]u8)? = null,

    /// Frees all heap-allocated fields owned by this token response.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.access_token.len > 0) {
            alloc.free(_.access_token);
            _.access_token = [];
        }
        if (_.token_type.len > 0) {
            alloc.free(_.token_type);
            _.token_type = [];
        }
        if (v := _.refresh_token) {
            alloc.free(v);
            _.refresh_token = null;
        }
        if (v := _.scope) {
            alloc.free(v);
            _.scope = null;
        }
        if (v := _.id_token) {
            alloc.free(v);
            _.id_token = null;
        }
        if (v := _.oauth_error) {
            alloc.free(v);
            _.oauth_error = null;
        }
        if (v := _.error_description) {
            alloc.free(v);
            _.error_description = null;
        }
        if (v := _.error_uri) {
            alloc.free(v);
            _.error_uri = null;
        }
    }

    /// Computes and stores absolute `expires_at` from `issued_at_sec + expires_in`.
    pub fn setIssuedAt(*_, issued_at_sec: u64) {
        if (exp := _.expires_in) {
            _.expires_at = issued_at_sec +| exp;
        }
    }

    /// Returns true if the token is expired (or within `skew_sec` of expiration) at `now_sec`.
    /// Uses `expires_at` if set, otherwise compares against `expires_in`. Returns false if neither is set.
    pub fn isExpired(*_, now_sec: u64, skew_sec: u64): bool {
        const deadline = _.expires_at orelse _.expires_in orelse return false;
        return (now_sec +| skew_sec) >= deadline;
    }

    /// Checks whether the space-delimited `scope` string contains exact `target_scope`.
    pub fn hasScope(*_, target_scope: []const u8): bool {
        if (target_scope.len == 0) return false;
        const sc = _.scope orelse return false;
        let i: usize = 0;
        while (i < sc.len) {
            while (i < sc.len && sc[i] == ' ') {
                i += 1;
            }
            if (i >= sc.len) break;
            const start = i;
            while (i < sc.len && sc[i] != ' ') {
                i += 1;
            }
            const token = sc[start..i];
            if (std.mem.eql(u8, token, target_scope)) {
                return true;
            }
        }
        return false;
    }
}

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

/// Constant-time byte slice comparison to prevent timing side-channel leaks.
pub fn constantTimeEql(a: []const u8, b: []const u8): bool {
    let diff: u8 = if (a.len == b.len) 0 else 1;
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        diff |= a[i] ^ b[i];
        i += 1;
    }
    return diff == 0;
}

/// Checks if a byte is an RFC 3986 Section 2.3 unreserved character: ALPHA / DIGIT / "-" / "." / "_" / "~".
pub fn isUnreservedChar(c: u8): bool {
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return c == '-' || c == '.' || c == '_' || c == '~';
}

/// Computes the exact byte length of `input` after RFC 3986 percent-encoding.
pub fn percentEncodedLen(input: []const u8): usize {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (isUnreservedChar(input[i])) {
            out_len += 1;
        } else {
            out_len += 3;
        }
        i += 1;
    }
    return out_len;
}

/// Writes RFC 3986 percent-encoded `input` into `dest` and returns the number of bytes written.
pub fn writePercentEncoded(dest: []u8, input: []const u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (isUnreservedChar(c)) {
            dest[pos] = c;
            pos += 1;
        } else {
            dest[pos] = '%';
            dest[pos + 1] = HEX_UPPER[(c >> 4) & 0x0F];
            dest[pos + 2] = HEX_UPPER[c & 0x0F];
            pos += 3;
        }
        i += 1;
    }
    return pos;
}

/// Allocates and returns the RFC 3986 percent-encoded representation of `input`.
pub fn percentEncode(alloc: std.mem.Allocator, input: []const u8): ([]u8) ! OAuth2Error {
    const out_len = percentEncodedLen(input);
    let out = alloc.alloc(u8, out_len) catch return OAuth2Error.OutOfMemory;
    let * = writePercentEncoded(out, input);
    return out;
}

fn hexDigitVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    return null;
}

/// Decodes a percent-encoded string; when `plus_as_space` is true, '+' is decoded as ' '.
pub fn percentDecode(alloc: std.mem.Allocator, input: []const u8, plus_as_space: bool): ([]u8) ! OAuth2Error {
    let out = alloc.alloc(u8, input.len) catch return OAuth2Error.OutOfMemory;
    errdefer alloc.free(out);

    let in_pos: usize = 0;
    let out_pos: usize = 0;
    while (in_pos < input.len) {
        const c = input[in_pos];
        if (c == '%') {
            if (in_pos + 2 >= input.len) {
                return OAuth2Error.InvalidCallback;
            }
            const hi = hexDigitVal(input[in_pos + 1]) orelse return OAuth2Error.InvalidCallback;
            const lo = hexDigitVal(input[in_pos + 2]) orelse return OAuth2Error.InvalidCallback;
            out[out_pos] = (hi << 4) | lo;
            out_pos += 1;
            in_pos += 3;
        } else if (plus_as_space && c == '+') {
            out[out_pos] = ' ';
            out_pos += 1;
            in_pos += 1;
        } else {
            out[out_pos] = c;
            out_pos += 1;
            in_pos += 1;
        }
    }

    if (out_pos == out.len) {
        return out;
    }
    const shrunk = alloc.realloc(out, out_pos) catch return OAuth2Error.OutOfMemory;
    return shrunk;
}

/// Joins scope slices with single ASCII space into an owned buffer.
pub fn joinScopes(alloc: std.mem.Allocator, scopes: []const ([]const u8)): ([]u8) ! OAuth2Error {
    if (scopes.len == 0) {
        return alloc.alloc(u8, 0) catch return OAuth2Error.OutOfMemory;
    }
    let total_len: usize = 0;
    let i: usize = 0;
    while (i < scopes.len) {
        if (i > 0) total_len += 1;
        total_len += scopes[i].len;
        i += 1;
    }
    let out = alloc.alloc(u8, total_len) catch return OAuth2Error.OutOfMemory;
    let pos: usize = 0;
    i = 0;
    while (i < scopes.len) {
        if (i > 0) {
            out[pos] = ' ';
            pos += 1;
        }
        const s = scopes[i];
        @memcpy(out[pos..(pos + s.len)], s);
        pos += s.len;
        i += 1;
    }
    return out;
}
