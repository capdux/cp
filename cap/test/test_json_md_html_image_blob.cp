// cap/test/test_json_md_html_image_blob.cp
// Comprehensive E2E Regression Suite for Data Format & Media Standard Libraries:
// cap:json, cap:md, cap:html, cap:image, cap:blob

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JSON, JsonParser, JsonValue, JsonTag, FastJsonScanner, JsonBufferPool } from "cap:json";
import { MdOptions, Markdown, MdTableAlign, MdTable } from "cap:md";
import { HtmlEscape, CssSelector, HtmlRewriter } from "cap:html";
import {
    Codecs, ImageFormat, ExifOrientation, ExifTransform,
    Png, PngImage, RgbaColor,
    Bmp,
    Quantizer,
    Gif,
    ThumbHash,
    Exif,
    Jpeg,
    Webp,
    Image,
} from "cap:image";
import { Blob, File, BlobStore, BlobKind } from "cap:blob";

pub struct MediaMeta {
    id: i64 = 0,
    title: []const u8 = "",
    published: bool = false,
    score: f64 = 0,
    tags: [][]const u8 = [],
    channel: []const u8 = "default",
}

fn runJsonDynamicAndZeroCopy(): !void {
    const allocator = std.heap.page_allocator;

    // 1. SIMD scanner check
    const raw_scan = "leading_padding_bytes_\"hello_simd_json\"_tail";
    const quote_idx = std.mem.indexOfScalar(u8, raw_scan, '"').?;
    const hit = FastJsonScanner.indexOfQuoteOrEscape(raw_scan[quote_idx + 1 ..]);
    expect_eq(hit != null, true);
    if (h := hit) {
        expect_eq(std.mem.eql(u8, raw_scan[quote_idx + 1 .. quote_idx + 1 + h], "hello_simd_json"), true);
    }

    // 2. Zero-copy DOM parse & nested lookup
    const payload = "{\"agent\":\"cp_media\",\"ok\":true,\"empty\":null,\"nums\":[10,20.5,30]}";
    const root = try JSON.parse(allocator, payload);
    expect_eq(root.isObject(), true);
    expect_eq(root.len(), 4 as usize);

    const agent_v = root.get("agent").?;
    expect_eq(agent_v.isString(), true);
    const agent_s = agent_v.asString().?;
    expect_eq(std.mem.eql(u8, agent_s, "cp_media"), true);
    // Verify zero-copy pointer borrow into payload
    const base_ptr = @intFromPtr(payload.ptr);
    const str_ptr = @intFromPtr(agent_s.ptr);
    expect_eq(str_ptr >= base_ptr && str_ptr < base_ptr + payload.len, true);

    expect_eq(root.get("ok").?.asBool().?, true);
    expect_eq(root.get("empty").?.isNull(), true);

    const nums = root.get("nums").?;
    expect_eq(nums.isArray(), true);
    expect_eq(nums.len(), 3 as usize);
    const second = nums.getIndex(1).?.asNumber().?;
    expect_eq(second > 20.4 && second < 20.6, true);
}

fn runJsonTypedAndPool(): !void {
    const allocator = std.heap.page_allocator;
    const typed_src = "{\"id\":42,\"title\":\"CP Media\",\"published\":true,\"score\":98.5,\"tags\":[\"png\",\"md\",\"blob\"],\"ignored\":{\"nested\":[1,2]}}";

    let meta = MediaMeta{};
    zig {
        meta = try JSON.parseTyped(MediaMeta, allocator, typed_src);
    }
    expect_eq(meta.id, 42 as i64);
    expect_eq(std.mem.eql(u8, meta.title, "CP Media"), true);
    expect_eq(meta.published, true);
    expect_eq(meta.score > 98.4 && meta.score < 98.6, true);
    expect_eq(meta.tags.len, 3 as usize);
    expect_eq(std.mem.eql(u8, meta.tags[0], "png"), true);
    expect_eq(std.mem.eql(u8, meta.tags[2], "blob"), true);
    expect_eq(std.mem.eql(u8, meta.channel, "default"), true);

    // Verify JsonBufferPool thread-local acquire/recycle reuse
    const n1 = JsonBufferPool.acquire(allocator);
    const ptr1 = @intFromPtr(n1);
    JsonBufferPool.recycle(n1);
    const n2 = JsonBufferPool.acquire(allocator);
    expect_eq(@intFromPtr(n2), ptr1);
    JsonBufferPool.recycle(n2);
}

fn main() {
    test("1. cap:json — SIMD Scanner & Zero-Copy DOM Parser", () => {
        try runJsonDynamicAndZeroCopy();
    });

    test("2. cap:json — Typed Struct Deserialization & Thread-Local BufferPool", () => {
        try runJsonTypedAndPool();
    });

    test("3. cap:md — MdOptions, ATX Headings, Lists, Inline Styles & ANSI Renderer", () => {
        const cm = MdOptions.commonmark();
        expect_eq(cm.tables, false);
        expect_eq(cm.heading_ids, false);

        const gh = MdOptions.github();
        expect_eq(gh.tables, true);
        expect_eq(gh.strikethrough, true);
        expect_eq(gh.heading_ids, true);

        const md_doc = "# Hello **CP**\n- item `one`\nPlain **bold** text\n";
        let html_buf: [512]u8 = [0; 512];
        const rendered = Markdown.renderHtml(md_doc, html_buf[0..512]);
        const expected = "<h1>Hello <strong>CP</strong></h1>\n<li>item <code>one</code></li>\n<p>Plain <strong>bold</strong> text</p>\n";
        expect_eq(std.mem.eql(u8, rendered, expected), true);

        let ansi_buf: [256]u8 = [0; 256];
        const ansi_out = Markdown.renderAnsi("# Title\nBody", ansi_buf[0..256]);
        expect_eq(std.mem.eql(u8, ansi_out, "\x1b[1;36mTitle\x1b[0m\nBody\n"), true);
    });

    test("4. cap:md — GFM Table Alignment, Row Splitting & HTML Table Renderer", () => {
        expect_eq(MdTable.parseAlignSpec(":---").? == MdTableAlign.Left, true);
        expect_eq(MdTable.parseAlignSpec(":---:").? == MdTableAlign.Center, true);
        expect_eq(MdTable.parseAlignSpec("---:").? == MdTableAlign.Right, true);
        expect_eq(MdTable.parseAlignSpec("---").? == MdTableAlign.None, true);
        expect_eq(MdTable.parseAlignSpec("bad") == null, true);

        const md_table = "| Format | Kind | Fast |\n| :--- | :---: | ---: |\n| PNG | Image | Yes |\n| JSON | Data | Yes |\n";
        let out_buf: [512]u8 = [0; 512];
        const table_html = MdTable.renderHtml(md_table, out_buf[0..512]);
        const expected = "<table><thead><tr><th align=\"left\">Format</th><th align=\"center\">Kind</th><th align=\"right\">Fast</th></tr></thead><tbody><tr><td align=\"left\">PNG</td><td align=\"center\">Image</td><td align=\"right\">Yes</td></tr><tr><td align=\"left\">JSON</td><td align=\"center\">Data</td><td align=\"right\">Yes</td></tr></tbody></table>";
        expect_eq(std.mem.eql(u8, table_html, expected), true);
    });

    test("5. cap:html — Entity Escaping/Unescaping, CssSelector & HtmlRewriter", () => {
        expect_eq(HtmlEscape.needsEscape("Clean Text"), false);
        expect_eq(HtmlEscape.needsEscape("a < b & c"), true);

        let esc_buf: [256]u8 = [0; 256];
        const escaped = HtmlEscape.escapeToBuf("<a href=\"/\">Tom & 'Jerry'</a>", esc_buf[0..256]);
        expect_eq(std.mem.eql(u8, escaped, "&lt;a href=&quot;/&quot;&gt;Tom &amp; &#39;Jerry&#39;&lt;/a&gt;"), true);

        let unesc_buf: [256]u8 = [0; 256];
        const unescaped = HtmlEscape.unescapeToBuf(escaped, unesc_buf[0..256]);
        expect_eq(std.mem.eql(u8, unescaped, "<a href=\"/\">Tom & 'Jerry'</a>"), true);

        const sel_id = CssSelector.parse("div#hero");
        expect_eq(sel_id.matchesTag("div"), true);
        expect_eq(sel_id.matchesTag("span"), false);
        expect_eq(std.mem.eql(u8, sel_id.id_name, "hero"), true);

        const sel_attr = CssSelector.parse("input.field[type=\"email\"]");
        expect_eq(sel_attr.matchesTag("input"), true);
        expect_eq(std.mem.eql(u8, sel_attr.class_name, "field"), true);
        expect_eq(std.mem.eql(u8, sel_attr.attr_name, "type"), true);
        expect_eq(std.mem.eql(u8, sel_attr.attr_val, "email"), true);

        let text_buf: [128]u8 = [0; 128];
        const plain = HtmlRewriter.stripTags("<div>Hello <b>CP</b>!</div>", text_buf[0..128]);
        expect_eq(std.mem.eql(u8, plain, "Hello CP!"), true);

        let rename_buf: [128]u8 = [0; 128];
        const renamed = HtmlRewriter.renameTag("<div class=\"x\">Hi</div>", "div", "section", rename_buf[0..128]);
        expect_eq(std.mem.eql(u8, renamed, "<section class=\"x\">Hi</section>"), true);
    });

    test("6. cap:image — Codecs Sniff, PNG/BMP Encode-Decode, Quantizer & Pipeline", () => {
        // Build 2x2 RGBA test image
        let ptr: [*]u8 = @alloc(16);
        let rgba = ptr[0..16];
        rgba[0] = 255; rgba[1] = 0; rgba[2] = 0; rgba[3] = 255;     // (0,0) Red
        rgba[4] = 0; rgba[5] = 255; rgba[6] = 0; rgba[7] = 255;     // (1,0) Green
        rgba[8] = 0; rgba[9] = 0; rgba[10] = 255; rgba[11] = 255;   // (0,1) Blue
        rgba[12] = 255; rgba[13] = 255; rgba[14] = 0; rgba[15] = 255; // (1,1) Yellow

        // PNG roundtrip
        const png_bytes = Png.encodeRgba(rgba, 2, 2);
        expect_eq(png_bytes.len > 40, true);
        expect_eq(Codecs.sniff(png_bytes).? == ImageFormat.Png, true);
        expect_eq(Codecs.mime(ImageFormat.Png), "image/png");
        expect_eq(Codecs.fromExtension("snap.webp").? == ImageFormat.WebP, true);

        let decoded_png = Png.decode(png_bytes).?;
        expect_eq(decoded_png.width, 2 as u32);
        expect_eq(decoded_png.height, 2 as u32);
        const px00 = decoded_png.getPixel(0, 0);
        expect_eq(px00.r, 255 as u8);
        expect_eq(px00.g, 0 as u8);
        const px11 = decoded_png.getPixel(1, 1);
        expect_eq(px11.r, 255 as u8);
        expect_eq(px11.g, 255 as u8);

        // BMP roundtrip
        const bmp_bytes = Bmp.encodeRgba(rgba, 2, 2);
        expect_eq(Codecs.sniff(bmp_bytes).? == ImageFormat.Bmp, true);
        let decoded_bmp = Bmp.decode(bmp_bytes).?;
        expect_eq(decoded_bmp.width, 2 as u32);
        expect_eq(decoded_bmp.getPixel(1, 0).g, 255 as u8);

        // Quantizer
        const q = Quantizer.quantize(rgba, 2, 2, 16);
        expect_eq(q.colors, 4 as u16);
        expect_eq(q.has_alpha, false);

        // Pipeline operations & export
        let img = Image.init(2, 2, rgba);
        let flopped = img.flop();
        expect_eq(flopped.rgba[0], 0 as u8);
        expect_eq(flopped.rgba[1], 255 as u8); // Green moved to (0,0)

        let flipped = img.flip();
        expect_eq(flipped.rgba[2], 255 as u8); // Blue moved to (0,0)

        let rot90 = img.rotate(90);
        expect_eq(rot90.width, 2 as usize);
        expect_eq(rot90.rgba[2], 255 as u8); // Blue at (0,1) -> (0,0)

        let cropped = img.crop(1, 1, 1, 1);
        expect_eq(cropped.width, 1 as usize);
        expect_eq(cropped.rgba[0], 255 as u8);
        expect_eq(cropped.rgba[1], 255 as u8);

        let resized = img.resize(4, 4);
        expect_eq(resized.width, 4 as usize);
        expect_eq(resized.rgba.len, 64 as usize);

        const exported_png = img.toPng();
        expect_eq(Codecs.sniff(exported_png).? == ImageFormat.Png, true);
        const exported_bmp = img.toBmp();
        expect_eq(Codecs.sniff(exported_bmp).? == ImageFormat.Bmp, true);
    });

    test("7. cap:image — GIF, JPEG, EXIF, WebP Probes & ThumbHash", () => {
        // GIF probe & decode
        const gif_data: string = "GIF89a\x02\x00\x02\x00\x80\x00\x00\xFF\x00\x00\x00\x00\xFF\x2C\x00\x00\x00\x00\x02\x00\x02\x00\x00\x02\x03\x04\x15\x05\x00\x3B";
        expect_eq(Codecs.sniff(gif_data).? == ImageFormat.Gif, true);
        const gh = Gif.probe(gif_data).?;
        expect_eq(gh.width, 2 as usize);
        expect_eq(gh.height, 2 as usize);
        expect_eq(gh.has_gct, true);
        expect_eq(gh.gct_size, 2 as usize);

        // JPEG probe + EXIF orientation (SOF0 64x32 + APP1 Rotate90)
        const jpeg_exif: string = "\xFF\xD8\xFF\xE1\x00\x22Exif\x00\x00II\x2A\x00\x08\x00\x00\x00\x01\x00\x12\x01\x03\x00\x01\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00\xFF\xC0\x00\x0B\x08\x00\x20\x00\x40\x03\x01\x11\x00\xFF\xDA";
        expect_eq(Codecs.sniff(jpeg_exif).? == ImageFormat.Jpeg, true);
        const jh = Jpeg.probe(jpeg_exif).?;
        expect_eq(jh.width, 64 as usize);
        expect_eq(jh.height, 32 as usize);
        expect_eq(jh.components, 3 as usize);
        expect_eq(jh.is_progressive, false);
        expect_eq(Codecs.getOrientationTransform(jh.orientation).rotate, 90 as u16);

        // WebP VP8 probe
        const webp_raw: string = "RIFF\x1E\x00\x00\x00WEBPVP8 \x0A\x00\x00\x00\x00\x00\x00\x9D\x01\x2A\x64\x00\x32\x00";
        expect_eq(Codecs.sniff(webp_raw).? == ImageFormat.WebP, true);
        const wp = Webp.probe(webp_raw).?;
        expect_eq(wp.width, 100 as usize);
        expect_eq(wp.height, 50 as usize);
        expect_eq(wp.is_lossless, false);

        // ThumbHash encode/decode/toDataUrl
        let th_ptr: [*]u8 = @alloc(4 * 4 * 4);
        let th_rgba = th_ptr[0..64];
        for (let i in 0..16) {
            let p = (i as usize) * 4;
            th_rgba[p + 0] = 100;
            th_rgba[p + 1] = 180;
            th_rgba[p + 2] = 240;
            th_rgba[p + 3] = 255;
        }
        const hash = ThumbHash.encode(th_rgba, 4, 4);
        expect_eq(hash.len, ThumbHash.MAX_LEN);
        const thumb_decoded = ThumbHash.decode(hash).?;
        expect_eq(thumb_decoded.width, 4 as usize);
        expect_eq(thumb_decoded.height, 4 as usize);
        expect_eq(thumb_decoded.rgba[0] >= 95, true);
        const data_url = ThumbHash.toDataUrl(hash);
        expect_eq(data_url.len > 0, true);
    });

    test("8. cap:blob — BlobStore RefCount, Zero-Copy Slice, ArrayBuffer & File Roundtrip", () => {
        let store = BlobStore.fromBytes("0123456789");
        expect_eq(store.kind == BlobKind.Bytes, true);
        expect_eq(store.ref_count, 1 as usize);
        store.ref();
        expect_eq(store.ref_count, 2 as usize);
        store.deref();
        store.deref();
        expect_eq(store.ref_count, 0 as usize);

        let blob_obj = Blob.fromString("CP-Blob-Payload-2026", "");
        expect_eq(blob_obj.size, 20 as usize);
        expect_eq(blob_obj.type, "text/plain;charset=utf-8");

        let sub = blob_obj.slice(3, 15, "application/x-cp");
        expect_eq(sub.text(), "Blob-Payload");
        expect_eq(sub.type, "application/x-cp");
        expect_eq(blob_obj.store.ref_count, 2 as usize);

        let ab = sub.arrayBuffer();
        expect_eq(ab.len, 12 as usize);
        expect_eq(ab[0], 66 as u8); // 'B'

        let file_obj = File.init("Media Suite File Data", "report.json", "application/json", 1732000000 as i64);
        expect_eq(file_obj.name, "report.json");
        expect_eq(file_obj.size, 21 as usize);

        const out_path = "temp/test_media_suite_blob.json";
        expect_eq(file_obj.writeTo(out_path), true);

        let loaded = File.fromPath(out_path, "").?;
        expect_eq(loaded.name, "test_media_suite_blob.json");
        expect_eq(loaded.type, "application/json");
        expect_eq(loaded.text(), "Media Suite File Data");

        loaded.release();
        file_obj.release();
        sub.release();
        blob_obj.release();
    });
}
