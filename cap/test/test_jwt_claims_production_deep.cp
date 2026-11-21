import {
    splitJwt,
    decodeBase64Url,
    JwtHeader,
    JwtClaims,
    ClaimStatus,
} from "cap:jwt_claims";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("splitJwt and Base64URL decode standard RFC 7519 token", () => {
    // {"alg":"HS256","typ":"JWT"} . {"sub":"1234567890","iss":"cap-auth","exp":1700001000,"nbf":1700000000} . sig
    const token =
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9." ++
        "eyJzdWIiOiIxMjM0NTY3ODkwIiwiaXNzIjoiY2FwLWF1dGgiLCJleHAiOjE3MDAwMDEwMDAsIm5iZiI6MTcwMDAwMDAwMH0." ++
        "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c";

    const parts = splitJwt(token).?;
    expect(strEq(parts.header_b64, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"));

    let hbuf: [256]u8 = [0; 256];
    const hjson = decodeBase64Url(parts.header_b64, hbuf[0..]).?;
    const hdr = JwtHeader.parseJson(hjson);
    expect(strEq(hdr.alg.?, "HS256"));
    expect(strEq(hdr.typ.?, "JWT"));
    expect(!hdr.isNoneAlg());

    let pbuf: [256]u8 = [0; 256];
    const pjson = decodeBase64Url(parts.payload_b64, pbuf[0..]).?;
    const claims = JwtClaims.parseJson(pjson);
    expect(strEq(claims.sub.?, "1234567890"));
    expect(strEq(claims.iss.?, "cap-auth"));
    expect(claims.exp.? == 1700001000);
    expect(claims.nbf.? == 1700000000);

    // Time window & identity validation
    expect(claims.validate(1700000500, 0, "cap-auth", "1234567890", null) == ClaimStatus.valid);
    expect(claims.validate(1700001000, 0, "cap-auth", null, null) == ClaimStatus.expired);
    expect(claims.validate(1700001005, 10, "cap-auth", null, null) == ClaimStatus.valid);
    expect(claims.validate(1699999990, 0, null, null, null) == ClaimStatus.not_yet_valid);
    expect(claims.validate(1700000500, 0, "wrong-iss", null, null) == ClaimStatus.issuer_mismatch);
});

test("reject malformed tokens and detect alg=none", () => {
    expect(splitJwt("only.two") == null);
    expect(splitJwt("a.b.c.d") == null);
    expect(splitJwt(".b.c") == null);

    const none_hdr = JwtHeader.parseJson("{\"alg\":\"NoNe\",\"typ\":\"JWT\"}");
    expect(none_hdr.isNoneAlg());

    // Nested object before top-level claim must not confuse extractor
    const nested = "{\"meta\":{\"sub\":\"nested\"},\"sub\":\"real-user\",\"exp\":2000}";
    const claims = JwtClaims.parseJson(nested);
    expect(strEq(claims.sub.?, "real-user"));
    expect(claims.exp.? == 2000);
});
