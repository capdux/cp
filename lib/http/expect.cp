// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T16:45:50Z (RFC 9110 HTTP Expect Header Parser & 100-continue Negotiator)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn unquoteExact(s: []const u8): ([]const u8)? {
    if (s.len < 2 || s[0] != '"') {
        return null;
    }
    let i: usize = 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '\\') {
            if (i + 1 >= s.len) {
                return null;
            }
            i += 2;
            continue;
        }
        if (c == '"') {
            if (i + 1 != s.len) {
                return null;
            }
            return s[1..i];
        }
        i += 1;
    }
    return null;
}

/// RFC 9110 §10.1.1 单个 Expect 期望项（兼容 RFC 7231 `token=value` / `token="quoted"` 扩展）
export struct ExpectItem {
    token: []const u8;
    value: ([]const u8)?;

    /// 判断是否为标准 `100-continue` 期望（大小写不敏感且不带参数值）
    fn isContinue(): bool {
        return _.value == null && eqlIgnoreCase(_.token, "100-continue");
    }
}

/// RFC 9110 `Expect` 头部逗号分隔零分配流式迭代器（支持引号感知与空项过滤）
export struct ExpectIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): ExpectIter {
        return ExpectIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ExpectItem? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            let in_quote = false;
            let escaped = false;

            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_quote = false;
                    }
                } else {
                    if (c == '"') {
                        in_quote = true;
                    } else if (c == ',') {
                        break;
                    }
                }
                _.pos += 1;
            }

            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0) {
                continue;
            }

            // 未闭合引号按 Fail-Closed 作为未知期望项返回，由协商器触发 417
            if (in_quote) {
                return ExpectItem{
                    token: seg,
                    value: null,
                };
            }

            let eq_pos: usize = 0;
            let found_eq = false;
            while (eq_pos < seg.len) {
                if (seg[eq_pos] == '=') {
                    found_eq = true;
                    break;
                }
                eq_pos += 1;
            }

            if (!found_eq) {
                return ExpectItem{
                    token: seg,
                    value: null,
                };
            }

            const tok = trimAscii(seg[0..eq_pos]);
            if (tok.len == 0) {
                return ExpectItem{
                    token: seg,
                    value: null,
                };
            }

            const raw_val = trimAscii(seg[(eq_pos + 1)..]);
            let val: []const u8 = raw_val;
            if (unquoted := unquoteExact(raw_val)) {
                val = unquoted;
            }
            return ExpectItem{
                token: tok,
                value: val,
            };
        }
        return null;
    }
}

/// RFC 9110 §10.1.1 服务端对 `Expect` 头的协商决策
export enum ExpectDecision {
    none,
    send_100_continue,
    expectation_failed_417,
}

/// 检查 `Expect` 头中是否包含标准 `100-continue` 期望
export fn hasContinueExpectation(header: []const u8): bool {
    let it = ExpectIter.init(header);
    while (item := it.next()) {
        if (item.isContinue()) {
            return true;
        }
    }
    return false;
}

/// 检查 `Expect` 头中是否包含任何非 `100-continue` 的未知期望项
export fn hasUnknownExpectation(header: []const u8): bool {
    let it = ExpectIter.init(header);
    while (item := it.next()) {
        if (!item.isContinue()) {
            return true;
        }
    }
    return false;
}

/// 按 RFC 9110 §10.1.1 评估 `Expect` 请求头：
/// - 存在任何未知期望 -> `expectation_failed_417`
/// - 空头 / 仅空白逗号 -> `none`
/// - HTTP/1.0 或无请求体 -> `none`
/// - HTTP/1.1+ 且有请求体且含 `100-continue` -> `send_100_continue`
export fn evaluateExpectHeader(
    header: []const u8,
    has_body: bool,
    http11_or_newer: bool,
): ExpectDecision {
    let saw_continue = false;
    let it = ExpectIter.init(header);
    while (item := it.next()) {
        if (item.isContinue()) {
            saw_continue = true;
        } else {
            return ExpectDecision.expectation_failed_417;
        }
    }
    if (!saw_continue || !http11_or_newer || !has_body) {
        return ExpectDecision.none;
    }
    return ExpectDecision.send_100_continue;
}
