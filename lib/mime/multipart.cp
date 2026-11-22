// Source: Absorbed from /data/dev/bun/src/http/HTTPRequestBody.cp | Synced: 2026-11-22T11:12:13Z (RFC 7578 multipart/form-data)

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) { start += 1; }
    while (end > start && isOws(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn isBoundaryChar(c: u8): bool {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '\'' || c == '(' || c == ')' || c == '+' || c == '_' ||
        c == ',' || c == '-' || c == '.' || c == '/' || c == ':' ||
        c == '=' || c == '?' || c == ' ';
}

/// 校验 RFC 2046 §5.1.1 multipart boundary 合法性（长度 1..70、合法字符集、尾字符非空格）
pub fn isValidBoundary(boundary: []const u8): bool {
    if (boundary.len == 0 || boundary.len > 70) {
        return false;
    }
    if (boundary[boundary.len - 1] == ' ') {
        return false;
    }
    let i: usize = 0;
    while (i < boundary.len) {
        if (!isBoundaryChar(boundary[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 单个 `multipart/form-data` 分片（零拷贝切片）
pub struct MultipartPart {
    raw_headers: []const u8 = "",
    body: []const u8 = "",

    /// 大小写无关查找分片内部 HTTP 头部（如 `Content-Disposition` / `Content-Type`）
    fn getHeader(header_name: []const u8): ([]const u8)? {
        let pos: usize = 0;
        while (pos < _.raw_headers.len) {
            const line_start = pos;
            while (pos < _.raw_headers.len && _.raw_headers[pos] != '\n') {
                pos += 1;
            }
            let line_end = pos;
            if (line_end > line_start && _.raw_headers[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (pos < _.raw_headers.len && _.raw_headers[pos] == '\n') {
                pos += 1;
            }

            const line = _.raw_headers[line_start..line_end];
            if (line.len == 0) { continue; }

            let colon_idx: usize? = null;
            let k: usize = 0;
            while (k < line.len) {
                if (line[k] == ':') {
                    colon_idx = k;
                    break;
                }
                k += 1;
            }
            if (ci := colon_idx) {
                const key = trimOws(line[0..ci]);
                if (eqlIgnoreCaseAscii(key, header_name)) {
                    return trimOws(line[(ci + 1)..line.len]);
                }
            }
        }
        return null;
    }
}

fn matchesDashBoundary(data: []const u8, idx: usize, boundary: []const u8): bool {
    const need = 2 + boundary.len;
    if (idx + need > data.len) { return false; }
    if (data[idx] != '-' || data[idx + 1] != '-') { return false; }
    return eqlBytes(data[(idx + 2)..(idx + need)], boundary);
}

struct BoundaryLineMatch {
    is_final: bool = false,
    next_pos: usize = 0,
}

fn checkBoundaryLineTail(data: []const u8, after_boundary: usize): BoundaryLineMatch? {
    let p = after_boundary;
    // 终止边界: --<boundary>--
    if (p + 2 <= data.len && data[p] == '-' && data[p + 1] == '-') {
        return BoundaryLineMatch{ is_final: true, next_pos: data.len };
    }
    // 允许边界后带可选 OWS 后跟 CRLF 或 LF
    while (p < data.len && isOws(data[p])) {
        p += 1;
    }
    if (p + 1 < data.len && data[p] == '\r' && data[p + 1] == '\n') {
        return BoundaryLineMatch{ is_final: false, next_pos: p + 2 };
    }
    if (p < data.len && data[p] == '\n') {
        return BoundaryLineMatch{ is_final: false, next_pos: p + 1 };
    }
    return null;
}

/// 零堆分配 RFC 7578 `multipart/form-data` 流式解析器
pub struct MultipartParser {
    body: []const u8,
    boundary: []const u8,
    pos: usize = 0,
    started: bool = false,
    finished: bool = false,
    valid: bool = false,

    static fn init(body: []const u8, boundary: []const u8): MultipartParser {
        return MultipartParser{
            body: body,
            boundary: boundary,
            pos: 0,
            started: false,
            finished: false,
            valid: isValidBoundary(boundary),
        };
    }

    /// 是否已遇到合法终止边界 `--<boundary>--`
    fn isFinished(): bool {
        return _.finished;
    }

    /// 解析并返回下一个 `MultipartPart`，到达终止边界或遇到截断输入时返回 `null`
    fn next(*_): MultipartPart? {
        if (!_.valid || _.finished || _.pos >= _.body.len) {
            return null;
        }

        const dash_len = 2 + _.boundary.len;

        // 1. 跳过 preamble，定位首个 --<boundary>
        if (!_.started) {
            let found = false;
            let scan: usize = 0;
            while (scan + dash_len <= _.body.len) {
                const at_line_start = (scan == 0) || (_.body[scan - 1] == '\n');
                if (at_line_start && matchesDashBoundary(_.body, scan, _.boundary)) {
                    if (tail := checkBoundaryLineTail(_.body, scan + dash_len)) {
                        _.started = true;
                        if (tail.is_final) {
                            _.finished = true;
                            _.pos = _.body.len;
                            return null;
                        }
                        _.pos = tail.next_pos;
                        found = true;
                        break;
                    }
                }
                scan += 1;
            }
            if (!found) {
                _.pos = _.body.len;
                return null;
            }
        }

        if (_.pos >= _.body.len) {
            return null;
        }

        // 2. 分离 part 头部区与 body 区（\r\n\r\n 或 \n\n，或空头部直接换行）
        const hdr_start = _.pos;
        let hdr_end = _.pos;
        let body_start = _.pos;
        let found_sep = false;

        if (_.pos + 1 < _.body.len && _.body[_.pos] == '\r' && _.body[_.pos + 1] == '\n') {
            hdr_end = _.pos;
            body_start = _.pos + 2;
            found_sep = true;
        } else if (_.body[_.pos] == '\n') {
            hdr_end = _.pos;
            body_start = _.pos + 1;
            found_sep = true;
        } else {
            let p = _.pos;
            while (p < _.body.len) {
                if (p + 3 < _.body.len &&
                    _.body[p] == '\r' && _.body[p + 1] == '\n' &&
                    _.body[p + 2] == '\r' && _.body[p + 3] == '\n')
                {
                    hdr_end = p;
                    body_start = p + 4;
                    found_sep = true;
                    break;
                }
                if (p + 1 < _.body.len && _.body[p] == '\n' && _.body[p + 1] == '\n') {
                    hdr_end = p;
                    body_start = p + 2;
                    found_sep = true;
                    break;
                }
                p += 1;
            }
        }

        if (!found_sep) {
            _.pos = _.body.len;
            return null;
        }

        const raw_headers = _.body[hdr_start..hdr_end];

        // 3. 查找紧随其后的边界（兼容空 body 紧邻 --<boundary> 与标准前置 CRLF/LF 两种情形）
        if (matchesDashBoundary(_.body, body_start, _.boundary)) {
            if (tail := checkBoundaryLineTail(_.body, body_start + dash_len)) {
                if (tail.is_final) {
                    _.finished = true;
                    _.pos = _.body.len;
                } else {
                    _.pos = tail.next_pos;
                }
                return MultipartPart{
                    raw_headers: raw_headers,
                    body: _.body[body_start..body_start],
                };
            }
        }

        let scan = body_start;
        while (scan < _.body.len) {
            if (_.body[scan] == '\n') {
                const cand = scan + 1;
                if (matchesDashBoundary(_.body, cand, _.boundary)) {
                    if (tail := checkBoundaryLineTail(_.body, cand + dash_len)) {
                        const body_end = if (scan > body_start && _.body[scan - 1] == '\r') scan - 1 else scan;
                        if (tail.is_final) {
                            _.finished = true;
                            _.pos = _.body.len;
                        } else {
                            _.pos = tail.next_pos;
                        }
                        return MultipartPart{
                            raw_headers: raw_headers,
                            body: _.body[body_start..body_end],
                        };
                    }
                }
            }
            scan += 1;
        }

        // 未找到下一个或终止边界（截断报文），不置位 finished
        _.pos = _.body.len;
        return null;
    }
}
