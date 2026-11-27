// @cap/spdx — Zero-allocation SPDX 2.3 license expression parser, validator & compatibility evaluator for CP
// Clean-room MIT implementation inspired by npm `spdx-expression-parse` (55M/wk), `spdx-satisfies` (30M/wk), and Rust `spdx` (40M).

import { strEq, startsWith, isSpace } from "cap:ascii";

const MAX_LICENSES: usize = 32;
const MAX_DEPTH: usize = 32;

pub enum LicenseCategory {
    Permissive,
    WeakCopyleft,
    StrongCopyleft,
    NetworkCopyleft,
    PublicDomain,
    Unknown,
}

pub struct LicenseInfo {
    pub id: string,
    pub category: LicenseCategory,
    pub isOsiApproved: bool,
}

pub struct SpdxResult {
    pub valid: bool,
    pub allKnown: bool,
    pub errorOffset: usize,
    pub licenses: [MAX_LICENSES]string,
    pub licenseCount: usize,

    pub fn getLicense(idx: usize) -> string? {
        if (idx >= _.licenseCount) {
            return null;
        }
        return _.licenses[idx];
    }
}

fn isIdChar(c: u8) -> bool {
    if (c >= 'a' && c <= 'z') {
        return true;
    }
    if (c >= 'A' && c <= 'Z') {
        return true;
    }
    if (c >= '0' && c <= '9') {
        return true;
    }
    return c == '-' || c == '.';
}

fn isPermissiveCategory(cat: LicenseCategory) -> bool {
    switch (cat) {
        LicenseCategory.Permissive => {
            return true;
        },
        LicenseCategory.PublicDomain => {
            return true;
        },
        LicenseCategory.WeakCopyleft => {
            return false;
        },
        LicenseCategory.StrongCopyleft => {
            return false;
        },
        LicenseCategory.NetworkCopyleft => {
            return false;
        },
        LicenseCategory.Unknown => {
            return false;
        },
    }
}

fn lookupBaseLicense(id: string) -> LicenseInfo? {
    // Permissive (OSI-approved & widely used)
    if (strEq(id, "MIT")) { return LicenseInfo{ id: "MIT", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "MIT-0")) { return LicenseInfo{ id: "MIT-0", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "Apache-2.0")) { return LicenseInfo{ id: "Apache-2.0", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "BSD-2-Clause")) { return LicenseInfo{ id: "BSD-2-Clause", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "BSD-3-Clause")) { return LicenseInfo{ id: "BSD-3-Clause", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "BSD-4-Clause")) { return LicenseInfo{ id: "BSD-4-Clause", category: LicenseCategory.Permissive, isOsiApproved: false }; }
    if (strEq(id, "ISC")) { return LicenseInfo{ id: "ISC", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "Zlib")) { return LicenseInfo{ id: "Zlib", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "BSL-1.0")) { return LicenseInfo{ id: "BSL-1.0", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "0BSD")) { return LicenseInfo{ id: "0BSD", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "BlueOak-1.0.0")) { return LicenseInfo{ id: "BlueOak-1.0.0", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "PostgreSQL")) { return LicenseInfo{ id: "PostgreSQL", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "curl")) { return LicenseInfo{ id: "curl", category: LicenseCategory.Permissive, isOsiApproved: false }; }
    if (strEq(id, "FTL")) { return LicenseInfo{ id: "FTL", category: LicenseCategory.Permissive, isOsiApproved: false }; }
    if (strEq(id, "OpenSSL")) { return LicenseInfo{ id: "OpenSSL", category: LicenseCategory.Permissive, isOsiApproved: false }; }
    if (strEq(id, "NCSA")) { return LicenseInfo{ id: "NCSA", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "Python-2.0")) { return LicenseInfo{ id: "Python-2.0", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "Artistic-2.0")) { return LicenseInfo{ id: "Artistic-2.0", category: LicenseCategory.Permissive, isOsiApproved: true }; }
    if (strEq(id, "CC-BY-4.0")) { return LicenseInfo{ id: "CC-BY-4.0", category: LicenseCategory.Permissive, isOsiApproved: false }; }

    // Public Domain
    if (strEq(id, "Unlicense")) { return LicenseInfo{ id: "Unlicense", category: LicenseCategory.PublicDomain, isOsiApproved: false }; }
    if (strEq(id, "CC0-1.0")) { return LicenseInfo{ id: "CC0-1.0", category: LicenseCategory.PublicDomain, isOsiApproved: false }; }
    if (strEq(id, "WTFPL")) { return LicenseInfo{ id: "WTFPL", category: LicenseCategory.PublicDomain, isOsiApproved: false }; }

    // Weak Copyleft
    if (strEq(id, "MPL-2.0")) { return LicenseInfo{ id: "MPL-2.0", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "LGPL-2.1-only")) { return LicenseInfo{ id: "LGPL-2.1-only", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "LGPL-2.1-or-later")) { return LicenseInfo{ id: "LGPL-2.1-or-later", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "LGPL-3.0-only")) { return LicenseInfo{ id: "LGPL-3.0-only", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "LGPL-3.0-or-later")) { return LicenseInfo{ id: "LGPL-3.0-or-later", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "LGPL-2.1")) { return LicenseInfo{ id: "LGPL-2.1", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "LGPL-3.0")) { return LicenseInfo{ id: "LGPL-3.0", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "EPL-2.0")) { return LicenseInfo{ id: "EPL-2.0", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "EPL-1.0")) { return LicenseInfo{ id: "EPL-1.0", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "CDDL-1.0")) { return LicenseInfo{ id: "CDDL-1.0", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "EUPL-1.2")) { return LicenseInfo{ id: "EUPL-1.2", category: LicenseCategory.WeakCopyleft, isOsiApproved: true }; }
    if (strEq(id, "CC-BY-SA-4.0")) { return LicenseInfo{ id: "CC-BY-SA-4.0", category: LicenseCategory.WeakCopyleft, isOsiApproved: false }; }

    // Strong Copyleft
    if (strEq(id, "GPL-2.0-only")) { return LicenseInfo{ id: "GPL-2.0-only", category: LicenseCategory.StrongCopyleft, isOsiApproved: true }; }
    if (strEq(id, "GPL-2.0-or-later")) { return LicenseInfo{ id: "GPL-2.0-or-later", category: LicenseCategory.StrongCopyleft, isOsiApproved: true }; }
    if (strEq(id, "GPL-3.0-only")) { return LicenseInfo{ id: "GPL-3.0-only", category: LicenseCategory.StrongCopyleft, isOsiApproved: true }; }
    if (strEq(id, "GPL-3.0-or-later")) { return LicenseInfo{ id: "GPL-3.0-or-later", category: LicenseCategory.StrongCopyleft, isOsiApproved: true }; }
    if (strEq(id, "GPL-2.0")) { return LicenseInfo{ id: "GPL-2.0", category: LicenseCategory.StrongCopyleft, isOsiApproved: true }; }
    if (strEq(id, "GPL-3.0")) { return LicenseInfo{ id: "GPL-3.0", category: LicenseCategory.StrongCopyleft, isOsiApproved: true }; }

    // Network Copyleft
    if (strEq(id, "AGPL-3.0-only")) { return LicenseInfo{ id: "AGPL-3.0-only", category: LicenseCategory.NetworkCopyleft, isOsiApproved: true }; }
    if (strEq(id, "AGPL-3.0-or-later")) { return LicenseInfo{ id: "AGPL-3.0-or-later", category: LicenseCategory.NetworkCopyleft, isOsiApproved: true }; }
    if (strEq(id, "AGPL-3.0")) { return LicenseInfo{ id: "AGPL-3.0", category: LicenseCategory.NetworkCopyleft, isOsiApproved: true }; }

    return null;
}

pub fn lookupLicense(id: string) -> LicenseInfo? {
    const exact = lookupBaseLicense(id);
    if (exact != null) {
        return exact;
    }
    if (id.len > 1 && id[id.len - 1] == '+') {
        const base = id[0..(id.len - 1)];
        return lookupBaseLicense(base);
    }
    return null;
}

enum EvalMode {
    ValidateOnly,
    PermissiveOnly,
    Satisfies,
}

struct Parser {
    src: string,
    pos: usize,
    depth: usize,
    failed: bool,
    errorOffset: usize,
    allKnown: bool,
    licenses: [MAX_LICENSES]string,
    licenseCount: usize,
    mode: EvalMode,
    allowed: []const string,
}

fn initParser(src: string, mode: EvalMode, allowed: []const string) -> Parser {
    let arr: [MAX_LICENSES]string = undefined;
    let i: usize = 0;
    while (i < MAX_LICENSES) {
        arr[i] = "";
        i += 1;
    }
    return Parser{
        src: src,
        pos: 0 as usize,
        depth: 0 as usize,
        failed: false,
        errorOffset: 0 as usize,
        allKnown: true,
        licenses: arr,
        licenseCount: 0 as usize,
        mode: mode,
        allowed: allowed,
    };
}

fn skipSpaces(p: *Parser) {
    while (p.pos < p.src.len && isSpace(p.src[p.pos])) {
        p.pos += 1;
    }
}

fn failAt(p: *Parser, offset: usize) -> bool {
    if (!p.failed) {
        p.failed = true;
        p.errorOffset = offset;
    }
    return false;
}

fn isKeyword(tok: string) -> bool {
    return strEq(tok, "AND") || strEq(tok, "OR") || strEq(tok, "WITH");
}

fn peekKeyword(p: *Parser, kw: string) -> bool {
    const saved = p.pos;
    skipSpaces(p);
    const start = p.pos;
    if (start + kw.len > p.src.len) {
        p.pos = saved;
        return false;
    }
    const slice = p.src[start..(start + kw.len)];
    if (!strEq(slice, kw)) {
        p.pos = saved;
        return false;
    }
    const after = start + kw.len;
    if (after < p.src.len) {
        const next = p.src[after];
        if (isIdChar(next) || next == '+' || next == ':') {
            p.pos = saved;
            return false;
        }
    }
    p.pos = saved;
    return true;
}

fn consumeKeyword(p: *Parser, kw: string) -> bool {
    if (!peekKeyword(p, kw)) {
        return false;
    }
    skipSpaces(p);
    p.pos += kw.len;
    return true;
}

fn recordLicense(p: *Parser, lic: string) {
    let i: usize = 0;
    while (i < p.licenseCount) {
        if (strEq(p.licenses[i], lic)) {
            return;
        }
        i += 1;
    }
    if (p.licenseCount < MAX_LICENSES) {
        p.licenses[p.licenseCount] = lic;
        p.licenseCount += 1;
    }
}

fn isValidRefId(s: string) -> bool {
    if (startsWith(s, "LicenseRef-")) {
        return s.len > 11;
    }
    if (startsWith(s, "DocumentRef-")) {
        // Must contain ":LicenseRef-" with non-empty doc and license IDs
        let colon: usize = 0;
        let found = false;
        let i: usize = 12;
        while (i < s.len) {
            if (s[i] == ':') {
                colon = i;
                found = true;
                break;
            }
            if (!isIdChar(s[i])) {
                return false;
            }
            i += 1;
        }
        if (!found || colon == 12) {
            return false;
        }
        const rest = s[(colon + 1)..s.len];
        if (!startsWith(rest, "LicenseRef-") || rest.len <= 11) {
            return false;
        }
        let j: usize = 11;
        while (j < rest.len) {
            if (!isIdChar(rest[j])) {
                return false;
            }
            j += 1;
        }
        return true;
    }
    return false;
}

fn licenseTokenMatches(licBase: string, hasPlus: bool, allowedLic: string) -> bool {
    if (hasPlus) {
        // Exact match with "+" suffix (e.g. "GPL-2.0+")
        if (allowedLic.len == licBase.len + 1 && allowedLic[allowedLic.len - 1] == '+') {
            if (strEq(allowedLic[0..licBase.len], licBase)) {
                return true;
            }
        }
        // Equivalence with "-or-later" (e.g. "GPL-2.0+" <-> "GPL-2.0-or-later")
        if (allowedLic.len == licBase.len + 9) {
            if (strEq(allowedLic[0..licBase.len], licBase) && strEq(allowedLic[licBase.len..allowedLic.len], "-or-later")) {
                return true;
            }
        }
        return false;
    }

    if (strEq(licBase, allowedLic)) {
        return true;
    }
    // Equivalence: "GPL-2.0-or-later" matches "GPL-2.0+"
    if (licBase.len > 9 && strEq(licBase[(licBase.len - 9)..licBase.len], "-or-later")) {
        const stem = licBase[0..(licBase.len - 9)];
        if (allowedLic.len == stem.len + 1 && allowedLic[stem.len] == '+' && strEq(allowedLic[0..stem.len], stem)) {
            return true;
        }
    }
    return false;
}

fn matchAllowedEntry(licBase: string, hasPlus: bool, hasExc: bool, exc: string, entry: string) -> bool {
    // Trim leading/trailing spaces on entry
    let start: usize = 0;
    while (start < entry.len && isSpace(entry[start])) {
        start += 1;
    }
    let end: usize = entry.len;
    while (end > start && isSpace(entry[end - 1])) {
        end -= 1;
    }
    const trimmed = entry[start..end];
    if (trimmed.len == 0) {
        return false;
    }

    if (!hasExc) {
        return licenseTokenMatches(licBase, hasPlus, trimmed);
    }

    // Parse "<allowed_lic> WITH <allowed_exc>" from trimmed
    let i: usize = 0;
    while (i < trimmed.len && !isSpace(trimmed[i])) {
        i += 1;
    }
    const left = trimmed[0..i];
    if (!licenseTokenMatches(licBase, hasPlus, left)) {
        return false;
    }
    while (i < trimmed.len && isSpace(trimmed[i])) {
        i += 1;
    }
    if (i + 4 > trimmed.len || !strEq(trimmed[i..(i + 4)], "WITH")) {
        return false;
    }
    i += 4;
    if (i >= trimmed.len || !isSpace(trimmed[i])) {
        return false;
    }
    while (i < trimmed.len && isSpace(trimmed[i])) {
        i += 1;
    }
    const right = trimmed[i..trimmed.len];
    return strEq(right, exc);
}

fn evalSimple(p: *Parser, licBase: string, hasPlus: bool, hasExc: bool, exc: string, fullLic: string) -> bool {
    const info = lookupLicense(fullLic);
    if (info == null) {
        p.allKnown = false;
    }
    recordLicense(p, fullLic);

    switch (p.mode) {
        EvalMode.ValidateOnly => {
            return true;
        },
        EvalMode.PermissiveOnly => {
            if (info == null) {
                return false;
            }
            const unwrapped = info ?? LicenseInfo{
                id: "",
                category: LicenseCategory.Unknown,
                isOsiApproved: false,
            };
            return isPermissiveCategory(unwrapped.category);
        },
        EvalMode.Satisfies => {
            let i: usize = 0;
            while (i < p.allowed.len) {
                if (matchAllowedEntry(licBase, hasPlus, hasExc, exc, p.allowed[i])) {
                    return true;
                }
                i += 1;
            }
            return false;
        },
    }
}

fn parseSimple(p: *Parser) -> bool {
    skipSpaces(p);
    const start = p.pos;
    if (start >= p.src.len) {
        return failAt(p, start);
    }

    // Read license-id or DocumentRef-...:LicenseRef-...
    while (p.pos < p.src.len && (isIdChar(p.src[p.pos]) || p.src[p.pos] == ':')) {
        p.pos += 1;
    }
    if (p.pos == start) {
        return failAt(p, start);
    }

    const base = p.src[start..p.pos];
    if (isKeyword(base)) {
        return failAt(p, start);
    }

    // Validate ':' usage (only valid inside DocumentRef-...:LicenseRef-...)
    let hasColon = false;
    let ci: usize = 0;
    while (ci < base.len) {
        if (base[ci] == ':') {
            hasColon = true;
            break;
        }
        ci += 1;
    }
    const isRef = startsWith(base, "LicenseRef-") || startsWith(base, "DocumentRef-");
    if (hasColon && !isValidRefId(base)) {
        return failAt(p, start);
    }
    if (isRef && !isValidRefId(base)) {
        return failAt(p, start);
    }

    // Optional '+' suffix immediately attached to license-id (forbidden on LicenseRef/DocumentRef)
    let hasPlus = false;
    if (p.pos < p.src.len && p.src[p.pos] == '+') {
        if (isRef) {
            return failAt(p, p.pos);
        }
        hasPlus = true;
        p.pos += 1;
        // Reject chained '++' or trailing id chars directly after '+'
        if (p.pos < p.src.len && (p.src[p.pos] == '+' || isIdChar(p.src[p.pos]))) {
            return failAt(p, p.pos);
        }
    }

    const fullLic = p.src[start..p.pos];

    // Optional WITH <exception-id>
    let hasExc = false;
    let exc: string = "";
    if (consumeKeyword(p, "WITH")) {
        skipSpaces(p);
        const excStart = p.pos;
        while (p.pos < p.src.len && isIdChar(p.src[p.pos])) {
            p.pos += 1;
        }
        if (p.pos == excStart) {
            return failAt(p, excStart);
        }
        exc = p.src[excStart..p.pos];
        if (isKeyword(exc)) {
            return failAt(p, excStart);
        }
        // Exception cannot have '+' suffix
        if (p.pos < p.src.len && p.src[p.pos] == '+') {
            return failAt(p, p.pos);
        }
        hasExc = true;
    }

    return evalSimple(p, base, hasPlus, hasExc, exc, fullLic);
}

fn parsePrimary(p: *Parser) -> bool {
    skipSpaces(p);
    if (p.pos >= p.src.len) {
        return failAt(p, p.pos);
    }
    if (p.src[p.pos] == '(') {
        if (p.depth >= MAX_DEPTH) {
            return failAt(p, p.pos);
        }
        p.pos += 1;
        p.depth += 1;
        const inner = parseOr(p);
        p.depth -= 1;
        if (p.failed) {
            return false;
        }
        skipSpaces(p);
        if (p.pos >= p.src.len || p.src[p.pos] != ')') {
            return failAt(p, p.pos);
        }
        p.pos += 1;
        return inner;
    }
    return parseSimple(p);
}

fn parseAnd(p: *Parser) -> bool {
    let acc = parsePrimary(p);
    if (p.failed) {
        return false;
    }
    while (consumeKeyword(p, "AND")) {
        const rhs = parsePrimary(p);
        if (p.failed) {
            return false;
        }
        acc = acc && rhs;
    }
    return acc;
}

fn parseOr(p: *Parser) -> bool {
    let acc = parseAnd(p);
    if (p.failed) {
        return false;
    }
    while (consumeKeyword(p, "OR")) {
        const rhs = parseAnd(p);
        if (p.failed) {
            return false;
        }
        acc = acc || rhs;
    }
    return acc;
}

fn runParse(p: *Parser) -> bool {
    skipSpaces(p);
    if (p.pos >= p.src.len) {
        return failAt(p, 0 as usize);
    }
    const val = parseOr(p);
    if (p.failed) {
        return false;
    }
    skipSpaces(p);
    if (p.pos != p.src.len) {
        return failAt(p, p.pos);
    }
    return val;
}

pub fn validateExpression(expr: string) -> SpdxResult {
    let empty: [0]string = undefined;
    let p = initParser(expr, EvalMode.ValidateOnly, empty[0..0]);
    * = runParse(&p);
    return SpdxResult{
        valid: !p.failed,
        allKnown: !p.failed && p.allKnown,
        errorOffset: p.errorOffset,
        licenses: p.licenses,
        licenseCount: p.licenseCount,
    };
}

pub fn isPermissiveOnly(expr: string) -> bool {
    let empty: [0]string = undefined;
    let p = initParser(expr, EvalMode.PermissiveOnly, empty[0..0]);
    const val = runParse(&p);
    return !p.failed && val;
}

pub fn satisfies(expr: string, allowed: []const string) -> bool {
    let p = initParser(expr, EvalMode.Satisfies, allowed);
    const val = runParse(&p);
    return !p.failed && val;
}

export {
    LicenseCategory,
    LicenseInfo,
    SpdxResult,
    lookupLicense,
    validateExpression,
    isPermissiveOnly,
    satisfies,
};
