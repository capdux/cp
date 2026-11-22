// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T14:26:00Z (RFC 9110 Transfer-Encoding & TE Parser)

/// HTTP `Transfer-Encoding` 传输编码枚举
pub enum TransferCoding: u8 {
    chunked = 0,
    gzip = 1,
    deflate = 2,
    compress = 3,
    br = 4,
    zstd = 5,
    unknown = 6,

    static fn parse(tok: []const u8): TransferCoding {
        if (eqlIgnoreCaseAscii(tok, "chunked")) { return .chunked; }
        if (eqlIgnoreCaseAscii(tok, "gzip") || eqlIgnoreCaseAscii(tok, "x-gzip")) { return .gzip; }
        if (eqlIgnoreCaseAscii(tok, "deflate")) { return .deflate; }
        if (eqlIgnoreCaseAscii(tok, "compress") || eqlIgnoreCaseAscii(tok, "x-compress")) { return .compress; }
        if (eqlIgnoreCaseAscii(tok, "br")) { return .br; }
        if (eqlIgnoreCaseAscii(tok, "zstd")) { return .zstd; }
        return .unknown;
    }

    fn asStr(): string {
        return switch (_) {
            .chunked => "chunked",
            .gzip => "gzip",
            .deflate => "deflate",
            .compress => "compress",
            .br => "br",
            .zstd => "zstd",
            .unknown => "unknown",
        };
    }
}

/// `Transfer-Encoding` 头部安全校验结果枚举
pub enum TransferEncodingValidation: u8 {
    valid = 0,
    empty = 1,
    duplicate_chunked = 2,
    chunked_not_final = 3,
    unknown_coding = 4,

    fn asStr(): string {
        return switch (_) {
            .valid => "valid",
            .empty => "empty",
            .duplicate_chunked => "duplicate_chunked",
            .chunked_not_final => "chunked_not_final",
            .unknown_coding => "unknown_coding",
        };
    }
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isOws(s[start])) { start += 1; }
    while (end > start && isOws(s[end - 1])) { end -= 1; }
    return s[start..end];
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

/// 零堆分配 `Transfer-Encoding` 逗号列表迭代器
pub struct TransferEncodingIter {
    header: []const u8,
    pos: usize = 0,

    static fn init(header: []const u8): TransferEncodingIter {
        return TransferEncodingIter{ header: header, pos: 0 };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.header.len) {
            const start = _.pos;
            while (_.pos < _.header.len && _.header[_.pos] != ',') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.header.len && _.header[_.pos] == ',') {
                _.pos += 1;
            }
            const item = trimOws(_.header[start..end]);
            if (item.len > 0) {
                return item;
            }
        }
        return null;
    }
}

/// 按 RFC 9112 §6.1/§6.3 校验 `Transfer-Encoding` 头部合法性
/// 拒绝空列表、重复 `chunked`、`chunked` 后继续跟随其他编码以及未知编码
pub fn validateTransferEncoding(header: []const u8, require_chunked_final: bool): TransferEncodingValidation {
    let it = TransferEncodingIter.init(header);
    let count: usize = 0;
    let seen_chunked = false;
    let last_coding: TransferCoding = .unknown;

    while (tok := it.next()) {
        const coding = TransferCoding.parse(tok);
        if (coding == .unknown) {
            return .unknown_coding;
        }
        if (coding == .chunked) {
            if (seen_chunked) {
                return .duplicate_chunked;
            }
            seen_chunked = true;
        } else if (seen_chunked) {
            return .chunked_not_final;
        }
        last_coding = coding;
        count += 1;
    }

    if (count == 0) {
        return .empty;
    }
    if (require_chunked_final && last_coding != .chunked) {
        return .chunked_not_final;
    }
    return .valid;
}

/// 判断 `Transfer-Encoding` 头部末项是否为唯一合法的 `chunked`（大小写无关）
pub fn isChunkedTransfer(header: []const u8): bool {
    let it = TransferEncodingIter.init(header);
    let chunked_count: usize = 0;
    let last_is_chunked = false;

    while (tok := it.next()) {
        const is_ch = eqlIgnoreCaseAscii(tok, "chunked");
        if (is_ch) {
            chunked_count += 1;
        }
        last_is_chunked = is_ch;
    }

    return chunked_count == 1 && last_is_chunked;
}

/// RFC 9112 §6.3 异常报文校验：同时携带 `Content-Length` 与 `Transfer-Encoding` 时返回 `true`
pub fn hasConflictingClAndTe(has_content_length: bool, transfer_encoding: ([]const u8)?): bool {
    return has_content_length && transfer_encoding != null;
}

fn isZeroQValue(val: []const u8): bool {
    const s = trimOws(val);
    if (s.len == 0) { return false; }
    if (s[0] != '0') { return false; }
    if (s.len == 1) { return true; }
    if (s[1] != '.') { return false; }
    let i: usize = 2;
    while (i < s.len) {
        if (s[i] != '0') { return false; }
        i += 1;
    }
    return true;
}

/// 解析 RFC 9110 §10.1.4 `TE` 请求头是否接受 `trailers`（支持 `;q=...` 并拒绝 `q=0`）
pub fn teAcceptsTrailers(te_header: []const u8): bool {
    let it = TransferEncodingIter.init(te_header);
    while (item := it.next()) {
        // 按 ';' 拆分编码名与参数
        let semi_idx: usize? = null;
        let i: usize = 0;
        while (i < item.len) {
            if (item[i] == ';') {
                semi_idx = i;
                break;
            }
            i += 1;
        }

        const coding = if (si := semi_idx) trimOws(item[0..si]) else item;
        if (!eqlIgnoreCaseAscii(coding, "trailers")) {
            continue;
        }

        let rejected_by_q0 = false;
        if (si := semi_idx) {
            let p = si + 1;
            while (p < item.len) {
                const seg_start = p;
                while (p < item.len && item[p] != ';') { p += 1; }
                const seg = trimOws(item[seg_start..p]);
                if (p < item.len && item[p] == ';') { p += 1; }

                let eq_idx: usize? = null;
                let k: usize = 0;
                while (k < seg.len) {
                    if (seg[k] == '=') {
                        eq_idx = k;
                        break;
                    }
                    k += 1;
                }
                if (ei := eq_idx) {
                    const pname = trimOws(seg[0..ei]);
                    const pval = trimOws(seg[(ei + 1)..seg.len]);
                    if (eqlIgnoreCaseAscii(pname, "q") && isZeroQValue(pval)) {
                        rejected_by_q0 = true;
                        break;
                    }
                }
            }
        }

        if (!rejected_by_q0) {
            return true;
        }
    }
    return false;
}
