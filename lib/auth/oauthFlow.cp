// Source: cp/lib/oauth/flow.cp (RFC 6749 OAuth 2.0 Authorization URL, Token Codec & AuthHeader)
// Pure CP RFC 6749 OAuth 2.0 authorization URL builder, form-urlencoded token request codec,
// OIDC/OAuth token JSON response parser, and RFC 6750 / RFC 7617 Authorization header codec.

const std = @import("std");
import { JSON, JsonValue } from "cap:json";
import { OAuthError, Pkce, PkceChallenge } from "./oauthPkce.cp";

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

fn containsControlOrNull(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch < 0x20 || ch == 0x7F) {
            return true;
        }
    }
    return false;
}

fn containsSpaceOrControl(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch <= 0x20 || ch >= 0x7F) {
            return true;
        }
    }
    return false;
}

fn isRfc3986Unreserved(ch: u8): bool {
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= '0' && ch <= '9') return true;
    return ch == '-' || ch == '.' || ch == '_' || ch == '~';
}

fn appendPercentEncoded(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, input: string): void ! OAuthError {
    let i: usize = 0;
    while (i < input.len) : (i += 1) {
        const ch = input[i];
        if (isRfc3986Unreserved(ch)) {
            buf.append(alloc, ch) catch return OAuthError.OutOfMemory;
        } else {
            buf.append(alloc, '%') catch return OAuthError.OutOfMemory;
            buf.append(alloc, HEX_UPPER[(ch >> 4) & 0x0F]) catch return OAuthError.OutOfMemory;
            buf.append(alloc, HEX_UPPER[ch & 0x0F]) catch return OAuthError.OutOfMemory;
        }
    }
}

fn appendQueryPair(
    buf: *std.ArrayListUnmanaged(u8),
    alloc: std.mem.Allocator,
    need_amp: *bool,
    key: string,
    val: string,
) : void ! OAuthError {
    if (need_amp.*) {
        buf.append(alloc, '&') catch return OAuthError.OutOfMemory;
    } else {
        need_amp.* = true;
    }
    appendPercentEncoded(buf, alloc, key)?;
    buf.append(alloc, '=') catch return OAuthError.OutOfMemory;
    appendPercentEncoded(buf, alloc, val)?;
}

fn appendEncodedScopes(
    buf: *std.ArrayListUnmanaged(u8),
    alloc: std.mem.Allocator,
    need_amp: *bool,
    scopes: []const string,
) : void ! OAuthError {
    if (scopes.len > 0) {
        let has_any = false;
        for (s of scopes) {
            if (containsControlOrNull(s)) {
                return OAuthError.InvalidConfig;
            }
            if (s.len > 0) {
                has_any = true;
            }
        }
        if (has_any) {
            if (need_amp.*) {
                buf.append(alloc, '&') catch return OAuthError.OutOfMemory;
            } else {
                need_amp.* = true;
            }
            buf.appendSlice(alloc, "scope=") catch return OAuthError.OutOfMemory;

            let first = true;
            for (s of scopes) {
                if (s.len == 0) continue;
                if (!first) {
                    buf.appendSlice(alloc, "%20") catch return OAuthError.OutOfMemory;
                }
                first = false;
                appendPercentEncoded(buf, alloc, s)?;
            }
        }
    }
}

fn validateClientConfig(cfg: OAuthConfig): void ! OAuthError {
    if (cfg.client_id.len == 0 || containsControlOrNull(cfg.client_id)) {
        return OAuthError.InvalidConfig;
    }
    if (containsControlOrNull(cfg.redirect_uri)) {
        return OAuthError.InvalidConfig;
    }
    if (containsControlOrNull(cfg.token_url)) {
        return OAuthError.InvalidConfig;
    }
    if (cfg.client_secret != null && containsControlOrNull(cfg.client_secret.?)) {
        return OAuthError.InvalidConfig;
    }
}

/// RFC 6749 OAuth 2.0 / OIDC client configuration.
pub struct OAuthConfig {
    client_id: string = "",
    client_secret: string? = null,
    authorize_url: string = "",
    token_url: string = "",
    redirect_uri: string = "",
    scopes: []const string = [],
}

/// RFC 6749 Authorization URL & `application/x-www-form-urlencoded` token request builder.
pub static struct OAuthFlow {
    /// Builds an RFC 6749 §4.1.1 authorization URL with RFC 3986 percent-encoding,
    /// handling base URLs with or without existing query parameters (`?` / `&`),
    /// optional `state`, and optional RFC 7636 PKCE `code_challenge` + `code_challenge_method`.
    pub static fn buildAuthorizeUrl(
        alloc: std.mem.Allocator,
        cfg: OAuthConfig,
        state: string,
        pkce: PkceChallenge?,
    ): ([]u8) ! OAuthError {
        validateClientConfig(cfg)?;
        if (cfg.authorize_url.len == 0 || containsControlOrNull(cfg.authorize_url)) {
            return OAuthError.InvalidConfig;
        }
        if (std.mem.indexOfScalar(u8, cfg.authorize_url, '#') != null) {
            return OAuthError.InvalidConfig;
        }
        if (containsControlOrNull(state)) {
            return OAuthError.InvalidConfig;
        }

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, cfg.authorize_url) catch return OAuthError.OutOfMemory;

        const has_query = (std.mem.indexOfScalar(u8, cfg.authorize_url, '?') != null);
        const last_ch = cfg.authorize_url[cfg.authorize_url.len - 1];
        let need_amp = false;
        if (!has_query) {
            out.append(alloc, '?') catch return OAuthError.OutOfMemory;
        } else if (last_ch != '?' && last_ch != '&') {
            out.append(alloc, '&') catch return OAuthError.OutOfMemory;
        }

        appendQueryPair(&out, alloc, &need_amp, "response_type", "code")?;
        appendQueryPair(&out, alloc, &need_amp, "client_id", cfg.client_id)?;

        if (cfg.redirect_uri.len > 0) {
            appendQueryPair(&out, alloc, &need_amp, "redirect_uri", cfg.redirect_uri)?;
        }
        appendEncodedScopes(&out, alloc, &need_amp, cfg.scopes)?;
        if (state.len > 0) {
            appendQueryPair(&out, alloc, &need_amp, "state", state)?;
        }
        if (pkce != null) {
            const p = pkce.?;
            if (p.challenge.len == 0 || p.method.len == 0 || containsControlOrNull(p.challenge) || containsControlOrNull(p.method)) {
                return OAuthError.InvalidVerifier;
            }
            appendQueryPair(&out, alloc, &need_amp, "code_challenge", p.challenge)?;
            appendQueryPair(&out, alloc, &need_amp, "code_challenge_method", p.method)?;
        }

        return out.toOwnedSlice(alloc) catch return OAuthError.OutOfMemory;
    }

    /// Encodes an `application/x-www-form-urlencoded` body for RFC 6749 §4.1.3 `authorization_code` exchange.
    pub static fn buildAuthorizationCodeBody(
        alloc: std.mem.Allocator,
        cfg: OAuthConfig,
        code: string,
        pkce_verifier: string?,
    ): ([]u8) ! OAuthError {
        validateClientConfig(cfg)?;
        if (code.len == 0 || containsControlOrNull(code)) {
            return OAuthError.InvalidConfig;
        }

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let need_amp = false;
        appendQueryPair(&out, alloc, &need_amp, "grant_type", "authorization_code")?;
        appendQueryPair(&out, alloc, &need_amp, "code", code)?;
        appendQueryPair(&out, alloc, &need_amp, "client_id", cfg.client_id)?;

        if (cfg.redirect_uri.len > 0) {
            appendQueryPair(&out, alloc, &need_amp, "redirect_uri", cfg.redirect_uri)?;
        }
        if (cfg.client_secret != null && cfg.client_secret.?.len > 0) {
            appendQueryPair(&out, alloc, &need_amp, "client_secret", cfg.client_secret.?)?;
        }
        if (pkce_verifier != null) {
            const verifier = pkce_verifier.?;
            if (!Pkce.isValidVerifier(verifier)) {
                return OAuthError.InvalidVerifier;
            }
            appendQueryPair(&out, alloc, &need_amp, "code_verifier", verifier)?;
        }

        return out.toOwnedSlice(alloc) catch return OAuthError.OutOfMemory;
    }

    /// Encodes an `application/x-www-form-urlencoded` body for RFC 6749 §6 `refresh_token` grant.
    pub static fn buildRefreshTokenBody(
        alloc: std.mem.Allocator,
        cfg: OAuthConfig,
        refresh_token: string,
    ): ([]u8) ! OAuthError {
        validateClientConfig(cfg)?;
        if (refresh_token.len == 0 || containsControlOrNull(refresh_token)) {
            return OAuthError.InvalidConfig;
        }

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let need_amp = false;
        appendQueryPair(&out, alloc, &need_amp, "grant_type", "refresh_token")?;
        appendQueryPair(&out, alloc, &need_amp, "refresh_token", refresh_token)?;
        appendQueryPair(&out, alloc, &need_amp, "client_id", cfg.client_id)?;

        if (cfg.client_secret != null && cfg.client_secret.?.len > 0) {
            appendQueryPair(&out, alloc, &need_amp, "client_secret", cfg.client_secret.?)?;
        }
        appendEncodedScopes(&out, alloc, &need_amp, cfg.scopes)?;

        return out.toOwnedSlice(alloc) catch return OAuthError.OutOfMemory;
    }

    /// Encodes an `application/x-www-form-urlencoded` body for RFC 6749 §4.4 `client_credentials` grant.
    pub static fn buildClientCredentialsBody(
        alloc: std.mem.Allocator,
        cfg: OAuthConfig,
    ): ([]u8) ! OAuthError {
        validateClientConfig(cfg)?;

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let need_amp = false;
        appendQueryPair(&out, alloc, &need_amp, "grant_type", "client_credentials")?;
        appendQueryPair(&out, alloc, &need_amp, "client_id", cfg.client_id)?;

        if (cfg.client_secret != null && cfg.client_secret.?.len > 0) {
            appendQueryPair(&out, alloc, &need_amp, "client_secret", cfg.client_secret.?)?;
        }
        appendEncodedScopes(&out, alloc, &need_amp, cfg.scopes)?;

        return out.toOwnedSlice(alloc) catch return OAuthError.OutOfMemory;
    }
}

struct OptStr {
    has_value: bool = false,
    value: string = "",
}

fn extractOptionalString(alloc: std.mem.Allocator, root: JsonValue, key: string): OptStr ! OAuthError {
    const val_opt = root.get(key);
    if (val_opt == null) return OptStr{ has_value: false, value: "" };
    const val = val_opt.?;
    if (val.isNull()) return OptStr{ has_value: false, value: "" };
    const s_opt = val.asString();
    if (s_opt == null) return OAuthError.InvalidTokenResponse;
    const duped = alloc.dupe(u8, s_opt.?) catch return OAuthError.OutOfMemory;
    return OptStr{ has_value: true, value: duped };
}

/// Parsed RFC 6749 §5.1 / OIDC §3.1.3.3 token endpoint JSON response.
/// Call `deinit()` to release all owned string buffers.
pub struct OAuthTokenResponse {
    alloc: std.mem.Allocator = undefined,
    access_token: string = "",
    token_type: string = "",
    expires_in: i64? = null,
    refresh_token: string? = null,
    scope: string? = null,
    id_token: string? = null,

    /// Releases all owned string fields allocated during `parseJson`.
    pub fn deinit(*_) {
        if (_.access_token.len > 0) {
            _.alloc.free(_.access_token);
            _.access_token = "";
        }
        if (_.token_type.len > 0) {
            _.alloc.free(_.token_type);
            _.token_type = "";
        }
        if (_.refresh_token != null) {
            _.alloc.free(_.refresh_token.?);
            _.refresh_token = null;
        }
        if (_.scope != null) {
            _.alloc.free(_.scope.?);
            _.scope = null;
        }
        if (_.id_token != null) {
            _.alloc.free(_.id_token.?);
            _.id_token = null;
        }
    }

    /// Parses a token endpoint JSON payload into an owned `OAuthTokenResponse`.
    /// Requires non-empty string `access_token` and `token_type` per RFC 6749 §5.1.
    pub static fn parseJson(alloc: std.mem.Allocator, json_str: string): OAuthTokenResponse ! OAuthError {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();

        const root = JSON.parse(tmp_arena.allocator(), json_str) catch return OAuthError.InvalidTokenResponse;
        if (!root.isObject()) {
            return OAuthError.InvalidTokenResponse;
        }

        const at_opt = root.get("access_token");
        if (at_opt == null) return OAuthError.InvalidTokenResponse;
        const at_str_opt = at_opt.?.asString();
        if (at_str_opt == null) return OAuthError.InvalidTokenResponse;
        const at_raw = at_str_opt.?;
        if (at_raw.len == 0) {
            return OAuthError.InvalidTokenResponse;
        }

        const tt_opt = root.get("token_type");
        if (tt_opt == null) return OAuthError.InvalidTokenResponse;
        const tt_str_opt = tt_opt.?.asString();
        if (tt_str_opt == null) return OAuthError.InvalidTokenResponse;
        const tt_raw = tt_str_opt.?;
        if (tt_raw.len == 0) {
            return OAuthError.InvalidTokenResponse;
        }

        let expires_in_val: i64? = null;
        const exp_opt = root.get("expires_in");
        if (exp_opt != null && !exp_opt.?.isNull()) {
            const num_opt = exp_opt.?.asNumber();
            if (num_opt == null) return OAuthError.InvalidTokenResponse;
            const num = num_opt.?;
            if (std.math.isNan(num) || std.math.isInf(num) || num < 0.0 || num >= 9223372036854775808.0) {
                return OAuthError.InvalidTokenResponse;
            }
            const exp_i64: i64 = @intFromFloat(num);
            expires_in_val = exp_i64;
        }

        let access_token: string = "";
        let token_type: string = "";
        let refresh_token: string? = null;
        let scope: string? = null;
        let id_token: string? = null;
        errdefer {
            if (access_token.len > 0) alloc.free(access_token);
            if (token_type.len > 0) alloc.free(token_type);
            if (refresh_token != null) alloc.free(refresh_token.?);
            if (scope != null) alloc.free(scope.?);
            if (id_token != null) alloc.free(id_token.?);
        }

        access_token = alloc.dupe(u8, at_raw) catch return OAuthError.OutOfMemory;
        token_type = alloc.dupe(u8, tt_raw) catch return OAuthError.OutOfMemory;
        const rt_res = extractOptionalString(alloc, root, "refresh_token")?;
        if (rt_res.has_value) refresh_token = rt_res.value;
        const sc_res = extractOptionalString(alloc, root, "scope")?;
        if (sc_res.has_value) scope = sc_res.value;
        const id_res = extractOptionalString(alloc, root, "id_token")?;
        if (id_res.has_value) id_token = id_res.value;

        return OAuthTokenResponse{
            alloc: alloc,
            access_token: access_token,
            token_type: token_type,
            expires_in: expires_in_val,
            refresh_token: refresh_token,
            scope: scope,
            id_token: id_token,
        };
    }

    /// Returns `true` if `expires_in` is present and `now_sec + skew_sec >= issued_at_sec + expires_in`.
    pub fn isExpiredAt(issued_at_sec: i64, now_sec: i64, skew_sec: i64): bool {
        if (_.expires_in == null) return false;
        const ttl = _.expires_in.?;
        const expires_at: i128 = (issued_at_sec as i128) + (ttl as i128);
        const effective_now: i128 = (now_sec as i128) + (skew_sec as i128);
        return effective_now >= expires_at;
    }

    /// Returns `true` if the space-delimited `scope` claim contains an exact token match for `target`.
    pub fn hasScope(target: string): bool {
        if (target.len == 0 || _.scope == null) return false;
        const s = _.scope.?;
        let it = std.mem.tokenizeScalar(u8, s, ' ');
        while (tok := it.next()) {
            if (std.mem.eql(u8, tok, target)) {
                return true;
            }
        }
        return false;
    }
}

/// Decoded HTTP Basic authentication credentials (`username:password`).
pub struct BasicCredentials {
    alloc: std.mem.Allocator = undefined,
    username: string = "",
    password: string = "",
    raw_decoded: []u8 = "",

    /// Releases the underlying decoded buffer.
    pub fn deinit(*_) {
        if (_.raw_decoded.len > 0) {
            _.alloc.free(_.raw_decoded);
            _.raw_decoded = "";
        }
        _.username = "";
        _.password = "";
    }
}

/// RFC 6750 Bearer & RFC 7617 Basic HTTP `Authorization` header encoder/decoder.
pub static struct AuthHeader {
    /// Formats `Bearer <token>` after rejecting empty tokens, whitespace, and control characters.
    pub static fn formatBearer(alloc: std.mem.Allocator, token: string): ([]u8) ! OAuthError {
        if (token.len == 0 || containsSpaceOrControl(token)) {
            return OAuthError.InvalidAuthHeader;
        }
        return std.fmt.allocPrint(alloc, "Bearer {s}", .{ token }) catch return OAuthError.OutOfMemory;
    }

    /// Extracts the token from a `Bearer <token>` header (case-insensitive scheme match).
    /// Returns `null` if missing, malformed, empty token, or contains whitespace/control characters.
    pub static fn parseBearer(header: string): string? {
        const trimmed = std.mem.trim(u8, header, " \t");
        if (trimmed.len <= 7) return null;
        if (!std.ascii.eqlIgnoreCase(trimmed[0..6], "bearer")) return null;
        if (trimmed[6] != ' ' && trimmed[6] != '\t') return null;

        const token = std.mem.trim(u8, trimmed[7..], " \t");
        if (token.len == 0 || containsSpaceOrControl(token)) return null;
        return token;
    }

    /// Formats `Basic <base64(user:pass)>` per RFC 7617. Rejects `:` in `user` and CR/LF/NUL control bytes.
    pub static fn formatBasic(alloc: std.mem.Allocator, user: string, pass: string): ([]u8) ! OAuthError {
        if (std.mem.indexOfScalar(u8, user, ':') != null) {
            return OAuthError.InvalidAuthHeader;
        }
        if (containsControlOrNull(user) || containsControlOrNull(pass)) {
            return OAuthError.InvalidAuthHeader;
        }

        const raw_len = user.len + 1 + pass.len;
        let raw = alloc.alloc(u8, raw_len) catch return OAuthError.OutOfMemory;
        defer alloc.free(raw);

        @memcpy(raw[0..user.len], user);
        raw[user.len] = ':';
        @memcpy(raw[user.len + 1 .. raw_len], pass);

        const encoder = std.base64.standard.Encoder;
        const b64_len = encoder.calcSize(raw_len);
        let b64 = alloc.alloc(u8, b64_len) catch return OAuthError.OutOfMemory;
        defer alloc.free(b64);
        * = encoder.encode(b64, raw);

        return std.fmt.allocPrint(alloc, "Basic {s}", .{ b64 }) catch return OAuthError.OutOfMemory;
    }

    /// Parses and Base64-decodes a `Basic <base64>` header (case-insensitive scheme match) into `user:pass`.
    pub static fn parseBasic(alloc: std.mem.Allocator, header: string): BasicCredentials ! OAuthError {
        const trimmed = std.mem.trim(u8, header, " \t");
        if (trimmed.len <= 6) {
            return OAuthError.InvalidAuthHeader;
        }
        if (!std.ascii.eqlIgnoreCase(trimmed[0..5], "basic")) {
            return OAuthError.InvalidAuthHeader;
        }
        if (trimmed[5] != ' ' && trimmed[5] != '\t') {
            return OAuthError.InvalidAuthHeader;
        }

        const b64_part = std.mem.trim(u8, trimmed[6..], " \t");
        if (b64_part.len == 0 || containsSpaceOrControl(b64_part)) {
            return OAuthError.InvalidAuthHeader;
        }

        const decoder = std.base64.standard.Decoder;
        const dec_len = decoder.calcSizeForSlice(b64_part) catch return OAuthError.InvalidAuthHeader;
        let decoded = alloc.alloc(u8, dec_len) catch return OAuthError.OutOfMemory;
        errdefer alloc.free(decoded);

        decoder.decode(decoded, b64_part) catch return OAuthError.InvalidAuthHeader;
        if (containsControlOrNull(decoded)) {
            return OAuthError.InvalidAuthHeader;
        }

        const colon_opt = std.mem.indexOfScalar(u8, decoded, ':');
        if (colon_opt == null) return OAuthError.InvalidAuthHeader;
        const colon = colon_opt.?;
        return BasicCredentials{
            alloc: alloc,
            username: decoded[0..colon],
            password: decoded[colon + 1 ..],
            raw_decoded: decoded,
        };
    }
}
