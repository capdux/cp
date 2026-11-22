// Source: Absorbed from /data/dev/bun/src/bundler/HTMLScanner.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/bundler/HTMLScanner.cp & /data/dev/bun/docs/bundler/html-static.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation HTML entry static asset scanner, WHATWG srcset candidate parser,
// external/data URI classifier, comment/raw-text skipping tokenizer, and exact-span URL rewriter.

/// 单次 HTML 扫描支持的最大静态资源引用数量
pub const HTML_SCANNER_MAX_ASSETS: usize = 64;

/// 单个 `srcset` 属性支持的最大候选数量
pub const HTML_SCANNER_MAX_SRCSET_CANDIDATES: usize = 16;

/// 单个 HTML 开始标签最多解析的属性个数
const HTML_MAX_TAG_ATTRS: usize = 24;

/// HTML 静态资源扫描与改写错误集
pub err HtmlScannerError {
    TooManyAssets,
    TooManySrcsetCandidates,
    BufferTooSmall,
}

/// HTML 资源导入分类（对齐 Bun `ImportKind`: `stmt` / `at` / `url`）
pub enum HtmlImportKind: u8 {
    /// 脚本与 Worker 入口（`<script src>`, `<link as="worker" href>`）
    stmt = 0,
    /// CSS 样式表（`<link rel="stylesheet" href>`, `<link as="style" href>`）
    at = 1,
    /// 静态媒体、字体、图标与 Manifest 资源
    url = 2,
}

/// 命中的 HTML 标签类型
pub enum HtmlTagKind: u8 {
    script = 0,
    link = 1,
    img = 2,
    video = 3,
    audio = 4,
    source = 5,
}

/// 提取出的单条 HTML 静态资源引用（零拷贝借用原 HTML 切片 + 精确字节偏移）
pub struct HtmlAssetRef {
    tag: HtmlTagKind = HtmlTagKind.img,
    kind: HtmlImportKind = HtmlImportKind.url,
    attr_name: []const u8 = "",
    url: []const u8 = "",
    url_start: usize = 0,
    url_end: usize = 0,
    is_external: bool = false,
    is_module_script: bool = false,

    /// 是否为需要打包器解析与哈希改写的本地静态资源
    fn isLocalBundleable(): bool {
        return !_.is_external && _.url.len > 0;
    }
}

/// WHATWG `srcset` 单个候选条目
pub struct SrcsetCandidate {
    url: []const u8 = "",
    descriptor: []const u8 = "",
    url_start: usize = 0,
    url_end: usize = 0,
}

/// HTML 资源 URL 改写映射项
pub struct HtmlRewritePair {
    from_url: []const u8 = "",
    to_url: []const u8 = "",
}

/// HTML 静态资源扫描结果（值语义，零堆分配）
pub struct HtmlScanResult {
    assets: [HTML_SCANNER_MAX_ASSETS]HtmlAssetRef = [HtmlAssetRef{}; HTML_SCANNER_MAX_ASSETS],
    count: usize = 0,
    has_head: bool = false,
    has_html: bool = false,
    script_count: usize = 0,
    stylesheet_count: usize = 0,
    media_count: usize = 0,

    fn assetAt(idx: usize): HtmlAssetRef? {
        if (idx >= _.count) {
            return null;
        }
        return _.assets[idx];
    }

    fn localAssetCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.assets[i].isLocalBundleable()) {
                n += 1;
            }
            i += 1;
        }
        return n;
    }

    fn pushAsset(*_, asset: HtmlAssetRef)! {
        if (_.count >= HTML_SCANNER_MAX_ASSETS) {
            return HtmlScannerError.TooManyAssets;
        }
        _.assets[_.count] = asset;
        _.count += 1;
        switch (asset.kind) {
            .stmt => {
                _.script_count += 1;
            },
            .at => {
                _.stylesheet_count += 1;
            },
            .url => {
                _.media_count += 1;
            },
        }
    }
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isHtmlWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn asciiStartsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return asciiEqlIgnoreCase(s[0..prefix.len], prefix);
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

fn trimHtmlWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHtmlWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isHtmlWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

/// 判断 URL 是否为外部地址、协议相对地址、内联数据 URI 或页面锚点（不纳入本地打包）
pub fn isExternalHtmlUrl(url: []const u8): bool {
    const t = trimHtmlWs(url);
    if (t.len == 0) {
        return true;
    }
    if (t[0] == '#') {
        return true;
    }
    if (t.len >= 2 && t[0] == '/' && t[1] == '/') {
        return true;
    }
    if (asciiStartsWithIgnoreCase(t, "http://") ||
        asciiStartsWithIgnoreCase(t, "https://") ||
        asciiStartsWithIgnoreCase(t, "data:") ||
        asciiStartsWithIgnoreCase(t, "blob:") ||
        asciiStartsWithIgnoreCase(t, "mailto:") ||
        asciiStartsWithIgnoreCase(t, "tel:") ||
        asciiStartsWithIgnoreCase(t, "javascript:"))
    {
        return true;
    }
    return false;
}

/// 解析 WHATWG HTML `srcset` 候选列表（支持 `1x`/`2x`/`800w` 描述符与 `data:` URI 内嵌逗号）
pub fn parseSrcsetCandidates(srcset: []const u8, out: []SrcsetCandidate): usize! {
    let count: usize = 0;
    let i: usize = 0;

    while (i < srcset.len) {
        while (i < srcset.len && (isHtmlWs(srcset[i]) || srcset[i] == ',')) {
            i += 1;
        }
        if (i >= srcset.len) {
            break;
        }

        const url_start = i;
        const is_data = asciiStartsWithIgnoreCase(srcset[i..], "data:");
        let seen_data_comma = false;

        while (i < srcset.len) {
            const c = srcset[i];
            if (isHtmlWs(c)) {
                break;
            }
            if (c == ',') {
                // `data:image/png;base64,AAA` 包含一个分隔元数据的逗号，不能提前截断
                if (is_data && !seen_data_comma) {
                    seen_data_comma = true;
                    i += 1;
                    continue;
                }
                break;
            }
            i += 1;
        }
        const url_end = i;

        // 跳过 URL 后的空白并扫描描述符（至下一个逗号或结束）
        while (i < srcset.len && isHtmlWs(srcset[i])) {
            i += 1;
        }
        let desc_start = i;
        let desc_end = i;
        if (i < srcset.len && srcset[i] != ',') {
            desc_start = i;
            let in_paren = false;
            while (i < srcset.len) {
                const c = srcset[i];
                if (c == '(') {
                    in_paren = true;
                } else if (c == ')') {
                    in_paren = false;
                } else if (c == ',' && !in_paren) {
                    break;
                }
                i += 1;
            }
            desc_end = i;
            while (desc_end > desc_start && isHtmlWs(srcset[desc_end - 1])) {
                desc_end -= 1;
            }
        }
        if (i < srcset.len && srcset[i] == ',') {
            i += 1;
        }

        if (url_end > url_start) {
            if (count >= out.len) {
                return HtmlScannerError.TooManySrcsetCandidates;
            }
            out[count] = SrcsetCandidate{
                url: srcset[url_start..url_end],
                descriptor: srcset[desc_start..desc_end],
                url_start: url_start,
                url_end: url_end,
            };
            count += 1;
        }
    }

    return count;
}

struct ParsedAttr {
    name: []const u8 = "",
    value: []const u8 = "",
    val_start: usize = 0,
    val_end: usize = 0,
}

fn findAttr(attrs: []const ParsedAttr, name: []const u8): ParsedAttr? {
    let i: usize = 0;
    while (i < attrs.len) {
        if (asciiEqlIgnoreCase(attrs[i].name, name)) {
            return attrs[i];
        }
        i += 1;
    }
    return null;
}

fn attrValueHasToken(value: []const u8, token: []const u8): bool {
    let i: usize = 0;
    while (i < value.len) {
        while (i < value.len && isHtmlWs(value[i])) {
            i += 1;
        }
        const start = i;
        while (i < value.len && !isHtmlWs(value[i])) {
            i += 1;
        }
        if (i > start && asciiEqlIgnoreCase(value[start..i], token)) {
            return true;
        }
    }
    return false;
}

fn classifyLinkImportKind(attrs: []const ParsedAttr): HtmlImportKind? {
    if (rel := findAttr(attrs, "rel")) {
        if (attrValueHasToken(rel.value, "stylesheet")) {
            return HtmlImportKind.at;
        }
        if (attrValueHasToken(rel.value, "manifest") ||
            attrValueHasToken(rel.value, "icon") ||
            attrValueHasToken(rel.value, "apple-touch-icon"))
        {
            return HtmlImportKind.url;
        }
    }
    if (as_attr := findAttr(attrs, "as")) {
        const v = trimHtmlWs(as_attr.value);
        if (asciiEqlIgnoreCase(v, "style")) {
            return HtmlImportKind.at;
        }
        if (asciiEqlIgnoreCase(v, "worker")) {
            return HtmlImportKind.stmt;
        }
        if (asciiEqlIgnoreCase(v, "font") ||
            asciiEqlIgnoreCase(v, "image") ||
            asciiEqlIgnoreCase(v, "video") ||
            asciiEqlIgnoreCase(v, "audio"))
        {
            return HtmlImportKind.url;
        }
    }
    if (type_attr := findAttr(attrs, "type")) {
        if (asciiStartsWithIgnoreCase(trimHtmlWs(type_attr.value), "font/")) {
            return HtmlImportKind.url;
        }
    }
    return null;
}

fn pushSingleUrlAttr(
    res: *HtmlScanResult,
    tag: HtmlTagKind,
    kind: HtmlImportKind,
    attr: ParsedAttr,
    is_module_script: bool,
)! {
    const trimmed = trimHtmlWs(attr.value);
    if (trimmed.len == 0) {
        return;
    }
    // 定位 trim 后在原 HTML 中的精确偏移
    let leading: usize = 0;
    while (leading < attr.value.len && isHtmlWs(attr.value[leading])) {
        leading += 1;
    }
    const start = attr.val_start + leading;
    const end = start + trimmed.len;
    res.pushAsset(HtmlAssetRef{
        tag: tag,
        kind: kind,
        attr_name: attr.name,
        url: trimmed,
        url_start: start,
        url_end: end,
        is_external: isExternalHtmlUrl(trimmed),
        is_module_script: is_module_script,
    })?;
}

fn pushSrcsetAttr(res: *HtmlScanResult, tag: HtmlTagKind, attr: ParsedAttr)! {
    let cands: [HTML_SCANNER_MAX_SRCSET_CANDIDATES]SrcsetCandidate = [SrcsetCandidate{}; HTML_SCANNER_MAX_SRCSET_CANDIDATES];
    const n = parseSrcsetCandidates(attr.value, cands[0..])?;
    let i: usize = 0;
    while (i < n) {
        const c = cands[i];
        res.pushAsset(HtmlAssetRef{
            tag: tag,
            kind: HtmlImportKind.url,
            attr_name: attr.name,
            url: c.url,
            url_start: attr.val_start + c.url_start,
            url_end: attr.val_start + c.url_end,
            is_external: isExternalHtmlUrl(c.url),
            is_module_script: false,
        })?;
        i += 1;
    }
}

fn skipRawTextUntilCloseTag(html: []const u8, start_pos: usize, tag_name: []const u8): usize {
    let i = start_pos;
    while (i + 2 + tag_name.len < html.len) {
        if (html[i] == '<' && html[i + 1] == '/') {
            const name_start = i + 2;
            const name_end = name_start + tag_name.len;
            if (asciiEqlIgnoreCase(html[name_start..name_end], tag_name)) {
                let k = name_end;
                while (k < html.len && isHtmlWs(html[k])) {
                    k += 1;
                }
                if (k < html.len && html[k] == '>') {
                    return k + 1;
                }
            }
        }
        i += 1;
    }
    return html.len;
}

/// 扫描 HTML 文档并提取所有打包器关注的静态资源引用（完整覆盖 Bun `HTMLScanner.cp` 16 条规则）
pub fn scanHtmlAssets(html: []const u8): HtmlScanResult! {
    let res = HtmlScanResult{};
    let i: usize = 0;

    while (i < html.len) {
        if (html[i] != '<') {
            i += 1;
            continue;
        }

        // 跳过 HTML 注释 `<!-- ... -->`
        if (i + 4 <= html.len && bytesEql(html[i..i + 4], "<!--")) {
            i += 4;
            while (i + 3 <= html.len) {
                if (bytesEql(html[i..i + 3], "-->")) {
                    i += 3;
                    break;
                }
                i += 1;
            }
            if (i + 3 > html.len) {
                i = html.len;
            }
            continue;
        }

        // 跳过 `<!DOCTYPE ...>` / `<?...?>` / 结束标签 `</...>`
        if (i + 1 < html.len && (html[i + 1] == '!' || html[i + 1] == '?' || html[i + 1] == '/')) {
            i += 2;
            while (i < html.len && html[i] != '>') {
                i += 1;
            }
            if (i < html.len) {
                i += 1;
            }
            continue;
        }

        // 解析开始标签名
        const tag_start = i + 1;
        let p = tag_start;
        while (p < html.len && !isHtmlWs(html[p]) && html[p] != '>' && html[p] != '/') {
            p += 1;
        }
        const tag_name = html[tag_start..p];
        if (tag_name.len == 0) {
            i += 1;
            continue;
        }

        // 解析开始标签上的属性列表
        let attrs: [HTML_MAX_TAG_ATTRS]ParsedAttr = [ParsedAttr{}; HTML_MAX_TAG_ATTRS];
        let attr_count: usize = 0;
        let self_closing = false;

        while (p < html.len) {
            while (p < html.len && isHtmlWs(html[p])) {
                p += 1;
            }
            if (p >= html.len) {
                break;
            }
            if (html[p] == '>') {
                p += 1;
                break;
            }
            if (html[p] == '/') {
                self_closing = true;
                p += 1;
                if (p < html.len && html[p] == '>') {
                    p += 1;
                    break;
                }
                continue;
            }

            const a_start = p;
            while (p < html.len && !isHtmlWs(html[p]) && html[p] != '=' && html[p] != '>' && html[p] != '/') {
                p += 1;
            }
            const a_name = html[a_start..p];
            while (p < html.len && isHtmlWs(html[p])) {
                p += 1;
            }

            let val_start = p;
            let val_end = p;
            if (p < html.len && html[p] == '=') {
                p += 1;
                while (p < html.len && isHtmlWs(html[p])) {
                    p += 1;
                }
                if (p < html.len && (html[p] == '"' || html[p] == '\'')) {
                    const quote = html[p];
                    p += 1;
                    val_start = p;
                    while (p < html.len && html[p] != quote) {
                        p += 1;
                    }
                    val_end = p;
                    if (p < html.len && html[p] == quote) {
                        p += 1;
                    }
                } else {
                    val_start = p;
                    while (p < html.len && !isHtmlWs(html[p]) && html[p] != '>') {
                        p += 1;
                    }
                    val_end = p;
                }
            }

            if (a_name.len > 0 && attr_count < HTML_MAX_TAG_ATTRS) {
                attrs[attr_count] = ParsedAttr{
                    name: a_name,
                    value: html[val_start..val_end],
                    val_start: val_start,
                    val_end: val_end,
                };
                attr_count += 1;
            }
        }

        const attr_slice = attrs[0..attr_count];

        if (asciiEqlIgnoreCase(tag_name, "head")) {
            res.has_head = true;
        } else if (asciiEqlIgnoreCase(tag_name, "html")) {
            res.has_html = true;
        } else if (asciiEqlIgnoreCase(tag_name, "script")) {
            let is_mod = false;
            if (t := findAttr(attr_slice, "type")) {
                is_mod = asciiEqlIgnoreCase(trimHtmlWs(t.value), "module");
            }
            if (src := findAttr(attr_slice, "src")) {
                pushSingleUrlAttr(&res, HtmlTagKind.script, HtmlImportKind.stmt, src, is_mod)?;
            }
            if (!self_closing) {
                p = skipRawTextUntilCloseTag(html, p, "script");
            }
        } else if (asciiEqlIgnoreCase(tag_name, "style")) {
            if (!self_closing) {
                p = skipRawTextUntilCloseTag(html, p, "style");
            }
        } else if (asciiEqlIgnoreCase(tag_name, "link")) {
            if (href := findAttr(attr_slice, "href")) {
                if (kind := classifyLinkImportKind(attr_slice)) {
                    pushSingleUrlAttr(&res, HtmlTagKind.link, kind, href, false)?;
                }
            }
        } else if (asciiEqlIgnoreCase(tag_name, "img")) {
            if (src := findAttr(attr_slice, "src")) {
                pushSingleUrlAttr(&res, HtmlTagKind.img, HtmlImportKind.url, src, false)?;
            }
            if (srcset := findAttr(attr_slice, "srcset")) {
                pushSrcsetAttr(&res, HtmlTagKind.img, srcset)?;
            }
        } else if (asciiEqlIgnoreCase(tag_name, "video")) {
            if (src := findAttr(attr_slice, "src")) {
                pushSingleUrlAttr(&res, HtmlTagKind.video, HtmlImportKind.url, src, false)?;
            }
            if (poster := findAttr(attr_slice, "poster")) {
                pushSingleUrlAttr(&res, HtmlTagKind.video, HtmlImportKind.url, poster, false)?;
            }
        } else if (asciiEqlIgnoreCase(tag_name, "audio")) {
            if (src := findAttr(attr_slice, "src")) {
                pushSingleUrlAttr(&res, HtmlTagKind.audio, HtmlImportKind.url, src, false)?;
            }
        } else if (asciiEqlIgnoreCase(tag_name, "source")) {
            if (src := findAttr(attr_slice, "src")) {
                pushSingleUrlAttr(&res, HtmlTagKind.source, HtmlImportKind.url, src, false)?;
            }
            if (srcset := findAttr(attr_slice, "srcset")) {
                pushSrcsetAttr(&res, HtmlTagKind.source, srcset)?;
            }
        }

        i = p;
    }

    return res;
}

fn findRewriteTarget(url: []const u8, rewrites: []const HtmlRewritePair): ([]const u8)? {
    let i: usize = 0;
    while (i < rewrites.len) {
        if (bytesEql(rewrites[i].from_url, url)) {
            return rewrites[i].to_url;
        }
        i += 1;
    }
    return null;
}

fn appendChunk(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return HtmlScannerError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// 基于 `scanHtmlAssets` 的精确字节区间，将 HTML 中匹配到的本地资源 URL 无损改写为新路径（不误伤注释与脚本正文）
pub fn rewriteHtmlAssetUrls(
    html: []const u8,
    rewrites: []const HtmlRewritePair,
    out: []u8,
): usize! {
    const scan = scanHtmlAssets(html)?;
    let out_pos: usize = 0;
    let cursor: usize = 0;

    let i: usize = 0;
    while (i < scan.count) {
        const asset = scan.assets[i];
        if (asset.url_start >= cursor && asset.url_end <= html.len) {
            if (replacement := findRewriteTarget(asset.url, rewrites)) {
                out_pos = appendChunk(out, out_pos, html[cursor..asset.url_start])?;
                out_pos = appendChunk(out, out_pos, replacement)?;
                cursor = asset.url_end;
            }
        }
        i += 1;
    }

    out_pos = appendChunk(out, out_pos, html[cursor..html.len])?;
    return out_pos;
}
