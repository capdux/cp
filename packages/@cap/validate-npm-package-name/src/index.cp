pub const MAX_PKG_NAME_LEN: usize = 214;

pub const ERR_NONE: u8 = 0;
pub const ERR_EMPTY: u8 = 1;
pub const ERR_LEADING_DOT: u8 = 2;
pub const ERR_LEADING_UNDERSCORE: u8 = 3;
pub const ERR_WHITESPACE: u8 = 4;
pub const ERR_INVALID_SCOPE: u8 = 5;
pub const ERR_NON_URL_SAFE: u8 = 6;
pub const ERR_BLACKLISTED: u8 = 7;

pub const WARN_NONE: u8 = 0;
pub const WARN_TOO_LONG: u8 = 1;
pub const WARN_UPPERCASE: u8 = 2;
pub const WARN_SPECIAL_CHARS: u8 = 3;
pub const WARN_CORE_MODULE: u8 = 4;

pub struct PackageNameValidation {
    pub valid_for_new: bool,
    pub valid_for_old: bool,
    pub error_code: u8,
    pub warning_code: u8,
}

fn vnpStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn vnpEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = if (a[i] >= 65 && a[i] <= 90) { a[i] + 32 } else { a[i] };
        let cb = if (b[i] >= 65 && b[i] <= 90) { b[i] + 32 } else { b[i] };
        if (ca != cb) { return false; }
        i = i + 1;
    }
    return true;
}

fn vnpIsUrlSafeChar(c: u8): bool {
    if (c >= 97 && c <= 122) { return true; } // a-z
    if (c >= 65 && c <= 90) { return true; }  // A-Z
    if (c >= 48 && c <= 57) { return true; }  // 0-9
    // '-', '_', '.', '~', '!', '\'', '(', ')', '*'
    return c == 45 || c == 95 || c == 46 || c == 126 ||
        c == 33 || c == 39 || c == 40 || c == 41 || c == 42;
}

fn vnpIsSpecialWarnChar(c: u8): bool {
    return c == 126 || c == 39 || c == 33 || c == 40 || c == 41 || c == 42;
}

pub fn isBlacklistedName(name: string): bool {
    return vnpEqIgnoreCase(name, "node_modules") || vnpEqIgnoreCase(name, "favicon.ico");
}

pub fn isCoreModuleName(name: string): bool {
    return vnpStrEq(name, "fs") || vnpStrEq(name, "path") || vnpStrEq(name, "http") ||
        vnpStrEq(name, "https") || vnpStrEq(name, "net") || vnpStrEq(name, "os") ||
        vnpStrEq(name, "url") || vnpStrEq(name, "util") || vnpStrEq(name, "crypto") ||
        vnpStrEq(name, "stream") || vnpStrEq(name, "events") || vnpStrEq(name, "buffer") ||
        vnpStrEq(name, "child_process") || vnpStrEq(name, "dns") || vnpStrEq(name, "tls") ||
        vnpStrEq(name, "zlib") || vnpStrEq(name, "assert") || vnpStrEq(name, "querystring");
}

fn vnpCheckSegment(seg: string): u8 {
    if (seg.len == 0) { return ERR_INVALID_SCOPE; }
    let i: usize = 0;
    while (i < seg.len) {
        if (!vnpIsUrlSafeChar(seg[i])) {
            return ERR_NON_URL_SAFE;
        }
        i = i + 1;
    }
    return ERR_NONE;
}

pub fn validatePackageName(name: string): PackageNameValidation {
    if (name.len == 0) {
        return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: ERR_EMPTY, warning_code: WARN_NONE };
    }
    if (name[0] == 32 || name[name.len - 1] == 32) {
        return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: ERR_WHITESPACE, warning_code: WARN_NONE };
    }
    if (name[0] == 46) { // '.'
        return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: ERR_LEADING_DOT, warning_code: WARN_NONE };
    }
    if (name[0] == 95) { // '_'
        return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: ERR_LEADING_UNDERSCORE, warning_code: WARN_NONE };
    }
    if (isBlacklistedName(name)) {
        return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: ERR_BLACKLISTED, warning_code: WARN_NONE };
    }

    let check_part = name;
    if (name[0] == 64) { // '@' scoped package: @scope/pkg
        let slash: usize = 0;
        let slash_count: usize = 0;
        let idx: usize = 1;
        while (idx < name.len) {
            if (name[idx] == 47) {
                slash = idx;
                slash_count = slash_count + 1;
            }
            idx = idx + 1;
        }
        if (slash_count != 1 || slash <= 1 || slash + 1 >= name.len) {
            return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: ERR_INVALID_SCOPE, warning_code: WARN_NONE };
        }
        let scope = name[1..slash];
        let pkg = name[(slash + 1)..name.len];
        let se = vnpCheckSegment(scope);
        if (se != ERR_NONE) {
            return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: se, warning_code: WARN_NONE };
        }
        if (pkg[0] == 46 || pkg[0] == 95) {
            let ec: u8 = if (pkg[0] == 46) { ERR_LEADING_DOT } else { ERR_LEADING_UNDERSCORE };
            return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: ec, warning_code: WARN_NONE };
        }
        check_part = pkg;
    }

    let pe = vnpCheckSegment(check_part);
    if (pe != ERR_NONE) {
        return PackageNameValidation { valid_for_new: false, valid_for_old: false, error_code: pe, warning_code: WARN_NONE };
    }

    // Warnings (valid_for_old = true, valid_for_new = false)
    let warn: u8 = WARN_NONE;
    if (name.len > MAX_PKG_NAME_LEN) {
        warn = WARN_TOO_LONG;
    } else if (isCoreModuleName(name)) {
        warn = WARN_CORE_MODULE;
    } else {
        let i: usize = 0;
        while (i < name.len) {
            let c = name[i];
            if (c >= 65 && c <= 90) {
                warn = WARN_UPPERCASE;
                break;
            }
            if (vnpIsSpecialWarnChar(c)) {
                warn = WARN_SPECIAL_CHARS;
                break;
            }
            i = i + 1;
        }
    }

    return PackageNameValidation {
        valid_for_new: warn == WARN_NONE,
        valid_for_old: true,
        error_code: ERR_NONE,
        warning_code: warn,
    };
}

pub fn isValidNewPackageName(name: string): bool {
    return validatePackageName(name).valid_for_new;
}
