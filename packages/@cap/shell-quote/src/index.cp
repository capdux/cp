// @cap/shell-quote — Zero-heap POSIX Shell Argument Quoting & Command-Line Tokenizer
// Clean-room MIT implementation in pure CP (modeled after npm `shell-quote`).

fn isSafeBareChar(c: u8): bool {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '_' || c == '-' || c == '.' || c == '/' || c == ':' || c == '@' || c == '+' || c == ',' || c == '=';
}

pub fn needsQuoting(arg: []const u8): bool {
    if (arg.len == 0) {
        return true;
    }
    let i: usize = 0;
    while (i < arg.len) {
        if (!isSafeBareChar(arg[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

// Safely quote a single argument for POSIX sh using single-quote escaping (' -> '\'' )
pub fn quoteArg(arg: []const u8, dst: []u8): i64 {
    if (arg.len == 0) {
        if (dst.len < 2) {
            return -1;
        }
        dst[0] = '\'';
        dst[1] = '\'';
        return 2;
    }
    if (!needsQuoting(arg)) {
        if (dst.len < arg.len) {
            return -1;
        }
        let i: usize = 0;
        while (i < arg.len) {
            dst[i] = arg[i];
            i += 1;
        }
        return arg.len as i64;
    }
    let pos: usize = 0;
    if (pos >= dst.len) { return -1; }
    dst[pos] = '\'';
    pos += 1;
    let i: usize = 0;
    while (i < arg.len) {
        let c: u8 = arg[i];
        if (c == '\'') {
            // Close single quote, emit \', reopen single quote: '\''
            if (pos + 4 > dst.len) { return -1; }
            dst[pos] = '\'';
            dst[pos + 1] = '\\';
            dst[pos + 2] = '\'';
            dst[pos + 3] = '\'';
            pos += 4;
        } else {
            if (pos >= dst.len) { return -1; }
            dst[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    if (pos >= dst.len) { return -1; }
    dst[pos] = '\'';
    pos += 1;
    return pos as i64;
}

// Quote an array of arguments joined by single spaces
pub fn quoteArgs(args: []const []const u8, dst: []u8): i64 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < args.len) {
        if (i > 0) {
            if (pos >= dst.len) { return -1; }
            dst[pos] = ' ';
            pos += 1;
        }
        let n: i64 = quoteArg(args[i], dst[pos..dst.len]);
        if (n < 0) { return -1; }
        pos = pos + (n as usize);
        i += 1;
    }
    return pos as i64;
}

pub struct ParsedArgSpan {
    start: usize,
    len: usize,
}

pub struct SplitResult {
    arg_count: usize,
    bytes_used: usize,
    ok: bool,
}

// Parse a POSIX-style command line into unescaped arguments stored consecutively in `out_buf` with spans in `out_spans`
pub fn splitCommand(cmd: []const u8, out_buf: []u8, out_spans: []ParsedArgSpan): SplitResult {
    let arg_count: usize = 0;
    let bpos: usize = 0;
    let i: usize = 0;
    let n: usize = cmd.len;

    while (i < n) {
        while (i < n && (cmd[i] == ' ' || cmd[i] == '\t' || cmd[i] == '\n' || cmd[i] == '\r')) {
            i += 1;
        }
        if (i >= n) {
            break;
        }
        if (arg_count >= out_spans.len) {
            return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
        }
        let start_bpos: usize = bpos;
        let in_single: bool = false;
        let in_double: bool = false;

        while (i < n) {
            let c: u8 = cmd[i];
            if (in_single) {
                if (c == '\'') {
                    in_single = false;
                    i += 1;
                } else {
                    if (bpos >= out_buf.len) {
                        return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
                    }
                    out_buf[bpos] = c;
                    bpos += 1;
                    i += 1;
                }
            } else if (in_double) {
                if (c == '"') {
                    in_double = false;
                    i += 1;
                } else if (c == '\\' && i + 1 < n) {
                    let nxt: u8 = cmd[i + 1];
                    if (nxt == '"' || nxt == '\\' || nxt == '$' || nxt == '`') {
                        if (bpos >= out_buf.len) {
                            return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
                        }
                        out_buf[bpos] = nxt;
                        bpos += 1;
                        i += 2;
                    } else {
                        if (bpos >= out_buf.len) {
                            return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
                        }
                        out_buf[bpos] = c;
                        bpos += 1;
                        i += 1;
                    }
                } else {
                    if (bpos >= out_buf.len) {
                        return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
                    }
                    out_buf[bpos] = c;
                    bpos += 1;
                    i += 1;
                }
            } else {
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    break;
                } else if (c == '\'') {
                    in_single = true;
                    i += 1;
                } else if (c == '"') {
                    in_double = true;
                    i += 1;
                } else if (c == '\\') {
                    if (i + 1 >= n) {
                        return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
                    }
                    if (bpos >= out_buf.len) {
                        return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
                    }
                    out_buf[bpos] = cmd[i + 1];
                    bpos += 1;
                    i += 2;
                } else {
                    if (bpos >= out_buf.len) {
                        return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
                    }
                    out_buf[bpos] = c;
                    bpos += 1;
                    i += 1;
                }
            }
        }

        if (in_single || in_double) {
            return SplitResult { arg_count: 0, bytes_used: 0, ok: false };
        }
        out_spans[arg_count] = ParsedArgSpan { start: start_bpos, len: bpos - start_bpos };
        arg_count += 1;
    }

    return SplitResult { arg_count: arg_count, bytes_used: bpos, ok: true };
}
