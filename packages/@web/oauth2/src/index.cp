// Official @web/oauth2 Plugin — OAuth 2.0 + RFC 7636 PKCE for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const OA_MAX_STATES: usize = 64;
pub const OA_MAX_STATE_LEN: usize = 128;
pub const OA_URL_BUF_CAP: usize = 4096;
pub const OA_RESP_BUF_CAP: usize = 2048;

pub struct OAuth2Config {
    client_id: string = "",
    authorize_url: string = "",
    token_url: string = "",
    redirect_uri: string = "",
    scope: string = "openid profile email",
    login_path: string = "/auth/login",
    callback_path: string = "/auth/callback",
}

struct OaStateSlot {
    buf: [OA_MAX_STATE_LEN]u8 = [0; OA_MAX_STATE_LEN],
    len: usize = 0,
    active: bool = false,
}

let oa_cfg: OAuth2Config = OAuth2Config{};
let oa_states: [OA_MAX_STATES]OaStateSlot = [OaStateSlot{}; OA_MAX_STATES];
let oa_state_next: usize = 0;
let oa_auto_seq: u64 = 1;
let oa_location_buf: [OA_URL_BUF_CAP]u8 = [0; OA_URL_BUF_CAP];
let oa_resp_buf: [OA_RESP_BUF_CAP]u8 = [0; OA_RESP_BUF_CAP];
let oa_auto_state_buf: [64]u8 = [0; 64];
let oa_auto_challenge_buf: [64]u8 = [0; 64];

fn oaBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn oaWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn oaWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn oaHexUpper(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('A' + (n - 10));
}

fn oaHexLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn oaIsUnreserved(c: u8): bool {
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c == '-' || c == '_' || c == '.' || c == '~') return true;
    return false;
}

fn oaWriteUrlEncoded(out_buf: []u8, pos: *usize, s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (oaIsUnreserved(c)) {
            if (!oaWriteByte(out_buf, pos, c)) return false;
        } else {
            if (!oaWriteByte(out_buf, pos, '%')) return false;
            if (!oaWriteByte(out_buf, pos, oaHexUpper((c >> 4) & 0x0F))) return false;
            if (!oaWriteByte(out_buf, pos, oaHexUpper(c & 0x0F))) return false;
        }
    }
    return true;
}

fn oaWriteJsonEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    if (!oaWriteByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!oaWriteRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!oaWriteRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!oaWriteRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!oaWriteRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!oaWriteRaw(out_buf, pos, "\\t")) return false;
        } else if (b < 0x20) {
            if (!oaWriteRaw(out_buf, pos, "\\u00")) return false;
            if (!oaWriteByte(out_buf, pos, oaHexLower((b >> 4) & 0x0F))) return false;
            if (!oaWriteByte(out_buf, pos, oaHexLower(b & 0x0F))) return false;
        } else {
            if (!oaWriteByte(out_buf, pos, b)) return false;
        }
    }
    return oaWriteByte(out_buf, pos, '"');
}

fn oaWriteU64(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        return oaWriteByte(out_buf, pos, '0');
    }
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        if (!oaWriteByte(out_buf, pos, tmp[n])) return false;
    }
    return true;
}

fn oaHasQueryMark(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '?') return true;
    }
    return false;
}

/// Build an RFC 6749 + RFC 7636 PKCE (S256) authorization redirect URL into `out_buf`.
pub fn oauth2BuildAuthorizeUrl(
    cfg: OAuth2Config,
    state: string,
    code_challenge: string,
    out_buf: []u8,
): usize? {
    if (cfg.authorize_url.len == 0 || cfg.client_id.len == 0) return null;
    if (state.len == 0 || code_challenge.len == 0) return null;

    let pos: usize = 0;
    if (!oaWriteRaw(out_buf, *pos, cfg.authorize_url)) return null;
    if (oaHasQueryMark(cfg.authorize_url)) {
        if (!oaWriteByte(out_buf, *pos, '&')) return null;
    } else {
        if (!oaWriteByte(out_buf, *pos, '?')) return null;
    }
    if (!oaWriteRaw(out_buf, *pos, "response_type=code&client_id=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, cfg.client_id)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&redirect_uri=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, cfg.redirect_uri)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&scope=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, cfg.scope)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&state=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, state)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&code_challenge=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, code_challenge)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&code_challenge_method=S256")) return null;
    return pos;
}

/// Register a one-time CSRF `state` token for later callback verification.
pub fn oauth2RegisterState(state: string) {
    if (state.len == 0 || state.len > OA_MAX_STATE_LEN) return;

    let i: usize = 0;
    let free_idx: usize = OA_MAX_STATES;
    while (i < OA_MAX_STATES) : (i += 1) {
        if (oa_states[i].active) {
            if (oaBytesEql(oa_states[i].buf[0..oa_states[i].len], state)) {
                return;
            }
        } else if (free_idx == OA_MAX_STATES) {
            free_idx = i;
        }
    }

    let slot_idx: usize = free_idx;
    if (slot_idx == OA_MAX_STATES) {
        slot_idx = oa_state_next;
        oa_state_next = (oa_state_next + 1) % OA_MAX_STATES;
    }

    let j: usize = 0;
    while (j < state.len) : (j += 1) {
        oa_states[slot_idx].buf[j] = state[j];
    }
    oa_states[slot_idx].len = state.len;
    oa_states[slot_idx].active = true;
}

/// Verify and consume a one-time CSRF `state` token (prevents replay).
pub fn oauth2ConsumeState(state: string): bool {
    if (state.len == 0 || state.len > OA_MAX_STATE_LEN) return false;
    let i: usize = 0;
    while (i < OA_MAX_STATES) : (i += 1) {
        if (oa_states[i].active && oaBytesEql(oa_states[i].buf[0..oa_states[i].len], state)) {
            oa_states[i].active = false;
            oa_states[i].len = 0;
            return true;
        }
    }
    return false;
}

/// Clear all registered OAuth2 states (useful for test isolation).
pub fn oauth2ResetStates() {
    let i: usize = 0;
    while (i < OA_MAX_STATES) : (i += 1) {
        oa_states[i].active = false;
        oa_states[i].len = 0;
    }
    oa_state_next = 0;
}

/// Build an `application/x-www-form-urlencoded` token exchange body with PKCE `code_verifier`.
pub fn oauth2BuildTokenFormBody(
    cfg: OAuth2Config,
    code: string,
    code_verifier: string,
    out_buf: []u8,
): usize? {
    if (cfg.client_id.len == 0 || code.len == 0 || code_verifier.len == 0) return null;

    let pos: usize = 0;
    if (!oaWriteRaw(out_buf, *pos, "grant_type=authorization_code&client_id=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, cfg.client_id)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&redirect_uri=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, cfg.redirect_uri)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&code=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, code)) return null;
    if (!oaWriteRaw(out_buf, *pos, "&code_verifier=")) return null;
    if (!oaWriteUrlEncoded(out_buf, *pos, code_verifier)) return null;
    return pos;
}

fn oaLoginHandler(ctx: *Context): Response {
    let state_val: string = "";
    if (q_state := ctx.query("state")) {
        if (q_state.len > 0) {
            state_val = q_state;
        }
    }
    if (state_val.len == 0) {
        let spos: usize = 0;
        * = oaWriteRaw(oa_auto_state_buf[0..], *spos, "oa-state-");
        * = oaWriteU64(oa_auto_state_buf[0..], *spos, oa_auto_seq);
        oa_auto_seq += 1;
        state_val = oa_auto_state_buf[0..spos];
    }
    oauth2RegisterState(state_val);

    let challenge_val: string = "";
    if (q_ch := ctx.query("code_challenge")) {
        if (q_ch.len > 0) {
            challenge_val = q_ch;
        }
    }
    if (challenge_val.len == 0) {
        let cpos: usize = 0;
        * = oaWriteRaw(oa_auto_challenge_buf[0..], *cpos, "pkce-challenge-");
        * = oaWriteU64(oa_auto_challenge_buf[0..], *cpos, oa_auto_seq);
        challenge_val = oa_auto_challenge_buf[0..cpos];
    }

    if (url_len := oauth2BuildAuthorizeUrl(oa_cfg, state_val, challenge_val, oa_location_buf[0..])) {
        return Response.redirect(302, oa_location_buf[0..url_len]);
    }
    return Response.json(500, "{\"error\":\"oauth2_config_error\"}");
}

fn oaCallbackHandler(ctx: *Context): Response {
    let code_val: string = "";
    if (q_code := ctx.query("code")) {
        code_val = q_code;
    }
    let state_val: string = "";
    if (q_state := ctx.query("state")) {
        state_val = q_state;
    }

    if (code_val.len == 0 || state_val.len == 0) {
        return Response.json(400, "{\"error\":\"invalid_request\",\"verified\":false}");
    }

    if (!oauth2ConsumeState(state_val)) {
        return Response.json(403, "{\"error\":\"invalid_state\",\"verified\":false}");
    }

    let pos: usize = 0;
    if (!oaWriteRaw(oa_resp_buf[0..], *pos, "{\"code\":")) {
        return Response.json(500, "{\"error\":\"buffer_overflow\"}");
    }
    if (!oaWriteJsonEscaped(oa_resp_buf[0..], *pos, code_val)) {
        return Response.json(500, "{\"error\":\"buffer_overflow\"}");
    }
    if (!oaWriteRaw(oa_resp_buf[0..], *pos, ",\"verified\":true}")) {
        return Response.json(500, "{\"error\":\"buffer_overflow\"}");
    }
    return Response.json(200, oa_resp_buf[0..pos]);
}

/// Register OAuth 2.0 + PKCE login (`GET cfg.login_path`) and callback (`GET cfg.callback_path`) routes.
pub fn registerOAuth2(app: *App, cfg: OAuth2Config): *App {
    oa_cfg = cfg;
    let p = Plugin.init("@web/oauth2");
    * = p.withScope(PluginScope.Global);
    * = p.route(AppMethod.GET, cfg.login_path, "OAuth2 PKCE Login", &oaLoginHandler);
    * = p.route(AppMethod.GET, cfg.callback_path, "OAuth2 PKCE Callback", &oaCallbackHandler);
    return app.usePlugin(&p);
}
