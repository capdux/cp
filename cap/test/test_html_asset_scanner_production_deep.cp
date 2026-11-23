// Source: cp/cap/test/test_html_asset_scanner_production_deep.cp (Production deep tests for cap:html_asset_scanner)

import { test, expect_eq } from "cap:test";
import {
    AssetKind,
    AssetRef,
    SrcsetCandidate,
    parseSrcset,
    HtmlAssetScanner,
} from "cap:html";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

test("html_asset_scanner: full document mixed asset types and link classification") {
    const html =
        "<!DOCTYPE html><html><head>" ++
        "<link rel=\"stylesheet\" href=\"/css/app.css\">" ++
        "<link rel=\"modulepreload\" href=\"/js/chunk.js\">" ++
        "<link rel=\"preload\" as=\"font\" href=\"/fonts/inter.woff2\" crossorigin>" ++
        "<link rel=\"preload\" as=\"worker\" href=\"/sw.js\">" ++
        "<link rel=\"shortcut icon\" href=\"/favicon.ico\">" ++
        "<link rel=\"manifest\" href=\"/site.webmanifest\">" ++
        "<script type=\"module\" src=\"/js/main.js\"></script>" ++
        "</head><body>" ++
        "<img src=\"/img/logo.png\" srcset=\"/img/logo-1x.png 1x, /img/logo-2x.png 2x\">" ++
        "<video src=\"/media/demo.mp4\" poster=\"/img/poster.jpg\"></video>" ++
        "<audio src=\"/media/bgm.ogg\"></audio>" ++
        "<picture><source srcset=\"/img/hero.avif 800w\"><source src=\"/media/alt.webm\"></picture>" ++
        "</body></html>";

    let refs: [24]AssetRef = [AssetRef{}; 24];
    const n = HtmlAssetScanner.scan(html, refs[0..]);
    expect_eq(n, 14);

    expect_eq(refs[0].kind == .stylesheet, true);
    expect_eq(bytesEq(refs[0].url, "/css/app.css"), true);
    expect_eq(bytesEq(refs[0].kind.asStr(), "stylesheet"), true);

    expect_eq(refs[1].kind == .script, true);
    expect_eq(bytesEq(refs[1].url, "/js/chunk.js"), true);

    expect_eq(refs[2].kind == .font, true);
    expect_eq(bytesEq(refs[2].url, "/fonts/inter.woff2"), true);

    expect_eq(refs[3].kind == .worker, true);
    expect_eq(bytesEq(refs[3].url, "/sw.js"), true);

    expect_eq(refs[4].kind == .icon, true);
    expect_eq(bytesEq(refs[4].url, "/favicon.ico"), true);

    expect_eq(refs[5].kind == .manifest, true);
    expect_eq(bytesEq(refs[5].url, "/site.webmanifest"), true);

    expect_eq(refs[6].kind == .script, true);
    expect_eq(bytesEq(refs[6].url, "/js/main.js"), true);

    expect_eq(refs[7].kind == .image, true);
    expect_eq(bytesEq(refs[7].url, "/img/logo.png"), true);
    expect_eq(bytesEq(refs[7].attr, "src"), true);

    expect_eq(refs[8].kind == .image, true);
    expect_eq(bytesEq(refs[8].attr, "srcset"), true);

    expect_eq(refs[9].kind == .media, true);
    expect_eq(bytesEq(refs[9].url, "/media/demo.mp4"), true);

    expect_eq(refs[10].kind == .image, true);
    expect_eq(bytesEq(refs[10].url, "/img/poster.jpg"), true);
    expect_eq(bytesEq(refs[10].attr, "poster"), true);

    expect_eq(refs[11].kind == .media, true);
    expect_eq(bytesEq(refs[11].url, "/media/bgm.ogg"), true);

    expect_eq(refs[12].kind == .image, true);
    expect_eq(bytesEq(refs[12].url, "/img/hero.avif 800w"), true);

    expect_eq(refs[13].kind == .media, true);
    expect_eq(bytesEq(refs[13].url, "/media/alt.webm"), true);
}

test("html_asset_scanner: exact byte offsets for in-place bundler URL rewriting") {
    const html = "<SCRIPT SRC='/a.js'></SCRIPT><IMG src=unquoted.png /><link rel='apple-touch-icon' href=\"/touch.png\">";
    let refs: [8]AssetRef = [AssetRef{}; 8];
    const n = HtmlAssetScanner.scan(html, refs[0..]);
    expect_eq(n, 3);

    let i: usize = 0;
    while (i < n) {
        const r = refs[i];
        expect_eq(r.end_offset > r.start_offset, true);
        expect_eq(r.end_offset <= html.len, true);
        const sliced = html[r.start_offset..r.end_offset];
        expect_eq(bytesEq(sliced, r.url), true);
        i += 1;
    }

    expect_eq(bytesEq(refs[0].url, "/a.js"), true);
    expect_eq(bytesEq(refs[1].url, "unquoted.png"), true);
    expect_eq(bytesEq(refs[2].url, "/touch.png"), true);
    expect_eq(refs[2].kind == .icon, true);
}

test("html_asset_scanner: skips HTML comments and raw text inside script and style blocks") {
    const html =
        "<!-- <img src=\"/commented.png\"><script src=\"/commented.js\"></script> -->" ++
        "<script>const tpl = '<img src=\"/fake-in-js.png\"><link rel=\"stylesheet\" href=\"/fake.css\">';</script>" ++
        "<style>/* <img src=\"/fake-in-css.png\"> */ body { color: red; }</style>" ++
        "<img src=\"/real.png\">";

    let refs: [8]AssetRef = [AssetRef{}; 8];
    const n = HtmlAssetScanner.scan(html, refs[0..]);
    expect_eq(n, 1);
    expect_eq(refs[0].kind == .image, true);
    expect_eq(bytesEq(refs[0].url, "/real.png"), true);
    expect_eq(bytesEq(html[refs[0].start_offset..refs[0].end_offset], "/real.png"), true);
}

test("html_asset_scanner: parseSrcset multi-candidate, CDN commas, descriptors and truncation") {
    const srcset = "  /img/small.jpg 480w, https://cdn.example/w_800,q_80/hero.jpg 800w , /img/retina.jpg 2x, /img/default.jpg  ";
    let cands: [8]SrcsetCandidate = [SrcsetCandidate{}; 8];
    const n = parseSrcset(srcset, cands[0..]);
    expect_eq(n, 4);

    expect_eq(bytesEq(cands[0].url, "/img/small.jpg"), true);
    expect_eq(bytesEq(cands[0].descriptor, "480w"), true);

    expect_eq(bytesEq(cands[1].url, "https://cdn.example/w_800,q_80/hero.jpg"), true);
    expect_eq(bytesEq(cands[1].descriptor, "800w"), true);

    expect_eq(bytesEq(cands[2].url, "/img/retina.jpg"), true);
    expect_eq(bytesEq(cands[2].descriptor, "2x"), true);

    expect_eq(bytesEq(cands[3].url, "/img/default.jpg"), true);
    expect_eq(bytesEq(cands[3].descriptor, ""), true);

    // 小容量输出缓冲区截断保护
    let small_out: [2]SrcsetCandidate = [SrcsetCandidate{}; 2];
    const m = parseSrcset(srcset, small_out[0..]);
    expect_eq(m, 2);
    expect_eq(bytesEq(small_out[0].url, "/img/small.jpg"), true);
    expect_eq(bytesEq(small_out[1].url, "https://cdn.example/w_800,q_80/hero.jpg"), true);
}

test("html_asset_scanner: link preload as image, media and font mime type") {
    const html =
        "<link rel=\"preload\" as=\"image\" href=\"/banner.webp\">" ++
        "<link rel=\"preload\" as=\"video\" href=\"/intro.mp4\">" ++
        "<link rel=\"preload\" as=\"audio\" href=\"/ping.mp3\">" ++
        "<link rel=\"preload\" type=\"font/woff2\" href=\"/mono.woff2\">" ++
        "<link rel=\"canonical\" href=\"https://example.com/\">";

    let refs: [8]AssetRef = [AssetRef{}; 8];
    const n = HtmlAssetScanner.scan(html, refs[0..]);
    expect_eq(n, 4);
    expect_eq(refs[0].kind == .image, true);
    expect_eq(refs[1].kind == .media, true);
    expect_eq(refs[2].kind == .media, true);
    expect_eq(refs[3].kind == .font, true);
}

test("html_asset_scanner: malformed tags, unclosed comments/quotes and capacity guard") {
    // 1. 未闭合注释不应崩溃
    const unclosed_comment = "<img src=\"/ok.png\"><!-- unclosed <img src=\"/ignored.png\">";
    let refs1: [4]AssetRef = [AssetRef{}; 4];
    const n1 = HtmlAssetScanner.scan(unclosed_comment, refs1[0..]);
    expect_eq(n1, 1);
    expect_eq(bytesEq(refs1[0].url, "/ok.png"), true);

    // 2. 空属性值与无引号/未闭合引号畸形标签
    const malformed = "<img src=\"\"><script src=\"/unterminated.js";
    let refs2: [4]AssetRef = [AssetRef{}; 4];
    const n2 = HtmlAssetScanner.scan(malformed, refs2[0..]);
    expect_eq(n2, 1);
    expect_eq(refs2[0].kind == .script, true);
    expect_eq(bytesEq(refs2[0].url, "/unterminated.js"), true);

    // 3. 容量上限保护
    let cap_out: [1]AssetRef = [AssetRef{}; 1];
    const multi = "<img src=\"/1.png\"><img src=\"/2.png\"><img src=\"/3.png\">";
    const n3 = HtmlAssetScanner.scan(multi, cap_out[0..]);
    expect_eq(n3, 1);
    expect_eq(bytesEq(cap_out[0].url, "/1.png"), true);
}
