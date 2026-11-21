import { test, expect_eq } from "cap:test";
import {
    AUTH_MESSAGE_TAG,
    PgAuthError,
    AuthType,
    authTypeFromInt,
    decodeAuthMessage,
    PgWireWriter,
} from "cap:pg";

test("1. AuthType decoding for Ok, CleartextPassword & MD5Password 4-byte salt (zero leak)", () => {
    const alloc = std.testing.allocator;
    let w = PgWireWriter.init(alloc);

    // AuthenticationOk (code = 0, len = 8)
    const p_ok = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.Ok.toInt()) catch unreachable;
    w.endMessage(p_ok) catch unreachable;
    let msg_ok = decodeAuthMessage(alloc, w.bytes()) catch unreachable;
    expect_eq(msg_ok.isOk(), true);
    expect_eq(msg_ok.bytes_read, usize(9));

    // AuthenticationCleartextPassword (code = 3, len = 8)
    w.reset();
    const p_clr = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.CleartextPassword.toInt()) catch unreachable;
    w.endMessage(p_clr) catch unreachable;
    let msg_clr = decodeAuthMessage(alloc, w.bytes()) catch unreachable;
    expect_eq(msg_clr.auth_type == AuthType.CleartextPassword, true);
    expect_eq(msg_clr.isOk(), false);

    // AuthenticationMD5Password (code = 5, len = 12, 4-byte salt)
    w.reset();
    const p_md5 = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.MD5Password.toInt()) catch unreachable;
    const salt: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
    w.write(salt[0..]) catch unreachable;
    w.endMessage(p_md5) catch unreachable;
    let msg_md5 = decodeAuthMessage(alloc, w.bytes()) catch unreachable;
    expect_eq(msg_md5.auth_type == AuthType.MD5Password, true);
    expect_eq(msg_md5.md5_salt[0], u8(0xDE));
    expect_eq(msg_md5.md5_salt[1], u8(0xAD));
    expect_eq(msg_md5.md5_salt[2], u8(0xBE));
    expect_eq(msg_md5.md5_salt[3], u8(0xEF));
    expect_eq(msg_md5.bytes_read, usize(13));
});

test("2. AuthenticationSASL mechanism list parsing & RAII ~init() zero-leak cleanup", () => {
    const alloc = std.testing.allocator;
    let w = PgWireWriter.init(alloc);

    const pos = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.SASL.toInt()) catch unreachable;
    w.writeCString("SCRAM-SHA-256") catch unreachable;
    w.writeCString("SCRAM-SHA-256-PLUS") catch unreachable;
    w.writeByte(0) catch unreachable; // terminating empty string NUL
    w.endMessage(pos) catch unreachable;

    let msg = decodeAuthMessage(alloc, w.bytes()) catch unreachable;
    expect_eq(msg.auth_type == AuthType.SASL, true);
    expect_eq(msg.sasl_mechanisms.len, usize(2));
    expect_eq(msg.hasMechanism("SCRAM-SHA-256"), true);
    expect_eq(msg.hasMechanism("SCRAM-SHA-256-PLUS"), true);
    expect_eq(msg.hasMechanism("PLAIN"), false);
});

test("3. AuthenticationSASLContinue (SCRAM r/s/i) & AuthenticationSASLFinal decoding", () => {
    const alloc = std.testing.allocator;
    let w = PgWireWriter.init(alloc);

    const p_cont = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.SASLContinue.toInt()) catch unreachable;
    w.write("r=clientNonce+serverNonce,s=c2FsdDEyMzQ=,i=4096") catch unreachable;
    w.endMessage(p_cont) catch unreachable;

    let msg_cont = decodeAuthMessage(alloc, w.bytes()) catch unreachable;
    expect_eq(msg_cont.auth_type == AuthType.SASLContinue, true);
    const info = msg_cont.sasl_continue.?;
    expect_eq(std.mem.eql(u8, info.nonce, "clientNonce+serverNonce"), true);
    expect_eq(std.mem.eql(u8, info.salt_b64, "c2FsdDEyMzQ="), true);
    expect_eq(info.iterations, u32(4096));

    // SASLFinal (server-final-message: v=<verifier>)
    w.reset();
    const p_fin = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.SASLFinal.toInt()) catch unreachable;
    w.write("v=dGVzdHZlcmlmaWVy") catch unreachable;
    w.endMessage(p_fin) catch unreachable;

    let msg_fin = decodeAuthMessage(alloc, w.bytes()) catch unreachable;
    expect_eq(msg_fin.auth_type == AuthType.SASLFinal, true);
    expect_eq(std.mem.eql(u8, msg_fin.payload, "v=dGVzdHZlcmlmaWVy"), true);
});

test("4. PgWireWriter big-endian integers, C-strings, NULL marker & length backpatching", () => {
    const alloc = std.testing.allocator;
    let w = PgWireWriter.init(alloc);

    // Build a PasswordMessage ('p') frame with length backpatching
    const p = w.beginMessage('p') catch unreachable;
    w.writeCString("secret_pw") catch unreachable;
    w.writeI16(-2) catch unreachable;
    w.writeI32(-1) catch unreachable;
    w.writeU64(0x0102030405060708) catch unreachable;
    w.writeNull() catch unreachable;
    w.endMessage(p) catch unreachable;

    const raw = w.bytes();
    expect_eq(raw[0], u8('p'));
    // Body = "secret_pw\0" (10) + i16 (2) + i32 (4) + u64 (8) + null (4) = 28; total length field = 28 + 4 = 32
    expect_eq(raw[1], u8(0));
    expect_eq(raw[2], u8(0));
    expect_eq(raw[3], u8(0));
    expect_eq(raw[4], u8(32));
    expect_eq(raw.len, usize(33));

    // Out-of-bounds patch rejected
    const tiny: [2]u8 = [1, 2];
    std.testing.expectError(PgAuthError.OutOfBoundsPatch, w.pwrite(tiny[0..], 32)) catch unreachable;
});

test("5. Truncated packets, wrong tag, unknown auth code & malformed SCRAM guards", () => {
    const alloc = std.testing.allocator;

    // Truncated (< 9 bytes)
    const short_pkt: [5]u8 = ['R', 0, 0, 0, 8];
    std.testing.expectError(PgAuthError.UnexpectedEof, decodeAuthMessage(alloc, short_pkt[0..])) catch unreachable;

    // Wrong message tag ('Z' instead of 'R')
    const wrong_tag: [9]u8 = ['Z', 0, 0, 0, 8, 0, 0, 0, 0];
    std.testing.expectError(PgAuthError.InvalidMessageTag, decodeAuthMessage(alloc, wrong_tag[0..])) catch unreachable;

    // Unknown auth type code (99)
    std.testing.expectError(PgAuthError.InvalidAuthType, authTypeFromInt(99)) catch unreachable;
    const unknown_code: [9]u8 = ['R', 0, 0, 0, 8, 0, 0, 0, 99];
    std.testing.expectError(PgAuthError.InvalidAuthType, decodeAuthMessage(alloc, unknown_code[0..])) catch unreachable;

    // MD5Password with wrong length (8 instead of 12)
    const bad_md5_len: [9]u8 = ['R', 0, 0, 0, 8, 0, 0, 0, 5];
    std.testing.expectError(PgAuthError.InvalidMessageLength, decodeAuthMessage(alloc, bad_md5_len[0..])) catch unreachable;

    // SASLContinue missing 'i=' field -> InvalidSaslPayload
    let w = PgWireWriter.init(alloc);
    const p1 = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.SASLContinue.toInt()) catch unreachable;
    w.write("r=abc,s=def") catch unreachable;
    w.endMessage(p1) catch unreachable;
    std.testing.expectError(PgAuthError.InvalidSaslPayload, decodeAuthMessage(alloc, w.bytes())) catch unreachable;

    // SASLContinue invalid iteration count ('i=0') -> InvalidIterationCount
    w.reset();
    const p2 = w.beginMessage(AUTH_MESSAGE_TAG) catch unreachable;
    w.writeU32(AuthType.SASLContinue.toInt()) catch unreachable;
    w.write("r=abc,s=def,i=0") catch unreachable;
    w.endMessage(p2) catch unreachable;
    std.testing.expectError(PgAuthError.InvalidIterationCount, decodeAuthMessage(alloc, w.bytes())) catch unreachable;
});
