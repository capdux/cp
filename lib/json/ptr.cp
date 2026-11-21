// Source: Absorbed from /data/dev/bun/src/json_parser.zig | Synced: 2026-11-21T14:31:57Z (RFC 6901 JSON Pointer)

/// 校验字符串是否为合法 RFC 6901 JSON Pointer
/// 空串合法代表文档根；非空必须以 `/` 开头；任意 `~` 后必须紧跟 `0` 或 `1`
pub fn isValidPointer(ptr: []const u8): bool {
    if (ptr.len == 0) {
        return true;
    }
    if (ptr[0] != '/') {
        return false;
    }
    let i: usize = 1;
    while (i < ptr.len) {
        if (ptr[i] == '~') {
            if (i + 1 >= ptr.len) {
                return false;
            }
            const next_c = ptr[i + 1];
            if (next_c != '0' && next_c != '1') {
                return false;
            }
            i += 2;
        } else {
            i += 1;
        }
    }
    return true;
}

/// 按 RFC 6901 §4 解析数组索引段：仅接受 `"0"` 或 `[1-9][0-9]*`，拒绝前导零、负数与溢出
pub fn parseArrayIndex(token: []const u8): usize? {
    if (token.len == 0) {
        return null;
    }
    if (token.len == 1 && token[0] == '0') {
        return 0;
    }
    if (token[0] < '1' || token[0] > '9') {
        return null;
    }

    const max_val: usize = @as(usize, 0) -% 1;
    let acc: usize = 0;
    let i: usize = 0;
    while (i < token.len) {
        const c = token[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const d: usize = @as(usize, c - '0');
        if (acc > ((max_val - d) / 10)) {
            return null;
        }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

/// 反转义 JSON Pointer 单个路径段：`~1` -> `/`，`~0` -> `~`
/// 遇到非法 `~` 序列或 `out` 缓冲区不足时返回 `null`
pub fn unescapeToken(raw: []const u8, out: []u8): usize? {
    let wpos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '~') {
            if (i + 1 >= raw.len) {
                return null;
            }
            const next_c = raw[i + 1];
            let decoded: u8 = 0;
            if (next_c == '0') {
                decoded = '~';
            } else if (next_c == '1') {
                decoded = '/';
            } else {
                return null;
            }
            if (wpos >= out.len) {
                return null;
            }
            out[wpos] = decoded;
            wpos += 1;
            i += 2;
        } else {
            if (wpos >= out.len) {
                return null;
            }
            out[wpos] = raw[i];
            wpos += 1;
            i += 1;
        }
    }
    return wpos;
}

/// 转义原始键名为 RFC 6901 路径段：`~` -> `~0`，`/` -> `~1`
/// `out` 缓冲区不足时返回 `null`
pub fn escapeToken(raw: []const u8, out: []u8): usize? {
    let wpos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c == '~' || c == '/') {
            if (wpos + 2 > out.len) {
                return null;
            }
            out[wpos] = '~';
            out[wpos + 1] = if (c == '~') '0' else '1';
            wpos += 2;
        } else {
            if (wpos >= out.len) {
                return null;
            }
            out[wpos] = c;
            wpos += 1;
        }
        i += 1;
    }
    return wpos;
}

/// 零堆分配 RFC 6901 JSON Pointer 路径段迭代器
pub struct PointerIter {
    ptr: []const u8,
    pos: usize = 0,

    /// 校验并初始化迭代器；非法指针返回 `null`
    static fn init(ptr: []const u8): PointerIter? {
        if (!isValidPointer(ptr)) {
            return null;
        }
        return PointerIter{ ptr: ptr, pos: 0 };
    }

    /// 返回下一个原始（未反转义）路径段切片
    fn nextRaw(*_): ([]const u8)? {
        if (_.pos >= _.ptr.len) {
            return null;
        }
        if (_.ptr[_.pos] != '/') {
            return null;
        }
        _.pos += 1;
        const start = _.pos;
        while (_.pos < _.ptr.len && _.ptr[_.pos] != '/') {
            _.pos += 1;
        }
        return _.ptr[start.._.pos];
    }

    /// 读取下一个路径段并反转义写入 `out`，返回反转义后的切片
    fn nextUnescaped(*_, out: []u8): ([]const u8)? {
        if (raw := _.nextRaw()) {
            if (n := unescapeToken(raw, out)) {
                return out[0..n];
            }
        }
        return null;
    }

    /// 返回尚未迭代的剩余指针切片
    fn remaining(): []const u8 {
        return _.ptr[_.pos.._.ptr.len];
    }
}

/// 在合法 `base_ptr` 后追加并自动转义一个路径段，写入 `out` 并返回总长度
pub fn appendToken(base_ptr: []const u8, token: []const u8, out: []u8): usize? {
    if (!isValidPointer(base_ptr)) {
        return null;
    }
    if (base_ptr.len + 1 > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < base_ptr.len) {
        out[i] = base_ptr[i];
        i += 1;
    }
    out[base_ptr.len] = '/';
    const prefix_len = base_ptr.len + 1;
    if (esc_len := escapeToken(token, out[prefix_len..out.len])) {
        return prefix_len + esc_len;
    }
    return null;
}

/// 返回父级 JSON Pointer（根 `""` 或非法指针返回 `null`，`"/a"` 返回 `""`，`"/a/b"` 返回 `"/a"`）
pub fn parentPointer(ptr: []const u8): ([]const u8)? {
    if (!isValidPointer(ptr) || ptr.len == 0) {
        return null;
    }
    let i = ptr.len;
    while (i > 0) {
        i -= 1;
        if (ptr[i] == '/') {
            return ptr[0..i];
        }
    }
    return null;
}
