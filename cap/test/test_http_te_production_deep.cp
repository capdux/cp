// Source: cp/cap/test/test_http_te_production_deep.cp (Production deep tests for cap:http_te)

import { test, expect_eq } from "cap:test";
import {
    TransferCoding,
    TransferEncodingIter,
    TransferEncodingValidation,
    validateTransferEncoding,
    isChunkedTransfer,
    hasConflictingClAndTe,
    teAcceptsTrailers,
} from "cap:http";

test("http_te: TransferCoding parsing and TransferEncodingIter comma/OWS handling") {
    expect_eq(TransferCoding.parse("chunked") == .chunked, true);
    expect_eq(TransferCoding.parse("GZIP") == .gzip, true);
    expect_eq(TransferCoding.parse("x-gzip") == .gzip, true);
    expect_eq(TransferCoding.parse("deflate") == .deflate, true);
    expect_eq(TransferCoding.parse("compress") == .compress, true);
    expect_eq(TransferCoding.parse("x-compress") == .compress, true);
    expect_eq(TransferCoding.parse("br") == .br, true);
    expect_eq(TransferCoding.parse("zstd") == .zstd, true);
    expect_eq(TransferCoding.parse("lz4") == .unknown, true);
    expect_eq(std.mem.eql(u8, TransferCoding.zstd.asStr(), "zstd"), true);

    let it = TransferEncodingIter.init(" , \t gzip , , chunked , ");
    expect_eq(std.mem.eql(u8, it.next().?, "gzip"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "chunked"), true);
    expect_eq(it.next() == null, true);
}

test("http_te: validateTransferEncoding RFC 9112 §6.1/§6.3 enforcement") {
    expect_eq(validateTransferEncoding("chunked", true) == .valid, true);
    expect_eq(validateTransferEncoding("gzip, chunked", true) == .valid, true);
    expect_eq(validateTransferEncoding("br, zstd, CHUNKED", true) == .valid, true);

    // 响应可不含 chunked（当 require_chunked_final=false）
    expect_eq(validateTransferEncoding("gzip", false) == .valid, true);
    expect_eq(validateTransferEncoding("gzip", true) == .chunked_not_final, true);

    // 异常报文校验：空值、重复 chunked、chunked 后跟随编码、未知编码
    expect_eq(validateTransferEncoding("", false) == .empty, true);
    expect_eq(validateTransferEncoding(" , , ", true) == .empty, true);
    expect_eq(validateTransferEncoding("chunked, chunked", true) == .duplicate_chunked, true);
    expect_eq(validateTransferEncoding("gzip, chunked, chunked", true) == .duplicate_chunked, true);
    expect_eq(validateTransferEncoding("chunked, gzip", false) == .chunked_not_final, true);
    expect_eq(validateTransferEncoding("bogus, chunked", true) == .unknown_coding, true);
    expect_eq(std.mem.eql(u8, TransferEncodingValidation.duplicate_chunked.asStr(), "duplicate_chunked"), true);
}

test("http_te: isChunkedTransfer detects unique final chunked case-insensitively") {
    expect_eq(isChunkedTransfer("chunked"), true);
    expect_eq(isChunkedTransfer("gzip, CHUNKED"), true);
    expect_eq(isChunkedTransfer("chunked, gzip"), false);
    expect_eq(isChunkedTransfer("chunked, chunked"), false);
    expect_eq(isChunkedTransfer("gzip"), false);
    expect_eq(isChunkedTransfer(""), false);
}

test("http_te: hasConflictingClAndTe RFC 9112 §6.3 dual framing guard") {
    expect_eq(hasConflictingClAndTe(true, "chunked"), true);
    expect_eq(hasConflictingClAndTe(true, ""), true);
    expect_eq(hasConflictingClAndTe(true, null), false);
    expect_eq(hasConflictingClAndTe(false, "chunked"), false);
    expect_eq(hasConflictingClAndTe(false, null), false);
}

test("http_te: teAcceptsTrailers with q-values and q=0 rejection") {
    expect_eq(teAcceptsTrailers("trailers"), true);
    expect_eq(teAcceptsTrailers("gzip;q=0.5, TRAILERS"), true);
    expect_eq(teAcceptsTrailers("trailers; q=0.8"), true);
    expect_eq(teAcceptsTrailers("trailers; q=1.0"), true);

    // q=0 / q=0.0 / q=0.000 表示明确拒绝
    expect_eq(teAcceptsTrailers("trailers;q=0"), false);
    expect_eq(teAcceptsTrailers("trailers; q=0.000"), false);
    expect_eq(teAcceptsTrailers("gzip, deflate"), false);
    expect_eq(teAcceptsTrailers(""), false);
}
