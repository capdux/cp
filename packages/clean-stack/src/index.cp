// @cap/clean-stack — Zero-allocation stack trace cleaner in pure CP
// Clean-room MIT implementation of npm `clean-stack` v5.

pub struct CleanStackOptions {
    pub pretty: bool,
    pub base_path: string,
    pub home_dir: string,

    pub static fn defaults(): CleanStackOptions {
        return CleanStackOptions {
            pretty: false,
            base_path: "",
            home_dir: "",
        };
    }
}

fn csContains(hay: string, needle: string): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    const limit = hay.len - needle.len + 1;
    while (i < limit) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn csStartsWith(s: string, prefix: string): bool {
    if (prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn csAppend(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

/// Returns `true` if a stack frame line looks like a runtime-internal frame that should be filtered out.
pub fn isInternalStackFrame(line: string): bool {
    // Only filter stack frame lines (`    at ...`)
    let a: usize = 0;
    while (a < line.len && (line[a] == 32 || line[a] == 9)) {
        a += 1;
    }
    if (!csStartsWith(line[a..line.len], "at ")) {
        return false;
    }
    if (csContains(line, "(node:internal/") || csContains(line, " node:internal/")) {
        return true;
    }
    if (csContains(line, "(internal/") || csContains(line, " internal/")) {
        return true;
    }
    if (csContains(line, "node:events") || csContains(line, "node:timers") || csContains(line, "node:module")) {
        return true;
    }
    if (csContains(line, "(native)") || csContains(line, "<anonymous>")) {
        return true;
    }
    if (csContains(line, "/node_modules/pirates/") || csContains(line, "/node_modules/ava/")) {
        return true;
    }
    return false;
}

fn csWriteCleanedLine(line: string, opts: CleanStackOptions, out: []u8, pos: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < line.len && p < out.len) {
        // Strip file:// prefix
        if (csStartsWith(line[i..line.len], "file://")) {
            i += 7;
            continue;
        }
        // Strip base_path prefix if configured
        if (opts.base_path.len > 0 && csStartsWith(line[i..line.len], opts.base_path)) {
            i += opts.base_path.len;
            if (i < line.len && (line[i] == 47 || line[i] == 92)) {
                i += 1;
            }
            continue;
        }
        // Replace home_dir with '~' when pretty == true
        if (opts.pretty && opts.home_dir.len > 0 && csStartsWith(line[i..line.len], opts.home_dir)) {
            p = csAppend(out, p, "~");
            i += opts.home_dir.len;
            continue;
        }
        out[p] = line[i];
        p += 1;
        i += 1;
    }
    return p;
}

/// Cleans a multi-line stack trace using `opts`, filtering internal frames and normalizing paths.
pub fn cleanStackWithOptions(stack: string, opts: CleanStackOptions, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    let first_written = true;
    while (i < stack.len) {
        const line_start = i;
        while (i < stack.len && stack[i] != 10) {
            i += 1;
        }
        let line_end = i;
        if (line_end > line_start && stack[line_end - 1] == 13) {
            line_end -= 1;
        }
        if (i < stack.len && stack[i] == 10) {
            i += 1;
        }
        const line = stack[line_start..line_end];
        if (isInternalStackFrame(line)) {
            continue;
        }
        if (!first_written && p < out.len) {
            out[p] = 10;
            p += 1;
        }
        p = csWriteCleanedLine(line, opts, out, p);
        first_written = false;
    }
    return p;
}

/// Cleans a stack trace with default options (`pretty=false`, no `base_path`).
pub fn cleanStack(stack: string, out: []u8): usize {
    return cleanStackWithOptions(stack, CleanStackOptions.defaults(), out);
}

export {
    CleanStackOptions,
    isInternalStackFrame,
    cleanStack,
    cleanStackWithOptions,
};
