import { test, expect } from "cap:test";
import { IntegrityTag, Integrity, IntegrityVerifier } from "cap:sri";

test("sri: compute, format, parse, and verify roundtrip for sha256/sha384/sha512", () => {
    const payload = "hello cp subresource integrity";
    let buf: [128]u8 = undefined;

    const i256 = Integrity.compute(.sha256, payload) catch unreachable;
    const s256 = i256.format(buf[0..]) catch unreachable;
    const p256 = Integrity.parse(s256) catch unreachable;
    expect(p256.tag == .sha256).toBeTrue();
    expect(p256.eql(i256)).toBeTrue();
    expect(p256.verify(payload)).toBeTrue();
    expect(p256.verify("tampered")).toBeFalse();

    const i512 = Integrity.compute(.sha512, payload) catch unreachable;
    const s512 = i512.format(buf[0..]) catch unreachable;
    const p512 = Integrity.parse(s512) catch unreachable;
    expect(p512.tag == .sha512).toBeTrue();
    expect(p512.verify(payload)).toBeTrue();
});

test("sri: streaming IntegrityVerifier across multiple chunks", () => {
    const full = "chunk-one|chunk-two|chunk-three";
    const expected = Integrity.compute(.sha384, full) catch unreachable;

    let v = IntegrityVerifier.init(expected);
    v.update("chunk-one");
    v.update("|chunk-two|");
    v.update("chunk-three");
    expect(v.verify()).toBeTrue();

    let v_bad = IntegrityVerifier.init(expected);
    v_bad.update("chunk-one");
    v_bad.update("|corrupted|");
    v_bad.update("chunk-three");
    expect(v_bad.verify()).toBeFalse();
});

test("sri: multi-token strongest algorithm selection, ?options, and sha1 hex", () => {
    // Known SHA-1("abc") = a9993e364706816aba3e25717850c26c9cd0d89d
    const h1 = Integrity.parseSha1Hex("a9993e364706816aba3e25717850c26c9cd0d89d") catch unreachable;
    expect(h1.tag == .sha1).toBeTrue();
    expect(h1.verify("abc")).toBeTrue();

    // Multi-token header: sha256 + sha512 -> selects strongest (sha512)
    let b256: [128]u8 = undefined;
    let b512: [128]u8 = undefined;
    const i256 = Integrity.compute(.sha256, "abc") catch unreachable;
    const i512 = Integrity.compute(.sha512, "abc") catch unreachable;
    const s256 = i256.format(b256[0..]) catch unreachable;
    const s512 = i512.format(b512[0..]) catch unreachable;

    let combined: [256]u8 = undefined;
    @memcpy(combined[0..s256.len], s256);
    combined[s256.len] = ' ';
    @memcpy(combined[s256.len + 1 .. s256.len + 1 + s512.len], s512);
    const chosen = Integrity.parse(combined[0 .. s256.len + 1 + s512.len]) catch unreachable;
    expect(chosen.tag == .sha512).toBeTrue();
    expect(chosen.verify("abc")).toBeTrue();

    // Malformed inputs rejected
    expect(Integrity.parseSingle("md5-AAAA") == null).toBeTrue();
    expect(Integrity.parseSingle("sha256-not-valid-len") == null).toBeTrue();
    const bad_hex = Integrity.parseSha1Hex("zzzz") catch null;
    expect(bad_hex == null).toBeTrue();
});
