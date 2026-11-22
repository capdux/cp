// Origin: Pure-CP Stdlib (Zero-Alloc RFC 4180 CSV/TSV Table Header Mapping & Typed Row Decoder) | Created: 2026-11-22
// Zero-heap-allocation RFC 4180 CSV/TSV reader, header-to-column lookup, typed scalar decoders
// (i64/f64/bool), field unescaping, and RFC 4180 row encoder over caller-provided buffers.

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn strEqLower(a: string, lower_b: string): bool {
    if (a.len != lower_b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (asciiLower(a[i]) != lower_b[i]) return false;
    }
    return true;
}

fn trimFieldWs(s: string, delimiter: u8): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || (s[start] == '\t' && delimiter != '\t'))) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || (s[end - 1] == '\t' && delimiter != '\t'))) : (end -= 1) {}
    return s[start..end];
}

fn trimScalarWs(s: string): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) : (end -= 1) {}
    if (end - start >= 2 && s[start] == '"' && s[end - 1] == '"') {
        start += 1;
        end -= 1;
        while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    }
    return s[start..end];
}

/// Configuration options for `CsvTableReader`.
pub struct CsvTableOptions {
    delimiter: u8 = ',';
    has_header: bool = true;
    trim_whitespace: bool = false;

    /// Standard RFC 4180 comma-separated CSV options.
    static fn csv(): CsvTableOptions {
        return CsvTableOptions{ delimiter: ',', has_header: true, trim_whitespace: false };
    }

    /// Tab-separated TSV options.
    static fn tsv(): CsvTableOptions {
        return CsvTableOptions{ delimiter: '\t', has_header: true, trim_whitespace: false };
    }
}

/// Metadata for a single parsed CSV/TSV row returned by `CsvTableReader.nextRow`.
pub struct CsvRowCursor {
    row_index: usize = 0;
    fields_count: usize = 0;
    fields_stored: usize = 0;
    truncated: bool = false;
    malformed: bool = false;
}

/// Zero-copy header column index backed by the caller-provided header names slice.
pub struct CsvTableHeader {
    names: []const string;

    /// Returns the zero-based column index for exact column `name`, or `null` if absent.
    fn columnIndex(name: string): usize? {
        let i: usize = 0;
        while (i < _.names.len) : (i += 1) {
            if (strEq(_.names[i], name)) {
                return i;
            }
        }
        return null;
    }

    /// Returns the number of header columns.
    fn columnCount(): usize {
        return _.names.len;
    }

    /// Returns the column name at `idx`, or `null` if out of bounds.
    fn nameAt(idx: usize): string? {
        if (idx >= _.names.len) return null;
        return _.names[idx];
    }
}

/// Zero-heap-allocation RFC 4180 CSV/TSV table reader supporting custom delimiters,
/// quoted fields with embedded delimiters/newlines, `""` escapes, and header mapping.
pub struct CsvTableReader {
    data: string = "";
    opts: CsvTableOptions = CsvTableOptions{};
    pos: usize = 0;
    row_index: usize = 0;

    /// Initializes a `CsvTableReader` over `data` with `opts`.
    static fn init(data: string, opts: CsvTableOptions): CsvTableReader {
        let start_pos: usize = 0;
        // Skip optional UTF-8 BOM (0xEF 0xBB 0xBF)
        if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
            start_pos = 3;
        }
        return CsvTableReader{
            data: data,
            opts: opts,
            pos: start_pos,
            row_index: 0,
        };
    }

    /// Resets the reader cursor back to the beginning of the input.
    fn reset(*_) {
        _.pos = 0;
        if (_.data.len >= 3 && _.data[0] == 0xEF && _.data[1] == 0xBB && _.data[2] == 0xBF) {
            _.pos = 3;
        }
        _.row_index = 0;
    }

    /// Parses the first row as a `CsvTableHeader`, storing column name slices into `out_names`.
    /// Returns `null` if the input is empty or the header row is malformed.
    fn parseHeader(*_, out_names: []string): CsvTableHeader? {
        const cur = _.nextRow(out_names) ?? return null;
        if (cur.malformed || cur.fields_stored == 0) {
            return null;
        }
        return CsvTableHeader{
            names: out_names[0..cur.fields_stored],
        };
    }

    /// Reads the next RFC 4180 row into `out_fields`, returning `CsvRowCursor` or `null` at EOF.
    /// Quoted fields are returned without their outer quotes (preserving inner `""` for `unescapeCsvField`).
    /// Unclosed quotes set `cursor.malformed = true`.
    fn nextRow(*_, out_fields: []string): CsvRowCursor? {
        if (_.pos >= _.data.len) {
            return null;
        }

        let cur = CsvRowCursor{
            row_index: _.row_index,
            fields_count: 0,
            fields_stored: 0,
            truncated: false,
            malformed: false,
        };
        _.row_index += 1;

        const delim = _.opts.delimiter;
        const n = _.data.len;

        while (true) {
            let field_slice: string = "";

            if (_.pos < n && _.data[_.pos] == '"') {
                // Quoted RFC 4180 field
                _.pos += 1;
                const content_start = _.pos;
                let closed = false;
                let content_end = _.pos;

                while (_.pos < n) {
                    if (_.data[_.pos] == '"') {
                        if (_.pos + 1 < n && _.data[_.pos + 1] == '"') {
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
                    cur.malformed = true;
                    _.pos = n;
                    return cur;
                }

                field_slice = _.data[content_start..content_end] as string;

                // Consume optional spaces before delimiter or line break
                while (_.pos < n && _.data[_.pos] == ' ' && delim != ' ') : (_.pos += 1) {}
                if (_.pos < n && _.data[_.pos] != delim && _.data[_.pos] != '\r' && _.data[_.pos] != '\n') {
                    cur.malformed = true;
                    // Advance to end of line to keep stream recoverable
                    while (_.pos < n && _.data[_.pos] != '\n' && _.data[_.pos] != '\r') : (_.pos += 1) {}
                    if (_.pos < n && _.data[_.pos] == '\r') _.pos += 1;
                    if (_.pos < n && _.data[_.pos] == '\n') _.pos += 1;
                    return cur;
                }
            } else {
                // Unquoted field
                const start = _.pos;
                while (_.pos < n && _.data[_.pos] != delim && _.data[_.pos] != '\r' && _.data[_.pos] != '\n') : (_.pos += 1) {}
                field_slice = _.data[start.._.pos] as string;
                if (_.opts.trim_whitespace) {
                    field_slice = trimFieldWs(field_slice, delim);
                }
            }

            cur.fields_count += 1;
            if (cur.fields_stored < out_fields.len) {
                out_fields[cur.fields_stored] = field_slice;
                cur.fields_stored += 1;
            } else {
                cur.truncated = true;
            }

            if (_.pos >= n) {
                break;
            }
            if (_.data[_.pos] == delim) {
                _.pos += 1;
                continue;
            }
            if (_.data[_.pos] == '\r') {
                _.pos += 1;
                if (_.pos < n && _.data[_.pos] == '\n') {
                    _.pos += 1;
                }
                break;
            }
            if (_.data[_.pos] == '\n') {
                _.pos += 1;
                break;
            }
        }

        return cur;
    }
}

// ============================================================================
// RFC 4180 Field Unescaping & Strongly-Typed Scalar Decoders
// ============================================================================

/// Unescapes an RFC 4180 CSV field into `out_buf`, converting `""` to `"`.
/// Also strips matching outer quotes if present. Returns bytes written, or `null` if `out_buf` is too small.
pub fn unescapeCsvField(raw: string, out_buf: []u8): usize? {
    let start: usize = 0;
    let end: usize = raw.len;
    if (end - start >= 2 && raw[start] == '"' && raw[end - 1] == '"') {
        start += 1;
        end -= 1;
    }

    let i = start;
    let out_pos: usize = 0;
    while (i < end) {
        if (out_pos >= out_buf.len) return null;
        if (raw[i] == '"' && i + 1 < end && raw[i + 1] == '"') {
            out_buf[out_pos] = '"';
            out_pos += 1;
            i += 2;
        } else {
            out_buf[out_pos] = raw[i];
            out_pos += 1;
            i += 1;
        }
    }
    return out_pos;
}

/// Parses a signed 64-bit integer from a CSV field (trims surrounding spaces/quotes).
/// Returns `null` on empty input, invalid characters, or `i64` overflow.
pub fn parseCsvI64(field: string): i64? {
    const s = trimScalarWs(field);
    if (s.len == 0) return null;

    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return null;

    const max_mag: u64 = if (neg) 9223372036854775808 else 9223372036854775807;
    let acc: u64 = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b < '0' || b > '9') return null;
        const d = (b - '0') as u64;
        if (acc > (max_mag - d) / 10) return null;
        acc = acc * 10 + d;
    }

    if (neg) {
        if (acc == 9223372036854775808) {
            return -9223372036854775807 - 1;
        }
        return -(acc as i64);
    }
    return acc as i64;
}

/// Parses a 64-bit IEEE-754 floating-point number from a CSV field (supports sign, decimals, and `e`/`E` exponent).
/// Returns `null` on empty or malformed input.
pub fn parseCsvF64(field: string): f64? {
    const s = trimScalarWs(field);
    if (s.len == 0) return null;

    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return null;

    let int_part: f64 = 0.0;
    let has_digits = false;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {
        has_digits = true;
        int_part = int_part * 10.0 + ((s[i] - '0') as f64);
    }

    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {
            has_digits = true;
            frac_part = frac_part * 10.0 + ((s[i] - '0') as f64);
            frac_scale = frac_scale * 10.0;
        }
    }

    if (!has_digits) return null;
    let val: f64 = int_part + (frac_part / frac_scale);

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || s[i] < '0' || s[i] > '9') return null;
        let exp_val: u32 = 0;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + ((s[i] - '0') as u32);
            }
        }
        if (exp_val > 308) return null;
        let p: u32 = 0;
        while (p < exp_val) : (p += 1) {
            if (exp_neg) {
                val = val / 10.0;
            } else {
                val = val * 10.0;
            }
        }
    }

    if (i != s.len) return null;
    return if (neg) -val else val;
}

/// Parses a boolean from a CSV field (case-insensitive `true`/`false`/`1`/`0`/`yes`/`no`).
/// Returns `null` if the field does not match a recognized boolean literal.
pub fn parseCsvBool(field: string): bool? {
    const s = trimScalarWs(field);
    if (s.len == 0) return null;
    if (strEqLower(s, "true") || strEq(s, "1") || strEqLower(s, "yes")) return true;
    if (strEqLower(s, "false") || strEq(s, "0") || strEqLower(s, "no")) return false;
    return null;
}

// ============================================================================
// RFC 4180 Zero-Alloc Row Writer
// ============================================================================

fn fieldNeedsQuoting(f: string, delimiter: u8): bool {
    let i: usize = 0;
    while (i < f.len) : (i += 1) {
        const b = f[i];
        if (b == delimiter || b == '"' || b == '\n' || b == '\r') {
            return true;
        }
    }
    return false;
}

/// Encodes `fields` as a single RFC 4180 CSV/TSV row (without trailing newline) into `out_buf`.
/// Automatically wraps fields containing `delimiter`, `"`, `\r`, or `\n` in double quotes
/// and escapes internal `"` as `""`. Returns bytes written, or `null` if `out_buf` is too small.
pub fn writeCsvRow(fields: []const string, delimiter: u8, out_buf: []u8): usize? {
    let out_pos: usize = 0;
    let idx: usize = 0;
    while (idx < fields.len) : (idx += 1) {
        if (idx > 0) {
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = delimiter;
            out_pos += 1;
        }
        const f = fields[idx];
        if (fieldNeedsQuoting(f, delimiter)) {
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = '"';
            out_pos += 1;

            let j: usize = 0;
            while (j < f.len) : (j += 1) {
                const b = f[j];
                if (b == '"') {
                    if (out_pos + 2 > out_buf.len) return null;
                    out_buf[out_pos] = '"';
                    out_buf[out_pos + 1] = '"';
                    out_pos += 2;
                } else {
                    if (out_pos >= out_buf.len) return null;
                    out_buf[out_pos] = b;
                    out_pos += 1;
                }
            }

            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = '"';
            out_pos += 1;
        } else {
            if (out_pos + f.len > out_buf.len) return null;
            let j: usize = 0;
            while (j < f.len) : (j += 1) {
                out_buf[out_pos + j] = f[j];
            }
            out_pos += f.len;
        }
    }
    return out_pos;
}
