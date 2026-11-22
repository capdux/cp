// Source: Absorbed from /data/dev/bun/src/string/immutable.zig | Synced: 2026-11-22T11:50:35Z (RFC 4180 CSV / TSV Escaper & Row Builder)

/// 判断字段值在 RFC 4180 CSV/TSV 编码中是否需要双引号包裹
export fn needsCsvQuotes(field: []const u8, delim: u8): bool {
    for (c of field) {
        if (c == delim || c == '"' || c == '\r' || c == '\n') {
            return true;
        }
    }
    return false;
}

/// 按 RFC 4180 规则转义单个 CSV/TSV 字段（零堆分配）
export fn escapeCsvField(field: []const u8, delim: u8, out: []u8): ([]const u8)? {
    if (!needsCsvQuotes(field, delim)) {
        if (out.len < field.len) {
            return null;
        }
        let i: usize = 0;
        while (i < field.len) {
            out[i] = field[i];
            i += 1;
        }
        return out[0..field.len];
    }

    let pos: usize = 0;
    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;

    for (c of field) {
        if (c == '"') {
            if (pos + 2 > out.len) return null;
            out[pos] = '"';
            out[pos + 1] = '"';
            pos += 2;
        } else {
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
        }
    }

    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;
    return out[0..pos];
}

/// 严格反转义单个 CSV/TSV 字段：非引号原样拷贝；引号包裹则还原 `""` -> `"`，未闭合引号或内部孤立 `"` 返回 null
export fn unescapeCsvField(raw: []const u8, out: []u8): ([]const u8)? {
    if (raw.len == 0) {
        return out[0..0];
    }

    if (raw[0] != '"') {
        if (out.len < raw.len) {
            return null;
        }
        let i: usize = 0;
        while (i < raw.len) {
            out[i] = raw[i];
            i += 1;
        }
        return out[0..raw.len];
    }

    // 引号包裹字段必须至少 2 字节且以 '"' 结尾
    if (raw.len < 2 || raw[raw.len - 1] != '"') {
        return null;
    }

    const inner = raw[1..raw.len - 1];
    let i: usize = 0;
    let pos: usize = 0;
    while (i < inner.len) {
        if (inner[i] == '"') {
            // 内部双引号必须成对出现 ""
            if (i + 1 >= inner.len || inner[i + 1] != '"') {
                return null;
            }
            if (pos >= out.len) return null;
            out[pos] = '"';
            pos += 1;
            i += 2;
        } else {
            if (pos >= out.len) return null;
            out[pos] = inner[i];
            pos += 1;
            i += 1;
        }
    }

    return out[0..pos];
}

/// 将一行字段按 RFC 4180 编码拼接并附加行尾换行符（`crlf=true` 附加 `\r\n`，否则 `\n`）
export fn formatCsvRow(fields: []const []const u8, delim: u8, crlf: bool, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < fields.len) {
        if (i > 0) {
            if (pos >= out.len) return null;
            out[pos] = delim;
            pos += 1;
        }
        const written = escapeCsvField(fields[i], delim, out[pos..]) ?? return null;
        pos += written.len;
        i += 1;
    }

    if (crlf) {
        if (pos + 2 > out.len) return null;
        out[pos] = '\r';
        out[pos + 1] = '\n';
        pos += 2;
    } else {
        if (pos + 1 > out.len) return null;
        out[pos] = '\n';
        pos += 1;
    }
    return out[0..pos];
}

/// 检测字段是否存在电子表格公式前缀风险（首字符为 `=`, `+`, `-`, `@`, `\t`, `\r`）
export fn isFormulaInjectionRisk(field: []const u8): bool {
    if (field.len == 0) {
        return false;
    }
    const c = field[0];
    return c == '=' || c == '+' || c == '-' || c == '@' || c == '\t' || c == '\r';
}
