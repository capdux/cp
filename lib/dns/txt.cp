// Source: Absorbed from /data/dev/bun/src/dns.zig | Synced: 2026-11-21T16:36:24Z (DNS TXT Key-Value, SPF & DMARC Record Parser)

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

fn parsePct(s: []const u8): u8? {
    if (s.len == 0 || s.len > 3) {
        return null;
    }
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + u16(c - '0');
        if (acc > 100) {
            return null;
        }
        i += 1;
    }
    return acc as u8;
}

/// DNS TXT 分号分隔键值对（如 DMARC / DKIM / BIMI）
export struct TxtTag {
    key: []const u8;
    value: []const u8;
}

/// DNS TXT 分号分隔 `k=v; k2=v2` 零分配流式迭代器
export struct TxtTagIter {
    src: []const u8;
    pos: usize;

    static fn init(record: []const u8): TxtTagIter {
        return TxtTagIter{
            src: record,
            pos: 0,
        };
    }

    fn next(*_): TxtTag? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ';') {
                _.pos += 1;
            }
            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ';') {
                _.pos += 1;
            }
            if (seg.len == 0) {
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

            const k = trimAscii(seg[0..eq_pos]);
            if (k.len == 0) {
                continue;
            }
            const v = trimAscii(seg[(eq_pos + 1)..]);
            return TxtTag{
                key: k,
                value: v,
            };
        }
        return null;
    }
}

/// 大小写不敏感查找 DNS TXT 记录中指定标签的值
export fn findTxtTag(record: []const u8, key: []const u8): ([]const u8)? {
    if (key.len == 0) {
        return null;
    }
    let it = TxtTagIter.init(record);
    while (tag := it.next()) {
        if (eqlIgnoreCase(tag.key, key)) {
            return tag.value;
        }
    }
    return null;
}

/// RFC 7489 DMARC 策略类型
export enum DmarcPolicy {
    none,
    quarantine,
    reject,

    static fn parse(raw: []const u8): DmarcPolicy? {
        const s = trimAscii(raw);
        if (eqlIgnoreCase(s, "none")) return DmarcPolicy.none;
        if (eqlIgnoreCase(s, "quarantine")) return DmarcPolicy.quarantine;
        if (eqlIgnoreCase(s, "reject")) return DmarcPolicy.reject;
        return null;
    }
}

/// RFC 7489 DMARC TXT 策略记录解析结果
export struct DmarcRecord {
    policy: DmarcPolicy;
    subdomain_policy: DmarcPolicy?;
    pct: u8;
    rua: ([]const u8)?;
    ruf: ([]const u8)?;

    /// 解析 DMARC 记录（严格校验首标签为 `v=DMARC1` 且包含必选 `p=`）
    fn parse(record: []const u8): DmarcRecord? {
        let it = TxtTagIter.init(record);
        const first = it.next() ?? return null;
        if (!eqlIgnoreCase(first.key, "v") || !eqlBytes(first.value, "DMARC1")) {
            return null;
        }

        let policy: DmarcPolicy? = null;
        let subdomain_policy: DmarcPolicy? = null;
        let pct: u8 = 100;
        let rua: ([]const u8)? = null;
        let ruf: ([]const u8)? = null;

        while (tag := it.next()) {
            if (eqlIgnoreCase(tag.key, "p")) {
                const p = DmarcPolicy.parse(tag.value) ?? return null;
                policy = p;
            } else if (eqlIgnoreCase(tag.key, "sp")) {
                const sp = DmarcPolicy.parse(tag.value) ?? return null;
                subdomain_policy = sp;
            } else if (eqlIgnoreCase(tag.key, "pct")) {
                pct = parsePct(tag.value) ?? return null;
            } else if (eqlIgnoreCase(tag.key, "rua")) {
                rua = tag.value;
            } else if (eqlIgnoreCase(tag.key, "ruf")) {
                ruf = tag.value;
            }
        }

        const final_policy = policy ?? return null;
        return DmarcRecord{
            policy: final_policy,
            subdomain_policy: subdomain_policy,
            pct: pct,
            rua: rua,
            ruf: ruf,
        };
    }
}

/// RFC 7208 SPF 机制限定符
export enum SpfQualifier {
    pass,
    fail,
    softfail,
    neutral,
}

/// RFC 7208 SPF 单个机制或修饰词项
export struct SpfTerm {
    qualifier: SpfQualifier;
    mechanism: []const u8;
    value: ([]const u8)?;
}

/// RFC 7208 SPF TXT 记录流式迭代器（校验 `v=spf1` 前缀后按空白提取各机制）
export struct SpfIter {
    src: []const u8;
    pos: usize;
    valid: bool;

    static fn init(record: []const u8): SpfIter {
        let pos: usize = 0;
        while (pos < record.len && isSpace(record[pos])) {
            pos += 1;
        }
        const start = pos;
        while (pos < record.len && !isSpace(record[pos])) {
            pos += 1;
        }
        const version_tok = record[start..pos];
        if (!eqlIgnoreCase(version_tok, "v=spf1")) {
            return SpfIter{
                src: record,
                pos: record.len,
                valid: false,
            };
        }
        return SpfIter{
            src: record,
            pos: pos,
            valid: true,
        };
    }

    fn isValid(): bool {
        return _.valid;
    }

    fn next(*_): SpfTerm? {
        if (!_.valid) {
            return null;
        }
        while (_.pos < _.src.len) {
            while (_.pos < _.src.len && isSpace(_.src[_.pos])) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len) {
                return null;
            }
            const tok_start = _.pos;
            while (_.pos < _.src.len && !isSpace(_.src[_.pos])) {
                _.pos += 1;
            }
            const tok = _.src[tok_start.._.pos];
            if (tok.len == 0) {
                continue;
            }

            let qual = SpfQualifier.pass;
            let body = tok;
            const first = tok[0];
            if (first == '+') {
                qual = SpfQualifier.pass;
                body = tok[1..];
            } else if (first == '-') {
                qual = SpfQualifier.fail;
                body = tok[1..];
            } else if (first == '~') {
                qual = SpfQualifier.softfail;
                body = tok[1..];
            } else if (first == '?') {
                qual = SpfQualifier.neutral;
                body = tok[1..];
            }

            if (body.len == 0) {
                continue;
            }

            let sep_pos: usize = 0;
            let found_sep = false;
            while (sep_pos < body.len) {
                if (body[sep_pos] == ':' || body[sep_pos] == '=') {
                    found_sep = true;
                    break;
                }
                sep_pos += 1;
            }

            if (found_sep) {
                if (sep_pos == 0) {
                    continue;
                }
                return SpfTerm{
                    qualifier: qual,
                    mechanism: body[0..sep_pos],
                    value: body[(sep_pos + 1)..],
                };
            }

            return SpfTerm{
                qualifier: qual,
                mechanism: body,
                value: null,
            };
        }
        return null;
    }
}
