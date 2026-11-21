// Source: Absorbed from /data/dev/bun/src/bun_core/fmt.cp | Synced: 2026-11-21T14:24:41Z (RFC 4180 CSV/TSV streaming)

/// 单个 CSV/TSV 字段切片描述（零堆分配）
export struct CsvField {
    raw: []const u8;
    was_quoted: bool;
    has_escaped_quotes: bool;
    is_row_end: bool;
}

const EMPTY_FIELD: CsvField = CsvField{
    raw: "",
    was_quoted: false,
    has_escaped_quotes: false,
    is_row_end: false,
};

/// 固定容量 32 列的单行 CSV 记录（零堆分配）
export struct CsvRow {
    const MAX_COLS: usize = 32;

    fields: [32]CsvField;
    len: usize;

    /// 按列下标安全读取字段
    fn get(index: usize): CsvField? {
        if (index >= _.len) {
            return null;
        }
        return _.fields[index];
    }
}

fn unescapeFieldImpl(field: CsvField, out: []u8): ([]const u8)? {
    if (!field.has_escaped_quotes) {
        return field.raw;
    }
    let i: usize = 0;
    let pos: usize = 0;
    while (i < field.raw.len) {
        if (pos >= out.len) {
            return null;
        }
        if (field.raw[i] == '"' && i + 1 < field.raw.len && field.raw[i + 1] == '"') {
            out[pos] = '"';
            pos += 1;
            i += 2;
        } else {
            out[pos] = field.raw[i];
            pos += 1;
            i += 1;
        }
    }
    return out[0..pos];
}

/// 将字段中转义的双引号 `""` 还原为 `"`；未转义字段直接零拷贝返回原切片
export fn unescapeField(field: CsvField, out: []u8): ([]const u8)? {
    return unescapeFieldImpl(field, out);
}

/// RFC 4180 CSV / TSV 流式解析器（支持 `,` / `\t` / `;` 及 CRLF/LF）
export struct CsvReader {
    src: []const u8;
    pos: usize;
    delimiter: u8;
    pending_empty_field: bool;
    had_error: bool;

    /// 初始化流式 CSV/TSV 解析器
    fn init(src: []const u8, delimiter: u8): CsvReader {
        return CsvReader{
            src: src,
            pos: 0,
            delimiter: delimiter,
            pending_empty_field: false,
            had_error: false,
        };
    }

    /// 是否已到达输入末尾
    fn isAtEnd(): bool {
        return _.pos >= _.src.len && !_.pending_empty_field;
    }

    /// 还原字段中转义的 `""` 为 `"`；未转义字段零拷贝返回
    fn unescapeField(field: CsvField, out: []u8): ([]const u8)? {
        return unescapeFieldImpl(field, out);
    }

    /// 流式读取下一个字段；到达末尾或遇到未闭合引号时返回 null
    fn nextField(*_): CsvField? {
        if (_.pending_empty_field) {
            _.pending_empty_field = false;
            return CsvField{
                raw: "",
                was_quoted: false,
                has_escaped_quotes: false,
                is_row_end: true,
            };
        }
        if (_.pos >= _.src.len) {
            return null;
        }

        // 1. 双引号包裹字段
        if (_.src[_.pos] == '"') {
            _.pos += 1;
            const content_start = _.pos;
            let content_end = _.pos;
            let has_escaped = false;
            let closed = false;

            while (_.pos < _.src.len) {
                if (_.src[_.pos] == '"') {
                    if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '"') {
                        has_escaped = true;
                        _.pos += 2;
                        continue;
                    }
                    content_end = _.pos;
                    _.pos += 1;
                    closed = true;
                    break;
                }
                _.pos += 1;
            }

            if (!closed) {
                _.had_error = true;
                _.pos = _.src.len;
                return null;
            }

            if (_.pos < _.src.len && _.src[_.pos] == _.delimiter) {
                _.pos += 1;
                if (_.pos >= _.src.len) {
                    _.pending_empty_field = true;
                } else if (_.src[_.pos] == '\r') {
                    _.pos += 1;
                    if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                        _.pos += 1;
                    }
                    _.pending_empty_field = true;
                } else if (_.src[_.pos] == '\n') {
                    _.pos += 1;
                    _.pending_empty_field = true;
                }
                return CsvField{
                    raw: _.src[content_start..content_end],
                    was_quoted: true,
                    has_escaped_quotes: has_escaped,
                    is_row_end: false,
                };
            }

            if (_.pos < _.src.len && _.src[_.pos] == '\r') {
                _.pos += 1;
            }
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            return CsvField{
                raw: _.src[content_start..content_end],
                was_quoted: true,
                has_escaped_quotes: has_escaped,
                is_row_end: true,
            };
        }

        // 2. 非引号普通字段
        const start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != _.delimiter && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
            _.pos += 1;
        }
        const end = _.pos;

        if (_.pos < _.src.len && _.src[_.pos] == _.delimiter) {
            _.pos += 1;
            if (_.pos >= _.src.len) {
                _.pending_empty_field = true;
            } else if (_.src[_.pos] == '\r') {
                _.pos += 1;
                if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                    _.pos += 1;
                }
                _.pending_empty_field = true;
            } else if (_.src[_.pos] == '\n') {
                _.pos += 1;
                _.pending_empty_field = true;
            }
            return CsvField{
                raw: _.src[start..end],
                was_quoted: false,
                has_escaped_quotes: false,
                is_row_end: false,
            };
        }

        if (_.pos < _.src.len && _.src[_.pos] == '\r') {
            _.pos += 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }
        return CsvField{
            raw: _.src[start..end],
            was_quoted: false,
            has_escaped_quotes: false,
            is_row_end: true,
        };
    }

    /// 读取一整行（最多 32 列）；到达末尾或遇到未闭合引号异常时返回 null
    fn nextRow(*_): CsvRow? {
        if (_.isAtEnd()) {
            return null;
        }
        let row = CsvRow{
            fields: [
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
                EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD, EMPTY_FIELD,
            ],
            len: 0,
        };

        while (true) {
            if (field := _.nextField()) {
                if (row.len < CsvRow.MAX_COLS) {
                    row.fields[row.len] = field;
                    row.len += 1;
                }
                if (field.is_row_end) {
                    break;
                }
            } else {
                if (_.had_error || row.len == 0) {
                    return null;
                }
                break;
            }
        }
        return row;
    }
}

/// 判断字段值在 RFC 4180 编码中是否需要双引号包裹
export fn needsCsvQuoting(s: []const u8, delimiter: u8): bool {
    for (c of s) {
        if (c == delimiter || c == '"' || c == '\n' || c == '\r') {
            return true;
        }
    }
    return false;
}

/// 将单个字段编码为 RFC 4180 CSV/TSV 文本（自动加引号并转义内部 `"` 为 `""`）
export fn formatCsvField(s: []const u8, delimiter: u8, out: []u8): ([]const u8)? {
    if (!needsCsvQuoting(s, delimiter)) {
        if (out.len < s.len) {
            return null;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[i] = s[i];
            i += 1;
        }
        return out[0..s.len];
    }

    let pos: usize = 0;
    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;

    for (c of s) {
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

/// 将一行字段切片编码为 RFC 4180 CSV/TSV 单行文本（不含末尾换行符）
export fn formatCsvRow(fields: []const []const u8, delimiter: u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < fields.len) {
        if (i > 0) {
            if (pos >= out.len) return null;
            out[pos] = delimiter;
            pos += 1;
        }
        const written = formatCsvField(fields[i], delimiter, out[pos..]) ?? return null;
        pos += written.len;
        i += 1;
    }
    return out[0..pos];
}
