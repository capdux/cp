// Source: Absorbed from /data/dev/bun/src/install/PackageManager/security_scanner.cp | Synced: 2026-11-22T12:34:06Z
// Ported from Bun: /data/dev/bun/src/install/PackageManager/security_scanner.cp & /data/dev/bun/docs/pm/security-scanner-api.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation package security scanner protocol types, strict JSON advisory parser,
// TTY/CI install decision evaluator, warning confirmation parser, and human-readable report formatter.

/// 支持的 Bun Security Scanner API 协议版本
pub const SECURITY_SCANNER_PROTOCOL_VERSION: []const u8 = "1";

/// 单次扫描结果支持的最大安全通告数量
pub const SECURITY_SCANNER_MAX_ADVISORIES: usize = 64;

/// 单条通告依赖链支持的最大祖先深度
pub const SECURITY_SCANNER_MAX_PATH_DEPTH: usize = 16;

/// 安全扫描器解析、校验与格式化错误集
pub err SecurityScannerError {
    InvalidVersion,
    MissingPackageField,
    EmptyPackageField,
    InvalidPackageField,
    MissingLevelField,
    InvalidLevelValue,
    InvalidDescriptionField,
    InvalidUrlField,
    InvalidAdvisoriesFormat,
    InvalidAdvisoryFormat,
    TooManyAdvisories,
    TooManyPathSegments,
    BufferTooSmall,
}

/// 扫描器子进程标准错误码（对应 Bun `SecurityScannerErrorCode`）
pub enum SecurityScannerErrorCode {
    module_not_found,
    invalid_version,
    scan_failed,

    fn asCode(): []const u8 {
        return switch (_) {
            .module_not_found => "MODULE_NOT_FOUND",
            .invalid_version => "INVALID_VERSION",
            .scan_failed => "SCAN_FAILED",
        };
    }
}

/// 安全通告严重级别
pub enum SecurityAdvisoryLevel {
    fatal,
    warn,

    static fn parse(s: []const u8): SecurityAdvisoryLevel! {
        if (bytesEql(s, "fatal")) {
            return SecurityAdvisoryLevel.fatal;
        }
        if (bytesEql(s, "warn")) {
            return SecurityAdvisoryLevel.warn;
        }
        return SecurityScannerError.InvalidLevelValue;
    }

    fn asKeyword(): []const u8 {
        return switch (_) {
            .fatal => "fatal",
            .warn => "warn",
        };
    }

    fn asLabel(): []const u8 {
        return switch (_) {
            .fatal => "FATAL",
            .warn => "WARNING",
        };
    }
}

/// 扫描完成后的安装策略决策
pub enum SecurityInstallDecision {
    /// 无安全通告，允许继续安装
    proceed,
    /// 仅存在 `warn` 级别且处于交互式非 CI 终端，需提示用户 `[y/N]` 确认
    prompt_user,
    /// 存在 `fatal` 级别通告，立即中止安装
    cancel_fatal,
    /// 存在 `warn` 级别但处于 CI 或非 TTY 环境，无法交互确认，直接中止安装
    cancel_non_interactive_warn,
}

/// 待扫描包输入项（用于序列化发送给外部安全扫描器）
pub struct SecurityScanPackageInput {
    name: []const u8 = "",
    version: []const u8 = "",
    tarball: []const u8 = "",
    requested_range: []const u8 = "",
}

/// 单条安全通告（值语义，零堆分配）
pub struct SecurityAdvisory {
    level: SecurityAdvisoryLevel = SecurityAdvisoryLevel.warn,
    package: []const u8 = "",
    url: ([]const u8)? = null,
    description: ([]const u8)? = null,
    ancestors: [SECURITY_SCANNER_MAX_PATH_DEPTH]([]const u8) = [""; SECURITY_SCANNER_MAX_PATH_DEPTH],
    ancestor_count: usize = 0,

    /// 校验通告必填字段是否合法
    fn validate()! {
        if (_.package.len == 0) {
            return SecurityScannerError.EmptyPackageField;
        }
    }

    /// 追加依赖链祖先包名（从直接依赖到父依赖）
    fn addAncestor(*_, name: []const u8)! {
        if (_.ancestor_count >= SECURITY_SCANNER_MAX_PATH_DEPTH) {
            return SecurityScannerError.TooManyPathSegments;
        }
        _.ancestors[_.ancestor_count] = name;
        _.ancestor_count += 1;
    }

    /// 是否为直接依赖（无祖先链路）
    fn isDirectDependency(): bool {
        return _.ancestor_count == 0;
    }
}

/// 安全扫描聚合结果（值语义，零堆分配）
pub struct SecurityScanResults {
    advisories: [SECURITY_SCANNER_MAX_ADVISORIES]SecurityAdvisory = [SecurityAdvisory{}; SECURITY_SCANNER_MAX_ADVISORIES],
    advisory_count: usize = 0,
    fatal_count: usize = 0,
    warn_count: usize = 0,
    packages_scanned: usize = 0,
    duration_ms: i64 = 0,
    scanner_name: []const u8 = "",

    static fn init(scanner_name: []const u8): SecurityScanResults {
        return SecurityScanResults{
            scanner_name: scanner_name,
        };
    }

    fn addAdvisory(*_, advisory: SecurityAdvisory)! {
        advisory.validate()?;
        if (_.advisory_count >= SECURITY_SCANNER_MAX_ADVISORIES) {
            return SecurityScannerError.TooManyAdvisories;
        }
        _.advisories[_.advisory_count] = advisory;
        _.advisory_count += 1;
        switch (advisory.level) {
            .fatal => {
                _.fatal_count += 1;
            },
            .warn => {
                _.warn_count += 1;
            },
        }
    }

    fn count(): usize {
        return _.advisory_count;
    }

    fn advisoryAt(idx: usize): SecurityAdvisory? {
        if (idx >= _.advisory_count) {
            return null;
        }
        return _.advisories[idx];
    }

    fn hasFatalAdvisories(): bool {
        return _.fatal_count > 0;
    }

    fn hasWarnings(): bool {
        return _.warn_count > 0;
    }

    fn hasAdvisories(): bool {
        return _.advisory_count > 0;
    }

    /// 根据通告级别与运行环境（交互式 TTY / CI）判定安装动作
    fn evaluateInstallDecision(is_interactive_tty: bool, is_ci: bool): SecurityInstallDecision {
        if (_.fatal_count > 0) {
            return SecurityInstallDecision.cancel_fatal;
        }
        if (_.warn_count > 0) {
            if (is_ci || !is_interactive_tty) {
                return SecurityInstallDecision.cancel_non_interactive_warn;
            }
            return SecurityInstallDecision.prompt_user;
        }
        return SecurityInstallDecision.proceed;
    }

    /// 格式化单行汇总（对齐 Bun `printSecurityAdvisories` 单复数规则）
    fn formatSummaryLine(out: []u8): usize! {
        let w = BufWriter{ buf: out };
        const total = _.fatal_count + _.warn_count;
        if (total == 0) {
            w.writeStr("0 advisories")?;
            return w.pos;
        }
        if (total == 1) {
            if (_.fatal_count == 1) {
                w.writeStr("1 advisory (1 fatal)")?;
            } else {
                w.writeStr("1 advisory (1 warning)")?;
            }
            return w.pos;
        }

        w.writeUsize(total)?;
        w.writeStr(" advisories (")?;
        if (_.fatal_count > 0 && _.warn_count > 0) {
            w.writeUsize(_.fatal_count)?;
            w.writeStr(" fatal, ")?;
            w.writeUsize(_.warn_count)?;
            w.writeStr(if (_.warn_count == 1) " warning)" else " warnings)")?;
        } else if (_.fatal_count > 0) {
            w.writeUsize(_.fatal_count)?;
            w.writeStr(" fatal)")?;
        } else {
            w.writeUsize(_.warn_count)?;
            w.writeStr(if (_.warn_count == 1) " warning)" else " warnings)")?;
        }
        return w.pos;
    }

    /// 格式化完整多行安全通告报告（纯文本，无 ANSI 转义）
    fn formatReport(out: []u8): usize! {
        let w = BufWriter{ buf: out };
        if (_.advisory_count == 0) {
            return _.formatSummaryLine(out);
        }

        let i: usize = 0;
        while (i < _.advisory_count) {
            const adv = _.advisories[i];
            w.writeStr("  ")?;
            w.writeStr(adv.level.asLabel())?;
            w.writeStr(": ")?;
            w.writeStr(adv.package)?;
            w.writeByte('\n')?;

            if (adv.ancestor_count > 0) {
                w.writeStr("    via ")?;
                let a: usize = 0;
                while (a < adv.ancestor_count) {
                    if (a > 0) {
                        w.writeStr(" > ")?;
                    }
                    w.writeStr(adv.ancestors[a])?;
                    a += 1;
                }
                w.writeStr(" > ")?;
                w.writeStr(adv.package)?;
                w.writeByte('\n')?;
            } else {
                w.writeStr("    (direct dependency)\n")?;
            }

            if (desc := adv.description) {
                if (desc.len > 0) {
                    w.writeStr("    ")?;
                    w.writeStr(desc)?;
                    w.writeByte('\n')?;
                }
            }
            if (u := adv.url) {
                if (u.len > 0) {
                    w.writeStr("    ")?;
                    w.writeStr(u)?;
                    w.writeByte('\n')?;
                }
            }
            w.writeByte('\n')?;
            i += 1;
        }

        const n = _.formatSummaryLine(out[w.pos..])?;
        w.pos += n;
        w.writeByte('\n')?;
        return w.pos;
    }
}

/// 校验外部安全扫描器声明的协议版本（当前仅支持 `"1"`）
pub fn validateScannerProtocolVersion(version: []const u8)! {
    if (!bytesEql(version, SECURITY_SCANNER_PROTOCOL_VERSION)) {
        return SecurityScannerError.InvalidVersion;
    }
}

/// 解析交互式 `[y/N]` 警告确认输入（精准还原 Bun `promptForWarnings` 严格语义）：
/// 仅接受 `"y"`、`"Y"`、`"y\n"`、`"Y\n"`、`"y\r\n"`、`"Y\r\n"`，其他输入均安全返回 `false`。
pub fn parseWarningConfirmation(input: []const u8): bool {
    if (input.len == 0) {
        return false;
    }
    const first = input[0];
    if (first != 'y' && first != 'Y') {
        return false;
    }
    if (input.len == 1) {
        return true;
    }
    if (input.len == 2 && input[1] == '\n') {
        return true;
    }
    if (input.len == 3 && input[1] == '\r' && input[2] == '\n') {
        return true;
    }
    return false;
}

/// 将待扫描包列表序列化为 JSON 请求数组（零堆分配，带 JSON 字符串转义）
pub fn formatScanRequestJson(packages: []const SecurityScanPackageInput, out: []u8): usize! {
    let w = BufWriter{ buf: out };
    w.writeByte('[')?;
    let i: usize = 0;
    while (i < packages.len) {
        if (i > 0) {
            w.writeByte(',')?;
        }
        const pkg = packages[i];
        w.writeStr("{\"name\":")?;
        w.writeJsonString(pkg.name)?;
        w.writeStr(",\"version\":")?;
        w.writeJsonString(pkg.version)?;
        w.writeStr(",\"tarball\":")?;
        w.writeJsonString(pkg.tarball)?;
        w.writeStr(",\"requestedRange\":")?;
        w.writeJsonString(pkg.requested_range)?;
        w.writeByte('}')?;
        i += 1;
    }
    w.writeByte(']')?;
    return w.pos;
}

/// 零堆分配严格解析安全扫描器返回的 JSON 通告列表：
/// 支持顶层数组 `[ { "package": "...", "level": "fatal"|"warn", ... } ]`
/// 或对象包装 `{ "advisories": [ ... ] }`。
pub fn parseSecurityAdvisoriesJson(json: []const u8, scanner_name: []const u8): SecurityScanResults! {
    let cur = JsonCursor{ src: json, pos: 0 };
    cur.skipWs();
    if (cur.pos >= cur.src.len) {
        return SecurityScannerError.InvalidAdvisoriesFormat;
    }

    let results = SecurityScanResults.init(scanner_name);

    if (cur.src[cur.pos] == '{') {
        cur.pos += 1;
        cur.skipWs();
        let found_advisories = false;
        while (cur.pos < cur.src.len && cur.src[cur.pos] != '}') {
            const key = cur.parseStringSlice() catch {
                return SecurityScannerError.InvalidAdvisoriesFormat;
            };
            cur.skipWs();
            if (!cur.consumeByte(':')) {
                return SecurityScannerError.InvalidAdvisoriesFormat;
            }
            cur.skipWs();
            if (bytesEql(key, "advisories")) {
                found_advisories = true;
                parseAdvisoriesArray(&cur, &results)?;
            } else {
                cur.skipValue() catch {
                    return SecurityScannerError.InvalidAdvisoriesFormat;
                };
            }
            cur.skipWs();
            if (cur.consumeByte(',')) {
                cur.skipWs();
                continue;
            }
            break;
        }
        if (!cur.consumeByte('}') || !found_advisories) {
            return SecurityScannerError.InvalidAdvisoriesFormat;
        }
    } else if (cur.src[cur.pos] == '[') {
        parseAdvisoriesArray(&cur, &results)?;
    } else {
        return SecurityScannerError.InvalidAdvisoriesFormat;
    }

    cur.skipWs();
    if (cur.pos != cur.src.len) {
        return SecurityScannerError.InvalidAdvisoriesFormat;
    }

    return results;
}

// ============================================================================
// 内部零分配辅助工具（字符串比较、缓冲区写入与严格 JSON 扫描器）
// ============================================================================

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

struct BufWriter {
    buf: []u8,
    pos: usize = 0,

    fn writeByte(*_, b: u8)! {
        if (_.pos >= _.buf.len) {
            return SecurityScannerError.BufferTooSmall;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn writeStr(*_, s: []const u8)! {
        if (_.pos + s.len > _.buf.len) {
            return SecurityScannerError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos += s.len;
    }

    fn writeUsize(*_, value: usize)! {
        if (value == 0) {
            _.writeByte('0')?;
            return;
        }
        let tmp: [24]u8 = [0; 24];
        let n: usize = 0;
        let v = value;
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
        while (n > 0) {
            n -= 1;
            _.writeByte(tmp[n])?;
        }
    }

    fn writeJsonString(*_, s: []const u8)! {
        _.writeByte('"')?;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c == '"') {
                _.writeStr("\\\"")?;
            } else if (c == '\\') {
                _.writeStr("\\\\")?;
            } else if (c == '\n') {
                _.writeStr("\\n")?;
            } else if (c == '\r') {
                _.writeStr("\\r")?;
            } else if (c == '\t') {
                _.writeStr("\\t")?;
            } else if (c < 0x20) {
                _.writeStr("\\u00")?;
                const hex = "0123456789abcdef";
                _.writeByte(hex[(c >> 4) & 0x0f])?;
                _.writeByte(hex[c & 0x0f])?;
            } else {
                _.writeByte(c)?;
            }
            i += 1;
        }
        _.writeByte('"')?;
    }
}

struct JsonCursor {
    src: []const u8,
    pos: usize = 0,

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn consumeByte(*_, expected: u8): bool {
        if (_.pos < _.src.len && _.src[_.pos] == expected) {
            _.pos += 1;
            return true;
        }
        return false;
    }

    fn consumeLiteral(*_, lit: []const u8): bool {
        if (_.pos + lit.len > _.src.len) {
            return false;
        }
        if (bytesEql(_.src[_.pos.._.pos + lit.len], lit)) {
            _.pos += lit.len;
            return true;
        }
        return false;
    }

    fn parseStringSlice(*_): ([]const u8)! {
        if (!_.consumeByte('"')) {
            return SecurityScannerError.InvalidAdvisoryFormat;
        }
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\\') {
                if (_.pos + 1 >= _.src.len) {
                    return SecurityScannerError.InvalidAdvisoryFormat;
                }
                _.pos += 2;
                continue;
            }
            if (c == '"') {
                const slice = _.src[start.._.pos];
                _.pos += 1;
                return slice;
            }
            if (c < 0x20) {
                return SecurityScannerError.InvalidAdvisoryFormat;
            }
            _.pos += 1;
        }
        return SecurityScannerError.InvalidAdvisoryFormat;
    }

    fn skipValue(*_)! {
        _.skipWs();
        if (_.pos >= _.src.len) {
            return SecurityScannerError.InvalidAdvisoryFormat;
        }
        const c = _.src[_.pos];
        if (c == '"') {
            * = _.parseStringSlice()?;
            return;
        }
        if (c == '{' || c == '[') {
            let depth: usize = 0;
            let in_str = false;
            while (_.pos < _.src.len) {
                const ch = _.src[_.pos];
                if (in_str) {
                    if (ch == '\\') {
                        if (_.pos + 1 >= _.src.len) {
                            return SecurityScannerError.InvalidAdvisoryFormat;
                        }
                        _.pos += 2;
                        continue;
                    }
                    if (ch == '"') {
                        in_str = false;
                    }
                    _.pos += 1;
                    continue;
                }
                if (ch == '"') {
                    in_str = true;
                } else if (ch == '{' || ch == '[') {
                    depth += 1;
                } else if (ch == '}' || ch == ']') {
                    if (depth == 0) {
                        return SecurityScannerError.InvalidAdvisoryFormat;
                    }
                    depth -= 1;
                    _.pos += 1;
                    if (depth == 0) {
                        return;
                    }
                    continue;
                }
                _.pos += 1;
            }
            return SecurityScannerError.InvalidAdvisoryFormat;
        }
        if (_.consumeLiteral("true") || _.consumeLiteral("false") || _.consumeLiteral("null")) {
            return;
        }
        // Number token
        const num_start = _.pos;
        if (_.pos < _.src.len && _.src[_.pos] == '-') {
            _.pos += 1;
        }
        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if ((ch >= '0' && ch <= '9') || ch == '.' || ch == 'e' || ch == 'E' || ch == '+' || ch == '-') {
                _.pos += 1;
            } else {
                break;
            }
        }
        if (_.pos == num_start) {
            return SecurityScannerError.InvalidAdvisoryFormat;
        }
    }
}

fn parseAdvisoriesArray(cur: *JsonCursor, results: *SecurityScanResults)! {
    if (!cur.consumeByte('[')) {
        return SecurityScannerError.InvalidAdvisoriesFormat;
    }
    cur.skipWs();
    if (cur.consumeByte(']')) {
        return;
    }

    while (true) {
        cur.skipWs();
        const adv = parseSingleAdvisoryObject(cur)?;
        results.addAdvisory(adv)?;
        cur.skipWs();
        if (cur.consumeByte(',')) {
            continue;
        }
        if (cur.consumeByte(']')) {
            return;
        }
        return SecurityScannerError.InvalidAdvisoriesFormat;
    }
}

fn parseSingleAdvisoryObject(cur: *JsonCursor): SecurityAdvisory! {
    if (!cur.consumeByte('{')) {
        return SecurityScannerError.InvalidAdvisoryFormat;
    }
    cur.skipWs();

    let has_package = false;
    let has_level = false;
    let pkg_name: []const u8 = "";
    let level = SecurityAdvisoryLevel.warn;
    let desc: ([]const u8)? = null;
    let url: ([]const u8)? = null;

    while (cur.pos < cur.src.len && cur.src[cur.pos] != '}') {
        const key = cur.parseStringSlice() catch {
            return SecurityScannerError.InvalidAdvisoryFormat;
        };
        cur.skipWs();
        if (!cur.consumeByte(':')) {
            return SecurityScannerError.InvalidAdvisoryFormat;
        }
        cur.skipWs();

        if (bytesEql(key, "package")) {
            if (cur.pos >= cur.src.len || cur.src[cur.pos] != '"') {
                return SecurityScannerError.InvalidPackageField;
            }
            const val = cur.parseStringSlice()?;
            if (val.len == 0) {
                return SecurityScannerError.EmptyPackageField;
            }
            pkg_name = val;
            has_package = true;
        } else if (bytesEql(key, "level")) {
            if (cur.pos >= cur.src.len || cur.src[cur.pos] != '"') {
                return SecurityScannerError.InvalidLevelValue;
            }
            const val = cur.parseStringSlice()?;
            level = SecurityAdvisoryLevel.parse(val)?;
            has_level = true;
        } else if (bytesEql(key, "description")) {
            if (cur.consumeLiteral("null")) {
                desc = null;
            } else if (cur.pos < cur.src.len && cur.src[cur.pos] == '"') {
                desc = cur.parseStringSlice()?;
            } else {
                return SecurityScannerError.InvalidDescriptionField;
            }
        } else if (bytesEql(key, "url")) {
            if (cur.consumeLiteral("null")) {
                url = null;
            } else if (cur.pos < cur.src.len && cur.src[cur.pos] == '"') {
                url = cur.parseStringSlice()?;
            } else {
                return SecurityScannerError.InvalidUrlField;
            }
        } else {
            cur.skipValue()?;
        }

        cur.skipWs();
        if (cur.consumeByte(',')) {
            cur.skipWs();
            continue;
        }
        break;
    }

    if (!cur.consumeByte('}')) {
        return SecurityScannerError.InvalidAdvisoryFormat;
    }
    if (!has_package) {
        return SecurityScannerError.MissingPackageField;
    }
    if (!has_level) {
        return SecurityScannerError.MissingLevelField;
    }

    return SecurityAdvisory{
        level: level,
        package: pkg_name,
        url: url,
        description: desc,
    };
}
