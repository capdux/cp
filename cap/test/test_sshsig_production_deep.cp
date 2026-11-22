// Source: cp/cap/test/test_sshsig_production_deep.cp
// Deep Production Test Suite for cap:sshsig
// Covers:
//   1. Realistic ssh-ed25519 + git namespace + sha512 binary SSHSIG parsing & nested fields
//   2. PEM Armor (-----BEGIN/END SSH SIGNATURE-----) Base64 decode + parse end-to-end
//   3. buildSshsigSignedData byte-exact preimage framing verification
//   4. sha256 + file namespace + ecdsa-sha2-nistp256 binary SSHSIG parsing
//   5. Wrong magic, version!=1, empty namespace, trailing garbage & malformed Base64 guards

import { test, expect_eq } from "cap:test";
import {
    SSHSIG_MAGIC,
    SSHSIG_VERSION,
    SSHSIG_ARMOR_BEGIN,
    SSHSIG_ARMOR_END,
    SshsigBlob,
    isSshsigBinary,
    isSshsigArmored,
    decodeSshsigArmor,
    parseSshsigBinary,
    buildSshsigSignedData,
} from "cap:crypto";

struct WireBuilder {
    buf: [1024]u8 = [0; 1024],
    len: usize = 0,

    fn writeBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.len] = s[i];
            _.len += 1;
            i += 1;
        }
    }

    fn writeU32Be(*_, v: u32) {
        _.buf[_.len] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.len + 1] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.len + 2] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.len + 3] = (v & 0xFF) as u8;
        _.len += 4;
    }

    fn writeSshString(*_, s: []const u8) {
        _.writeU32Be(s.len as u32);
        _.writeBytes(s);
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn base64Encode(src: []const u8, out: []u8): usize {
    const table: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let i: usize = 0;
    let w: usize = 0;
    while (i < src.len) {
        const b0 = src[i];
        const has1 = i + 1 < src.len;
        const has2 = i + 2 < src.len;
        const b1: u8 = if (has1) src[i + 1] else 0;
        const b2: u8 = if (has2) src[i + 2] else 0;

        out[w] = table[(b0 >> 2) as usize];
        out[w + 1] = table[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize];
        out[w + 2] = if (has1) table[(((b1 & 0x0F) << 2) | (b2 >> 6)) as usize] else '=';
        out[w + 3] = if (has2) table[(b2 & 0x3F) as usize] else '=';
        w += 4;
        i += 3;
    }
    return w;
}

fn buildSampleSshsig(
    wb: *WireBuilder,
    version: u32,
    key_type: []const u8,
    pub_material: []const u8,
    namespace: []const u8,
    reserved: []const u8,
    hash_alg: []const u8,
    sig_type: []const u8,
    raw_sig: []const u8,
) {
    let pk = WireBuilder {};
    pk.writeSshString(key_type);
    pk.writeSshString(pub_material);

    let sg = WireBuilder {};
    sg.writeSshString(sig_type);
    sg.writeSshString(raw_sig);

    wb.writeBytes(SSHSIG_MAGIC);
    wb.writeU32Be(version);
    wb.writeSshString(pk.slice());
    wb.writeSshString(namespace);
    wb.writeSshString(reserved);
    wb.writeSshString(hash_alg);
    wb.writeSshString(sg.slice());
}

test("sshsig: ssh-ed25519 + git namespace + sha512 binary SSHSIG parsing") {
    let pub_key: [32]u8 = [0x42; 32];
    let raw_sig: [64]u8 = [0xAB; 64];

    let wb = WireBuilder {};
    buildSampleSshsig(
        *wb,
        SSHSIG_VERSION,
        "ssh-ed25519",
        pub_key[0..32],
        "git",
        "",
        "sha512",
        "ssh-ed25519",
        raw_sig[0..64],
    );

    const blob = wb.slice();
    expect_eq(isSshsigBinary(blob), true);

    if (sig := parseSshsigBinary(blob)) {
        expect_eq(sig.version, 1);
        expect_eq(strEq(sig.key_type, "ssh-ed25519"), true);
        expect_eq(strEq(sig.namespace, "git"), true);
        expect_eq(sig.reserved.len, 0);
        expect_eq(strEq(sig.hash_algorithm, "sha512"), true);
        expect_eq(strEq(sig.sig_type, "ssh-ed25519"), true);
        expect_eq(sig.raw_signature.len, 64);
        expect_eq(sig.raw_signature[0], 0xAB);
        expect_eq(sig.raw_signature[63], 0xAB);
    } else {
        expect_eq(true, false);
    }
}

test("sshsig: PEM Armor Base64 decode + parseSshsigBinary end-to-end") {
    let pub_key: [32]u8 = [0x11; 32];
    let raw_sig: [64]u8 = [0xCD; 64];

    let wb = WireBuilder {};
    buildSampleSshsig(
        *wb,
        1,
        "ssh-ed25519",
        pub_key[0..32],
        "git",
        "",
        "sha512",
        "ssh-ed25519",
        raw_sig[0..64],
    );

    let b64_buf: [512]u8 = [0; 512];
    const b64_len = base64Encode(wb.slice(), b64_buf[0..512]);

    // Assemble PEM armored text with leading whitespace and CRLF line breaks
    let pem = WireBuilder {};
    pem.writeBytes("  \r\n");
    pem.writeBytes(SSHSIG_ARMOR_BEGIN);
    pem.writeBytes("\r\n");
    // Wrap Base64 at 64 chars per line
    let off: usize = 0;
    while (off < b64_len) {
        const end = if (off + 64 < b64_len) off + 64 else b64_len;
        pem.writeBytes(b64_buf[off..end]);
        pem.writeBytes("\r\n");
        off = end;
    }
    pem.writeBytes(SSHSIG_ARMOR_END);
    pem.writeBytes("\r\n");

    const armored = pem.slice();
    expect_eq(isSshsigArmored(armored), true);

    let decoded: [512]u8 = [0; 512];
    if (dec_len := decodeSshsigArmor(armored, decoded[0..512])) {
        expect_eq(dec_len, wb.slice().len);
        if (sig := parseSshsigBinary(decoded[0..dec_len])) {
            expect_eq(strEq(sig.key_type, "ssh-ed25519"), true);
            expect_eq(strEq(sig.namespace, "git"), true);
            expect_eq(sig.raw_signature.len, 64);
            expect_eq(sig.raw_signature[0], 0xCD);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("sshsig: buildSshsigSignedData byte-exact preimage framing") {
    const digest = "\x01\x02\x03\x04";
    let out: [128]u8 = [0; 128];

    if (n := buildSshsigSignedData("git", "", "sha512", digest, out[0..128])) {
        // Expected: 6 ("SSHSIG") + (4+3) ("git") + (4+0) ("") + (4+6) ("sha512") + (4+4) (digest) = 35
        expect_eq(n, 35);
        expect_eq(strEq(out[0..6], "SSHSIG"), true);
        // string("git") -> 00 00 00 03 "git"
        expect_eq(out[9], 3);
        expect_eq(strEq(out[10..13], "git"), true);
        // string("") -> 00 00 00 00
        expect_eq(out[13], 0);
        expect_eq(out[16], 0);
        // string("sha512") -> 00 00 00 06 "sha512"
        expect_eq(out[20], 6);
        expect_eq(strEq(out[21..27], "sha512"), true);
        // string(digest) -> 00 00 00 04 \x01\x02\x03\x04
        expect_eq(out[30], 4);
        expect_eq(out[31], 0x01);
        expect_eq(out[34], 0x04);
    } else {
        expect_eq(true, false);
    }

    // Empty namespace rejected
    if (n := buildSshsigSignedData("", "", "sha512", digest, out[0..128])) {
        * = n;
        expect_eq(true, false);
    }
}

test("sshsig: sha256 + file namespace + ecdsa-sha2-nistp256 binary SSHSIG") {
    let pub_mat: [16]u8 = [0x55; 16];
    let raw_sig: [48]u8 = [0x77; 48];

    let wb = WireBuilder {};
    buildSampleSshsig(
        *wb,
        1,
        "ecdsa-sha2-nistp256",
        pub_mat[0..16],
        "file",
        "res",
        "sha256",
        "ecdsa-sha2-nistp256",
        raw_sig[0..48],
    );

    if (sig := parseSshsigBinary(wb.slice())) {
        expect_eq(strEq(sig.key_type, "ecdsa-sha2-nistp256"), true);
        expect_eq(strEq(sig.namespace, "file"), true);
        expect_eq(strEq(sig.reserved, "res"), true);
        expect_eq(strEq(sig.hash_algorithm, "sha256"), true);
        expect_eq(sig.raw_signature.len, 48);
    } else {
        expect_eq(true, false);
    }
}

test("sshsig: wrong magic, version!=1, empty namespace, trailing garbage & bad Base64 guards") {
    let pub_k: [8]u8 = [0x01; 8];
    let sig_b: [8]u8 = [0x02; 8];

    // 1. Wrong magic
    expect_eq(isSshsigBinary("BADSIG\x00\x00\x00\x01"), false);

    // 2. Version != 1
    let bad_ver = WireBuilder {};
    buildSampleSshsig(*bad_ver, 2, "ssh-ed25519", pub_k[0..8], "git", "", "sha512", "ssh-ed25519", sig_b[0..8]);
    expect_eq(isSshsigBinary(bad_ver.slice()), false);
    if (s := parseSshsigBinary(bad_ver.slice())) {
        * = s;
        expect_eq(true, false);
    }

    // 3. Empty namespace
    let empty_ns = WireBuilder {};
    buildSampleSshsig(*empty_ns, 1, "ssh-ed25519", pub_k[0..8], "", "", "sha512", "ssh-ed25519", sig_b[0..8]);
    if (s := parseSshsigBinary(empty_ns.slice())) {
        * = s;
        expect_eq(true, false);
    }

    // 4. Trailing garbage byte after signature_blob
    let trailing = WireBuilder {};
    buildSampleSshsig(*trailing, 1, "ssh-ed25519", pub_k[0..8], "git", "", "sha512", "ssh-ed25519", sig_b[0..8]);
    trailing.writeBytes("\xFF");
    if (s := parseSshsigBinary(trailing.slice())) {
        * = s;
        expect_eq(true, false);
    }

    // 5. Malformed Base64 inside PEM armor
    let out: [64]u8 = [0; 64];
    const bad_b64 = "-----BEGIN SSH SIGNATURE-----\n!!!!\n-----END SSH SIGNATURE-----\n";
    if (n := decodeSshsigArmor(bad_b64, out[0..64])) {
        * = n;
        expect_eq(true, false);
    }
    const short_b64 = "-----BEGIN SSH SIGNATURE-----\nABC\n-----END SSH SIGNATURE-----\n";
    if (n := decodeSshsigArmor(short_b64, out[0..64])) {
        * = n;
        expect_eq(true, false);
    }
}
