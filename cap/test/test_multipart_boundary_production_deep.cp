// Source: cp/cap/test/test_multipart_boundary_production_deep.cp (Production deep tests for cap:multipart_boundary)

import { test, expect_eq } from "cap:test";
import {
    MultipartPart,
    MultipartParser,
    isValidBoundary,
} from "cap:mime";

test("multipart_boundary: multi-field and binary upload with preamble and epilogue") {
    const body =
        "This is a preamble ignored by RFC 7578\r\n" ++
        "--capBoundary123\r\n" ++
        "Content-Disposition: form-data; name=\"username\"\r\n" ++
        "\r\n" ++
        "alice\r\n" ++
        "--capBoundary123\r\n" ++
        "Content-Disposition: form-data; name=\"file\"; filename=\"bin.dat\"\r\n" ++
        "Content-Type: application/octet-stream\r\n" ++
        "\r\n" ++
        "\x89PNG\r\n\x00\x01\xFF\nEND\r\n" ++
        "--capBoundary123--\r\n" ++
        "Trailing epilogue text";

    let parser = MultipartParser.init(body, "capBoundary123");
    expect_eq(parser.isFinished(), false);

    const p1 = parser.next().?;
    expect_eq(std.mem.eql(u8, p1.getHeader("content-disposition").?, "form-data; name=\"username\""), true);
    expect_eq(p1.getHeader("content-type") == null, true);
    expect_eq(std.mem.eql(u8, p1.body, "alice"), true);

    const p2 = parser.next().?;
    expect_eq(std.mem.eql(u8, p2.getHeader("CONTENT-TYPE").?, "application/octet-stream"), true);
    expect_eq(std.mem.eql(u8, p2.body, "\x89PNG\r\n\x00\x01\xFF\nEND"), true);

    expect_eq(parser.next() == null, true);
    expect_eq(parser.isFinished(), true);
}

test("multipart_boundary: mixed LF and CRLF line endings and internal newlines preserved") {
    const body =
        "--b1\n" ++
        "Content-Disposition: form-data; name=\"note\"\n" ++
        "\n" ++
        "line1\nline2\r\nline3\n" ++
        "\n" ++
        "--b1--\n";

    let parser = MultipartParser.init(body, "b1");
    const p1 = parser.next().?;
    // 紧邻 --b1-- 前的那一个 '\n' 被剥离，body 内末尾的 '\n' 完整保留
    expect_eq(std.mem.eql(u8, p1.body, "line1\nline2\r\nline3\n"), true);
    expect_eq(parser.next() == null, true);
    expect_eq(parser.isFinished(), true);
}

test("multipart_boundary: empty body parts both with and without extra CRLF") {
    const body =
        "--emptyB\r\n" ++
        "Content-Disposition: form-data; name=\"a\"\r\n" ++
        "\r\n" ++
        "\r\n" ++
        "--emptyB\r\n" ++
        "Content-Disposition: form-data; name=\"b\"\r\n" ++
        "\r\n" ++
        "--emptyB--";

    let parser = MultipartParser.init(body, "emptyB");

    const p1 = parser.next().?;
    expect_eq(p1.body.len, 0);
    expect_eq(std.mem.eql(u8, p1.getHeader("Content-Disposition").?, "form-data; name=\"a\""), true);

    const p2 = parser.next().?;
    expect_eq(p2.body.len, 0);
    expect_eq(std.mem.eql(u8, p2.getHeader("Content-Disposition").?, "form-data; name=\"b\""), true);

    expect_eq(parser.next() == null, true);
    expect_eq(parser.isFinished(), true);
}

test("multipart_boundary: isValidBoundary RFC 2046 validation") {
    expect_eq(isValidBoundary("----WebKitFormBoundary7MA4YWxkTrZu0gW"), true);
    expect_eq(isValidBoundary("a b"), true); // 中间空格允许
    expect_eq(isValidBoundary("a+b_c-d.e/f:g=h?i"), true);

    // 长度为 0、超过 70、尾随空格、非法字符均拒绝
    expect_eq(isValidBoundary(""), false);
    expect_eq(isValidBoundary("trailing "), false);
    expect_eq(isValidBoundary("bad;char"), false);
    expect_eq(isValidBoundary("bad\"quote"), false);
    expect_eq(
        isValidBoundary("12345678901234567890123456789012345678901234567890123456789012345678901"),
        false
    );
}

test("multipart_boundary: truncated body, missing closing boundary and invalid boundary guards") {
    // 1. 缺失终止边界 --bnd-- -> 解析终止且 isFinished() 为 false
    const trunc =
        "--bnd\r\n" ++
        "Content-Disposition: form-data; name=\"x\"\r\n" ++
        "\r\n" ++
        "partial data without end";
    let p_trunc = MultipartParser.init(trunc, "bnd");
    expect_eq(p_trunc.next() == null, true);
    expect_eq(p_trunc.isFinished(), false);

    // 2. 首段完整但第二段缺失终止边界
    const half =
        "--bnd\r\n" ++
        "Content-Disposition: form-data; name=\"ok\"\r\n" ++
        "\r\n" ++
        "good\r\n" ++
        "--bnd\r\n" ++
        "Content-Disposition: form-data; name=\"cut\"\r\n" ++
        "\r\n" ++
        "cut_off";
    let p_half = MultipartParser.init(half, "bnd");
    const first = p_half.next().?;
    expect_eq(std.mem.eql(u8, first.body, "good"), true);
    expect_eq(p_half.next() == null, true);
    expect_eq(p_half.isFinished(), false);

    // 3. 非法 boundary 拒绝解析
    let p_bad = MultipartParser.init("--bad \r\n\r\n--bad --", "bad ");
    expect_eq(p_bad.next() == null, true);
    expect_eq(p_bad.isFinished(), false);
}
