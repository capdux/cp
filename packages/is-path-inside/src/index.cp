// @cap/is-path-inside — Zero-allocation path containment check in pure CP
// Clean-room MIT implementation of npm `is-path-inside` (#655, 88.6M/wk).

fn ipiIsSep(b: u8): bool {
    return b == 47 || b == 92; // '/' or '\'
}

/// Normalizes a path into `out` (resolving `.` and `..`, collapsing slashes to '/').
/// Returns normalized length.
fn ipiNormalize(input: string, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let is_abs = ipiIsSep(input[0]);
    let seg_starts: [128]usize = [0; 128];
    let seg_lens: [128]usize = [0; 128];
    let seg_count: usize = 0;

    let i: usize = 0;
    while (i < input.len) {
        while (i < input.len && ipiIsSep(input[i])) {
            i += 1;
        }
        if (i >= input.len) {
            break;
        }
        const start = i;
        while (i < input.len && !ipiIsSep(input[i])) {
            i += 1;
        }
        const slen = i - start;
        if (slen == 1 && input[start] == 46) {
            // "." -> skip
            continue;
        }
        if (slen == 2 && input[start] == 46 && input[start + 1] == 46) {
            // ".."
            if (seg_count > 0) {
                const prev_s = seg_starts[seg_count - 1];
                const prev_l = seg_lens[seg_count - 1];
                const prev_is_dotdot = prev_l == 2 && input[prev_s] == 46 && input[prev_s + 1] == 46;
                if (!prev_is_dotdot) {
                    seg_count -= 1;
                    continue;
                }
            }
            if (!is_abs && seg_count < 128) {
                seg_starts[seg_count] = start;
                seg_lens[seg_count] = slen;
                seg_count += 1;
            }
            continue;
        }
        if (seg_count < 128) {
            seg_starts[seg_count] = start;
            seg_lens[seg_count] = slen;
            seg_count += 1;
        }
    }

    let pos: usize = 0;
    if (is_abs && pos < out.len) {
        out[pos] = 47;
        pos += 1;
    }
    let k: usize = 0;
    while (k < seg_count) {
        if (k > 0 && pos < out.len) {
            out[pos] = 47;
            pos += 1;
        }
        const s0 = seg_starts[k];
        const l0 = seg_lens[k];
        let j: usize = 0;
        while (j < l0 && pos < out.len) {
            out[pos] = input[s0 + j];
            pos += 1;
            j += 1;
        }
        k += 1;
    }
    return pos;
}

/// Returns `true` if `child_path` is strictly inside `parent_path` (resolving `.` and `..`).
/// Returns `false` if they are equal, if `child_path` escapes via `..`, or if one is absolute and the other relative.
pub fn isPathInside(child_path: string, parent_path: string): bool {
    if (child_path.len == 0 || parent_path.len == 0) {
        return false;
    }
    const child_abs = ipiIsSep(child_path[0]);
    const parent_abs = ipiIsSep(parent_path[0]);
    if (child_abs != parent_abs) {
        return false;
    }

    let cbuf: [1024]u8 = [0; 1024];
    let pbuf: [1024]u8 = [0; 1024];
    const clen = ipiNormalize(child_path, cbuf[0..1024]);
    const plen = ipiNormalize(parent_path, pbuf[0..1024]);

    if (clen <= plen) {
        return false;
    }

    // Special case: parent is "/"
    if (plen == 1 && pbuf[0] == 47) {
        return clen > 1 && cbuf[0] == 47;
    }

    // Check prefix match
    let i: usize = 0;
    while (i < plen) {
        if (cbuf[i] != pbuf[i]) {
            return false;
        }
        i += 1;
    }
    // Next character in child MUST be '/' so "/a/bc" is NOT inside "/a/b"
    return cbuf[plen] == 47;
}

/// Returns `true` if `child_path` is inside OR equal to `parent_path`.
pub fn isPathInsideOrEqual(child_path: string, parent_path: string): bool {
    if (isPathInside(child_path, parent_path)) {
        return true;
    }
    if (child_path.len == 0 || parent_path.len == 0) {
        return false;
    }
    let cbuf: [1024]u8 = [0; 1024];
    let pbuf: [1024]u8 = [0; 1024];
    const clen = ipiNormalize(child_path, cbuf[0..1024]);
    const plen = ipiNormalize(parent_path, pbuf[0..1024]);
    if (clen != plen) {
        return false;
    }
    let i: usize = 0;
    while (i < clen) {
        if (cbuf[i] != pbuf[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

export {
    isPathInside,
    isPathInsideOrEqual,
};
