// Origin: Pure-CP Stdlib (Zero-Alloc JUnit XML CI Test Report Parser) | Created: 2026-11-22

/// 单个测试用例执行结果状态
pub enum JunitOutcome: u8 {
    passed,
    failed,
    errored,
    skipped,
}

/// JUnit <testsuite> 套件记录（零拷贝切片）
pub struct JunitSuite {
    name: []const u8,
    tests: u32,
    failures: u32,
    errors: u32,
    skipped: u32,
    time_sec: f64,
    timestamp: []const u8,
    raw_xml: []const u8,

    /// 返回该套件内全部 <testcase> 的零分配迭代器
    fn cases(): JunitCaseIter {
        return JunitCaseIter.init(_.raw_xml);
    }
}

/// JUnit <testcase> 用例记录（零拷贝切片）
pub struct JunitCase {
    name: []const u8,
    classname: []const u8,
    time_sec: f64,
    outcome: JunitOutcome,
    message: []const u8,
    fail_type: []const u8,
    detail_text: []const u8,
}

/// JUnit XML 报告单遍统计汇总
pub struct JunitSummary {
    suite_count: u32,
    total: u32,
    passed: u32,
    failed: u32,
    errored: u32,
    skipped: u32,
    total_time_sec: f64,

    /// 是否全部通过（无 failed 且无 errored）
    fn allPassed(): bool {
        return _.failed == 0 && _.errored == 0;
    }
}

// ---------------------------------------------------------------------------
// 内部零拷贝 XML 扫描、属性提取与数值解析辅助函数
// ---------------------------------------------------------------------------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isTagBoundary(c: u8): bool {
    return isWs(c) || c == '>' || c == '/';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
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

fn startsWithAt(haystack: []const u8, pos: usize, needle: []const u8): bool {
    if (pos + needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (haystack[pos + i] != needle[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn skipBom(data: []const u8): usize {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return 3;
    }
    return 0;
}

fn findSubstrFrom(haystack: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0) {
        return start;
    }
    if (start + needle.len > haystack.len) {
        return null;
    }
    let i = start;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (startsWithAt(haystack, i, needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 跳过从 '<' 开始的注释、CDATA、XML 声明或 DOCTYPE，成功返回跳过后的新位置
fn skipXmlSpecial(data: []const u8, p: usize): usize? {
    if (startsWithAt(data, p, "<!--")) {
        if (end := findSubstrFrom(data, p + 4, "-->")) {
            return end + 3;
        }
        return data.len;
    }
    if (startsWithAt(data, p, "<![CDATA[")) {
        if (end := findSubstrFrom(data, p + 9, "]]>")) {
            return end + 3;
        }
        return data.len;
    }
    if (startsWithAt(data, p, "<?")) {
        if (end := findSubstrFrom(data, p + 2, "?>")) {
            return end + 2;
        }
        return data.len;
    }
    if (startsWithAt(data, p, "<!")) {
        let i = p + 2;
        while (i < data.len && data[i] != '>') {
            i += 1;
        }
        return if (i < data.len) (i + 1) else data.len;
    }
    return null;
}

/// 定位开标签 '<tag ...>' 的闭合 '>' 位置（跳过引号内字符），返回 '>' 偏移量
fn findOpenTagClose(data: []const u8, start: usize): usize? {
    let i = start;
    let quote: u8 = 0;
    while (i < data.len) {
        const c = data[i];
        if (quote != 0) {
            if (c == quote) {
                quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                quote = c;
            } else if (c == '>') {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

/// 判断开标签在 data[pos..] 是否精确匹配指定标签名（如 "testsuite" 不匹配 "testsuites"）
fn matchOpenTag(data: []const u8, pos: usize, tag: []const u8): bool {
    if (pos + 1 + tag.len >= data.len) {
        return false;
    }
    if (data[pos] != '<') {
        return false;
    }
    if (!startsWithAt(data, pos + 1, tag)) {
        return false;
    }
    return isTagBoundary(data[pos + 1 + tag.len]);
}

/// 判断闭合标签在 data[pos..] 是否精确匹配 "</tag>"
fn matchCloseTag(data: []const u8, pos: usize, tag: []const u8): usize? {
    if (pos + 2 + tag.len >= data.len) {
        return null;
    }
    if (data[pos] != '<' || data[pos + 1] != '/') {
        return null;
    }
    if (!startsWithAt(data, pos + 2, tag)) {
        return null;
    }
    let i = pos + 2 + tag.len;
    while (i < data.len && isWs(data[i])) {
        i += 1;
    }
    if (i < data.len && data[i] == '>') {
        return i + 1;
    }
    return null;
}

/// 从开标签正文起始处查找匹配的闭合标签（支持同名嵌套、安全跳过注释与 CDATA），返回 (inner_end, after_close)
struct CloseTagSpan {
    inner_end: usize,
    after_close: usize,
}

fn findMatchingClose(data: []const u8, body_start: usize, tag: []const u8): CloseTagSpan? {
    let pos = body_start;
    let depth: u32 = 1;
    while (pos < data.len) {
        if (data[pos] != '<') {
            pos += 1;
            continue;
        }
        if (next_p := skipXmlSpecial(data, pos)) {
            pos = next_p;
            continue;
        }
        if (after := matchCloseTag(data, pos, tag)) {
            depth -= 1;
            if (depth == 0) {
                return CloseTagSpan{
                    inner_end: pos,
                    after_close: after,
                };
            }
            pos = after;
            continue;
        }
        if (matchOpenTag(data, pos, tag)) {
            const gt = findOpenTagClose(data, pos + 1 + tag.len) ?? return null;
            const self_closing = gt > pos && data[gt - 1] == '/';
            if (!self_closing) {
                depth += 1;
            }
            pos = gt + 1;
            continue;
        }
        pos += 1;
    }
    return null;
}

/// 从开标签属性切片中零拷贝提取指定属性值（支持双引号与单引号）
fn getXmlAttr(attrs: []const u8, name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < attrs.len) {
        while (i < attrs.len && (isWs(attrs[i]) || attrs[i] == '/')) {
            i += 1;
        }
        if (i >= attrs.len) {
            break;
        }
        const key_start = i;
        while (i < attrs.len && !isWs(attrs[i]) && attrs[i] != '=' && attrs[i] != '/') {
            i += 1;
        }
        const key = attrs[key_start..i];
        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] != '=') {
            continue;
        }
        i += 1;
        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            break;
        }
        const q = attrs[i];
        if (q != '"' && q != '\'') {
            while (i < attrs.len && !isWs(attrs[i])) {
                i += 1;
            }
            continue;
        }
        i += 1;
        const val_start = i;
        while (i < attrs.len && attrs[i] != q) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const val = attrs[val_start..i];
        i += 1;
        if (eqlBytes(key, name)) {
            return val;
        }
    }
    return null;
}

/// 剥离首尾空白与可选的外层 <![CDATA[...]]>
fn stripCdata(raw: []const u8): []const u8 {
    const trimmed = trimWs(raw);
    if (trimmed.len >= 12 && startsWithAt(trimmed, 0, "<![CDATA[")) {
        const end_off = trimmed.len - 3;
        if (startsWithAt(trimmed, end_off, "]]>")) {
            return trimmed[9..end_off];
        }
    }
    return trimmed;
}

fn parseU32OrZero(raw: []const u8): u32 {
    const s = trimWs(raw);
    if (s.len == 0) {
        return 0;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return 0;
        }
        acc = acc * 10 + u64(s[i] - '0');
        if (acc > 4294967295) {
            return 0;
        }
        i += 1;
    }
    return acc as u32;
}

fn parseF64OrZero(raw: []const u8): f64 {
    const s = trimWs(raw);
    if (s.len == 0) {
        return 0.0;
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return 0.0;
    }
    let val: f64 = 0.0;
    let digits: u32 = 0;
    while (i < s.len && isDigit(s[i])) {
        val = val * 10.0 + f64(s[i] - '0');
        digits += 1;
        i += 1;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        let frac: f64 = 0.1;
        while (i < s.len && isDigit(s[i])) {
            val += f64(s[i] - '0') * frac;
            frac *= 0.1;
            digits += 1;
            i += 1;
        }
    }
    if (digits == 0) {
        return 0.0;
    }
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || !isDigit(s[i])) {
            return 0.0;
        }
        let exp_val: i32 = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + i32(s[i] - '0');
            }
            i += 1;
        }
        if (exp_val > 308) {
            return 0.0;
        }
        let scale: f64 = 1.0;
        let p: i32 = 0;
        while (p < exp_val) {
            scale *= 10.0;
            p += 1;
        }
        if (exp_neg) {
            val /= scale;
        } else {
            val *= scale;
        }
    }
    if (i != s.len) {
        return 0.0;
    }
    return if (neg) -val else val;
}

/// 在 <testcase> 内部扫描 <failure> / <error> / <skipped> 子节点
fn inspectCaseBody(body: []const u8, out_case: *JunitCase) {
    let pos: usize = 0;
    while (pos < body.len) {
        if (body[pos] != '<') {
            pos += 1;
            continue;
        }
        if (next_p := skipXmlSpecial(body, pos)) {
            pos = next_p;
            continue;
        }

        let tag_name = body[0..0];
        let outcome = JunitOutcome.passed;
        if (matchOpenTag(body, pos, "failure")) {
            tag_name = "failure";
            outcome = JunitOutcome.failed;
        } else if (matchOpenTag(body, pos, "error")) {
            tag_name = "error";
            outcome = JunitOutcome.errored;
        } else if (matchOpenTag(body, pos, "skipped")) {
            tag_name = "skipped";
            outcome = JunitOutcome.skipped;
        } else {
            pos += 1;
            continue;
        }

        const gt = findOpenTagClose(body, pos + 1 + tag_name.len) ?? break;
        const self_closing = gt > pos && body[gt - 1] == '/';
        const attr_end = if (self_closing) (gt - 1) else gt;
        const attrs = body[pos + 1 + tag_name.len..attr_end];

        out_case.outcome = outcome;
        if (msg := getXmlAttr(attrs, "message")) {
            out_case.message = msg;
        }
        if (ft := getXmlAttr(attrs, "type")) {
            out_case.fail_type = ft;
        }

        if (!self_closing) {
            if (span := findMatchingClose(body, gt + 1, tag_name)) {
                out_case.detail_text = stripCdata(body[gt + 1..span.inner_end]);
            }
        }
        return;
    }
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 快速检测缓冲区是否为 JUnit XML 报告（跳过可选 BOM / XML 声明 / 注释，检测 <testsuites 或 <testsuite）
pub fn isJunitXml(data: []const u8): bool {
    let pos = skipBom(data);
    while (pos < data.len) {
        while (pos < data.len && isWs(data[pos])) {
            pos += 1;
        }
        if (pos >= data.len || data[pos] != '<') {
            return false;
        }
        if (next_p := skipXmlSpecial(data, pos)) {
            pos = next_p;
            continue;
        }
        return matchOpenTag(data, pos, "testsuites") || matchOpenTag(data, pos, "testsuite");
    }
    return false;
}

/// 零分配迭代全部 <testsuite ...>...</testsuite>（兼容 <testsuites> 包裹、单顶层与自闭合空套件）
pub struct JunitSuiteIter {
    data: []const u8,
    pos: usize,
    malformed: bool,

    static fn init(data: []const u8): JunitSuiteIter {
        return JunitSuiteIter{
            data: data,
            pos: skipBom(data),
            malformed: false,
        };
    }

    fn next(*_): JunitSuite? {
        if (_.malformed) {
            return null;
        }
        while (_.pos < _.data.len) {
            if (_.data[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (next_p := skipXmlSpecial(_.data, _.pos)) {
                _.pos = next_p;
                continue;
            }
            if (!matchOpenTag(_.data, _.pos, "testsuite")) {
                _.pos += 1;
                continue;
            }

            const tag_start = _.pos;
            const gt_opt = findOpenTagClose(_.data, tag_start + 10);
            if (gt_opt == null) {
                _.malformed = true;
                return null;
            }
            const gt = gt_opt ?? unreachable;
            const self_closing = gt > tag_start && _.data[gt - 1] == '/';
            const attr_end = if (self_closing) (gt - 1) else gt;
            const attrs = _.data[tag_start + 10..attr_end];

            let raw_body = _.data[0..0];
            if (self_closing) {
                _.pos = gt + 1;
            } else {
                if (span := findMatchingClose(_.data, gt + 1, "testsuite")) {
                    raw_body = _.data[gt + 1..span.inner_end];
                    _.pos = span.after_close;
                } else {
                    _.malformed = true;
                    return null;
                }
            }

            const empty = _.data[0..0];
            return JunitSuite{
                name: getXmlAttr(attrs, "name") ?? empty,
                tests: parseU32OrZero(getXmlAttr(attrs, "tests") ?? empty),
                failures: parseU32OrZero(getXmlAttr(attrs, "failures") ?? empty),
                errors: parseU32OrZero(getXmlAttr(attrs, "errors") ?? empty),
                skipped: parseU32OrZero(getXmlAttr(attrs, "skipped") ?? empty),
                time_sec: parseF64OrZero(getXmlAttr(attrs, "time") ?? empty),
                timestamp: getXmlAttr(attrs, "timestamp") ?? empty,
                raw_xml: raw_body,
            };
        }
        return null;
    }
}

/// 零分配迭代 <testcase ...>（支持自闭合通过用例与含 failure/error/skipped/system-out 的完整用例）
pub struct JunitCaseIter {
    data: []const u8,
    pos: usize,
    malformed: bool,

    static fn init(suite_body: []const u8): JunitCaseIter {
        return JunitCaseIter{
            data: suite_body,
            pos: skipBom(suite_body),
            malformed: false,
        };
    }

    fn next(*_): JunitCase? {
        if (_.malformed) {
            return null;
        }
        while (_.pos < _.data.len) {
            if (_.data[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (next_p := skipXmlSpecial(_.data, _.pos)) {
                _.pos = next_p;
                continue;
            }
            if (!matchOpenTag(_.data, _.pos, "testcase")) {
                _.pos += 1;
                continue;
            }

            const tag_start = _.pos;
            const gt_opt = findOpenTagClose(_.data, tag_start + 9);
            if (gt_opt == null) {
                _.malformed = true;
                return null;
            }
            const gt = gt_opt ?? unreachable;
            const self_closing = gt > tag_start && _.data[gt - 1] == '/';
            const attr_end = if (self_closing) (gt - 1) else gt;
            const attrs = _.data[tag_start + 9..attr_end];

            const empty = _.data[0..0];
            let tc = JunitCase{
                name: getXmlAttr(attrs, "name") ?? empty,
                classname: getXmlAttr(attrs, "classname") ?? empty,
                time_sec: parseF64OrZero(getXmlAttr(attrs, "time") ?? empty),
                outcome: JunitOutcome.passed,
                message: empty,
                fail_type: empty,
                detail_text: empty,
            };

            if (self_closing) {
                _.pos = gt + 1;
            } else {
                if (span := findMatchingClose(_.data, gt + 1, "testcase")) {
                    inspectCaseBody(_.data[gt + 1..span.inner_end], *tc);
                    _.pos = span.after_close;
                } else {
                    _.malformed = true;
                    return null;
                }
            }

            return tc;
        }
        return null;
    }
}

/// 单遍统计 JUnit XML 测试报告的套件数、四态用例计数与总耗时
pub fn summarizeJunit(data: []const u8): JunitSummary {
    let summary = JunitSummary{
        suite_count: 0,
        total: 0,
        passed: 0,
        failed: 0,
        errored: 0,
        skipped: 0,
        total_time_sec: 0.0,
    };

    let sit = JunitSuiteIter.init(data);
    while (suite := sit.next()) {
        summary.suite_count += 1;
        let cases_time: f64 = 0.0;

        let cit = suite.cases();
        while (tc := cit.next()) {
            summary.total += 1;
            cases_time += tc.time_sec;
            if (tc.outcome == JunitOutcome.passed) {
                summary.passed += 1;
            } else if (tc.outcome == JunitOutcome.failed) {
                summary.failed += 1;
            } else if (tc.outcome == JunitOutcome.errored) {
                summary.errored += 1;
            } else if (tc.outcome == JunitOutcome.skipped) {
                summary.skipped += 1;
            }
        }

        if (suite.time_sec > 0.0) {
            summary.total_time_sec += suite.time_sec;
        } else {
            summary.total_time_sec += cases_time;
        }
    }

    return summary;
}
