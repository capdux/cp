// Source: Absorbed from /data/dev/bun/src/http_types/{Encoding,FetchCacheMode,FetchRedirect,FetchRequestMode,URLPath}.cp

/// 内容编码支持位掩码常量
pub const ENC_MASK_IDENTITY: u8 = 1 << 0;
pub const ENC_MASK_GZIP: u8 = 1 << 1;
pub const ENC_MASK_DEFLATE: u8 = 1 << 2;
pub const ENC_MASK_BR: u8 = 1 << 3;
pub const ENC_MASK_ZSTD: u8 = 1 << 4;
pub const ENC_MASK_ALL: u8 = 0x1F;

fn trimAscii(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

/// HTTP Content-Encoding 枚举（`identity` / `gzip` / `deflate` / `br` / `zstd`）
pub enum ContentEncoding: u8 {
    identity = 0,
    gzip = 1,
    deflate = 2,
    br = 3,
    zstd = 4,

    static fn parse(input: []const u8): ContentEncoding? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "identity")) { return .identity; }
        if (std.ascii.eqlIgnoreCase(s, "gzip") || std.ascii.eqlIgnoreCase(s, "x-gzip")) { return .gzip; }
        if (std.ascii.eqlIgnoreCase(s, "deflate")) { return .deflate; }
        if (std.ascii.eqlIgnoreCase(s, "br") || std.ascii.eqlIgnoreCase(s, "brotli")) { return .br; }
        if (std.ascii.eqlIgnoreCase(s, "zstd")) { return .zstd; }
        return null;
    }

    fn toHeaderValue(): []const u8 {
        return switch (_) {
            .identity => "identity",
            .gzip => "gzip",
            .deflate => "deflate",
            .br => "br",
            .zstd => "zstd",
        };
    }

    fn maskBit(): u8 {
        return switch (_) {
            .identity => ENC_MASK_IDENTITY,
            .gzip => ENC_MASK_GZIP,
            .deflate => ENC_MASK_DEFLATE,
            .br => ENC_MASK_BR,
            .zstd => ENC_MASK_ZSTD,
        };
    }

    fn isCompressed(): bool {
        return _ != .identity;
    }

    fn canUseLibDeflate(): bool {
        return _ == .gzip || _ == .deflate;
    }
}

fn parseQualityParam(params: []const u8): f64? {
    let pos: usize = 0;
    while (pos < params.len) {
        let end = pos;
        while (end < params.len && params[end] != ';') {
            end += 1;
        }
        const seg = trimAscii(params[pos..end]);
        if (seg.len >= 3 && (seg[0] == 'q' || seg[0] == 'Q') && seg[1] == '=') {
            const qv = std.fmt.parseFloat(f64, trimAscii(seg[2..])) catch return null;
            if (std.math.isNan(qv) || qv < 0.0 || qv > 1.0) {
                return null;
            }
            return qv;
        }
        pos = if (end < params.len) (end + 1) else end;
    }
    return 1.0;
}

/// 按 RFC 9110 §12.5.3 解析 `Accept-Encoding` 头部并结合 `supported_mask` 协商最优内容编码。
/// 支持 `;q=0.0..1.0` 质量因子、`*` 通配与 `q=0` 显式禁用；同权重优先顺序：`zstd > br > gzip > deflate > identity`。
pub fn negotiateEncoding(accept_encoding: []const u8, supported_mask: u8): ContentEncoding {
    const s = trimAscii(accept_encoding);
    if (s.len == 0) {
        return ContentEncoding.identity;
    }

    let explicit_q: [5]f64 = [-1.0, -1.0, -1.0, -1.0, -1.0];
    let wildcard_q: f64 = -1.0;

    let start: usize = 0;
    let pos: usize = 0;
    while (pos <= s.len) {
        if (pos == s.len || s[pos] == ',') {
            const item = trimAscii(s[start..pos]);
            if (item.len > 0) {
                let coding = item;
                let q: f64 = 1.0;
                let valid_q = true;
                if (semi := std.mem.indexOfScalar(u8, item, ';')) {
                    coding = trimAscii(item[0..semi]);
                    if (parsed_q := parseQualityParam(item[semi + 1 ..])) {
                        q = parsed_q;
                    } else {
                        valid_q = false;
                    }
                }
                if (valid_q) {
                    if (std.mem.eql(u8, coding, "*")) {
                        wildcard_q = q;
                    } else if (enc := ContentEncoding.parse(coding)) {
                        const idx: usize = @intFromEnum(enc);
                        explicit_q[idx] = q;
                    }
                }
            }
            start = pos + 1;
        }
        pos += 1;
    }

    // 服务端优先序：zstd > br > gzip > deflate > identity
    const order: [5]ContentEncoding = [
        ContentEncoding.zstd,
        ContentEncoding.br,
        ContentEncoding.gzip,
        ContentEncoding.deflate,
        ContentEncoding.identity,
    ];

    let best_enc = ContentEncoding.identity;
    let best_q: f64 = 0.0;

    for (enc of order) {
        if ((supported_mask & enc.maskBit()) == 0) {
            continue;
        }
        const idx: usize = @intFromEnum(enc);
        let eff_q: f64 = 0.0;
        if (explicit_q[idx] >= 0.0) {
            eff_q = explicit_q[idx];
        } else if (wildcard_q >= 0.0) {
            eff_q = wildcard_q;
        } else if (enc == ContentEncoding.identity) {
            eff_q = 0.0001; // identity 默认隐式允许，但优先级低于任何显式/通配压缩编码
        }
        if (eff_q > best_q) {
            best_q = eff_q;
            best_enc = enc;
        }
    }

    return best_enc;
}

/// WHATWG Fetch `Request.cache` 缓存模式
pub enum FetchCacheMode: u8 {
    default_mode = 0,
    no_store = 1,
    reload = 2,
    no_cache = 3,
    force_cache = 4,
    only_if_cached = 5,

    static fn default(): FetchCacheMode { return .default_mode; }

    static fn parse(input: []const u8): FetchCacheMode? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "default")) { return .default_mode; }
        if (std.ascii.eqlIgnoreCase(s, "no-store")) { return .no_store; }
        if (std.ascii.eqlIgnoreCase(s, "reload")) { return .reload; }
        if (std.ascii.eqlIgnoreCase(s, "no-cache")) { return .no_cache; }
        if (std.ascii.eqlIgnoreCase(s, "force-cache")) { return .force_cache; }
        if (std.ascii.eqlIgnoreCase(s, "only-if-cached")) { return .only_if_cached; }
        return null;
    }

    fn toStr(): []const u8 {
        return switch (_) {
            .default_mode => "default",
            .no_store => "no-store",
            .reload => "reload",
            .no_cache => "no-cache",
            .force_cache => "force-cache",
            .only_if_cached => "only-if-cached",
        };
    }

    fn bypassesCacheRead(): bool {
        return _ == .no_store || _ == .reload;
    }

    fn skipsCacheWrite(): bool {
        return _ == .no_store;
    }
}

/// WHATWG Fetch `Request.redirect` 重定向策略
pub enum FetchRedirect: u8 {
    follow = 0,
    error_mode = 1,
    manual = 2,

    static fn default(): FetchRedirect { return .follow; }

    static fn parse(input: []const u8): FetchRedirect? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "follow")) { return .follow; }
        if (std.ascii.eqlIgnoreCase(s, "error")) { return .error_mode; }
        if (std.ascii.eqlIgnoreCase(s, "manual")) { return .manual; }
        return null;
    }

    fn toStr(): []const u8 {
        return switch (_) {
            .follow => "follow",
            .error_mode => "error",
            .manual => "manual",
        };
    }
}

/// WHATWG Fetch `Request.mode` 请求模式
pub enum FetchRequestMode: u8 {
    navigate = 0,
    same_origin = 1,
    no_cors = 2,
    cors = 3,

    static fn default(): FetchRequestMode { return .cors; }

    static fn parse(input: []const u8): FetchRequestMode? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "navigate")) { return .navigate; }
        if (std.ascii.eqlIgnoreCase(s, "same-origin")) { return .same_origin; }
        if (std.ascii.eqlIgnoreCase(s, "no-cors")) { return .no_cors; }
        if (std.ascii.eqlIgnoreCase(s, "cors")) { return .cors; }
        return null;
    }

    fn toStr(): []const u8 {
        return switch (_) {
            .navigate => "navigate",
            .same_origin => "same-origin",
            .no_cors => "no-cors",
            .cors => "cors",
        };
    }

    fn allowsCrossOrigin(): bool {
        return _ == .cors || _ == .no_cors || _ == .navigate;
    }
}

/// URL 路径零拷贝拆分结果（`path` / `query` / `fragment` / `extname` / `basename`）
pub struct UrlPath {
    path: []const u8 = "",
    query: ([]const u8)? = null,
    fragment: ([]const u8)? = null,
    extname: []const u8 = "",
    basename: []const u8 = "",

    /// 零拷贝拆分 URL 路径串（如 `"/api/v1/report.tar.gz?fmt=json#top"`）
    static fn parse(raw: []const u8): UrlPath {
        let main_part = raw;
        let frag: ([]const u8)? = null;
        if (hash := std.mem.indexOfScalar(u8, raw, '#')) {
            main_part = raw[0..hash];
            frag = raw[hash + 1 ..];
        }

        let path_part = main_part;
        let query_part: ([]const u8)? = null;
        if (qmark := std.mem.indexOfScalar(u8, main_part, '?')) {
            path_part = main_part[0..qmark];
            query_part = main_part[qmark + 1 ..];
        }

        // 计算 basename（去除末尾 '/' 后的最后一段）
        let end = path_part.len;
        while (end > 0 && path_part[end - 1] == '/') {
            end -= 1;
        }
        let base: []const u8 = "";
        if (end > 0) {
            let start = end;
            while (start > 0 && path_part[start - 1] != '/') {
                start -= 1;
            }
            base = path_part[start..end];
        }

        // 计算 extname（包含前导 '.'；隐藏文件如 `.env` 无扩展名）
        let ext: []const u8 = "";
        if (base.len > 1) {
            if (dot := std.mem.lastIndexOfScalar(u8, base, '.')) {
                if (dot > 0) {
                    ext = base[dot..];
                }
            }
        }

        return UrlPath{
            path: path_part,
            query: query_part,
            fragment: frag,
            extname: ext,
            basename: base,
        };
    }

    /// 检测路径是否已规范化（不含 `//`、`..` 段或 `.` 段）
    fn isNormalized(): bool {
        const p = _.path;
        if (p.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < p.len) {
            if (i + 1 < p.len && p[i] == '/' && p[i + 1] == '/') {
                return false;
            }
            if (p[i] == '/') {
                i += 1;
                continue;
            }
            const seg_start = i;
            while (i < p.len && p[i] != '/') {
                i += 1;
            }
            const seg = p[seg_start..i];
            if (std.mem.eql(u8, seg, ".") || std.mem.eql(u8, seg, "..")) {
                return false;
            }
        }
        return true;
    }
}
