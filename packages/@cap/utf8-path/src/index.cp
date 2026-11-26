// @cap/utf8-path — Zero-allocation UTF-8 path manipulation, normalization & safeJoin sandboxing for CP
// Clean-room MIT implementation inspired by Rust `camino` (341M downloads).

const MAX_SEGS: usize = 64;
const MAX_TEMP_PATH: usize = 512;

fn isSep(c: u8) -> bool {
    return c == '/' || c == '\\';
}

fn isAsciiAlpha(c: u8) -> bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn strEq(a: string, b: string) -> bool {
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

fn hasDrivePrefix(p: string) -> bool {
    return p.len >= 2 && isAsciiAlpha(p[0]) && p[1] == ':';
}

pub fn isAbsolute(p: string) -> bool {
    if (p.len > 0 && isSep(p[0])) {
        return true;
    }
    if (hasDrivePrefix(p) && p.len >= 3 && isSep(p[2])) {
        return true;
    }
    return false;
}

fn trimTrailingSeps(p: string) -> string {
    if (p.len == 0) {
        return p;
    }
    let minLen: usize = 1;
    if (hasDrivePrefix(p) && p.len >= 3 && isSep(p[2])) {
        minLen = 3 as usize;
    }
    let end = p.len;
    while (end > minLen && isSep(p[end - 1])) {
        end -= 1;
    }
    return p[0..end];
}

pub fn fileName(p: string) -> string? {
    const trimmed = trimTrailingSeps(p);
    if (trimmed.len == 0) {
        return null;
    }
    if (trimmed.len == 1 && isSep(trimmed[0])) {
        return null;
    }
    if (hasDrivePrefix(trimmed) && trimmed.len <= 3) {
        return null;
    }

    let i = trimmed.len;
    let sepIdx: usize = 0;
    let hasSep = false;
    while (i > 0) {
        i -= 1;
        if (isSep(trimmed[i])) {
            sepIdx = i;
            hasSep = true;
            break;
        }
    }
    const name = if (hasSep) { trimmed[(sepIdx + 1)..trimmed.len] } else { trimmed };
    if (name.len == 0 || strEq(name, ".") || strEq(name, "..")) {
        return null;
    }
    return name;
}

pub fn fileStem(p: string) -> string? {
    const nameOpt = fileName(p);
    if (nameOpt == null) {
        return null;
    }
    const name = nameOpt ?? "";
    let i = name.len;
    while (i > 0) {
        i -= 1;
        if (name[i] == '.') {
            if (i == 0) {
                return name;
            }
            return name[0..i];
        }
    }
    return name;
}

pub fn extension(p: string) -> string? {
    const nameOpt = fileName(p);
    if (nameOpt == null) {
        return null;
    }
    const name = nameOpt ?? "";
    let i = name.len;
    while (i > 0) {
        i -= 1;
        if (name[i] == '.') {
            if (i == 0 || i + 1 >= name.len) {
                return null;
            }
            return name[(i + 1)..name.len];
        }
    }
    return null;
}

pub fn parentDir(p: string) -> string? {
    const trimmed = trimTrailingSeps(p);
    if (trimmed.len == 0) {
        return null;
    }
    if (trimmed.len == 1 && isSep(trimmed[0])) {
        return null;
    }
    if (hasDrivePrefix(trimmed) && trimmed.len == 3 && isSep(trimmed[2])) {
        return null;
    }

    let i = trimmed.len;
    while (i > 0) {
        i -= 1;
        if (isSep(trimmed[i])) {
            if (i == 0) {
                return "/";
            }
            if (hasDrivePrefix(trimmed) && i == 2) {
                return trimmed[0..3];
            }
            return trimTrailingSeps(trimmed[0..i]);
        }
    }
    return "";
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

struct SplitResult {
    isAbs: bool,
    hasDrive: bool,
    driveLetter: u8,
    segs: [MAX_SEGS]string,
    count: usize,
}

fn splitAndClean(p: string) -> SplitResult {
    let segs: [MAX_SEGS]string = undefined;
    let k: usize = 0;
    while (k < MAX_SEGS) {
        segs[k] = "";
        k += 1;
    }

    let isAbs = false;
    let hasDrive = false;
    let driveLetter: u8 = 0;
    let i: usize = 0;

    if (hasDrivePrefix(p)) {
        hasDrive = true;
        driveLetter = p[0];
        i = 2 as usize;
        if (i < p.len && isSep(p[i])) {
            isAbs = true;
            i += 1;
        }
    } else if (p.len > 0 && isSep(p[0])) {
        isAbs = true;
        i = 1 as usize;
    }

    let count: usize = 0;
    while (i < p.len) {
        while (i < p.len && isSep(p[i])) {
            i += 1;
        }
        if (i >= p.len) {
            break;
        }
        const start = i;
        while (i < p.len && !isSep(p[i])) {
            i += 1;
        }
        const seg = p[start..i];
        if (strEq(seg, ".")) {
            continue;
        }
        if (strEq(seg, "..")) {
            if (count > 0 && !strEq(segs[count - 1], "..")) {
                count -= 1;
            } else if (!isAbs) {
                if (count < MAX_SEGS) {
                    segs[count] = "..";
                    count += 1;
                }
            }
            continue;
        }
        if (count < MAX_SEGS) {
            segs[count] = seg;
            count += 1;
        }
    }

    return SplitResult{
        isAbs: isAbs,
        hasDrive: hasDrive,
        driveLetter: driveLetter,
        segs: segs,
        count: count,
    };
}

pub fn normalizePath(p: string, out: []u8) -> string {
    if (p.len == 0) {
        return out[0..0];
    }
    const sp = splitAndClean(p);
    let pos: usize = 0;
    if (sp.hasDrive) {
        pos = appendByte(out, pos, sp.driveLetter);
        pos = appendByte(out, pos, ':');
    }
    if (sp.isAbs) {
        pos = appendByte(out, pos, '/');
    }
    let i: usize = 0;
    while (i < sp.count) {
        if (i > 0) {
            pos = appendByte(out, pos, '/');
        }
        pos = appendStr(out, pos, sp.segs[i]);
        i += 1;
    }
    if (pos == 0) {
        pos = appendByte(out, pos, '.');
    }
    return out[0..pos];
}

pub fn joinPath(base: string, rel: string, out: []u8) -> string {
    if (rel.len == 0) {
        return normalizePath(base, out);
    }
    if (base.len == 0 || isAbsolute(rel)) {
        return normalizePath(rel, out);
    }
    let tmp: [MAX_TEMP_PATH]u8 = undefined;
    let pos: usize = 0;
    pos = appendStr(tmp[0..MAX_TEMP_PATH], pos, base);
    pos = appendByte(tmp[0..MAX_TEMP_PATH], pos, '/');
    pos = appendStr(tmp[0..MAX_TEMP_PATH], pos, rel);
    return normalizePath(tmp[0..pos], out);
}

fn relEscapesRoot(rel: string) -> bool {
    let depth: usize = 0;
    let i: usize = 0;
    while (i < rel.len) {
        if (rel[i] == (0 as u8)) {
            return true;
        }
        while (i < rel.len && isSep(rel[i])) {
            i += 1;
        }
        if (i >= rel.len) {
            break;
        }
        const start = i;
        while (i < rel.len && !isSep(rel[i])) {
            if (rel[i] == (0 as u8)) {
                return true;
            }
            i += 1;
        }
        const seg = rel[start..i];
        if (strEq(seg, ".")) {
            continue;
        }
        if (strEq(seg, "..")) {
            if (depth == 0) {
                return true;
            }
            depth -= 1;
        } else {
            depth += 1;
        }
    }
    return false;
}

pub fn safeJoin(root: string, untrusted_rel: string, out: []u8) -> string? {
    if (root.len == 0) {
        return null;
    }
    if (isAbsolute(untrusted_rel) || hasDrivePrefix(untrusted_rel)) {
        return null;
    }
    if (relEscapesRoot(untrusted_rel)) {
        return null;
    }

    let rootNormBuf: [MAX_TEMP_PATH]u8 = undefined;
    const normRoot = normalizePath(root, rootNormBuf[0..MAX_TEMP_PATH]);
    const joined = joinPath(normRoot, untrusted_rel, out);

    // Verify joined path stays strictly within normRoot boundary
    if (strEq(joined, normRoot)) {
        return joined;
    }
    if (joined.len < normRoot.len) {
        return null;
    }
    let i: usize = 0;
    while (i < normRoot.len) {
        if (joined[i] != normRoot[i]) {
            return null;
        }
        i += 1;
    }
    if (normRoot[normRoot.len - 1] == '/' || joined[normRoot.len] == '/') {
        return joined;
    }
    return null;
}

pub fn relativePath(from_dir: string, to_path: string, out: []u8) -> string {
    const fsp = splitAndClean(from_dir);
    const tsp = splitAndClean(to_path);

    let common: usize = 0;
    while (common < fsp.count && common < tsp.count && strEq(fsp.segs[common], tsp.segs[common])) {
        common += 1;
    }

    let pos: usize = 0;
    let written: usize = 0;
    let up: usize = common;
    while (up < fsp.count) {
        if (written > 0) {
            pos = appendByte(out, pos, '/');
        }
        pos = appendStr(out, pos, "..");
        written += 1;
        up += 1;
    }
    let down: usize = common;
    while (down < tsp.count) {
        if (written > 0) {
            pos = appendByte(out, pos, '/');
        }
        pos = appendStr(out, pos, tsp.segs[down]);
        written += 1;
        down += 1;
    }
    if (written == 0) {
        pos = appendByte(out, pos, '.');
    }
    return out[0..pos];
}

pub fn withExtension(p: string, new_ext: string, out: []u8) -> string {
    const stemOpt = fileStem(p);
    if (stemOpt == null) {
        const n = appendStr(out, 0 as usize, p);
        return out[0..n];
    }
    const stem = stemOpt ?? "";
    const parentOpt = parentDir(p);
    let pos: usize = 0;
    if (parentOpt != null) {
        const parent = parentOpt ?? "";
        if (parent.len > 0) {
            pos = appendStr(out, pos, parent);
            if (!isSep(parent[parent.len - 1])) {
                pos = appendByte(out, pos, '/');
            }
        }
    }
    pos = appendStr(out, pos, stem);

    let ext = new_ext;
    if (ext.len > 0 && ext[0] == '.') {
        ext = ext[1..ext.len];
    }
    if (ext.len > 0) {
        pos = appendByte(out, pos, '.');
        pos = appendStr(out, pos, ext);
    }
    return out[0..pos];
}

export {
    isAbsolute,
    fileName,
    fileStem,
    extension,
    parentDir,
    normalizePath,
    joinPath,
    safeJoin,
    relativePath,
    withExtension,
};
