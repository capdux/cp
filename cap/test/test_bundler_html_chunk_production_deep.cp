// cap/test/test_bundler_html_chunk_production_deep.cp
// 深度生产级回归测试：cap:bundler HTML Chunk 编译注入与 Standalone HTML 引擎 (lib/bundler/html_chunk.cp)

import { test } from "cap:test";
import {
    HTML_CHUNK_MAX_PRELOADS,
    HtmlChunkCompileOptions,
    findHtmlEndTagIndices,
    writeEscapedAttrUrl,
    writeEscapedInlineScript,
    writeEscapedInlineStyle,
    compileHtmlChunk,
} from "cap:bundler";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSlice(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEq(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

fn indexOfSlice(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return null;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEq(haystack[i..i + needle.len], needle)) return i;
        i += 1;
    }
    return null;
}

test("html_chunk: standard bundle mode injects CSS -> modulepreload -> JS before </head>") {
    const html = "<!doctype html><html><head><title>App</title></head><body><div id=\"root\"></div></body></html>";
    let opts = HtmlChunkCompileOptions{
        standalone_html: false,
        minify_whitespace: true,
        add_crossorigin: true,
        css_ref: "/assets/app-1234.css",
        js_ref: "/assets/app-5678.js",
    };
    opts.addModulePreload("/assets/chunk-shared.js")?;
    opts.addModulePreload("/assets/vendor.js")?;

    let out: [1024]u8 = [0; 1024];
    const res = compileHtmlChunk(html, opts, out[0..])?;
    const rendered = out[0..res.bytes_written];

    if (!res.head_injected || res.body_injected) return err.UnexpectedInjectionFlags;

    const css_pos = indexOfSlice(rendered, "<link rel=\"stylesheet\" crossorigin href=\"/assets/app-1234.css\">");
    const pre1_pos = indexOfSlice(rendered, "<link rel=\"modulepreload\" crossorigin href=\"/assets/chunk-shared.js\">");
    const pre2_pos = indexOfSlice(rendered, "<link rel=\"modulepreload\" crossorigin href=\"/assets/vendor.js\">");
    const js_pos = indexOfSlice(rendered, "<script type=\"module\" crossorigin src=\"/assets/app-5678.js\"></script>");
    const head_end = indexOfSlice(rendered, "</head>");

    if (css_pos == null || pre1_pos == null || pre2_pos == null || js_pos == null || head_end == null) {
        return err.MissingInjectedTag;
    }
    // 严格校验注入顺序：CSS < preload1 < preload2 < JS < </head>（防 FOUC 闪烁）
    if (!(css_pos.? < pre1_pos.? && pre1_pos.? < pre2_pos.? && pre2_pos.? < js_pos.? && js_pos.? < head_end.?)) {
        return err.WrongInjectionOrder;
    }
}

test("html_chunk: standalone HTML mode inlines <style> in <head> and <script type=module> before </body>") {
    const html = "<!doctype html><html><head><meta charset=\"utf-8\"></head><body><div id=\"root\"></div></body></html>";
    const opts = HtmlChunkCompileOptions{
        standalone_html: true,
        minify_whitespace: true,
        css_ref: "body{margin:0;background:#fff}",
        js_ref: "document.getElementById('root').textContent='hi';",
    };

    let out: [1024]u8 = [0; 1024];
    const res = compileHtmlChunk(html, opts, out[0..])?;
    const rendered = out[0..res.bytes_written];

    if (!res.head_injected || !res.body_injected) return err.StandaloneFlagsMismatch;

    const style_pos = indexOfSlice(rendered, "<style>body{margin:0;background:#fff}</style>");
    const head_end = indexOfSlice(rendered, "</head>");
    const div_pos = indexOfSlice(rendered, "<div id=\"root\"></div>");
    const script_pos = indexOfSlice(rendered, "<script type=\"module\">document.getElementById('root').textContent='hi';</script>");
    const body_end = indexOfSlice(rendered, "</body>");

    if (style_pos == null || head_end == null || div_pos == null || script_pos == null || body_end == null) {
        return err.StandaloneMissingTag;
    }
    // CSS 在 </head> 之前，DOM 节点在 <script> 之前，<script> 在 </body> 之前
    if (!(style_pos.? < head_end.? && head_end.? < div_pos.? && div_pos.? < script_pos.? && script_pos.? < body_end.?)) {
        return err.StandaloneOrderMismatch;
    }
}

test("html_chunk: comment & raw <script>/<style> body skipping in findHtmlEndTagIndices") {
    // 注释和内联脚本/样式中故意放入伪 </head> / </body> / </html>
    const html =
        "<html><head>" ++
        "<!-- </head> </body> </html> -->" ++
        "<script>const s = '</head></body></html>';</script>" ++
        "<style>/* </head></body> */</style>" ++
        "</head><body><p>ok</p></body></html>";

    const idx = findHtmlEndTagIndices(html);
    if (idx.head == null || idx.body == null || idx.html == null) {
        return err.EndTagsNotFound;
    }
    // 验证找到的是真实闭合标签，而不是注释或 script/style 内的伪标签
    if (!bytesEq(html[idx.head.?..idx.head.? + 7], "</head>")) return err.WrongHeadOffset;
    if (!bytesEq(html[idx.body.?..idx.body.? + 7], "</body>")) return err.WrongBodyOffset;
    if (!bytesEq(html[idx.html.?..idx.html.? + 7], "</html>")) return err.WrongHtmlOffset;
    if (!(idx.head.? < idx.body.? && idx.body.? < idx.html.?)) return err.EndTagOrderWrong;

    // 验证真实 </head> 位于 "</style>" 之后
    const style_close = indexOfSlice(html, "</style>").?;
    if (idx.head.? <= style_close) return err.MatchedFakeHeadInsideStyle;
}

test("html_chunk: fallback anchors when </head>, </body>, or </html> are missing") {
    // 1. 无 </head> 但有 </body>：外链模式回退至 </body> 前
    const html_no_head = "<html><body><h1>Hi</h1></body></html>";
    const opts_ext = HtmlChunkCompileOptions{
        standalone_html: false,
        minify_whitespace: true,
        add_crossorigin: false,
        js_ref: "/app.js",
    };
    let out: [512]u8 = [0; 512];
    const r1 = compileHtmlChunk(html_no_head, opts_ext, out[0..])?;
    const s1 = out[0..r1.bytes_written];
    if (!bytesEq(s1, "<html><body><h1>Hi</h1><script type=\"module\" src=\"/app.js\"></script></body></html>")) {
        return err.FallbackBodyFailed;
    }

    // 2. 纯片段无任何闭合标签：standalone 模式回退至文末（先 CSS 后 JS）
    const frag = "<div id=\"app\"></div>";
    const opts_sa = HtmlChunkCompileOptions{
        standalone_html: true,
        minify_whitespace: true,
        css_ref: "a{color:red}",
        js_ref: "init()",
    };
    const r2 = compileHtmlChunk(frag, opts_sa, out[0..])?;
    const s2 = out[0..r2.bytes_written];
    if (!bytesEq(s2, "<div id=\"app\"></div><style>a{color:red}</style><script type=\"module\">init()</script>")) {
        return err.FallbackFragmentFailed;
    }
}

test("html_chunk: inline </script> & </style> breakout defense and URL attribute escaping") {
    let out: [512]u8 = [0; 512];

    // 1. 内联 JS/CSS 中的大小写混合闭合标签必须被安全转义
    const opts_sa = HtmlChunkCompileOptions{
        standalone_html: true,
        minify_whitespace: true,
        css_ref: "/* </STYLE> */ body{}",
        js_ref: "const x = \"</ScRiPt><img src=x>\";",
    };
    const r_sa = compileHtmlChunk("<head></head><body></body>", opts_sa, out[0..])?;
    const s_sa = out[0..r_sa.bytes_written];
    if (containsSlice(s_sa, "</STYLE>") || containsSlice(s_sa, "</ScRiPt>")) {
        return err.BreakoutNotEscaped;
    }
    if (!containsSlice(s_sa, "<\\/STYLE>") || !containsSlice(s_sa, "<\\/ScRiPt>")) {
        return err.ExpectedEscapedClosingTag;
    }

    // 2. 属性 URL 特殊字符安全转义
    let attr_buf: [128]u8 = [0; 128];
    let pos: usize = 0;
    writeEscapedAttrUrl("/a?x=1&y=\"2\"<z>", attr_buf[0..], &pos)?;
    if (!bytesEq(attr_buf[0..pos], "/a?x=1&amp;y=&quot;2&quot;&lt;z&gt;")) {
        return err.AttrEscapeMismatch;
    }

    // 3. 独立辅助函数直接调用校验
    let js_pos: usize = 0;
    writeEscapedInlineScript("a</script>b", attr_buf[0..], &js_pos)?;
    if (!bytesEq(attr_buf[0..js_pos], "a<\\/script>b")) return err.DirectScriptEscapeFailed;

    let css_pos: usize = 0;
    writeEscapedInlineStyle("a</style>b", attr_buf[0..], &css_pos)?;
    if (!bytesEq(attr_buf[0..css_pos], "a<\\/style>b")) return err.DirectStyleEscapeFailed;
}

test("html_chunk: malformed tag ordering & passthrough round-trip when no assets provided") {
    // 1. 无任何 CSS/JS/preload 时，原 HTML 必须 100% 无损原样 round-trip
    const orig = "<!doctype html><HTML><HEAD></HEAD><BODY>hello</BODY></HTML>";
    const opts_noop = HtmlChunkCompileOptions{};
    let out: [256]u8 = [0; 256];
    const r_noop = compileHtmlChunk(orig, opts_noop, out[0..])?;
    if (r_noop.head_injected || r_noop.body_injected) return err.NoopShouldNotInject;
    if (!bytesEq(out[0..r_noop.bytes_written], orig)) return err.NoopRoundTripFailed;

    // 2. 畸形顺序（</body> 出现在 </head> 之前）：standalone 模式按物理偏移升序无损拼接不崩溃
    const weird = "<body>x</body><head>y</head>";
    const opts_sa = HtmlChunkCompileOptions{
        standalone_html: true,
        minify_whitespace: true,
        css_ref: "p{}",
        js_ref: "f()",
    };
    const r_weird = compileHtmlChunk(weird, opts_sa, out[0..])?;
    const s_weird = out[0..r_weird.bytes_written];
    if (!bytesEq(s_weird, "<body>x<script type=\"module\">f()</script></body><head>y<style>p{}</style></head>")) {
        return err.MalformedOrderHandlingFailed;
    }
}

test("html_chunk: boundary guards (InvalidAssetUrl, TooManyPreloads, BufferTooSmall)") {
    // 1. 空 URL 拒绝加入 modulepreload
    let opts = HtmlChunkCompileOptions{};
    let caught_empty = false;
    opts.addModulePreload("") catch {
        caught_empty = true;
    };
    if (!caught_empty) return err.ExpectedInvalidAssetUrl;

    // 2. 超过 HTML_CHUNK_MAX_PRELOADS 拒绝
    let i: usize = 0;
    while (i < HTML_CHUNK_MAX_PRELOADS) {
        opts.addModulePreload("/chunk.js")?;
        i += 1;
    }
    let caught_many = false;
    opts.addModulePreload("/overflow.js") catch {
        caught_many = true;
    };
    if (!caught_many) return err.ExpectedTooManyPreloads;

    // 3. 输出缓冲过小安全返回 BufferTooSmall
    let tiny: [16]u8 = [0; 16];
    const opts_small = HtmlChunkCompileOptions{
        standalone_html: false,
        css_ref: "/assets/app.css",
    };
    let caught_small = false;
    * = compileHtmlChunk("<html><head></head></html>", opts_small, tiny[0..]) catch {
        caught_small = true;
    };
    if (!caught_small) return err.ExpectedBufferTooSmall;
}
