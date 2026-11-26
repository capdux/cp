// @cap/cargo-platform — Zero-allocation target triple parser & Cargo cfg(...) evaluator
// Inspired by Rust `cargo-platform`
// MIT License

pub struct TargetTriple {
    pub arch_start: usize,
    pub arch_len: usize,
    pub vendor_start: usize,
    pub vendor_len: usize,
    pub os_start: usize,
    pub os_len: usize,
    pub env_start: usize,
    pub env_len: usize,
    pub has_env: bool,
    pub valid: bool,
}

pub struct TargetEnv {
    pub arch: string,
    pub os: string,
    pub family: string,
    pub env: string,
    pub pointer_width: u32,
    pub is_big_endian: bool,
}

pub struct CfgEvalResult {
    pub matched: bool,
    pub ok: bool,
}

fn invalidTriple(): TargetTriple {
    return TargetTriple {
        arch_start: 0,
        arch_len: 0,
        vendor_start: 0,
        vendor_len: 0,
        os_start: 0,
        os_len: 0,
        env_start: 0,
        env_len: 0,
        has_env: false,
        valid: false,
    };
}

pub fn parseTargetTriple(triple: string): TargetTriple {
    if (triple.len == 0) {
        return invalidTriple();
    }
    let starts: [4]usize = [0; 4];
    let lens: [4]usize = [0; 4];
    let seg_count: usize = 0;

    let pos: usize = 0;
    while (pos <= triple.len) {
        let seg_start: usize = pos;
        while (pos < triple.len && triple[pos] != 45) { // '-'
            pos = pos + 1;
        }
        let seg_len: usize = pos - seg_start;
        if (seg_len == 0 || seg_count >= 4) {
            return invalidTriple();
        }
        starts[seg_count] = seg_start;
        lens[seg_count] = seg_len;
        seg_count = seg_count + 1;

        if (pos == triple.len) {
            break;
        }
        // Advance past '-'
        pos = pos + 1;
        if (pos == triple.len) {
            // Trailing '-' -> empty last segment
            return invalidTriple();
        }
    }

    if (seg_count < 3) {
        return invalidTriple();
    }
    return TargetTriple {
        arch_start: starts[0],
        arch_len: lens[0],
        vendor_start: starts[1],
        vendor_len: lens[1],
        os_start: starts[2],
        os_len: lens[2],
        env_start: starts[3],
        env_len: lens[3],
        has_env: seg_count == 4,
        valid: true,
    };
}

struct CfgParser {
    pos: usize,
    ok: bool,
    depth: usize,
}

fn isWs(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn isIdentStart(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95;
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) || (c >= 48 && c <= 57);
}

fn skipWs(p: *CfgParser, src: string) {
    while (p.*.pos < src.len && isWs(src[p.*.pos])) {
        p.*.pos = p.*.pos + 1;
    }
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn u32MatchesDecimal(n: u32, s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        let c: u8 = s[i];
        if (c < 48 || c > 57) {
            return false;
        }
        acc = acc * 10 + ((c - 48) as u64);
        if (acc > 0xFFFFFFFF) {
            return false;
        }
        i = i + 1;
    }
    return acc == (n as u64);
}

fn evalKeyValue(key: string, val: string, target: TargetEnv): bool {
    if (strEq(key, "target_os")) {
        return strEq(val, target.os);
    }
    if (strEq(key, "target_arch")) {
        return strEq(val, target.arch);
    }
    if (strEq(key, "target_env")) {
        return strEq(val, target.env);
    }
    if (strEq(key, "target_family")) {
        return strEq(val, target.family);
    }
    if (strEq(key, "target_pointer_width")) {
        return u32MatchesDecimal(target.pointer_width, val);
    }
    if (strEq(key, "target_endian")) {
        if (target.is_big_endian) {
            return strEq(val, "big");
        }
        return strEq(val, "little");
    }
    return false;
}

fn evalFlag(flag: string, target: TargetEnv): bool {
    if (strEq(flag, "unix")) {
        return strEq(target.family, "unix");
    }
    if (strEq(flag, "windows")) {
        return strEq(target.family, "windows");
    }
    return false;
}

fn parsePredicate(p: *CfgParser, src: string, target: TargetEnv): bool {
    skipWs(p, src);
    if (!p.*.ok || p.*.pos >= src.len || p.*.depth >= 64) {
        p.*.ok = false;
        return false;
    }
    if (!isIdentStart(src[p.*.pos])) {
        p.*.ok = false;
        return false;
    }
    let id_start: usize = p.*.pos;
    while (p.*.pos < src.len && isIdentCont(src[p.*.pos])) {
        p.*.pos = p.*.pos + 1;
    }
    let ident: string = src[id_start..p.*.pos];
    skipWs(p, src);

    if (p.*.pos < src.len && src[p.*.pos] == 40) { // '('
        p.*.pos = p.*.pos + 1;
        p.*.depth = p.*.depth + 1;

        if (strEq(ident, "all") || strEq(ident, "any")) {
            let is_all: bool = strEq(ident, "all");
            let acc: bool = is_all; // all() == true, any() == false
            skipWs(p, src);
            if (p.*.pos < src.len && src[p.*.pos] == 41) { // ')'
                p.*.pos = p.*.pos + 1;
                p.*.depth = p.*.depth - 1;
                return acc;
            }
            while (p.*.ok) {
                let item: bool = parsePredicate(p, src, target);
                if (!p.*.ok) {
                    p.*.depth = p.*.depth - 1;
                    return false;
                }
                if (is_all) {
                    acc = acc && item;
                } else {
                    acc = acc || item;
                }
                skipWs(p, src);
                if (p.*.pos >= src.len) {
                    p.*.ok = false;
                    p.*.depth = p.*.depth - 1;
                    return false;
                }
                if (src[p.*.pos] == 44) { // ','
                    p.*.pos = p.*.pos + 1;
                    skipWs(p, src);
                    if (p.*.pos < src.len && src[p.*.pos] == 41) {
                        p.*.pos = p.*.pos + 1;
                        p.*.depth = p.*.depth - 1;
                        return acc;
                    }
                } else if (src[p.*.pos] == 41) { // ')'
                    p.*.pos = p.*.pos + 1;
                    p.*.depth = p.*.depth - 1;
                    return acc;
                } else {
                    p.*.ok = false;
                    p.*.depth = p.*.depth - 1;
                    return false;
                }
            }
            p.*.depth = p.*.depth - 1;
            return false;
        }

        if (strEq(ident, "not") || strEq(ident, "cfg")) {
            let is_not: bool = strEq(ident, "not");
            let inner: bool = parsePredicate(p, src, target);
            skipWs(p, src);
            if (p.*.pos < src.len && src[p.*.pos] == 44) {
                p.*.pos = p.*.pos + 1;
                skipWs(p, src);
            }
            if (!p.*.ok || p.*.pos >= src.len || src[p.*.pos] != 41) {
                p.*.ok = false;
                p.*.depth = p.*.depth - 1;
                return false;
            }
            p.*.pos = p.*.pos + 1;
            p.*.depth = p.*.depth - 1;
            if (is_not) {
                return !inner;
            }
            return inner;
        }

        p.*.ok = false;
        p.*.depth = p.*.depth - 1;
        return false;
    }

    if (p.*.pos < src.len && src[p.*.pos] == 61) { // '='
        p.*.pos = p.*.pos + 1;
        skipWs(p, src);
        if (p.*.pos >= src.len || src[p.*.pos] != 34) { // '"'
            p.*.ok = false;
            return false;
        }
        p.*.pos = p.*.pos + 1;
        let val_start: usize = p.*.pos;
        while (p.*.pos < src.len && src[p.*.pos] != 34) {
            p.*.pos = p.*.pos + 1;
        }
        if (p.*.pos >= src.len) {
            p.*.ok = false;
            return false;
        }
        let val: string = src[val_start..p.*.pos];
        p.*.pos = p.*.pos + 1; // closing '"'
        return evalKeyValue(ident, val, target);
    }

    return evalFlag(ident, target);
}

pub fn evalCfgExpr(expr: string, target: TargetEnv): CfgEvalResult {
    let p: CfgParser = CfgParser { pos: 0, ok: true, depth: 0 };
    let res: bool = parsePredicate(&p, expr, target);
    skipWs(&p, expr);
    if (!p.ok || p.pos != expr.len) {
        return CfgEvalResult { matched: false, ok: false };
    }
    return CfgEvalResult { matched: res, ok: true };
}
