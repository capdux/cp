// @cap/extract-stack — Zero-allocation & zero-copy Error.stack frame extractor for CP
// Clean-room MIT implementation with parity to npm `extract-stack` v2.

fn exsIsHspace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn exsIsFrameLine(line: string): bool {
    // Matches /^\s+at / (at least one leading space or tab followed by "at ")
    let i: usize = 0;
    while (i < line.len && exsIsHspace(line[i])) : (i += 1) {}
    if (i == 0) {
        return false;
    }
    if (i + 3 <= line.len && line[i] == 'a' && line[i + 1] == 't' && line[i + 2] == ' ') {
        return true;
    }
    return false;
}

fn exsFindFirstFrameOffset(stack: string): usize {
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= stack.len) {
        if (i == stack.len || stack[i] == '\n') {
            let line_end = i;
            if (line_end > line_start && stack[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (exsIsFrameLine(stack[line_start..line_end])) {
                return line_start;
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return stack.len;
}

fn exsTrimLine(line: string): string {
    let start: usize = 0;
    let end: usize = line.len;
    while (start < end && (line[start] == ' ' || line[start] == '\t' || line[start] == '\r')) {
        start += 1;
    }
    while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t' || line[end - 1] == '\r')) {
        end -= 1;
    }
    return line[start..end];
}

/// Zero-copy extraction of the actual stack frames portion from `Error.stack`,
/// skipping any single-line or multi-line `Error: message` header before the first
/// indented `    at ...` line. Returns `""` if no stack frame lines are found.
pub fn extractStack(stack: string): string {
    if (stack.len == 0) {
        return "";
    }
    const offset = exsFindFirstFrameOffset(stack);
    if (offset >= stack.len) {
        return "";
    }
    return stack[offset..stack.len];
}

/// Zero-copy extraction of the error header (name + possibly multi-line message)
/// preceding the first `    at ...` stack frame line, with trailing `\r`/`\n` stripped.
pub fn extractErrorHeader(stack: string): string {
    if (stack.len == 0) {
        return "";
    }
    let end = exsFindFirstFrameOffset(stack);
    while (end > 0 && (stack[end - 1] == '\n' || stack[end - 1] == '\r')) {
        end -= 1;
    }
    return stack[0..end];
}

/// Extracts individual stack frame lines from `stack`, stripping leading/trailing
/// whitespace and the leading `"at "` prefix from each frame, and writes slices into `out`.
/// Returns the number of frame lines written.
pub fn extractStackLines(stack: string, out: []string): usize {
    const frames = extractStack(stack);
    if (frames.len == 0 || out.len == 0) {
        return 0;
    }

    let count: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= frames.len && count < out.len) {
        if (i == frames.len || frames[i] == '\n') {
            const trimmed = exsTrimLine(frames[line_start..i]);
            if (trimmed.len > 0) {
                if (trimmed.len >= 3 && trimmed[0] == 'a' && trimmed[1] == 't' && trimmed[2] == ' ') {
                    out[count] = trimmed[3..trimmed.len];
                } else {
                    out[count] = trimmed;
                }
                count += 1;
            }
            line_start = i + 1;
        }
        i += 1;
    }

    return count;
}
