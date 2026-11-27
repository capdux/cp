// @cap/commondir — Zero-heap deepest common parent directory calculator in pure CP
// Clean-room MIT implementation of npm `commondir` (#976, 57.1M/wk).

fn cmdIsSep(b: u8): bool {
    return b == 47 || b == 92; // '/' or '\'
}

/// Computes the longest common directory prefix of two paths, returning a slice of `a`.
pub fn commonDirTwo(a: string, b: string): string {
    if (a.len == 0 || b.len == 0) {
        return "";
    }
    let min_len = a.len;
    if (b.len < min_len) { min_len = b.len; }

    let i: usize = 0;
    let last_sep: usize = 0;
    let saw_sep = false;

    while (i < min_len) {
        const ca = if (cmdIsSep(a[i])) (47 as u8) else a[i];
        const cb = if (cmdIsSep(b[i])) (47 as u8) else b[i];
        if (ca != cb) {
            break;
        }
        if (ca == 47) {
            last_sep = i;
            saw_sep = true;
        }
        i += 1;
    }

    // Exact match or one ends at a directory boundary of the other
    if (i == min_len) {
        if (a.len == b.len) {
            // Strip trailing slash unless root "/"
            if (i > 1 && cmdIsSep(a[i - 1])) {
                return a[0..(i - 1)];
            }
            return a[0..i];
        }
        if (a.len > min_len && cmdIsSep(a[min_len])) {
            if (min_len > 1 && cmdIsSep(a[min_len - 1])) {
                return a[0..(min_len - 1)];
            }
            return a[0..min_len];
        }
        if (b.len > min_len && cmdIsSep(b[min_len])) {
            if (min_len > 1 && cmdIsSep(b[min_len - 1])) {
                return a[0..(min_len - 1)];
            }
            return a[0..min_len];
        }
    }

    if (!saw_sep) {
        return "";
    }
    if (last_sep == 0) {
        return a[0..1]; // root "/"
    }
    return a[0..last_sep];
}

/// Computes the deepest common parent directory across all `paths`.
pub fn commonDir(paths: []const string): string {
    if (paths.len == 0) {
        return "";
    }
    let acc = paths[0];
    if (paths.len == 1) {
        if (acc.len > 1 && cmdIsSep(acc[acc.len - 1])) {
            return acc[0..(acc.len - 1)];
        }
        return acc;
    }
    let i: usize = 1;
    while (i < paths.len) {
        acc = commonDirTwo(acc, paths[i]);
        if (acc.len == 0) {
            return "";
        }
        i += 1;
    }
    return acc;
}

export {
    commonDirTwo,
    commonDir,
};
