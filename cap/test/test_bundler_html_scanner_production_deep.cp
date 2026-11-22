// cap/test/test_bundler_html_scanner_production_deep.cp
// Production-deep regression tests for cap:bundler html_scanner module
// Source: /data/dev/bun/src/bundler/HTMLScanner.cp & /data/dev/bun/docs/bundler/html-static.mdx

import { test, expect } from "cap:test";
import {
    HTML_SCANNER_MAX_ASSETS,
    HTML_SCANNER_MAX_SRCSET_CANDIDATES,
    HtmlScannerError,
    HtmlImportKind,
    HtmlTagKind,
    SrcsetCandidate,
    HtmlRewritePair,
    isExternalHtmlUrl,
    parseSrcsetCandidates,
    scanHtmlAssets,
    rewriteHtmlAssetUrls,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEq(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

test("bundler html_scanner: full 16-handler coverage (script, link, img, video, audio, source)") {
    const html =
        "<!doctype html>" ++
        "<html><head>" ++
        "<link rel=\"stylesheet\" href=\"./styles.css\" />" ++
        "<link rel=\"preload\" as=\"font\" href=\"./inter.woff2\" />" ++
        "<link rel=\"preload\" as=\"worker\" href=\"./worker.ts\" />" ++
        "<link rel=\"icon\" href=\"./favicon.ico\" />" ++
        "<link rel=\"manifest\" href=\"./site.webmanifest\" />" ++
        "<script type=\"module\" src=\"./app.tsx\"></script>" ++
        "</head><body>" ++
        "<img src=\"./logo.png\" />" ++
        "<video src=\"./demo.mp4\" poster=\"./thumb.jpg\"><source src=\"./demo.webm\" /></video>" ++
        "<audio src=\"./intro.mp3\"></audio>" ++
        "</body></html>";

    const res = scanHtmlAssets(html) catch {
        expect(false);
        return;
    };
    expect(res.has_html);
    expect(res.has_head);
    expect(res.count == 11);
    expect(res.localAssetCount() == 11);
    expect(res.script_count == 2);     // worker.ts + app.tsx
    expect(res.stylesheet_count == 1); // styles.css
    expect(res.media_count == 8);      // font, icon, manifest, img, video src, video poster, source, audio

    if (a0 := res.assetAt(0)) {
        expect(a0.tag == HtmlTagKind.link);
        expect(a0.kind == HtmlImportKind.at);
        expect(strEq(a0.url, "./styles.css"));
    } else {
        expect(false);
    }

    if (a5 := res.assetAt(5)) {
        expect(a5.tag == HtmlTagKind.script);
        expect(a5.kind == HtmlImportKind.stmt);
        expect(a5.is_module_script);
        expect(strEq(a5.url, "./app.tsx"));
    } else {
        expect(false);
    }
}

test("bundler html_scanner: WHATWG srcset parsing with descriptors & data URI commas") {
    let cands: [8]SrcsetCandidate = [SrcsetCandidate{}; 8];
    const srcset = " ./img-1x.png 1x , ./img-2x.png 2x, data:image/png;base64,iVBORw0KGgo= 3x, ./hero-800.webp 800w ";
    const n = parseSrcsetCandidates(srcset, cands[0..]) catch {
        expect(false);
        return;
    };
    expect(n == 4);
    expect(strEq(cands[0].url, "./img-1x.png"));
    expect(strEq(cands[0].descriptor, "1x"));
    expect(strEq(cands[1].url, "./img-2x.png"));
    expect(strEq(cands[1].descriptor, "2x"));
    expect(strEq(cands[2].url, "data:image/png;base64,iVBORw0KGgo="));
    expect(strEq(cands[2].descriptor, "3x"));
    expect(strEq(cands[3].url, "./hero-800.webp"));
    expect(strEq(cands[3].descriptor, "800w"));

    // Capacity overflow check
    let small: [2]SrcsetCandidate = [SrcsetCandidate{}; 2];
    let overflow = false;
    * = parseSrcsetCandidates(srcset, small[0..]) catch |e| {
        expect(e == HtmlScannerError.TooManySrcsetCandidates);
        overflow = true;
    };
    expect(overflow);
}

test("bundler html_scanner: external URI classification & comment/raw-script isolation") {
    expect(isExternalHtmlUrl("https://cdn.example.com/a.js"));
    expect(isExternalHtmlUrl("HTTP://example.com/a.css"));
    expect(isExternalHtmlUrl("//cdn.example.com/x.png"));
    expect(isExternalHtmlUrl("data:image/svg+xml,<svg/>"));
    expect(isExternalHtmlUrl("blob:https://example.com/1"));
    expect(isExternalHtmlUrl("#section-1"));
    expect(isExternalHtmlUrl("   "));
    expect(!isExternalHtmlUrl("./local.js"));
    expect(!isExternalHtmlUrl("/assets/logo.svg"));
    expect(!isExternalHtmlUrl("images/banner.png"));

    // Comments and inline <script>/<style> bodies must NOT produce false asset hits
    const html =
        "<!-- <script src=\"./commented.js\"></script> <img src=\"./ghost.png\"> -->" ++
        "<script>const s = '<img src=\"./inside-js.png\">';</script>" ++
        "<style>/* <link rel=\"stylesheet\" href=\"./inside-css.css\"> */</style>" ++
        "<img src=\"https://cdn.example.com/ext.png\" />" ++
        "<img src=\"./real.png\" />";

    const res = scanHtmlAssets(html) catch {
        expect(false);
        return;
    };
    expect(res.count == 2);
    expect(res.localAssetCount() == 1);
    if (a0 := res.assetAt(0)) {
        expect(a0.is_external);
        expect(!a0.isLocalBundleable());
        expect(strEq(a0.url, "https://cdn.example.com/ext.png"));
    } else {
        expect(false);
    }
    if (a1 := res.assetAt(1)) {
        expect(!a1.is_external);
        expect(a1.isLocalBundleable());
        expect(strEq(a1.url, "./real.png"));
    } else {
        expect(false);
    }
}

test("bundler html_scanner: exact-span URL rewrite round-trip including srcset") {
    const html =
        "<html><head>" ++
        "<link rel=\"stylesheet\" href=\"./styles.css\">" ++
        "<script type=\"module\" src=\"./app.ts\"></script>" ++
        "</head><body>" ++
        "<!-- keep ./app.ts untouched in comments -->" ++
        "<img srcset=\"./pic-1x.png 1x, ./pic-2x.png 2x\" src=\"./pic-1x.png\">" ++
        "</body></html>";

    const pairs: [4]HtmlRewritePair = [
        HtmlRewritePair{ from_url: "./styles.css", to_url: "/chunk-css-1111.css" },
        HtmlRewritePair{ from_url: "./app.ts", to_url: "/chunk-app-2222.js" },
        HtmlRewritePair{ from_url: "./pic-1x.png", to_url: "/assets/pic-1x-aaa.png" },
        HtmlRewritePair{ from_url: "./pic-2x.png", to_url: "/assets/pic-2x-bbb.png" },
    ];

    let out: [512]u8 = [0; 512];
    const n = rewriteHtmlAssetUrls(html, pairs[0..], out[0..]) catch {
        expect(false);
        return;
    };
    const rewritten = out[0..n];
    expect(containsStr(rewritten, "href=\"/chunk-css-1111.css\""));
    expect(containsStr(rewritten, "src=\"/chunk-app-2222.js\""));
    expect(containsStr(rewritten, "srcset=\"/assets/pic-1x-aaa.png 1x, /assets/pic-2x-bbb.png 2x\""));
    expect(containsStr(rewritten, "src=\"/assets/pic-1x-aaa.png\""));
    // Comment must remain untouched!
    expect(containsStr(rewritten, "<!-- keep ./app.ts untouched in comments -->"));

    // Round-trip re-scan of rewritten HTML must observe the new hashed URLs
    const res2 = scanHtmlAssets(rewritten) catch {
        expect(false);
        return;
    };
    expect(res2.count == 5);
    if (r0 := res2.assetAt(0)) { expect(strEq(r0.url, "/chunk-css-1111.css")); } else { expect(false); }
    if (r1 := res2.assetAt(1)) { expect(strEq(r1.url, "/chunk-app-2222.js")); } else { expect(false); }
}

test("bundler html_scanner: case-insensitive tags/attrs, single & unquoted values, link type^='font/'") {
    const html =
        "<LINK TYPE='font/woff2' HREF=./font.woff2>" ++
        "<SCRIPT SRC='./main.js'></SCRIPT>" ++
        "<SOURCE SRCSET='./v1.avif 1x, ./v2.avif 2x'>";

    const res = scanHtmlAssets(html) catch {
        expect(false);
        return;
    };
    expect(res.count == 4);
    if (a0 := res.assetAt(0)) {
        expect(a0.kind == HtmlImportKind.url);
        expect(strEq(a0.url, "./font.woff2"));
    } else {
        expect(false);
    }
    if (a1 := res.assetAt(1)) {
        expect(a1.kind == HtmlImportKind.stmt);
        expect(strEq(a1.url, "./main.js"));
    } else {
        expect(false);
    }
    if (a3 := res.assetAt(3)) {
        expect(a3.tag == HtmlTagKind.source);
        expect(strEq(a3.url, "./v2.avif"));
    } else {
        expect(false);
    }
}

test("bundler html_scanner: malformed/truncated HTML & BufferTooSmall defenses") {
    // Truncated comment, unclosed quote, unclosed tag must never panic or loop infinitely
    const malformed1 = "<!-- unterminated comment <img src=\"x.png\">";
    const r1 = scanHtmlAssets(malformed1) catch { expect(false); return; };
    expect(r1.count == 0);

    const malformed2 = "<img src=\"./unclosed.png";
    const r2 = scanHtmlAssets(malformed2) catch { expect(false); return; };
    expect(r2.count == 1);
    if (a0 := r2.assetAt(0)) { expect(strEq(a0.url, "./unclosed.png")); } else { expect(false); }

    const malformed3 = "<script>unterminated script body";
    const r3 = scanHtmlAssets(malformed3) catch { expect(false); return; };
    expect(r3.count == 0);

    // BufferTooSmall protection on rewriteHtmlAssetUrls
    const pairs: [1]HtmlRewritePair = [
        HtmlRewritePair{ from_url: "./a.png", to_url: "/very/long/hashed/asset/path/a-12345678.png" },
    ];
    let tiny: [12]u8 = [0; 12];
    let got_small = false;
    * = rewriteHtmlAssetUrls("<img src=\"./a.png\">", pairs[0..], tiny[0..]) catch |e| {
        expect(e == HtmlScannerError.BufferTooSmall);
        got_small = true;
    };
    expect(got_small);

    // Constants sanity
    expect(HTML_SCANNER_MAX_ASSETS == 64);
    expect(HTML_SCANNER_MAX_SRCSET_CANDIDATES == 16);
}
