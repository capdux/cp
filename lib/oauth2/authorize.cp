// Source: cp/lib/oauth2/authorize.cp (OAuth 2.0 / OIDC Authorization URL Builder & Callback Parser)

const std = @import("std");
import {
    OAuth2Error,
    OAuth2Config,
    ExtraParam,
    CallbackResult,
    constantTimeEql,
    percentEncodedLen,
    writePercentEncoded,
    percentDecode,
    joinScopes,
} from "./types.cp";

fn appendEncodedPair(buf: []u8, pos: *usize, need_sep: *bool, key: []const u8, val: []const u8) {
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

/// Builds an RFC 6749 / OIDC authorization URL with RFC 3986 percent-encoded parameters.
pub fn buildAuthorizeUrl(
    alloc: std.mem.Allocator,
    config: OAuth2Config,
    state: ([]const u8)?,
    pkce_challenge: ([]const u8)?,
    nonce: ([]const u8)?,
    extra_params: []const ExtraParam,
): ([]u8) ! OAuth2Error {
    if (config.authorize_url.len == 0 || config.client_id.len == 0) {
        return OAuth2Error.InvalidConfig;
    }

    let joined_scopes: ([]u8)? = null;
    defer {
        if (s := joined_scopes) alloc.free(s);
    }
    if (config.scopes.len > 0) {
        joined_scopes = joinScopes(alloc, config.scopes)?;
    }

    // Determine separator after base authorize_url
    const has_q = std.mem.indexOfScalar(u8, config.authorize_url, '?') != null;
    const last_ch = config.authorize_url[config.authorize_url.len - 1];
    const ends_with_sep = (last_ch == '?' || last_ch == '&');

    let total_len: usize = config.authorize_url.len;
    if (!has_q) {
        total_len += 1; // '?'
    } else if (!ends_with_sep) {
        total_len += 1; // '&'
    }

    // response_type=code (18 bytes) + "&client_id=" (11) + encoded(client_id)
    total_len += "response_type=code".len;
    total_len += 1 + "client_id".len + 1 + percentEncodedLen(config.client_id);

    if (ruri := config.redirect_uri) {
        if (ruri.len > 0) {
            total_len += 1 + "redirect_uri".len + 1 + percentEncodedLen(ruri);
        }
    }
    if (sc := joined_scopes) {
        if (sc.len > 0) {
            total_len += 1 + "scope".len + 1 + percentEncodedLen(sc);
        }
    }
    if (st := state) {
        if (st.len > 0) {
            total_len += 1 + "state".len + 1 + percentEncodedLen(st);
        }
    }
    if (ch := pkce_challenge) {
        if (ch.len > 0) {
            total_len += 1 + "code_challenge".len + 1 + percentEncodedLen(ch);
            total_len += 1 + "code_challenge_method=S256".len;
        }
    }
    if (nc := nonce) {
        if (nc.len > 0) {
            total_len += 1 + "nonce".len + 1 + percentEncodedLen(nc);
        }
    }

    let i: usize = 0;
    while (i < extra_params.len) {
        const ep = extra_params[i];
        if (ep.name.len > 0) {
            total_len += 1 + percentEncodedLen(ep.name) + 1 + percentEncodedLen(ep.value);
        }
        i += 1;
    }

    let out = alloc.alloc(u8, total_len) catch return OAuth2Error.OutOfMemory;
    let pos: usize = 0;
    @memcpy(out[0..config.authorize_url.len], config.authorize_url);
    pos += config.authorize_url.len;

    let need_sep: bool = false;
    if (!has_q) {
        out[pos] = '?';
        pos += 1;
    } else if (!ends_with_sep) {
        need_sep = true;
    }

    appendEncodedPair(out, &pos, &need_sep, "response_type", "code");
    appendEncodedPair(out, &pos, &need_sep, "client_id", config.client_id);

    if (ruri := config.redirect_uri) {
        if (ruri.len > 0) {
            appendEncodedPair(out, &pos, &need_sep, "redirect_uri", ruri);
        }
    }
    if (sc := joined_scopes) {
        if (sc.len > 0) {
            appendEncodedPair(out, &pos, &need_sep, "scope", sc);
        }
    }
    if (st := state) {
        if (st.len > 0) {
            appendEncodedPair(out, &pos, &need_sep, "state", st);
        }
    }
    if (ch := pkce_challenge) {
        if (ch.len > 0) {
            appendEncodedPair(out, &pos, &need_sep, "code_challenge", ch);
            appendEncodedPair(out, &pos, &need_sep, "code_challenge_method", "S256");
        }
    }
    if (nc := nonce) {
        if (nc.len > 0) {
            appendEncodedPair(out, &pos, &need_sep, "nonce", nc);
        }
    }

    i = 0;
    while (i < extra_params.len) {
        const ep = extra_params[i];
        if (ep.name.len > 0) {
            appendEncodedPair(out, &pos, &need_sep, ep.name, ep.value);
        }
        i += 1;
    }

    return out;
}

/// Parses an OAuth 2.0 callback URL or query string into `CallbackResult`.
/// Supports full URLs (`https://cb?code=...&state=...`), `?code=...`, or raw `code=...`.
pub fn parseCallbackQuery(alloc: std.mem.Allocator, query_or_url: []const u8): CallbackResult ! OAuth2Error {
    if (query_or_url.len == 0) {
        return OAuth2Error.InvalidCallback;
    }

    let raw = query_or_url;
    // Strip URL fragment if present
    if (hash_idx := std.mem.indexOfScalar(u8, raw, '#')) {
        raw = raw[0..hash_idx];
    }
    // Extract query part after '?' if present
    if (q_idx := std.mem.indexOfScalar(u8, raw, '?')) {
        raw = raw[(q_idx + 1)..];
    }
    if (raw.len == 0) {
        return OAuth2Error.InvalidCallback;
    }

    let res = CallbackResult{};
    errdefer res.deinit(alloc);

    let start: usize = 0;
    while (start <= raw.len) {
        let end: usize = start;
        while (end < raw.len && raw[end] != '&') {
            end += 1;
        }
        const pair = raw[start..end];
        if (pair.len > 0) {
            let eq_idx: usize = 0;
            while (eq_idx < pair.len && pair[eq_idx] != '=') {
                eq_idx += 1;
            }
            const raw_key = pair[0..eq_idx];
            const raw_val = if (eq_idx < pair.len) pair[(eq_idx + 1)..] else "";

            const key = percentDecode(alloc, raw_key, true)?;
            defer alloc.free(key);

            if (std.mem.eql(u8, key, "code")) {
                if (res.code != null) return OAuth2Error.InvalidCallback;
                res.code = percentDecode(alloc, raw_val, true)?;
            } else if (std.mem.eql(u8, key, "state")) {
                if (res.state != null) return OAuth2Error.InvalidCallback;
                res.state = percentDecode(alloc, raw_val, true)?;
            } else if (std.mem.eql(u8, key, "error")) {
                if (res.error_code != null) return OAuth2Error.InvalidCallback;
                res.error_code = percentDecode(alloc, raw_val, true)?;
                res.is_error = true;
            } else if (std.mem.eql(u8, key, "error_description")) {
                if (res.error_description != null) return OAuth2Error.InvalidCallback;
                res.error_description = percentDecode(alloc, raw_val, true)?;
            } else if (std.mem.eql(u8, key, "error_uri")) {
                if (res.error_uri != null) return OAuth2Error.InvalidCallback;
                res.error_uri = percentDecode(alloc, raw_val, true)?;
            }
        }
        if (end >= raw.len) break;
        start = end + 1;
    }

    if (res.is_error && res.code != null) {
        return OAuth2Error.InvalidCallback;
    }
    if (!res.is_error && res.code == null) {
        return OAuth2Error.InvalidCallback;
    }
    return res;
}

/// Constant-time CSRF `state` verification helper.
pub fn verifyCallbackState(expected_state: []const u8, actual_state: ([]const u8)?): bool {
    if (expected_state.len == 0) return false;
    const actual = actual_state orelse return false;
    return constantTimeEql(expected_state, actual);
}
