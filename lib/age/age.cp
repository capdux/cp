// Origin: Pure-CP Stdlib (Zero-Alloc age-encryption.org/v1 File Format Parser) | Created: 2026-11-22

/// age v1 版本行（不含换行符）
pub const AGE_VERSION_LINE: []const u8 = "age-encryption.org/v1";

/// age ASCII Armor 起始标记
pub const AGE_ARMOR_BEGIN: []const u8 = "-----BEGIN AGE ENCRYPTED FILE-----";

/// age ASCII Armor 结束标记
pub const AGE_ARMOR_END: []const u8 = "-----END AGE ENCRYPTED FILE-----";

/// age STREAM 密文分块明文大小（64 KiB）
pub const AGE_STREAM_CHUNK_SIZE: usize = 65536;

/// age STREAM Poly1305 认证标签大小（16 字节）
pub const AGE_STREAM_TAG_SIZE: usize = 16;

/// age STREAM 负载前导 Nonce 大小（16 字节）
pub const AGE_NONCE_SIZE: usize = 16;

/// 单个 age Recipient Stanza（零拷贝切片视图）
pub struct AgeStanza {
    stanza_type: []const u8,
    args_slice: []const u8,
    body_b64: []const u8,

    /// 返回 `args_slice` 中的参数个数（不含 `stanza_type`）
    fn argCount(): usize {
        if (_.args_slice.len == 0) {
            return 0;
        }
        let count: usize = 1;
        let i: usize = 0;
        while (i < _.args_slice.len) {
            if (_.args_slice[i] == ' ') {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// 获取 `args_slice` 中第 `idx` 个参数的零拷贝切片，越界返回 `null`
    fn argAt(idx: usize): ([]const u8)? {
        if (_.args_slice.len == 0) {
            return null;
        }
        let cur: usize = 0;
        let start: usize = 0;
        let i: usize = 0;
        while (i <= _.args_slice.len) {
            if (i == _.args_slice.len || _.args_slice[i] == ' ') {
                if (cur == idx) {
                    return _.args_slice[start..i];
                }
                cur += 1;
                start = i + 1;
            }
            i += 1;
        }
        return null;
    }
}

/// age v1 完整文件头解析结果（零拷贝切片视图）
pub struct AgeHeader {
    header_for_mac: []const u8,
    mac_b64: []const u8,
    stanza_count: u32,
    stanzas_slice: []const u8,
    payload_offset: usize,
    payload: []const u8,

    /// 提取负载前 16 字节 STREAM nonce；若负载不足 16 字节返回 `null`
    fn payloadNonce(): ([]const u8)? {
        if (_.payload.len < AGE_NONCE_SIZE) {
            return null;
        }
        return _.payload[0..AGE_NONCE_SIZE];
    }
}

/// 零分配 age Stanza 顺序迭代器
pub struct AgeStanzaIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(stanzas_slice: []const u8): AgeStanzaIter {
        return AgeStanzaIter {
            data: stanzas_slice,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): AgeStanza? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const parsed = parseOneStanza(_.data, _.pos);
        if (parsed == null) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const res = parsed ?? return null;
        _.pos = res.next_pos;
        return res.stanza;
    }
}

struct ParsedStanza {
    stanza: AgeStanza,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// 内部字符集与行解析辅助函数
// ---------------------------------------------------------------------------

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isVChar(c: u8): bool {
    return c >= 0x21 && c <= 0x7E;
}

fn isUnpaddedB64Char(c: u8): bool {
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return c == '+' || c == '/';
}

fn isBech32Lower(c: u8): bool {
    if (c >= '0' && c <= '9') {
        return c != '1';
    }
    if (c >= 'a' && c <= 'z') {
        return c != 'b' && c != 'i' && c != 'o';
    }
    return false;
}

fn isBech32Upper(c: u8): bool {
    if (c >= '0' && c <= '9') {
        return c != '1';
    }
    if (c >= 'A' && c <= 'Z') {
        return c != 'B' && c != 'I' && c != 'O';
    }
    return false;
}

fn findLf(data: []const u8, start: usize): usize? {
    let i: usize = start;
    while (i < data.len) {
        if (data[i] == '\n') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseOneStanza(data: []const u8, start: usize): ParsedStanza? {
    // 首行必须以 "-> " 开头并以 '\n' 结尾
    if (data.len - start < 4) {
        return null;
    }
    if (data[start] != '-' || data[start + 1] != '>' || data[start + 2] != ' ') {
        return null;
    }
    const lf = findLf(data, start + 3) ?? return null;
    const args_line = data[start + 3..lf];
    if (args_line.len == 0) {
        return null;
    }

    // 校验所有参数：严格 VCHAR (0x21..0x7E)，单空格分隔，无前导/尾随/连续空格
    let i: usize = 0;
    let first_space: usize = args_line.len;
    let prev_space: bool = true;
    while (i < args_line.len) {
        const c = args_line[i];
        if (c == ' ') {
            if (prev_space) {
                return null;
            }
            if (first_space == args_line.len) {
                first_space = i;
            }
            prev_space = true;
        } else if (isVChar(c)) {
            prev_space = false;
        } else {
            return null;
        }
        i += 1;
    }
    if (prev_space) {
        // 尾随空格非法
        return null;
    }

    const stanza_type = args_line[0..first_space];
    const args_slice = if (first_space < args_line.len)
        args_line[first_space + 1..args_line.len]
    else
        args_line[args_line.len..args_line.len];

    // 扫描紧随其后的规范 64 列折行无填充 Base64 body
    const body_start: usize = lf + 1;
    let cur: usize = body_start;
    let body_end: usize = body_start;

    while (true) {
        const line_lf = findLf(data, cur) ?? return null;
        const line = data[cur..line_lf];
        if (line.len > 64) {
            return null;
        }
        if ((line.len % 4) == 1) {
            return null;
        }
        let k: usize = 0;
        while (k < line.len) {
            if (!isUnpaddedB64Char(line[k])) {
                return null;
            }
            k += 1;
        }
        body_end = line_lf;
        cur = line_lf + 1;
        if (line.len < 64) {
            break;
        }
    }

    return ParsedStanza {
        stanza: AgeStanza {
            stanza_type: stanza_type,
            args_slice: args_slice,
            body_b64: data[body_start..body_end],
        },
        next_pos: cur,
    };
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 校验数据首行是否精确为 `"age-encryption.org/v1\n"`（严格 LF，不接受 `\r\n`）
pub fn isAge(data: []const u8): bool {
    if (data.len < AGE_VERSION_LINE.len + 1) {
        return false;
    }
    if (!startsWith(data, AGE_VERSION_LINE)) {
        return false;
    }
    return data[AGE_VERSION_LINE.len] == '\n';
}

/// 跳过前导空白后校验是否以 `AGE_ARMOR_BEGIN` 开头
pub fn isAgeArmored(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len && (data[i] == ' ' || data[i] == '\t' || data[i] == '\r' || data[i] == '\n')) {
        i += 1;
    }
    return startsWith(data[i..data.len], AGE_ARMOR_BEGIN);
}

/// 零分配解析完整 age v1 文件头（版本行、全部 Recipient Stanza、43B MAC 与 Payload 边界）
pub fn parseAgeHeader(data: []const u8): AgeHeader? {
    if (!isAge(data)) {
        return null;
    }

    const stanzas_start: usize = AGE_VERSION_LINE.len + 1;
    let pos: usize = stanzas_start;
    let stanza_count: u32 = 0;

    while (pos < data.len) {
        if (data.len - pos >= 3 && data[pos] == '-' && data[pos + 1] == '-' && data[pos + 2] == '-') {
            break;
        }
        const parsed = parseOneStanza(data, pos) ?? return null;
        stanza_count += 1;
        pos = parsed.next_pos;
    }

    // age v1 规范要求至少存在 1 个 Recipient Stanza
    if (stanza_count == 0) {
        return null;
    }

    // 校验 footer MAC 行："--- <43-char-unpadded-base64-MAC>\n"
    if (data.len - pos < 4) {
        return null;
    }
    if (data[pos] != '-' || data[pos + 1] != '-' || data[pos + 2] != '-' || data[pos + 3] != ' ') {
        return null;
    }
    const mac_lf = findLf(data, pos + 4) ?? return null;
    const mac_b64 = data[pos + 4..mac_lf];
    if (mac_b64.len != 43) {
        return null;
    }
    let m: usize = 0;
    while (m < mac_b64.len) {
        if (!isUnpaddedB64Char(mac_b64[m])) {
            return null;
        }
        m += 1;
    }

    const payload_offset: usize = mac_lf + 1;
    return AgeHeader {
        header_for_mac: data[0..pos + 3],
        mac_b64: mac_b64,
        stanza_count: stanza_count,
        stanzas_slice: data[stanzas_start..pos],
        payload_offset: payload_offset,
        payload: data[payload_offset..data.len],
    };
}

/// 校验 age1 Bech32 公钥格式（`"age1"` + 58 个小写 Bech32 字符 = 62 字节）
pub fn isValidAgePublicKey(s: []const u8): bool {
    if (s.len != 62) {
        return false;
    }
    if (!startsWith(s, "age1")) {
        return false;
    }
    let i: usize = 4;
    while (i < 62) {
        if (!isBech32Lower(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 校验 age Bech32 私钥格式（`"AGE-SECRET-KEY-1"` + 58 个大写 Bech32 字符 = 74 字节）
pub fn isValidAgeSecretKey(s: []const u8): bool {
    if (s.len != 74) {
        return false;
    }
    if (!startsWith(s, "AGE-SECRET-KEY-1")) {
        return false;
    }
    let i: usize = 16;
    while (i < 74) {
        if (!isBech32Upper(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}
