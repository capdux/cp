// Source: cp/cap/test/test_multipart_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:multipart & cap.multipart

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MultipartError,
    MultipartPart,
    MultipartDocument,
    MultipartBuilder,
    extractBoundary,
    parse,
    parseWithLimits,
} from "cap:web";

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Build text field + binary file containing embedded \x00 and \r\n + escaped quotes in filename
    const boundary = "----CPTestBoundary987654321";
    let builder = MultipartBuilder.init(alloc, boundary)?;
    defer builder.deinit();

    builder.addField("title", "hello world")?;
    const bin_payload = "\x89PNG\r\n\x1a\n\x00\x00\x00\r\nIHDR\x00\xff";
    builder.addFile("upload", "my \"photo\".png", "image/png", bin_payload)?;

    const ct_header = builder.contentTypeHeader(alloc)?;
    defer alloc.free(ct_header);
    expect_eq(std.mem.eql(u8, ct_header, "multipart/form-data; boundary=----CPTestBoundary987654321"), true);

    const body = builder.finish(alloc)?;
    defer alloc.free(body);

    // 2. Round-trip extractBoundary + parse
    const extracted = extractBoundary(ct_header)?;
    expect_eq(std.mem.eql(u8, extracted, boundary), true);

    let doc = parse(alloc, extracted, body)?;
    defer doc.deinit(alloc);

    expect_eq(doc.parts.len, 2 as usize);

    const p0 = doc.findField("title") orelse return error.MissingPart;
    expect_eq(std.mem.eql(u8, p0.name, "title"), true);
    expect_eq(p0.filename == null, true);
    expect_eq(std.mem.eql(u8, p0.content_type, "text/plain"), true);
    expect_eq(std.mem.eql(u8, p0.data, "hello world"), true);

    const p1 = doc.findField("upload") orelse return error.MissingPart;
    expect_eq(std.mem.eql(u8, p1.name, "upload"), true);
    const fname = p1.filename orelse return error.MissingFilename;
    expect_eq(std.mem.eql(u8, fname, "my \"photo\".png"), true);
    expect_eq(std.mem.eql(u8, p1.content_type, "image/png"), true);
    expect_eq(std.mem.eql(u8, p1.data, bin_payload), true);
}

fn runTest2(): !void {
    // 1. Valid extractBoundary cases: quoted, unquoted, mixed case, extra semicolon params
    const b1 = extractBoundary("Multipart/Form-Data; charset=utf-8; BOUNDARY=\"----WebKit777\"; q=1")?;
    expect_eq(std.mem.eql(u8, b1, "----WebKit777"), true);

    const b2 = extractBoundary("  multipart/form-data ; boundary=simple_bnd-1.2:3  ")?;
    expect_eq(std.mem.eql(u8, b2, "simple_bnd-1.2:3"), true);

    const max70 = "a123456789b123456789c123456789d123456789e123456789f123456789g123456789";
    const b3 = extractBoundary("multipart/form-data; boundary=a123456789b123456789c123456789d123456789e123456789f123456789g123456789")?;
    expect_eq(std.mem.eql(u8, b3, max70), true);

    // 2. Invalid Content-Type rejections
    let saw_invalid_ct = false;
    * = extractBoundary("application/json; charset=utf-8") catch |e| {
        if (e == MultipartError.InvalidContentType) saw_invalid_ct = true;
    };
    expect_eq(saw_invalid_ct, true);

    // 3. Invalid Boundary rejections: empty, >70 chars, unclosed quote, illegal char
    let saw_empty_bnd = false;
    * = extractBoundary("multipart/form-data; boundary=\"\"") catch |e| {
        if (e == MultipartError.InvalidBoundary) saw_empty_bnd = true;
    };
    expect_eq(saw_empty_bnd, true);

    let saw_long_bnd = false;
    * = extractBoundary("multipart/form-data; boundary=a123456789b123456789c123456789d123456789e123456789f123456789g123456789X") catch |e| {
        if (e == MultipartError.InvalidBoundary) saw_long_bnd = true;
    };
    expect_eq(saw_long_bnd, true);

    let saw_unclosed = false;
    * = extractBoundary("multipart/form-data; boundary=\"abc") catch |e| {
        if (e == MultipartError.InvalidBoundary) saw_unclosed = true;
    };
    expect_eq(saw_unclosed, true);

    let saw_bad_char = false;
    * = extractBoundary("multipart/form-data; boundary=bad@bnd") catch |e| {
        if (e == MultipartError.InvalidBoundary) saw_bad_char = true;
    };
    expect_eq(saw_bad_char, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // Preamble + LF-only line endings + multiple same-name fields + default text/plain + epilogue
    const raw =
        "This is a preamble that must be ignored\n" ++
        "--bndLF\n" ++
        "Content-Disposition: form-data; name=\"tag\"\n" ++
        "\n" ++
        "alpha\n" ++
        "--bndLF\n" ++
        "Content-Disposition: form-data; name=\"other\"\n" ++
        "Content-Type: application/json\n" ++
        "\n" ++
        "{\"ok\":true}\n" ++
        "--bndLF\n" ++
        "Content-Disposition: form-data; name=\"tag\"\n" ++
        "\n" ++
        "beta\n" ++
        "--bndLF--\n" ++
        "Trailing epilogue ignored\n";

    let doc = parse(alloc, "bndLF", raw)?;
    defer doc.deinit(alloc);

    expect_eq(doc.parts.len, 3 as usize);
    expect_eq(doc.findField("missing") == null, true);

    const tags = doc.findAll(alloc, "tag")?;
    defer alloc.free(tags);
    expect_eq(tags.len, 2 as usize);
    expect_eq(std.mem.eql(u8, tags[0].data, "alpha"), true);
    expect_eq(std.mem.eql(u8, tags[0].content_type, "text/plain"), true);
    expect_eq(std.mem.eql(u8, tags[1].data, "beta"), true);
    expect_eq(std.mem.eql(u8, tags[1].content_type, "text/plain"), true);

    const other = doc.findField("other") orelse return error.MissingOther;
    expect_eq(std.mem.eql(u8, other.content_type, "application/json"), true);
    expect_eq(std.mem.eql(u8, other.data, "{\"ok\":true}"), true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. MissingEndBoundary
    const no_end =
        "--b1\r\n" ++
        "Content-Disposition: form-data; name=\"k\"\r\n" ++
        "\r\n" ++
        "val\r\n" ++
        "--b1\r\n";
    let saw_missing_end = false;
    * = parse(alloc, "b1", no_end) catch |e| {
        if (e == MultipartError.MissingEndBoundary) saw_missing_end = true;
    };
    expect_eq(saw_missing_end, true);

    // 2. TooManyParts
    const two_parts =
        "--b1\r\nContent-Disposition: form-data; name=\"a\"\r\n\r\n1\r\n" ++
        "--b1\r\nContent-Disposition: form-data; name=\"b\"\r\n\r\n2\r\n" ++
        "--b1--\r\n";
    let saw_too_many = false;
    * = parseWithLimits(alloc, "b1", two_parts, 1, 1024) catch |e| {
        if (e == MultipartError.TooManyParts) saw_too_many = true;
    };
    expect_eq(saw_too_many, true);

    // 3. PartTooLarge
    let saw_too_large = false;
    * = parseWithLimits(alloc, "b1", two_parts, 10, 0) catch |e| {
        if (e == MultipartError.PartTooLarge) saw_too_large = true;
    };
    expect_eq(saw_too_large, true);

    // 4. MalformedPartHeader (missing Content-Disposition)
    const bad_hdr =
        "--b1\r\nContent-Type: text/plain\r\n\r\nhello\r\n--b1--\r\n";
    let saw_bad_hdr = false;
    * = parse(alloc, "b1", bad_hdr) catch |e| {
        if (e == MultipartError.MalformedPartHeader) saw_bad_hdr = true;
    };
    expect_eq(saw_bad_hdr, true);

    // 5. CR/LF header injection interception in MultipartBuilder
    let b = MultipartBuilder.init(alloc, "b1")?;
    defer b.deinit();

    let saw_inj_field = false;
    b.addField("bad\r\nX-Inject: 1", "v") catch |e| {
        if (e == MultipartError.InvalidHeaderValue) saw_inj_field = true;
    };
    expect_eq(saw_inj_field, true);

    let saw_inj_file = false;
    b.addFile("ok", "evil\nname.txt", "text/plain", "abc") catch |e| {
        if (e == MultipartError.InvalidHeaderValue) saw_inj_file = true;
    };
    expect_eq(saw_inj_file, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mounting via cap.multipart & std.testing.allocator zero-leak check
    let builder = cap.web.MultipartBuilder.init(alloc, "CapHubBnd42")?;
    defer builder.deinit();

    builder.addField("mode", "production")?;
    builder.addFile("doc", "notes.txt", "text/plain", "line1\r\nline2")?;

    const ct = builder.contentTypeHeader(alloc)?;
    defer alloc.free(ct);
    const body = builder.finish(alloc)?;
    defer alloc.free(body);

    const bnd = cap.web.extractBoundary(ct)?;
    let doc = cap.web.parse(alloc, bnd, body)?;
    defer doc.deinit(alloc);

    expect_eq(doc.parts.len, 2 as usize);
    const mode = doc.findField("mode") orelse return error.MissingMode;
    expect_eq(std.mem.eql(u8, mode.data, "production"), true);
}

test("cap:multipart builder and binary round-trip parse") {
    runTest1() catch unreachable;
}

test("cap:multipart extractBoundary quoted, unquoted, case-insensitive and rejections") {
    runTest2() catch unreachable;
}

test("cap:multipart findAll, preamble/epilogue, LF line endings and default text/plain") {
    runTest3() catch unreachable;
}

test("cap:multipart MissingEndBoundary, TooManyParts, PartTooLarge, MalformedPartHeader and CRLF injection") {
    runTest4() catch unreachable;
}

test("cap.multipart dynamic hub mounting and zero-leak verification") {
    runTest5() catch unreachable;
}
