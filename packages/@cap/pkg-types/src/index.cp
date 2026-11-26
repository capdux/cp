// @cap/pkg-types — Zero-heap package.json / cap.json manifest parser and package name validator.

pub struct PkgManifest {
    name: string,
    version: string,
    license: string,
    main: string,
    module: string,
    entry: string,
    is_private: bool,
    valid: bool,
}

pub struct PkgNameParts {
    scope: string,
    pkg_name: string,
    has_scope: bool,
}

fn pkgtIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn pkgtSkipSpace(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && pkgtIsSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn pkgtStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn pkgtScanStringEnd(s: string, open_quote: usize): usize {
    let i = open_quote + 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '\\') {
            i += 2;
            continue;
        }
        if (c == '"') {
            return i;
        }
        i += 1;
    }
    return s.len;
}

fn pkgtSkipValue(s: string, start: usize): usize {
    let i = pkgtSkipSpace(s, start);
    if (i >= s.len) return s.len;
    const c = s[i];
    if (c == '"') {
        const end_q = pkgtScanStringEnd(s, i);
        if (end_q >= s.len) return s.len;
        return end_q + 1;
    }
    if (c == '{' || c == '[') {
        let depth: i32 = 0;
        while (i < s.len) {
            const ch = s[i];
            if (ch == '"') {
                const end_q = pkgtScanStringEnd(s, i);
                if (end_q >= s.len) return s.len;
                i = end_q + 1;
                continue;
            }
            if (ch == '{' || ch == '[') {
                depth += 1;
            } else if (ch == '}' || ch == ']') {
                depth -= 1;
                if (depth == 0) {
                    return i + 1;
                }
            }
            i += 1;
        }
        return s.len;
    }
    while (i < s.len && s[i] != ',' && s[i] != '}' && !pkgtIsSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn pkgtEmptyManifest(): PkgManifest {
    return PkgManifest{
        name: "",
        version: "",
        license: "",
        main: "",
        module: "",
        entry: "",
        is_private: false,
        valid: false,
    };
}

pub fn parsePkgManifest(json_text: string): PkgManifest {
    let out = pkgtEmptyManifest();
    let i = pkgtSkipSpace(json_text, 0);
    if (i >= json_text.len || json_text[i] != '{') {
        return out;
    }
    i += 1;

    while (i < json_text.len) {
        i = pkgtSkipSpace(json_text, i);
        if (i >= json_text.len) {
            return pkgtEmptyManifest();
        }
        if (json_text[i] == '}') {
            const tail = pkgtSkipSpace(json_text, i + 1);
            if (tail == json_text.len) {
                out.valid = true;
                return out;
            }
            return pkgtEmptyManifest();
        }
        if (json_text[i] != '"') {
            return pkgtEmptyManifest();
        }

        const key_end = pkgtScanStringEnd(json_text, i);
        if (key_end >= json_text.len) {
            return pkgtEmptyManifest();
        }
        const key = json_text[i + 1..key_end];
        i = pkgtSkipSpace(json_text, key_end + 1);
        if (i >= json_text.len || json_text[i] != ':') {
            return pkgtEmptyManifest();
        }
        i = pkgtSkipSpace(json_text, i + 1);
        if (i >= json_text.len) {
            return pkgtEmptyManifest();
        }

        if (json_text[i] == '"') {
            const val_end = pkgtScanStringEnd(json_text, i);
            if (val_end >= json_text.len) {
                return pkgtEmptyManifest();
            }
            const val = json_text[i + 1..val_end];
            if (pkgtStrEq(key, "name")) {
                out.name = val;
            } else if (pkgtStrEq(key, "version")) {
                out.version = val;
            } else if (pkgtStrEq(key, "license")) {
                out.license = val;
            } else if (pkgtStrEq(key, "main")) {
                out.main = val;
            } else if (pkgtStrEq(key, "module")) {
                out.module = val;
            } else if (pkgtStrEq(key, "entry")) {
                out.entry = val;
            }
            i = val_end + 1;
        } else {
            const val_end = pkgtSkipValue(json_text, i);
            if (pkgtStrEq(key, "private")) {
                const raw = json_text[i..val_end];
                if (pkgtStrEq(raw, "true")) {
                    out.is_private = true;
                } else if (pkgtStrEq(raw, "false")) {
                    out.is_private = false;
                }
            }
            i = val_end;
        }

        i = pkgtSkipSpace(json_text, i);
        if (i >= json_text.len) {
            return pkgtEmptyManifest();
        }
        if (json_text[i] == ',') {
            i += 1;
            continue;
        }
        if (json_text[i] == '}') {
            const tail = pkgtSkipSpace(json_text, i + 1);
            if (tail == json_text.len) {
                out.valid = true;
                return out;
            }
            return pkgtEmptyManifest();
        }
        return pkgtEmptyManifest();
    }

    return pkgtEmptyManifest();
}

fn pkgtIsValidSegmentChar(c: u8): bool {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c == '-' || c == '_' || c == '.' || c == '~') return true;
    return false;
}

fn pkgtIsValidSegment(seg: string): bool {
    if (seg.len == 0) return false;
    if (seg[0] == '.' || seg[0] == '_') return false;
    let i: usize = 0;
    while (i < seg.len) {
        if (!pkgtIsValidSegmentChar(seg[i])) return false;
        i += 1;
    }
    return true;
}

pub fn isValidPkgName(name: string): bool {
    if (name.len == 0 || name.len > 214) return false;
    if (name[0] == '@') {
        let slash: usize = name.len;
        let i: usize = 1;
        while (i < name.len) {
            if (name[i] == '/') {
                if (slash != name.len) return false;
                slash = i;
            }
            i += 1;
        }
        if (slash <= 1 || slash + 1 >= name.len) return false;
        const scope_body = name[1..slash];
        const pkg_body = name[slash + 1..name.len];
        return pkgtIsValidSegment(scope_body) && pkgtIsValidSegment(pkg_body);
    }
    return pkgtIsValidSegment(name);
}

pub fn splitPkgScope(name: string): PkgNameParts {
    if (name.len > 0 && name[0] == '@') {
        let i: usize = 1;
        while (i < name.len) {
            if (name[i] == '/') {
                if (i > 1 && i + 1 < name.len) {
                    return PkgNameParts{
                        scope: name[0..i],
                        pkg_name: name[i + 1..name.len],
                        has_scope: true,
                    };
                }
                break;
            }
            i += 1;
        }
    }
    return PkgNameParts{
        scope: "",
        pkg_name: name,
        has_scope: false,
    };
}
