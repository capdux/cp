// Source: cp/cap/test/testWebSchemaAndFileType.cp
// Comprehensive regression + 2000-iteration deterministic Fuzz suite for:
//   - lib/web/schema.cp (16-lane SIMD fastDecodeUriComponent, query helpers,
//     ObjectSchema, validateJsonObject, exactMirrorJson, schemaToOpenApiJson)
//   - lib/web/fileType.cp (30-format zero-heap magic-number + text heuristic detectFileType)

import { test, expect_eq } from "cap:test";
import {
    FieldKind,
    FieldRule,
    ObjectSchema,
    ValidationIssue,
    ValidationReport,
    QueryPair,
    fastDecodeUriComponent,
    fastDecodeFormComponent,
    fastDecodeUriComponentOpts,
    parseQueryPair,
    findQueryParamDecoded,
    validateJsonObject,
    exactMirrorJson,
    schemaToOpenApiJson,
} from "../../lib/web/schema.cp";
import {
    FileKind,
    FileTypeInfo,
    detectFileType,
} from "../../lib/web/fileType.cp";

fn containsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (std.mem.eql(u8, haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

fn xorshift64(state: *u64): u64 {
    let x: u64 = unsafe { state.* };
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    unsafe { state.* = x; }
    return x;
}

fn buildUserSchema(): ObjectSchema {
    let s = ObjectSchema.init("UserCreate");
    * = s.strField("name", true, 2, 32)
        .intField("age", true, 18, 120)
        .boolField("active", false)
        .emailField("email", true)
        .uuidField("id", true)
        .urlField("website", false)
        .enumField("role", true, "admin,user,guest");
    return s;
}

fn main() {
    test("1. SIMD fastDecodeUriComponent: ASCII fast-path, UTF-8 Chinese, plus, invalid hex", () => {
        let out: [256]u8 = [0; 256];

        // Long pure ASCII (> 32 bytes, exercises 16-lane SIMD zero-branch copy)
        const ascii_in = "abcdefghijklmnopqrstuvwxyz0123456789-_.~plain_segment_ok";
        const ascii_len = fastDecodeUriComponent(ascii_in, out[0..]) ?? 0;
        expect_eq(ascii_len, ascii_in.len);
        expect_eq(std.mem.eql(u8, out[0..ascii_len], ascii_in), true);

        // UTF-8 Chinese percent-encoding ("你好世界")
        const zh_in = "prefix_16bytes__%E4%BD%A0%E5%A5%BD%E4%B8%96%E7%95%8C_tail";
        const zh_len = fastDecodeUriComponent(zh_in, out[0..]) ?? 0;
        expect_eq(std.mem.eql(u8, out[0..zh_len], "prefix_16bytes__你好世界_tail"), true);

        // Plus handling: literal in URI component vs space in form component
        const plus_in = "hello+world+from+cp%21";
        const lit_len = fastDecodeUriComponent(plus_in, out[0..]) ?? 0;
        expect_eq(std.mem.eql(u8, out[0..lit_len], "hello+world+from+cp!"), true);

        const form_len = fastDecodeFormComponent(plus_in, out[0..]) ?? 0;
        expect_eq(std.mem.eql(u8, out[0..form_len], "hello world from cp!"), true);

        // Invalid hex & truncated escapes must return null
        expect_eq(fastDecodeUriComponent("bad%ZZhex", out[0..]) == null, true);
        expect_eq(fastDecodeUriComponent("truncated%2", out[0..]) == null, true);
        expect_eq(fastDecodeUriComponent("trailing%", out[0..]) == null, true);
        expect_eq(fastDecodeUriComponent("0123456789abcdef%G0", out[0..]) == null, true);

        // Buffer too small returns null
        let tiny: [4]u8 = [0; 4];
        expect_eq(fastDecodeUriComponent("12345678901234567", tiny[0..]) == null, true);

        // Query pair & decoded lookup
        const pair = parseQueryPair("q=cp+lang%20rocks");
        expect_eq(std.mem.eql(u8, pair.key, "q"), true);
        expect_eq(std.mem.eql(u8, pair.value, "cp+lang%20rocks"), true);

        const qstr = "?page=2&q=%E4%BD%A0%E5%A5%BD+CP&tag=simd%2Fweb";
        const q_len = findQueryParamDecoded(qstr, "q", out[0..]) ?? 0;
        expect_eq(std.mem.eql(u8, out[0..q_len], "你好 CP"), true);

        const tag_len = findQueryParamDecoded(qstr, "tag", out[0..]) ?? 0;
        expect_eq(std.mem.eql(u8, out[0..tag_len], "simd/web"), true);
        expect_eq(findQueryParamDecoded(qstr, "missing", out[0..]) == null, true);
    });

    test("2. SIMD fastDecodeUriComponent: 1000-iter deterministic xorshift64 Fuzz (0 panic)", () => {
        let seed: u64 = 0x1E3779B97F4A7C15;
        let in_buf: [96]u8 = [0; 96];
        let out_buf: [128]u8 = [0; 128];
        let ok_count: usize = 0;
        let err_count: usize = 0;

        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const len: usize = @as(usize, @intCast(xorshift64(&seed) % 96));
            let j: usize = 0;
            while (j < len) : (j += 1) {
                const r = xorshift64(&seed);
                const pick = r % 8;
                if (pick == 0) {
                    in_buf[j] = '%';
                } else if (pick == 1) {
                    in_buf[j] = '+';
                } else if (pick == 2) {
                    in_buf[j] = ('0' as u8) + @as(u8, @intCast(r % 10));
                } else if (pick == 3) {
                    in_buf[j] = ('A' as u8) + @as(u8, @intCast(r % 6));
                } else if (pick == 4) {
                    in_buf[j] = ('G' as u8) + @as(u8, @intCast(r % 10));
                } else {
                    in_buf[j] = @as(u8, @intCast(r & 0xFF));
                }
            }
            const plus_flag = (iter & 1) == 1;
            if (decoded_len := fastDecodeUriComponentOpts(in_buf[0..len], out_buf[0..], plus_flag)) {
                expect_eq(decoded_len <= len, true);
                ok_count += 1;
            } else {
                err_count += 1;
            }
        }
        expect_eq(ok_count + err_count, 1000);
        expect_eq(err_count > 0, true);
    });

    test("3. ObjectSchema & validateJsonObject: valid, missing, bounds, formats, nested/escaped", () => {
        const schema = buildUserSchema();

        // 1. Valid payload with nested extra object & escaped strings
        const valid_json = "{\"id\":\"550e8400-e29b-41d4-a716-446655440000\",\"name\":\"Alice \\\"CP\\\"\",\"age\":28,\"active\":true,\"email\":\"alice@cp-lang.dev\",\"website\":\"https://cp-lang.dev/docs\",\"role\":\"admin\",\"meta\":{\"nested\":[1,2,{\"k\":\"v\"}]}}";
        const r_ok = validateJsonObject(valid_json, &schema);
        expect_eq(r_ok.ok, true);
        expect_eq(r_ok.issue_count, 0);

        // 2. Missing required fields
        const missing_json = "{\"name\":\"Bob\",\"age\":25}";
        const r_miss = validateJsonObject(missing_json, &schema);
        expect_eq(r_miss.ok, false);
        expect_eq(r_miss.issue_count, 3);

        // 3. Too short / out of bounds / bad types / bad email, uuid, url, enum
        const bad_json = "{\"id\":\"not-a-uuid\",\"name\":\"A\",\"age\":15,\"active\":\"yes\",\"email\":\"bad@@host\",\"website\":\"ftp://nope\",\"role\":\"superroot\"}";
        const r_bad = validateJsonObject(bad_json, &schema);
        expect_eq(r_bad.ok, false);
        expect_eq(r_bad.issue_count, 7);

        // 4. Max length exceeded
        const long_name_json = "{\"id\":\"550e8400-e29b-41d4-a716-446655440000\",\"name\":\"012345678901234567890123456789012345\",\"age\":30,\"email\":\"a@b.co\",\"role\":\"user\"}";
        const r_long = validateJsonObject(long_name_json, &schema);
        expect_eq(r_long.ok, false);
        expect_eq(r_long.issue_count, 1);
        expect_eq(std.mem.eql(u8, r_long.issues[0].code, "max_len"), true);
    });

    test("4. validateJsonObject: 1000-iter deterministic malformed JSON Fuzz (0 panic)", () => {
        const schema = buildUserSchema();
        let seed: u64 = 0x51B54A32D192ED03;
        const alphabet = "{}[]\":, \t\n\\u0041truefalsenull-0123456789.eE@nameageidrole";
        let buf: [128]u8 = [0; 128];
        let total_checked: usize = 0;

        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const len: usize = @as(usize, @intCast(xorshift64(&seed) % 128));
            let j: usize = 0;
            while (j < len) : (j += 1) {
                const idx: usize = @as(usize, @intCast(xorshift64(&seed) % alphabet.len));
                buf[j] = alphabet[idx];
            }
            const rep = validateJsonObject(buf[0..len], &schema);
            if (!rep.ok) {
                expect_eq(rep.issue_count > 0, true);
            }
            total_checked += 1;
        }
        expect_eq(total_checked, 1000);
    });

    test("5. exactMirrorJson: strips sensitive undeclared fields and preserves valid schema fields", () => {
        let schema = ObjectSchema.init("PublicUser");
        * = schema.strField("name", true, 1, 64)
            .intField("age", true, 0, 150)
            .emailField("email", true)
            .enumField("role", true, "admin,user");

        const dirty_json = "{\"password_hash\":\"$2b$12$secret\",\"name\":\"Alice\",\"internal_role\":\"root\",\"age\":29,\"email\":\"alice@example.com\",\"role\":\"user\",\"session_token\":\"tok_123\"}";
        let out: [512]u8 = [0; 512];
        const n = exactMirrorJson(dirty_json, &schema, out[0..]) ?? 0;
        expect_eq(n > 0, true);

        const mirrored = out[0..n];
        expect_eq(std.mem.eql(u8, mirrored, "{\"name\":\"Alice\",\"age\":29,\"email\":\"alice@example.com\",\"role\":\"user\"}"), true);
        expect_eq(containsSubstr(mirrored, "password_hash"), false);
        expect_eq(containsSubstr(mirrored, "internal_role"), false);
        expect_eq(containsSubstr(mirrored, "session_token"), false);

        // Output itself must pass schema validation
        const rep = validateJsonObject(mirrored, &schema);
        expect_eq(rep.ok, true);

        // Malformed JSON returns null
        expect_eq(exactMirrorJson("{\"name\":\"Alice\"", &schema, out[0..]) == null, true);
    });

    test("6. schemaToOpenApiJson: emits OpenAPI 3.1.0 JSON Schema fragment", () => {
        const schema = buildUserSchema();
        let out: [1024]u8 = [0; 1024];
        const n = schemaToOpenApiJson(&schema, out[0..]) ?? 0;
        expect_eq(n > 0, true);

        const spec = out[0..n];
        expect_eq(containsSubstr(spec, "\"title\":\"UserCreate\""), true);
        expect_eq(containsSubstr(spec, "\"type\":\"object\""), true);
        expect_eq(containsSubstr(spec, "\"minLength\":2,\"maxLength\":32"), true);
        expect_eq(containsSubstr(spec, "\"type\":\"integer\",\"minimum\":18,\"maximum\":120"), true);
        expect_eq(containsSubstr(spec, "\"format\":\"email\""), true);
        expect_eq(containsSubstr(spec, "\"format\":\"uuid\""), true);
        expect_eq(containsSubstr(spec, "\"format\":\"uri\""), true);
        expect_eq(containsSubstr(spec, "\"enum\":[\"admin\",\"user\",\"guest\"]"), true);
        expect_eq(containsSubstr(spec, "\"required\":[\"name\",\"age\",\"email\",\"id\",\"role\"]"), true);
        expect_eq(containsSubstr(spec, "\"additionalProperties\":false"), true);
    });

    test("7. detectFileType: 30 binary & text formats + truncated/empty safety", () => {
        // Images
        const png = detectFileType("\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR").?;
        expect_eq(png.kind == FileKind.Png, true);
        expect_eq(std.mem.eql(u8, png.mime, "image/png"), true);

        const jpg = detectFileType("\xFF\xD8\xFF\xE0\x00\x10JFIF").?;
        expect_eq(jpg.kind == FileKind.Jpeg, true);
        expect_eq(std.mem.eql(u8, jpg.ext, "jpg"), true);

        const gif = detectFileType("GIF89a\x01\x00\x01\x00").?;
        expect_eq(gif.kind == FileKind.Gif, true);

        const webp = detectFileType("RIFF\x24\x00\x00\x00WEBPVP8 ").?;
        expect_eq(webp.kind == FileKind.Webp, true);

        const avif = detectFileType("\x00\x00\x00\x1cftypavif\x00\x00\x00\x00").?;
        expect_eq(avif.kind == FileKind.Avif, true);

        const bmp = detectFileType("BM\x36\x00\x00\x00\x00\x00\x00\x00\x36\x00\x00\x00").?;
        expect_eq(bmp.kind == FileKind.Bmp, true);

        const ico = detectFileType("\x00\x00\x01\x00\x01\x00\x10\x10").?;
        expect_eq(ico.kind == FileKind.Ico, true);

        // Documents & Archives
        expect_eq(detectFileType("%PDF-1.7\n").?.kind == FileKind.Pdf, true);
        expect_eq(detectFileType("PK\x03\x04\x14\x00\x00\x00").?.kind == FileKind.Zip, true);
        expect_eq(detectFileType("\x1F\x8B\x08\x00\x00\x00").?.kind == FileKind.Gzip, true);
        expect_eq(detectFileType("\x28\xB5\x2F\xFD\x00\x58").?.kind == FileKind.Zstd, true);
        expect_eq(detectFileType("\xFD7zXZ\x00\x00\x04").?.kind == FileKind.Xz, true);
        expect_eq(detectFileType("BZh91AY&SY").?.kind == FileKind.Bz2, true);
        expect_eq(detectFileType("\x04\x22\x4D\x18\x64\x40").?.kind == FileKind.Lz4, true);

        // Media
        expect_eq(detectFileType("\x00\x00\x00\x18ftypisom\x00\x00\x02\x00").?.kind == FileKind.Mp4, true);
        expect_eq(detectFileType("\x1A\x45\xDF\xA3\x9F\x42").?.kind == FileKind.Webm, true);
        expect_eq(detectFileType("ID3\x03\x00\x00\x00").?.kind == FileKind.Mp3, true);
        expect_eq(detectFileType("\xFF\xFB\x90\x00").?.kind == FileKind.Mp3, true);
        expect_eq(detectFileType("fLaC\x00\x00\x00\x22").?.kind == FileKind.Flac, true);
        expect_eq(detectFileType("OggS\x00\x02\x00\x00").?.kind == FileKind.Ogg, true);
        expect_eq(detectFileType("RIFF\x24\x00\x00\x00WAVEfmt ").?.kind == FileKind.Wav, true);

        // WASM, Fonts, SQLite3
        expect_eq(detectFileType("\x00asm\x01\x00\x00\x00").?.kind == FileKind.Wasm, true);
        expect_eq(detectFileType("wOF2\x00\x01\x00\x00").?.kind == FileKind.Woff2, true);
        expect_eq(detectFileType("wOFF\x00\x01\x00\x00").?.kind == FileKind.Woff, true);
        expect_eq(detectFileType("\x00\x01\x00\x00\x00\x0f\x00\x80").?.kind == FileKind.Ttf, true);
        expect_eq(detectFileType("OTTO\x00\x0c\x00\x80").?.kind == FileKind.Otf, true);
        expect_eq(detectFileType("SQLite format 3\x00\x10\x00").?.kind == FileKind.Sqlite3, true);

        // Text Heuristics (SVG, HTML, XML, JSON)
        expect_eq(detectFileType("  <svg xmlns=\"http://www.w3.org/2000/svg\"></svg>").?.kind == FileKind.Svg, true);
        expect_eq(detectFileType("<?xml version=\"1.0\"?><svg></svg>").?.kind == FileKind.Svg, true);
        expect_eq(detectFileType("<!DOCTYPE html><html><body>hi</body></html>").?.kind == FileKind.Html, true);
        expect_eq(detectFileType("<?xml version=\"1.0\"?><root/>").?.kind == FileKind.Xml, true);
        expect_eq(detectFileType("  {\"ok\":true} \n").?.kind == FileKind.Json, true);
        expect_eq(detectFileType("[1, 2, 3]").?.kind == FileKind.Json, true);

        // Truncated / Empty / Unknown safety
        expect_eq(detectFileType("") == null, true);
        expect_eq(detectFileType("A") == null, true);
        expect_eq(detectFileType("\x89PN") == null, true);
        expect_eq(detectFileType("RIFF\x00\x00") == null, true);
        expect_eq(detectFileType("SQLite format 3") == null, true);
        expect_eq(detectFileType("plain unknown text payload") == null, true);
    });

    test("8. ObjectSchema: recursive nested Object & Array validation", () => {
        let addr_schema = ObjectSchema.init("Address");
        * = addr_schema
            .strField("city", true, 2, 64)
            .intField("zip", true, 10000, 99999);

        let item_schema = ObjectSchema.init("OrderItem");
        * = item_schema
            .strField("sku", true, 2, 32)
            .intField("qty", true, 1, 1000);

        let order_schema = ObjectSchema.init("Order");
        * = order_schema
            .objectField("shipping", true, &addr_schema)
            .arrayField("tags", true, FieldKind.Str, 1, 5)
            .objectArrayField("items", true, &item_schema, 1, 10);

        const valid_json = "{\"shipping\":{\"city\":\"Tokyo\",\"zip\":10001},\"tags\":[\"vip\",\"fast\"],\"items\":[{\"sku\":\"GPU-H200\",\"qty\":4}]}";
        const ok_rep = validateJsonObject(valid_json, &order_schema);
        expect_eq(ok_rep.ok, true);

        const bad_nested = "{\"shipping\":{\"city\":\"X\",\"zip\":99},\"tags\":[],\"items\":[{\"sku\":\"OK\",\"qty\":0}]}";
        const bad_rep = validateJsonObject(bad_nested, &order_schema);
        expect_eq(bad_rep.ok, false);
        expect_eq(bad_rep.issue_count >= 4, true);

        let oa_buf: [2048]u8 = [0; 2048];
        const oa_len = schemaToOpenApiJson(&order_schema, oa_buf[0..]) ?? 0;
        expect_eq(oa_len > 0, true);
        expect_eq(containsSubstr(oa_buf[0..oa_len], "\"shipping\":{\"type\":\"object\"}"), true);
        expect_eq(containsSubstr(oa_buf[0..oa_len], "\"tags\":{\"type\":\"array\",\"minItems\":1,\"maxItems\":5}"), true);
    });
}
