import {
    DigestIter,
    findDigest,
    formatDigest,
    WantDigestIter,
    negotiateDigestAlg,
} from "cap:http";
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

test("DigestIter, findDigest, and formatDigest round-trip", () => {
    const hdr = "sha-256=:X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE=:, sha-512=:WZDPaVn/7XgHaAy8pmojAkGWoRx2UFChF41A2svX+TaPm+AbwAgBWnrIiYllu7BNNyealdVLvRwEmTHWXvJwew==:";
    let it = DigestIter.init(hdr);
    const d1 = it.next().?;
    expect(strEq(d1.algorithm, "sha-256"));
    expect(strEq(d1.base64_value, "X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE="));

    const d2 = it.next().?;
    expect(strEq(d2.algorithm, "sha-512"));
    expect(it.next() == null);

    expect(strEq(findDigest(hdr, "SHA-256").?, "X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE="));
    expect(findDigest(hdr, "md5") == null);

    let buf: [128]u8 = [0; 128];
    expect(strEq(formatDigest("SHA-256", "AAAA", buf[0..]).?, "sha-256=:AAAA:"));
    expect(formatDigest("bad key", "AAAA", buf[0..]) == null);
});

test("WantDigestIter and negotiateDigestAlg", () => {
    let it = WantDigestIter.init("sha-512=3, sha-256=10, md5=0");
    const p1 = it.next().?;
    expect(strEq(p1.algorithm, "sha-512") && p1.weight == 3);
    const p2 = it.next().?;
    expect(strEq(p2.algorithm, "sha-256") && p2.weight == 10);
    const p3 = it.next().?;
    expect(strEq(p3.algorithm, "md5") && p3.weight == 0);
    expect(it.next() == null);

    const supported: [3][]const u8 = ["md5", "sha-512", "sha-256"];
    expect(negotiateDigestAlg("sha-512=3, sha-256=10, md5=0", supported[0..]).? == 2);
    expect(negotiateDigestAlg("md5=0", supported[0..]) == null);
});
