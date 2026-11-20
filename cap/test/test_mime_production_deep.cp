// Source: cp/cap/test/test_mime_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:mime & cap.mime

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MimeError,
    MimeParam,
    MediaType,
    ContentNegotiator,
    MimeDb,
} from "cap:mime";

fn strEql(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn testMediaTypeParseAndMatches(): ! {
    const alloc = std.heap.page_allocator;

    // Case normalization, quoted charset with escaped chars, and multi-parameter parsing
    let mt = MediaType.parse(alloc, "Text/HTML; charset=\"utf-8\"; boundary=abc; note=\"a\\\"b\"")?;
    defer mt.deinit();

    expect_eq(strEql(mt.type, "text"), true);
    expect_eq(strEql(mt.subtype, "html"), true);
    expect_eq(strEql(mt.suffix, ""), true);
    expect_eq(strEql(mt.essence(), "text/html"), true);
    expect_eq(strEql(mt.charset().?, "utf-8"), true);
    expect_eq(strEql(mt.getParam("BOUNDARY").?, "abc"), true);
    expect_eq(strEql(mt.getParam("note").?, "a\"b"), true);
    expect_eq(mt.getParam("missing") == null, true);

    expect_eq(mt.matches("*/*"), true);
    expect_eq(mt.matches("TEXT/*"), true);
    expect_eq(mt.matches("text/html"), true);
    expect_eq(mt.matches("application/*"), false);
    expect_eq(mt.matches("+json"), false);

    // Structured syntax suffix (+json) extraction & matching
    let api = MediaType.parse(alloc, "Application/VND.Api+JSON; charset=utf-8")?;
    defer api.deinit();

    expect_eq(strEql(api.type, "application"), true);
    expect_eq(strEql(api.subtype, "vnd.api+json"), true);
    expect_eq(strEql(api.suffix, "+json"), true);
    expect_eq(strEql(api.essence(), "application/vnd.api+json"), true);
    expect_eq(api.matches("+json"), true);
    expect_eq(api.matches("*+json"), true);
    expect_eq(api.matches("application/*+json"), true);
    expect_eq(api.matches("text/*+json"), false);

    // Malformed media types & parameters must be rejected cleanly
    const bad_inputs: []const []const u8 = [
        "",
        "   ",
        "text",
        "/html",
        "text/",
        "text / html",
        "text/html/extra",
        "text/foo+",
        "text/+json",
        "text/html; charset",
        "text/html; =utf-8",
        "text/html; charset=",
        "text/html; charset=\"unterminated",
    ];
    for (bad of bad_inputs) {
        let rejected = false;
        * = MediaType.parse(alloc, bad) catch {
            rejected = true;
        };
        expect_eq(rejected, true);
    }
}

fn testAcceptContentNegotiation(): ! {
    const alloc = std.heap.page_allocator;
    const avail: []const []const u8 = ["application/json", "text/plain", "text/html"];

    // q-weight selection
    const best1 = ContentNegotiator.negotiateAccept(
        alloc,
        "text/html;q=0.7, application/json;q=0.9, */*;q=0.1",
        avail,
    )?;
    expect_eq(strEql(best1.?, "application/json"), true);

    // Specificity tie-breaker when q-weights are equal: type/subtype > type/* > */*
    const best2 = ContentNegotiator.negotiateAccept(
        alloc,
        "*/*;q=0.8, text/*;q=0.8, text/plain;q=0.8",
        avail,
    )?;
    expect_eq(strEql(best2.?, "text/plain"), true);

    // Explicit q=0 exclusion overrides broader wildcard match
    const text_avail: []const []const u8 = ["text/plain", "text/html"];
    const best3 = ContentNegotiator.negotiateAccept(
        alloc,
        "text/*;q=0.9, text/plain;q=0",
        text_avail,
    )?;
    expect_eq(strEql(best3.?, "text/html"), true);

    // No match returns null
    const none1 = ContentNegotiator.negotiateAccept(
        alloc,
        "image/png, video/mp4",
        avail,
    ) catch unreachable;
    expect_eq(none1 == null, true);

    // All rejected via */*;q=0 returns null
    const none2 = ContentNegotiator.negotiateAccept(
        alloc,
        "*/*;q=0",
        avail,
    ) catch unreachable;
    expect_eq(none2 == null, true);

    // Invalid quality value returns InvalidQuality
    let got_bad_q = false;
    * = ContentNegotiator.negotiateAccept(alloc, "text/html;q=1.5", avail) catch e => {
        if (strEql(@errorName(e), "InvalidQuality")) got_bad_q = true;
    };
    expect_eq(got_bad_q, true);
}

fn testMimeDatabaseAndClassification(): ! {
    // Path & extension lookup (case-insensitive, strips dir & dot, compound ext)
    expect_eq(strEql(MimeDb.fromExtension("dir/app.tar.gz").?, "application/gzip"), true);
    expect_eq(strEql(MimeDb.fromExtension(".WASM").?, "application/wasm"), true);
    expect_eq(strEql(MimeDb.fromExtension("json").?, "application/json"), true);
    expect_eq(strEql(MimeDb.fromExtension("/assets/fonts/inter.woff2").?, "font/woff2"), true);
    expect_eq(strEql(MimeDb.fromExtension("models/weights.safetensors").?, "application/x-safetensors"), true);
    expect_eq(MimeDb.fromExtension("dir/Makefile") == null, true);
    expect_eq(MimeDb.fromExtension("file.unknown_ext_xyz") == null, true);

    // Reverse lookup (strips parameters)
    expect_eq(strEql(MimeDb.toExtension("text/html; charset=utf-8").?, "html"), true);
    expect_eq(strEql(MimeDb.toExtension("Application/JSON").?, "json"), true);
    expect_eq(strEql(MimeDb.toExtension("application/wasm").?, "wasm"), true);
    expect_eq(MimeDb.toExtension("application/x-nonexistent") == null, true);

    // isText classification
    expect_eq(MimeDb.isText("text/plain; charset=utf-8"), true);
    expect_eq(MimeDb.isText("application/vnd.api+json"), true);
    expect_eq(MimeDb.isText("image/svg+xml"), true);
    expect_eq(MimeDb.isText("application/javascript"), true);
    expect_eq(MimeDb.isText("application/toml"), true);
    expect_eq(MimeDb.isText("image/png"), false);
    expect_eq(MimeDb.isText("application/wasm"), false);

    // isCompressible classification
    expect_eq(MimeDb.isCompressible("text/css; charset=utf-8"), true);
    expect_eq(MimeDb.isCompressible("application/json"), true);
    expect_eq(MimeDb.isCompressible("application/wasm"), true);
    expect_eq(MimeDb.isCompressible("font/ttf"), true);
    expect_eq(MimeDb.isCompressible("image/png"), false);
    expect_eq(MimeDb.isCompressible("video/mp4"), false);
    expect_eq(MimeDb.isCompressible("application/zip"), false);
    expect_eq(MimeDb.isCompressible("font/woff2"), false);
}

fn testMediaTypeToStringRoundTrip(): ! {
    const alloc = std.heap.page_allocator;

    let mt1 = MediaType.parse(alloc, "Multipart/Form-Data; boundary=\"----WebKit Boundary\"; q=0.8")?;
    defer mt1.deinit();

    const formatted = mt1.toString(alloc)?;
    defer alloc.free(formatted);
    expect_eq(strEql(formatted, "multipart/form-data; boundary=\"----WebKit Boundary\"; q=0.8"), true);

    // Round-trip re-parse equivalence
    let mt2 = MediaType.parse(alloc, formatted)?;
    defer mt2.deinit();

    expect_eq(strEql(mt1.essence(), mt2.essence()), true);
    expect_eq(strEql(mt1.getParam("boundary").?, mt2.getParam("boundary").?), true);
    expect_eq(strEql(mt1.getParam("q").?, mt2.getParam("q").?), true);
}

fn testCapMimeDynamicMount(): ! {
    const alloc = std.heap.page_allocator;

    let mt = cap.mime.MediaType.parse(alloc, "application/problem+json; charset=utf-8")?;
    defer mt.deinit();
    expect_eq(strEql(mt.suffix, "+json"), true);
    expect_eq(mt.matches("+json"), true);

    const mime = cap.mime.MimeDb.fromExtension("index.html");
    expect_eq(strEql(mime.?, "text/html"), true);

    const avail: []const []const u8 = ["text/plain", "application/json"];
    const chosen = cap.mime.ContentNegotiator.negotiateAccept(alloc, "application/json, text/plain;q=0.5", avail)?;
    expect_eq(strEql(chosen.?, "application/json"), true);
}

fn main() {
    test("1. MediaType.parse Normalization, Quoted Params, +suffix, matches() & Malformed Rejection", () => {
        testMediaTypeParseAndMatches()?;
    });
    test("2. ContentNegotiator.negotiateAccept RFC 9110 q-Weights, q=0 Exclusion & Specificity", () => {
        testAcceptContentNegotiation()?;
    });
    test("3. MimeDb Extension/Path Lookup, Reverse Lookup, isText & isCompressible", () => {
        testMimeDatabaseAndClassification()?;
    });
    test("4. MediaType.toString Canonical Formatting & Round-Trip Equivalence", () => {
        testMediaTypeToStringRoundTrip()?;
    });
    test("5. import cap from \"cap\"; cap.mime Dynamic Lazy Mounting", () => {
        testCapMimeDynamicMount()?;
    });
}
