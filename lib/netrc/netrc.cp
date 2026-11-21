// Source: Absorbed from /data/dev/bun/src/install/npm_client.zig (.netrc Credential Parser)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn strEql(a: []const u8, b: []const u8): bool {
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

/// 单条 .netrc / _netrc 认证凭据记录（零拷贝切片）
export struct NetrcEntry {
    machine: ([]const u8)?;
    is_default: bool;
    login: ([]const u8)?;
    password: ([]const u8)?;
    account: ([]const u8)?;
}

/// .netrc / _netrc 零堆分配流式解析器
export struct NetrcIter {
    src: []const u8;
    pos: usize;

    /// 初始化 .netrc 流式解析器
    fn init(src: []const u8): NetrcIter {
        return NetrcIter{
            src: src,
            pos: 0,
        };
    }

    fn skipWsAndComments(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            break;
        }
    }

    fn nextToken(*_): ([]const u8)? {
        _.skipWsAndComments();
        if (_.pos >= _.src.len) {
            return null;
        }

        // 双引号包裹值（支持包含空格）
        if (_.src[_.pos] == '"') {
            _.pos += 1;
            const start = _.pos;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (c == '\\' && _.pos + 1 < _.src.len) {
                    _.pos += 2;
                    continue;
                }
                if (c == '"') {
                    const tok = _.src[start.._.pos];
                    _.pos += 1;
                    return tok;
                }
                _.pos += 1;
            }
            return _.src[start.._.pos];
        }

        // 普通非空白 token（保留值内部可能包含的 '#' 字符）
        const start = _.pos;
        while (_.pos < _.src.len && !isSpace(_.src[_.pos])) {
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }

    /// 跳过 `macdef <name>` 宏定义体直至遇到空行或文件结尾
    fn skipMacdef(*_) {
        // 消费宏名称（若存在）
        * = _.nextToken();
        // 跳过宏定义首行剩余内容
        while (_.pos < _.src.len && _.src[_.pos] != '\n') {
            _.pos += 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }
        // 逐行跳过直到空行
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            // 判断该行是否为空行（仅含可选 \r/空格/制表符）
            let all_ws = true;
            let k = line_start;
            while (k < line_end) {
                if (!isSpace(_.src[k])) {
                    all_ws = false;
                    break;
                }
                k += 1;
            }
            if (all_ws) {
                break;
            }
        }
    }

    /// 流式读取下一条 machine 或 default 认证条目
    fn next(*_): NetrcEntry? {
        let entry = NetrcEntry{
            machine: null,
            is_default: false,
            login: null,
            password: null,
            account: null,
        };
        let found = false;

        // 定位下一条 machine 或 default 起点
        while (true) {
            if (tok := _.nextToken()) {
                if (strEql(tok, "machine")) {
                    const host = _.nextToken() ?? return null;
                    entry.machine = host;
                    entry.is_default = false;
                    found = true;
                    break;
                }
                if (strEql(tok, "default")) {
                    entry.machine = null;
                    entry.is_default = true;
                    found = true;
                    break;
                }
                if (strEql(tok, "macdef")) {
                    _.skipMacdef();
                }
            } else {
                break;
            }
        }

        if (!found) {
            return null;
        }

        // 解析当前条目的 login / password / account / macdef 字段
        while (true) {
            const saved_pos = _.pos;
            if (tok := _.nextToken()) {
                if (strEql(tok, "machine") || strEql(tok, "default")) {
                    _.pos = saved_pos;
                    break;
                }
                if (strEql(tok, "login")) {
                    entry.login = _.nextToken();
                } else if (strEql(tok, "password")) {
                    entry.password = _.nextToken();
                } else if (strEql(tok, "account")) {
                    entry.account = _.nextToken();
                } else if (strEql(tok, "macdef")) {
                    _.skipMacdef();
                }
            } else {
                break;
            }
        }

        return entry;
    }
}

/// 按主机名大小写不敏感查找 .netrc 凭据；未精确命中时回退至 default 条目
export fn lookupNetrc(src: []const u8, host: []const u8): NetrcEntry? {
    const norm_host = trimAscii(host);
    if (norm_host.len == 0) {
        return null;
    }

    let it = NetrcIter.init(src);
    let fallback: NetrcEntry? = null;

    while (true) {
        if (entry := it.next()) {
            if (entry.is_default) {
                if (fallback == null) {
                    fallback = entry;
                }
            } else if (m := entry.machine) {
                if (asciiEqlIgnoreCase(m, norm_host)) {
                    return entry;
                }
            }
        } else {
            break;
        }
    }

    return fallback;
}
