// Source: Absorbed from /data/dev/bun/src/string/immutable.zig | Synced: 2026-11-24T04:42:24Z (RFC 4180 CSV / TSV Escaper & Row Builder)

type CsvEscVec16u8 = @Vector(16, u8);

fn simdIndexOfCsvDelim(field: []const u8, delim: u8): usize? {
    let i: usize = 0;
    const n = field.len;
    const v_delim: CsvEscVec16u8 = @splat(delim);
    const v_quote: CsvEscVec16u8 = @splat('"');
    const v_cr: CsvEscVec16u8 = @splat('\r');
    const v_lf: CsvEscVec16u8 = @splat('\n');

    while (i + 16 <= n) {
        const chunk: CsvEscVec16u8 = field[i..][0..16].*;
        const m_d: u16 = @bitCast(chunk == v_delim);
        const m_q: u16 = @bitCast(chunk == v_quote);
        const m_r: u16 = @bitCast(chunk == v_cr);
        const m_n: u16 = @bitCast(chunk == v_lf);
        const mask: u16 = m_d | m_q | m_r | m_n;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }

    while (i < n) : (i += 1) {
        const c = field[i];
        if (c == delim || c == '"' || c == '\r' || c == '\n') {
            return i;
        }
    }
    return null;
}

fn simdIndexOfQuote(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_quote: CsvEscVec16u8 = @splat('"');
    while (i + 16 <= n) {
        const chunk: CsvEscVec16u8 = s[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_quote);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (s[i] == '"') return i;
    }
    return n;
}

/// 判断字段值在 RFC 4180 CSV/TSV 编码中是否需要双引号包裹
pub fn needsCsvQuotes(field: []const u8, delim: u8): bool {
    return simdIndexOfCsvDelim(field, delim) != null;
}

/// 按 RFC 4180 规则转义单个 CSV/TSV 字段（零堆分配）
pub fn escapeCsvField(field: []const u8, delim: u8, out: []u8): ([]const u8)? {
    if (!needsCsvQuotes(field, delim)) {
        if (out.len < field.len) {
            return null;
        }
        if (field.len > 0) {
            @memcpy(out[0..field.len], field);
        }
        return out[0..field.len];
    }

    let pos: usize = 0;
    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;

    let i: usize = 0;
    while (i < field.len) {
        const hit = simdIndexOfQuote(field, i);
        const span = hit - i;
        if (span > 0) {
            if (pos + span > out.len) return null;
            @memcpy(out[pos .. pos + span], field[i..hit]);
            pos += span;
            i = hit;
        }
        if (i < field.len) {
            if (pos + 2 > out.len) return null;
            out[pos] = '"';
            out[pos + 1] = '"';
            pos += 2;
            i += 1;
        }
    }

    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;
    return out[0..pos];
}

/// 严格反转义单个 CSV/TSV 字段：非引号原样拷贝；引号包裹则还原 `""` -> `"`，未闭合引号或内部孤立 `"` 返回 null
pub fn unescapeCsvField(raw: []const u8, out: []u8): ([]const u8)? {
    if (raw.len == 0) {
        return out[0..0];
    }

    if (raw[0] != '"') {
        if (out.len < raw.len) {
            return null;
        }
        @memcpy(out[0..raw.len], raw);
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
        const hit = simdIndexOfQuote(inner, i);
        const span = hit - i;
        if (span > 0) {
            if (pos + span > out.len) return null;
            @memcpy(out[pos .. pos + span], inner[i..hit]);
            pos += span;
            i = hit;
        }
        if (i < inner.len) {
            // 内部双引号必须成对出现 ""
            if (i + 1 >= inner.len || inner[i + 1] != '"') {
                return null;
            }
            if (pos >= out.len) return null;
            out[pos] = '"';
            pos += 1;
            i += 2;
        }
    }

    return out[0..pos];
}

/// 将一行字段按 RFC 4180 编码拼接并附加行尾换行符（`crlf=true` 附加 `\r\n`，否则 `\n`）
pub fn formatCsvRow(fields: []const []const u8, delim: u8, crlf: bool, out: []u8): ([]const u8)? {
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
pub fn isFormulaInjectionRisk(field: []const u8): bool {
    if (field.len == 0) {
        return false;
    }
    const c = field[0];
    return c == '=' || c == '+' || c == '-' || c == '@' || c == '\t' || c == '\r';
}
