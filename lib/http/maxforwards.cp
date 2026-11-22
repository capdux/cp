// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-22T12:49:17Z (RFC 9110 Max-Forwards Hop Limiter)

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) {
        start += 1;
    }
    while (end > start && isOws(s[end - 1])) {
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

/// RFC 9110 §7.6.2 `Max-Forwards` 代理裁定动作
export enum MaxForwardsAction {
    forward,
    respond_directly,
    bad_request,
}

/// `Max-Forwards` 评估结果：包含裁定动作与递减后的下一跳值
export struct MaxForwardsResult {
    action: MaxForwardsAction;
    next_value: u32;
}

/// 严格按 RFC 9110 §7.6.2 解析 `Max-Forwards = 1*DIGIT`（允许首尾可选 OWS），
/// 拒绝负数、`+` 号、空白夹杂、非数字及 `u32` 溢出
export fn parseMaxForwards(header: []const u8): u32? {
    const trimmed = trimOws(header);
    if (trimmed.len == 0) {
        return null;
    }

    const max_div10: u32 = 429496729;
    const max_mod10: u32 = 5;

    let acc: u32 = 0;
    let i: usize = 0;
    while (i < trimmed.len) {
        const c = trimmed[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u32 = u32(c - '0');
        if (acc > max_div10 || (acc == max_div10 && digit > max_mod10)) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

/// RFC 9110 §7.6.2 代理跳数裁定器：
/// - 仅对 `TRACE` 和 `OPTIONS` 生效；其他方法忽略并返回 `forward`
/// - 未携带头部返回 `forward`
/// - 畸形头部返回 `bad_request`
/// - 值为 `0` 返回 `respond_directly`
/// - 值 `> 0` 返回 `forward` 且 `next_value = val - 1`
export fn evaluateMaxForwards(method: []const u8, header: ([]const u8)?): MaxForwardsResult {
    if (!eqlBytes(method, "TRACE") && !eqlBytes(method, "OPTIONS")) {
        return MaxForwardsResult{
            action: MaxForwardsAction.forward,
            next_value: 0,
        };
    }

    if (hdr := header) {
        if (val := parseMaxForwards(hdr)) {
            if (val == 0) {
                return MaxForwardsResult{
                    action: MaxForwardsAction.respond_directly,
                    next_value: 0,
                };
            }
            return MaxForwardsResult{
                action: MaxForwardsAction.forward,
                next_value: val - 1,
            };
        }
        return MaxForwardsResult{
            action: MaxForwardsAction.bad_request,
            next_value: 0,
        };
    }

    return MaxForwardsResult{
        action: MaxForwardsAction.forward,
        next_value: 0,
    };
}

/// 无前导零十进制格式化 `Max-Forwards` 值，输出缓冲区不足时返回 `null`
export fn formatMaxForwards(val: u32, out: []u8): ([]const u8)? {
    if (val == 0) {
        if (out.len < 1) {
            return null;
        }
        out[0] = '0';
        return out[0..1];
    }

    let tmp: [10]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let n: u32 = val;
    let len: usize = 0;
    while (n > 0) {
        tmp[len] = u8('0' + (n % 10));
        n /= 10;
        len += 1;
    }

    if (out.len < len) {
        return null;
    }

    let i: usize = 0;
    while (i < len) {
        out[i] = tmp[len - 1 - i];
        i += 1;
    }
    return out[0..len];
}
