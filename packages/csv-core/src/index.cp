// @cap/csv-core — Zero-allocation streaming RFC 4180 CSV state machine for CP
// Clean-room MIT implementation (Rust `csv-core` v0.2 parity).
// Supports arbitrary input chunking, output-buffer resumption, quoted fields with
// embedded delimiters/newlines, doubled-quote escaping (""), CRLF/LF, and custom delimiters.

pub const CSV_READ_INPUT_EMPTY: i32 = 0;
pub const CSV_READ_OUTPUT_FULL: i32 = 1;
pub const CSV_READ_FIELD: i32 = 2;
pub const CSV_READ_RECORD_END: i32 = 3;
pub const CSV_READ_END: i32 = 4;

const CSVC_STATE_START_RECORD: u8 = 0;
const CSVC_STATE_START_RECORD_CR: u8 = 1;
const CSVC_STATE_START_FIELD: u8 = 2;
const CSVC_STATE_IN_FIELD: u8 = 3;
const CSVC_STATE_IN_QUOTED_FIELD: u8 = 4;
const CSVC_STATE_IN_QUOTED_QUOTE: u8 = 5;
const CSVC_STATE_CR: u8 = 6;
const CSVC_STATE_AFTER_RECORD: u8 = 7;
const CSVC_STATE_EOF_RECORD_END: u8 = 8;
const CSVC_STATE_END: u8 = 9;

/// Result of a single `CsvReader.readField` step.
pub struct CsvReadResult {
    pub status: i32,
    pub bytes_read: usize,
    pub bytes_written: usize
}

// ============================================================================
// Private helpers (`csvc` prefix to avoid module-flattening symbol collisions)
// ============================================================================

type CsvcVec16 = @Vector(16, u8);

fn csvcFindFieldEnd(buf: []const u8, delim: u8): usize {
    let i: usize = 0;
    const len = buf.len;
    const v_del: CsvcVec16 = @splat(delim);
    const v_lf: CsvcVec16 = @splat(0x0A as u8);
    const v_cr: CsvcVec16 = @splat(0x0D as u8);
    while (i + 16 <= len) {
        const v: CsvcVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_del) || @reduce(.Or, v == v_lf) || @reduce(.Or, v == v_cr)) {
            let k: usize = 0;
            while (k < 16) {
                const b = buf[i + k];
                if (b == delim || b == 0x0A || b == 0x0D) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const b = buf[i];
        if (b == delim || b == 0x0A || b == 0x0D) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn csvcFindQuote(buf: []const u8, quote: u8, start: usize): usize {
    let i: usize = start;
    const len = buf.len;
    const v_q: CsvcVec16 = @splat(quote);
    while (i + 16 <= len) {
        const v: CsvcVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q)) {
            let k: usize = 0;
            while (k < 16) {
                if (buf[i + k] == quote) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (buf[i] == quote) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn csvcCountLf(buf: []const u8): u64 {
    let i: usize = 0;
    let total: u64 = 0;
    const len = buf.len;
    const v_lf: CsvcVec16 = @splat(0x0A as u8);
    while (i + 16 <= len) {
        const v: CsvcVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_lf)) {
            let k: usize = 0;
            while (k < 16) {
                if (buf[i + k] == 0x0A) {
                    total += 1;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (buf[i] == 0x0A) {
            total += 1;
        }
        i += 1;
    }
    return total;
}

fn csvcNeedsQuote(field: []const u8, delim: u8, quote: u8, always_quote: bool): bool {
    if (always_quote) {
        return true;
    }
    let i: usize = 0;
    const len = field.len;
    const v_del: CsvcVec16 = @splat(delim);
    const v_q: CsvcVec16 = @splat(quote);
    const v_lf: CsvcVec16 = @splat(0x0A as u8);
    const v_cr: CsvcVec16 = @splat(0x0D as u8);
    while (i + 16 <= len) {
        const v: CsvcVec16 = field[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_del) || @reduce(.Or, v == v_q) || @reduce(.Or, v == v_lf) || @reduce(.Or, v == v_cr)) {
            return true;
        }
        i += 16;
    }
    while (i < len) {
        let b = field[i];
        if (b == delim || b == quote || b == 0x0A || b == 0x0D) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

// ============================================================================
// Public Struct — CsvReader (Zero-Heap Streaming DFA)
// ============================================================================

/// Zero-allocation streaming RFC 4180 CSV reader state machine.
pub struct CsvReader {
    pub delimiter: u8,
    pub quote: u8,
    pub quoting: bool,
    state: u8,
    line_num: u64,

    pub static fn new(): CsvReader {
        return CsvReader.withDelimiter(0x2C); // ','
    }

    pub static fn withDelimiter(delim: u8): CsvReader {
        return CsvReader {
            delimiter: delim,
            quote: 0x22, // '"'
            quoting: true,
            state: CSVC_STATE_START_RECORD,
            line_num: 1
        };
    }

    pub fn line(): u64 {
        return _.line_num;
    }

    pub fn reset(*_) {
        _.state = CSVC_STATE_START_RECORD;
        _.line_num = 1;
    }

    /// Reads the next CSV field (or partial field on `CSV_READ_INPUT_EMPTY` / `CSV_READ_OUTPUT_FULL`).
    /// Pass an empty `input` slice (`input.len == 0`) to signal EOF.
    pub fn readField(*_, input: []const u8, out: []u8): CsvReadResult {
        let in_pos: usize = 0;
        let out_pos: usize = 0;
        let is_eof = input.len == 0;

        while (true) {
            let st = _.state;

            if (st == CSVC_STATE_END) {
                return CsvReadResult { status: CSV_READ_END, bytes_read: in_pos, bytes_written: out_pos };
            }

            if (st == CSVC_STATE_AFTER_RECORD) {
                _.state = CSVC_STATE_START_RECORD;
                return CsvReadResult { status: CSV_READ_RECORD_END, bytes_read: in_pos, bytes_written: out_pos };
            }

            if (st == CSVC_STATE_EOF_RECORD_END) {
                _.state = CSVC_STATE_END;
                return CsvReadResult { status: CSV_READ_RECORD_END, bytes_read: in_pos, bytes_written: out_pos };
            }

            if (st == CSVC_STATE_CR) {
                if (is_eof) {
                    _.line_num = _.line_num + 1;
                    _.state = CSVC_STATE_END;
                    return CsvReadResult { status: CSV_READ_RECORD_END, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (in_pos >= input.len) {
                    return CsvReadResult { status: CSV_READ_INPUT_EMPTY, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (input[in_pos] == 0x0A) {
                    in_pos = in_pos + 1;
                }
                _.line_num = _.line_num + 1;
                _.state = CSVC_STATE_START_RECORD;
                return CsvReadResult { status: CSV_READ_RECORD_END, bytes_read: in_pos, bytes_written: out_pos };
            }

            if (st == CSVC_STATE_START_RECORD) {
                if (is_eof) {
                    _.state = CSVC_STATE_END;
                    return CsvReadResult { status: CSV_READ_END, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (in_pos >= input.len) {
                    return CsvReadResult { status: CSV_READ_INPUT_EMPTY, bytes_read: in_pos, bytes_written: out_pos };
                }
                let b = input[in_pos];
                if (b == 0x0A) {
                    in_pos = in_pos + 1;
                    _.line_num = _.line_num + 1;
                    continue;
                }
                if (b == 0x0D) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_START_RECORD_CR;
                    continue;
                }
                if (b == _.delimiter) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_START_FIELD;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (_.quoting && b == _.quote) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_IN_QUOTED_FIELD;
                    continue;
                }
                _.state = CSVC_STATE_IN_FIELD;
                continue;
            }

            if (st == CSVC_STATE_START_RECORD_CR) {
                if (is_eof) {
                    _.line_num = _.line_num + 1;
                    _.state = CSVC_STATE_END;
                    return CsvReadResult { status: CSV_READ_END, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (in_pos >= input.len) {
                    return CsvReadResult { status: CSV_READ_INPUT_EMPTY, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (input[in_pos] == 0x0A) {
                    in_pos = in_pos + 1;
                }
                _.line_num = _.line_num + 1;
                _.state = CSVC_STATE_START_RECORD;
                continue;
            }

            if (st == CSVC_STATE_START_FIELD) {
                if (is_eof) {
                    _.state = CSVC_STATE_EOF_RECORD_END;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (in_pos >= input.len) {
                    return CsvReadResult { status: CSV_READ_INPUT_EMPTY, bytes_read: in_pos, bytes_written: out_pos };
                }
                let b = input[in_pos];
                if (b == _.delimiter) {
                    in_pos = in_pos + 1;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (b == 0x0A) {
                    in_pos = in_pos + 1;
                    _.line_num = _.line_num + 1;
                    _.state = CSVC_STATE_AFTER_RECORD;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (b == 0x0D) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_CR;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (_.quoting && b == _.quote) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_IN_QUOTED_FIELD;
                    continue;
                }
                _.state = CSVC_STATE_IN_FIELD;
                continue;
            }

            if (st == CSVC_STATE_IN_FIELD) {
                if (is_eof) {
                    _.state = CSVC_STATE_EOF_RECORD_END;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (in_pos >= input.len) {
                    return CsvReadResult { status: CSV_READ_INPUT_EMPTY, bytes_read: in_pos, bytes_written: out_pos };
                }
                let b = input[in_pos];
                if (b == _.delimiter) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_START_FIELD;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (b == 0x0A) {
                    in_pos = in_pos + 1;
                    _.line_num = _.line_num + 1;
                    _.state = CSVC_STATE_AFTER_RECORD;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (b == 0x0D) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_CR;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (out_pos >= out.len) {
                    return CsvReadResult { status: CSV_READ_OUTPUT_FULL, bytes_read: in_pos, bytes_written: out_pos };
                }
                let avail_in = input.len - in_pos;
                let avail_out = out.len - out_pos;
                let max_copy = if (avail_in < avail_out) { avail_in } else { avail_out };
                let chunk_len = csvcFindFieldEnd(input[in_pos..(in_pos + max_copy)], _.delimiter);
                @memcpy(out[out_pos..(out_pos + chunk_len)], input[in_pos..(in_pos + chunk_len)]);
                out_pos = out_pos + chunk_len;
                in_pos = in_pos + chunk_len;
                continue;
            }

            if (st == CSVC_STATE_IN_QUOTED_FIELD) {
                if (is_eof) {
                    _.state = CSVC_STATE_EOF_RECORD_END;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (in_pos >= input.len) {
                    return CsvReadResult { status: CSV_READ_INPUT_EMPTY, bytes_read: in_pos, bytes_written: out_pos };
                }
                let b = input[in_pos];
                if (b == _.quote) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_IN_QUOTED_QUOTE;
                    continue;
                }
                if (out_pos >= out.len) {
                    return CsvReadResult { status: CSV_READ_OUTPUT_FULL, bytes_read: in_pos, bytes_written: out_pos };
                }
                let avail_in = input.len - in_pos;
                let avail_out = out.len - out_pos;
                let max_copy = if (avail_in < avail_out) { avail_in } else { avail_out };
                let window = input[in_pos..(in_pos + max_copy)];
                let chunk_len = csvcFindQuote(window, _.quote, 0);
                let chunk = input[in_pos..(in_pos + chunk_len)];
                _.line_num = _.line_num + csvcCountLf(chunk);
                @memcpy(out[out_pos..(out_pos + chunk_len)], chunk);
                out_pos = out_pos + chunk_len;
                in_pos = in_pos + chunk_len;
                continue;
            }

            if (st == CSVC_STATE_IN_QUOTED_QUOTE) {
                if (is_eof) {
                    _.state = CSVC_STATE_EOF_RECORD_END;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (in_pos >= input.len) {
                    return CsvReadResult { status: CSV_READ_INPUT_EMPTY, bytes_read: in_pos, bytes_written: out_pos };
                }
                let b = input[in_pos];
                if (b == _.quote) {
                    if (out_pos >= out.len) {
                        return CsvReadResult { status: CSV_READ_OUTPUT_FULL, bytes_read: in_pos, bytes_written: out_pos };
                    }
                    out[out_pos] = _.quote;
                    out_pos = out_pos + 1;
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_IN_QUOTED_FIELD;
                    continue;
                }
                if (b == _.delimiter) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_START_FIELD;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (b == 0x0A) {
                    in_pos = in_pos + 1;
                    _.line_num = _.line_num + 1;
                    _.state = CSVC_STATE_AFTER_RECORD;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (b == 0x0D) {
                    in_pos = in_pos + 1;
                    _.state = CSVC_STATE_CR;
                    return CsvReadResult { status: CSV_READ_FIELD, bytes_read: in_pos, bytes_written: out_pos };
                }
                if (out_pos >= out.len) {
                    return CsvReadResult { status: CSV_READ_OUTPUT_FULL, bytes_read: in_pos, bytes_written: out_pos };
                }
                out[out_pos] = b;
                out_pos = out_pos + 1;
                in_pos = in_pos + 1;
                _.state = CSVC_STATE_IN_FIELD;
                continue;
            }
        }
    }
}

// ============================================================================
// Public Struct — CsvWriter (Zero-Heap RFC 4180 Encoder)
// ============================================================================

/// Zero-allocation RFC 4180 CSV writer.
pub struct CsvWriter {
    pub delimiter: u8,
    pub quote: u8,
    pub always_quote: bool,

    pub static fn new(): CsvWriter {
        return CsvWriter.withDelimiter(0x2C);
    }

    pub static fn withDelimiter(delim: u8): CsvWriter {
        return CsvWriter {
            delimiter: delim,
            quote: 0x22,
            always_quote: false
        };
    }

    /// Encodes a single CSV field into `out`, automatically quoting and escaping `"` as `""` when needed.
    /// Returns bytes written to `out`.
    pub fn writeField(field: []const u8, out: []u8): usize {
        if (out.len == 0) {
            return 0;
        }
        let quoted = csvcNeedsQuote(field, _.delimiter, _.quote, _.always_quote);
        let w: usize = 0;
        if (quoted) {
            if (w < out.len) {
                out[w] = _.quote;
                w = w + 1;
            }
            let i: usize = 0;
            while (i < field.len && w < out.len) {
                const next_q = csvcFindQuote(field, _.quote, i);
                if (next_q > i) {
                    const seg_len = next_q - i;
                    const avail = out.len - w;
                    const n = if (seg_len < avail) { seg_len } else { avail };
                    @memcpy(out[w..(w + n)], field[i..(i + n)]);
                    w = w + n;
                    i = i + n;
                    if (n < seg_len || i >= field.len) {
                        break;
                    }
                }
                if (w + 2 > out.len) {
                    break;
                }
                out[w] = _.quote;
                out[w + 1] = _.quote;
                w = w + 2;
                i = i + 1;
            }
            if (w < out.len) {
                out[w] = _.quote;
                w = w + 1;
            }
            return w;
        }

        let n = if (field.len < out.len) { field.len } else { out.len };
        @memcpy(out[0..n], field[0..n]);
        return n;
    }

    /// Encodes a full CSV record (`fields` joined by `delimiter` plus trailing `\n`) into `out`.
    /// Returns bytes written to `out`.
    pub fn writeRecord(fields: []const string, out: []u8): usize {
        let w: usize = 0;
        let i: usize = 0;
        while (i < fields.len && w < out.len) {
            if (i > 0) {
                out[w] = _.delimiter;
                w = w + 1;
                if (w >= out.len) {
                    break;
                }
            }
            let n = _.writeField(fields[i], out[w..out.len]);
            w = w + n;
            i = i + 1;
        }
        if (w < out.len) {
            out[w] = 0x0A;
            w = w + 1;
        }
        return w;
    }
}

// ============================================================================
// Public Convenience Counters
// ============================================================================

/// Counts total CSV records in `input` using the RFC 4180 streaming state machine
/// (correctly handling quoted newlines and empty lines).
pub fn countCsvRecords(input: []const u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let rdr = CsvReader.new();
    let scratch: [128]u8 = undefined;
    let pos: usize = 0;
    let records: usize = 0;
    while (true) {
        let slice = input[pos..input.len];
        let res = rdr.readField(slice, scratch[0..128]);
        pos = pos + res.bytes_read;
        if (res.status == CSV_READ_RECORD_END) {
            records = records + 1;
        } else if (res.status == CSV_READ_END) {
            break;
        }
    }
    return records;
}

/// Counts the number of fields in the first CSV record of `input`.
pub fn countCsvFieldsInLine(input: []const u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let rdr = CsvReader.new();
    let scratch: [128]u8 = undefined;
    let pos: usize = 0;
    let fields: usize = 0;
    while (true) {
        let slice = input[pos..input.len];
        let res = rdr.readField(slice, scratch[0..128]);
        pos = pos + res.bytes_read;
        if (res.status == CSV_READ_FIELD) {
            fields = fields + 1;
        } else if (res.status == CSV_READ_RECORD_END || res.status == CSV_READ_END) {
            break;
        }
    }
    return fields;
}

export {
    CSV_READ_INPUT_EMPTY,
    CSV_READ_OUTPUT_FULL,
    CSV_READ_FIELD,
    CSV_READ_RECORD_END,
    CSV_READ_END,
    CsvReadResult,
    CsvReader,
    CsvWriter,
    countCsvRecords,
    countCsvFieldsInLine
};
