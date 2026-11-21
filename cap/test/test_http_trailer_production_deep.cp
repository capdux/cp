import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isForbiddenTrailerField,
    isValidTrailerFieldName,
    TrailerFieldIter,
    validateTrailerHeader,
    hasTrailerField,
} from "cap:http";

test("1. isForbiddenTrailerField blocks all 25 RFC 9110 §6.5.1 forbidden trailer fields case-insensitively", () => {
    const forbidden: [25][]const u8 = [
        "transfer-encoding",
        "Content-Length",
        "HOST",
        "Cache-Control",
        "Max-Forwards",
        "TE",
        "Authorization",
        "Set-Cookie",
        "Content-Encoding",
        "Content-Type",
        "Content-Range",
        "Trailer",
        "Connection",
        "Keep-Alive",
        "Upgrade",
        "Proxy-Authenticate",
        "Proxy-Authorization",
        "WWW-Authenticate",
        "Age",
        "Expires",
        "Date",
        "Location",
        "Retry-After",
        "Vary",
        "Warning",
    ];

    let i: usize = 0;
    while (i < 25) {
        expect_eq(isForbiddenTrailerField(forbidden[i]), true);
        i += 1;
    }

    // Allowed trailer fields must return false
    expect_eq(isForbiddenTrailerField("X-Checksum"), false);
    expect_eq(isForbiddenTrailerField("Grpc-Status"), false);
    expect_eq(isForbiddenTrailerField("Server-Timing"), false);
    expect_eq(isForbiddenTrailerField("ETag"), false);
    expect_eq(isForbiddenTrailerField(""), false);
});

test("2. isValidTrailerFieldName enforces RFC 9110 tchar token syntax and excludes forbidden fields", () => {
    // Valid custom and standard trailerable field names
    expect_eq(isValidTrailerFieldName("X-Checksum-SHA256"), true);
    expect_eq(isValidTrailerFieldName("Grpc-Message"), true);
    expect_eq(isValidTrailerFieldName("Server-Timing"), true);
    expect_eq(isValidTrailerFieldName("x_custom.field~1!#$%&'*+^`|"), true);

    // Forbidden fields are rejected even though they are valid tokens
    expect_eq(isValidTrailerFieldName("Transfer-Encoding"), false);
    expect_eq(isValidTrailerFieldName("content-length"), false);
    expect_eq(isValidTrailerFieldName("Set-Cookie"), false);
    expect_eq(isValidTrailerFieldName("Trailer"), false);

    // Malformed tokens are rejected
    expect_eq(isValidTrailerFieldName(""), false);
    expect_eq(isValidTrailerFieldName("X Bad"), false);
    expect_eq(isValidTrailerFieldName("X-Bad:Value"), false);
    expect_eq(isValidTrailerFieldName("X-Bad\r\n"), false);
    expect_eq(isValidTrailerFieldName("X-Bad,Other"), false);
    expect_eq(isValidTrailerFieldName("\"Quoted\""), false);
});

test("3. TrailerFieldIter iterates comma-separated tokens, trims whitespace, skips empty and malformed segments", () => {
    const hdr = " , X-Checksum , \t Server-Timing , , bad token , \"quoted\" , Grpc-Status , ";
    let it = TrailerFieldIter.init(hdr);

    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "X-Checksum"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "Server-Timing"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "Grpc-Status"), true);
    expect_eq(it.next() == null, true);

    // Empty and comma-only headers yield nothing
    let empty_it = TrailerFieldIter.init(" , , \t ");
    expect_eq(empty_it.next() == null, true);
});

test("4. validateTrailerHeader validates entire header and rejects empty, malformed, or forbidden entries", () => {
    // Valid headers (including extra commas and OWS per RFC 9110 §5.6.1.2)
    expect_eq(validateTrailerHeader("X-Checksum"), true);
    expect_eq(validateTrailerHeader("X-Checksum, Server-Timing, Grpc-Status"), true);
    expect_eq(validateTrailerHeader(" , X-Digest , , X-Signature \t "), true);

    // Empty / whitespace / comma-only headers must fail
    expect_eq(validateTrailerHeader(""), false);
    expect_eq(validateTrailerHeader("   \t  "), false);
    expect_eq(validateTrailerHeader(",,, ,"), false);

    // Presence of any forbidden field fails the entire header
    expect_eq(validateTrailerHeader("X-Checksum, Content-Length"), false);
    expect_eq(validateTrailerHeader("X-Checksum, transfer-encoding, Grpc-Status"), false);
    expect_eq(validateTrailerHeader("Authorization"), false);
    expect_eq(validateTrailerHeader("Set-Cookie, X-Trace"), false);

    // Presence of any malformed token fails the entire header
    expect_eq(validateTrailerHeader("X-Checksum, bad field"), false);
    expect_eq(validateTrailerHeader("X-Checksum, X-Bad:1"), false);
    expect_eq(validateTrailerHeader("X-Checksum, X-Inject\r\nEvil"), false);
});

test("5. hasTrailerField performs case-insensitive lookup and guards against invalid query names", () => {
    const hdr = "X-Checksum-SHA256, Server-Timing, Grpc-Status, Grpc-Message";

    expect_eq(hasTrailerField(hdr, "x-checksum-sha256"), true);
    expect_eq(hasTrailerField(hdr, "SERVER-TIMING"), true);
    expect_eq(hasTrailerField(hdr, "Grpc-Status"), true);
    expect_eq(hasTrailerField(hdr, "grpc-message"), true);

    // Non-existent or partial matches return false
    expect_eq(hasTrailerField(hdr, "Grpc"), false);
    expect_eq(hasTrailerField(hdr, "X-Other"), false);

    // Invalid field_name or empty header returns false
    expect_eq(hasTrailerField(hdr, ""), false);
    expect_eq(hasTrailerField(hdr, "Grpc-Status "), false);
    expect_eq(hasTrailerField(hdr, "bad name"), false);
    expect_eq(hasTrailerField("", "X-Checksum"), false);
});
