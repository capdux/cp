// Source: Absorbed from /data/dev/bun/src/shell/builtin/export.cp | Synced: 2026-11-24T04:33:35Z
// ============================================================================
// lib/shell/exportBuiltin.cp — Zero-Alloc POSIX `export` Builtin & Env Table
// ============================================================================
// Implements Bun Shell's `export` builtin semantics with zero dynamic heap
// allocation:
//   - Validates POSIX shell variable identifiers (`[A-Za-z_][A-Za-z0-9_]*`).
//   - Supports bare name export (`export FOO` -> exports existing value or `""`)
//     and assignment export (`export FOO=bar=baz` -> splits on first `=`).
//   - No-argument mode (`formatSorted`) lexicographically sorts all exported
//     variables by key and formats `"KEY=VALUE\n"` into a caller-provided buffer.

pub const SHELL_EXPORT_MAX_VARS: usize = 64;

pub err ShellExportError {
    InvalidIdentifier,
    TableFull,
    BufferTooSmall,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn cmpBytesAsc(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn findEq(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '=') return i;
        i += 1;
    }
    return null;
}

/// Validates a POSIX shell variable identifier: `[A-Za-z_][A-Za-z0-9_]*`.
pub fn isValidShellIdent(name: []const u8): bool {
    if (name.len == 0) return false;
    const c0 = name[0];
    const valid_start = (c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '_';
    if (!valid_start) return false;
    let i: usize = 1;
    while (i < name.len) {
        const c = name[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

/// A single environment entry stored in `ShellExportTable`.
pub struct ShellExportEntry {
    key: []const u8 = "",
    value: []const u8 = "",
    exported: bool = false,
}

/// Fixed-capacity zero-allocation shell environment & `export` builtin table.
pub struct ShellExportTable {
    entries: [64]ShellExportEntry = [ShellExportEntry{}; 64],
    len: usize = 0,

    static fn init(): ShellExportTable {
        return ShellExportTable{};
    }

    fn findIndex(key: []const u8): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (eqlBytes(_.entries[i].key, key)) return i;
            i += 1;
        }
        return null;
    }

    /// Sets a local (non-exported) or exported shell variable.
    fn setVar(*_, key: []const u8, value: []const u8, exported: bool): ShellExportError!void {
        if (!isValidShellIdent(key)) return ShellExportError.InvalidIdentifier;
        if (idx := _.findIndex(key)) {
            _.entries[idx].value = value;
            if (exported) {
                _.entries[idx].exported = true;
            }
            return;
        }
        if (_.len >= SHELL_EXPORT_MAX_VARS) return ShellExportError.TableFull;
        _.entries[_.len] = ShellExportEntry{
            key: key,
            value: value,
            exported: exported,
        };
        _.len += 1;
    }

    /// Applies a single `export` builtin argument (aligned with Bun `Export.start`):
    /// - Empty string is ignored.
    /// - Bare `NAME`: validates identifier; promotes existing local var to exported,
    ///   or creates exported `NAME=""` if absent.
    /// - `NAME=VALUE`: splits on first `=`, validates `NAME`, sets value and marks exported.
    fn applyArg(*_, arg: []const u8): ShellExportError!void {
        if (arg.len == 0) return;

        if (eq_idx := findEq(arg)) {
            const key = arg[0..eq_idx];
            const val = arg[(eq_idx + 1)..arg.len];
            _.setVar(key, val, true)?;
            return;
        }

        if (!isValidShellIdent(arg)) return ShellExportError.InvalidIdentifier;
        if (idx := _.findIndex(arg)) {
            _.entries[idx].exported = true;
            return;
        }
        _.setVar(arg, "", true)?;
    }

    /// Applies a slice of `export` arguments sequentially. Returns the number
    /// of non-empty arguments applied.
    fn applyArgs(*_, args: []const []const u8): ShellExportError!usize {
        let applied: usize = 0;
        let i: usize = 0;
        while (i < args.len) {
            if (args[i].len > 0) {
                _.applyArg(args[i])?;
                applied += 1;
            }
            i += 1;
        }
        return applied;
    }

    fn get(key: []const u8): ([]const u8)? {
        if (idx := _.findIndex(key)) {
            return _.entries[idx].value;
        }
        return null;
    }

    fn isExported(key: []const u8): bool {
        if (idx := _.findIndex(key)) {
            return _.entries[idx].exported;
        }
        return false;
    }

    /// Removes a variable from the table (`unset`). Returns true if removed.
    fn unset(*_, key: []const u8): bool {
        if (idx := _.findIndex(key)) {
            let i = idx;
            while (i + 1 < _.len) {
                _.entries[i] = _.entries[i + 1];
                i += 1;
            }
            _.len -= 1;
            return true;
        }
        return false;
    }

    fn count(): usize {
        return _.len;
    }

    fn exportedCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].exported) n += 1;
            i += 1;
        }
        return n;
    }

    /// Formats all exported variables in ascending lexicographical key order as
    /// `"KEY=VALUE\n"` into `out` (aligned with Bun `export` no-argument output).
    fn formatSorted(out: []u8): ShellExportError!usize {
        let order: [64]usize = [0; 64];
        let count_exp: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].exported) {
                order[count_exp] = i;
                count_exp += 1;
            }
            i += 1;
        }

        // Insertion sort by key (ascending ASCII lexicographical order).
        let a: usize = 1;
        while (a < count_exp) {
            const cur_idx = order[a];
            let b = a;
            while (b > 0 && cmpBytesAsc(_.entries[order[b - 1]].key, _.entries[cur_idx].key) > 0) {
                order[b] = order[b - 1];
                b -= 1;
            }
            order[b] = cur_idx;
            a += 1;
        }

        let pos: usize = 0;
        let k: usize = 0;
        while (k < count_exp) {
            const ent = _.entries[order[k]];
            const need = ent.key.len + 1 + ent.value.len + 1;
            if (pos + need > out.len) return ShellExportError.BufferTooSmall;
            let j: usize = 0;
            while (j < ent.key.len) {
                out[pos + j] = ent.key[j];
                j += 1;
            }
            pos += ent.key.len;
            out[pos] = '=';
            pos += 1;
            j = 0;
            while (j < ent.value.len) {
                out[pos + j] = ent.value[j];
                j += 1;
            }
            pos += ent.value.len;
            out[pos] = '\n';
            pos += 1;
            k += 1;
        }
        return pos;
    }
}
