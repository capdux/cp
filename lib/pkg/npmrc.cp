// Source: Absorbed from /data/dev/bun/src/install/npmrc.zig | Synced: 2026-11-21T16:02:57Z (.npmrc Config & Scoped Registry Resolver)

pub const DEFAULT_NPM_REGISTRY: []const u8 = "https://registry.npmjs.org/";

pub struct NpmrcEntry {
    key: []const u8;
    value: []const u8;
    line_no: usize;
}

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return asciiEqlIgnoreCase(s[0..prefix.len], prefix);
}

fn isScopedRegistryKey(key: []const u8, clean_scope: []const u8): bool {
    // 匹配 "@<clean_scope>:registry"
    const suffix = ":registry";
    if (key.len != 1 + clean_scope.len + suffix.len) {
        return false;
    }
    if (key[0] != '@') {
        return false;
    }
    if (!bytesEql(key[1..(1 + clean_scope.len)], clean_scope)) {
        return false;
    }
    return bytesEql(key[(1 + clean_scope.len)..key.len], suffix);
}

pub struct NpmrcIter {
    src: []const u8;
    pos: usize;
    line_no: usize;

    fn init(content: []const u8): NpmrcIter {
        let start: usize = 0;
        if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
            start = 3;
        }
        return NpmrcIter {
            src: content,
            pos: start,
            line_no: 1,
        };
    }

    fn next(*_): NpmrcEntry? {
        while (_.pos < _.src.len) {
            const cur_line = _.line_no;
            const line_start = _.pos;

            while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                _.pos += 1;
            }
            const line_end = _.pos;

            if (_.pos < _.src.len) {
                if (_.src[_.pos] == '\r') {
                    _.pos += 1;
                    if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                        _.pos += 1;
                    }
                } else if (_.src[_.pos] == '\n') {
                    _.pos += 1;
                }
                _.line_no += 1;
            }

            // 去除行首尾水平空白
            let s = line_start;
            let e = line_end;
            while (s < e && isHWhitespace(_.src[s])) {
                s += 1;
            }
            while (e > s && isHWhitespace(_.src[e - 1])) {
                e -= 1;
            }
            if (s >= e) {
                continue;
            }

            // 跳过 # 或 ; 注释行
            if (_.src[s] == '#' || _.src[s] == ';') {
                continue;
            }

            // 查找首个 '='
            let eq_idx = s;
            let found_eq = false;
            while (eq_idx < e) {
                if (_.src[eq_idx] == '=') {
                    found_eq = true;
                    break;
                }
                eq_idx += 1;
            }
            if (!found_eq) {
                continue;
            }

            // 键名去尾空白
            let key_end = eq_idx;
            while (key_end > s && isHWhitespace(_.src[key_end - 1])) {
                key_end -= 1;
            }
            if (key_end <= s) {
                continue;
            }

            // 值去首尾空白并剥离配对的外层单/双引号
            let val_start = eq_idx + 1;
            let val_end = e;
            while (val_start < val_end && isHWhitespace(_.src[val_start])) {
                val_start += 1;
            }
            while (val_end > val_start && isHWhitespace(_.src[val_end - 1])) {
                val_end -= 1;
            }
            if (val_end - val_start >= 2) {
                const first_q = _.src[val_start];
                const last_q = _.src[val_end - 1];
                if ((first_q == '"' && last_q == '"') || (first_q == '\'' && last_q == '\'')) {
                    val_start += 1;
                    val_end -= 1;
                }
            }

            return NpmrcEntry {
                key: _.src[s..key_end],
                value: _.src[val_start..val_end],
                line_no: cur_line,
            };
        }
        return null;
    }
}

pub fn getNpmrcValue(content: []const u8, key: []const u8): ([]const u8)? {
    if (key.len == 0) {
        return null;
    }
    let it = NpmrcIter.init(content);
    let result: ([]const u8)? = null;
    while (entry := it.next()) {
        if (bytesEql(entry.key, key)) {
            result = entry.value;
        }
    }
    return result;
}

pub fn getNpmrcBool(content: []const u8, key: []const u8): bool? {
    const val = getNpmrcValue(content, key) ?? return null;
    if (asciiEqlIgnoreCase(val, "true")) {
        return true;
    }
    if (asciiEqlIgnoreCase(val, "false")) {
        return false;
    }
    return null;
}

pub fn resolveRegistryForScope(content: []const u8, scope: ([]const u8)?): []const u8 {
    let clean_scope: []const u8 = "";
    if (raw_scope := scope) {
        if (raw_scope.len > 0 && raw_scope[0] == '@') {
            clean_scope = raw_scope[1..raw_scope.len];
        } else {
            clean_scope = raw_scope;
        }
    }

    let it = NpmrcIter.init(content);
    let scoped_reg: ([]const u8)? = null;
    let default_reg: ([]const u8)? = null;

    while (entry := it.next()) {
        if (clean_scope.len > 0 && isScopedRegistryKey(entry.key, clean_scope)) {
            if (entry.value.len > 0) {
                scoped_reg = entry.value;
            }
        } else if (bytesEql(entry.key, "registry")) {
            if (entry.value.len > 0) {
                default_reg = entry.value;
            }
        }
    }

    if (r := scoped_reg) {
        return r;
    }
    if (r := default_reg) {
        return r;
    }
    return DEFAULT_NPM_REGISTRY;
}

pub fn findRegistrySetting(content: []const u8, registry_url: []const u8, suffix: []const u8): ([]const u8)? {
    if (registry_url.len == 0 || suffix.len == 0) {
        return null;
    }

    // 剥离协议前缀 https:// 或 http:// 或 //
    let url_start: usize = 0;
    if (startsWithIgnoreCase(registry_url, "https://")) {
        url_start = 8;
    } else if (startsWithIgnoreCase(registry_url, "http://")) {
        url_start = 7;
    } else if (registry_url.len >= 2 && registry_url[0] == '/' && registry_url[1] == '/') {
        url_start = 2;
    }

    let url_end = registry_url.len;
    while (url_end > url_start && registry_url[url_end - 1] == '/') {
        url_end -= 1;
    }
    if (url_end <= url_start) {
        return null;
    }
    const target_host_path = registry_url[url_start..url_end];

    let it = NpmrcIter.init(content);
    let result: ([]const u8)? = null;

    while (entry := it.next()) {
        const k = entry.key;
        if (k.len < 4 || k[0] != '/' || k[1] != '/') {
            continue;
        }

        // 从右向左查找最后一个 ':' 分隔出配置后缀（兼容 host:port 形式）
        let colon_idx = k.len;
        let found_colon = false;
        while (colon_idx > 2) {
            colon_idx -= 1;
            if (k[colon_idx] == ':') {
                found_colon = true;
                break;
            }
        }
        if (!found_colon) {
            continue;
        }

        const key_suffix = k[(colon_idx + 1)..k.len];
        if (!bytesEql(key_suffix, suffix)) {
            continue;
        }

        let prefix_end = colon_idx;
        while (prefix_end > 2 && k[prefix_end - 1] == '/') {
            prefix_end -= 1;
        }
        if (bytesEql(k[2..prefix_end], target_host_path)) {
            result = entry.value;
        }
    }

    return result;
}
