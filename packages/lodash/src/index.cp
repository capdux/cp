// @cap/dot-prop — Zero-allocation dot-path parser & deep JSON path reader for CP
// Clean-room MIT implementation inspired by npm `dot-prop` + `lodash.get` / `dlv` (~280M/wk).

import { isSpace, isDigit } from "cap:ascii";

const MAX_SEGMENTS: usize = 32;
const MAX_KEY_BUF: usize = 512;

pub struct PathSegment {
    pub is_index: bool,
    pub key: string,
    pub index: usize,
}

pub struct ParsedDotPath {
    pub segs: [MAX_SEGMENTS]PathSegment,
    pub count: usize,
    pub valid: bool,

    pub fn get(idx: usize) -> PathSegment? {
        if (idx >= _.count) {
            return null;
        }
        return _.segs[idx];
    }
}

fn emptySegment() -> PathSegment {
    return PathSegment{
        is_index: false,
        key: "",
        index: 0 as usize,
    };
}

fn invalidPath() -> ParsedDotPath {
    let segs: [MAX_SEGMENTS]PathSegment = undefined;
    let i: usize = 0;
    while (i < MAX_SEGMENTS) {
        segs[i] = emptySegment();
        i += 1;
    }
    return ParsedDotPath{
        segs: segs,
        count: 0 as usize,
        valid: false,
    };
}

fn parseUsize(s: string) -> usize? {
    if (s.len == 0) {
        return null;
    }
    let val: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        val = val * (10 as usize) + ((s[i] - '0') as usize);
        i += 1;
    }
    return val;
}

pub fn parseDotPath(path: string, key_buf: []u8) -> ParsedDotPath {
    if (path.len == 0) {
        return invalidPath();
    }

    let segs: [MAX_SEGMENTS]PathSegment = undefined;
    let initIdx: usize = 0;
    while (initIdx < MAX_SEGMENTS) {
        segs[initIdx] = emptySegment();
        initIdx += 1;
    }

    let count: usize = 0;
    let kpos: usize = 0;
    let i: usize = 0;

    while (i < path.len) {
        if (count >= MAX_SEGMENTS) {
            return invalidPath();
        }

        if (path[i] == '[') {
            i += 1;
            const numStart = i;
            while (i < path.len && path[i] != ']') {
                if (!isDigit(path[i])) {
                    return invalidPath();
                }
                i += 1;
            }
            if (i >= path.len || path[i] != ']' || i == numStart) {
                return invalidPath();
            }
            const numStr = path[numStart..i];
            const idxOpt = parseUsize(numStr);
            if (idxOpt == null) {
                return invalidPath();
            }
            segs[count] = PathSegment{
                is_index: true,
                key: numStr,
                index: idxOpt ?? (0 as usize),
            };
            count += 1;
            i += 1; // consume ']'

            if (i < path.len) {
                if (path[i] == '.') {
                    i += 1;
                    if (i >= path.len) {
                        return invalidPath();
                    }
                } else if (path[i] != '[') {
                    return invalidPath();
                }
            }
            continue;
        }

        if (path[i] == '.' || path[i] == ']') {
            return invalidPath();
        }

        // Parse property key segment until unescaped '.' or '['
        const segStart = i;
        let hasEscape = false;
        let scan = i;
        while (scan < path.len && path[scan] != '.' && path[scan] != '[') {
            if (path[scan] == '\\') {
                if (scan + 1 >= path.len) {
                    return invalidPath();
                }
                hasEscape = true;
                scan += 2;
            } else {
                scan += 1;
            }
        }
        if (scan == segStart) {
            return invalidPath();
        }

        let keySlice: string = "";
        if (!hasEscape) {
            keySlice = path[segStart..scan];
        } else {
            const kStart = kpos;
            let p = segStart;
            while (p < scan) {
                if (path[p] == '\\') {
                    p += 1;
                }
                if (kpos >= key_buf.len) {
                    return invalidPath();
                }
                key_buf[kpos] = path[p];
                kpos += 1;
                p += 1;
            }
            keySlice = key_buf[kStart..kpos];
        }

        segs[count] = PathSegment{
            is_index: false,
            key: keySlice,
            index: 0 as usize,
        };
        count += 1;
        i = scan;

        if (i < path.len && path[i] == '.') {
            i += 1;
            if (i >= path.len) {
                return invalidPath();
            }
        }
    }

    return ParsedDotPath{
        segs: segs,
        count: count,
        valid: count > 0,
    };
}

fn skipSpaces(s: string, pos: usize) -> usize {
    let i = pos;
    while (i < s.len && isSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn skipJsonString(s: string, start: usize) -> usize? {
    if (start >= s.len || s[start] != '"') {
        return null;
    }
    let i = start + 1;
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
            return i + 1;
        }
        i += 1;
    }
    return null;
}

fn skipJsonValue(s: string, start: usize) -> usize? {
    const pos = skipSpaces(s, start);
    if (pos >= s.len) {
        return null;
    }
    const first = s[pos];
    if (first == '"') {
        return skipJsonString(s, pos);
    }
    if (first == '{' || first == '[') {
        let depth: usize = 0;
        let i = pos;
        while (i < s.len) {
            const c = s[i];
            if (c == '"') {
                const nextPos = skipJsonString(s, i);
                if (nextPos == null) {
                    return null;
                }
                i = nextPos ?? s.len;
                continue;
            }
            if (c == '{' || c == '[') {
                depth += 1;
            } else if (c == '}' || c == ']') {
                if (depth == 0) {
                    return null;
                }
                depth -= 1;
                if (depth == 0) {
                    return i + 1;
                }
            }
            i += 1;
        }
        return null;
    }

    // Primitive: number, true, false, null
    let i = pos;
    while (i < s.len) {
        const c = s[i];
        if (isSpace(c) || c == ',' || c == '}' || c == ']') {
            break;
        }
        i += 1;
    }
    if (i == pos) {
        return null;
    }
    return i;
}

fn jsonStringEquals(rawQuoted: string, target: string) -> bool {
    if (rawQuoted.len < 2 || rawQuoted[0] != '"' || rawQuoted[rawQuoted.len - 1] != '"') {
        return false;
    }
    const inner = rawQuoted[1..(rawQuoted.len - 1)];
    let i: usize = 0;
    let j: usize = 0;
    while (i < inner.len && j < target.len) {
        let ch = inner[i];
        if (ch == '\\' && i + 1 < inner.len) {
            const esc = inner[i + 1];
            if (esc == '"' || esc == '\\' || esc == '/') {
                ch = esc;
            } else if (esc == 'n') {
                ch = '\n';
            } else if (esc == 'r') {
                ch = '\r';
            } else if (esc == 't') {
                ch = '\t';
            } else {
                ch = esc;
            }
            i += 2;
        } else {
            i += 1;
        }
        if (ch != target[j]) {
            return false;
        }
        j += 1;
    }
    return i == inner.len && j == target.len;
}

fn lookupObjectKey(obj: string, key: string) -> string? {
    let i = skipSpaces(obj, 0 as usize);
    if (i >= obj.len || obj[i] != '{') {
        return null;
    }
    i = skipSpaces(obj, i + 1);
    if (i < obj.len && obj[i] == '}') {
        return null;
    }

    while (i < obj.len) {
        i = skipSpaces(obj, i);
        if (i >= obj.len || obj[i] != '"') {
            return null;
        }
        const keyEndOpt = skipJsonString(obj, i);
        if (keyEndOpt == null) {
            return null;
        }
        const keyEnd = keyEndOpt ?? obj.len;
        const rawKey = obj[i..keyEnd];

        i = skipSpaces(obj, keyEnd);
        if (i >= obj.len || obj[i] != ':') {
            return null;
        }
        const valStart = skipSpaces(obj, i + 1);
        const valEndOpt = skipJsonValue(obj, valStart);
        if (valEndOpt == null) {
            return null;
        }
        const valEnd = valEndOpt ?? obj.len;

        if (jsonStringEquals(rawKey, key)) {
            return obj[valStart..valEnd];
        }

        i = skipSpaces(obj, valEnd);
        if (i < obj.len && obj[i] == ',') {
            i += 1;
            continue;
        }
        if (i < obj.len && obj[i] == '}') {
            return null;
        }
        return null;
    }
    return null;
}

fn lookupArrayIndex(arr: string, targetIdx: usize) -> string? {
    let i = skipSpaces(arr, 0 as usize);
    if (i >= arr.len || arr[i] != '[') {
        return null;
    }
    i = skipSpaces(arr, i + 1);
    if (i < arr.len && arr[i] == ']') {
        return null;
    }

    let curIdx: usize = 0;
    while (i < arr.len) {
        const valStart = skipSpaces(arr, i);
        const valEndOpt = skipJsonValue(arr, valStart);
        if (valEndOpt == null) {
            return null;
        }
        const valEnd = valEndOpt ?? arr.len;

        if (curIdx == targetIdx) {
            return arr[valStart..valEnd];
        }

        curIdx += 1;
        i = skipSpaces(arr, valEnd);
        if (i < arr.len && arr[i] == ',') {
            i += 1;
            continue;
        }
        if (i < arr.len && arr[i] == ']') {
            return null;
        }
        return null;
    }
    return null;
}

pub fn getJsonPath(json: string, path: string) -> string? {
    let keyBuf: [MAX_KEY_BUF]u8 = undefined;
    const parsed = parseDotPath(path, keyBuf[0..MAX_KEY_BUF]);
    if (!parsed.valid) {
        return null;
    }

    let curStart = skipSpaces(json, 0 as usize);
    const rootEndOpt = skipJsonValue(json, curStart);
    if (rootEndOpt == null) {
        return null;
    }
    let cur = json[curStart..(rootEndOpt ?? json.len)];

    let sIdx: usize = 0;
    while (sIdx < parsed.count) {
        const seg = parsed.segs[sIdx];
        const firstIdx = skipSpaces(cur, 0 as usize);
        if (firstIdx >= cur.len) {
            return null;
        }
        const first = cur[firstIdx];
        if (first == '{') {
            const nextVal = lookupObjectKey(cur, seg.key);
            if (nextVal == null) {
                return null;
            }
            cur = nextVal ?? "";
        } else if (first == '[') {
            let idx: usize = 0;
            if (seg.is_index) {
                idx = seg.index;
            } else {
                const parsedIdx = parseUsize(seg.key);
                if (parsedIdx == null) {
                    return null;
                }
                idx = parsedIdx ?? (0 as usize);
            }
            const nextVal = lookupArrayIndex(cur, idx);
            if (nextVal == null) {
                return null;
            }
            cur = nextVal ?? "";
        } else {
            return null;
        }
        sIdx += 1;
    }

    return cur;
}

pub fn getJsonString(json: string, path: string) -> string? {
    const rawOpt = getJsonPath(json, path);
    if (rawOpt == null) {
        return null;
    }
    const raw = rawOpt ?? "";
    if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
        return raw[1..(raw.len - 1)];
    }
    return null;
}

pub fn hasJsonPath(json: string, path: string) -> bool {
    return getJsonPath(json, path) != null;
}

pub fn escapePathSegment(seg: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < seg.len && pos < out.len) {
        const c = seg[i];
        if (c == '.' || c == '[' || c == ']' || c == '\\') {
            if (pos + 2 > out.len) {
                break;
            }
            out[pos] = '\\';
            out[pos + 1] = c;
            pos += 2;
        } else {
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return out[0..pos];
}

export {
    PathSegment,
    ParsedDotPath,
    parseDotPath,
    getJsonPath,
    getJsonString,
    hasJsonPath,
    escapePathSegment,
};
