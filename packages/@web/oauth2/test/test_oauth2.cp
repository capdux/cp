// Self-contained unit & integration tests for @web/oauth2

const std = @import("std");
import { App, AppMethod, Request } from "../../../../lib/web/app.cp";
import {
    OAuth2Config,
    oauth2BuildAuthorizeUrl,
    oauth2RegisterState,
    oauth2ConsumeState,
    oauth2ResetStates,
    oauth2BuildTokenFormBody,
    registerOAuth2,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn contains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        let eq: bool = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                eq = false;
                break;
            }
        }
        if (eq) return true;
    }
    return false;
}

fn testAuthorizeUrlAndTokenBody() {
    const cfg = OAuth2Config{
        client_id: "cp-app-01",
        authorize_url: "https://idp.example.com/oauth2/authorize",
        token_url: "https://idp.example.com/oauth2/token",
        redirect_uri: "https://app.example.com/auth/callback",
        scope: "openid profile email",
    };

    let url_buf: [1024]u8 = [0; 1024];
    const url_len = oauth2BuildAuthorizeUrl(cfg, "st-abc-123", "ch-xyz-789", url_buf[0..]);
    assertTrue(url_len != null && url_len.? > 0, "authorize url built");
    const url = url_buf[0..url_len.?];
    assertTrue(contains(url, "https://idp.example.com/oauth2/authorize?response_type=code"), "has authorize_url & response_type=code");
    assertTrue(contains(url, "&client_id=cp-app-01"), "has client_id");
    assertTrue(contains(url, "&redirect_uri=https%3A%2F%2Fapp.example.com%2Fauth%2Fcallback"), "has encoded redirect_uri");
    assertTrue(contains(url, "&scope=openid%20profile%20email"), "has encoded scope");
    assertTrue(contains(url, "&state=st-abc-123"), "has state");
    assertTrue(contains(url, "&code_challenge=ch-xyz-789"), "has code_challenge");
    assertTrue(contains(url, "&code_challenge_method=S256"), "has S256 method");

    // Authorize URL that already has a query string should append with `&`
    const cfg2 = OAuth2Config{
        client_id: "cp-app-01",
        authorize_url: "https://idp.example.com/auth?prompt=consent",
        token_url: "https://idp.example.com/token",
        redirect_uri: "https://app.example.com/cb",
    };
    const url_len2 = oauth2BuildAuthorizeUrl(cfg2, "st2", "ch2", url_buf[0..]);
    assertTrue(url_len2 != null, "authorize url with existing query");
    assertTrue(contains(url_buf[0..url_len2.?], "?prompt=consent&response_type=code"), "appends with &");

    // Token form body
    let body_buf: [1024]u8 = [0; 1024];
    const body_len = oauth2BuildTokenFormBody(cfg, "auth-code-999", "verifier-secret-42", body_buf[0..]);
    assertTrue(body_len != null && body_len.? > 0, "token form body built");
    const body = body_buf[0..body_len.?];
    assertTrue(contains(body, "grant_type=authorization_code"), "has grant_type");
    assertTrue(contains(body, "&client_id=cp-app-01"), "body has client_id");
    assertTrue(contains(body, "&code=auth-code-999"), "body has code");
    assertTrue(contains(body, "&code_verifier=verifier-secret-42"), "body has code_verifier");
}

fn testStateReplayProtection() {
    oauth2ResetStates();
    oauth2RegisterState("csrf-token-1");
    assertTrue(oauth2ConsumeState("csrf-token-1"), "first consume succeeds");
    assertTrue(!oauth2ConsumeState("csrf-token-1"), "second consume rejected (replay)");
    assertTrue(!oauth2ConsumeState("unknown-token"), "unknown state rejected");
}

fn testOAuth2Routes() {
    oauth2ResetStates();
    let app = App.init();
    const cfg = OAuth2Config{
        client_id: "demo-client",
        authorize_url: "https://auth.cp.dev/authorize",
        token_url: "https://auth.cp.dev/token",
        redirect_uri: "https://demo.cp.dev/auth/callback",
    };
    * = registerOAuth2(*app, cfg);

    // 1. GET /auth/login -> 302 with Location
    const resp_login = app.dispatch(Request.init(AppMethod.GET, "/auth/login?state=login-st-1&code_challenge=pkce-ch-1"));
    assertTrue(resp_login.status == 302, "login returns 302");
    const loc = resp_login.header("Location");
    assertTrue(loc != null, "login sets Location header");
    assertTrue(contains(loc.?, "https://auth.cp.dev/authorize?response_type=code"), "Location has authorize url");
    assertTrue(contains(loc.?, "state=login-st-1"), "Location has state");
    assertTrue(contains(loc.?, "code_challenge=pkce-ch-1"), "Location has code_challenge");
    assertTrue(contains(loc.?, "code_challenge_method=S256"), "Location has S256");

    // 2. GET /auth/callback missing params -> 400
    const resp_missing = app.dispatch(Request.init(AppMethod.GET, "/auth/callback?state=login-st-1"));
    assertTrue(resp_missing.status == 400, "missing code returns 400");

    // 3. GET /auth/callback invalid state -> 403
    const resp_bad_state = app.dispatch(Request.init(AppMethod.GET, "/auth/callback?code=c1&state=forged"));
    assertTrue(resp_bad_state.status == 403, "forged state returns 403");

    // 4. GET /auth/callback valid code & registered state -> 200
    const resp_ok = app.dispatch(Request.init(AppMethod.GET, "/auth/callback?code=code-ok-77&state=login-st-1"));
    assertTrue(resp_ok.status == 200, "valid callback returns 200");
    assertTrue(contains(resp_ok.content_type, "application/json"), "callback json content-type");
    assertTrue(contains(resp_ok.body, "\"code\":\"code-ok-77\""), "callback json contains code");
    assertTrue(contains(resp_ok.body, "\"verified\":true"), "callback json verified:true");

    // 5. Replay same state -> 403
    const resp_replay = app.dispatch(Request.init(AppMethod.GET, "/auth/callback?code=code-ok-77&state=login-st-1"));
    assertTrue(resp_replay.status == 403, "replayed state returns 403");
}

pub fn main() {
    testAuthorizeUrlAndTokenBody();
    testStateReplayProtection();
    testOAuth2Routes();
    std.debug.print("ALL @web/oauth2 TESTS PASSED!\n", .{});
}
