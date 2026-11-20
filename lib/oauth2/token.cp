// Source: cp/lib/oauth2/token.cp (OAuth 2.0 Token Request Builders, Basic Auth & JSON Response Parser)

const std = @import("std");
import {
    OAuth2Error,
    OAuth2Config,
    TokenResponse,
    percentEncodedLen,
    writePercentEncoded,
    joinScopes,
} from "./types.cp";
import { isValidCodeVerifier } from "./pkce.cp";

fn appendFormPair(buf: []u8, pos: *usize, need_sep: *bool, key: []const u8, val: []const u8) {
    if (need_sep.*) {
        buf[pos.*] = '&';
        pos.* += 1;
    }
    pos.* += writePercentEncoded(buf[pos.*..], key);
    buf[pos.*] = '=';
    pos.* += 1;
    pos.* += writePercentEncoded(buf[pos.*..], val);
    need_sep.* = true;
}

/// Builds RFC 6749 Section 4.1.3 `application/x-www-form-urlencoded` body for `authorization_code` grant.
pub fn buildAuthorizationCodeBody(
    alloc: std.mem.Allocator,
    config: OAuth2Config,
    code: []const u8,
    code_verifier: ([]const u8)?,
): ([]u8) ! OAuth2Error {
    if (code.len == 0) {
        return OAuth2Error.InvalidParameter;
    }
    if (cv := code_verifier) {
        if (!isValidCodeVerifier(cv)) {
            return OAuth2Error.InvalidVerifier;
        }
    }

    let total_len: usize = "grant_type=authorization_code".len;
    total_len += 1 + "code".len + 1 + percentEncodedLen(code);

    if (ruri := config.redirect_uri) {
        if (ruri.len > 0) {
            total_len += 1 + "redirect_uri".len + 1 + percentEncodedLen(ruri);
        }
    }
    if (config.client_id.len > 0) {
        total_len += 1 + "client_id".len + 1 + percentEncodedLen(config.client_id);
    }
    if (sec := config.client_secret) {
        if (sec.len > 0) {
            total_len += 1 + "client_secret".len + 1 + percentEncodedLen(sec);
        }
    }
    if (cv := code_verifier) {
        total_len += 1 + "code_verifier".len + 1 + percentEncodedLen(cv);
    }

    let out = alloc.alloc(u8, total_len) catch return OAuth2Error.OutOfMemory;
    let pos: usize = 0;
    let need_sep: bool = false;

    appendFormPair(out, &pos, &need_sep, "grant_type", "authorization_code");
    appendFormPair(out, &pos, &need_sep, "code", code);

    if (ruri := config.redirect_uri) {
        if (ruri.len > 0) {
            appendFormPair(out, &pos, &need_sep, "redirect_uri", ruri);
        }
    }
    if (config.client_id.len > 0) {
        appendFormPair(out, &pos, &need_sep, "client_id", config.client_id);
    }
    if (sec := config.client_secret) {
        if (sec.len > 0) {
            appendFormPair(out, &pos, &need_sep, "client_secret", sec);
        }
    }
    if (cv := code_verifier) {
        appendFormPair(out, &pos, &need_sep, "code_verifier", cv);
    }

    return out;
}

/// Builds RFC 6749 Section 4.4 `application/x-www-form-urlencoded` body for `client_credentials` grant.
pub fn buildClientCredentialsBody(
    alloc: std.mem.Allocator,
    config: OAuth2Config,
    scopes: []const ([]const u8),
): ([]u8) ! OAuth2Error {
    const active_scopes = if (scopes.len > 0) scopes else config.scopes;
    let joined_scopes: ([]u8)? = null;
    defer {
        if (s := joined_scopes) alloc.free(s);
    }
    if (active_scopes.len > 0) {
        joined_scopes = joinScopes(alloc, active_scopes)?;
    }

    let total_len: usize = "grant_type=client_credentials".len;
    if (sc := joined_scopes) {
        if (sc.len > 0) {
            total_len += 1 + "scope".len + 1 + percentEncodedLen(sc);
        }
    }
    if (config.client_id.len > 0) {
        total_len += 1 + "client_id".len + 1 + percentEncodedLen(config.client_id);
    }
    if (sec := config.client_secret) {
        if (sec.len > 0) {
            total_len += 1 + "client_secret".len + 1 + percentEncodedLen(sec);
        }
    }

    let out = alloc.alloc(u8, total_len) catch return OAuth2Error.OutOfMemory;
    let pos: usize = 0;
    let need_sep: bool = false;

    appendFormPair(out, &pos, &need_sep, "grant_type", "client_credentials");
    if (sc := joined_scopes) {
        if (sc.len > 0) {
            appendFormPair(out, &pos, &need_sep, "scope", sc);
        }
    }
    if (config.client_id.len > 0) {
        appendFormPair(out, &pos, &need_sep, "client_id", config.client_id);
    }
    if (sec := config.client_secret) {
        if (sec.len > 0) {
            appendFormPair(out, &pos, &need_sep, "client_secret", sec);
        }
    }

    return out;
}

/// Builds RFC 6749 Section 6 `application/x-www-form-urlencoded` body for `refresh_token` grant.
pub fn buildRefreshTokenBody(
    alloc: std.mem.Allocator,
    config: OAuth2Config,
    refresh_token: []const u8,
    scopes: []const ([]const u8),
): ([]u8) ! OAuth2Error {
    if (refresh_token.len == 0) {
        return OAuth2Error.InvalidParameter;
    }

    let joined_scopes: ([]u8)? = null;
    defer {
        if (s := joined_scopes) alloc.free(s);
    }
    if (scopes.len > 0) {
        joined_scopes = joinScopes(alloc, scopes)?;
    }

    let total_len: usize = "grant_type=refresh_token".len;
    total_len += 1 + "refresh_token".len + 1 + percentEncodedLen(refresh_token);

    if (sc := joined_scopes) {
        if (sc.len > 0) {
            total_len += 1 + "scope".len + 1 + percentEncodedLen(sc);
        }
    }
    if (config.client_id.len > 0) {
        total_len += 1 + "client_id".len + 1 + percentEncodedLen(config.client_id);
    }
    if (sec := config.client_secret) {
        if (sec.len > 0) {
            total_len += 1 + "client_secret".len + 1 + percentEncodedLen(sec);
        }
    }

    let out = alloc.alloc(u8, total_len) catch return OAuth2Error.OutOfMemory;
    let pos: usize = 0;
    let need_sep: bool = false;

    appendFormPair(out, &pos, &need_sep, "grant_type", "refresh_token");
    appendFormPair(out, &pos, &need_sep, "refresh_token", refresh_token);
    if (sc := joined_scopes) {
        if (sc.len > 0) {
            appendFormPair(out, &pos, &need_sep, "scope", sc);
        }
    }
    if (config.client_id.len > 0) {
        appendFormPair(out, &pos, &need_sep, "client_id", config.client_id);
    }
    if (sec := config.client_secret) {
        if (sec.len > 0) {
            appendFormPair(out, &pos, &need_sep, "client_secret", sec);
        }
    }

    return out;
}

/// Builds RFC 7009 Token Revocation `application/x-www-form-urlencoded` body.
pub fn buildRevocationBody(
    alloc: std.mem.Allocator,
    token: []const u8,
    token_type_hint: ([]const u8)?,
    client_id: ([]const u8)?,
    client_secret: ([]const u8)?,
): ([]u8) ! OAuth2Error {
    if (token.len == 0) {
        return OAuth2Error.InvalidParameter;
    }

    let total_len: usize = "token".len + 1 + percentEncodedLen(token);
    if (hint := token_type_hint) {
        if (hint.len > 0) {
            total_len += 1 + "token_type_hint".len + 1 + percentEncodedLen(hint);
        }
    }
    if (cid := client_id) {
        if (cid.len > 0) {
            total_len += 1 + "client_id".len + 1 + percentEncodedLen(cid);
        }
    }
    if (sec := client_secret) {
        if (sec.len > 0) {
            total_len += 1 + "client_secret".len + 1 + percentEncodedLen(sec);
        }
    }

    let out = alloc.alloc(u8, total_len) catch return OAuth2Error.OutOfMemory;
    let pos: usize = 0;
    let need_sep: bool = false;

    appendFormPair(out, &pos, &need_sep, "token", token);
    if (hint := token_type_hint) {
        if (hint.len > 0) {
            appendFormPair(out, &pos, &need_sep, "token_type_hint", hint);
        }
    }
    if (cid := client_id) {
        if (cid.len > 0) {
            appendFormPair(out, &pos, &need_sep, "client_id", cid);
        }
    }
    if (sec := client_secret) {
        if (sec.len > 0) {
            appendFormPair(out, &pos, &need_sep, "client_secret", sec);
        }
    }

    return out;
}

/// Builds HTTP Basic Authorization header per RFC 6749 Section 2.3.1:
/// `"Basic " + Base64(percentEncode(client_id) + ":" + percentEncode(client_secret))`.
pub fn buildBasicAuthHeader(
    alloc: std.mem.Allocator,
    client_id: []const u8,
    client_secret: []const u8,
): ([]u8) ! OAuth2Error {
    if (client_id.len == 0) {
        return OAuth2Error.InvalidConfig;
    }

    const enc_id_len = percentEncodedLen(client_id);
    const enc_sec_len = percentEncodedLen(client_secret);
    const raw_len = enc_id_len + 1 + enc_sec_len;

    let raw = alloc.alloc(u8, raw_len) catch return OAuth2Error.OutOfMemory;
    defer alloc.free(raw);

    let pos: usize = 0;
    pos += writePercentEncoded(raw[pos..], client_id);
    raw[pos] = ':';
    pos += 1;
    * = writePercentEncoded(raw[pos..], client_secret);

    const encoder = std.base64.standard.Encoder;
    const b64_len = encoder.calcSize(raw_len);
    const prefix = "Basic ";
    let out = alloc.alloc(u8, prefix.len + b64_len) catch return OAuth2Error.OutOfMemory;
    @memcpy(out[0..prefix.len], prefix);
    * = encoder.encode(out[prefix.len..], raw);
    return out;
}

fn getOptionalJsonString(alloc: std.mem.Allocator, obj: std.json.ObjectMap, key: []const u8): (([]u8)?) ! OAuth2Error {
    const val = obj.get(key) orelse return null;
    return switch (val) {
        .string => |s| alloc.dupe(u8, s) catch return OAuth2Error.OutOfMemory,
        .null => null,
        else => OAuth2Error.InvalidTokenResponse,
    };
}

/// Parses an OAuth 2.0 / OIDC Token Endpoint JSON response (or error JSON) into `TokenResponse`.
/// Unlike `parseTokenResponse`, OAuth2 error payloads (`{"error":"..."}`) are returned with `oauth_error` populated.
pub fn parseTokenResponseOrError(alloc: std.mem.Allocator, json_bytes: []const u8): TokenResponse ! OAuth2Error {
    if (json_bytes.len == 0) {
        return OAuth2Error.InvalidTokenResponse;
    }

    let parsed = std.json.parseFromSlice(std.json.Value, alloc, json_bytes, .{}) catch {
        return OAuth2Error.InvalidTokenResponse;
    };
    defer parsed.deinit();

    const obj = switch (parsed.value) {
        .object => |o| o,
        else => return OAuth2Error.InvalidTokenResponse,
    };

    let res = TokenResponse{};
    errdefer res.deinit(alloc);

    if (err_str := getOptionalJsonString(alloc, obj, "error")?) {
        if (err_str.len == 0) {
            alloc.free(err_str);
            return OAuth2Error.InvalidTokenResponse;
        }
        res.oauth_error = err_str;
        res.error_description = getOptionalJsonString(alloc, obj, "error_description")?;
        res.error_uri = getOptionalJsonString(alloc, obj, "error_uri")?;
        return res;
    }

    if (at := getOptionalJsonString(alloc, obj, "access_token")?) {
        if (at.len == 0) {
            alloc.free(at);
            return OAuth2Error.InvalidTokenResponse;
        }
        res.access_token = at;
    } else {
        return OAuth2Error.InvalidTokenResponse;
    }

    if (tt := getOptionalJsonString(alloc, obj, "token_type")?) {
        if (tt.len == 0) {
            alloc.free(tt);
            return OAuth2Error.InvalidTokenResponse;
        }
        res.token_type = tt;
    } else {
        return OAuth2Error.InvalidTokenResponse;
    }

    if (exp_val := obj.get("expires_in")) {
        switch (exp_val) {
            .integer => |n| {
                if (n < 0) return OAuth2Error.InvalidTokenResponse;
                res.expires_in = n as u64;
            },
            .float => |f| {
                if (f < 0.0) return OAuth2Error.InvalidTokenResponse;
                res.expires_in = f as u64;
            },
            .string => |s| {
                res.expires_in = std.fmt.parseInt(u64, s, 10) catch return OAuth2Error.InvalidTokenResponse;
            },
            .null => {},
            else => return OAuth2Error.InvalidTokenResponse,
        }
    }

    res.refresh_token = getOptionalJsonString(alloc, obj, "refresh_token")?;
    res.scope = getOptionalJsonString(alloc, obj, "scope")?;
    res.id_token = getOptionalJsonString(alloc, obj, "id_token")?;

    return res;
}

/// Parses an OAuth 2.0 / OIDC Token Endpoint JSON response.
/// Returns `OAuth2Error.OAuthErrorResponse` if the JSON payload contains an OAuth2 `"error"` field.
pub fn parseTokenResponse(alloc: std.mem.Allocator, json_bytes: []const u8): TokenResponse ! OAuth2Error {
    let res = parseTokenResponseOrError(alloc, json_bytes)?;
    if (res.oauth_error != null) {
        res.deinit(alloc);
        return OAuth2Error.OAuthErrorResponse;
    }
    return res;
}
