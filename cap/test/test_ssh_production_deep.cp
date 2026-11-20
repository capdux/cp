// Source: cp/cap/test/test_ssh_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:ssh & cap.ssh

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { sha256Raw } from "cap:crypto";
import { Base64 } from "cap:encoding";
import {
    SshError,
    SshKeyType,
    SshPublicKey,
    AuthorizedKeyEntry,
    AuthorizedKeys,
} from "cap:ssh";

fn appendWireString(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): !void {
    const len_u32 = u32(s.len);
    const hdr: [4]u8 = [
        u8((len_u32 >> 24) & 0xFF),
        u8((len_u32 >> 16) & 0xFF),
        u8((len_u32 >> 8) & 0xFF),
        u8(len_u32 & 0xFF),
    ];
    try buf.appendSlice(alloc, &hdr);
    try buf.appendSlice(alloc, s);
}

fn buildEd25519LineAlloc(alloc: std.mem.Allocator, seed_byte: u8, comment: string): ![]u8 {
    let wire: std.ArrayList(u8) = .empty;
    defer wire.deinit(alloc);
    try appendWireString(&wire, alloc, "ssh-ed25519");
    let pk: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        pk[i] = seed_byte +% u8(i);
    }
    try appendWireString(&wire, alloc, &pk);

    const b64 = try Base64.encodeAlloc(alloc, wire.items);
    defer alloc.free(b64);

    if (comment.len > 0) {
        return std.fmt.allocPrint(alloc, "ssh-ed25519 {s} {s}", .{ b64, comment });
    }
    return std.fmt.allocPrint(alloc, "ssh-ed25519 {s}", .{b64});
}

fn buildRsaLineAlloc(alloc: std.mem.Allocator, comment: string): ![]u8 {
    let wire: std.ArrayList(u8) = .empty;
    defer wire.deinit(alloc);
    try appendWireString(&wire, alloc, "ssh-rsa");
    const e: [3]u8 = [0x01, 0x00, 0x01];
    try appendWireString(&wire, alloc, &e);
    let n: [32]u8 = [0xAB; 32];
    n[0] = 0x00;
    n[1] = 0xC1;
    try appendWireString(&wire, alloc, &n);

    const b64 = try Base64.encodeAlloc(alloc, wire.items);
    defer alloc.free(b64);
    if (comment.len > 0) {
        return std.fmt.allocPrint(alloc, "ssh-rsa {s} {s}", .{ b64, comment });
    }
    return std.fmt.allocPrint(alloc, "ssh-rsa {s}", .{b64});
}

fn buildEcdsaP256LineAlloc(alloc: std.mem.Allocator, comment: string): ![]u8 {
    let wire: std.ArrayList(u8) = .empty;
    defer wire.deinit(alloc);
    try appendWireString(&wire, alloc, "ecdsa-sha2-nistp256");
    try appendWireString(&wire, alloc, "nistp256");
    let q: [65]u8 = [0x55; 65];
    q[0] = 0x04;
    try appendWireString(&wire, alloc, &q);

    const b64 = try Base64.encodeAlloc(alloc, wire.items);
    defer alloc.free(b64);
    if (comment.len > 0) {
        return std.fmt.allocPrint(alloc, "ecdsa-sha2-nistp256 {s} {s}", .{ b64, comment });
    }
    return std.fmt.allocPrint(alloc, "ecdsa-sha2-nistp256 {s}", .{b64});
}

fn buildSkEd25519LineAlloc(alloc: std.mem.Allocator, comment: string): ![]u8 {
    let wire: std.ArrayList(u8) = .empty;
    defer wire.deinit(alloc);
    try appendWireString(&wire, alloc, "sk-ssh-ed25519@openssh.com");
    let pk: [32]u8 = [0x77; 32];
    try appendWireString(&wire, alloc, &pk);
    try appendWireString(&wire, alloc, "ssh:");

    const b64 = try Base64.encodeAlloc(alloc, wire.items);
    defer alloc.free(b64);
    if (comment.len > 0) {
        return std.fmt.allocPrint(alloc, "sk-ssh-ed25519@openssh.com {s} {s}", .{ b64, comment });
    }
    return std.fmt.allocPrint(alloc, "sk-ssh-ed25519@openssh.com {s}", .{b64});
}

fn testParseFingerprintAndFormatRoundTrip(): !void {
    const alloc = std.testing.allocator;

    // 1. Ed25519
    const ed_line = try buildEd25519LineAlloc(alloc, 0x01, "alice@cp-lang.org");
    defer alloc.free(ed_line);

    let ed_key = try SshPublicKey.parse(alloc, ed_line);
    defer ed_key.deinit();

    expect_eq(ed_key.key_type == SshKeyType.Ed25519, true);
    expect_eq(ed_key.algo_name, "ssh-ed25519");
    expect_eq(ed_key.comment, "alice@cp-lang.org");
    expect_eq(ed_key.wire_blob.len, 4 + 11 + 4 + 32);

    const fp = try ed_key.fingerprintSha256(alloc);
    defer alloc.free(fp);
    expect_eq(std.mem.startsWith(u8, fp, "SHA256:"), true);
    expect_eq(fp.len, 50); // "SHA256:" (7) + 43 unpadded base64 chars
    expect_eq(fp[fp.len - 1] != '=', true);

    // Cross-check fingerprint against independent SHA-256 + unpadded Base64
    let raw_digest: [32]u8 = undefined;
    sha256Raw(&raw_digest, ed_key.wire_blob);
    let expected_b64: [43]u8 = undefined;
    const b64_n = Base64.encodeNoPad(&expected_b64, &raw_digest);
    expect_eq(b64_n, 43);
    expect_eq(fp[7..50], expected_b64[0..43]);

    // Format round-trip
    const formatted = try ed_key.format(alloc);
    defer alloc.free(formatted);
    expect_eq(formatted, ed_line);

    let reparsed = try SshPublicKey.parse(alloc, formatted);
    defer reparsed.deinit();
    expect_eq(ed_key.equals(&reparsed), true);

    // 2. RSA
    const rsa_line = try buildRsaLineAlloc(alloc, "bob@host");
    defer alloc.free(rsa_line);
    let rsa_key = try SshPublicKey.parse(alloc, rsa_line);
    defer rsa_key.deinit();
    expect_eq(rsa_key.key_type == SshKeyType.Rsa, true);
    const rsa_fmt = try rsa_key.format(alloc);
    defer alloc.free(rsa_fmt);
    expect_eq(rsa_fmt, rsa_line);
    const rsa_fp = try rsa_key.fingerprintSha256(alloc);
    defer alloc.free(rsa_fp);
    expect_eq(rsa_fp.len, 50);

    // 3. ECDSA P-256
    const ecdsa_line = try buildEcdsaP256LineAlloc(alloc, "carol@host");
    defer alloc.free(ecdsa_line);
    let ecdsa_key = try SshPublicKey.parse(alloc, ecdsa_line);
    defer ecdsa_key.deinit();
    expect_eq(ecdsa_key.key_type == SshKeyType.EcdsaP256, true);
    const ecdsa_fmt = try ecdsa_key.format(alloc);
    defer alloc.free(ecdsa_fmt);
    expect_eq(ecdsa_fmt, ecdsa_line);
    expect_eq(ed_key.equals(&ecdsa_key), false);

    // 4. SK-Ed25519
    const sk_line = try buildSkEd25519LineAlloc(alloc, "");
    defer alloc.free(sk_line);
    let sk_key = try SshPublicKey.parse(alloc, sk_line);
    defer sk_key.deinit();
    expect_eq(sk_key.key_type == SshKeyType.SkEd25519, true);
    expect_eq(sk_key.comment, "");
}

fn testProtocolSecurityValidation(): !void {
    const alloc = std.testing.allocator;

    // 1. AlgorithmMismatch: outer says ssh-rsa, inner wire says ssh-ed25519
    let ed_wire: std.ArrayList(u8) = .empty;
    defer ed_wire.deinit(alloc);
    try appendWireString(&ed_wire, alloc, "ssh-ed25519");
    let pk: [32]u8 = [0x11; 32];
    try appendWireString(&ed_wire, alloc, &pk);
    const ed_b64 = try Base64.encodeAlloc(alloc, ed_wire.items);
    defer alloc.free(ed_b64);

    const mismatch_line = try std.fmt.allocPrint(alloc, "ssh-rsa {s} attacker", .{ed_b64});
    defer alloc.free(mismatch_line);

    let got_mismatch = false;
    let k1 = SshPublicKey.parse(alloc, mismatch_line) catch e => {
        expect_eq(e == SshError.AlgorithmMismatch, true);
        got_mismatch = true;
    };
    if (!got_mismatch) {
        k1.deinit();
    }
    expect_eq(got_mismatch, true);

    // 2. WireTruncated: truncated wire blob
    const trunc_b64 = try Base64.encodeAlloc(alloc, ed_wire.items[0..20]);
    defer alloc.free(trunc_b64);
    const trunc_line = try std.fmt.allocPrint(alloc, "ssh-ed25519 {s}", .{trunc_b64});
    defer alloc.free(trunc_line);

    let got_trunc = false;
    let k2 = SshPublicKey.parse(alloc, trunc_line) catch e => {
        expect_eq(e == SshError.WireTruncated, true);
        got_trunc = true;
    };
    if (!got_trunc) {
        k2.deinit();
    }
    expect_eq(got_trunc, true);

    // 3. InvalidBase64
    let got_b64_err = false;
    let k3 = SshPublicKey.parse(alloc, "ssh-ed25519 !!!invalid_b64@@@") catch e => {
        expect_eq(e == SshError.InvalidBase64, true);
        got_b64_err = true;
    };
    if (!got_b64_err) {
        k3.deinit();
    }
    expect_eq(got_b64_err, true);

    // 4. UnsupportedKeyType
    let dss_wire: std.ArrayList(u8) = .empty;
    defer dss_wire.deinit(alloc);
    try appendWireString(&dss_wire, alloc, "ssh-dss");
    const dss_b64 = try Base64.encodeAlloc(alloc, dss_wire.items);
    defer alloc.free(dss_b64);
    const dss_line = try std.fmt.allocPrint(alloc, "ssh-dss {s}", .{dss_b64});
    defer alloc.free(dss_line);

    let got_unsup = false;
    let k4 = SshPublicKey.parse(alloc, dss_line) catch e => {
        expect_eq(e == SshError.UnsupportedKeyType, true);
        got_unsup = true;
    };
    if (!got_unsup) {
        k4.deinit();
    }
    expect_eq(got_unsup, true);
}

fn testAuthorizedKeysComplexParsing(): !void {
    const alloc = std.testing.allocator;

    const k1 = try buildEd25519LineAlloc(alloc, 0x10, "deploy@prod");
    defer alloc.free(k1);
    const k2 = try buildRsaLineAlloc(alloc, "ca-root");
    defer alloc.free(k2);
    const k3 = try buildEcdsaP256LineAlloc(alloc, "ops@host");
    defer alloc.free(k3);

    const content = try std.fmt.allocPrint(
        alloc,
        "# Header comment\r\n" ++
        "   # Indented comment\n" ++
        "\n" ++
        "command=\"git-shell -c \\\"echo hello, world\\\"\",no-pty,no-port-forwarding,from=\"10.0.0.0/8,192.168.1.0/24\",principals=\"deploy,ci-bot\" {s}\r\n" ++
        "cert-authority,principals=\"admin\" {s}\n" ++
        "   \t  \n" ++
        "{s}\n",
        .{ k1, k2, k3 },
    );
    defer alloc.free(content);

    const entries = try AuthorizedKeys.parseFile(alloc, content);
    defer AuthorizedKeys.freeEntries(alloc, entries);

    expect_eq(entries.len, 3);

    // Entry 0: complex quoted options with commas & escaped quotes
    expect_eq(entries[0].command.?, "git-shell -c \"echo hello, world\"");
    expect_eq(entries[0].from_pattern.?, "10.0.0.0/8,192.168.1.0/24");
    expect_eq(entries[0].principals.?, "deploy,ci-bot");
    expect_eq(entries[0].no_pty, true);
    expect_eq(entries[0].no_port_forwarding, true);
    expect_eq(entries[0].cert_authority, false);
    expect_eq(entries[0].public_key.key_type == SshKeyType.Ed25519, true);
    expect_eq(entries[0].public_key.comment, "deploy@prod");

    // Entry 1: cert-authority
    expect_eq(entries[1].cert_authority, true);
    expect_eq(entries[1].principals.?, "admin");
    expect_eq(entries[1].no_pty, false);
    expect_eq(entries[1].no_port_forwarding, false);
    expect_eq(entries[1].command == null, true);
    expect_eq(entries[1].public_key.key_type == SshKeyType.Rsa, true);

    // Entry 2: plain key without options
    expect_eq(entries[2].options_raw, "");
    expect_eq(entries[2].command == null, true);
    expect_eq(entries[2].from_pattern == null, true);
    expect_eq(entries[2].public_key.key_type == SshKeyType.EcdsaP256, true);
    expect_eq(entries[2].public_key.comment, "ops@host");
}

fn testAuthorizedKeysMatchKey(): !void {
    const alloc = std.testing.allocator;

    const k1 = try buildEd25519LineAlloc(alloc, 0x21, "user1@host");
    defer alloc.free(k1);
    const k2 = try buildEd25519LineAlloc(alloc, 0x42, "user2@host");
    defer alloc.free(k2);

    const auth_content = try std.fmt.allocPrint(
        alloc,
        "no-port-forwarding {s}\n{s}\n",
        .{ k1, k2 },
    );
    defer alloc.free(auth_content);

    const entries = try AuthorizedKeys.parseFile(alloc, auth_content);
    defer AuthorizedKeys.freeEntries(alloc, entries);

    // Candidate 1 with different comment should still match entry 0
    const cand1_line = try buildEd25519LineAlloc(alloc, 0x21, "different-comment");
    defer alloc.free(cand1_line);
    let cand1 = try SshPublicKey.parse(alloc, cand1_line);
    defer cand1.deinit();

    // Candidate 2 should match entry 1
    const cand2_line = try buildEd25519LineAlloc(alloc, 0x42, "");
    defer alloc.free(cand2_line);
    let cand2 = try SshPublicKey.parse(alloc, cand2_line);
    defer cand2.deinit();

    // Unauthorized candidate should return null
    const unauth_line = try buildEd25519LineAlloc(alloc, 0x99, "stranger");
    defer alloc.free(unauth_line);
    let unauth = try SshPublicKey.parse(alloc, unauth_line);
    defer unauth.deinit();

    expect_eq(AuthorizedKeys.matchKey(entries, &cand1).?, 0);
    expect_eq(AuthorizedKeys.matchKey(entries, &cand2).?, 1);
    expect_eq(AuthorizedKeys.matchKey(entries, &unauth) == null, true);
}

fn testCapSshDynamicMount(): !void {
    const alloc = std.testing.allocator;

    const line = try buildEd25519LineAlloc(alloc, 0xAA, "dynamic@cap");
    defer alloc.free(line);

    let key = try cap.ssh.SshPublicKey.parse(alloc, line);
    defer key.deinit();

    expect_eq(key.key_type == cap.ssh.SshKeyType.Ed25519, true);

    const fp = try key.fingerprintSha256(alloc);
    defer alloc.free(fp);
    expect_eq(std.mem.startsWith(u8, fp, "SHA256:"), true);

    const entries = try cap.ssh.AuthorizedKeys.parseFile(alloc, line);
    defer cap.ssh.AuthorizedKeys.freeEntries(alloc, entries);
    expect_eq(cap.ssh.AuthorizedKeys.matchKey(entries, &key).?, 0);
}

test("SshPublicKey: parse Ed25519/RSA/ECDSA-P256/SK-Ed25519, SHA256 fingerprint & format round-trip", testParseFingerprintAndFormatRoundTrip);
test("SshPublicKey: strict security validation (AlgorithmMismatch, WireTruncated, InvalidBase64, UnsupportedKeyType)", testProtocolSecurityValidation);
test("AuthorizedKeys: multi-line parsing with comments, blank lines & quoted/escaped options", testAuthorizedKeysComplexParsing);
test("AuthorizedKeys: matchKey exact wire match and unauthorized null with zero leaks", testAuthorizedKeysMatchKey);
test("cap.ssh: dynamic lazy mount verification", testCapSshDynamicMount);
