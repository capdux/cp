// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T20:23:41Z (RFC 9421 HTTP Message Signatures Header Parser)

const I64_MAX: i64 = 9223372036854775807;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
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

fn isValidSfKeyStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || c == '*';
}

fn isValidSfKeyChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '_'
        || c == '-'
        || c == '.'
        || c == '*';
}

fn isValidSfKey(s: []const u8): bool {
    if (s.len == 0 || !isValidSfKeyStart(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!isValidSfKeyChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isBase64Char(c: u8): bool {
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '+'
        || c == '/'
        || c == '=';
}

fn parseQuotedStringExact(s: []const u8): ([]const u8)? {
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

fn parseI64(s: []const u8): i64? {
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
        if (i >= s.len) {
            return null;
        }
    }
    let acc: u64 = 0;
    const limit: u64 = if (neg) u64(9223372036854775808) else u64(9223372036854775807);
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = u64(c - '0');
        if (acc > (limit - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    if (neg) {
        if (acc == u64(9223372036854775808)) {
            return -9223372036854775807 - 1;
        }
        return -(acc as i64);
    }
    return acc as i64;
}

/// RFC 9421 `Signature-Input` 单条解析结果（零堆分配切片视图）
export struct SignatureInput {
    label: []const u8;
    components_raw: []const u8;
    created: i64?;
    expires: i64?;
    nonce: ([]const u8)?;
    alg: ([]const u8)?;
    keyid: ([]const u8)?;
    tag: ([]const u8)?;

    /// 校验签名在给定时间戳 `now_unix_sec` 与时钟容忍窗口 `leeway_sec` 内是否有效
    fn isActiveAt(now_unix_sec: i64, leeway_sec: i64): bool {
        const tol: i64 = if (leeway_sec < 0) 0 else leeway_sec;

        if (c := _.created) {
            if (e := _.expires) {
                if (c > e) {
                    return false;
                }
            }
        }

        if (c := _.created) {
            if (now_unix_sec <= I64_MAX - tol) {
                if (c > now_unix_sec + tol) {
                    return false;
                }
            }
        }

        if (e := _.expires) {
            if (e <= I64_MAX - tol) {
                if (now_unix_sec > e + tol) {
                    return false;
                }
            }
        }

        return true;
    }
}

/// RFC 9421 覆盖组件列表流式迭代器（提取每个带引号的组件名，跳过组件后缀参数）
export struct ComponentIter {
    src: []const u8;
    pos: usize;

    fn init(components_raw: []const u8): ComponentIter {
        return ComponentIter{
            src: components_raw,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            while (_.pos < _.src.len && isSpace(_.src[_.pos])) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len) {
                return null;
            }
            if (_.src[_.pos] != '"') {
                // 跳过非引号畸形 token 直至空白
                while (_.pos < _.src.len && !isSpace(_.src[_.pos])) {
                    _.pos += 1;
                }
                continue;
            }

            // 解析带引号的组件名
            _.pos += 1;
            const start = _.pos;
            let closed = false;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (c == '\\') {
                    if (_.pos + 1 >= _.src.len) {
                        _.pos = _.src.len;
                        return null;
                    }
                    _.pos += 2;
                    continue;
                }
                if (c == '"') {
                    closed = true;
                    break;
                }
                _.pos += 1;
            }
            if (!closed) {
                _.pos = _.src.len;
                return null;
            }
            const name = _.src[start.._.pos];
            _.pos += 1;

            // 跳过该组件可能携带的参数（如 ;req ;sf ;name="id"）
            if (_.pos < _.src.len && _.src[_.pos] == ';') {
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
                        } else if (isSpace(c)) {
                            break;
                        }
                    }
                    _.pos += 1;
                }
            }

            return name;
        }
        return null;
    }
}

fn parseSignatureInputEntry(seg: []const u8): SignatureInput? {
    // 1. 解析 label = (...)
    let eq_pos: usize = 0;
    let found_eq = false;
    while (eq_pos < seg.len) {
        if (seg[eq_pos] == '=') {
            found_eq = true;
            break;
        }
        if (seg[eq_pos] == '(' || seg[eq_pos] == ';' || seg[eq_pos] == '"') {
            return null;
        }
        eq_pos += 1;
    }
    if (!found_eq) {
        return null;
    }

    const label = trimAscii(seg[0..eq_pos]);
    if (!isValidSfKey(label)) {
        return null;
    }

    let i: usize = eq_pos + 1;
    while (i < seg.len && isSpace(seg[i])) {
        i += 1;
    }
    if (i >= seg.len || seg[i] != '(') {
        return null;
    }

    // 2. 提取括号内覆盖组件列表（引号感知）
    const comp_start = i + 1;
    i = comp_start;
    let in_quote = false;
    let escaped = false;
    let found_close = false;
    while (i < seg.len) {
        const c = seg[i];
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
            } else if (c == ')') {
                found_close = true;
                break;
            }
        }
        i += 1;
    }
    if (!found_close || in_quote) {
        return null;
    }

    const components_raw = trimAscii(seg[comp_start..i]);
    i += 1;

    // 3. 解析可选分号参数（created / expires / nonce / alg / keyid / tag）
    let created: i64? = null;
    let expires: i64? = null;
    let nonce: ([]const u8)? = null;
    let alg: ([]const u8)? = null;
    let keyid: ([]const u8)? = null;
    let tag: ([]const u8)? = null;

    while (i < seg.len && isSpace(seg[i])) {
        i += 1;
    }
    if (i < seg.len && seg[i] != ';') {
        return null;
    }

    while (i < seg.len) {
        if (seg[i] == ';') {
            i += 1;
        }
        const p_start = i;
        let p_in_quote = false;
        let p_escaped = false;
        while (i < seg.len) {
            const c = seg[i];
            if (p_in_quote) {
                if (p_escaped) {
                    p_escaped = false;
                } else if (c == '\\') {
                    p_escaped = true;
                } else if (c == '"') {
                    p_in_quote = false;
                }
            } else {
                if (c == '"') {
                    p_in_quote = true;
                } else if (c == ';') {
                    break;
                }
            }
            i += 1;
        }
        if (p_in_quote) {
            return null;
        }

        const param = trimAscii(seg[p_start..i]);
        if (param.len == 0) {
            continue;
        }

        let peq: usize = 0;
        let has_eq = false;
        while (peq < param.len) {
            if (param[peq] == '=') {
                has_eq = true;
                break;
            }
            peq += 1;
        }
        if (!has_eq) {
            // 无值布尔参数，合法但非标准字段，忽略
            continue;
        }

        const pkey = trimAscii(param[0..peq]);
        const pval = trimAscii(param[(peq + 1)..]);

        if (eqlBytes(pkey, "created")) {
            if (v := parseI64(pval)) {
                created = v;
            } else {
                return null;
            }
        } else if (eqlBytes(pkey, "expires")) {
            if (v := parseI64(pval)) {
                expires = v;
            } else {
                return null;
            }
        } else if (eqlBytes(pkey, "nonce")) {
            if (v := parseQuotedStringExact(pval)) {
                nonce = v;
            } else {
                return null;
            }
        } else if (eqlBytes(pkey, "alg")) {
            if (v := parseQuotedStringExact(pval)) {
                alg = v;
            } else {
                return null;
            }
        } else if (eqlBytes(pkey, "keyid")) {
            if (v := parseQuotedStringExact(pval)) {
                keyid = v;
            } else {
                return null;
            }
        } else if (eqlBytes(pkey, "tag")) {
            if (v := parseQuotedStringExact(pval)) {
                tag = v;
            } else {
                return null;
            }
        }
    }

    return SignatureInput{
        label: label,
        components_raw: components_raw,
        created: created,
        expires: expires,
        nonce: nonce,
        alg: alg,
        keyid: keyid,
        tag: tag,
    };
}

/// RFC 9421 `Signature-Input` 头部多签名流式迭代器（逗号分隔、括号与引号感知）
export struct SignatureInputIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): SignatureInputIter {
        return SignatureInputIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): SignatureInput? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            let in_quote = false;
            let escaped = false;
            let paren_depth: usize = 0;

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
                    } else if (c == '(') {
                        paren_depth += 1;
                    } else if (c == ')') {
                        if (paren_depth > 0) {
                            paren_depth -= 1;
                        }
                    } else if (c == ',' && paren_depth == 0) {
                        break;
                    }
                }
                _.pos += 1;
            }

            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0 || in_quote || paren_depth != 0) {
                continue;
            }

            if (item := parseSignatureInputEntry(seg)) {
                return item;
            }
        }
        return null;
    }
}

/// 按标签名从 `Signature-Input` 头中查找对应的 `SignatureInput`
export fn findSignatureInput(header: []const u8, label: []const u8): SignatureInput? {
    if (label.len == 0) {
        return null;
    }
    let it = SignatureInputIter.init(header);
    while (item := it.next()) {
        if (eqlBytes(item.label, label)) {
            return item;
        }
    }
    return null;
}

/// 从 RFC 9421 `Signature` 头（如 `sig1=:base64:`）中提取指定标签的 Base64 字节序列
export fn findSignatureValue(sig_header: []const u8, label: []const u8): ([]const u8)? {
    if (label.len == 0) {
        return null;
    }

    let pos: usize = 0;
    while (pos < sig_header.len) {
        const seg_start = pos;
        let in_quote = false;
        let escaped = false;
        let in_bseq = false;

        while (pos < sig_header.len) {
            const c = sig_header[pos];
            if (in_quote) {
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_quote = false;
                }
            } else if (in_bseq) {
                if (c == ':') {
                    in_bseq = false;
                }
            } else {
                if (c == '"') {
                    in_quote = true;
                } else if (c == ':') {
                    in_bseq = true;
                } else if (c == ',') {
                    break;
                }
            }
            pos += 1;
        }

        const seg = trimAscii(sig_header[seg_start..pos]);
        if (pos < sig_header.len && sig_header[pos] == ',') {
            pos += 1;
        }
        if (seg.len == 0 || in_quote || in_bseq) {
            continue;
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
            continue;
        }

        const key = trimAscii(seg[0..eq_pos]);
        if (!isValidSfKey(key) || !eqlBytes(key, label)) {
            continue;
        }

        const rhs = trimAscii(seg[(eq_pos + 1)..]);
        if (rhs.len < 2 || rhs[0] != ':') {
            return null;
        }

        let close_colon: usize = 1;
        let found_close = false;
        while (close_colon < rhs.len) {
            const c = rhs[close_colon];
            if (c == ':') {
                found_close = true;
                break;
            }
            if (!isBase64Char(c)) {
                return null;
            }
            close_colon += 1;
        }
        if (!found_close) {
            return null;
        }

        // 闭合冒号之后只允许为空或接参数 `;...`
        const rest = trimAscii(rhs[(close_colon + 1)..]);
        if (rest.len > 0 && rest[0] != ';') {
            return null;
        }

        return rhs[1..close_colon];
    }

    return null;
}
