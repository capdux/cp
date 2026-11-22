// Source: cp/cap/test/test_pem_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:pem & cap.pem

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PemError,
    PemHeader,
    PemBlock,
    parse,
    parseWithMaxBlocks,
    parseFirst,
    encode,
    encodeWithHeaders,
    encodeChain,
    freeBlocks,
} from "cap:auth";

fn emptyBlocks(): []PemBlock {
    return std.heap.page_allocator.alloc(PemBlock, 0) catch unreachable;
}

fn emptyBytes(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Single block > 48 bytes so Base64 wraps across multiple 64-char lines
    let payload: [96]u8 = [0; 96];
    let i: usize = 0;
    while (i < 96) : (i += 1) {
        payload[i] = ((i * 13 + 7) & 0xFF) as u8;
    }

    const encoded = encode(alloc, "CERTIFICATE", &payload)?;
    defer alloc.free(encoded);

    // Verify 64-column wrapping: first base64 line should be exactly 64 chars followed by '\n'
    const first_nl = std.mem.indexOfScalar(u8, encoded, '\n').?;
    const body_start = first_nl + 1;
    const second_nl_rel = std.mem.indexOfScalar(u8, encoded[body_start..], '\n').?;
    expect_eq(second_nl_rel, 64 as usize);

    const parsed_single = parseFirst(alloc, encoded)?;
    expect_eq(parsed_single != null, true);
    let first_blk = parsed_single.?;
    defer first_blk.deinit(alloc);
    expect_eq(std.mem.eql(u8, first_blk.label, "CERTIFICATE"), true);
    expect_eq(std.mem.eql(u8, first_blk.bytes, &payload), true);
    expect_eq(first_blk.headers.len, 0 as usize);

    // 2. Multi-block chain with inter-block comments and CRLF (\r\n) line endings
    const chain_input =
        "# Leading comment before leaf cert\r\n" ++
        "-----BEGIN CERTIFICATE-----\r\n" ++
        "AQIDBA==\r\n" ++
        "-----END CERTIFICATE-----\r\n" ++
        "\r\n" ++
        "Issuer: Intermediate CA\r\n" ++
        "-----BEGIN X509 CRL-----\r\n" ++
        "REVGR0g=\r\n" ++
        "-----END X509 CRL-----\r\n" ++
        "Trailing comment ignored\r\n";

    const blocks = parse(alloc, chain_input)?;
    defer freeBlocks(alloc, blocks);
    expect_eq(blocks.len, 2 as usize);
    expect_eq(std.mem.eql(u8, blocks[0].label, "CERTIFICATE"), true);
    const exp0: [4]u8 = [1, 2, 3, 4];
    expect_eq(std.mem.eql(u8, blocks[0].bytes, &exp0), true);

    expect_eq(std.mem.eql(u8, blocks[1].label, "X509 CRL"), true);
    expect_eq(std.mem.eql(u8, blocks[1].bytes, "DEFGH"), true);

    // Round-trip encodeChain -> parse
    const reencoded_chain = encodeChain(alloc, blocks)?;
    defer alloc.free(reencoded_chain);
    const reparsed = parse(alloc, reencoded_chain)?;
    defer freeBlocks(alloc, reparsed);
    expect_eq(reparsed.len, 2 as usize);
    expect_eq(std.mem.eql(u8, reparsed[0].bytes, &exp0), true);
    expect_eq(std.mem.eql(u8, reparsed[1].bytes, "DEFGH"), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    const headers: [2]PemHeader = [
        PemHeader{ name: "Proc-Type", value: "4,ENCRYPTED" },
        PemHeader{ name: "DEK-Info", value: "AES-128-CBC,0123456789ABCDEF0123456789ABCDEF" },
    ];
    const key_bytes = "encrypted-rsa-private-key-material-0123456789";

    const pem_key = encodeWithHeaders(alloc, "RSA PRIVATE KEY", &headers, key_bytes)?;
    defer alloc.free(pem_key);

    const blocks = parse(alloc, pem_key)?;
    defer freeBlocks(alloc, blocks);
    expect_eq(blocks.len, 1 as usize);
    expect_eq(std.mem.eql(u8, blocks[0].label, "RSA PRIVATE KEY"), true);
    expect_eq(std.mem.eql(u8, blocks[0].bytes, key_bytes), true);
    expect_eq(blocks[0].headers.len, 2 as usize);

    const proc_type = blocks[0].header("Proc-Type");
    expect_eq(proc_type != null, true);
    expect_eq(std.mem.eql(u8, proc_type.?, "4,ENCRYPTED"), true);

    // Case-insensitive header lookup
    const dek_info = blocks[0].header("dek-info");
    expect_eq(dek_info != null, true);
    expect_eq(std.mem.eql(u8, dek_info.?, "AES-128-CBC,0123456789ABCDEF0123456789ABCDEF"), true);

    // Missing header returns null
    expect_eq(blocks[0].header("X-Missing") == null, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 1. Mismatched BEGIN A / END B
    let mismatch_err = false;
    * = parse(alloc, "-----BEGIN A-----\nAQID\n-----END B-----\n") catch e => err_mm: {
        mismatch_err = (e == PemError.MismatchedEndLabel);
        break :err_mm emptyBlocks();
    };
    expect_eq(mismatch_err, true);

    // 2. Missing END marker
    let missing_end = false;
    * = parse(alloc, "-----BEGIN CERTIFICATE-----\nAQID\n") catch e => err_me: {
        missing_end = (e == PemError.MissingEndMarker);
        break :err_me emptyBlocks();
    };
    expect_eq(missing_end, true);

    // 3. Invalid Base64 (bad char & bad length/padding)
    let bad_b64_char = false;
    * = parse(alloc, "-----BEGIN CERTIFICATE-----\nAQ!D\n-----END CERTIFICATE-----\n") catch e => err_b1: {
        bad_b64_char = (e == PemError.InvalidBase64);
        break :err_b1 emptyBlocks();
    };
    expect_eq(bad_b64_char, true);

    let bad_b64_len = false;
    * = parse(alloc, "-----BEGIN CERTIFICATE-----\nAQI\n-----END CERTIFICATE-----\n") catch e => err_b2: {
        bad_b64_len = (e == PemError.InvalidBase64);
        break :err_b2 emptyBlocks();
    };
    expect_eq(bad_b64_len, true);

    // 4. Empty payload
    let empty_payload = false;
    * = parse(alloc, "-----BEGIN CERTIFICATE-----\n\n-----END CERTIFICATE-----\n") catch e => err_ep: {
        empty_payload = (e == PemError.EmptyPayload);
        break :err_ep emptyBlocks();
    };
    expect_eq(empty_payload, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. InvalidLabel: empty label in encode
    let empty_enc_label = false;
    * = encode(alloc, "", "abc") catch e => err_el: {
        empty_enc_label = (e == PemError.InvalidLabel);
        break :err_el emptyBytes();
    };
    expect_eq(empty_enc_label, true);

    // 2. InvalidLabel: newline inside label in encode
    let nl_enc_label = false;
    * = encode(alloc, "BAD\nLABEL", "abc") catch e => err_nl: {
        nl_enc_label = (e == PemError.InvalidLabel);
        break :err_nl emptyBytes();
    };
    expect_eq(nl_enc_label, true);

    // 3. InvalidLabel: empty label marker in parse
    let empty_parse_label = false;
    * = parse(alloc, "-----BEGIN-----\nAQID\n-----END-----\n") catch e => err_pl: {
        empty_parse_label = (e == PemError.InvalidLabel);
        break :err_pl emptyBlocks();
    };
    expect_eq(empty_parse_label, true);

    // 4. TooManyBlocks resource exhaustion guard (also verifies errdefer frees block 1 & 2)
    const three_blocks =
        "-----BEGIN CERT-----\nAQID\n-----END CERT-----\n" ++
        "-----BEGIN CERT-----\nBAUG\n-----END CERT-----\n" ++
        "-----BEGIN CERT-----\nBwgJ\n-----END CERT-----\n";

    let too_many = false;
    * = parseWithMaxBlocks(alloc, three_blocks, 2) catch e => err_tm: {
        too_many = (e == PemError.TooManyBlocks);
        break :err_tm emptyBlocks();
    };
    expect_eq(too_many, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mounting via import cap from "cap"; cap.pem
    const secret = "cap-pem-dynamic-mount-payload";
    const encoded = cap.auth.encode(alloc, "PRIVATE KEY", secret)?;
    defer alloc.free(encoded);

    const blocks = cap.auth.parse(alloc, encoded)?;
    defer cap.auth.freeBlocks(alloc, blocks);
    expect_eq(blocks.len, 1 as usize);
    expect_eq(std.mem.eql(u8, blocks[0].label, "PRIVATE KEY"), true);
    expect_eq(std.mem.eql(u8, blocks[0].bytes, secret), true);

    // Empty input returns 0 blocks cleanly
    const no_blocks = cap.auth.parse(alloc, "just plain comment text\n")?;
    defer cap.auth.freeBlocks(alloc, no_blocks);
    expect_eq(no_blocks.len, 0 as usize);
}

test("cap:pem single/multi-block chain parse, comments, CRLF & 64-col wrapping") {
    runTest1() catch unreachable;
}

test("cap:pem RFC 1421 Proc-Type/DEK-Info headers encodeWithHeaders & parse") {
    runTest2() catch unreachable;
}

test("cap:pem error interception: MismatchedEndLabel, MissingEndMarker, InvalidBase64, EmptyPayload") {
    runTest3() catch unreachable;
}

test("cap:pem InvalidLabel & TooManyBlocks resource exhaustion guard") {
    runTest4() catch unreachable;
}

test("cap.pem dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
