// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/generateCompileResultForHtmlChunk.cp | Synced: 2026-11-22T09:10:00Z
// Ported from Bun: /data/dev/bun/src/bundler/linker_context/generateCompileResultForHtmlChunk.cp & /data/dev/bun/docs/bundler/standalone-html.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation HTML Chunk Compiler & Standalone Single-File HTML Asset Injector:
// - Locates </head>, </body>, and </html> anchor offsets while skipping HTML comments and raw <script>/<style> bodies
// - Standard bundle mode injects CSS <link rel="stylesheet">, <link rel="modulepreload">, and <script type="module"> (CSS before JS to avoid FOUC)
// - Standalone HTML mode inlines CSS into <head> as <style> and JS before </body> as <script type="module"> so DOM elements are ready
// - Safely escapes </script> / </style> closing sequences in inline payloads and HTML attribute special characters in URLs.

/// 单次 HTML Chunk 编译支持的最大 `modulepreload` 预加载条目数
pub const HTML_CHUNK_MAX_PRELOADS: usize = 32;

/// HTML Chunk 编译与注入错误集
pub err HtmlChunkError {
    BufferTooSmall,
    TooManyPreloads,
    InvalidAssetUrl,
}

/// HTML 文档关键闭合标签字节偏移（精确对齐 Bun `EndTagIndices`）
pub struct HtmlEndTagIndices {
    head: usize? = null,
    body: usize? = null,
    html: usize? = null,

    /// 计算 `<head>` 区域标签的确定性注入偏移：`</head>` -> `</body>` -> `</html>` -> 文末
    fn headInsertOffset(doc_len: usize): usize {
        if (h := _.head) return h;
        if (b := _.body) return b;
        if (ht := _.html) return ht;
        return doc_len;
    }

    /// 计算 `<body>` 区域内联脚本的确定性注入偏移：`</body>` -> `</html>` -> 文末
    fn bodyInsertOffset(doc_len: usize): usize {
        if (b := _.body) return b;
        if (ht := _.html) return ht;
        return doc_len;
    }
}

/// HTML Chunk 编译与注入选项
pub struct HtmlChunkCompileOptions {
    /// 是否启用单文件自包含 Standalone HTML 模式（对齐 `bun build --compile --target=browser`）
    standalone_html: bool = false,
    /// 是否压缩标签间多余换行空白
    minify_whitespace: bool = false,
    /// 外链模式下是否为 `<link>` 与 `<script>` 附加 `crossorigin` 属性
    add_crossorigin: bool = true,
    /// 外链模式为 CSS URL / unique_key；standalone 模式为内联 CSS 源码
    css_ref: ([]const u8)? = null,
    /// 外链模式为 JS URL / unique_key；standalone 模式为内联 JS 源码
    js_ref: ([]const u8)? = null,
    /// 外链模式下需要注入 `<link rel="modulepreload">` 的子模块/共享 chunk URL 列表
    modulepreloads: [HTML_CHUNK_MAX_PRELOADS]([]const u8) = [""; HTML_CHUNK_MAX_PRELOADS],
    preload_count: usize = 0,

    fn addModulePreload(*_, url: []const u8): ! {
        if (url.len == 0) return err.InvalidAssetUrl;
        if (_.preload_count >= HTML_CHUNK_MAX_PRELOADS) {
            return err.TooManyPreloads;
        }
        _.modulepreloads[_.preload_count] = url;
        _.preload_count += 1;
    }
}

/// HTML Chunk 编译输出元信息
pub struct HtmlChunkCompileResult {
    bytes_written: usize = 0,
    end_tags: HtmlEndTagIndices = HtmlEndTagIndices{},
    head_injected: bool = false,
    body_injected: bool = false,
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + (32 as u8);
    }
    return c;
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn startsWithIgnoreCase(haystack: []const u8, pos: usize, needle_lower: []const u8): bool {
    if (pos + needle_lower.len > haystack.len) return false;
    let i: usize = 0;
    while (i < needle_lower.len) {
        if (asciiLower(haystack[pos + i]) != needle_lower[i]) return false;
        i += 1;
    }
    return true;
}

fn matchEndTagAt(html: []const u8, pos: usize, tag_lower: []const u8): bool {
    // 匹配 "</" + tag_lower + 空白* + ">"
    if (pos + 2 + tag_lower.len >= html.len) return false;
    if (html[pos] != '<' || html[pos + 1] != '/') return false;
    if (!startsWithIgnoreCase(html, pos + 2, tag_lower)) return false;
    let p = pos + 2 + tag_lower.len;
    while (p < html.len && isAsciiSpace(html[p])) {
        p += 1;
    }
    return p < html.len && html[p] == '>';
}

fn matchOpenTagAt(html: []const u8, pos: usize, tag_lower: []const u8): bool {
    if (pos + 1 + tag_lower.len >= html.len) return false;
    if (html[pos] != '<') return false;
    if (!startsWithIgnoreCase(html, pos + 1, tag_lower)) return false;
    const after = html[pos + 1 + tag_lower.len];
    return after == '>' || after == '/' || isAsciiSpace(after);
}

fn skipRawTextElement(html: []const u8, start_pos: usize, tag_lower: []const u8): usize {
    // 先跳过起始标签的 '>'
    let p = start_pos + 1 + tag_lower.len;
    while (p < html.len && html[p] != '>') {
        p += 1;
    }
    if (p < html.len) p += 1;
    // 查找对应的闭合标签 "</tag_lower>"
    while (p < html.len) {
        if (html[p] == '<' && matchEndTagAt(html, p, tag_lower)) {
            let q = p + 2 + tag_lower.len;
            while (q < html.len && html[q] != '>') {
                q += 1;
            }
            if (q < html.len) return q + 1;
            return html.len;
        }
        p += 1;
    }
    return html.len;
}

/// 扫描 HTML 文档，定位 `</head>`、`</body>`、`</html>` 的精确字节偏移
/// 自动跳过 `<!-- ... -->` 注释与 `<script>` / `<style>` 原始正文，避免误命中内联字符串中的伪标签。
pub fn findHtmlEndTagIndices(html: []const u8): HtmlEndTagIndices {
    let indices = HtmlEndTagIndices{};
    let i: usize = 0;

    while (i < html.len) {
        if (html[i] != '<') {
            i += 1;
            continue;
        }

        // 跳过 HTML 注释 <!-- ... -->
        if (i + 4 <= html.len &&
            html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-')
        {
            i += 4;
            while (i + 2 < html.len) {
                if (html[i] == '-' && html[i + 1] == '-' && html[i + 2] == '>') {
                    i += 3;
                    break;
                }
                i += 1;
            }
            continue;
        }

        // 跳过原始正文元素 <script> / <style>
        if (matchOpenTagAt(html, i, "script")) {
            i = skipRawTextElement(html, i, "script");
            continue;
        }
        if (matchOpenTagAt(html, i, "style")) {
            i = skipRawTextElement(html, i, "style");
            continue;
        }

        // 记录首次出现的 </head>、</body>、</html>
        if (indices.head == null && matchEndTagAt(html, i, "head")) {
            indices.head = i;
        } else if (indices.body == null && matchEndTagAt(html, i, "body")) {
            indices.body = i;
        } else if (indices.html == null && matchEndTagAt(html, i, "html")) {
            indices.html = i;
        }

        i += 1;
    }

    return indices;
}

fn appendBytes(out: []u8, pos: *usize, data: []const u8): ! {
    if (pos.* + data.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < data.len) {
        out[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): ! {
    if (pos.* >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

/// 将 URL 安全写入双引号 HTML 属性值（防属性逃逸）
pub fn writeEscapedAttrUrl(url: []const u8, out: []u8, pos: *usize): ! {
    let i: usize = 0;
    while (i < url.len) {
        const ch = url[i];
        if (ch == '"') {
            appendBytes(out, pos, "&quot;")?;
        } else if (ch == '&') {
            appendBytes(out, pos, "&amp;")?;
        } else if (ch == '<') {
            appendBytes(out, pos, "&lt;")?;
        } else if (ch == '>') {
            appendBytes(out, pos, "&gt;")?;
        } else {
            appendByte(out, pos, ch)?;
        }
        i += 1;
    }
}

/// 安全写入内联 `<script>` 正文：将任意大小写的 `</script` 转义为 `<\/script` 防止提前闭合标签
pub fn writeEscapedInlineScript(js_code: []const u8, out: []u8, pos: *usize): ! {
    let i: usize = 0;
    while (i < js_code.len) {
        if (js_code[i] == '<' && startsWithIgnoreCase(js_code, i, "</script")) {
            appendBytes(out, pos, "<\\/")?;
            i += 2;
            continue;
        }
        appendByte(out, pos, js_code[i])?;
        i += 1;
    }
}

/// 安全写入内联 `<style>` 正文：将任意大小写的 `</style` 转义为 `<\/style` 防止提前闭合标签
pub fn writeEscapedInlineStyle(css_code: []const u8, out: []u8, pos: *usize): ! {
    let i: usize = 0;
    while (i < css_code.len) {
        if (css_code[i] == '<' && startsWithIgnoreCase(css_code, i, "</style")) {
            appendBytes(out, pos, "<\\/")?;
            i += 2;
            continue;
        }
        appendByte(out, pos, css_code[i])?;
        i += 1;
    }
}

fn writeHeadInjection(opts: HtmlChunkCompileOptions, out: []u8, pos: *usize): bool! {
    let wrote_any = false;

    if (opts.standalone_html) {
        // Standalone HTML 模式：<head> 中仅内联 CSS <style>，JS 延迟至 </body> 前
        if (css := opts.css_ref) {
            appendBytes(out, pos, "<style>")?;
            writeEscapedInlineStyle(css, out, pos)?;
            appendBytes(out, pos, "</style>")?;
            if (!opts.minify_whitespace) appendByte(out, pos, '\n')?;
            wrote_any = true;
        }
        return wrote_any;
    }

    // 标准打包模式：按 CSS -> modulepreload -> JS 顺序注入（避免 FOUC 闪烁）
    if (css_url := opts.css_ref) {
        if (opts.add_crossorigin) {
            appendBytes(out, pos, "<link rel=\"stylesheet\" crossorigin href=\"")?;
        } else {
            appendBytes(out, pos, "<link rel=\"stylesheet\" href=\"")?;
        }
        writeEscapedAttrUrl(css_url, out, pos)?;
        appendBytes(out, pos, "\">")?;
        if (!opts.minify_whitespace) appendByte(out, pos, '\n')?;
        wrote_any = true;
    }

    let p_idx: usize = 0;
    while (p_idx < opts.preload_count) {
        const preload_url = opts.modulepreloads[p_idx];
        if (opts.add_crossorigin) {
            appendBytes(out, pos, "<link rel=\"modulepreload\" crossorigin href=\"")?;
        } else {
            appendBytes(out, pos, "<link rel=\"modulepreload\" href=\"")?;
        }
        writeEscapedAttrUrl(preload_url, out, pos)?;
        appendBytes(out, pos, "\">")?;
        if (!opts.minify_whitespace) appendByte(out, pos, '\n')?;
        wrote_any = true;
        p_idx += 1;
    }

    if (js_url := opts.js_ref) {
        if (opts.add_crossorigin) {
            appendBytes(out, pos, "<script type=\"module\" crossorigin src=\"")?;
        } else {
            appendBytes(out, pos, "<script type=\"module\" src=\"")?;
        }
        writeEscapedAttrUrl(js_url, out, pos)?;
        appendBytes(out, pos, "\"></script>")?;
        if (!opts.minify_whitespace) appendByte(out, pos, '\n')?;
        wrote_any = true;
    }

    return wrote_any;
}

fn writeBodyInjection(opts: HtmlChunkCompileOptions, out: []u8, pos: *usize): bool! {
    if (!opts.standalone_html) return false;
    if (js := opts.js_ref) {
        appendBytes(out, pos, "<script type=\"module\">")?;
        writeEscapedInlineScript(js, out, pos)?;
        appendBytes(out, pos, "</script>")?;
        if (!opts.minify_whitespace) appendByte(out, pos, '\n')?;
        return true;
    }
    return false;
}

/// 编译并注入 HTML Chunk 产物（对齐 Bun `generateCompileResultForHtmlChunk.cp`）
pub fn compileHtmlChunk(
    html: []const u8,
    opts: HtmlChunkCompileOptions,
    out: []u8,
): HtmlChunkCompileResult! {
    const end_tags = findHtmlEndTagIndices(html);
    let pos: usize = 0;
    let res = HtmlChunkCompileResult{
        bytes_written: 0,
        end_tags: end_tags,
        head_injected: false,
        body_injected: false,
    };

    if (!opts.standalone_html) {
        const insert_at = end_tags.headInsertOffset(html.len);
        appendBytes(out, &pos, html[0..insert_at])?;
        res.head_injected = writeHeadInjection(opts, out, &pos)?;
        appendBytes(out, &pos, html[insert_at..html.len])?;
        res.bytes_written = pos;
        return res;
    }

    // Standalone HTML 模式：CSS 注入 head 锚点，JS 注入 body 锚点
    // 若缺少 </head>，CSS 回退到 </html> 或文末（若 </body> 存在则 CSS 仍位于 head_pos <= body_pos）
    let head_pos: usize = html.len;
    if (h := end_tags.head) {
        head_pos = h;
    } else if (ht := end_tags.html) {
        head_pos = ht;
    }
    const body_pos = end_tags.bodyInsertOffset(html.len);

    if (head_pos <= body_pos) {
        appendBytes(out, &pos, html[0..head_pos])?;
        res.head_injected = writeHeadInjection(opts, out, &pos)?;
        appendBytes(out, &pos, html[head_pos..body_pos])?;
        res.body_injected = writeBodyInjection(opts, out, &pos)?;
        appendBytes(out, &pos, html[body_pos..html.len])?;
    } else {
        // 畸形文档（如 </body> 出现在 </head> 或 </html> 之前）：按物理偏移升序无损拼接
        appendBytes(out, &pos, html[0..body_pos])?;
        res.body_injected = writeBodyInjection(opts, out, &pos)?;
        appendBytes(out, &pos, html[body_pos..head_pos])?;
        res.head_injected = writeHeadInjection(opts, out, &pos)?;
        appendBytes(out, &pos, html[head_pos..html.len])?;
    }

    res.bytes_written = pos;
    return res;
}
