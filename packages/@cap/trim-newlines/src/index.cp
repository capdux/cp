// @cap/trim-newlines — Zero-allocation CR/LF leading & trailing newline trimmer in pure CP
// Clean-room MIT implementation of npm `trim-newlines` v5.

fn tnlIsNewline(b: u8): bool {
    return b == 10 || b == 13; // '\n' or '\r'
}

/// Returns a zero-copy slice with all leading `\r` and `\n` bytes removed.
pub fn trimNewlinesStart(input: string): string {
    let start: usize = 0;
    while (start < input.len && tnlIsNewline(input[start])) {
        start += 1;
    }
    return input[start..input.len];
}

/// Returns a zero-copy slice with all trailing `\r` and `\n` bytes removed.
pub fn trimNewlinesEnd(input: string): string {
    let end: usize = input.len;
    while (end > 0 && tnlIsNewline(input[end - 1])) {
        end -= 1;
    }
    return input[0..end];
}

/// Returns a zero-copy slice with both leading and trailing `\r` and `\n` bytes removed.
pub fn trimNewlines(input: string): string {
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && tnlIsNewline(input[start])) {
        start += 1;
    }
    while (end > start && tnlIsNewline(input[end - 1])) {
        end -= 1;
    }
    return input[start..end];
}

/// Counts leading `\r`/`\n` bytes.
pub fn countLeadingNewlines(input: string): usize {
    let n: usize = 0;
    while (n < input.len && tnlIsNewline(input[n])) {
        n += 1;
    }
    return n;
}

/// Counts trailing `\r`/`\n` bytes.
pub fn countTrailingNewlines(input: string): usize {
    let end: usize = input.len;
    let n: usize = 0;
    while (end > 0 && tnlIsNewline(input[end - 1])) {
        end -= 1;
        n += 1;
    }
    return n;
}

export {
    trimNewlines,
    trimNewlinesStart,
    trimNewlinesEnd,
    countLeadingNewlines,
    countTrailingNewlines,
};
