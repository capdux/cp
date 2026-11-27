// cap/test/testCapDataUri.cp
// Comprehensive unit tests for @cap/data-uri

import { test, expect_eq } from "cap:test";
import {
    DataUri,
    parseDataUri,
    decodeDataUri,
    encodeDataUriBase64,
    encodeDataUriText,
    isDataUri,
} from "../../packages/data-uri-to-buffer/src/index.cp";

fn main() {
    test("1. @cap/data-uri — Minimal data:,Hello defaults & isDataUri", () => {
        const parsed = parseDataUri("data:,Hello");
        expect_eq(parsed.ok, true);
        expect_eq(parsed.media_type, "text/plain");
        expect_eq(parsed.charset, "US-ASCII");
        expect_eq(parsed.is_base64, false);
        expect_eq(parsed.raw_payload, "Hello");
        expect_eq(isDataUri("data:,Hello"), true);

        let out: [64]u8 = undefined;
        const bytes = decodeDataUri("data:,Hello", out[0..64]);
        expect_eq(bytes != null, true);
        expect_eq(bytes.? as string, "Hello");
    });

    test("2. @cap/data-uri — Base64 text & binary payload decoding", () => {
        const uri = "DATA:text/plain;charset=utf-8;BASE64,SGVsbG8sIENQIQ==";
        const parsed = parseDataUri(uri);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.media_type, "text/plain");
        expect_eq(parsed.charset, "utf-8");
        expect_eq(parsed.is_base64, true);

        let out: [128]u8 = undefined;
        const decoded = decodeDataUri(uri, out[0..128]);
        expect_eq(decoded != null, true);
        expect_eq(decoded.? as string, "Hello, CP!");

        // Binary PNG magic header: 89 50 4E 47 0D 0A 1A 0A -> iVBORw0KGgo=
        const png_uri = "data:image/png;base64,iVBORw0KGgo=";
        const png_info = parseDataUri(png_uri);
        expect_eq(png_info.media_type, "image/png");
        const png_bytes = decodeDataUri(png_uri, out[0..128]);
        expect_eq(png_bytes != null, true);
        expect_eq(png_bytes.?.len, 8 as usize);
        expect_eq(png_bytes.?[0], 0x89 as u8);
        expect_eq(png_bytes.?[1], 0x50 as u8);
        expect_eq(png_bytes.?[2], 0x4E as u8);
        expect_eq(png_bytes.?[3], 0x47 as u8);
    });

    test("3. @cap/data-uri — Percent-encoded UTF-8 decoding & custom charset", () => {
        // "你好 CP!" UTF-8 percent-encoded
        const uri = "data:text/plain;charset=\"utf-8\",%E4%BD%A0%E5%A5%BD%20CP%21";
        const parsed = parseDataUri(uri);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.media_type, "text/plain");
        expect_eq(parsed.charset, "utf-8");
        expect_eq(parsed.is_base64, false);

        let out: [128]u8 = undefined;
        const decoded = decodeDataUri(uri, out[0..128]);
        expect_eq(decoded != null, true);
        expect_eq(decoded.? as string, "你好 CP!");
    });

    test("4. @cap/data-uri — Base64 encode/decode round-trip (binary & text)", () => {
        let bin: [6]u8 = [0x00, 0xFF, 0x10, 0xAB, 0xCD, 0x42];
        let uri_buf: [256]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const uri = encodeDataUriBase64("application/octet-stream", bin[0..6], uri_buf[0..256]);
        expect_eq(uri, "data:application/octet-stream;base64,AP8Qq81C");

        const roundtrip = decodeDataUri(uri, dec_buf[0..64]);
        expect_eq(roundtrip != null, true);
        expect_eq(roundtrip.?.len, 6 as usize);
        expect_eq(roundtrip.?[0], 0x00 as u8);
        expect_eq(roundtrip.?[1], 0xFF as u8);
        expect_eq(roundtrip.?[4], 0xCD as u8);
        expect_eq(roundtrip.?[5], 0x42 as u8);
    });

    test("5. @cap/data-uri — encodeDataUriText percent-encoding & round-trip", () => {
        let uri_buf: [256]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        const uri = encodeDataUriText("text/html;charset=utf-8", "<h1>Hi!</h1>", uri_buf[0..256]);
        expect_eq(uri, "data:text/html;charset=utf-8,%3Ch1%3EHi%21%3C%2Fh1%3E");

        const decoded = decodeDataUri(uri, dec_buf[0..128]);
        expect_eq(decoded != null, true);
        expect_eq(decoded.? as string, "<h1>Hi!</h1>");
    });

    test("6. @cap/data-uri — Invalid Data URIs & malformed payloads rejected", () => {
        let out: [64]u8 = undefined;

        expect_eq(isDataUri("https://example.com"), false);
        expect_eq(isDataUri("data:text/plain"), false);
        expect_eq(isDataUri("data:invalidmediatype,hello"), false);
        expect_eq(isDataUri(""), false);

        // Invalid Base64 characters / truncated padding
        expect_eq(decodeDataUri("data:;base64,!!!invalid!!!", out[0..64]) == null, true);
        expect_eq(decodeDataUri("data:;base64,A", out[0..64]) == null, true);

        // Invalid percent-encoding
        expect_eq(decodeDataUri("data:,bad%2", out[0..64]) == null, true);
        expect_eq(decodeDataUri("data:,bad%ZZ", out[0..64]) == null, true);
    });
}
