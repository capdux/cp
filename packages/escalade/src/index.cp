// @cap/escalade — Zero-heap ancestor directory path walker
// Clean-room MIT CP implementation of npm `escalade` (120M+/wk, dependency of yargs / find-up / pkg-dir)
// Reuses `@cap/normalize-path` for cross-platform separator normalization.

import { normalizePath } from "../../normalize-path/src/index.cp";

pub const ESCALADE_MAX_PATH: usize = 512;

/// Zero-heap stateful ancestor directory walker.
/// Starting from `start_dir`, each call to `next(out)` yields the current directory
/// and then ascends one level toward the filesystem root (`"/"`, `"C:/"`, or `"."`).
pub struct AncestorWalker {
    buf: [512]u8,
    len: usize,
    done: bool,

    pub static fn new(start_path: string): AncestorWalker {
        let w = AncestorWalker {
            buf: undefined,
            len: 0,
            done: false,
        };
        if (start_path.len == 0) {
            w.done = true;
            return w;
        }
        w.len = normalizePath(start_path, true, w.buf[0..512]);
        if (w.len == 0) {
            w.done = true;
        }
        return w;
    }

    pub fn isDone(): bool {
        return _.done;
    }

    /// Writes the current ancestor path into `out` and advances to its parent directory.
    /// Returns the byte length written, or `0` when all ancestors have been visited.
    pub fn next(*_, out: []u8): usize {
        if (_.done || _.len == 0) {
            _.done = true;
            return 0;
        }
        let copy_len = if (_.len <= out.len) { _.len } else { out.len };
        let i: usize = 0;
        while (i < copy_len) {
            out[i] = _.buf[i];
            i = i + 1;
        }

        // Compute parent directory in-place
        // Case 1: already at POSIX root "/"
        if (_.len == 1 && _.buf[0] == 47) {
            _.done = true;
            return copy_len;
        }
        // Case 2: already at Windows drive root "C:" or "C:/"
        if ((_.len == 2 || (_.len == 3 && _.buf[2] == 47)) && _.buf[1] == 58) {
            _.done = true;
            return copy_len;
        }

        // Find last '/'
        let last_slash: i64 = -1;
        let k: usize = _.len;
        while (k > 0) {
            k = k - 1;
            if (_.buf[k] == 47) {
                last_slash = k as i64;
                break;
            }
        }

        if (last_slash < 0) {
            // Relative single segment like "src" -> done after yielding
            _.done = true;
        } else if (last_slash == 0) {
            // "/foo" -> "/"
            _.len = 1;
        } else if (last_slash == 2 && _.buf[1] == 58) {
            // "C:/foo" -> "C:/"
            _.len = 3;
        } else {
            _.len = last_slash as usize;
        }
        return copy_len;
    }

    /// Counts how many ancestor levels remain (including current).
    pub fn remainingDepth(): usize {
        let clone = AncestorWalker {
            buf: _.buf,
            len: _.len,
            done: _.done,
        };
        let tmp: [1]u8 = undefined;
        let count: usize = 0;
        while (!clone.isDone()) {
            let n = clone.next(tmp[0..1]);
            if (n == 0) { break; }
            count = count + 1;
        }
        return count;
    }
}

/// Joins an ancestor directory `dir` and a target filename `name` into `out` (e.g. `"/a/b"` + `"package.json"` -> `"/a/b/package.json"`).
pub fn joinCandidate(dir: string, name: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < dir.len && pos < out.len) {
        out[pos] = dir[i];
        pos = pos + 1;
        i = i + 1;
    }
    let need_slash = (dir.len > 0 && dir[dir.len - 1] != 47);
    if (need_slash && pos < out.len) {
        out[pos] = 47;
        pos = pos + 1;
    }
    let j: usize = 0;
    while (j < name.len && pos < out.len) {
        out[pos] = name[j];
        pos = pos + 1;
        j = j + 1;
    }
    return pos;
}
