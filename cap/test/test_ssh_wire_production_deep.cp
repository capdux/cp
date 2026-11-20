// Source: cp/cap/test/test_ssh_wire_production_deep.cp (Deep tests for cap:ssh_wire)
const std = @import("std");
type TestNameSlice = []const u8;
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SshError,
    SshReader,
    SshWriter,
    SshKeyType,
    buildEd25519PublicKeyBlob,
    buildEcdsaP256PublicKeyBlob,
    parsePublicKeyLine,
    formatPublicKeyLine,
    computeSha256Fingerprint,
    encodeOpenSshPrivateKeyEd25519,
    parseOpenSshPrivateKey,
    signSshSigEd25519,
    verifySshSigEd25519,
} from "cap:ssh_wire";

test("RFC 4251 SshWriter/SshReader mpint and truncation guards") {
    const alloc = std.testing.allocator;
    let w = SshWriter.init(alloc);
    defer w.deinit();

    try w.writeU32(0x12345678);
    try w.writeString("ssh-ed25519");

    // mpint: zero -> empty
    try w.writeMpint("\x00\x00");

    // mpint: positive with MSB 0x80 -> prefixed with 0x00
    try w.writeMpint("\x80\x01");

    // mpint: negative two's complement with redundant 0xff
    try w.writeSignedMpint("\xff\xff\x80");

    // name-list
    let names_list = std.ArrayList(TestNameSlice).empty;
    defer names_list.deinit(alloc);
    try names_list.append(alloc, "curve25519-sha256");
    try names_list.append(alloc, "ecdh-sha2-nistp256");
    try w.writeNameList(names_list.items);

    const wire = try w.toOwnedSlice();
    defer alloc.free(wire);

    let r = SshReader.init(wire);
    expect_eq(try r.readU32(), 0x12345678);
    const s = try r.readString();
    expect_eq(std.mem.eql(u8, s, "ssh-ed25519"), true);

    const m0 = try r.readMpint();
    expect_eq(m0.len, 0);

    const m1 = try r.readMpint();
    expect_eq(m1.len, 3);
    expect_eq(m1[0], 0x00);
    expect_eq(m1[1], 0x80);
    expect_eq(m1[2], 0x01);

    const m2 = try r.readMpint();
    expect_eq(m2.len, 2);
    expect_eq(m2[0], 0xff);
    expect_eq(m2[1], 0x80);

    const parsed_names = try r.readNameList(alloc);
    defer alloc.free(parsed_names);
    expect_eq(parsed_names.len, 2);
    expect_eq(std.mem.eql(u8, parsed_names[0], "curve25519-sha256"), true);
    expect_eq(std.mem.eql(u8, parsed_names[1], "ecdh-sha2-nistp256"), true);
    expect_eq(r.remaining(), 0);

    // Truncated input guard
    let r_trunc = SshReader.init(wire[0..2]);
    let got_eof = false;
    * = r_trunc.readU32() catch |e| {
        if (e == SshError.UnexpectedEof) {
            got_eof = true;
        }
    };
    expect_eq(got_eof, true);
}

test("OpenSSH public key line & SHA256 fingerprint for Ed25519 and ECDSA-P256") {
    const alloc = std.testing.allocator;

    // 1. Ed25519
    const seed: [32]u8 = @splat(0x42);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch unreachable;
    const ed_blob = try buildEd25519PublicKeyBlob(alloc, kp.public_key.toBytes());
    defer alloc.free(ed_blob);

    const b64_len = std.base64.standard.Encoder.calcSize(ed_blob.len);
    const b64_buf = try alloc.alloc(u8, b64_len);
    defer alloc.free(b64_buf);
    const ed_b64 = std.base64.standard.Encoder.encode(b64_buf, ed_blob);

    const ed_line = try std.fmt.allocPrint(alloc, "ssh-ed25519 {s} alice@cp", .{ed_b64});
    defer alloc.free(ed_line);

    let parsed_ed = try parsePublicKeyLine(alloc, ed_line);
    defer parsed_ed.deinit(alloc);
    expect_eq(parsed_ed.key_type == SshKeyType.Ed25519, true);
    expect_eq(std.mem.eql(u8, &parsed_ed.ed25519_pub, &kp.public_key.toBytes()), true);
    expect_eq(std.mem.eql(u8, parsed_ed.comment, "alice@cp"), true);

    const formatted_ed = try formatPublicKeyLine(alloc, &parsed_ed);
    defer alloc.free(formatted_ed);
    expect_eq(std.mem.eql(u8, formatted_ed, ed_line), true);

    const fp_ed = try computeSha256Fingerprint(alloc, parsed_ed.raw_blob);
    defer alloc.free(fp_ed);
    expect_eq(std.mem.startsWith(u8, fp_ed, "SHA256:"), true);
    expect_eq(std.mem.indexOfScalar(u8, fp_ed, '=') == null, true);

    // 2. ECDSA P-256
    const p256_seed: [32]u8 = @splat(0x19);
    const p256_kp = std.crypto.sign.ecdsa.EcdsaP256Sha256.KeyPair.generateDeterministic(p256_seed) catch unreachable;
    const sec1 = p256_kp.public_key.toUncompressedSec1();
    const p256_blob = try buildEcdsaP256PublicKeyBlob(alloc, sec1);
    defer alloc.free(p256_blob);

    const p256_b64_len = std.base64.standard.Encoder.calcSize(p256_blob.len);
    const p256_b64_buf = try alloc.alloc(u8, p256_b64_len);
    defer alloc.free(p256_b64_buf);
    const p256_b64 = std.base64.standard.Encoder.encode(p256_b64_buf, p256_blob);

    const p256_line = try std.fmt.allocPrint(alloc, "ecdsa-sha2-nistp256 {s} bob@cp", .{p256_b64});
    defer alloc.free(p256_line);

    let parsed_p256 = try parsePublicKeyLine(alloc, p256_line);
    defer parsed_p256.deinit(alloc);
    expect_eq(parsed_p256.key_type == SshKeyType.EcdsaP256, true);
    expect_eq(std.mem.eql(u8, &parsed_p256.ecdsa_p256_pub, &sec1), true);
    expect_eq(std.mem.eql(u8, parsed_p256.comment, "bob@cp"), true);
}

test("openssh-key-v1 private key roundtrip & checkint/magic guards") {
    const alloc = std.testing.allocator;
    const seed: [32]u8 = @splat(0x7a);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch unreachable;
    const secret_64 = kp.secret_key.toBytes();

    const pem = try encodeOpenSshPrivateKeyEd25519(alloc, secret_64, "deploy@cp", 0xdeadbeef);
    defer alloc.free(pem);

    let parsed = try parseOpenSshPrivateKey(alloc, pem);
    defer parsed.deinit(alloc);
    expect_eq(parsed.key_type == SshKeyType.Ed25519, true);
    expect_eq(std.mem.eql(u8, &parsed.ed25519_pub, &kp.public_key.toBytes()), true);
    expect_eq(std.mem.eql(u8, &parsed.ed25519_secret, &secret_64), true);
    expect_eq(std.mem.eql(u8, parsed.comment, "deploy@cp"), true);

    // Invalid magic guard
    const bad_magic_pem =
        "-----BEGIN OPENSSH PRIVATE KEY-----\n" ++
        "YmFkLW1hZ2ljLXYxAAAA\n" ++
        "-----END OPENSSH PRIVATE KEY-----\n";
    let got_bad_magic = false;
    * = parseOpenSshPrivateKey(alloc, bad_magic_pem) catch |e| {
        if (e == SshError.InvalidMagic) {
            got_bad_magic = true;
        }
    };
    expect_eq(got_bad_magic, true);

    // Checkint mismatch guard: construct raw payload with checkint1 != checkint2
    const pub_blob = try buildEd25519PublicKeyBlob(alloc, kp.public_key.toBytes());
    defer alloc.free(pub_blob);
    let pw = SshWriter.init(alloc);
    defer pw.deinit();
    try pw.writeU32(0x11111111);
    try pw.writeU32(0x22222222);
    try pw.writeString("ssh-ed25519");
    try pw.writeString(&kp.public_key.toBytes());
    try pw.writeString(&secret_64);
    try pw.writeString("bad");

    let ow = SshWriter.init(alloc);
    defer ow.deinit();
    try ow.writeRaw("openssh-key-v1\x00");
    try ow.writeString("none");
    try ow.writeString("none");
    try ow.writeString("");
    try ow.writeU32(1);
    try ow.writeString(pub_blob);
    try ow.writeString(pw.items());

    const b64_len = std.base64.standard.Encoder.calcSize(ow.items().len);
    const b64_buf = try alloc.alloc(u8, b64_len);
    defer alloc.free(b64_buf);
    const b64 = std.base64.standard.Encoder.encode(b64_buf, ow.items());
    const bad_checkint_pem = try std.fmt.allocPrint(
        alloc,
        "-----BEGIN OPENSSH PRIVATE KEY-----\n{s}\n-----END OPENSSH PRIVATE KEY-----\n",
        .{b64},
    );
    defer alloc.free(bad_checkint_pem);

    let got_checkint = false;
    * = parseOpenSshPrivateKey(alloc, bad_checkint_pem) catch |e| {
        if (e == SshError.CheckintMismatch) {
            got_checkint = true;
        }
    };
    expect_eq(got_checkint, true);
}

test("SSHSIG Ed25519 sign & verify & namespace/tamper guards") {
    const alloc = std.testing.allocator;
    const seed: [32]u8 = @splat(0x9c);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch unreachable;
    const secret_64 = kp.secret_key.toBytes();
    const pub_32 = kp.public_key.toBytes();

    const msg = "artifact-digest-sha256:abcdef0123456789";
    const sig_pem = try signSshSigEd25519(alloc, secret_64, "git", msg);
    defer alloc.free(sig_pem);

    expect_eq(std.mem.startsWith(u8, sig_pem, "-----BEGIN SSH SIGNATURE-----"), true);
    try verifySshSigEd25519(alloc, pub_32, "git", msg, sig_pem);

    // Wrong namespace guard
    let got_ns_err = false;
    verifySshSigEd25519(alloc, pub_32, "file", msg, sig_pem) catch |e| {
        if (e == SshError.InvalidNamespace) {
            got_ns_err = true;
        }
    };
    expect_eq(got_ns_err, true);

    // Tampered message guard
    let got_sig_err = false;
    verifySshSigEd25519(alloc, pub_32, "git", "tampered-payload", sig_pem) catch |e| {
        if (e == SshError.InvalidSignature) {
            got_sig_err = true;
        }
    };
    expect_eq(got_sig_err, true);
}

test("cap.ssh_wire dynamic mount & std.testing.allocator zero leak") {
    const alloc = std.testing.allocator;
    let w = cap.ssh_wire.SshWriter.init(alloc);
    defer w.deinit();

    try w.writeString("cap-ssh-wire");
    try w.writeU32(42);
    const bytes = try w.toOwnedSlice();
    defer alloc.free(bytes);

    let r = cap.ssh_wire.SshReader.init(bytes);
    const tag = try r.readString();
    const num = try r.readU32();
    expect_eq(std.mem.eql(u8, tag, "cap-ssh-wire"), true);
    expect_eq(num, 42);
    expect_eq(r.remaining(), 0);
}
