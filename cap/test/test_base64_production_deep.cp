// Source: cp/cap/test/test_base64_production_deep.cp (Production deep test suite for cap:base64)

const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    Base64Variant,
    Base64Error,
    encodedLen,
    maxDecodedLen,
    exactDecodedLen,
    encodeToSlice,
    decodeToSlice,
    decodeIgnoreWhitespaceToSlice,
    encodeAlloc,
    decodeAlloc,
    StreamingBase64Encoder,
    StreamingBase64Decoder,
} from "cap:base64";

test("RFC 4648 official vectors across all 4 variants") {
    let inputs: [7][]const u8 = ["", "f", "fo", "foo", "foob", "fooba", "foobar"];
    let std_expected: [7][]const u8 = ["", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy"];
    let std_nopad_expected: [7][]const u8 = ["", "Zg", "Zm8", "Zm9v", "Zm9vYg", "Zm9vYmE", "Zm9vYmFy"];

    let enc_buf: [64]u8 = [0; 64];
    let dec_buf: [64]u8 = [0; 64];

    let i: usize = 0;
    while (i < 7) {
        let src = inputs[i];

        // Standard
        let e1 = encodeToSlice(&enc_buf, src, .Standard)?;
        expect_eq(std.mem.eql(u8, e1, std_expected[i]), true);
        expect_eq(encodedLen(src.len, .Standard), std_expected[i].len);
        expect_eq(exactDecodedLen(e1, .Standard)?, src.len);
        let d1 = decodeToSlice(&dec_buf, e1, .Standard)?;
        expect_eq(std.mem.eql(u8, d1, src), true);

        // StandardNoPad
        let e2 = encodeToSlice(&enc_buf, src, .StandardNoPad)?;
        expect_eq(std.mem.eql(u8, e2, std_nopad_expected[i]), true);
        expect_eq(encodedLen(src.len, .StandardNoPad), std_nopad_expected[i].len);
        expect_eq(exactDecodedLen(e2, .StandardNoPad)?, src.len);
        let d2 = decodeToSlice(&dec_buf, e2, .StandardNoPad)?;
        expect_eq(std.mem.eql(u8, d2, src), true);

        // UrlSafe & UrlSafeNoPad round-trip
        let e3 = encodeToSlice(&enc_buf, src, .UrlSafe)?;
        let d3 = decodeToSlice(&dec_buf, e3, .UrlSafe)?;
        expect_eq(std.mem.eql(u8, d3, src), true);

        let e4 = encodeToSlice(&enc_buf, src, .UrlSafeNoPad)?;
        let d4 = decodeToSlice(&dec_buf, e4, .UrlSafeNoPad)?;
        expect_eq(std.mem.eql(u8, d4, src), true);

        i += 1;
    }

    // Verify +/ vs -_ alphabet differentiation (0xFB, 0xEF, 0xFF -> "++//" vs "--__")
    let bin_bytes: [3]u8 = [0xFB, 0xEF, 0xFF];
    let std_bin = encodeToSlice(&enc_buf, &bin_bytes, .Standard)?;
    expect_eq(std.mem.eql(u8, std_bin, "++//"), true);
    let url_bin = encodeToSlice(&enc_buf, &bin_bytes, .UrlSafe)?;
    expect_eq(std.mem.eql(u8, url_bin, "--__"), true);
    let dec_url = decodeToSlice(&dec_buf, url_bin, .UrlSafe)?;
    expect_eq(std.mem.eql(u8, dec_url, &bin_bytes), true);
}

test("RFC 4648 section 3.5 non-canonical padding bits and invalid input rejection") {
    let dec_buf: [64]u8 = [0; 64];
    let small_buf: [2]u8 = [0; 2];

    // Non-canonical trailing bits: "ZH==" has lower 4 bits of 'H' (7) non-zero; "Zm9=" has lower 2 bits of '9' (61) non-zero
    let res_nc1 = decodeToSlice(&dec_buf, "ZH==", .Standard);
    expect_eq(res_nc1 == err.NonCanonicalPaddingBits, true);

    let res_nc2 = decodeToSlice(&dec_buf, "Zm9=", .Standard);
    expect_eq(res_nc2 == err.NonCanonicalPaddingBits, true);

    let res_nc3 = decodeToSlice(&dec_buf, "ZH", .StandardNoPad);
    expect_eq(res_nc3 == err.NonCanonicalPaddingBits, true);

    // Invalid length: len % 4 == 1
    let res_len1 = decodeToSlice(&dec_buf, "A", .StandardNoPad);
    expect_eq(res_len1 == err.InvalidLength, true);

    let res_len2 = decodeToSlice(&dec_buf, "Zm9vA", .StandardNoPad);
    expect_eq(res_len2 == err.InvalidLength, true);

    // Unpadded length in padded mode
    let res_unpadded = decodeToSlice(&dec_buf, "Zm8", .Standard);
    expect_eq(res_unpadded == err.InvalidLength, true);

    // Invalid padding: extra '=', '=' in middle, or '=' in NoPad mode
    let res_pad1 = decodeToSlice(&dec_buf, "Z===", .Standard);
    expect_eq(res_pad1 == err.InvalidPadding, true);

    let res_pad2 = decodeToSlice(&dec_buf, "Zm=v", .Standard);
    expect_eq(res_pad2 == err.InvalidPadding, true);

    let res_pad3 = decodeToSlice(&dec_buf, "Zm8=", .StandardNoPad);
    expect_eq(res_pad3 == err.InvalidPadding, true);

    // Invalid character
    let res_char = decodeToSlice(&dec_buf, "Zm$v", .Standard);
    expect_eq(res_char == err.InvalidCharacter, true);

    // Destination too small
    let res_small = decodeToSlice(&small_buf, "Zm9v", .Standard);
    expect_eq(res_small == err.DestinationTooSmall, true);
    expect_eq(maxDecodedLen(4), 3);
}

test("MIME decodeIgnoreWhitespaceToSlice handles CR LF spaces and tabs") {
    let dec_buf: [64]u8 = [0; 64];
    let mime_input = "  Zm9v\r\n  YmFy \t ";
    let decoded = decodeIgnoreWhitespaceToSlice(&dec_buf, mime_input, .Standard)?;
    expect_eq(std.mem.eql(u8, decoded, "foobar"), true);

    let padded_mime = "Z\nm\r\n8\t=";
    let decoded2 = decodeIgnoreWhitespaceToSlice(&dec_buf, padded_mime, .Standard)?;
    expect_eq(std.mem.eql(u8, decoded2, "fo"), true);

    // Still rejects non-canonical padding bits even with whitespace
    let bad_mime = "Z H\r\n = =";
    let bad_res = decodeIgnoreWhitespaceToSlice(&dec_buf, bad_mime, .Standard);
    expect_eq(bad_res == err.NonCanonicalPaddingBits, true);
}

test("StreamingBase64Encoder and StreamingBase64Decoder 1..7 byte arbitrary chunking") {
    let alloc = std.testing.allocator;
    let payload = "The quick brown fox jumps over 13 lazy dogs! +/=?";

    let expected_enc = encodeAlloc(alloc, payload, .Standard)?;
    defer alloc.free(expected_enc);

    let chunk_size: usize = 1;
    while (chunk_size <= 7) {
        let enc = StreamingBase64Encoder.init(.Standard);
        let enc_list = std.ArrayList(u8).empty;
        defer enc_list.deinit(alloc);

        let pos: usize = 0;
        while (pos < payload.len) {
            let end = if (pos + chunk_size < payload.len) { pos + chunk_size } else { payload.len };
            enc.updateToList(alloc, &enc_list, payload[pos..end])?;
            pos = end;
        }
        enc.finishToList(alloc, &enc_list)?;
        expect_eq(std.mem.eql(u8, enc_list.items, expected_enc), true);

        let decoder = StreamingBase64Decoder.init(.Standard);
        let dec_list = std.ArrayList(u8).empty;
        defer dec_list.deinit(alloc);

        let dpos: usize = 0;
        while (dpos < enc_list.items.len) {
            let dend = if (dpos + chunk_size < enc_list.items.len) { dpos + chunk_size } else { enc_list.items.len };
            decoder.updateToList(alloc, &dec_list, enc_list.items[dpos..dend])?;
            dpos = dend;
        }
        decoder.finishToList(alloc, &dec_list)?;
        expect_eq(std.mem.eql(u8, dec_list.items, payload), true);

        chunk_size += 1;
    }
}

test("cap.base64 dynamic namespace and zero-leak std.testing.allocator") {
    let alloc = std.testing.allocator;
    let payload = "CP Base64URL zero-leak verification payload 2026!";

    let encoded = cap.base64.encodeAlloc(alloc, payload, .UrlSafeNoPad)?;
    defer alloc.free(encoded);

    let decoded = cap.base64.decodeAlloc(alloc, encoded, .UrlSafeNoPad)?;
    defer alloc.free(decoded);

    expect_eq(std.mem.eql(u8, decoded, payload), true);

    // Streaming ignore-whitespace via cap.base64
    let decoder = cap.base64.StreamingBase64Decoder.initIgnoreWhitespace(.Standard);
    let out = std.ArrayList(u8).empty;
    defer out.deinit(alloc);
    decoder.updateToList(alloc, &out, "Zm9v\r\n")?;
    decoder.updateToList(alloc, &out, " YmFy")?;
    decoder.finishToList(alloc, &out)?;
    expect_eq(std.mem.eql(u8, out.items, "foobar"), true);
}
