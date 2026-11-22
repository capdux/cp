// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:49:17Z (RFC 9110 §10.2.1 HTTP Allow & Method Set Codec)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
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

/// RFC 9110 §5.6.2 `tchar` 字符判定
fn isTchar(c: u8): bool {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
        || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
        || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn isValidHttpToken(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isTchar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

/// 标准 HTTP 方法位掩码常量
export struct HttpMethodBit {
    const GET: u16 = 1 << 0;
    const HEAD: u16 = 1 << 1;
    const POST: u16 = 1 << 2;
    const PUT: u16 = 1 << 3;
    const DELETE: u16 = 1 << 4;
    const CONNECT: u16 = 1 << 5;
    const OPTIONS: u16 = 1 << 6;
    const TRACE: u16 = 1 << 7;
    const PATCH: u16 = 1 << 8;
}

fn methodToBit(method: []const u8): u16 {
    if (eqlBytes(method, "GET")) return HttpMethodBit.GET;
    if (eqlBytes(method, "HEAD")) return HttpMethodBit.HEAD;
    if (eqlBytes(method, "POST")) return HttpMethodBit.POST;
    if (eqlBytes(method, "PUT")) return HttpMethodBit.PUT;
    if (eqlBytes(method, "DELETE")) return HttpMethodBit.DELETE;
    if (eqlBytes(method, "CONNECT")) return HttpMethodBit.CONNECT;
    if (eqlBytes(method, "OPTIONS")) return HttpMethodBit.OPTIONS;
    if (eqlBytes(method, "TRACE")) return HttpMethodBit.TRACE;
    if (eqlBytes(method, "PATCH")) return HttpMethodBit.PATCH;
    return 0;
}

/// RFC 9110 `Allow` / `Access-Control-Allow-Methods` 零分配方法名迭代器
export struct AllowIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): AllowIter {
        return AllowIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (isValidHttpToken(seg)) {
                return seg;
            }
        }
        return null;
    }
}

/// HTTP 允许方法集合（标准方法位掩码 + 扩展方法标记）
export struct AllowMethodSet {
    mask: u16;
    has_custom: bool;

    /// 判断集合是否包含指定标准方法位（支持单比特或组合掩码）
    fn containsStandard(bit: u16): bool {
        return bit != 0 && (_.mask & bit) == bit;
    }

    /// 判断集合是否为空（无标准方法且无扩展方法）
    fn isEmpty(): bool {
        return _.mask == 0 && !_.has_custom;
    }

    /// 统计集合中标准 HTTP 方法数量
    fn countStandard(): u32 {
        let m: u16 = _.mask & 0x01FF;
        let count: u32 = 0;
        while (m != 0) {
            count += u32(m & 1);
            m >>= 1;
        }
        return count;
    }

    /// 按固定规范顺序格式化集合中的标准方法（如 `"GET, HEAD, POST, OPTIONS"`）
    fn formatStandard(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        const bits: [9]u16 = [
            HttpMethodBit.GET,
            HttpMethodBit.HEAD,
            HttpMethodBit.POST,
            HttpMethodBit.PUT,
            HttpMethodBit.DELETE,
            HttpMethodBit.CONNECT,
            HttpMethodBit.OPTIONS,
            HttpMethodBit.TRACE,
            HttpMethodBit.PATCH,
        ];
        const names: [9][]const u8 = [
            "GET",
            "HEAD",
            "POST",
            "PUT",
            "DELETE",
            "CONNECT",
            "OPTIONS",
            "TRACE",
            "PATCH",
        ];

        let i: usize = 0;
        while (i < 9) {
            if ((_.mask & bits[i]) != 0) {
                if (pos > 0) {
                    if (!writeSlice(out, &pos, ", ")) {
                        return null;
                    }
                }
                if (!writeSlice(out, &pos, names[i])) {
                    return null;
                }
            }
            i += 1;
        }
        return out[0..pos];
    }
}

/// 解析 `Allow` / `Access-Control-Allow-Methods` 头部为 `AllowMethodSet`（严格区分大小写）
export fn parseAllowHeader(header: []const u8): AllowMethodSet {
    let mask: u16 = 0;
    let has_custom = false;
    let it = AllowIter.init(header);
    while (method := it.next()) {
        const bit = methodToBit(method);
        if (bit != 0) {
            mask |= bit;
        } else {
            has_custom = true;
        }
    }
    return AllowMethodSet{
        mask: mask,
        has_custom: has_custom,
    };
}

/// 精确区分大小写判断指定方法（含扩展方法）是否在 `Allow` 头部中
export fn isMethodAllowed(header: []const u8, method: []const u8): bool {
    if (!isValidHttpToken(method)) {
        return false;
    }
    let it = AllowIter.init(header);
    while (item := it.next()) {
        if (eqlBytes(item, method)) {
            return true;
        }
    }
    return false;
}

/// RFC 9110 §9.2.1 安全方法判定（`GET` / `HEAD` / `OPTIONS` / `TRACE`）
export fn isSafeMethod(method: []const u8): bool {
    return eqlBytes(method, "GET")
        || eqlBytes(method, "HEAD")
        || eqlBytes(method, "OPTIONS")
        || eqlBytes(method, "TRACE");
}

/// RFC 9110 §9.2.2 幂等方法判定（`GET` / `HEAD` / `PUT` / `DELETE` / `OPTIONS` / `TRACE`）
export fn isIdempotentMethod(method: []const u8): bool {
    return isSafeMethod(method)
        || eqlBytes(method, "PUT")
        || eqlBytes(method, "DELETE");
}
