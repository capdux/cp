// Source: cp/cap/test/test_http_digest_auth_production_deep.cp (Production deep tests for cap:http_digest_auth)

import { test, expect_eq } from "cap:test";
import {
    DigestChallenge,
    DigestCredentials,
    formatDigestHeader,
} from "cap:http_digest_auth";

test("http_digest_auth: DigestChallenge parses full RFC 7616 WWW-Authenticate header") {
    const hdr =
        "Digest realm=\"http-auth@example.org\", " ++
        "qop=\"auth, auth-int\", " ++
        "algorithm=SHA-256, " ++
        "nonce=\"7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v\", " ++
        "opaque=\"FQhe/qaU925kfnzjCev0ciny7QMkPqMAFRtzCUYo5tdS\", " ++
        "stale=TRUE, " ++
        "userhash=true";

    const ch = DigestChallenge.parse(hdr).?;
    expect_eq(std.mem.eql(u8, ch.realm, "http-auth@example.org"), true);
    expect_eq(std.mem.eql(u8, ch.qop.?, "auth, auth-int"), true);
    expect_eq(std.mem.eql(u8, ch.algorithm.?, "SHA-256"), true);
    expect_eq(std.mem.eql(u8, ch.nonce, "7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v"), true);
    expect_eq(std.mem.eql(u8, ch.opaque_val.?, "FQhe/qaU925kfnzjCev0ciny7QMkPqMAFRtzCUYo5tdS"), true);
    expect_eq(ch.stale, true);
    expect_eq(ch.userhash, true);
}

test("http_digest_auth: DigestCredentials parses full Authorization header") {
    const hdr =
        "digest username=\"Mufasa\", " ++
        "realm=\"http-auth@example.org\", " ++
        "uri=\"/dir/index.html\", " ++
        "algorithm=SHA-256, " ++
        "nonce=\"7ypf/xlj9XXwfDPE\", " ++
        "nc=00000001, " ++
        "cnonce=\"f2/wE4q74E6zIJEtWaHKaf5wv/H5QzzpXusqGemxURZJ\", " ++
        "qop=auth, " ++
        "response=\"753927fa0e85d155564e2e272a28d1802ca10daf4496794697cf8db5856cb6c1\", " ++
        "opaque=\"FQhe/qaU925kfnzj\"";

    const cr = DigestCredentials.parse(hdr).?;
    expect_eq(std.mem.eql(u8, cr.username, "Mufasa"), true);
    expect_eq(std.mem.eql(u8, cr.realm, "http-auth@example.org"), true);
    expect_eq(std.mem.eql(u8, cr.uri, "/dir/index.html"), true);
    expect_eq(std.mem.eql(u8, cr.algorithm.?, "SHA-256"), true);
    expect_eq(std.mem.eql(u8, cr.nonce, "7ypf/xlj9XXwfDPE"), true);
    expect_eq(std.mem.eql(u8, cr.nc.?, "00000001"), true);
    expect_eq(std.mem.eql(u8, cr.cnonce.?, "f2/wE4q74E6zIJEtWaHKaf5wv/H5QzzpXusqGemxURZJ"), true);
    expect_eq(std.mem.eql(u8, cr.qop.?, "auth"), true);
    expect_eq(std.mem.eql(u8, cr.opaque_val.?, "FQhe/qaU925kfnzj"), true);
}

test("http_digest_auth: formatDigestHeader -> DigestCredentials.parse exact round-trip") {
    const orig = DigestCredentials{
        username: "alice",
        realm: "api.cap.dev",
        nonce: "nonce123",
        uri: "/v1/orders",
        response: "deadbeef01234567",
        algorithm: "SHA-256",
        cnonce: "cnonce999",
        nc: "00000002",
        qop: "auth",
        opaque_val: "opq42",
    };

    let buf: [512]u8 = [0; 512];
    const header = formatDigestHeader(orig, buf[0..]).?;
    const parsed = DigestCredentials.parse(header).?;

    expect_eq(std.mem.eql(u8, parsed.username, orig.username), true);
    expect_eq(std.mem.eql(u8, parsed.realm, orig.realm), true);
    expect_eq(std.mem.eql(u8, parsed.nonce, orig.nonce), true);
    expect_eq(std.mem.eql(u8, parsed.uri, orig.uri), true);
    expect_eq(std.mem.eql(u8, parsed.response, orig.response), true);
    expect_eq(std.mem.eql(u8, parsed.algorithm.?, "SHA-256"), true);
    expect_eq(std.mem.eql(u8, parsed.cnonce.?, "cnonce999"), true);
    expect_eq(std.mem.eql(u8, parsed.nc.?, "00000002"), true);
    expect_eq(std.mem.eql(u8, parsed.qop.?, "auth"), true);
    expect_eq(std.mem.eql(u8, parsed.opaque_val.?, "opq42"), true);
}

test("http_digest_auth: rejects wrong scheme, missing required fields and unclosed quotes") {
    // 非 Digest 方案
    expect_eq(DigestChallenge.parse("Basic realm=\"x\"") == null, true);
    expect_eq(DigestCredentials.parse("Bearer token123") == null, true);

    // 缺失必选字段
    expect_eq(DigestChallenge.parse("Digest realm=\"only\"") == null, true);
    expect_eq(DigestChallenge.parse("Digest nonce=\"only\"") == null, true);
    expect_eq(DigestCredentials.parse("Digest username=\"a\", realm=\"b\", nonce=\"c\", uri=\"/\"") == null, true);

    // 未闭合引号与缺失 '='
    expect_eq(DigestChallenge.parse("Digest realm=\"unclosed, nonce=\"abc\"") == null, true);
    expect_eq(DigestChallenge.parse("Digest realm=\"ok\", badparam, nonce=\"abc\"") == null, true);
}

test("http_digest_auth: formatDigestHeader missing required field and small buffer guards") {
    let buf: [256]u8 = [0; 256];
    expect_eq(formatDigestHeader(DigestCredentials{ username: "a" }, buf[0..]) == null, true);

    const valid = DigestCredentials{
        username: "alice",
        realm: "r",
        nonce: "n",
        uri: "/",
        response: "abcd",
    };
    let tiny: [16]u8 = [0; 16];
    expect_eq(formatDigestHeader(valid, tiny[0..]) == null, true);
}
