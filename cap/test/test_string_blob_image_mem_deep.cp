// Source: /data/dev/bun/src/string/ & /data/dev/bun/src/runtime/webcore/Blob.cp & /data/dev/bun/src/runtime/image/ & /data/dev/bun/src/bun_alloc/
// ============================================================================
// cap/test/test_string_blob_image_mem_deep.cp
// Deep Audit & Regression Suite for cap:string, cap:blob, cap:image, cap:mem
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    REPLACEMENT_CHAR,
    Utf8,
    Utf16,
    GraphemeIterator,
    graphemeCount,
    StringEscape,
    escapeJs,
    Strings,
    isAllAscii,
    toLowerAscii,
    toUpperAscii,
    eqlIgnoreCaseAscii,
    trim,
    trimStart,
    trimEnd,
    startsWith,
    endsWith,
    indexOf,
    lastIndexOf,
    split,
    replaceAll,
    replaceAllAlloc,
} from "cap:string";
import { Blob, File, BlobChunkReader } from "cap:blob";
import {
    Codecs,
    ImageFormat,
    ExifOrientation,
    probeDimensions,
    Png,
    Gif,
    ThumbHash,
    Image,
} from "cap:image";
import {
    isPowerOfTwo,
    alignForward,
    alignBackward,
    alignDelta,
    secureZero,
    FixedBufferAllocator,
    SlabPool,
} from "cap:mem";

fn testStringUtfAndGraphemes(): !void {
    const allocator = std.heap.page_allocator;

    // 1. UTF-8 <-> UTF-16LE with BMP & 4-byte non-BMP Emoji surrogate pairs
    const sample = "A你好🚀🎉"; // 'A'(1) + '你'(1) + '好'(1) + '🚀'(2) + '🎉'(2) = 7 u16 units
    expect_eq(Utf8.isValid(sample), true);
    expect_eq(Utf8.encodedLength('A' as u32), 1 as usize);
    expect_eq(Utf8.encodedLength(0x4F60), 3 as usize);
    expect_eq(Utf8.encodedLength(0x1F680), 4 as usize);
    expect_eq(Utf8.encodedLength(0xD800), 0 as usize); // surrogate invalid scalar

    expect_eq(Utf16.utf16LengthFromUtf8(sample), 7 as usize);
    let u16_buf: [16]u16 = [0; 16];
    const u16_len = try Utf16.encodeUtf8ToUtf16LeStrict(sample, u16_buf[0..16]);
    expect_eq(u16_len, 7 as usize);
    expect_eq(u16_buf[0], 'A' as u16);
    expect_eq(u16_buf[3], 0xD83D as u16); // 🚀 high surrogate
    expect_eq(u16_buf[4], 0xDE80 as u16); // 🚀 low surrogate
    expect_eq(Utf16.isValid(u16_buf[0..u16_len]), true);
    expect_eq(Utf16.utf8LengthFromUtf16Le(u16_buf[0..u16_len]), sample.len);

    let u8_buf: [32]u8 = [0; 32];
    const u8_len = try Utf16.decodeUtf16LeToUtf8Strict(u16_buf[0..u16_len], u8_buf[0..32]);
    expect_eq(std.mem.eql(u8, u8_buf[0..u8_len], sample), true);

    // Allocator-backed roundtrip
    const u16_alloc = try Utf16.encodeUtf8ToUtf16LeAlloc(allocator, sample);
    defer allocator.free(u16_alloc);
    const u8_alloc = try Utf16.decodeUtf16LeToUtf8Alloc(allocator, u16_alloc);
    defer allocator.free(u8_alloc);
    expect_eq(std.mem.eql(u8, u8_alloc, sample), true);

    // 2. Invalid UTF-8 & lone UTF-16 surrogate replacement with U+FFFD + strict errors
    const bad_utf8: []const u8 = "ok\xFF!";
    expect_eq(Utf8.isValid(bad_utf8), false);
    let rep_buf: [16]u8 = [0; 16];
    const rep_len = Utf8.replaceInvalid(bad_utf8, rep_buf[0..16]);
    expect_eq(std.mem.eql(u8, rep_buf[0..rep_len], "ok\xEF\xBF\xBD!"), true);

    let bad_u16: [3]u16 = ['x' as u16, 0xD800 as u16, 'y' as u16];
    expect_eq(Utf16.isValid(bad_u16[0..3]), false);
    let bad_u8_out: [16]u8 = [0; 16];
    const bad_u8_len = Utf16.decodeUtf16LeToUtf8(bad_u16[0..3], bad_u8_out[0..16]);
    expect_eq(std.mem.eql(u8, bad_u8_out[0..bad_u8_len], "x\xEF\xBF\xBDy"), true);

    let strict_err = false;
    * = Utf16.decodeUtf16LeToUtf8Strict(bad_u16[0..3], bad_u8_out[0..16]) catch {
        strict_err = true;
    };
    expect_eq(strict_err, true);

    // 3. GraphemeIterator & graphemeCount (CRLF, combining marks, ZWJ, Regional Indicator pairs)
    const gr_text = "a\r\ncafe\xCC\x81👨‍👩‍👧🇺🇸🇨🇳!";
    // Clusters: "a" | "\r\n" | "c" | "a" | "f" | "é" | "👨‍👩‍👧" | "🇺🇸" | "🇨🇳" | "!" => 10
    expect_eq(graphemeCount(gr_text), 10 as usize);

    let git = GraphemeIterator.init(gr_text);
    expect_eq(std.mem.eql(u8, git.next().?, "a"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "\r\n"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "c"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "a"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "f"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "e\xCC\x81"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "👨‍👩‍👧"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "🇺🇸"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "🇨🇳"), true);
    expect_eq(std.mem.eql(u8, git.next().?, "!"), true);
    expect_eq(git.next() == null, true);

    // Odd regional indicator count: 3 RIs => 2 clusters (pair + single)
    expect_eq(graphemeCount("🇺🇸🇨"), 2 as usize);

    // Hangul Jamo L+V+T composition (U+1100 U+1161 U+11A8 => 1 grapheme cluster)
    const hangul_jamo = "\u{1100}\u{1161}\u{11A8}";
    expect_eq(graphemeCount(hangul_jamo), 1 as usize);

    // GraphemeIterator.peek() does not advance cursor
    let pgit = GraphemeIterator.init("🇺🇸A");
    expect_eq(std.mem.eql(u8, pgit.peek().?, "🇺🇸"), true);
    expect_eq(std.mem.eql(u8, pgit.next().?, "🇺🇸"), true);
    expect_eq(std.mem.eql(u8, pgit.next().?, "A"), true);
    expect_eq(pgit.next() == null, true);
}

fn testStringUtilsAndEscape(): !void {
    const allocator = std.heap.page_allocator;

    // 1. SIMD isAllAscii (>32 bytes)
    const pure_ascii = "0123456789abcdef0123456789abcdef_pure_ascii_tail";
    const non_ascii_tail = "0123456789abcdef0123456789abcdef_non_ascii_你好";
    expect_eq(isAllAscii(pure_ascii), true);
    expect_eq(isAllAscii(non_ascii_tail), false);

    // 2. SIMD toLowerAscii / toUpperAscii / eqlIgnoreCaseAscii
    const mixed = "Hello_CP_SIMD_String_Engine_2026_Payload";
    let low_buf: [64]u8 = [0; 64];
    let up_buf: [64]u8 = [0; 64];
    const lowered = toLowerAscii(mixed, low_buf[0..64]);
    const uppered = toUpperAscii(mixed, up_buf[0..64]);
    expect_eq(std.mem.eql(u8, lowered, "hello_cp_simd_string_engine_2026_payload"), true);
    expect_eq(std.mem.eql(u8, uppered, "HELLO_CP_SIMD_STRING_ENGINE_2026_PAYLOAD"), true);
    expect_eq(eqlIgnoreCaseAscii(lowered, uppered), true);
    expect_eq(eqlIgnoreCaseAscii("abc", "abd"), false);

    // 3. trim / startsWith / endsWith / indexOf / lastIndexOf
    const padded = "  \t\n  cap:string:engine  \r\n ";
    expect_eq(std.mem.eql(u8, trimStart(padded), "cap:string:engine  \r\n "), true);
    expect_eq(std.mem.eql(u8, trimEnd(padded), "  \t\n  cap:string:engine"), true);
    const trimmed = trim(padded);
    expect_eq(std.mem.eql(u8, trimmed, "cap:string:engine"), true);
    expect_eq(startsWith(trimmed, "cap:"), true);
    expect_eq(endsWith(trimmed, ":engine"), true);
    expect_eq(indexOf(trimmed, ":").?, 3 as usize);
    expect_eq(lastIndexOf(trimmed, ":").?, 10 as usize);
    expect_eq(indexOf(trimmed, "missing") == null, true);

    // 4. split & replaceAll
    let sp = split("alpha::beta::gamma", "::");
    expect_eq(std.mem.eql(u8, sp.next().?, "alpha"), true);
    expect_eq(std.mem.eql(u8, sp.next().?, "beta"), true);
    expect_eq(std.mem.eql(u8, sp.next().?, "gamma"), true);
    expect_eq(sp.next() == null, true);

    let rep_buf: [64]u8 = [0; 64];
    const rep = replaceAll("foo-bar-baz-bar", "bar", "CP", rep_buf[0..64]);
    expect_eq(std.mem.eql(u8, rep, "foo-CP-baz-CP"), true);

    const rep_alloc = try replaceAllAlloc(allocator, "one/two/three", "/", "::");
    defer allocator.free(rep_alloc);
    expect_eq(std.mem.eql(u8, rep_alloc, "one::two::three"), true);

    // 5. escapeJs
    let js_buf: [64]u8 = [0; 64];
    const js_in = "hi \"cp\"\\\n\t\x01";
    const js_len = escapeJs(js_in, js_buf[0..64]);
    expect_eq(std.mem.eql(u8, js_buf[0..js_len], "hi \\\"cp\\\"\\\\\\n\\t\\u0001"), true);
    expect_eq(StringEscape.needsJsEscape("plain_text"), false);
    expect_eq(StringEscape.needsJsEscape(js_in), true);
}

fn testBlobAndFileDeep(): !void {
    // 1. sliceSigned with negative indices & inverted ranges
    let b = Blob.fromString("0123456789", "text/plain");
    let neg_sub = b.sliceSigned(-4, -1, "");
    expect_eq(neg_sub.text(), "678");
    expect_eq(neg_sub.size, 3 as usize);

    let inv_sub = b.sliceSigned(7, 3, "application/octet-stream");
    expect_eq(inv_sub.size, 0 as usize);
    expect_eq(inv_sub.text(), "");
    expect_eq(b.store.ref_count, 3 as usize);

    // 2. Blob.concat & Blob.equals
    let p1 = Blob.fromString("Hello, ", "text/plain");
    let p2 = Blob.fromString("CP ", "text/plain");
    let p3 = Blob.fromString("Blob!", "text/plain");
    let parts: [3]Blob = [p1, p2, p3];
    let merged = Blob.concat(parts[0..3], "text/plain");
    expect_eq(merged.text(), "Hello, CP Blob!");

    let expected_blob = Blob.fromString("Hello, CP Blob!", "text/plain");
    expect_eq(merged.equals(expected_blob), true);
    expect_eq(merged.equals(p1), false);

    // 3. Blob.stream(chunk_size)
    let reader = merged.stream(6);
    expect_eq(reader.remaining(), 15 as usize);
    expect_eq(std.mem.eql(u8, reader.next().?, "Hello,"), true);
    expect_eq(std.mem.eql(u8, reader.next().?, " CP Bl"), true);
    expect_eq(std.mem.eql(u8, reader.next().?, "ob!"), true);
    expect_eq(reader.next() == null, true);
    expect_eq(reader.remaining(), 0 as usize);
    reader.reset();
    expect_eq(reader.remaining(), 15 as usize);

    // 4. Non-existent file returns null safely
    expect_eq(Blob.fromFile("temp/does_not_exist_99999.bin", "") == null, true);
    expect_eq(File.fromPath("temp/does_not_exist_99999.bin", "") == null, true);

    // 5. File disk lifecycle: write -> exists -> stat -> fromPath -> delete
    const test_path = "temp/test_blob_deep_disk.txt";
    let f_handle = File.at(test_path, "text/plain");
    expect_eq(f_handle.write("CP-File-Disk-Payload"), true);
    expect_eq(f_handle.exists(), true);
    expect_eq(f_handle.size, 20 as usize);
    expect_eq(f_handle.lastModified > 0, true);
    expect_eq(std.mem.eql(u8, f_handle.path, test_path), true);
    expect_eq(std.mem.eql(u8, f_handle.name, "test_blob_deep_disk.txt"), true);

    let reloaded = File.fromPath(test_path, "").?;
    expect_eq(reloaded.text(), "CP-File-Disk-Payload");
    expect_eq(reloaded.size, 20 as usize);
    expect_eq(reloaded.lastModified > 0, true);
    expect_eq(reloaded.delete(), true);
    expect_eq(reloaded.exists(), false);

    reloaded.~init();
    f_handle.~init();
    expected_blob.~init();
    merged.~init();
    p3.~init();
    p2.~init();
    p1.~init();
    inv_sub.~init();
    neg_sub.~init();
    b.~init();
}

fn testImageDeep(): !void {
    // 1. ImageFormat.detect & probeDimensions across PNG/JPEG/GIF/WebP/BMP/ICO/SVG
    let raw_ptr: [*]u8 = @alloc(16);
    let rgba = raw_ptr[0..16];
    rgba[0] = 200; rgba[1] = 100; rgba[2] = 50;  rgba[3] = 255; // Red-ish
    rgba[4] = 40;  rgba[5] = 200; rgba[6] = 60;  rgba[7] = 255; // Green-ish
    rgba[8] = 30;  rgba[9] = 80;  rgba[10] = 220; rgba[11] = 255; // Blue-ish
    rgba[12] = 240; rgba[13] = 240; rgba[14] = 240; rgba[15] = 255; // Light gray

    const png_bytes = Png.encodeRgba(rgba, 2, 2);
    expect_eq(ImageFormat.detect(png_bytes).? == ImageFormat.Png, true);
    const png_dim = probeDimensions(png_bytes).?;
    expect_eq(png_dim.width, 2 as u32);
    expect_eq(png_dim.height, 2 as u32);

    const bmp_bytes = BmpEncodeHelper(rgba, 2, 2);
    expect_eq(ImageFormat.detect(bmp_bytes).? == ImageFormat.Bmp, true);
    const bmp_dim = probeDimensions(bmp_bytes).?;
    expect_eq(bmp_dim.width, 2 as u32);
    expect_eq(bmp_dim.height, 2 as u32);

    // Minimal valid 2x2 GIF89a with 2-color GCT (color 0 = Red, color 1 = Green) and LZW payload
    // Pixels: [0, 1, 1, 0] -> Red, Green, Green, Red
    // min_code=2 (clear=4, eoi=5): bits LSB: 4(3b)=100, 0(3b)=000, 1(3b)=001, 1(3b)=001, 0(3b)=000, 5(3b)=101
    // Stream of 18 bits: 00100100 10000100 00000010 -> bytes: 0x24, 0x84, 0x02? Wait:
    // Let's pack LSB-first carefully:
    // b0..b2: 4 (100)
    // b3..b5: 0 (000)
    // b6..b7 + b8: 1 (001) => byte0 = 0b01_000_100 = 0x44
    // b8: 0, b9..b11: 1 (001) => bits 8..15: b8=0, b9..11=001, b12..14=000, b15=1 => 0b1_000_001_0 = 0x82
    // b16..b17: 01 (rest of 5=101) => byte2 = 0x02
    const gif_bytes: string = "GIF89a\x02\x00\x02\x00\x80\x00\x00\xFF\x00\x00\x00\xFF\x00\x2C\x00\x00\x00\x00\x02\x00\x02\x00\x00\x02\x03\x44\x82\x02\x00\x3B";
    expect_eq(ImageFormat.detect(gif_bytes).? == ImageFormat.Gif, true);
    const gif_dim = probeDimensions(gif_bytes).?;
    expect_eq(gif_dim.width, 2 as u32);
    expect_eq(gif_dim.height, 2 as u32);
    const gif_img = Gif.decode(gif_bytes).?;
    expect_eq(gif_img.width, 2 as usize);
    expect_eq(gif_img.height, 2 as usize);
    expect_eq(gif_img.rgba[0], 255 as u8); // (0,0) Red
    expect_eq(gif_img.rgba[5], 255 as u8); // (1,0) Green

    // ICO header: 00 00 01 00 01 00 20 18 ... (32x24)
    const ico_bytes: string = "\x00\x00\x01\x00\x01\x00\x20\x18\x00\x00\x01\x00\x20\x00\x00\x00\x00\x00\x16\x00\x00\x00";
    expect_eq(ImageFormat.detect(ico_bytes).? == ImageFormat.Ico, true);
    const ico_dim = probeDimensions(ico_bytes).?;
    expect_eq(ico_dim.width, 32 as u32);
    expect_eq(ico_dim.height, 24 as u32);

    // SVG detection & dimension probing (attrs and viewBox)
    const svg_attr = "  <?xml version=\"1.0\"?><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"640px\" height=\"480px\"></svg>";
    expect_eq(ImageFormat.detect(svg_attr).? == ImageFormat.Svg, true);
    const svg_dim1 = probeDimensions(svg_attr).?;
    expect_eq(svg_dim1.width, 640 as u32);
    expect_eq(svg_dim1.height, 480 as u32);

    const svg_vb = "<svg viewBox=\"0 0 1280 720\"></svg>";
    expect_eq(ImageFormat.detect(svg_vb).? == ImageFormat.Svg, true);
    const svg_dim2 = probeDimensions(svg_vb).?;
    expect_eq(svg_dim2.width, 1280 as u32);
    expect_eq(svg_dim2.height, 720 as u32);

    // JPEG SOF0 & WebP VP8/VP8L probeDimensions (header-only)
    const jpeg_sof0: string = "\xFF\xD8\xFF\xC0\x00\x0B\x08\x00\x20\x00\x40\x03\x01\x11\x00\xFF\xDA";
    expect_eq(ImageFormat.detect(jpeg_sof0).? == ImageFormat.Jpeg, true);
    const jdim = probeDimensions(jpeg_sof0).?;
    expect_eq(jdim.width, 64 as u32);
    expect_eq(jdim.height, 32 as u32);

    const webp_vp8: string = "RIFF\x1E\x00\x00\x00WEBPVP8 \x0A\x00\x00\x00\x00\x00\x00\x9D\x01\x2A\x64\x00\x32\x00";
    expect_eq(ImageFormat.detect(webp_vp8).? == ImageFormat.WebP, true);
    const wdim = probeDimensions(webp_vp8).?;
    expect_eq(wdim.width, 100 as u32);
    expect_eq(wdim.height, 50 as u32);

    // 2. ThumbHash real Base64 dataUrl
    const th = ThumbHash.encode(rgba, 2, 2);
    const data_url = ThumbHash.toDataUrl(th);
    expect_eq(startsWith(data_url, "data:image/png;base64,iVBOR"), true);

    // 3. Image Pipeline: filters, thumbnail, decode, savePng -> load roundtrip
    let img = Image.init(2, 2, rgba);
    let gray = img.grayscale();
    expect_eq(gray.rgba[0], gray.rgba[1]);
    expect_eq(gray.rgba[1], gray.rgba[2]);
    expect_eq(gray.rgba[3], 255 as u8);

    let inv = img.invert();
    expect_eq(inv.rgba[0], (255 - 200) as u8);
    expect_eq(inv.rgba[3], 255 as u8);

    let bright = img.brightness(20);
    expect_eq(bright.rgba[0], 220 as u8);

    let sep = img.sepia();
    expect_eq(sep.rgba[3], 255 as u8);

    let blurred = img.blur();
    expect_eq(blurred.width, 2 as usize);

    let big = img.resize(10, 6);
    let thumb = big.thumbnail(5, 5);
    expect_eq(thumb.width, 5 as usize);
    expect_eq(thumb.height, 3 as usize);

    let oriented = img.applyExifOrientation(ExifOrientation.Rotate90);
    expect_eq(oriented.width, 2 as usize);

    const tmp_png = "temp/test_image_deep_roundtrip.png";
    expect_eq(gray.savePng(tmp_png), true);
    const loaded = Image.load(tmp_png).?;
    expect_eq(loaded.width, 2 as usize);
    expect_eq(loaded.height, 2 as usize);
    expect_eq(loaded.rgba[0], gray.rgba[0]);
}

fn BmpEncodeHelper(rgba: []const u8, w: usize, h: usize): []u8 {
    let img = Image.init(w, h, @constCast(rgba));
    return img.toBmp();
}

fn testMemDeep(): !void {
    // 1. Alignment primitives
    expect_eq(isPowerOfTwo(0), false);
    expect_eq(isPowerOfTwo(1), true);
    expect_eq(isPowerOfTwo(16), true);
    expect_eq(isPowerOfTwo(18), false);
    expect_eq(alignForward(13, 8), 16 as usize);
    expect_eq(alignForward(16, 16), 16 as usize);
    expect_eq(alignBackward(19, 8), 16 as usize);
    expect_eq(alignDelta(13, 16), 3 as usize);

    // 2. secureZero
    let secret: [16]u8 = [0xAA; 16];
    secureZero(secret[0..16]);
    for (let i in 0..16) {
        expect_eq(secret[i as usize], 0 as u8);
    }

    // 3. FixedBufferAllocator
    let backing: [128]u8 = [0; 128];
    let fba = FixedBufferAllocator.init(backing[0..128]);
    expect_eq(fba.capacity(), 128 as usize);
    expect_eq(fba.remaining(), 128 as usize);

    const a1 = fba.alloc(10).?;
    expect_eq(a1.len, 10 as usize);
    expect_eq(fba.used(), 10 as usize);
    expect_eq(fba.ownsSlice(a1), true);

    const a2 = fba.allocAligned(32, 16).?;
    expect_eq(a2.len, 32 as usize);
    let a2_addr: usize = 0;
    unsafe {
        a2_addr = @intFromPtr(a2.ptr);
    }
    expect_eq((a2_addr % 16) == 0, true);

    // LIFO free rewinds top allocation
    const used_before_free = fba.used();
    fba.free(a2);
    expect_eq(fba.used() < used_before_free, true);

    // Over-capacity returns null safely
    expect_eq(fba.alloc(256) == null, true);

    fba.reset();
    expect_eq(fba.used(), 0 as usize);
    expect_eq(fba.remaining(), 128 as usize);

    // 4. SlabPool: O(1) alloc/free & double-free defense
    let pool = SlabPool.init(32, 3);
    defer pool.~init();
    expect_eq(pool.slotSize(), 32 as usize);
    expect_eq(pool.capacity(), 3 as usize);
    expect_eq(pool.available(), 3 as usize);

    let s1 = pool.alloc().?;
    let s2 = pool.alloc().?;
    let s3 = pool.alloc().?;
    expect_eq(s1.len, 32 as usize);
    expect_eq(pool.used(), 3 as usize);
    expect_eq(pool.available(), 0 as usize);
    expect_eq(pool.contains(s2), true);
    // Pool exhausted -> null
    expect_eq(pool.alloc() == null, true);

    // Free s2 and verify double-free returns false
    expect_eq(pool.free(s2), true);
    expect_eq(pool.free(s2), false); // Double-free safely rejected!
    expect_eq(pool.used(), 2 as usize);
    expect_eq(pool.available(), 1 as usize);

    // Re-alloc succeeds in O(1)
    let s4 = pool.alloc().?;
    expect_eq(s4.len, 32 as usize);
    expect_eq(pool.used(), 3 as usize);

    // Foreign slice cannot be freed
    let foreign: [32]u8 = [0; 32];
    expect_eq(pool.free(foreign[0..32]), false);

    pool.reset();
    expect_eq(pool.used(), 0 as usize);
    expect_eq(pool.available(), 3 as usize);
}

fn main() {
    test("1. cap:string — UTF-8/UTF-16LE Surrogate Pairs, Strict Errors & UAX #29 GraphemeIterator", () => {
        try testStringUtfAndGraphemes();
    });
    test("2. cap:string — SIMD isAllAscii, Case, Trim, Split, ReplaceAll & escapeJs", () => {
        try testStringUtilsAndEscape();
    });
    test("3. cap:blob — sliceSigned, concat, equals, BlobChunkReader stream & std.c File CRUD", () => {
        try testBlobAndFileDeep();
    });
    test("4. cap:image — ImageFormat.detect, probeDimensions, GIF decode, real ThumbHash & Filters", () => {
        try testImageDeep();
    });
    test("5. cap:mem — Alignment Primitives, secureZero, FixedBufferAllocator & SlabPool Double-Free Guard", () => {
        try testMemDeep();
    });
}
