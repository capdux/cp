// Source: Absorbed from /data/dev/bun/src/http_types/FetchRequestMode.cp (WHATWG Fetch CORS)

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) { start += 1; }
    while (end > start && isOws(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

/// 零堆分配 CORS 策略配置
pub struct CorsConfig {
    allow_all_origins: bool = false,
    allowed_origins: [16][]const u8 = [""; 16],
    origin_len: usize = 0,
    allowed_methods: [12][]const u8 = [""; 12],
    method_len: usize = 0,
    allowed_headers: [16][]const u8 = [""; 16],
    header_len: usize = 0,
    allow_credentials: bool = false,
    max_age_secs: u32? = null,

    fn addOrigin(*_, origin: []const u8): bool {
        if (_.origin_len >= 16 || origin.len == 0) { return false; }
        _.allowed_origins[_.origin_len] = origin;
        _.origin_len += 1;
        return true;
    }

    fn addMethod(*_, method: []const u8): bool {
        if (_.method_len >= 12 || method.len == 0) { return false; }
        _.allowed_methods[_.method_len] = method;
        _.method_len += 1;
        return true;
    }

    fn addHeader(*_, header: []const u8): bool {
        if (_.header_len >= 16 || header.len == 0) { return false; }
        _.allowed_headers[_.header_len] = header;
        _.header_len += 1;
        return true;
    }
}

/// CORS 评估决策结果与待写入的响应头指令
pub struct CorsDecision {
    allowed: bool = false,
    allow_origin_header: []const u8 = "",
    vary_origin: bool = false,
    allow_credentials: bool = false,
    max_age_secs: u32? = null,
}

/// WHATWG Fetch CORS 策略评估与预检（Preflight）计算引擎
pub struct CorsPolicy {
    config: CorsConfig = CorsConfig{},

    static fn init(config: CorsConfig): CorsPolicy {
        return CorsPolicy{ config: config };
    }

    /// 判断请求头是否属于 WHATWG CORS-safelisted 请求头（大小写无关）
    static fn isSafelistedHeader(header_name: []const u8): bool {
        return eqlIgnoreCaseAscii(header_name, "accept") ||
            eqlIgnoreCaseAscii(header_name, "accept-language") ||
            eqlIgnoreCaseAscii(header_name, "content-language") ||
            eqlIgnoreCaseAscii(header_name, "content-type");
    }

    fn isOriginAllowed(origin: []const u8): bool {
        let i: usize = 0;
        while (i < _.config.origin_len) {
            if (eqlBytes(_.config.allowed_origins[i], origin)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn isMethodAllowed(method: []const u8): bool {
        if (_.config.method_len == 0) {
            return eqlIgnoreCaseAscii(method, "GET") ||
                eqlIgnoreCaseAscii(method, "HEAD") ||
                eqlIgnoreCaseAscii(method, "POST");
        }
        let i: usize = 0;
        while (i < _.config.method_len) {
            if (eqlIgnoreCaseAscii(_.config.allowed_methods[i], method)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn isHeaderAllowed(header_name: []const u8): bool {
        if (CorsPolicy.isSafelistedHeader(header_name)) {
            return true;
        }
        let i: usize = 0;
        while (i < _.config.header_len) {
            const allowed = _.config.allowed_headers[i];
            if (!_.config.allow_credentials && eqlBytes(allowed, "*")) {
                return true;
            }
            if (eqlIgnoreCaseAscii(allowed, header_name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 评估简单 CORS 请求（Simple Request）的 Origin 权限与响应头
    /// 当 `allow_credentials` 为 true 时严禁输出 `"*"`，强制回显具体 `origin` 并设置 `vary_origin = true`
    fn evaluateSimple(origin: []const u8): CorsDecision {
        const clean_origin = trimOws(origin);
        if (clean_origin.len == 0 || eqlBytes(clean_origin, "*")) {
            return CorsDecision{ allowed: false };
        }

        if (_.config.allow_all_origins) {
            if (_.config.allow_credentials) {
                return CorsDecision{
                    allowed: true,
                    allow_origin_header: clean_origin,
                    vary_origin: true,
                    allow_credentials: true,
                    max_age_secs: null,
                };
            }
            return CorsDecision{
                allowed: true,
                allow_origin_header: "*",
                vary_origin: false,
                allow_credentials: false,
                max_age_secs: null,
            };
        }

        if (_.isOriginAllowed(clean_origin)) {
            return CorsDecision{
                allowed: true,
                allow_origin_header: clean_origin,
                vary_origin: true,
                allow_credentials: _.config.allow_credentials,
                max_age_secs: null,
            };
        }

        return CorsDecision{
            allowed: false,
            vary_origin: true,
        };
    }

    /// 评估 OPTIONS 预检请求（Preflight）：依次校验 Origin、Method 与逗号分隔的 `Access-Control-Request-Headers`
    fn evaluatePreflight(origin: []const u8, req_method: []const u8, req_headers: []const u8): CorsDecision {
        const base = _.evaluateSimple(origin);
        if (!base.allowed) {
            return base;
        }

        const method = trimOws(req_method);
        if (method.len == 0 || !_.isMethodAllowed(method)) {
            return CorsDecision{
                allowed: false,
                vary_origin: base.vary_origin,
            };
        }

        // 逐项校验逗号分隔的 Access-Control-Request-Headers
        let pos: usize = 0;
        while (pos < req_headers.len) {
            const start = pos;
            while (pos < req_headers.len && req_headers[pos] != ',') {
                pos += 1;
            }
            const h = trimOws(req_headers[start..pos]);
            if (pos < req_headers.len && req_headers[pos] == ',') {
                pos += 1;
            }
            if (h.len == 0) {
                continue;
            }
            if (!_.isHeaderAllowed(h)) {
                return CorsDecision{
                    allowed: false,
                    vary_origin: base.vary_origin,
                };
            }
        }

        return CorsDecision{
            allowed: true,
            allow_origin_header: base.allow_origin_header,
            vary_origin: base.vary_origin,
            allow_credentials: base.allow_credentials,
            max_age_secs: _.config.max_age_secs,
        };
    }
}
