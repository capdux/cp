// Source: cp/cap/test/test_oauth2_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:oauth2 & cap.oauth2

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    OAuth2Error,
    PkceMethod,
    PkcePair,
    OAuth2Config,
    ExtraParam,
    CallbackResult,
    TokenResponse,
    isValidCodeVerifier,
    generateCodeVerifier,
    deriveCodeChallengeS256,
    generatePkcePair,
    verifyPkceChallenge,
    buildAuthorizeUrl,
    parseCallbackQuery,
    verifyCallbackState,
    buildAuthorizationCodeBody,
    buildClientCredentialsBody,
    buildRefreshTokenBody,
    buildRevocationBody,
    buildBasicAuthHeader,
    parseTokenResponse,
    parseTokenResponseOrError,
} from "cap:oauth2";

// Test 1: RFC 7636 Appendix B Official Vector, CSPRNG generation & invalid verifier rejection
fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // RFC 7636 Appendix B exact test vector
    const rfc_verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const rfc_expected_challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

    expect_eq(isValidCodeVerifier(rfc_verifier), true);
    const derived = deriveCodeChallengeS256(alloc, rfc_verifier)?;
    defer alloc.free(derived);
    expect_eq(std.mem.eql(u8, derived, rfc_expected_challenge), true);
    expect_eq(verifyPkceChallenge(rfc_verifier, rfc_expected_challenge, .S256), true);
    expect_eq(verifyPkceChallenge(rfc_verifier, rfc_verifier, .Plain), true);
    expect_eq(verifyPkceChallenge(rfc_verifier, "wrong-challenge-xxxxxxxxxxxxxxxxxxxxxxxxxxx", .S256), false);

    // Invalid verifier length & charset rejection
    expect_eq(isValidCodeVerifier("too_short_42_chars_xxxxxxxxxxxxxxxxxxxxxxx"), false);
    let short_err = false;
    deriveCodeChallengeS256(alloc, "short") catch |e| {
        if (e == OAuth2Error.InvalidVerifier) short_err = true;
    };
    expect_eq(short_err, true);

    const bad_charset = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjX!";
    expect_eq(isValidCodeVerifier(bad_charset), false);
    expect_eq(verifyPkceChallenge(bad_charset, rfc_expected_challenge, .S256), false);

    // CSPRNG PKCE pair generation
    let pair = generatePkcePair(alloc, .S256, 64)?;
    defer pair.deinit(alloc);
    expect_eq(pair.code_verifier.len, 64);
    expect_eq(pair.code_challenge.len, 43);
    expect_eq(isValidCodeVerifier(pair.code_verifier), true);
    expect_eq(verifyPkceChallenge(pair.code_verifier, pair.code_challenge, .S256), true);

    const v43 = generateCodeVerifier(alloc, 43)?;
    defer alloc.free(v43);
    expect_eq(v43.len, 43);
    expect_eq(isValidCodeVerifier(v43), true);
}

// Test 2: Authorize URL construction (RFC 3986) & Callback parsing + CSRF state check
fn runTest2(): !void {
    const alloc = std.testing.allocator;

    const scopes = [ "openid", "profile", "email" ];
    const cfg = OAuth2Config{
        .client_id = "app+client/123",
        .authorize_url = "https://auth.example.com/oauth2/authorize",
        .token_url = "https://auth.example.com/oauth2/token",
        .redirect_uri = "https://myapp.local/cb?env=prod",
        .scopes = &scopes,
    };
    const extras = [
        ExtraParam{ .name = "prompt", .value = "consent" },
        ExtraParam{ .name = "login_hint", .value = "alice@example.com" },
    ];

    const url = buildAuthorizeUrl(
        alloc,
        cfg,
        "state_xyz_987",
        "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM",
        "nonce_abc_111",
        &extras,
    )?;
    defer alloc.free(url);

    expect_eq(std.mem.startsWith(u8, url, "https://auth.example.com/oauth2/authorize?response_type=code"), true);
    expect_eq(std.mem.indexOf(u8, url, "client_id=app%2Bclient%2F123") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "redirect_uri=https%3A%2F%2Fmyapp.local%2Fcb%3Fenv%3Dprod") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "scope=openid%20profile%20email") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "state=state_xyz_987") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "code_challenge=E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "code_challenge_method=S256") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "nonce=nonce_abc_111") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "login_hint=alice%40example.com") != null, true);

    // Success callback parsing + constant-time state verification
    let cb_ok = parseCallbackQuery(alloc, "https://myapp.local/cb?code=auth%2Fcode_123&state=state_xyz_987#frag")?;
    defer cb_ok.deinit(alloc);
    expect_eq(cb_ok.isSuccess(), true);
    expect_eq(cb_ok.is_error, false);
    expect_eq(std.mem.eql(u8, cb_ok.code.?, "auth/code_123"), true);
    expect_eq(cb_ok.verifyState("state_xyz_987"), true);
    expect_eq(cb_ok.verifyState("state_xyz_988"), false);
    expect_eq(verifyCallbackState("state_xyz_987", cb_ok.state), true);
    expect_eq(verifyCallbackState("", cb_ok.state), false);

    // Error callback parsing
    let cb_err = parseCallbackQuery(alloc, "?error=access_denied&error_description=User+cancelled+login&error_uri=https%3A%2F%2Fauth.example.com%2Ferr&state=state_xyz_987")?;
    defer cb_err.deinit(alloc);
    expect_eq(cb_err.isSuccess(), false);
    expect_eq(cb_err.is_error, true);
    expect_eq(std.mem.eql(u8, cb_err.error_code.?, "access_denied"), true);
    expect_eq(std.mem.eql(u8, cb_err.error_description.?, "User cancelled login"), true);
    expect_eq(std.mem.eql(u8, cb_err.error_uri.?, "https://auth.example.com/err"), true);
}

// Test 3: Token Grant Request Bodies (auth_code, client_credentials, refresh_token, revocation) & Basic Auth
fn runTest3(): !void {
    const alloc = std.testing.allocator;

    const scopes = [ "read:data", "write:data" ];
    const cfg = OAuth2Config{
        .client_id = "my_client",
        .client_secret = "s3cr3t+key/==",
        .authorize_url = "https://auth.example.com/authorize",
        .token_url = "https://auth.example.com/token",
        .redirect_uri = "https://app.local/callback",
        .scopes = &scopes,
    };

    const verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const auth_body = buildAuthorizationCodeBody(alloc, cfg, "code_abc", verifier)?;
    defer alloc.free(auth_body);
    expect_eq(std.mem.indexOf(u8, auth_body, "grant_type=authorization_code") != null, true);
    expect_eq(std.mem.indexOf(u8, auth_body, "code=code_abc") != null, true);
    expect_eq(std.mem.indexOf(u8, auth_body, "code_verifier=dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk") != null, true);
    expect_eq(std.mem.indexOf(u8, auth_body, "client_secret=s3cr3t%2Bkey%2F%3D%3D") != null, true);

    const empty_scopes: []const ([]const u8) = [];
    const cc_body = buildClientCredentialsBody(alloc, cfg, empty_scopes)?;
    defer alloc.free(cc_body);
    expect_eq(std.mem.indexOf(u8, cc_body, "grant_type=client_credentials") != null, true);
    expect_eq(std.mem.indexOf(u8, cc_body, "scope=read%3Adata%20write%3Adata") != null, true);

    const refresh_scopes = [ "read:data" ];
    const rt_body = buildRefreshTokenBody(alloc, cfg, "rt_xyz_999", &refresh_scopes)?;
    defer alloc.free(rt_body);
    expect_eq(std.mem.indexOf(u8, rt_body, "grant_type=refresh_token") != null, true);
    expect_eq(std.mem.indexOf(u8, rt_body, "refresh_token=rt_xyz_999") != null, true);
    expect_eq(std.mem.indexOf(u8, rt_body, "scope=read%3Adata") != null, true);

    const rev_body = buildRevocationBody(alloc, "tok_123", "refresh_token", "my_client", null)?;
    defer alloc.free(rev_body);
    expect_eq(std.mem.eql(u8, rev_body, "token=tok_123&token_type_hint=refresh_token&client_id=my_client"), true);

    const basic_hdr = buildBasicAuthHeader(alloc, "user@1", "pass+2")?;
    defer alloc.free(basic_hdr);
    // "user%401:pass%2B2" -> base64 "dXNlciU0MDE6cGFzcyUyQjI="
    expect_eq(std.mem.eql(u8, basic_hdr, "Basic dXNlciU0MDE6cGFzcyUyQjI="), true);
}

// Test 4: TokenResponse JSON parsing, Scope matching, Expiration check & OAuth2 error interception
fn runTest4(): !void {
    const alloc = std.testing.allocator;

    const ok_json = "{\"access_token\":\"eyJ.abc.def\",\"token_type\":\"Bearer\",\"expires_in\":3600,\"refresh_token\":\"rt_123\",\"scope\":\"openid profile read\",\"id_token\":\"eyJ.id.tok\"}";
    let tok = parseTokenResponse(alloc, ok_json)?;
    defer tok.deinit(alloc);

    expect_eq(std.mem.eql(u8, tok.access_token, "eyJ.abc.def"), true);
    expect_eq(std.mem.eql(u8, tok.token_type, "Bearer"), true);
    expect_eq(tok.expires_in.?, 3600);
    expect_eq(std.mem.eql(u8, tok.refresh_token.?, "rt_123"), true);
    expect_eq(std.mem.eql(u8, tok.id_token.?, "eyJ.id.tok"), true);

    expect_eq(tok.hasScope("openid"), true);
    expect_eq(tok.hasScope("profile"), true);
    expect_eq(tok.hasScope("read"), true);
    expect_eq(tok.hasScope("rea"), false);
    expect_eq(tok.hasScope("write"), false);

    tok.setIssuedAt(1000);
    expect_eq(tok.isExpired(4500, 30), false);
    expect_eq(tok.isExpired(4570, 30), true);
    expect_eq(tok.isExpired(4600, 0), true);

    // OAuth2 error JSON interception
    const err_json = "{\"error\":\"invalid_grant\",\"error_description\":\"Code expired\",\"error_uri\":\"https://auth.example.com/docs\"}";
    let caught_oauth_err = false;
    parseTokenResponse(alloc, err_json) catch |e| {
        if (e == OAuth2Error.OAuthErrorResponse) caught_oauth_err = true;
    };
    expect_eq(caught_oauth_err, true);

    let err_detail = parseTokenResponseOrError(alloc, err_json)?;
    defer err_detail.deinit(alloc);
    expect_eq(std.mem.eql(u8, err_detail.oauth_error.?, "invalid_grant"), true);
    expect_eq(std.mem.eql(u8, err_detail.error_description.?, "Code expired"), true);
}

// Test 5: Dynamic cap.oauth2 mounting & std.testing.allocator zero-leak verification
fn runTest5(): !void {
    const alloc = std.testing.allocator;

    let pair = cap.oauth2.generatePkcePair(alloc, .S256, 43)?;
    defer pair.deinit(alloc);
    expect_eq(cap.oauth2.verifyPkceChallenge(pair.code_verifier, pair.code_challenge, .S256), true);

    const cfg = cap.oauth2.OAuth2Config{
        .client_id = "hub_client",
        .authorize_url = "https://idp.example.com/auth",
        .token_url = "https://idp.example.com/token",
    };
    const empty_extra: []const cap.oauth2.ExtraParam = [];
    const auth_url = cap.oauth2.buildAuthorizeUrl(alloc, cfg, "st_hub", pair.code_challenge, null, empty_extra)?;
    defer alloc.free(auth_url);
    expect_eq(std.mem.indexOf(u8, auth_url, "client_id=hub_client") != null, true);

    let cb = cap.oauth2.parseCallbackQuery(alloc, "code=hub_code&state=st_hub")?;
    defer cb.deinit(alloc);
    expect_eq(cb.verifyState("st_hub"), true);

    const hdr = cap.oauth2.buildBasicAuthHeader(alloc, "hub_client", "hub_secret")?;
    defer alloc.free(hdr);
    expect_eq(std.mem.startsWith(u8, hdr, "Basic "), true);
}

test("oauth2: RFC 7636 PKCE vector, CSPRNG & invalid verifier rejection") {
    runTest1() catch unreachable;
}

test("oauth2: authorize URL builder, callback parser & CSRF state verification") {
    runTest2() catch unreachable;
}

test("oauth2: grant token bodies, revocation & RFC 6749 Basic Auth header") {
    runTest3() catch unreachable;
}

test("oauth2: TokenResponse JSON parsing, scope, expiration & error interception") {
    runTest4() catch unreachable;
}

test("oauth2: cap.oauth2 dynamic hub mount & std.testing.allocator zero leaks") {
    runTest5() catch unreachable;
}
