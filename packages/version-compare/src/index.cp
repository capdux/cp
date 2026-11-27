// @cap/version-compare — Zero-heap flexible version string comparison
// Clean-room pure CP counterpart to crates.io:version-compare

pub enum VersionCmp {
    Lt,
    Eq,
    Gt,
    Invalid,
}

const VC_MAX_SEGS: usize = 32;
const VC_MAX_U64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

struct VcSeg {
    isNum: bool,
    numVal: u64,
    start: usize,
    len: usize,
}

struct VcParsed {
    segs: [32]VcSeg,
    count: usize,
    valid: bool,
}

fn vcIsSpace(b: u8): bool {
    return b == 0x20 || b == 0x09 || b == 0x0A || b == 0x0D;
}

fn vcIsDigit(b: u8): bool {
    return b >= 0x30 && b <= 0x39;
}

fn vcIsAlpha(b: u8): bool {
    return (b >= 0x41 && b <= 0x5A) || (b >= 0x61 && b <= 0x7A);
}

fn vcIsSep(b: u8): bool {
    return b == 0x2E || b == 0x2D || b == 0x5F;
}

fn vcToLower(b: u8): u8 {
    if (b >= 0x41 && b <= 0x5A) {
        return b + 32;
    }
    return b;
}

fn vcStrEqAsciiLower(s: string, start: usize, len: usize, lit: string): bool {
    if (len != lit.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (vcToLower(s[start + i]) != lit[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn vcTagRank(s: string, start: usize, len: usize): i32 {
    if (vcStrEqAsciiLower(s, start, len, "dev")) return 1;
    if (vcStrEqAsciiLower(s, start, len, "alpha") || vcStrEqAsciiLower(s, start, len, "a")) return 2;
    if (vcStrEqAsciiLower(s, start, len, "beta") || vcStrEqAsciiLower(s, start, len, "b")) return 3;
    if (vcStrEqAsciiLower(s, start, len, "rc") || vcStrEqAsciiLower(s, start, len, "pre") || vcStrEqAsciiLower(s, start, len, "preview")) return 4;
    if (vcStrEqAsciiLower(s, start, len, "release") || vcStrEqAsciiLower(s, start, len, "rel") || vcStrEqAsciiLower(s, start, len, "final")) return 5;
    if (vcStrEqAsciiLower(s, start, len, "post") || vcStrEqAsciiLower(s, start, len, "pl") || vcStrEqAsciiLower(s, start, len, "p")) return 6;
    return 0;
}

fn vcCmpLexLower(a: string, aStart: usize, aLen: usize, b: string, bStart: usize, bLen: usize): i32 {
    const minLen = if (aLen < bLen) aLen else bLen;
    let i: usize = 0;
    while (i < minLen) {
        const ca = vcToLower(a[aStart + i]);
        const cb = vcToLower(b[bStart + i]);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
        i += 1;
    }
    if (aLen < bLen) return -1;
    if (aLen > bLen) return 1;
    return 0;
}

fn vcParse(raw: string): VcParsed {
    let out = VcParsed{
        segs: undefined,
        count: 0,
        valid: false,
    };

    let start: usize = 0;
    let end = raw.len;
    while (start < end && vcIsSpace(raw[start])) {
        start += 1;
    }
    while (end > start && vcIsSpace(raw[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return out;
    }

    // Strip optional build metadata (+...)
    let k = start;
    while (k < end) {
        if (raw[k] == 0x2B) {
            end = k;
            break;
        }
        k += 1;
    }
    if (start >= end) {
        return out;
    }

    // Strip optional leading 'v' or 'V' when followed by a digit
    if ((raw[start] == 0x76 || raw[start] == 0x56) && start + 1 < end && vcIsDigit(raw[start + 1])) {
        start += 1;
    }

    let i = start;
    while (i < end) {
        if (vcIsSep(raw[i])) {
            i += 1;
            continue;
        }
        if (vcIsDigit(raw[i])) {
            const segStart = i;
            let val: u64 = 0;
            while (i < end && vcIsDigit(raw[i])) {
                const d = (raw[i] - 0x30) as u64;
                if (val <= (VC_MAX_U64 - d) / 10) {
                    val = val * 10 + d;
                } else {
                    val = VC_MAX_U64;
                }
                i += 1;
            }
            if (out.count < VC_MAX_SEGS) {
                out.segs[out.count] = VcSeg{
                    isNum: true,
                    numVal: val,
                    start: segStart,
                    len: i - segStart,
                };
                out.count += 1;
            }
            continue;
        }
        if (vcIsAlpha(raw[i])) {
            const segStart = i;
            while (i < end && vcIsAlpha(raw[i])) {
                i += 1;
            }
            if (out.count < VC_MAX_SEGS) {
                out.segs[out.count] = VcSeg{
                    isNum: false,
                    numVal: 0,
                    start: segStart,
                    len: i - segStart,
                };
                out.count += 1;
            }
            continue;
        }
        // Invalid character inside version string
        return out;
    }

    if (out.count == 0) {
        return out;
    }
    // Must contain at least one numeric segment to be a valid version
    let hasNum = false;
    let idx: usize = 0;
    while (idx < out.count) {
        if (out.segs[idx].isNum) {
            hasNum = true;
            break;
        }
        idx += 1;
    }
    out.valid = hasNum;
    return out;
}

/// Compare two version strings and return `VersionCmp` (`Lt`, `Eq`, `Gt`, or `Invalid`).
pub fn compareVersions(a: string, b: string): VersionCmp {
    const pa = vcParse(a);
    const pb = vcParse(b);
    if (!pa.valid || !pb.valid) {
        return VersionCmp.Invalid;
    }

    const maxCount = if (pa.count > pb.count) pa.count else pb.count;
    let i: usize = 0;
    while (i < maxCount) {
        const hasA = i < pa.count;
        const hasB = i < pb.count;

        if (hasA && hasB) {
            const sa = pa.segs[i];
            const sb = pb.segs[i];
            if (sa.isNum && sb.isNum) {
                if (sa.numVal < sb.numVal) return VersionCmp.Lt;
                if (sa.numVal > sb.numVal) return VersionCmp.Gt;
            } else if (!sa.isNum && !sb.isNum) {
                const ra = vcTagRank(a, sa.start, sa.len);
                const rb = vcTagRank(b, sb.start, sb.len);
                if (ra > 0 && rb > 0) {
                    if (ra < rb) return VersionCmp.Lt;
                    if (ra > rb) return VersionCmp.Gt;
                } else {
                    const cmp = vcCmpLexLower(a, sa.start, sa.len, b, sb.start, sb.len);
                    if (cmp < 0) return VersionCmp.Lt;
                    if (cmp > 0) return VersionCmp.Gt;
                }
            } else if (sa.isNum && !sb.isNum) {
                const rb = vcTagRank(b, sb.start, sb.len);
                if (rb == 6) {
                    return VersionCmp.Lt;
                }
                return VersionCmp.Gt;
            } else {
                const ra = vcTagRank(a, sa.start, sa.len);
                if (ra == 6) {
                    return VersionCmp.Gt;
                }
                return VersionCmp.Lt;
            }
        } else if (hasA) {
            const sa = pa.segs[i];
            if (sa.isNum) {
                if (sa.numVal > 0) return VersionCmp.Gt;
            } else {
                const ra = vcTagRank(a, sa.start, sa.len);
                if (ra == 6) return VersionCmp.Gt;
                if (ra != 5) return VersionCmp.Lt;
            }
        } else {
            const sb = pb.segs[i];
            if (sb.isNum) {
                if (sb.numVal > 0) return VersionCmp.Lt;
            } else {
                const rb = vcTagRank(b, sb.start, sb.len);
                if (rb == 6) return VersionCmp.Lt;
                if (rb != 5) return VersionCmp.Gt;
            }
        }

        i += 1;
    }

    return VersionCmp.Eq;
}

/// Returns true if version `a` is strictly less than version `b`.
pub fn isLess(a: string, b: string): bool {
    return compareVersions(a, b) == VersionCmp.Lt;
}

/// Returns true if version `a` is equivalent to version `b`.
pub fn isEqual(a: string, b: string): bool {
    return compareVersions(a, b) == VersionCmp.Eq;
}

/// Returns true if version `a` is strictly greater than version `b`.
pub fn isGreater(a: string, b: string): bool {
    return compareVersions(a, b) == VersionCmp.Gt;
}

fn vcOpEq(a: string, b: string): bool {
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

/// Evaluate `ver <op> target` where `op` is one of `"<"`, `"<="`, `"=="`, `"="`, `">="`, `">"`, `"!="`.
pub fn satisfies(ver: string, op: string, target: string): bool {
    const cmp = compareVersions(ver, target);
    if (cmp == VersionCmp.Invalid) {
        return false;
    }
    if (vcOpEq(op, "<")) {
        return cmp == VersionCmp.Lt;
    }
    if (vcOpEq(op, "<=")) {
        return cmp == VersionCmp.Lt || cmp == VersionCmp.Eq;
    }
    if (vcOpEq(op, "==") || vcOpEq(op, "=")) {
        return cmp == VersionCmp.Eq;
    }
    if (vcOpEq(op, ">=")) {
        return cmp == VersionCmp.Gt || cmp == VersionCmp.Eq;
    }
    if (vcOpEq(op, ">")) {
        return cmp == VersionCmp.Gt;
    }
    if (vcOpEq(op, "!=")) {
        return cmp != VersionCmp.Eq;
    }
    return false;
}
