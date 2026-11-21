import { test, expect_eq } from "cap:test";
import {
    PgCopySaslError,
    CopyFormat,
    CopyDirection,
    encodeCopyResponse,
    decodeCopyInResponse,
    decodeCopyOutResponse,
    encodeCopyData,
    decodeCopyData,
    encodeCopyDone,
    decodeCopyDone,
    encodeCopyFail,
    decodeCopyFail,
    encodePasswordMessage,
    decodePasswordMessage,
    encodeSaslInitialResponse,
    decodeSaslInitialResponse,
    encodeSaslResponse,
    decodeSaslResponse,
    encodeNegotiateProtocolVersion,
    decodeNegotiateProtocolVersion,
} from "cap:pg";

test("1. CopyInResponse & CopyOutResponse column format round-trip (zero leak)", () => {
    const alloc = std.testing.allocator;
    const cols: [3]CopyFormat = [CopyFormat.text, CopyFormat.binary, CopyFormat.text];

    let in_wire = encodeCopyResponse(alloc, CopyDirection.copy_in, CopyFormat.text, cols[0..]) catch unreachable;
    let in_dec = decodeCopyInResponse(alloc, in_wire.slice()) catch unreachable;
    expect_eq(in_dec.direction == CopyDirection.copy_in, true);
    expect_eq(in_dec.overall_format.isText(), true);
    expect_eq(in_dec.columnCount(), usize(3));
    expect_eq(in_dec.column_formats[0].isText(), true);
    expect_eq(in_dec.column_formats[1].isBinary(), true);
    expect_eq(in_dec.column_formats[2].isText(), true);

    let out_wire = encodeCopyResponse(alloc, CopyDirection.copy_out, CopyFormat.binary, cols[0..2]) catch unreachable;
    let out_dec = decodeCopyOutResponse(alloc, out_wire.slice()) catch unreachable;
    expect_eq(out_dec.direction == CopyDirection.copy_out, true);
    expect_eq(out_dec.overall_format.isBinary(), true);
    expect_eq(out_dec.columnCount(), usize(2));
});

test("2. CopyData, CopyDone & CopyFail encode/decode round-trip", () => {
    const alloc = std.testing.allocator;

    let cd_wire = encodeCopyData(alloc, "1\talice\n2\tbob\n") catch unreachable;
    const cd = decodeCopyData(cd_wire.slice()) catch unreachable;
    expect_eq(std.mem.eql(u8, cd.payload, "1\talice\n2\tbob\n"), true);
    expect_eq(cd.bytes_read, cd_wire.len());

    let done_wire = encodeCopyDone(alloc) catch unreachable;
    expect_eq(done_wire.len(), usize(5));
    expect_eq(decodeCopyDone(done_wire.slice()) catch unreachable, usize(5));

    let fail_wire = encodeCopyFail(alloc, "client aborted copy stream") catch unreachable;
    const cf = decodeCopyFail(fail_wire.slice()) catch unreachable;
    expect_eq(std.mem.eql(u8, cf.message, "client aborted copy stream"), true);
});

test("3. PasswordMessage (cleartext & md5), SASLInitialResponse & SASLResponse round-trip", () => {
    const alloc = std.testing.allocator;

    let pw_plain = encodePasswordMessage(alloc, "hunter2") catch unreachable;
    const d_plain = decodePasswordMessage(pw_plain.slice()) catch unreachable;
    expect_eq(std.mem.eql(u8, d_plain.password, "hunter2"), true);
    expect_eq(d_plain.isMd5Hash(), false);

    let pw_md5 = encodePasswordMessage(alloc, "md50123456789abcdef0123456789abcdef") catch unreachable;
    const d_md5 = decodePasswordMessage(pw_md5.slice()) catch unreachable;
    expect_eq(d_md5.isMd5Hash(), true);

    // SASLInitialResponse with payload
    let sasl_init = encodeSaslInitialResponse(alloc, "SCRAM-SHA-256", "n,,n=,r=fyko+d2lbbFgONRv9qkxdawL") catch unreachable;
    const d_init = decodeSaslInitialResponse(sasl_init.slice()) catch unreachable;
    expect_eq(std.mem.eql(u8, d_init.mechanism, "SCRAM-SHA-256"), true);
    expect_eq(std.mem.eql(u8, d_init.client_first.?, "n,,n=,r=fyko+d2lbbFgONRv9qkxdawL"), true);

    // SASLInitialResponse with null payload (-1 length)
    let sasl_null = encodeSaslInitialResponse(alloc, "SCRAM-SHA-256", null) catch unreachable;
    const d_null = decodeSaslInitialResponse(sasl_null.slice()) catch unreachable;
    expect_eq(d_null.client_first == null, true);

    // SASLResponse (client-final-message)
    let sasl_resp = encodeSaslResponse(alloc, "c=biws,r=nonce,p=proof==") catch unreachable;
    const d_resp = decodeSaslResponse(sasl_resp.slice()) catch unreachable;
    expect_eq(std.mem.eql(u8, d_resp.payload, "c=biws,r=nonce,p=proof=="), true);
});

test("4. NegotiateProtocolVersion unrecognized options round-trip (zero leak)", () => {
    const alloc = std.testing.allocator;
    const opts: [2]string = ["_pq_.protocol_v4", "custom_opt"];

    let wire = encodeNegotiateProtocolVersion(alloc, 196608, opts[0..]) catch unreachable;
    let decoded = decodeNegotiateProtocolVersion(alloc, wire.slice()) catch unreachable;
    expect_eq(decoded.newest_minor_version, u32(196608));
    expect_eq(decoded.optionCount(), usize(2));
    expect_eq(std.mem.eql(u8, decoded.unrecognized_options[0], "_pq_.protocol_v4"), true);
    expect_eq(std.mem.eql(u8, decoded.unrecognized_options[1], "custom_opt"), true);
    expect_eq(decoded.bytes_read, wire.len());
});

test("5. Truncated packets, invalid CopyFormat, empty SASL mechanism & length mismatch guards", () => {
    const alloc = std.testing.allocator;

    // Empty SASL mechanism rejected
    std.testing.expectError(
        PgCopySaslError.EmptyMechanism,
        encodeSaslInitialResponse(alloc, "", "abc"),
    ) catch unreachable;

    // Invalid CopyFormat byte (2) in CopyInResponse
    const bad_cf: [8]u8 = ['G', 0, 0, 0, 7, 2, 0, 0];
    std.testing.expectError(
        PgCopySaslError.InvalidCopyFormat,
        decodeCopyInResponse(alloc, bad_cf[0..]),
    ) catch unreachable;

    // CopyDone with non-zero body rejected
    const bad_done: [6]u8 = ['c', 0, 0, 0, 5, 0];
    std.testing.expectError(PgCopySaslError.InvalidMessageLength, decodeCopyDone(bad_done[0..])) catch unreachable;

    // Truncated NegotiateProtocolVersion (< 8 body bytes)
    const short_neg: [9]u8 = ['v', 0, 0, 0, 8, 0, 3, 0, 0];
    std.testing.expectError(
        PgCopySaslError.InvalidMessageLength,
        decodeNegotiateProtocolVersion(alloc, short_neg[0..]),
    ) catch unreachable;

    // SASLInitialResponse with negative data_len (-2)
    const bad_sasl: [12]u8 = ['p', 0, 0, 0, 11, 'X', 0, 0xFF, 0xFF, 0xFF, 0xFE, 0];
    std.testing.expectError(PgCopySaslError.InvalidDataLength, decodeSaslInitialResponse(bad_sasl[0..])) catch unreachable;
});
