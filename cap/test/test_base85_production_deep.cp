// Source: cp/cap/test/test_base85_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:base85 & cap.base85

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Base85Error,
    Z85,
    Ascii85,
    Rfc1924,
} from "cap:base85";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // Empty input
    const empty_enc = Z85.encode(alloc, "")?;
    defer alloc.free(empty_enc);
    expect_eq(empty_enc.len, 0 as usize);

    const empty_dec = Z85.decode(alloc, "")?;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0 as usize);

    // ZeroMQ Spec 32 Z85 official test vector: 0x864FD26FB559F75B -> "HelloWorld"
    const raw: [8]u8 = [0x86, 0x4F, 0xD2, 0x6F, 0xB5, 0x59, 0xF7, 0x5B];
    const enc = Z85.encode(alloc, &raw)?;
    defer alloc.free(enc);
    expect_eq(std.mem.eql(u8, enc, "HelloWorld"), true);

    const z85_dec = Z85.decode(alloc, "HelloWorld")?;
    defer alloc.free(z85_dec);
    expect_eq(std.mem.eql(u8, z85_dec, &raw), true);

    // All zeros 4 bytes -> "00000", all 0xFF 4 bytes -> "%nSc0"
    const zeros: [4]u8 = [0x00, 0x00, 0x00, 0x00];
    const z_enc = Z85.encode(alloc, &zeros)?;
    defer alloc.free(z_enc);
    expect_eq(std.mem.eql(u8, z_enc, "00000"), true);

    const max32: [4]u8 = [0xFF, 0xFF, 0xFF, 0xFF];
    const m_enc = Z85.encode(alloc, &max32)?;
    defer alloc.free(m_enc);
    expect_eq(std.mem.eql(u8, m_enc, "%nSc0"), true);

    // Non-4-byte encode length -> InvalidLength
    let bad_enc_len = false;
    * = Z85.encode(alloc, "abc") catch e => err_el: {
        bad_enc_len = (e == Base85Error.InvalidLength);
        break :err_el emptyMutSlice();
    };
    expect_eq(bad_enc_len, true);

    // Non-5-char decode length -> InvalidLength
    let bad_dec_len = false;
    * = Z85.decode(alloc, "HelloW") catch e => err_dl: {
        bad_dec_len = (e == Base85Error.InvalidLength);
        break :err_dl emptyMutSlice();
    };
    expect_eq(bad_dec_len, true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // Arbitrary lengths: 1, 2, 3, 4, 5, 7 bytes
    const sample: [7]u8 = [0x4D, 0x61, 0x6E, 0x20, 0x69, 0x73, 0x21]; // "Man is!"
    let len_idx: usize = 1;
    while (len_idx <= 7) : (len_idx += 1) {
        const slice = sample[0..len_idx];
        const enc = Ascii85.encode(alloc, slice)?;
        defer alloc.free(enc);
        const a85_dec = Ascii85.decode(alloc, enc)?;
        defer alloc.free(a85_dec);
        expect_eq(std.mem.eql(u8, a85_dec, slice), true);

        const framed = Ascii85.encodeFramed(alloc, slice)?;
        defer alloc.free(framed);
        expect_eq(framed.len >= 4, true);
        expect_eq(framed[0] == '<' && framed[1] == '~', true);
        expect_eq(framed[framed.len - 2] == '~' && framed[framed.len - 1] == '>', true);

        const dec_framed = Ascii85.decodeFramed(alloc, framed)?;
        defer alloc.free(dec_framed);
        expect_eq(std.mem.eql(u8, dec_framed, slice), true);
    }

    // All-zero 4-byte compression -> 'z'
    const z4: [4]u8 = [0, 0, 0, 0];
    const z4_enc = Ascii85.encode(alloc, &z4)?;
    defer alloc.free(z4_enc);
    expect_eq(std.mem.eql(u8, z4_enc, "z"), true);

    const z8: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    const z8_framed = Ascii85.encodeFramed(alloc, &z8)?;
    defer alloc.free(z8_framed);
    expect_eq(std.mem.eql(u8, z8_framed, "<~zz~>"), true);

    // Whitespace-ignoring decode & decodeFramed ("Man ") -> "9jqo^"
    const ws_raw = " 9j \n\t qo^ \r ";
    const ws_dec = Ascii85.decode(alloc, ws_raw)?;
    defer alloc.free(ws_dec);
    expect_eq(std.mem.eql(u8, ws_dec, "Man "), true);

    const ws_framed = "  <~ 9jqo^ \n z ~>  ";
    const ws_fdec = Ascii85.decodeFramed(alloc, ws_framed)?;
    defer alloc.free(ws_fdec);
    const expected_ws: [8]u8 = ['M', 'a', 'n', ' ', 0, 0, 0, 0];
    expect_eq(std.mem.eql(u8, ws_fdec, &expected_ws), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // Empty round-trip
    const empty_enc = Rfc1924.encode(alloc, "")?;
    defer alloc.free(empty_enc);
    expect_eq(empty_enc.len, 0 as usize);
    const empty_dec = Rfc1924.decode(alloc, "")?;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0 as usize);

    // 0xFFFFFFFF -> "|NsC0"
    const max32: [4]u8 = [0xFF, 0xFF, 0xFF, 0xFF];
    const max_enc = Rfc1924.encode(alloc, &max32)?;
    defer alloc.free(max_enc);
    expect_eq(std.mem.eql(u8, max_enc, "|NsC0"), true);
    const max_dec = Rfc1924.decode(alloc, "|NsC0")?;
    defer alloc.free(max_dec);
    expect_eq(std.mem.eql(u8, max_dec, &max32), true);

    // Arbitrary byte lengths (1..16 bytes, all byte values)
    let buf: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        buf[i] = ((i * 37) & 0xFF) as u8;
    }
    let l: usize = 1;
    while (l <= 16) : (l += 1) {
        const sub = buf[0..l];
        const enc = Rfc1924.encode(alloc, sub)?;
        defer alloc.free(enc);
        const rfc_dec = Rfc1924.decode(alloc, enc)?;
        defer alloc.free(rfc_dec);
        expect_eq(std.mem.eql(u8, rfc_dec, sub), true);
    }

    // Invalid length (mod 5 == 1)
    let bad_len = false;
    * = Rfc1924.decode(alloc, "A") catch e => err_rlen: {
        bad_len = (e == Base85Error.InvalidLength);
        break :err_rlen emptyMutSlice();
    };
    expect_eq(bad_len, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // InvalidCharacter across Z85, Ascii85, Rfc1924
    let z85_bad_char = false;
    * = Z85.decode(alloc, "Hell~") catch e => err_zc: {
        z85_bad_char = (e == Base85Error.InvalidCharacter);
        break :err_zc emptyMutSlice();
    };
    expect_eq(z85_bad_char, true);

    let a85_bad_char = false;
    * = Ascii85.decode(alloc, "9jqov") catch e => err_ac: {
        a85_bad_char = (e == Base85Error.InvalidCharacter);
        break :err_ac emptyMutSlice();
    };
    expect_eq(a85_bad_char, true);

    let rfc_bad_char = false;
    * = Rfc1924.decode(alloc, "0123,") catch e => err_rc: {
        rfc_bad_char = (e == Base85Error.InvalidCharacter);
        break :err_rc emptyMutSlice();
    };
    expect_eq(rfc_bad_char, true);

    // 32-bit ValueOverflow: Z85 "%%%%%", Ascii85 "uuuuu", Rfc1924 "|NsC1"
    let z85_ovf = false;
    * = Z85.decode(alloc, "%%%%%") catch e => err_zo: {
        z85_ovf = (e == Base85Error.ValueOverflow);
        break :err_zo emptyMutSlice();
    };
    expect_eq(z85_ovf, true);

    let a85_ovf = false;
    * = Ascii85.decode(alloc, "uuuuu") catch e => err_ao: {
        a85_ovf = (e == Base85Error.ValueOverflow);
        break :err_ao emptyMutSlice();
    };
    expect_eq(a85_ovf, true);

    let rfc_ovf = false;
    * = Rfc1924.decode(alloc, "|NsC1") catch e => err_ro: {
        rfc_ovf = (e == Base85Error.ValueOverflow);
        break :err_ro emptyMutSlice();
    };
    expect_eq(rfc_ovf, true);

    // 'z' in the middle of a block -> MalformedFraming
    let mid_z = false;
    * = Ascii85.decode(alloc, "9jzqo") catch e => err_mz: {
        mid_z = (e == Base85Error.MalformedFraming);
        break :err_mz emptyMutSlice();
    };
    expect_eq(mid_z, true);

    // Missing or malformed <~ / ~> framing -> MalformedFraming
    let no_prefix = false;
    * = Ascii85.decodeFramed(alloc, "9jqo^~>") catch e => err_np: {
        no_prefix = (e == Base85Error.MalformedFraming);
        break :err_np emptyMutSlice();
    };
    expect_eq(no_prefix, true);

    let no_suffix = false;
    * = Ascii85.decodeFramed(alloc, "<~9jqo^") catch e => err_ns: {
        no_suffix = (e == Base85Error.MalformedFraming);
        break :err_ns emptyMutSlice();
    };
    expect_eq(no_suffix, true);

    let trailing_garbage = false;
    * = Ascii85.decodeFramed(alloc, "<~9jqo^~>x") catch e => err_tg: {
        trailing_garbage = (e == Base85Error.MalformedFraming);
        break :err_tg emptyMutSlice();
    };
    expect_eq(trailing_garbage, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mounting via import cap from "cap"; cap.base85
    const payload: [12]u8 = [
        0x00, 0x00, 0x00, 0x00,
        0x86, 0x4F, 0xD2, 0x6F,
        0xB5, 0x59, 0xF7, 0x5B,
    ];

    const z_enc = cap.base85.Z85.encode(alloc, &payload)?;
    defer alloc.free(z_enc);
    const z_dec = cap.base85.Z85.decode(alloc, z_enc)?;
    defer alloc.free(z_dec);
    expect_eq(std.mem.eql(u8, z_dec, &payload), true);

    const a_enc = cap.base85.Ascii85.encodeFramed(alloc, &payload)?;
    defer alloc.free(a_enc);
    const a_dec = cap.base85.Ascii85.decodeFramed(alloc, a_enc)?;
    defer alloc.free(a_dec);
    expect_eq(std.mem.eql(u8, a_dec, &payload), true);

    const r_enc = cap.base85.Rfc1924.encode(alloc, &payload)?;
    defer alloc.free(r_enc);
    const r_dec = cap.base85.Rfc1924.decode(alloc, r_enc)?;
    defer alloc.free(r_dec);
    expect_eq(std.mem.eql(u8, r_dec, &payload), true);
}

test("cap:base85 Z85 ZeroMQ Spec 32 HelloWorld & alignment validation") {
    runTest1() catch unreachable;
}

test("cap:base85 Ascii85 arbitrary lengths, z-compression & <~...~> framing") {
    runTest2() catch unreachable;
}

test("cap:base85 Rfc1924 round-trip & arbitrary byte coverage") {
    runTest3() catch unreachable;
}

test("cap:base85 error interception: InvalidCharacter, ValueOverflow, MalformedFraming") {
    runTest4() catch unreachable;
}

test("cap.base85 dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
