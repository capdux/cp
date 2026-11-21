import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SignatureInput,
    SignatureInputIter,
    ComponentIter,
    findSignatureInput,
    findSignatureValue,
} from "cap:http";

test("1. SignatureInputIter parses multi-signature header with all RFC 9421 parameters", () => {
    const hdr =
        "sig1=(\"@method\" \"@authority\" \"content-digest\");created=1618884473;expires=1618884773;nonce=\"n,1;semi\";alg=\"rsa-pss-sha512\";keyid=\"test-key-1\";tag=\"app-v1\", "
        ++ "sig2=(\"@status\" \"content-type\");created=1618884500;keyid=\"key-2\";custom=\"ignored\";flag";

    let it = SignatureInputIter.init(hdr);

    const s1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s1.label, "sig1"), true);
    expect_eq(std.mem.eql(u8, s1.components_raw, "\"@method\" \"@authority\" \"content-digest\""), true);
    expect_eq(s1.created ?? unreachable, i64(1618884473));
    expect_eq(s1.expires ?? unreachable, i64(1618884773));
    expect_eq(std.mem.eql(u8, s1.nonce ?? unreachable, "n,1;semi"), true);
    expect_eq(std.mem.eql(u8, s1.alg ?? unreachable, "rsa-pss-sha512"), true);
    expect_eq(std.mem.eql(u8, s1.keyid ?? unreachable, "test-key-1"), true);
    expect_eq(std.mem.eql(u8, s1.tag ?? unreachable, "app-v1"), true);

    const s2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s2.label, "sig2"), true);
    expect_eq(std.mem.eql(u8, s2.components_raw, "\"@status\" \"content-type\""), true);
    expect_eq(s2.created ?? unreachable, i64(1618884500));
    expect_eq(s2.expires == null, true);
    expect_eq(s2.nonce == null, true);
    expect_eq(s2.alg == null, true);
    expect_eq(std.mem.eql(u8, s2.keyid ?? unreachable, "key-2"), true);
    expect_eq(s2.tag == null, true);

    expect_eq(it.next() == null, true);
});

test("2. findSignatureInput locates target label and handles missing or empty labels", () => {
    const hdr =
        "proxy=(\"@method\" \"@path\");keyid=\"proxy-k\", "
        ++ "origin=(\"@method\" \"@authority\" \"content-digest\");alg=\"ed25519\";keyid=\"origin-k\"";

    const origin = findSignatureInput(hdr, "origin") ?? unreachable;
    expect_eq(std.mem.eql(u8, origin.label, "origin"), true);
    expect_eq(std.mem.eql(u8, origin.alg ?? unreachable, "ed25519"), true);
    expect_eq(std.mem.eql(u8, origin.keyid ?? unreachable, "origin-k"), true);

    const proxy = findSignatureInput(hdr, "proxy") ?? unreachable;
    expect_eq(std.mem.eql(u8, proxy.keyid ?? unreachable, "proxy-k"), true);

    expect_eq(findSignatureInput(hdr, "missing") == null, true);
    expect_eq(findSignatureInput(hdr, "") == null, true);
});

test("3. ComponentIter extracts quoted component names and skips RFC 9421 component params", () => {
    const raw =
        "\"@method\" \"@query-param\";name=\"id\" \"content-type\";sf;bs \"@authority\";req \"x-custom\";key=\"a b\"";

    let cit = ComponentIter.init(raw);

    expect_eq(std.mem.eql(u8, cit.next() ?? unreachable, "@method"), true);
    expect_eq(std.mem.eql(u8, cit.next() ?? unreachable, "@query-param"), true);
    expect_eq(std.mem.eql(u8, cit.next() ?? unreachable, "content-type"), true);
    expect_eq(std.mem.eql(u8, cit.next() ?? unreachable, "@authority"), true);
    expect_eq(std.mem.eql(u8, cit.next() ?? unreachable, "x-custom"), true);
    expect_eq(cit.next() == null, true);

    // Empty component list (valid in RFC 9421: sig=())
    let empty_it = ComponentIter.init("");
    expect_eq(empty_it.next() == null, true);
});

test("4. SignatureInput.isActiveAt validates time window, leeway, boundaries, and inverted windows", () => {
    const hdr = "sig1=(\"@method\");created=1000;expires=2000";
    const s = findSignatureInput(hdr, "sig1") ?? unreachable;

    // Exact boundaries and interior
    expect_eq(s.isActiveAt(1000, 0), true);
    expect_eq(s.isActiveAt(1500, 0), true);
    expect_eq(s.isActiveAt(2000, 0), true);
    expect_eq(s.isActiveAt(999, 0), false);
    expect_eq(s.isActiveAt(2001, 0), false);

    // With leeway = 30s
    expect_eq(s.isActiveAt(970, 30), true);
    expect_eq(s.isActiveAt(969, 30), false);
    expect_eq(s.isActiveAt(2030, 30), true);
    expect_eq(s.isActiveAt(2031, 30), false);

    // Negative leeway clamped to 0
    expect_eq(s.isActiveAt(999, -10), false);
    expect_eq(s.isActiveAt(1000, -10), true);

    // Inverted window (created > expires) always inactive
    const inv = findSignatureInput("bad=(\"@method\");created=2000;expires=1000", "bad") ?? unreachable;
    expect_eq(inv.isActiveAt(1500, 600), false);

    // No created/expires constraints
    const unbounded = findSignatureInput("open=(\"@method\")", "open") ?? unreachable;
    expect_eq(unbounded.isActiveAt(0, 0), true);
});

test("5. findSignatureValue extracts Base64 byte sequences from Signature headers", () => {
    const sig_hdr =
        "sig1=:K2qGT5srn2OGbOIDzQ6kYT+ruWRW6yPZWppG5c1o=:, "
        ++ "sig2=:YWJjZGVmZ2g=:;note=\"ok,still-sig2\", "
        ++ "sig_empty=::";

    expect_eq(
        std.mem.eql(u8, findSignatureValue(sig_hdr, "sig1") ?? unreachable, "K2qGT5srn2OGbOIDzQ6kYT+ruWRW6yPZWppG5c1o="),
        true
    );
    expect_eq(
        std.mem.eql(u8, findSignatureValue(sig_hdr, "sig2") ?? unreachable, "YWJjZGVmZ2g="),
        true
    );
    expect_eq(
        std.mem.eql(u8, findSignatureValue(sig_hdr, "sig_empty") ?? unreachable, ""),
        true
    );
    expect_eq(findSignatureValue(sig_hdr, "sig3") == null, true);
    expect_eq(findSignatureValue(sig_hdr, "") == null, true);

    // Reject malformed byte sequences (unclosed colon, invalid chars, trailing garbage)
    expect_eq(findSignatureValue("sig1=:not base64!:", "sig1") == null, true);
    expect_eq(findSignatureValue("sig1=:unclosed", "sig1") == null, true);
    expect_eq(findSignatureValue("sig1=:AAAA==:trailing", "sig1") == null, true);
});

test("6. Malformed Signature-Input entries are skipped safely without panic", () => {
    const hdr =
        "BAD_UPPER=(\"@method\"), "
        ++ "no_parens=\"@method\", "
        ++ "unclosed=(\"@method\", "
        ++ "bad_int=(\"@method\");created=not_a_number, "
        ++ "unquoted_alg=(\"@method\");alg=ed25519, "
        ++ "good=(\"@method\" \"@path\");created=1700000000;alg=\"ed25519\"";

    let it = SignatureInputIter.init(hdr);
    const valid = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, valid.label, "good"), true);
    expect_eq(valid.created ?? unreachable, i64(1700000000));
    expect_eq(std.mem.eql(u8, valid.alg ?? unreachable, "ed25519"), true);
    expect_eq(it.next() == null, true);

    // Unclosed quote in ComponentIter terminates cleanly
    let bad_cit = ComponentIter.init("\"@method\" \"unterminated");
    expect_eq(std.mem.eql(u8, bad_cit.next() ?? unreachable, "@method"), true);
    expect_eq(bad_cit.next() == null, true);
});
