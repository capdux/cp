// cap/test/testBundlerCssChunkEmit.cp — Deep production tests for cap:bundler cssChunkEmit
// Covers:
//   1. Safe CSS minifier preserving strings, `/*! ... */` legal comments & `calc(100% - 16px)` spaces
//   2. SIMD `url(...)` asset rewriter (relative paths, query/hash preservation, skipping data:/http:/#)
//   3. Spec-compliant CSS Chunk assembly: single hoisted `@charset "UTF-8";` & deduplicated `@import`
//   4. `@layer` ordering, module `(byte_offset, start_line, end_line)` span tracking & SourceMap footer
//   5. Auto `@charset "UTF-8";` detection on non-ASCII UTF-8 content via `Simd.isAscii`
//   6. Diagnostic summary report formatting (`formatCssChunkEmitReport`) & buffer-too-small handling
//   7. 500-iteration deterministic random CSS fragment Fuzz gate (minifier + URL rewriter)

import { expect, expectEqual, test } from "cap:test";
import {
    CSS_CHARSET_UTF8,
    CssMinifyOptions,
    minifyCssChunkSafe,
    CssUrlAssetMapping,
    CssUrlRewriteOptions,
    isExternalOrDataCssUrl,
    rewriteCssAssetUrls,
    CssChunkPartKind,
    CssChunkPartInput,
    CssChunkEmitOptions,
    CssChunkModuleSpan,
    assembleCssChunkResult,
    formatCssChunkEmitReport,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

fn indexOfSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return null;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return i;
        i += 1;
    }
    return null;
}

test("cssChunkEmit: safe CSS minifier preserves strings, legal comments & calc() spaces", () => {
    const src =
        "/*! MIT License */\n" ++
        "/* regular comment to strip */\n" ++
        ".card  >  .title {\n" ++
        "  content: \"hello   /* not a comment */   world\";\n" ++
        "  width: calc( 100% - 16px + 2rem );\n" ++
        "  margin: 0  auto;\n" ++
        "}\n";

    let out: [512]u8 = [0; 512];
    const st = minifyCssChunkSafe(src, CssMinifyOptions.defaults(), out[0..]) catch unreachable;
    const min = out[0..st.output_bytes];

    expectEqual(st.legal_comments_kept, @as(u32, 1));
    expectEqual(st.comments_stripped, @as(u32, 1));
    expect(st.bytes_saved > 20);
    expect(containsSubstr(min, "/*! MIT License */"));
    expect(!containsSubstr(min, "regular comment"));
    expect(containsSubstr(min, "\"hello   /* not a comment */   world\""));
    expect(containsSubstr(min, "calc(100% - 16px + 2rem)"));
    expect(containsSubstr(min, ".card>.title{"));
    // Trailing semicolon before '}' stripped
    expect(containsSubstr(min, "margin:0 auto}"));
});

test("cssChunkEmit: SIMD url(...) rewriter skips external/data/# and preserves query/hash", () => {
    expect(isExternalOrDataCssUrl("data:image/png;base64,AAA"));
    expect(isExternalOrDataCssUrl("https://cdn.example.com/a.png"));
    expect(isExternalOrDataCssUrl("HTTP://example.com/b.png"));
    expect(isExternalOrDataCssUrl("//cdn.example.com/c.png"));
    expect(isExternalOrDataCssUrl("#clip-path-id"));
    expect(!isExternalOrDataCssUrl("./images/logo.svg"));

    const css =
        "/* comment url(./logo.png) */\n" ++
        ".a { background: url(./logo.png); }\n" ++
        ".b { src: URL('fonts/inter.woff2?v=2#iefix'); }\n" ++
        ".c { background: url(data:image/svg+xml;utf8,<svg/>); }\n" ++
        ".d { background: url(https://cdn.example.com/bg.jpg); }\n" ++
        ".e { clip-path: url(#mask1); }\n" ++
        ".f { background: url(unmapped/pic.webp); }\n";

    const maps: [2]CssUrlAssetMapping = [
        CssUrlAssetMapping{ original_rel_path: "logo.png", hashed_asset_path: "assets/logo-11223344.png" },
        CssUrlAssetMapping{ original_rel_path: "./fonts/inter.woff2", hashed_asset_path: "assets/inter-deadbeef.woff2" },
    ];

    let out: [1024]u8 = [0; 1024];
    const rw = rewriteCssAssetUrls(
        css,
        maps[0..],
        CssUrlRewriteOptions{ public_path: "/static/", quote_rewritten: true },
        out[0..],
    ) catch unreachable;
    const rewritten = out[0..rw.output_bytes];

    expectEqual(rw.urls_scanned, @as(u32, 6));
    expectEqual(rw.urls_rewritten, @as(u32, 2));
    expectEqual(rw.urls_skipped_external, @as(u32, 3));
    expectEqual(rw.urls_unmatched, @as(u32, 1));
    // Comment untouched
    expect(containsSubstr(rewritten, "/* comment url(./logo.png) */"));
    expect(containsSubstr(rewritten, "url(\"/static/assets/logo-11223344.png\")"));
    expect(containsSubstr(rewritten, "url(\"/static/assets/inter-deadbeef.woff2?v=2#iefix\")"));
    expect(containsSubstr(rewritten, "url(data:image/svg+xml;utf8,<svg/>)"));
    expect(containsSubstr(rewritten, "url(#mask1)"));
    expect(containsSubstr(rewritten, "url(unmapped/pic.webp)"));
});

test("cssChunkEmit: spec-compliant @charset hoisting & @import deduplication", () => {
    const parts: [5]CssChunkPartInput = [
        CssChunkPartInput{ kind: CssChunkPartKind.SourceModule, source_index: 1, code: "@charset \"UTF-8\";\n.a { color: red; }" },
        CssChunkPartInput{ kind: CssChunkPartKind.ExternalImport, code: "@import url(\"https://cdn.example.com/reset.css\");" },
        CssChunkPartInput{ kind: CssChunkPartKind.ExternalImport, code: "@import url(\"https://cdn.example.com/reset.css\");" }, // duplicate
        CssChunkPartInput{ kind: CssChunkPartKind.ExternalImport, code: "@import url(\"https://cdn.example.com/theme.css\") layer(theme);" },
        CssChunkPartInput{ kind: CssChunkPartKind.SourceModule, source_index: 2, code: "@charset \"UTF-8\";\n.b { color: blue; }" },
    ];

    const no_maps: [0]CssUrlAssetMapping = [];
    let scratch: [1024]u8 = [0; 1024];
    let out: [2048]u8 = [0; 2048];
    let spans: [4]CssChunkModuleSpan = [CssChunkModuleSpan.empty(); 4];

    const sum = assembleCssChunkResult(
        parts[0..],
        no_maps[0..],
        CssChunkEmitOptions{ banner: "/*! cap-bundle */", source_map_url: "bundle.css.map" },
        scratch[0..],
        out[0..],
        spans[0..],
    ) catch unreachable;
    const res = out[0..sum.output_bytes];

    expect(sum.emitted_charset);
    expectEqual(sum.hoisted_imports, @as(u32, 2)); // deduped from 3 -> 2
    expectEqual(sum.source_modules, @as(u32, 2));
    expectEqual(sum.spans_written, @as(usize, 2));

    // Verify CSS Syntax L3 ordering: @charset at byte 0 < @import < banner < .a < .b < sourceMappingURL
    const pos_charset = indexOfSubstr(res, CSS_CHARSET_UTF8) ?? 999;
    const pos_imp1 = indexOfSubstr(res, "@import url(\"https://cdn.example.com/reset.css\");") ?? 999;
    const pos_imp2 = indexOfSubstr(res, "@import url(\"https://cdn.example.com/theme.css\") layer(theme);") ?? 999;
    const pos_banner = indexOfSubstr(res, "/*! cap-bundle */") ?? 999;
    const pos_a = indexOfSubstr(res, ".a { color: red; }") ?? 999;
    const pos_sm = indexOfSubstr(res, "/*# sourceMappingURL=bundle.css.map */") ?? 999;

    expectEqual(pos_charset, @as(usize, 0));
    expect(pos_charset < pos_imp1);
    expect(pos_imp1 < pos_imp2);
    expect(pos_imp2 < pos_banner);
    expect(pos_banner < pos_a);
    expect(pos_a < pos_sm);
    // Ensure @charset appears only once
    expect(indexOfSubstr(res[CSS_CHARSET_UTF8.len..], CSS_CHARSET_UTF8) == null);
});

test("cssChunkEmit: @layer order, module spans & combined URL rewrite + minification", () => {
    const parts: [3]CssChunkPartInput = [
        CssChunkPartInput{ kind: CssChunkPartKind.LayerDecl, code: "@layer reset, components;" },
        CssChunkPartInput{ kind: CssChunkPartKind.SourceModule, source_index: 10, code: "@layer reset { body { margin: 0; } }" },
        CssChunkPartInput{ kind: CssChunkPartKind.SourceModule, source_index: 20, code: "@layer components { .icon { background: url(./icon.svg); width: calc(10px + 2px); } }" },
    ];
    const maps: [1]CssUrlAssetMapping = [
        CssUrlAssetMapping{ original_rel_path: "icon.svg", hashed_asset_path: "img/icon-abc123.svg" },
    ];

    let scratch: [1024]u8 = [0; 1024];
    let out: [2048]u8 = [0; 2048];
    let spans: [4]CssChunkModuleSpan = [CssChunkModuleSpan.empty(); 4];

    const sum = assembleCssChunkResult(
        parts[0..],
        maps[0..],
        CssChunkEmitOptions{ minify: true, public_path: "https://cdn.cap.dev/" },
        scratch[0..],
        out[0..],
        spans[0..],
    ) catch unreachable;
    const res = out[0..sum.output_bytes];

    expect(!sum.emitted_charset);
    expectEqual(sum.layer_decls, @as(u32, 1));
    expectEqual(sum.source_modules, @as(u32, 2));
    expectEqual(sum.urls_rewritten, @as(u32, 1));
    expect(containsSubstr(res, "@layer reset,components;"));
    expect(containsSubstr(res, "url(\"https://cdn.cap.dev/img/icon-abc123.svg\")"));
    expect(containsSubstr(res, "calc(10px + 2px)"));
    expectEqual(spans[0].source_index, @as(u32, 10));
    expectEqual(spans[1].source_index, @as(u32, 20));
    expect(spans[0].byte_len > 0);
    expect(spans[1].byte_offset >= spans[0].byte_offset + spans[0].byte_len);
});

test("cssChunkEmit: auto @charset UTF-8 emission when non-ASCII UTF-8 bytes detected", () => {
    const parts: [1]CssChunkPartInput = [
        CssChunkPartInput{ kind: CssChunkPartKind.SourceModule, source_index: 1, code: ".cn::after { content: \"\xe4\xb8\xad\xe6\x96\x87\"; }" },
    ];
    const no_maps: [0]CssUrlAssetMapping = [];
    let scratch: [256]u8 = [0; 256];
    let out: [512]u8 = [0; 512];
    let spans: [2]CssChunkModuleSpan = [CssChunkModuleSpan.empty(); 2];

    const sum = assembleCssChunkResult(parts[0..], no_maps[0..], CssChunkEmitOptions.defaults(), scratch[0..], out[0..], spans[0..]) catch unreachable;
    expect(sum.emitted_charset);
    expectEqual(indexOfSubstr(out[0..sum.output_bytes], CSS_CHARSET_UTF8) ?? 999, @as(usize, 0));
});

test("cssChunkEmit: diagnostic report formatting & BufferTooSmall safety", () => {
    const parts: [1]CssChunkPartInput = [
        CssChunkPartInput{ kind: CssChunkPartKind.SourceModule, source_index: 1, code: "body { color: #000; }" },
    ];
    const no_maps: [0]CssUrlAssetMapping = [];
    let scratch: [128]u8 = [0; 128];
    let out: [256]u8 = [0; 256];
    let spans: [2]CssChunkModuleSpan = [CssChunkModuleSpan.empty(); 2];

    const sum = assembleCssChunkResult(parts[0..], no_maps[0..], CssChunkEmitOptions.defaults(), scratch[0..], out[0..], spans[0..]) catch unreachable;
    let rep: [256]u8 = [0; 256];
    const rn = formatCssChunkEmitReport(sum, rep[0..]) catch unreachable;
    expect(containsSubstr(rep[0..rn], "CssChunkEmit(bytes="));
    expect(containsSubstr(rep[0..rn], "modules=1"));

    // Tiny output buffer returns BufferTooSmall cleanly
    let tiny: [4]u8 = [0; 4];
    let too_small = false;
    * = minifyCssChunkSafe("body { color: black; }", CssMinifyOptions.defaults(), tiny[0..]) catch {
        too_small = true;
    };
    expect(too_small);
});

test("cssChunkEmit: 500-iteration deterministic random CSS fragment Fuzz gate", () => {
    const alphabet = "abc0123 \t\n\r/*\"'():;{},>~+-?!@#urlcalc()";
    const maps: [1]CssUrlAssetMapping = [
        CssUrlAssetMapping{ original_rel_path: "a.png", hashed_asset_path: "h/a-1234.png" },
    ];

    let fuzz_in: [128]u8 = [0; 128];
    let rw_out: [512]u8 = [0; 512];
    let min_out: [512]u8 = [0; 512];

    let rng: u64 = 0xC55F022123456789;
    let completed: usize = 0;
    let iter: usize = 0;

    while (iter < 500) {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const len = 1 + @as(usize, @intCast((rng *% 0x2545F4914F6CDD1D) % 120));

        let k: usize = 0;
        while (k < len) {
            rng ^= rng >> 12;
            rng ^= rng << 25;
            rng ^= rng >> 27;
            const idx = @as(usize, @intCast(rng % @as(u64, alphabet.len)));
            fuzz_in[k] = alphabet[idx];
            k += 1;
        }

        // Inject tricky edge sequences periodically (unclosed comments, unclosed strings, unclosed url(), calc())
        if (iter % 4 == 0 && len >= 6) {
            fuzz_in[0] = 'u';
            fuzz_in[1] = 'r';
            fuzz_in[2] = 'l';
            fuzz_in[3] = '(';
        } else if (iter % 4 == 1 && len >= 6) {
            fuzz_in[0] = '/';
            fuzz_in[1] = '*';
            fuzz_in[2] = '!';
        }

        const rw = rewriteCssAssetUrls(fuzz_in[0..len], maps[0..], CssUrlRewriteOptions.defaults(), rw_out[0..]) catch unreachable;
        expect(rw.output_bytes <= rw_out.len);

        const mn = minifyCssChunkSafe(rw_out[0..rw.output_bytes], CssMinifyOptions.defaults(), min_out[0..]) catch unreachable;
        expect(mn.output_bytes <= min_out.len);

        completed += 1;
        iter += 1;
    }

    expectEqual(completed, @as(usize, 500));
});
