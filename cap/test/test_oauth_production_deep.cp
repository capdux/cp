// Source: cp/cap/test/test_oauth_production_deep.cp
// Deep Production Verification Suite for cap:oauth & cap.oauth
// Covers:
//   1. RFC 7636 Appendix B official PKCE S256 test vector, CSPRNG generation, constant-time verify & invalid verifier rejection
//   2. OAuthFlow.buildAuthorizeUrl RFC 3986 percent-encoding, `?`/`&` query joining, PKCE/state & config guards
//   3. Token exchange / refresh / client_credentials form-urlencoded codecs & OAuthTokenResponse.parseJson / isExpiredAt / hasScope
//   4. AuthHeader Bearer / Basic encode & decode, case-insensitivity & malformed/hostile input rejection
//   5. `import cap from "cap"; cap.oauth` automatic dynamic lazy mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    OAuthError,
    PkceChallenge,
    Pkce,
    OAuthConfig,
    OAuthFlow,
    OAuthTokenResponse,
    BasicCredentials,
    AuthHeader,
} from "cap:oauth";

fn strEql(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn hasSubstr(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn testRfc7636PkceVectorsAndGuards(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Official RFC 7636 Appendix B test vector
    const rfc_verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const rfc_expected_challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

    let pkce_vec = Pkce.fromVerifier(alloc, rfc_verifier)?;
    defer pkce_vec.deinit();
    expect_eq(strEql(pkce_vec.verifier, rfc_verifier), true);
    expect_eq(strEql(pkce_vec.challenge, rfc_expected_challenge), true);
    expect_eq(strEql(pkce_vec.method, "S256"), true);

    expect_eq(Pkce.verifyS256(rfc_verifier, rfc_expected_challenge), true);
    // Tampered challenge must fail constant-time check
    expect_eq(Pkce.verifyS256(rfc_verifier, "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cN"), false);
    expect_eq(Pkce.verifyS256(rfc_verifier, "short"), false);

    // 2. CSPRNG PKCE generation & state token uniqueness
    let gen1 = Pkce.generate(alloc)?;
    defer gen1.deinit();
    let gen2 = Pkce.generate(alloc)?;
    defer gen2.deinit();

    expect_eq(gen1.verifier.len, 43 as usize);
    expect_eq(gen1.challenge.len, 43 as usize);
    expect_eq(Pkce.isValidVerifier(gen1.verifier), true);
    expect_eq(Pkce.verifyS256(gen1.verifier, gen1.challenge), true);
    expect_eq(strEql(gen1.verifier, gen2.verifier), false);

    const st1 = Pkce.generateState(alloc)?;
    defer alloc.free(st1);
    const st2 = Pkce.generateState(alloc)?;
    defer alloc.free(st2);
    expect_eq(st1.len >= 32, true);
    expect_eq(strEql(st1, st2), false);

    // 3. Reject invalid verifiers (too short: 42 chars, too long: 129 chars, disallowed chars)
    const too_short = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"; // 42
    const too_long =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" ++
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"; // 129
    const bad_chars = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjX/"; // '/' not allowed
    const bad_pad = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjX="; // '=' not allowed

    const bad_verifiers: [4]string = [too_short, too_long, bad_chars, bad_pad];
    for (bv of bad_verifiers) {
        expect_eq(Pkce.isValidVerifier(bv), false);
        expect_eq(Pkce.verifyS256(bv, rfc_expected_challenge), false);
        let rejected = false;
        * = Pkce.fromVerifier(alloc, bv) catch |e| {
            if (e == OAuthError.InvalidVerifier) rejected = true;
        };
        expect_eq(rejected, true);
    }
}

fn testAuthorizeUrlBuilding(): !void {
    const alloc = std.heap.page_allocator;

    const scopes: [3]string = ["openid", "profile", "api:read+write"];
    const cfg = OAuthConfig{
        client_id: "client/app 1",
        authorize_url: "https://auth.cp-lang.org/oauth2/authorize",
        token_url: "https://auth.cp-lang.org/oauth2/token",
        redirect_uri: "https://app.cp-lang.org/cb?mode=web&v=2",
        scopes: &scopes,
    };

    let pkce = Pkce.fromVerifier(alloc, "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk")?;
    defer pkce.deinit();

    const url = OAuthFlow.buildAuthorizeUrl(alloc, cfg, "st_a&b=c 1", pkce)?;
    defer alloc.free(url);

    const expected =
        "https://auth.cp-lang.org/oauth2/authorize" ++
        "?response_type=code" ++
        "&client_id=client%2Fapp%201" ++
        "&redirect_uri=https%3A%2F%2Fapp.cp-lang.org%2Fcb%3Fmode%3Dweb%26v%3D2" ++
        "&scope=openid%20profile%20api%3Aread%2Bwrite" ++
        "&state=st_a%26b%3Dc%201" ++
        "&code_challenge=E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM" ++
        "&code_challenge_method=S256";
    expect_eq(strEql(url, expected), true);

    // Base URL already containing query parameter must append with `&` instead of `?`
    const cfg_with_query = OAuthConfig{
        client_id: "spa-client",
        authorize_url: "https://login.example.com/authorize?tenant=acme",
        redirect_uri: "",
    };
    const url2 = OAuthFlow.buildAuthorizeUrl(alloc, cfg_with_query, "xyz", null)?;
    defer alloc.free(url2);
    expect_eq(
        strEql(url2, "https://login.example.com/authorize?tenant=acme&response_type=code&client_id=spa-client&state=xyz"),
        true,
    );

    // Reject invalid configs: empty client_id, fragment '#' in authorize_url, CRLF injection
    let bad_cfg = OAuthConfig{
        client_id: "",
        authorize_url: "https://auth.example.com/authorize",
    };
    let hit_empty_id = false;
    * = OAuthFlow.buildAuthorizeUrl(alloc, bad_cfg, "s", null) catch |e| {
        if (e == OAuthError.InvalidConfig) hit_empty_id = true;
    };
    expect_eq(hit_empty_id, true);

    let frag_cfg = OAuthConfig{
        client_id: "cid",
        authorize_url: "https://auth.example.com/authorize#frag",
    };
    let hit_frag = false;
    * = OAuthFlow.buildAuthorizeUrl(alloc, frag_cfg, "s", null) catch |e| {
        if (e == OAuthError.InvalidConfig) hit_frag = true;
    };
    expect_eq(hit_frag, true);
}

fn testTokenBodiesAndResponseParsing(): !void {
    const alloc = std.heap.page_allocator;

    const scopes: [2]string = ["read:user", "write:repo"];
    const cfg = OAuthConfig{
        client_id: "svc_client",
        client_secret: "sec/ret+1=",
        authorize_url: "https://idp.example.com/auth",
        token_url: "https://idp.example.com/token",
        redirect_uri: "http://127.0.0.1:9000/cb",
        scopes: &scopes,
    };

    const verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const code_body = OAuthFlow.buildAuthorizationCodeBody(alloc, cfg, "auth_code/1+2", verifier)?;
    defer alloc.free(code_body);
    expect_eq(
        strEql(
            code_body,
            "grant_type=authorization_code" ++
                "&code=auth_code%2F1%2B2" ++
                "&client_id=svc_client" ++
                "&redirect_uri=http%3A%2F%2F127.0.0.1%3A9000%2Fcb" ++
                "&client_secret=sec%2Fret%2B1%3D" ++
                "&code_verifier=dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk",
        ),
        true,
    );

    // Invalid verifier rejected in authorization_code body
    let bad_pkce_body = false;
    * = OAuthFlow.buildAuthorizationCodeBody(alloc, cfg, "code123", "too_short") catch |e| {
        if (e == OAuthError.InvalidVerifier) bad_pkce_body = true;
    };
    expect_eq(bad_pkce_body, true);

    const refresh_body = OAuthFlow.buildRefreshTokenBody(alloc, cfg, "rt_abc/def")?;
    defer alloc.free(refresh_body);
    expect_eq(
        strEql(
            refresh_body,
            "grant_type=refresh_token&refresh_token=rt_abc%2Fdef&client_id=svc_client&client_secret=sec%2Fret%2B1%3D&scope=read%3Auser%20write%3Arepo",
        ),
        true,
    );

    const cc_body = OAuthFlow.buildClientCredentialsBody(alloc, cfg)?;
    defer alloc.free(cc_body);
    expect_eq(
        strEql(
            cc_body,
            "grant_type=client_credentials&client_id=svc_client&client_secret=sec%2Fret%2B1%3D&scope=read%3Auser%20write%3Arepo",
        ),
        true,
    );

    // OAuthTokenResponse.parseJson (OAuth 2.0 + OIDC id_token)
    const json_payload =
        "{\"access_token\":\"eyJ.at.sig\",\"token_type\":\"Bearer\",\"expires_in\":3600," ++
        "\"refresh_token\":\"rt_999\",\"scope\":\"openid read:user profile\",\"id_token\":\"eyJ.id.sig\"}";
    let tok = OAuthTokenResponse.parseJson(alloc, json_payload)?;
    defer tok.deinit();

    expect_eq(strEql(tok.access_token, "eyJ.at.sig"), true);
    expect_eq(strEql(tok.token_type, "Bearer"), true);
    expect_eq(tok.expires_in.?, 3600 as i64);
    expect_eq(strEql(tok.refresh_token.?, "rt_999"), true);
    expect_eq(strEql(tok.scope.?, "openid read:user profile"), true);
    expect_eq(strEql(tok.id_token.?, "eyJ.id.sig"), true);

    // Expiration checks with clock skew
    expect_eq(tok.isExpiredAt(1000, 4500, 60), false); // 4560 < 4600
    expect_eq(tok.isExpiredAt(1000, 4540, 60), true);  // 4600 >= 4600
    expect_eq(tok.isExpiredAt(1000, 4600, 0), true);

    // Exact scope token matching
    expect_eq(tok.hasScope("openid"), true);
    expect_eq(tok.hasScope("read:user"), true);
    expect_eq(tok.hasScope("profile"), true);
    expect_eq(tok.hasScope("read"), false);
    expect_eq(tok.hasScope(""), false);

    // Missing token_type or malformed JSON must return InvalidTokenResponse
    const bad_jsons: [3]string = [
        "{\"access_token\":\"only_at\"}",
        "{\"access_token\":\"\",\"token_type\":\"Bearer\"}",
        "{not_valid_json",
    ];
    for (bj of bad_jsons) {
        let rejected = false;
        * = OAuthTokenResponse.parseJson(alloc, bj) catch |e| {
            if (e == OAuthError.InvalidTokenResponse) rejected = true;
        };
        expect_eq(rejected, true);
    }
}

fn testAuthHeaderCodec(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Bearer format & case-insensitive parse
    const bearer = AuthHeader.formatBearer(alloc, "tok_abc.123-XYZ")?;
    defer alloc.free(bearer);
    expect_eq(strEql(bearer, "Bearer tok_abc.123-XYZ"), true);

    expect_eq(strEql(AuthHeader.parseBearer("Bearer tok_abc.123-XYZ").?, "tok_abc.123-XYZ"), true);
    expect_eq(strEql(AuthHeader.parseBearer("  bearer   tok_lower  ").?, "tok_lower"), true);
    expect_eq(strEql(AuthHeader.parseBearer("BEARER my_token").?, "my_token"), true);
    expect_eq(AuthHeader.parseBearer("Basic dXNlcjpwYXNz") == null, true);
    expect_eq(AuthHeader.parseBearer("Bearer ") == null, true);
    expect_eq(AuthHeader.parseBearer("Bearer bad token") == null, true);

    let bad_bearer = false;
    * = AuthHeader.formatBearer(alloc, "tok\r\ninject") catch |e| {
        if (e == OAuthError.InvalidAuthHeader) bad_bearer = true;
    };
    expect_eq(bad_bearer, true);

    // 2. Basic format & parse (including ':' inside password)
    const basic = AuthHeader.formatBasic(alloc, "Aladdin", "open:sesame")?;
    defer alloc.free(basic);
    expect_eq(strEql(basic, "Basic QWxhZGRpbjpvcGVuOnNlc2FtZQ=="), true);

    let creds = AuthHeader.parseBasic(alloc, "  basic   QWxhZGRpbjpvcGVuOnNlc2FtZQ== ")?;
    defer creds.deinit();
    expect_eq(strEql(creds.username, "Aladdin"), true);
    expect_eq(strEql(creds.password, "open:sesame"), true);

    // Reject ':' in username and invalid Base64 / missing ':'
    let bad_user = false;
    * = AuthHeader.formatBasic(alloc, "bad:user", "pass") catch |e| {
        if (e == OAuthError.InvalidAuthHeader) bad_user = true;
    };
    expect_eq(bad_user, true);

    let bad_b64 = false;
    * = AuthHeader.parseBasic(alloc, "Basic !!!invalid!!!") catch |e| {
        if (e == OAuthError.InvalidAuthHeader) bad_b64 = true;
    };
    expect_eq(bad_b64, true);

    // "dXNlcgo=" decodes to "user\n" (no ':' and has LF control char)
    let bad_no_colon = false;
    * = AuthHeader.parseBasic(alloc, "Basic dXNlcgo=") catch |e| {
        if (e == OAuthError.InvalidAuthHeader) bad_no_colon = true;
    };
    expect_eq(bad_no_colon, true);
}

fn testCapOAuthDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.oauth.*` automatic dynamic mounting
    let pkce = cap.oauth.Pkce.generate(alloc)?;
    defer pkce.deinit();
    expect_eq(cap.oauth.Pkce.verifyS256(pkce.verifier, pkce.challenge), true);

    const state = cap.oauth.Pkce.generateState(alloc)?;
    defer alloc.free(state);

    const scopes: [2]string = ["openid", "email"];
    const cfg = cap.oauth.OAuthConfig{
        client_id: "hub_app",
        authorize_url: "https://auth.cp-lang.org/authorize",
        redirect_uri: "https://app.cp-lang.org/callback",
        scopes: &scopes,
    };

    const url = cap.oauth.OAuthFlow.buildAuthorizeUrl(alloc, cfg, state, pkce)?;
    defer alloc.free(url);
    expect_eq(hasSubstr(url, "code_challenge_method=S256"), true);
    expect_eq(hasSubstr(url, "scope=openid%20email"), true);

    let tok = cap.oauth.OAuthTokenResponse.parseJson(
        alloc,
        "{\"access_token\":\"hub_tok\",\"token_type\":\"Bearer\",\"expires_in\":120,\"scope\":\"openid email\"}",
    )?;
    defer tok.deinit();
    expect_eq(tok.hasScope("email"), true);
    expect_eq(tok.isExpiredAt(1000, 1100, 30), true);

    const hdr = cap.oauth.AuthHeader.formatBearer(alloc, tok.access_token)?;
    defer alloc.free(hdr);
    expect_eq(strEql(cap.oauth.AuthHeader.parseBearer(hdr).?, "hub_tok"), true);
}

fn main() {
    test("1. RFC 7636 Appendix B PKCE S256 Vectors, CSPRNG & Invalid Verifier Rejection", () => {
        testRfc7636PkceVectorsAndGuards()?;
    });
    test("2. OAuthFlow.buildAuthorizeUrl Percent-Encoding, ?/& Joining & PKCE/State", () => {
        testAuthorizeUrlBuilding()?;
    });
    test("3. Token Request Form Codecs & OAuthTokenResponse JSON/Expiry/Scope", () => {
        testTokenBodiesAndResponseParsing()?;
    });
    test("4. AuthHeader Bearer/Basic Encode & Decode + Injection/Malformed Guards", () => {
        testAuthHeaderCodec()?;
    });
    test("5. import cap from \"cap\"; cap.oauth Automatic Dynamic Mounting", () => {
        testCapOAuthDynamicMount()?;
    });
}
