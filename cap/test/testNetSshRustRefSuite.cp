// Reference-hardened test suite against RustCrypto/SSH (MIT/Apache-2.0)
// Covers RFC 4251 wire codec, OpenSSH openssh-key-v1 private keys, SSHSIG Ed25519,
// 16-lane @Vector(16, u8) SIMD differential checks, and 1,800 deterministic PRNG fuzz cases.
const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SSH_MAX_WIRE_STRING_LEN,
    SshError,
    SshReader,
    SshWriter,
    simdSshCopyBytes,
    simdSshCopyBytesScalar,
    simdSshCtEqual,
    simdSshCtEqualScalar,
} from "../../lib/net/sshWire.cp";
import {
    SshKeyType,
    buildEd25519PublicKeyBlob,
    parsePublicKeyLine,
    formatPublicKeyLine,
    computeSha256Fingerprint,
    publicKeyEquals,
    verifySha256Fingerprint,
} from "../../lib/net/sshKeys.cp";
import {
    encodePemArmoring,
    decodePemArmoring,
    encodeOpenSshPrivateKeyEd25519,
    parseOpenSshPrivateKey,
} from "../../lib/net/sshOpensshKey.cp";
import {
    signSshSigEd25519,
    signSshSigEd25519WithHash,
    verifySshSigEd25519,
} from "../../lib/net/sshSig.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn fuzzReadNameList(alloc: std.mem.Allocator, data: []const u8) {
    let r = SshReader.init(data);
    const nl = r.readNameList(alloc) catch return;
    alloc.free(nl);
}

fn fuzzParsePrivKey(alloc: std.mem.Allocator, pem: []const u8) {
    let p = parseOpenSshPrivateKey(alloc, pem) catch return;
    p.deinit(alloc);
}

fn buildCustomPrivKeyPem(
    alloc: std.mem.Allocator,
    pub_blob: []const u8,
    checkint1: u32,
    checkint2: u32,
    pub_32: [32]u8,
    secret_64: [64]u8,
    comment: []const u8,
    extra_pad: []const u8,
) -> []u8 {
    let pw = SshWriter.init(alloc);
    defer pw.deinit();
    pw.writeU32(checkint1) catch unreachable;
    pw.writeU32(checkint2) catch unreachable;
    pw.writeString("ssh-ed25519") catch unreachable;
    pw.writeString(&pub_32) catch unreachable;
    pw.writeString(&secret_64) catch unreachable;
    pw.writeString(comment) catch unreachable;
    if (extra_pad.len > 0) {
        pw.writeRaw(extra_pad) catch unreachable;
    }

    let ow = SshWriter.init(alloc);
    defer ow.deinit();
    ow.writeRaw("openssh-key-v1\x00") catch unreachable;
    ow.writeString("none") catch unreachable;
    ow.writeString("none") catch unreachable;
    ow.writeString("") catch unreachable;
    ow.writeU32(1) catch unreachable;
    ow.writeString(pub_blob) catch unreachable;
    ow.writeString(pw.items()) catch unreachable;

    return encodePemArmoring(
        alloc,
        ow.items(),
        "-----BEGIN OPENSSH PRIVATE KEY-----",
        "-----END OPENSSH PRIVATE KEY-----",
    ) catch unreachable;
}

fn checkRfc4251(alloc: std.mem.Allocator): u32 {
    let w = SshWriter.init(alloc);
    defer w.deinit();

    // Official RFC 4251 section 5 mpint test vectors:
    w.writeMpint("") catch return 1;
    w.writeMpint("\x09\xa3\x78\xf9\xb2\xe3\x32\xa7") catch return 2;
    w.writeMpint("\x80") catch return 3;
    w.writeSignedMpint("\xed\xcc") catch return 4;
    w.writeSignedMpint("\xff\x21\x52\x41\x11") catch return 5;

    const wire = w.toOwnedSlice() catch return 6;
    defer alloc.free(wire);

    const expected_wire =
        "\x00\x00\x00\x00" ++
        "\x00\x00\x00\x08\x09\xa3\x78\xf9\xb2\xe3\x32\xa7" ++
        "\x00\x00\x00\x02\x00\x80" ++
        "\x00\x00\x00\x02\xed\xcc" ++
        "\x00\x00\x00\x05\xff\x21\x52\x41\x11";
    if (!simdSshCtEqual(wire, expected_wire)) return 7;

    let r = SshReader.init(wire);
    const v0 = r.readUnsignedMpint() catch return 8;
    if (v0.len != 0) return 9;
    const v1 = r.readUnsignedMpint() catch return 10;
    if (!simdSshCtEqual(v1, "\x09\xa3\x78\xf9\xb2\xe3\x32\xa7")) return 11;
    const v2 = r.readUnsignedMpint() catch return 12;
    if (!simdSshCtEqual(v2, "\x00\x80")) return 13;
    const v3 = r.readMpint() catch return 14;
    if (!simdSshCtEqual(v3, "\xed\xcc")) return 15;
    const v4 = r.readMpint() catch return 16;
    if (!simdSshCtEqual(v4, "\xff\x21\x52\x41\x11")) return 17;
    if (r.remaining() != 0) return 18;

    // Reject negative mpint in readUnsignedMpint
    let r_neg = SshReader.init("\x00\x00\x00\x02\xed\xcc");
    let got_neg_err = false;
    * = r_neg.readUnsignedMpint() catch |e| {
        if (e == SshError.InvalidFormat) got_neg_err = true;
    };
    if (!got_neg_err) return 19;

    // Reject non-canonical mpint encodings: [0x00], [0x00, 0x01], [0x00, 0x7f], [0xff, 0x80], [0xff, 0xff]
    const bad_mpints: [5][]const u8 = [
        "\x00\x00\x00\x01\x00",
        "\x00\x00\x00\x02\x00\x01",
        "\x00\x00\x00\x02\x00\x7f",
        "\x00\x00\x00\x02\xff\x80",
        "\x00\x00\x00\x02\xff\xff",
    ];
    for (bad of bad_mpints) {
        let rb = SshReader.init(bad);
        let rejected = false;
        * = rb.readMpint() catch |e| {
            if (e == SshError.InvalidFormat) rejected = true;
        };
        if (!rejected) return 20;
    }

    // Reject forged 0xFFFFFFFF and > SSH_MAX_WIRE_STRING_LEN string lengths
    let r_ff = SshReader.init("\xff\xff\xff\xffabcd");
    let got_too_large = false;
    * = r_ff.readString() catch |e| {
        if (e == SshError.PayloadTooLarge) got_too_large = true;
    };
    if (!got_too_large) return 21;

    let over_hdr = std.mem.zeroes([8]u8);
    std.mem.writeInt(u32, over_hdr[0..4], @as(u32, @intCast(SSH_MAX_WIRE_STRING_LEN + 1)), .big);
    let r_over = SshReader.init(&over_hdr);
    let got_over = false;
    * = r_over.readString() catch |e| {
        if (e == SshError.PayloadTooLarge) got_over = true;
    };
    if (!got_over) return 22;

    // Reject truncated string (len > data.len - pos)
    let r_trunc = SshReader.init("\x00\x00\x00\x20short");
    let got_eof = false;
    * = r_trunc.readString() catch |e| {
        if (e == SshError.UnexpectedEof) got_eof = true;
    };
    if (!got_eof) return 23;

    // Name-list validation: reject empty segments and control bytes
    const bad_namelists: [6][]const u8 = [
        "\x00\x00\x00\x01,",
        "\x00\x00\x00\x03a,,",
        "\x00\x00\x00\x04,abc",
        "\x00\x00\x00\x04abc,",
        "\x00\x00\x00\x04a\x00bc",
        "\x00\x00\x00\x04a bc",
    ];
    for (bnl of bad_namelists) {
        let rnl = SshReader.init(bnl);
        let rejected_nl = false;
        const parsed = rnl.readNameList(alloc) catch |e| {
            if (e == SshError.InvalidFormat) rejected_nl = true;
            if (!rejected_nl) return 24;
            continue;
        };
        alloc.free(parsed);
        return 25;
    }

    return 0;
}

fn checkPublicKey(alloc: std.mem.Allocator): u32 {
    const seed: [32]u8 = @splat(0x31);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch return 1;
    const blob = buildEd25519PublicKeyBlob(alloc, kp.public_key.toBytes()) catch return 2;
    defer alloc.free(blob);

    const b64_len = std.base64.standard.Encoder.calcSize(blob.len);
    const b64_buf = alloc.alloc(u8, b64_len) catch return 3;
    defer alloc.free(b64_buf);
    const b64 = std.base64.standard.Encoder.encode(b64_buf, blob);

    const line1 = std.fmt.allocPrint(alloc, "ssh-ed25519 {s} ops@host", .{b64}) catch return 4;
    defer alloc.free(line1);
    const line2 = std.fmt.allocPrint(alloc, "ssh-ed25519 {s} other@host", .{b64}) catch return 5;
    defer alloc.free(line2);

    let pk1 = parsePublicKeyLine(alloc, line1) catch return 6;
    defer pk1.deinit(alloc);
    let pk2 = parsePublicKeyLine(alloc, line2) catch return 7;
    defer pk2.deinit(alloc);

    if (pk1.key_type != SshKeyType.Ed25519) return 8;
    if (!publicKeyEquals(&pk1, &pk2)) return 9;

    const fmt1 = formatPublicKeyLine(alloc, &pk1) catch return 10;
    defer alloc.free(fmt1);
    if (!simdSshCtEqual(fmt1, line1)) return 11;

    const fp = computeSha256Fingerprint(alloc, pk1.raw_blob) catch return 12;
    defer alloc.free(fp);
    const fp_ok = verifySha256Fingerprint(alloc, pk1.raw_blob, fp) catch return 13;
    if (!fp_ok) return 14;

    let bad_fp = alloc.dupe(u8, fp) catch return 15;
    defer alloc.free(bad_fp);
    bad_fp[bad_fp.len - 1] = if (bad_fp[bad_fp.len - 1] == 'A') 'B' else 'A';
    const fp_bad = verifySha256Fingerprint(alloc, pk1.raw_blob, bad_fp) catch return 16;
    if (fp_bad) return 17;

    // Reject algorithm mismatch between outer label and inner wire blob
    const mismatch_line = std.fmt.allocPrint(alloc, "ecdsa-sha2-nistp256 {s}", .{b64}) catch return 18;
    defer alloc.free(mismatch_line);
    let got_mismatch = false;
    * = parsePublicKeyLine(alloc, mismatch_line) catch |e| {
        if (e == SshError.KeyTypeMismatch) got_mismatch = true;
    };
    if (!got_mismatch) return 19;

    // Reject embedded NUL byte in public key line
    const nul_line = std.fmt.allocPrint(alloc, "ssh-ed25519 {s} bad\x00comment", .{b64}) catch return 20;
    defer alloc.free(nul_line);
    let got_nul_err = false;
    * = parsePublicKeyLine(alloc, nul_line) catch |e| {
        if (e == SshError.InvalidFormat) got_nul_err = true;
    };
    if (!got_nul_err) return 21;

    return 0;
}

fn checkOpenSshPrivateKey(alloc: std.mem.Allocator): u32 {
    const seed: [32]u8 = @splat(0x5a);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch return 1;
    const secret_64 = kp.secret_key.toBytes();
    const pub_32 = kp.public_key.toBytes();

    // Roundtrip across comment lengths 0..8 to exercise every padding length 0..7
    const comments: [9][]const u8 = [ "", "a", "ab", "abc", "abcd", "abcde", "abcdef", "abcdefg", "abcdefgh" ];
    for (c of comments) {
        const pem = encodeOpenSshPrivateKeyEd25519(alloc, secret_64, c, 0x13572468) catch return 2;
        defer alloc.free(pem);
        let parsed = parseOpenSshPrivateKey(alloc, pem) catch return 3;
        if (!simdSshCtEqual(&parsed.ed25519_pub, &pub_32)) return 4;
        if (!simdSshCtEqual(&parsed.ed25519_secret, &secret_64)) return 5;
        if (!simdSshCtEqual(parsed.comment, c)) return 6;

        // Verify deinit performs std.crypto.secureZero on secret key bytes
        parsed.deinit(alloc);
        let zero_check: u8 = 0;
        for (b of parsed.ed25519_secret) {
            zero_check = zero_check | b;
        }
        if (zero_check != 0) return 7;
    }

    const pub_blob = buildEd25519PublicKeyBlob(alloc, pub_32) catch return 8;
    defer alloc.free(pub_blob);

    // Checkint mismatch (checkint1 != checkint2)
    const bad_ci_pem = buildCustomPrivKeyPem(alloc, pub_blob, 0x11112222, 0x33334444, pub_32, secret_64, "ci", "");
    defer alloc.free(bad_ci_pem);
    let rejected_ci = false;
    * = parseOpenSshPrivateKey(alloc, bad_ci_pem) catch |e| {
        if (e == SshError.CheckintMismatch) rejected_ci = true;
    };
    if (!rejected_ci) return 9;

    // Bad padding sequence (1, 2, 4 instead of 1, 2, 3)
    const bad_pad_pem = buildCustomPrivKeyPem(alloc, pub_blob, 0xaaaa5555, 0xaaaa5555, pub_32, secret_64, "padtest", "\x01\x02\x04");
    defer alloc.free(bad_pad_pem);
    let rejected_pad = false;
    * = parseOpenSshPrivateKey(alloc, bad_pad_pem) catch |e| {
        if (e == SshError.InvalidFormat) rejected_pad = true;
    };
    if (!rejected_pad) return 10;

    // Excessive padding (>= 8 bytes for cipher "none")
    const over_pad_pem = buildCustomPrivKeyPem(alloc, pub_blob, 0x12345678, 0x12345678, pub_32, secret_64, "overpad", "\x01\x02\x03\x04\x05\x06\x07\x08");
    defer alloc.free(over_pad_pem);
    let rejected_over = false;
    * = parseOpenSshPrivateKey(alloc, over_pad_pem) catch |e| {
        if (e == SshError.InvalidFormat) rejected_over = true;
    };
    if (!rejected_over) return 11;

    // Mismatched trailing pubkey bytes inside secret_64[32..64]
    let bad_secret = secret_64;
    bad_secret[63] = bad_secret[63] ^ 0x01;
    const bad_sec_pem = buildCustomPrivKeyPem(alloc, pub_blob, 0x22223333, 0x22223333, pub_32, bad_secret, "", "");
    defer alloc.free(bad_sec_pem);
    let rejected_sec = false;
    * = parseOpenSshPrivateKey(alloc, bad_sec_pem) catch |e| {
        if (e == SshError.InvalidFormat) rejected_sec = true;
    };
    if (!rejected_sec) return 12;

    return 0;
}

fn checkSshSig(alloc: std.mem.Allocator): u32 {
    const seed: [32]u8 = @splat(0xc3);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch return 1;
    const secret_64 = kp.secret_key.toBytes();
    const pub_32 = kp.public_key.toBytes();
    const msg = "release-manifest-v1.0.0:sha256:0123456789abcdef";

    // Both sha512 and sha256 roundtrips pass
    const sig512 = signSshSigEd25519(alloc, secret_64, "git", msg) catch return 2;
    defer alloc.free(sig512);
    verifySshSigEd25519(alloc, pub_32, "git", msg, sig512) catch return 3;

    const sig256 = signSshSigEd25519WithHash(alloc, secret_64, "file", "sha256", msg) catch return 4;
    defer alloc.free(sig256);
    verifySshSigEd25519(alloc, pub_32, "file", msg, sig256) catch return 5;

    // Reject unsupported hash algorithm on sign
    let got_bad_hash = false;
    * = signSshSigEd25519WithHash(alloc, secret_64, "git", "sha1", msg) catch |e| {
        if (e == SshError.InvalidHashAlgorithm) got_bad_hash = true;
    };
    if (!got_bad_hash) return 6;

    // Reject empty namespace on sign & verify
    let got_empty_ns_sign = false;
    * = signSshSigEd25519(alloc, secret_64, "", msg) catch |e| {
        if (e == SshError.InvalidNamespace) got_empty_ns_sign = true;
    };
    if (!got_empty_ns_sign) return 7;

    let got_empty_ns_verify = false;
    verifySshSigEd25519(alloc, pub_32, "", msg, sig512) catch |e| {
        if (e == SshError.InvalidNamespace) got_empty_ns_verify = true;
    };
    if (!got_empty_ns_verify) return 8;

    // Single-bit flip in decoded SSHSIG signature payload must be rejected
    let raw_sig = decodePemArmoring(
        alloc,
        sig512,
        "-----BEGIN SSH SIGNATURE-----",
        "-----END SSH SIGNATURE-----",
    ) catch return 9;
    defer alloc.free(raw_sig);

    raw_sig[raw_sig.len - 1] = raw_sig[raw_sig.len - 1] ^ 0x01;
    const tampered_pem = encodePemArmoring(
        alloc,
        raw_sig,
        "-----BEGIN SSH SIGNATURE-----",
        "-----END SSH SIGNATURE-----",
    ) catch return 10;
    defer alloc.free(tampered_pem);

    let got_tamper_err = false;
    verifySshSigEd25519(alloc, pub_32, "git", msg, tampered_pem) catch |e| {
        if (e == SshError.InvalidSignature) got_tamper_err = true;
    };
    if (!got_tamper_err) return 11;

    // Wrong SSHSIG version (version = 2) must be rejected
    raw_sig[raw_sig.len - 1] = raw_sig[raw_sig.len - 1] ^ 0x01;
    raw_sig[9] = 2;
    const bad_ver_pem = encodePemArmoring(
        alloc,
        raw_sig,
        "-----BEGIN SSH SIGNATURE-----",
        "-----END SSH SIGNATURE-----",
    ) catch return 12;
    defer alloc.free(bad_ver_pem);

    let got_ver_err = false;
    verifySshSigEd25519(alloc, pub_32, "git", msg, bad_ver_pem) catch |e| {
        if (e == SshError.InvalidFormat) got_ver_err = true;
    };
    if (!got_ver_err) return 13;

    // Wrong SSHSIG magic must be rejected
    raw_sig[9] = 1;
    raw_sig[0] = 'X';
    const bad_magic_pem = encodePemArmoring(
        alloc,
        raw_sig,
        "-----BEGIN SSH SIGNATURE-----",
        "-----END SSH SIGNATURE-----",
    ) catch return 14;
    defer alloc.free(bad_magic_pem);

    let got_magic_err = false;
    verifySshSigEd25519(alloc, pub_32, "git", msg, bad_magic_pem) catch |e| {
        if (e == SshError.InvalidMagic) got_magic_err = true;
    };
    if (!got_magic_err) return 15;

    return 0;
}

fn checkSimdParity(): u32 {
    let prng = XorShift64.init(0x5548_5349_4d44_0001);
    let src = std.mem.zeroes([260]u8);
    let dst_simd = std.mem.zeroes([260]u8);
    let dst_scalar = std.mem.zeroes([260]u8);
    let other = std.mem.zeroes([260]u8);

    prng.fillBytes(&src);

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        @memset(&dst_simd, 0xaa);
        @memset(&dst_scalar, 0xaa);

        simdSshCopyBytes(dst_simd[0..len], src[0..len]);
        simdSshCopyBytesScalar(dst_scalar[0..len], src[0..len]);

        if (!std.mem.eql(u8, dst_simd[0..len], dst_scalar[0..len])) return 1;
        if (!simdSshCtEqual(dst_simd[0..len], src[0..len])) return 2;
        if (!simdSshCtEqualScalar(dst_simd[0..len], src[0..len])) return 3;

        if (len > 0) {
            simdSshCopyBytes(other[0..len], src[0..len]);
            const positions: [3]usize = [ 0, len / 2, len - 1 ];
            for (pos of positions) {
                other[pos] = other[pos] ^ 0x01;
                if (simdSshCtEqual(src[0..len], other[0..len])) return 4;
                if (simdSshCtEqualScalar(src[0..len], other[0..len])) return 5;
                other[pos] = other[pos] ^ 0x01;
            }
        }
    }

    if (simdSshCtEqual(src[0..32], src[0..31])) return 6;
    if (simdSshCtEqualScalar(src[0..32], src[0..31])) return 7;
    return 0;
}

fn checkDeterministicFuzz(alloc: std.mem.Allocator): u32 {
    let prng = XorShift64.init(0x2026_1123_5353_4801);
    let buf = std.mem.zeroes([192]u8);

    // 1. 600 malformed SSH wire streams (string / mpint / unsigned mpint / name-list)
    let iter: usize = 0;
    while (iter < 600) : (iter += 1) {
        const n = @as(usize, @intCast(prng.next() % 192));
        prng.fillBytes(buf[0..n]);
        if (n >= 4 and (iter % 5 == 0)) {
            buf[0] = 0xff;
            buf[1] = 0xff;
            buf[2] = 0xff;
            buf[3] = 0xfe;
        }
        let r1 = SshReader.init(buf[0..n]);
        * = r1.readString() catch {};

        let r2 = SshReader.init(buf[0..n]);
        * = r2.readMpint() catch {};

        let r3 = SshReader.init(buf[0..n]);
        * = r3.readUnsignedMpint() catch {};

        fuzzReadNameList(alloc, buf[0..n]);
    }

    // 2. 600 malformed openssh-key-v1 private key inputs (mutated valid + random PEM)
    const seed: [32]u8 = @splat(0x77);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch return 1;
    const base_key_pem = encodeOpenSshPrivateKeyEd25519(alloc, kp.secret_key.toBytes(), "fuzz", 0x99887766) catch return 2;
    defer alloc.free(base_key_pem);
    const base_key_raw = decodePemArmoring(
        alloc,
        base_key_pem,
        "-----BEGIN OPENSSH PRIVATE KEY-----",
        "-----END OPENSSH PRIVATE KEY-----",
    ) catch return 3;
    defer alloc.free(base_key_raw);

    iter = 0;
    while (iter < 600) : (iter += 1) {
        if (iter % 2 == 0) {
            const cut_len = @as(usize, @intCast(prng.next() % (base_key_raw.len + 1)));
            let mutated = alloc.dupe(u8, base_key_raw[0..cut_len]) catch return 4;
            defer alloc.free(mutated);
            if (mutated.len > 0) {
                const flip_idx = @as(usize, @intCast(prng.next() % mutated.len));
                mutated[flip_idx] = mutated[flip_idx] ^ (prng.nextByte() | 1);
            }
            const pem = encodePemArmoring(
                alloc,
                mutated,
                "-----BEGIN OPENSSH PRIVATE KEY-----",
                "-----END OPENSSH PRIVATE KEY-----",
            ) catch return 5;
            defer alloc.free(pem);
            fuzzParsePrivKey(alloc, pem);
        } else {
            const n = @as(usize, @intCast(prng.next() % 160));
            prng.fillBytes(buf[0..n]);
            const pem = encodePemArmoring(
                alloc,
                buf[0..n],
                "-----BEGIN OPENSSH PRIVATE KEY-----",
                "-----END OPENSSH PRIVATE KEY-----",
            ) catch return 6;
            defer alloc.free(pem);
            fuzzParsePrivKey(alloc, pem);
        }
    }

    // 3. 600 malformed SSHSIG inputs (mutated valid + random PEM)
    const base_sig_pem = signSshSigEd25519(alloc, kp.secret_key.toBytes(), "git", "fuzz-msg") catch return 7;
    defer alloc.free(base_sig_pem);
    const base_sig_raw = decodePemArmoring(
        alloc,
        base_sig_pem,
        "-----BEGIN SSH SIGNATURE-----",
        "-----END SSH SIGNATURE-----",
    ) catch return 8;
    defer alloc.free(base_sig_raw);
    const pub_32 = kp.public_key.toBytes();

    iter = 0;
    while (iter < 600) : (iter += 1) {
        if (iter % 2 == 0) {
            const cut_len = @as(usize, @intCast(prng.next() % (base_sig_raw.len + 1)));
            let mutated = alloc.dupe(u8, base_sig_raw[0..cut_len]) catch return 9;
            defer alloc.free(mutated);
            if (mutated.len > 0) {
                const flip_idx = @as(usize, @intCast(prng.next() % mutated.len));
                mutated[flip_idx] = mutated[flip_idx] ^ (prng.nextByte() | 1);
            }
            const pem = encodePemArmoring(
                alloc,
                mutated,
                "-----BEGIN SSH SIGNATURE-----",
                "-----END SSH SIGNATURE-----",
            ) catch return 10;
            defer alloc.free(pem);
            verifySshSigEd25519(alloc, pub_32, "git", "fuzz-msg", pem) catch {};
        } else {
            const n = @as(usize, @intCast(prng.next() % 160));
            prng.fillBytes(buf[0..n]);
            const pem = encodePemArmoring(
                alloc,
                buf[0..n],
                "-----BEGIN SSH SIGNATURE-----",
                "-----END SSH SIGNATURE-----",
            ) catch return 11;
            defer alloc.free(pem);
            verifySshSigEd25519(alloc, pub_32, "git", "fuzz-msg", pem) catch {};
        }
    }

    return 0;
}

fn main() {
    test("1. RFC 4251 mpint official vectors, canonical guards, and string length bounds", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkRfc4251(alloc), 0 as u32);
    });

    test("2. OpenSSH public key parsing, constant-time equality, and fingerprint checks", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkPublicKey(alloc), 0 as u32);
    });

    test("3. OpenSSH openssh-key-v1 checkint, padding, pub/priv consistency, and secureZero", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkOpenSshPrivateKey(alloc), 0 as u32);
    });

    test("4. SSHSIG Ed25519 sha256/sha512 roundtrip, header checks, and 1-bit tamper rejection", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkSshSig(alloc), 0 as u32);
    });

    test("5. 16-lane @Vector(16, u8) SIMD copy & constant-time equal 0-diff parity", () => {
        expect_eq(checkSimdParity(), 0 as u32);
    });

    test("6. Deterministic PRNG fuzzing (1,800 malformed SSH wire / private key / SSHSIG inputs)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkDeterministicFuzz(alloc), 0 as u32);
    });
}
