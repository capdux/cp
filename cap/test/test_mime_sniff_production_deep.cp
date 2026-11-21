// Source: cp/cap/test/test_mime_sniff_production_deep.cp (Production deep tests for cap:mime_sniff)

import { test, expect_eq } from "cap:test";
import {
    isBinaryDataByte,
    isTextData,
    sniffImageMime,
    sniffArchiveOrExecMime,
    sniffAudioVideoMime,
    sniffMime,
} from "cap:mime_sniff";

test("mime_sniff: WHATWG binary byte classification and UTF-8/UTF-16 text detection") {
    expect_eq(isBinaryDataByte(0x00), true);
    expect_eq(isBinaryDataByte(0x08), true);
    expect_eq(isBinaryDataByte(0x0B), true);
    expect_eq(isBinaryDataByte(0x1F), true);
    expect_eq(isBinaryDataByte(0x7F), true);

    // TAB/LF/FF/CR/ESC 与可打印 ASCII 非二进制字节
    expect_eq(isBinaryDataByte(0x09), false);
    expect_eq(isBinaryDataByte(0x0A), false);
    expect_eq(isBinaryDataByte(0x0C), false);
    expect_eq(isBinaryDataByte(0x0D), false);
    expect_eq(isBinaryDataByte(0x1B), false);
    expect_eq(isBinaryDataByte('A'), false);

    expect_eq(isTextData(""), true);
    expect_eq(isTextData("hello\t世界\r\n"), true);
    expect_eq(isTextData("\xEF\xBB\xBFUTF-8 with BOM"), true);
    expect_eq(isTextData("\xFF\xFEh\x00i\x00"), true); // UTF-16LE BOM
    expect_eq(isTextData("\xFE\xFF\x00h\x00i"), true); // UTF-16BE BOM

    // 含 NUL 字节或非法 UTF-8 序列判定为非文本
    expect_eq(isTextData("abc\x00def"), false);
    expect_eq(isTextData("\xFF\x00\x01\x02"), false);
    expect_eq(isTextData("\xC0\xAF"), false); // overlong UTF-8
}

test("mime_sniff: image magic signatures PNG/JPEG/GIF/WebP/BMP/ICO/SVG") {
    expect_eq(std.mem.eql(u8, sniffImageMime("\x89PNG\r\n\x1A\n\x00").?, "image/png"), true);
    expect_eq(std.mem.eql(u8, sniffImageMime("\xFF\xD8\xFF\xE0").?, "image/jpeg"), true);
    expect_eq(std.mem.eql(u8, sniffImageMime("GIF87a...").?, "image/gif"), true);
    expect_eq(std.mem.eql(u8, sniffImageMime("GIF89a...").?, "image/gif"), true);
    expect_eq(std.mem.eql(u8, sniffImageMime("RIFF\x24\x00\x00\x00WEBP").?, "image/webp"), true);
    expect_eq(std.mem.eql(u8, sniffImageMime("BM\x36\x00").?, "image/bmp"), true);
    expect_eq(std.mem.eql(u8, sniffImageMime("\x00\x00\x01\x00\x01\x00").?, "image/x-icon"), true);
    expect_eq(std.mem.eql(u8, sniffImageMime("<?xml version=\"1.0\"?><svg xmlns=\"...\"></svg>").?, "image/svg+xml"), true);
    expect_eq(sniffImageMime("plain text") == null, true);
}

test("mime_sniff: archive and executable magic signatures PDF/ZIP/GZIP/ZSTD/BZIP2/XZ/WASM/ELF/SQLite") {
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("%PDF-1.7\n").?, "application/pdf"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("PK\x03\x04\x14\x00").?, "application/zip"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("\x1F\x8B\x08\x00").?, "application/gzip"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("\x28\xB5\x2F\xFD\x00").?, "application/zstd"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("BZh91AY").?, "application/x-bzip2"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("\xFD7zXZ\x00\x00").?, "application/x-xz"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("\x00asm\x01\x00\x00\x00").?, "application/wasm"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("\x7FELF\x02\x01\x01").?, "application/x-elf"), true);
    expect_eq(std.mem.eql(u8, sniffArchiveOrExecMime("SQLite format 3\x00...").?, "application/vnd.sqlite3"), true);
}

test("mime_sniff: audio and video magic signatures WAV/OGG/FLAC/MP3/MP4/WebM") {
    expect_eq(std.mem.eql(u8, sniffAudioVideoMime("RIFF\x24\x00\x00\x00WAVEfmt ").?, "audio/wav"), true);
    expect_eq(std.mem.eql(u8, sniffAudioVideoMime("OggS\x00\x02").?, "application/ogg"), true);
    expect_eq(std.mem.eql(u8, sniffAudioVideoMime("fLaC\x00\x00").?, "audio/flac"), true);
    expect_eq(std.mem.eql(u8, sniffAudioVideoMime("ID3\x03\x00").?, "audio/mpeg"), true);
    expect_eq(std.mem.eql(u8, sniffAudioVideoMime("\x00\x00\x00\x18ftypisom").?, "video/mp4"), true);
    expect_eq(std.mem.eql(u8, sniffAudioVideoMime("\x1A\x45\xDF\xA3\x93").?, "video/webm"), true);
}

test("mime_sniff: end-to-end sniffMime priority across HTML/XML/JSON/text/binary") {
    expect_eq(std.mem.eql(u8, sniffMime("  <!DOCTYPE html><html></html>"), "text/html; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, sniffMime("<?xml version=\"1.0\"?><root/>"), "application/xml"), true);
    expect_eq(std.mem.eql(u8, sniffMime("  {\"hello\": \"world\"} \n"), "application/json"), true);
    expect_eq(std.mem.eql(u8, sniffMime("[1, 2, 3]"), "application/json"), true);
    expect_eq(std.mem.eql(u8, sniffMime("just plain UTF-8 text"), "text/plain; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, sniffMime("\x01\x02\x03\x04\x05\x06"), "application/octet-stream"), true);
    expect_eq(std.mem.eql(u8, sniffMime("\x00asm\x01\x00\x00\x00"), "application/wasm"), true);
}
