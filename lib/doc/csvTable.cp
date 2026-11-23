// Origin: Pure-CP Stdlib (Zero-Alloc RFC 4180 CSV/TSV Dialect Sniffer, Schema Inference, Filter/Sort/Aggregate & JSONL Engine) | Created: 2026-11-22
// 100% pure CP, zero dynamic heap allocation, zero external FFI dependencies, caller-slice tabular data engine:
//   - 100% backwards-compatible `CsvTableOptions`, `CsvRowCursor`, `CsvTableHeader`, `CsvTableReader`,
//     `unescapeCsvField`, `parseCsvI64`, `parseCsvF64`, `parseCsvBool`, `writeCsvRow`
//   - Automatic dialect sniffer (`sniffCsvDialect`: `,`/`\t`/`;`/`|`, `"`/`'`, CRLF/LF, UTF-8 BOM & header heuristic)
//   - Single-pass monotonic-lattice column schema inference (`inferCsvSchema`: empty/bool/int/float/iso_date/string)
//     with Welford online mean & variance statistics (`CsvColumnProfile`)
//   - Zero-heap streaming multi-predicate AND filter + column projection (`projectAndFilterCsv`),
//     external-slice group-by aggregation (`aggregateCsvByGroup`: count/sum/min/max/avg),
//     and O(N log N) iterative stable merge sort (`sortCsvRowsByColumn`: numeric or lexicographic asc/desc)
//   - Bidirectional RFC 4180 CSV <-> RFC 8259 JSONL (NDJSON) zero-heap converter (`csvToJsonl` / `jsonlToCsv`)

import { Simd } from "../simd/index.cp";

// ============================================================================
// 1. Error Set & String/Byte Helpers
// ============================================================================

/// Error set returned by extended CSV dialect, schema, query, sort, aggregate, and JSONL operations.
pub err CsvTableError {
    EmptyInput,
    MalformedQuote,
    ColumnCountMismatch,
    ColumnNotFound,
    BufferTooSmall,
    ScratchTooSmall,
    GroupOverflow,
    RowOverflow,
    InvalidJsonl,
    InvalidParameter,
}

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

fn strCmp(a: string, b: string): i32 {
    const min_len: usize = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn strContains(hay: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > hay.len) return false;
    let i: usize = 0;
    while (i + needle.len <= hay.len) : (i += 1) {
        let j: usize = 0;
        let ok: bool = true;
        while (j < needle.len) : (j += 1) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn strStartsWith(hay: string, pref: string): bool {
    if (pref.len > hay.len) return false;
    let i: usize = 0;
    while (i < pref.len) : (i += 1) {
        if (hay[i] != pref[i]) return false;
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

fn copyBytes(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

// ============================================================================
// 2. Backwards-Compatible RFC 4180 Reader, Header, Scalar Decoders & Row Writer
// ============================================================================

/// Configuration options for `CsvTableReader`.
pub struct CsvTableOptions {
    delimiter: u8 = ',';
    has_header: bool = true;
    trim_whitespace: bool = false;
    quote_char: u8 = '"';
    comment_char: u8 = 0;
    strict_column_count: bool = false;

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
/// custom quote characters, `#` comment lines, UTF-8 BOM, embedded newlines, `""` escapes, and header mapping.
pub struct CsvTableReader {
    data: string = "";
    opts: CsvTableOptions = CsvTableOptions{};
    pos: usize = 0;
    row_index: usize = 0;
    expected_columns: usize = 0;

    /// Initializes a `CsvTableReader` over `data` with `opts`.
    static fn init(data: string, opts: CsvTableOptions): CsvTableReader {
        let start_pos: usize = 0;
        if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
            start_pos = 3;
        }
        return CsvTableReader{
            data: data,
            opts: opts,
            pos: start_pos,
            row_index: 0,
            expected_columns: 0,
        };
    }

    /// Resets the reader cursor back to the beginning of the input.
    fn reset(*_) {
        _.pos = 0;
        if (_.data.len >= 3 && _.data[0] == 0xEF && _.data[1] == 0xBB && _.data[2] == 0xBF) {
            _.pos = 3;
        }
        _.row_index = 0;
        _.expected_columns = 0;
    }

    /// Parses the first row as a `CsvTableHeader`, storing column name slices into `out_names`.
    fn parseHeader(*_, out_names: []string): CsvTableHeader? {
        const cur = _.nextRow(out_names) ?? return null;
        if (cur.malformed || cur.fields_stored == 0) {
            return null;
        }
        _.expected_columns = cur.fields_count;
        return CsvTableHeader{
            names: out_names[0..cur.fields_stored],
        };
    }

    /// Reads the next RFC 4180 row into `out_fields`, returning `CsvRowCursor` or `null` at EOF.
    fn nextRow(*_, out_fields: []string): CsvRowCursor? {
        const n = _.data.len;
        const cchar = _.opts.comment_char;
        if (cchar != 0) {
            while (_.pos < n && _.data[_.pos] == cchar) {
                if (rel := Simd.indexOfAny2(_.data[_.pos..n], '\n', '\r')) {
                    _.pos += rel;
                } else {
                    _.pos = n;
                }
                if (_.pos < n && _.data[_.pos] == '\r') _.pos += 1;
                if (_.pos < n && _.data[_.pos] == '\n') _.pos += 1;
            }
        }
        if (_.pos >= n) {
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
        const qchar = _.opts.quote_char;

        while (true) {
            let field_slice: string = "";

            if (_.pos < n && qchar != 0 && _.data[_.pos] == qchar) {
                _.pos += 1;
                const content_start = _.pos;
                let closed = false;
                let content_end = _.pos;

                while (_.pos < n) {
                    if (rel := Simd.indexOfByte(_.data[_.pos..n], qchar)) {
                        _.pos += rel;
                        if (_.pos + 1 < n && _.data[_.pos + 1] == qchar) {
                            _.pos += 2;
                            continue;
                        }
                        content_end = _.pos;
                        _.pos += 1;
                        closed = true;
                        break;
                    } else {
                        _.pos = n;
                        break;
                    }
                }

                if (!closed) {
                    cur.malformed = true;
                    _.pos = n;
                    return cur;
                }

                field_slice = _.data[content_start..content_end] as string;

                while (_.pos < n && _.data[_.pos] == ' ' && delim != ' ') : (_.pos += 1) {}
                if (_.pos < n && _.data[_.pos] != delim && _.data[_.pos] != '\r' && _.data[_.pos] != '\n') {
                    cur.malformed = true;
                    if (rel := Simd.indexOfAny2(_.data[_.pos..n], '\n', '\r')) {
                        _.pos += rel;
                    } else {
                        _.pos = n;
                    }
                    if (_.pos < n && _.data[_.pos] == '\r') _.pos += 1;
                    if (_.pos < n && _.data[_.pos] == '\n') _.pos += 1;
                    return cur;
                }
            } else {
                const start = _.pos;
                if (rel := Simd.indexOfAny4(_.data[_.pos..n], delim, '\r', '\n', '\n')) {
                    _.pos += rel;
                } else {
                    _.pos = n;
                }
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

        if (_.opts.strict_column_count) {
            if (_.expected_columns == 0) {
                _.expected_columns = cur.fields_count;
            } else if (cur.fields_count != _.expected_columns) {
                cur.malformed = true;
            }
        }
        return cur;
    }
}

/// Unescapes an RFC 4180 CSV field into `out_buf`, converting `""` to `"`.
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
pub fn parseCsvBool(field: string): bool? {
    const s = trimScalarWs(field);
    if (s.len == 0) return null;
    if (strEqLower(s, "true") || strEq(s, "1") || strEqLower(s, "yes")) return true;
    if (strEqLower(s, "false") || strEq(s, "0") || strEqLower(s, "no")) return false;
    return null;
}

fn fieldNeedsQuoting(f: string, delimiter: u8): bool {
    return Simd.indexOfAny4(f, delimiter, '"', '\n', '\r') != null;
}

/// Encodes `fields` as a single RFC 4180 CSV/TSV row (without trailing newline) into `out_buf`.
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

// ============================================================================
// 3. Automatic Dialect Sniffer (`sniffCsvDialect`)
// ============================================================================

/// Detected line ending convention.
pub enum CsvLineEnding {
    lf,
    crlf,
}

/// Result of `sniffCsvDialect`.
pub struct CsvDialectInfo {
    delimiter: u8 = ',';
    quote_char: u8 = '"';
    line_ending: CsvLineEnding = CsvLineEnding.lf;
    has_header: bool = true;
    has_bom: bool = false;
    estimated_columns: usize = 1;
    sample_rows: usize = 0;

    /// Converts the sniffed dialect directly into `CsvTableOptions`.
    fn toOptions(): CsvTableOptions {
        return CsvTableOptions{
            delimiter: delimiter,
            has_header: has_header,
            trim_whitespace: false,
            quote_char: quote_char,
            comment_char: 0,
            strict_column_count: false,
        };
    }
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

/// Returns `true` for ISO-8601 date/datetime literals (`YYYY-MM-DD` or `YYYY-MM-DD[T ]HH:MM:SS...`).
pub fn isIsoDateField(field: string): bool {
    const s = trimScalarWs(field);
    if (s.len < 10) return false;
    if (!isDigit(s[0]) || !isDigit(s[1]) || !isDigit(s[2]) || !isDigit(s[3])) return false;
    if (s[4] != '-' || !isDigit(s[5]) || !isDigit(s[6]) || s[7] != '-' || !isDigit(s[8]) || !isDigit(s[9])) return false;
    const month: u8 = (s[5] - '0') * 10 + (s[6] - '0');
    const day: u8 = (s[8] - '0') * 10 + (s[9] - '0');
    if (month < 1 || month > 12 || day < 1 || day > 31) return false;
    if (s.len == 10) return true;
    return s[10] == 'T' || s[10] == ' ';
}

/// Automatically sniffs delimiter (`,` / `\t` / `;` / `|`), quote character (`"` / `'`),
/// line ending (`CRLF` / `LF`), UTF-8 BOM, and whether row 0 is a header.
pub fn sniffCsvDialect(data: string, max_sample_lines: usize): CsvDialectInfo {
    let info = CsvDialectInfo{};
    let start: usize = 0;
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        info.has_bom = true;
        start = 3;
    }
    if (start >= data.len) return info;

    // Detect CRLF vs LF and quote character frequency using SIMD @popCount byte counting
    const scan_lim: usize = if (data.len - start > 4096) (start + 4096) else data.len;
    const sample_slice: []const u8 = data[start..scan_lim];
    const cr_cnt: usize = Simd.countByte(sample_slice, '\r');
    const lf_cnt: usize = Simd.countByte(sample_slice, '\n');
    const dquote_cnt: usize = Simd.countByte(sample_slice, '"');
    const squote_cnt: usize = Simd.countByte(sample_slice, '\'');
    if (cr_cnt > 0 && cr_cnt * 2 >= lf_cnt) {
        info.line_ending = CsvLineEnding.crlf;
    }
    if (squote_cnt >= 2 && squote_cnt > dquote_cnt) {
        info.quote_char = '\'';
    }

    const candidates: [4]u8 = [',', '\t', ';', '|'];
    const max_lines: usize = if (max_sample_lines == 0) 16 else max_sample_lines;
    let best_delim: u8 = ',';
    let best_score: i64 = -1;
    let best_cols: usize = 1;
    let best_rows: usize = 0;

    let ci: usize = 0;
    while (ci < 4) : (ci += 1) {
        const d = candidates[ci];
        let r = CsvTableReader.init(data, CsvTableOptions{
            delimiter: d,
            has_header: false,
            quote_char: info.quote_char,
        });
        let tmp: [1]string = [""];
        let rows: usize = 0;
        let first_cols: usize = 0;
        let consistent: usize = 0;
        let malformed_cnt: usize = 0;
        while (rows < max_lines) {
            const opt = r.nextRow(tmp[0..]);
            if (cur := opt) {
                if (cur.malformed) malformed_cnt += 1;
                if (rows == 0) {
                    first_cols = cur.fields_count;
                } else if (cur.fields_count == first_cols) {
                    consistent += 1;
                }
                rows += 1;
            } else {
                break;
            }
        }
        if (rows == 0) continue;
        // Prefer delimiter with >1 columns, high row consistency, and zero malformed rows
        let score: i64 = 0;
        if (first_cols > 1) {
            score = ((first_cols as i64) * 10) + ((consistent as i64) * 100) - ((malformed_cnt as i64) * 500);
        }
        if (score > best_score) {
            best_score = score;
            best_delim = d;
            best_cols = first_cols;
            best_rows = rows;
        }
    }

    info.delimiter = best_delim;
    info.estimated_columns = best_cols;
    info.sample_rows = best_rows;

    // Header heuristic: compare row 0 vs row 1 (if row 0 has 0 numeric/date/bool cells and row 1 has >=1, header = true)
    if (best_rows >= 2) {
        let r2 = CsvTableReader.init(data, CsvTableOptions{
            delimiter: best_delim,
            has_header: false,
            quote_char: info.quote_char,
        });
        let row0: [32]string = [""; 32];
        let row1: [32]string = [""; 32];
        if (c0 := r2.nextRow(row0[0..])) {
            if (c1 := r2.nextRow(row1[0..])) {
                let r0_typed: usize = 0;
                let r1_typed: usize = 0;
                let k: usize = 0;
                while (k < c0.fields_stored) : (k += 1) {
                    const k0 = classifyCsvCell(row0[k]);
                    if (k0 != CsvColumnKind.string_col && k0 != CsvColumnKind.empty) r0_typed += 1;
                }
                k = 0;
                while (k < c1.fields_stored) : (k += 1) {
                    const k1 = classifyCsvCell(row1[k]);
                    if (k1 != CsvColumnKind.string_col && k1 != CsvColumnKind.empty) r1_typed += 1;
                }
                info.has_header = (r0_typed == 0 && r1_typed > 0) || (r0_typed < r1_typed);
            }
        }
    }
    return info;
}

// ============================================================================
// 4. Column Schema Inference & Welford Online Statistics (`inferCsvSchema`)
// ============================================================================

/// Monotonic column type lattice for automatic CSV schema inference.
pub enum CsvColumnKind {
    empty,
    bool_col,
    int_col,
    float_col,
    iso_date_col,
    string_col,
}

/// Classifies a single CSV cell into the narrowest matching `CsvColumnKind`.
pub fn classifyCsvCell(field: string): CsvColumnKind {
    const s = trimScalarWs(field);
    if (s.len == 0) return CsvColumnKind.empty;
    if (parseCsvI64(s) != null) return CsvColumnKind.int_col;
    if (parseCsvF64(s) != null) return CsvColumnKind.float_col;
    if (isIsoDateField(s)) return CsvColumnKind.iso_date_col;
    if (strEqLower(s, "true") || strEqLower(s, "false") || strEqLower(s, "yes") || strEqLower(s, "no")) {
        return CsvColumnKind.bool_col;
    }
    return CsvColumnKind.string_col;
}

/// Monotonically promotes `cur` column type with a newly observed `cell` type.
pub fn promoteCsvColumnKind(cur: CsvColumnKind, cell: CsvColumnKind): CsvColumnKind {
    if (cell == CsvColumnKind.empty) return cur;
    if (cur == CsvColumnKind.empty) return cell;
    if (cur == cell) return cur;
    if ((cur == CsvColumnKind.int_col && cell == CsvColumnKind.float_col)
        || (cur == CsvColumnKind.float_col && cell == CsvColumnKind.int_col))
    {
        return CsvColumnKind.float_col;
    }
    return CsvColumnKind.string_col;
}

/// Column type & streaming Welford statistical profile produced by `inferCsvSchema`.
pub struct CsvColumnProfile {
    name: string = "";
    kind: CsvColumnKind = CsvColumnKind.empty;
    non_null_count: u64 = 0;
    null_count: u64 = 0;
    min_val: f64 = 0.0;
    max_val: f64 = 0.0;
    sum_val: f64 = 0.0;
    mean_val: f64 = 0.0;
    m2_val: f64 = 0.0;
    numeric_count: u64 = 0;
    max_str_len: usize = 0;

    /// Population variance (`m2 / n`) computed via Welford's online algorithm.
    fn variance(): f64 {
        if (numeric_count == 0) return 0.0;
        return m2_val / (numeric_count as f64);
    }

    /// Sample variance (`m2 / (n - 1)`) computed via Welford's online algorithm.
    fn sampleVariance(): f64 {
        if (numeric_count <= 1) return 0.0;
        return m2_val / ((numeric_count - 1) as f64);
    }
}

/// Single-pass zero-heap CSV schema inference and Welford online statistics over arbitrary column counts.
pub fn inferCsvSchema(
    data: string,
    opts: CsvTableOptions,
    out_profiles: []CsvColumnProfile,
    scratch_fields: []string,
): usize ! CsvTableError {
    if (data.len == 0) return CsvTableError.EmptyInput;
    if (out_profiles.len == 0 || scratch_fields.len == 0) return CsvTableError.ScratchTooSmall;

    let reader = CsvTableReader.init(data, opts);
    let col_count: usize = 0;

    if (opts.has_header) {
        const hdr_cur = reader.nextRow(scratch_fields) orelse return CsvTableError.EmptyInput;
        if (hdr_cur.malformed) return CsvTableError.MalformedQuote;
        col_count = hdr_cur.fields_stored;
        if (col_count > out_profiles.len) return CsvTableError.ScratchTooSmall;
        let c: usize = 0;
        while (c < col_count) : (c += 1) {
            out_profiles[c] = CsvColumnProfile{ name: scratch_fields[c] };
        }
    }

    while (cur := reader.nextRow(scratch_fields)) {
        if (cur.malformed) return CsvTableError.MalformedQuote;
        if (col_count == 0) {
            col_count = cur.fields_stored;
            if (col_count > out_profiles.len) return CsvTableError.ScratchTooSmall;
            let c: usize = 0;
            while (c < col_count) : (c += 1) {
                out_profiles[c] = CsvColumnProfile{};
            }
        }
        let c: usize = 0;
        while (c < col_count) : (c += 1) {
            const cell: string = if (c < cur.fields_stored) scratch_fields[c] else "";
            const ck = classifyCsvCell(cell);
            let prof = *out_profiles[c];
            if (ck == CsvColumnKind.empty) {
                prof.null_count += 1;
            } else {
                prof.non_null_count += 1;
                if (cell.len > prof.max_str_len) prof.max_str_len = cell.len;
                prof.kind = promoteCsvColumnKind(prof.kind, ck);
                if (ck == CsvColumnKind.int_col || ck == CsvColumnKind.float_col) {
                    if (v := parseCsvF64(cell)) {
                        if (prof.numeric_count == 0) {
                            prof.min_val = v;
                            prof.max_val = v;
                        } else {
                            if (v < prof.min_val) prof.min_val = v;
                            if (v > prof.max_val) prof.max_val = v;
                        }
                        prof.numeric_count += 1;
                        prof.sum_val += v;
                        const delta: f64 = v - prof.mean_val;
                        prof.mean_val += delta / (prof.numeric_count as f64);
                        const delta2: f64 = v - prof.mean_val;
                        prof.m2_val += delta * delta2;
                    }
                }
            }
        }
    }
    return col_count;
}

// ============================================================================
// 5. Streaming Filter, Projection, Group-By Aggregation & Stable Merge Sort
// ============================================================================

/// Predicate comparison operator for `CsvFilterPred`.
pub enum CsvFilterOp {
    eq,
    neq,
    lt,
    lte,
    gt,
    gte,
    contains,
    prefix,
    is_empty,
    not_empty,
}

/// Column filter predicate (combined with logical AND across a slice of predicates).
pub struct CsvFilterPred {
    col_index: usize = 0;
    op: CsvFilterOp = CsvFilterOp.eq;
    str_val: string = "";
    num_val: f64 = 0.0;
    numeric_compare: bool = false;
}

/// Evaluates one `CsvFilterPred` against a single cell string.
pub fn evalCsvFilterPred(field: string, pred: CsvFilterPred): bool {
    const trimmed = trimScalarWs(field);
    if (pred.op == CsvFilterOp.is_empty) return trimmed.len == 0;
    if (pred.op == CsvFilterOp.not_empty) return trimmed.len > 0;
    if (pred.op == CsvFilterOp.contains) return strContains(field, pred.str_val);
    if (pred.op == CsvFilterOp.prefix) return strStartsWith(field, pred.str_val);

    if (pred.numeric_compare) {
        const v = parseCsvF64(trimmed) orelse return false;
        return match (pred.op) {
            CsvFilterOp.eq => v == pred.num_val,
            CsvFilterOp.neq => v != pred.num_val,
            CsvFilterOp.lt => v < pred.num_val,
            CsvFilterOp.lte => v <= pred.num_val,
            CsvFilterOp.gt => v > pred.num_val,
            CsvFilterOp.gte => v >= pred.num_val,
            else => false,
        };
    }

    const cmp = strCmp(field, pred.str_val);
    return match (pred.op) {
        CsvFilterOp.eq => cmp == 0,
        CsvFilterOp.neq => cmp != 0,
        CsvFilterOp.lt => cmp < 0,
        CsvFilterOp.lte => cmp <= 0,
        CsvFilterOp.gt => cmp > 0,
        CsvFilterOp.gte => cmp >= 0,
        else => false,
    };
}

/// Filters CSV rows by multi-predicate AND `preds`, projects `col_indices`, and writes canonical RFC 4180 CSV into `out_buf`.
pub fn projectAndFilterCsv(
    data: string,
    opts: CsvTableOptions,
    col_indices: []const usize,
    preds: []const CsvFilterPred,
    scratch_fields: []string,
    scratch_proj: []string,
    out_buf: []u8,
): usize ! CsvTableError {
    if (col_indices.len == 0) return CsvTableError.InvalidParameter;
    if (scratch_proj.len < col_indices.len) return CsvTableError.ScratchTooSmall;

    let reader = CsvTableReader.init(data, opts);
    let out_pos: usize = 0;

    if (opts.has_header) {
        const hcur = reader.nextRow(scratch_fields) orelse return CsvTableError.EmptyInput;
        if (hcur.malformed) return CsvTableError.MalformedQuote;
        let k: usize = 0;
        while (k < col_indices.len) : (k += 1) {
            const ci = col_indices[k];
            if (ci >= hcur.fields_stored) return CsvTableError.ColumnNotFound;
            scratch_proj[k] = scratch_fields[ci];
        }
        const wn = writeCsvRow(scratch_proj[0..col_indices.len], opts.delimiter, out_buf[out_pos..out_buf.len]) orelse return CsvTableError.BufferTooSmall;
        out_pos += wn;
        if (out_pos >= out_buf.len) return CsvTableError.BufferTooSmall;
        out_buf[out_pos] = '\n';
        out_pos += 1;
    }

    while (cur := reader.nextRow(scratch_fields)) {
        if (cur.malformed) return CsvTableError.MalformedQuote;
        let keep: bool = true;
        let pi: usize = 0;
        while (pi < preds.len) : (pi += 1) {
            const pr = preds[pi];
            const cell: string = if (pr.col_index < cur.fields_stored) scratch_fields[pr.col_index] else "";
            if (!evalCsvFilterPred(cell, pr)) {
                keep = false;
                break;
            }
        }
        if (!keep) continue;

        let k: usize = 0;
        while (k < col_indices.len) : (k += 1) {
            const ci = col_indices[k];
            scratch_proj[k] = if (ci < cur.fields_stored) scratch_fields[ci] else "";
        }
        const wn = writeCsvRow(scratch_proj[0..col_indices.len], opts.delimiter, out_buf[out_pos..out_buf.len]) orelse return CsvTableError.BufferTooSmall;
        out_pos += wn;
        if (out_pos >= out_buf.len) return CsvTableError.BufferTooSmall;
        out_buf[out_pos] = '\n';
        out_pos += 1;
    }
    return out_pos;
}

/// Group-by aggregation metric kind.
pub enum CsvAggKind {
    count,
    sum,
    min,
    max,
    avg,
}

/// Per-group aggregation accumulator stored in caller-provided external slice.
pub struct CsvGroupAggSlot {
    key: string = "";
    count: u64 = 0;
    sum: f64 = 0.0;
    min: f64 = 0.0;
    max: f64 = 0.0;

    fn avg(): f64 {
        if (count == 0) return 0.0;
        return sum / (count as f64);
    }

    fn metric(kind: CsvAggKind): f64 {
        return match (kind) {
            CsvAggKind.count => count as f64,
            CsvAggKind.sum => sum,
            CsvAggKind.min => min,
            CsvAggKind.max => max,
            CsvAggKind.avg => if (count == 0) 0.0 else (sum / (count as f64)),
        };
    }
}

/// Zero-heap group-by aggregation over `group_col` accumulating `count/sum/min/max/avg` on numeric `value_col`.
pub fn aggregateCsvByGroup(
    data: string,
    opts: CsvTableOptions,
    group_col: usize,
    value_col: usize,
    out_groups: []CsvGroupAggSlot,
    scratch_fields: []string,
): usize ! CsvTableError {
    if (out_groups.len == 0 || scratch_fields.len == 0) return CsvTableError.ScratchTooSmall;
    let reader = CsvTableReader.init(data, opts);
    if (opts.has_header) {
        const hcur = reader.nextRow(scratch_fields) orelse return CsvTableError.EmptyInput;
        if (hcur.malformed) return CsvTableError.MalformedQuote;
    }

    let group_cnt: usize = 0;
    while (cur := reader.nextRow(scratch_fields)) {
        if (cur.malformed) return CsvTableError.MalformedQuote;
        if (group_col >= cur.fields_stored || value_col >= cur.fields_stored) continue;
        const gkey = scratch_fields[group_col];
        const val = parseCsvF64(scratch_fields[value_col]) orelse continue;

        let found_idx: usize = group_cnt;
        let g: usize = 0;
        while (g < group_cnt) : (g += 1) {
            if (strEq(out_groups[g].key, gkey)) {
                found_idx = g;
                break;
            }
        }
        if (found_idx == group_cnt) {
            if (group_cnt >= out_groups.len) return CsvTableError.GroupOverflow;
            out_groups[group_cnt] = CsvGroupAggSlot{
                key: gkey,
                count: 1,
                sum: val,
                min: val,
                max: val,
            };
            group_cnt += 1;
        } else {
            let slot = *out_groups[found_idx];
            slot.count += 1;
            slot.sum += val;
            if (val < slot.min) slot.min = val;
            if (val > slot.max) slot.max = val;
        }
    }
    return group_cnt;
}

/// Row span + cached sort key used by `sortCsvRowsByColumn` (zero dynamic heap allocation).
pub struct CsvRowSpan {
    start: usize = 0;
    end: usize = 0;
    key_str: string = "";
    key_num: f64 = 0.0;
    has_num: bool = false;
    orig_index: usize = 0;
}

fn rowSpanLe(a: CsvRowSpan, b: CsvRowSpan, numeric: bool, descending: bool): bool {
    let cmp: i32 = 0;
    if (numeric) {
        if (a.has_num && b.has_num) {
            if (a.key_num < b.key_num) {
                cmp = -1;
            } else if (a.key_num > b.key_num) {
                cmp = 1;
            }
        } else if (a.has_num) {
            cmp = -1;
        } else if (b.has_num) {
            cmp = 1;
        }
    } else {
        cmp = strCmp(a.key_str, b.key_str);
    }
    if (cmp == 0) {
        return a.orig_index <= b.orig_index; // preserve stability
    }
    return if (descending) (cmp > 0) else (cmp < 0);
}

/// Stable O(N log N) iterative merge sort of CSV rows by `sort_col` (numeric or lexicographic, asc or desc).
pub fn sortCsvRowsByColumn(
    data: string,
    opts: CsvTableOptions,
    sort_col: usize,
    numeric: bool,
    descending: bool,
    scratch_rows: []CsvRowSpan,
    scratch_tmp: []CsvRowSpan,
    scratch_fields: []string,
    out_buf: []u8,
): usize ! CsvTableError {
    if (scratch_rows.len == 0 || scratch_tmp.len < scratch_rows.len) return CsvTableError.ScratchTooSmall;
    let reader = CsvTableReader.init(data, opts);
    let out_pos: usize = 0;

    if (opts.has_header) {
        const h_start: usize = reader.pos;
        const hcur = reader.nextRow(scratch_fields) orelse return CsvTableError.EmptyInput;
        if (hcur.malformed) return CsvTableError.MalformedQuote;
        const h_end: usize = reader.pos;
        const h_slice = data[h_start..h_end];
        if (out_pos + h_slice.len > out_buf.len) return CsvTableError.BufferTooSmall;
        copyBytes(out_buf, out_pos, h_slice);
        out_pos += h_slice.len;
        if (out_pos > 0 && out_buf[out_pos - 1] != '\n') {
            if (out_pos >= out_buf.len) return CsvTableError.BufferTooSmall;
            out_buf[out_pos] = '\n';
            out_pos += 1;
        }
    }

    let n_rows: usize = 0;
    while (true) {
        const r_start: usize = reader.pos;
        const opt = reader.nextRow(scratch_fields);
        if (cur := opt) {
            if (cur.malformed) return CsvTableError.MalformedQuote;
            const r_end: usize = reader.pos;
            if (n_rows >= scratch_rows.len) return CsvTableError.RowOverflow;
            const key_s: string = if (sort_col < cur.fields_stored) scratch_fields[sort_col] else "";
            let kn: f64 = 0.0;
            let hn: bool = false;
            if (numeric) {
                if (v := parseCsvF64(key_s)) {
                    kn = v;
                    hn = true;
                }
            }
            scratch_rows[n_rows] = CsvRowSpan{
                start: r_start,
                end: r_end,
                key_str: key_s,
                key_num: kn,
                has_num: hn,
                orig_index: n_rows,
            };
            n_rows += 1;
        } else {
            break;
        }
    }

    // Bottom-up iterative stable merge sort
    let width: usize = 1;
    while (width < n_rows) : (width *= 2) {
        let left: usize = 0;
        while (left < n_rows) : (left += width * 2) {
            const mid: usize = if (left + width < n_rows) (left + width) else n_rows;
            const right: usize = if (left + width * 2 < n_rows) (left + width * 2) else n_rows;
            let i: usize = left;
            let j: usize = mid;
            let k: usize = left;
            while (i < mid && j < right) {
                if (rowSpanLe(scratch_rows[i], scratch_rows[j], numeric, descending)) {
                    scratch_tmp[k] = scratch_rows[i];
                    i += 1;
                } else {
                    scratch_tmp[k] = scratch_rows[j];
                    j += 1;
                }
                k += 1;
            }
            while (i < mid) : (i += 1) {
                scratch_tmp[k] = scratch_rows[i];
                k += 1;
            }
            while (j < right) : (j += 1) {
                scratch_tmp[k] = scratch_rows[j];
                k += 1;
            }
        }
        let cp: usize = 0;
        while (cp < n_rows) : (cp += 1) {
            scratch_rows[cp] = scratch_tmp[cp];
        }
    }

    let r: usize = 0;
    while (r < n_rows) : (r += 1) {
        const sp = scratch_rows[r];
        let end_trim: usize = sp.end;
        while (end_trim > sp.start && (data[end_trim - 1] == '\n' || data[end_trim - 1] == '\r')) : (end_trim -= 1) {}
        const row_slice = data[sp.start..end_trim];
        if (out_pos + row_slice.len + 1 > out_buf.len) return CsvTableError.BufferTooSmall;
        copyBytes(out_buf, out_pos, row_slice);
        out_pos += row_slice.len;
        out_buf[out_pos] = '\n';
        out_pos += 1;
    }
    return out_pos;
}

// ============================================================================
// 6. Bidirectional Zero-Heap RFC 4180 CSV <-> RFC 8259 JSONL (NDJSON) Converter
// ============================================================================

fn writeJsonEscapedStr(out: []u8, pos: usize, raw_csv_field: string): usize ! CsvTableError {
    const hex_tbl: [16]u8 = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'];
    let cur: usize = pos;
    if (cur >= out.len) return CsvTableError.BufferTooSmall;
    out[cur] = '"';
    cur += 1;

    let i: usize = 0;
    while (i < raw_csv_field.len) {
        let ch: u8 = raw_csv_field[i];
        if (ch == '"' && i + 1 < raw_csv_field.len && raw_csv_field[i + 1] == '"') {
            i += 2; // unescape CSV "" -> single "
        } else {
            i += 1;
        }
        if (ch == '"' || ch == '\\') {
            if (cur + 2 > out.len) return CsvTableError.BufferTooSmall;
            out[cur] = '\\';
            out[cur + 1] = ch;
            cur += 2;
        } else if (ch == '\n') {
            if (cur + 2 > out.len) return CsvTableError.BufferTooSmall;
            out[cur] = '\\';
            out[cur + 1] = 'n';
            cur += 2;
        } else if (ch == '\r') {
            if (cur + 2 > out.len) return CsvTableError.BufferTooSmall;
            out[cur] = '\\';
            out[cur + 1] = 'r';
            cur += 2;
        } else if (ch == '\t') {
            if (cur + 2 > out.len) return CsvTableError.BufferTooSmall;
            out[cur] = '\\';
            out[cur + 1] = 't';
            cur += 2;
        } else if (ch < 0x20) {
            if (cur + 6 > out.len) return CsvTableError.BufferTooSmall;
            out[cur] = '\\';
            out[cur + 1] = 'u';
            out[cur + 2] = '0';
            out[cur + 3] = '0';
            out[cur + 4] = hex_tbl[(ch >> 4) as usize];
            out[cur + 5] = hex_tbl[(ch & 0x0F) as usize];
            cur += 6;
        } else {
            if (cur >= out.len) return CsvTableError.BufferTooSmall;
            out[cur] = ch;
            cur += 1;
        }
    }

    if (cur >= out.len) return CsvTableError.BufferTooSmall;
    out[cur] = '"';
    return cur + 1;
}

fn appendLiteral(out: []u8, pos: usize, lit: []const u8): usize ! CsvTableError {
    if (pos + lit.len > out.len) return CsvTableError.BufferTooSmall;
    copyBytes(out, pos, lit);
    return pos + lit.len;
}

/// Converts RFC 4180 CSV into RFC 8259 JSONL (NDJSON) using inferred or caller-supplied `profiles`.
/// Numbers & booleans are emitted unquoted, empty cells as `null`, and strings fully JSON-escaped.
pub fn csvToJsonl(
    data: string,
    opts: CsvTableOptions,
    profiles: []const CsvColumnProfile,
    scratch_headers: []string,
    scratch_fields: []string,
    out_buf: []u8,
): usize ! CsvTableError {
    let reader = CsvTableReader.init(data, opts);
    const hcur = reader.nextRow(scratch_headers) orelse return CsvTableError.EmptyInput;
    if (hcur.malformed) return CsvTableError.MalformedQuote;
    const ncols: usize = hcur.fields_stored;
    let out_pos: usize = 0;

    while (cur := reader.nextRow(scratch_fields)) {
        if (cur.malformed) return CsvTableError.MalformedQuote;
        out_pos = appendLiteral(out_buf, out_pos, "{")?;
        let c: usize = 0;
        while (c < ncols) : (c += 1) {
            if (c > 0) out_pos = appendLiteral(out_buf, out_pos, ",")?;
            out_pos = writeJsonEscapedStr(out_buf, out_pos, scratch_headers[c])?;
            out_pos = appendLiteral(out_buf, out_pos, ":")?;

            const cell: string = if (c < cur.fields_stored) scratch_fields[c] else "";
            const trimmed = trimScalarWs(cell);
            const kind: CsvColumnKind = if (c < profiles.len) profiles[c].kind else CsvColumnKind.string_col;

            if (trimmed.len == 0) {
                out_pos = appendLiteral(out_buf, out_pos, "null")?;
            } else if ((kind == CsvColumnKind.int_col || kind == CsvColumnKind.float_col) && parseCsvF64(trimmed) != null) {
                out_pos = appendLiteral(out_buf, out_pos, trimmed)?;
            } else if (kind == CsvColumnKind.bool_col) {
                if (bv := parseCsvBool(trimmed)) {
                    out_pos = appendLiteral(out_buf, out_pos, if (bv) "true" else "false")?;
                } else {
                    out_pos = writeJsonEscapedStr(out_buf, out_pos, cell)?;
                }
            } else {
                out_pos = writeJsonEscapedStr(out_buf, out_pos, cell)?;
            }
        }
        out_pos = appendLiteral(out_buf, out_pos, "}\n")?;
    }
    return out_pos;
}

fn skipJsonWs(s: string, p: usize): usize {
    let i: usize = p;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) : (i += 1) {}
    return i;
}

fn parseJsonStringSpan(s: string, p: usize, out_end: *usize): string ! CsvTableError {
    if (p >= s.len || s[p] != '"') return CsvTableError.InvalidJsonl;
    let i: usize = p + 1;
    const content_start: usize = i;
    while (i < s.len) {
        if (s[i] == '\\') {
            if (i + 1 >= s.len) return CsvTableError.InvalidJsonl;
            i += 2;
            continue;
        }
        if (s[i] == '"') {
            out_end.* = i + 1;
            return s[content_start..i] as string;
        }
        i += 1;
    }
    return CsvTableError.InvalidJsonl;
}

fn unescapeJsonToBuf(raw: string, dst: []u8): usize ! CsvTableError {
    let i: usize = 0;
    let out_i: usize = 0;
    while (i < raw.len) {
        if (out_i >= dst.len) return CsvTableError.BufferTooSmall;
        if (raw[i] == '\\' && i + 1 < raw.len) {
            const nxt = raw[i + 1];
            if (nxt == 'n') { dst[out_i] = '\n'; }
            else if (nxt == 'r') { dst[out_i] = '\r'; }
            else if (nxt == 't') { dst[out_i] = '\t'; }
            else { dst[out_i] = nxt; }
            out_i += 1;
            i += 2;
        } else {
            dst[out_i] = raw[i];
            out_i += 1;
            i += 1;
        }
    }
    return out_i;
}

/// Converts RFC 8259 JSONL (NDJSON) flat objects into canonical RFC 4180 CSV using `columns` schema.
/// `scratch_unesc` holds unescaped cell bytes for the current row; `scratch_fields` holds per-column slices.
pub fn jsonlToCsv(
    jsonl: string,
    columns: []const string,
    delimiter: u8,
    scratch_fields: []string,
    scratch_unesc: []u8,
    out_buf: []u8,
): usize ! CsvTableError {
    if (columns.len == 0) return CsvTableError.InvalidParameter;
    if (scratch_fields.len < columns.len) return CsvTableError.ScratchTooSmall;

    let out_pos: usize = 0;
    const hn = writeCsvRow(columns, delimiter, out_buf[0..out_buf.len]) orelse return CsvTableError.BufferTooSmall;
    out_pos += hn;
    if (out_pos >= out_buf.len) return CsvTableError.BufferTooSmall;
    out_buf[out_pos] = '\n';
    out_pos += 1;

    let pos: usize = 0;
    while (pos < jsonl.len) {
        let line_end: usize = pos;
        while (line_end < jsonl.len && jsonl[line_end] != '\n') : (line_end += 1) {}
        const line: string = jsonl[pos..line_end] as string;
        pos = if (line_end < jsonl.len) (line_end + 1) else jsonl.len;

        let p: usize = skipJsonWs(line, 0);
        if (p >= line.len) continue;
        if (line[p] != '{') return CsvTableError.InvalidJsonl;
        p += 1;

        let ci: usize = 0;
        while (ci < columns.len) : (ci += 1) {
            scratch_fields[ci] = "";
        }
        let unesc_pos: usize = 0;

        while (true) {
            p = skipJsonWs(line, p);
            if (p >= line.len) return CsvTableError.InvalidJsonl;
            if (line[p] == '}') break;

            let next_p: usize = p;
            const key = parseJsonStringSpan(line, p, *next_p)?;
            p = skipJsonWs(line, next_p);
            if (p >= line.len || line[p] != ':') return CsvTableError.InvalidJsonl;
            p = skipJsonWs(line, p + 1);
            if (p >= line.len) return CsvTableError.InvalidJsonl;

            let val_str: string = "";
            if (line[p] == '"') {
                let vend: usize = p;
                const raw_s = parseJsonStringSpan(line, p, *vend)?;
                p = vend;
                const ulen = unescapeJsonToBuf(raw_s, scratch_unesc[unesc_pos..scratch_unesc.len])?;
                val_str = scratch_unesc[unesc_pos..(unesc_pos + ulen)] as string;
                unesc_pos += ulen;
            } else {
                const vstart: usize = p;
                while (p < line.len && line[p] != ',' && line[p] != '}' && line[p] != ' ' && line[p] != '\t' && line[p] != '\r') : (p += 1) {}
                const lit: string = line[vstart..p] as string;
                val_str = if (strEq(lit, "null")) "" else lit;
            }

            ci = 0;
            while (ci < columns.len) : (ci += 1) {
                if (strEq(columns[ci], key)) {
                    scratch_fields[ci] = val_str;
                    break;
                }
            }

            p = skipJsonWs(line, p);
            if (p < line.len && line[p] == ',') {
                p += 1;
                continue;
            }
            if (p < line.len && line[p] == '}') break;
            return CsvTableError.InvalidJsonl;
        }

        const rn = writeCsvRow(scratch_fields[0..columns.len], delimiter, out_buf[out_pos..out_buf.len]) orelse return CsvTableError.BufferTooSmall;
        out_pos += rn;
        if (out_pos >= out_buf.len) return CsvTableError.BufferTooSmall;
        out_buf[out_pos] = '\n';
        out_pos += 1;
    }
    return out_pos;
}
