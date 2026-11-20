// Source: /data/dev/bun/src/shell/EnvMap.cp
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/EnvMap.cp & interpreter.cp
// ============================================================================
// lib/shell/env.cp
// In-process Environment Table, Directory Stack, Shell Options, Aliases & Word Expansion Engine (cap:shell)

const std = @import("std");
import { StringJoiner } from "cap:string";
import { getpid, getcwd, chdir } from "cap:sys";

pub const MAX_ENV_VARS: usize = 256;
pub const MAX_DIR_STACK: usize = 32;
pub const MAX_POS_ARGS: usize = 32;
pub const MAX_ALIASES: usize = 32;

pub enum EnvSlotState: u8 {
    Empty = 0,
    Set = 1,
    Unset = 2,
}

pub struct EnvSlot {
    pub key: []const u8 = "",
    pub val: []const u8 = "",
    pub exported: bool = true,
    pub state: EnvSlotState = .Empty,
}

pub struct AliasSlot {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
    pub active: bool = false,
}

pub struct ShellOptions {
    pub errexit: bool = false,  // set -e
    pub pipefail: bool = false, // set -o pipefail
    pub nounset: bool = false,  // set -u
    pub xtrace: bool = false,   // set -x
}

fn allocCopyBytes(s: []const u8): []const u8 {
    if (s.len == 0) {
        return "";
    }
    let out: []const u8 = "";
    zig {
        const mem = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
        @memcpy(mem, s);
        out = mem;
    }
    return out;
}

fn freeCopyBytes(s: []const u8) {
    if (s.len == 0) {
        return;
    }
    zig {
        std.heap.page_allocator.free(s);
    }
}

pub struct ShellEnv {
    pub entries: [MAX_ENV_VARS]EnvSlot = undefined,
    pub len: usize = 0,

    pub static fn isValidIdent(name: []const u8): bool {
        if (name.len == 0 || !isIdentStart(name[0])) {
            return false;
        }
        let i: usize = 1;
        while (i < name.len) {
            if (!isIdentCont(name[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub static fn init(): ShellEnv {
        let env = ShellEnv{
            entries: undefined,
            len: 0,
        };
        let i: usize = 0;
        while (i < MAX_ENV_VARS) {
            env.entries[i] = EnvSlot{};
            i += 1;
        }
        return env;
    }

    pub fn clone(): ShellEnv {
        let copy = ShellEnv.init();
        let i: usize = 0;
        while (i < _.len) {
            const slot = _.entries[i];
            if (slot.state == .Set) {
                * = copy.set(slot.key, slot.val, slot.exported);
            } else if (slot.state == .Unset) {
                * = copy.unset(slot.key);
            }
            i += 1;
        }
        return copy;
    }

    fn findSlot(key: []const u8): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].state != .Empty && std.mem.eql(u8, _.entries[i].key, key)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    pub fn set(*_, key: []const u8, val: []const u8, exported: bool): bool {
        if (key.len == 0) {
            return false;
        }
        if (idx := _.findSlot(key)) {
            freeCopyBytes(_.entries[idx].val);
            _.entries[idx].val = allocCopyBytes(val);
            _.entries[idx].exported = _.entries[idx].exported || exported;
            _.entries[idx].state = .Set;
            return true;
        }
        if (_.len >= MAX_ENV_VARS) {
            return false;
        }
        _.entries[_.len] = EnvSlot{
            key: allocCopyBytes(key),
            val: allocCopyBytes(val),
            exported: exported,
            state: .Set,
        };
        _.len += 1;
        return true;
    }

    pub fn unset(*_, key: []const u8): bool {
        if (key.len == 0) {
            return false;
        }
        if (idx := _.findSlot(key)) {
            freeCopyBytes(_.entries[idx].val);
            _.entries[idx].val = "";
            _.entries[idx].state = .Unset;
            return true;
        }
        if (_.len >= MAX_ENV_VARS) {
            return false;
        }
        _.entries[_.len] = EnvSlot{
            key: allocCopyBytes(key),
            val: "",
            exported: false,
            state: .Unset,
        };
        _.len += 1;
        return true;
    }

    pub fn get(key: []const u8): ([]const u8)? {
        if (key.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].state != .Empty && std.mem.eql(u8, _.entries[i].key, key)) {
                if (_.entries[i].state == .Set) {
                    return _.entries[i].val;
                }
                return null;
            }
            i += 1;
        }
        // Fallback to OS process environment
        if (key.len < 256) {
            let key_buf: [256]u8 = [0; 256];
            @memcpy(key_buf[0..key.len], key);
            key_buf[key.len] = 0;
            let result: ([]const u8)? = null;
            zig {
                const c_key: [*:0]const u8 = key_buf[0..key.len :0].ptr;
                if (std.c.getenv(c_key)) |raw_val| {
                    result = std.mem.span(raw_val);
                }
            }
            return result;
        }
        return null;
    }

    pub fn isExplicitlyUnset(key: []const u8): bool {
        if (idx := _.findSlot(key)) {
            return _.entries[idx].state == .Unset;
        }
        return false;
    }

    pub fn contains(key: []const u8): bool {
        return _.get(key) != null;
    }

    pub fn count(): usize {
        let c: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].state == .Set) {
                c += 1;
            }
            i += 1;
        }
        return c;
    }

    /// Build a null-terminated-entry envp slice (`KEY=VAL`) merging OS environ and ShellEnv overrides.
    pub fn buildEnvp(allocator: std.mem.Allocator): [][]const u8 {
        let list: std.ArrayListUnmanaged(string) = .empty;
        // 1. Copy OS environ entries unless overridden or unset in ShellEnv
        let env_idx: usize = 0;
        while (true) {
            let entry_opt: ([]const u8)? = null;
            zig {
                if (std.c.environ[env_idx]) |c_entry| {
                    entry_opt = std.mem.span(c_entry);
                }
            }
            const entry_slice = entry_opt orelse break;
            if (eq_idx := std.mem.indexOfScalar(u8, entry_slice, '=')) {
                const k = entry_slice[0..eq_idx];
                if (_.findSlot(k) == null) {
                    const duped = allocator.dupe(u8, entry_slice) catch {
                        env_idx += 1;
                        continue;
                    };
                    list.append(allocator, duped) catch {};
                }
            }
            env_idx += 1;
        }
        // 2. Append all Set & exported ShellEnv entries
        let i: usize = 0;
        while (i < _.len) {
            const slot = _.entries[i];
            if (slot.state == .Set && slot.exported) {
                const kv = std.fmt.allocPrint(allocator, "{s}={s}", .{ slot.key, slot.val }) catch "";
                if (kv.len > 0) {
                    list.append(allocator, kv) catch {};
                }
            }
            i += 1;
        }
        return list.toOwnedSlice(allocator) catch &.{};
    }

    pub static fn freeEnvp(allocator: std.mem.Allocator, envp: [][]const u8) {
        for (item of envp) {
            allocator.free(item);
        }
        allocator.free(envp);
    }

    pub fn release(*_) {
        let i: usize = 0;
        while (i < _.len) {
            freeCopyBytes(_.entries[i].key);
            freeCopyBytes(_.entries[i].val);
            _.entries[i] = EnvSlot{};
            i += 1;
        }
        _.len = 0;
    }
}

pub struct AliasMap {
    pub slots: [MAX_ALIASES]AliasSlot = undefined,
    pub len: usize = 0,

    pub static fn init(): AliasMap {
        let m = AliasMap{ slots: undefined, len: 0 };
        let i: usize = 0;
        while (i < MAX_ALIASES) {
            m.slots[i] = AliasSlot{};
            i += 1;
        }
        return m;
    }

    pub fn clone(): AliasMap {
        let m = AliasMap.init();
        let i: usize = 0;
        while (i < _.len) {
            if (_.slots[i].active) {
                * = m.set(_.slots[i].name, _.slots[i].value);
            }
            i += 1;
        }
        return m;
    }

    pub fn set(*_, name: []const u8, val: []const u8): bool {
        if (name.len == 0) return false;
        let i: usize = 0;
        while (i < _.len) {
            if (std.mem.eql(u8, _.slots[i].name, name)) {
                freeCopyBytes(_.slots[i].value);
                _.slots[i].value = allocCopyBytes(val);
                _.slots[i].active = true;
                return true;
            }
            i += 1;
        }
        if (_.len >= MAX_ALIASES) return false;
        _.slots[_.len] = AliasSlot{
            name: allocCopyBytes(name),
            value: allocCopyBytes(val),
            active: true,
        };
        _.len += 1;
        return true;
    }

    pub fn unset(*_, name: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].name, name)) {
                _.slots[i].active = false;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn get(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].name, name)) {
                return _.slots[i].value;
            }
            i += 1;
        }
        return null;
    }

    pub fn release(*_) {
        let i: usize = 0;
        while (i < _.len) {
            freeCopyBytes(_.slots[i].name);
            freeCopyBytes(_.slots[i].value);
            _.slots[i] = AliasSlot{};
            i += 1;
        }
        _.len = 0;
    }
}

pub struct DirStack {
    cwd_buf: [1024]u8 = [0; 1024],
    cwd_len: usize = 0,
    stack: [MAX_DIR_STACK][512]u8 = undefined,
    stack_lens: [MAX_DIR_STACK]usize = [0; MAX_DIR_STACK],
    pub stack_len: usize = 0,

    pub static fn init(): DirStack {
        let ds = DirStack{
            cwd_buf: [0; 1024],
            cwd_len: 0,
            stack: undefined,
            stack_lens: [0; MAX_DIR_STACK],
            stack_len: 0,
        };
        ds.refreshCwd();
        return ds;
    }

    pub fn clone(): DirStack {
        let ds = DirStack{
            cwd_buf: _.cwd_buf,
            cwd_len: _.cwd_len,
            stack: _.stack,
            stack_lens: _.stack_lens,
            stack_len: _.stack_len,
        };
        return ds;
    }

    pub fn refreshCwd(*_) {
        const cur = getcwd(&_.cwd_buf) catch "";
        if (cur.len > 0) {
            _.cwd_len = cur.len;
        } else {
            _.cwd_buf[0] = '/';
            _.cwd_len = 1;
        }
    }

    pub fn cwd(): []const u8 {
        if (_.cwd_len == 0) {
            return "/";
        }
        return _.cwd_buf[0.._.cwd_len];
    }

    pub fn chdirTo(*_, target: []const u8, env: *ShellEnv): bool {
        let resolved_target = target;
        if (resolved_target.len == 0) {
            if (home := env.get("HOME")) {
                resolved_target = home;
            } else {
                resolved_target = "/";
            }
        } else if (std.mem.eql(u8, resolved_target, "-")) {
            if (old := env.get("OLDPWD")) {
                resolved_target = old;
            } else {
                return false;
            }
        }

        if (resolved_target.len >= 1024) {
            return false;
        }

        let prev_buf: [1024]u8 = [0; 1024];
        const prev_len = _.cwd_len;
        if (prev_len > 0) {
            @memcpy(prev_buf[0..prev_len], _.cwd_buf[0..prev_len]);
        }

        chdir(resolved_target) catch { return false; };

        * = env.set("OLDPWD", prev_buf[0..prev_len], true);
        _.refreshCwd();
        * = env.set("PWD", _.cwd(), true);
        return true;
    }

    pub fn pushd(*_, target: []const u8, env: *ShellEnv): bool {
        if (_.stack_len >= MAX_DIR_STACK || _.cwd_len >= 512) {
            return false;
        }
        let prev_buf: [512]u8 = [0; 512];
        const prev_len = _.cwd_len;
        @memcpy(prev_buf[0..prev_len], _.cwd_buf[0..prev_len]);

        if (!_.chdirTo(target, env)) {
            return false;
        }

        @memcpy(_.stack[_.stack_len][0..prev_len], prev_buf[0..prev_len]);
        _.stack_lens[_.stack_len] = prev_len;
        _.stack_len += 1;
        return true;
    }

    pub fn popd(*_, env: *ShellEnv): ([]const u8)? {
        if (_.stack_len == 0) {
            return null;
        }
        _.stack_len -= 1;
        const top_len = _.stack_lens[_.stack_len];
        const top_slice = _.stack[_.stack_len][0..top_len];
        if (!_.chdirTo(top_slice, env)) {
            _.stack_len += 1;
            return null;
        }
        return _.cwd();
    }

    pub fn depth(): usize {
        return _.stack_len;
    }
}

pub fn isShellIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

pub fn isShellIdentCont(c: u8): bool {
    return isShellIdentStart(c) || (c >= '0' && c <= '9');
}

pub fn parseShellInt64(s: []const u8): i64? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(i64, s, 10) catch null;
}

pub fn pushShellI64(out: *StringJoiner, val: i64) {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "0";
    out.push(s);
}

pub fn pushShellUsize(out: *StringJoiner, val: usize) {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "0";
    out.push(s);
}

fn isIdentStart(c: u8): bool {
    return isShellIdentStart(c);
}

fn isIdentCont(c: u8): bool {
    return isShellIdentCont(c);
}

fn pushI32(out: *StringJoiner, val: i32) {
    pushShellI64(out, val as i64);
}

fn pushUsize(out: *StringJoiner, val: usize) {
    pushShellUsize(out, val);
}

fn expandDollar(
    raw: []const u8,
    start_idx: usize,
    env: *ShellEnv,
    last_exit: i32,
    out: *StringJoiner,
): usize {
    // start_idx points at '$'
    let i = start_idx + 1;
    if (i >= raw.len) {
        out.push("$");
        return i;
    }

    const next = raw[i];
    if (next == '?') {
        pushI32(out, last_exit);
        return i + 1;
    }
    if (next == '$') {
        pushI32(out, getpid());
        return i + 1;
    }

    if (next == '{') {
        i += 1;
        const brace_start = i;
        let depth: usize = 1;
        while (i < raw.len && depth > 0) {
            if (raw[i] == '\\' && i + 1 < raw.len) {
                i += 2;
            } else if (raw[i] == '{') {
                depth += 1;
                i += 1;
            } else if (raw[i] == '}') {
                depth -= 1;
                if (depth == 0) {
                    break;
                }
                i += 1;
            } else {
                i += 1;
            }
        }
        const inner = raw[brace_start..i];
        if (i < raw.len && raw[i] == '}') {
            i += 1;
        }
        expandBraceBody(inner, env, last_exit, out);
        return i;
    }

    if (isIdentStart(next)) {
        const name_start = i;
        i += 1;
        while (i < raw.len && isIdentCont(raw[i])) {
            i += 1;
        }
        const var_name = raw[name_start..i];
        if (val := env.get(var_name)) {
            out.push(val);
        }
        return i;
    }

    out.push("$");
    return start_idx + 1;
}

pub fn expandBraceBody(
    inner: []const u8,
    env: *ShellEnv,
    last_exit: i32,
    out: *StringJoiner,
) {
    if (inner.len == 0) {
        return;
    }

    // ${#VAR} length operator
    if (inner[0] == '#' && inner.len > 1) {
        const vname = inner[1..];
        if (val := env.get(vname)) {
            pushUsize(out, val.len);
        } else {
            out.push("0");
        }
        return;
    }

    // Find parameter name end
    let name_end: usize = 0;
    while (name_end < inner.len && isIdentCont(inner[name_end])) {
        name_end += 1;
    }
    const var_name = inner[0..name_end];
    const rest = inner[name_end..];

    if (rest.len == 0) {
        if (val := env.get(var_name)) {
            out.push(val);
        }
        return;
    }

    // Handle :-, :=, :+, :?, -, =, +, ?
    let colon = false;
    let op_pos: usize = 0;
    if (rest[0] == ':' && rest.len >= 2) {
        colon = true;
        op_pos = 1;
    }
    const op_ch = rest[op_pos];
    const arg = rest[op_pos + 1 ..];

    const existing_opt = env.get(var_name);
    let is_set = false;
    if (v := existing_opt) {
        is_set = !colon || v.len > 0;
    }

    if (op_ch == '-') {
        if (is_set) {
            out.push(existing_opt.?);
        } else {
            let sub = StringJoiner.init();
            defer sub.release();
            expandWord(arg, env, last_exit, &sub);
            out.push(sub.slice());
        }
        return;
    }

    if (op_ch == '=') {
        if (is_set) {
            out.push(existing_opt.?);
        } else {
            let sub = StringJoiner.init();
            defer sub.release();
            expandWord(arg, env, last_exit, &sub);
            const expanded = sub.slice();
            * = env.set(var_name, expanded, false);
            out.push(expanded);
        }
        return;
    }

    if (op_ch == '+') {
        if (is_set) {
            let sub = StringJoiner.init();
            defer sub.release();
            expandWord(arg, env, last_exit, &sub);
            out.push(sub.slice());
        }
        return;
    }

    if (val := existing_opt) {
        out.push(val);
    }
}

/// Expands quotes, backslash escapes, and `$VAR` / `${VAR}` / `${VAR:-default}` into `out`.
pub fn expandWord(
    raw: []const u8,
    env: *ShellEnv,
    last_exit: i32,
    out: *StringJoiner,
) {
    if (raw.len == 0) {
        return;
    }

    // Leading tilde expansion when unquoted (~ or ~/...)
    let i: usize = 0;
    if (raw[0] == '~' && (raw.len == 1 || raw[1] == '/')) {
        if (home := env.get("HOME")) {
            out.push(home);
            i = 1;
        }
    }

    let in_single = false;
    let in_double = false;

    while (i < raw.len) {
        const ch = raw[i];
        if (in_single) {
            if (ch == '\'') {
                in_single = false;
            } else {
                out.push(raw[i .. i + 1]);
            }
            i += 1;
        } else if (in_double) {
            if (ch == '"') {
                in_double = false;
                i += 1;
            } else if (ch == '\\' && i + 1 < raw.len) {
                const esc = raw[i + 1];
                if (esc == '"' || esc == '\\' || esc == '$' || esc == '`') {
                    out.push(raw[i + 1 .. i + 2]);
                    i += 2;
                } else if (esc == '\n') {
                    i += 2;
                } else {
                    out.push(raw[i .. i + 2]);
                    i += 2;
                }
            } else if (ch == '$') {
                i = expandDollar(raw, i, env, last_exit, out);
            } else {
                out.push(raw[i .. i + 1]);
                i += 1;
            }
        } else {
            if (ch == '\'') {
                in_single = true;
                i += 1;
            } else if (ch == '"') {
                in_double = true;
                i += 1;
            } else if (ch == '$' && i + 1 < raw.len && raw[i + 1] == '\'') {
                // ANSI-C $'...' quoting
                i += 2;
                while (i < raw.len && raw[i] != '\'') {
                    if (raw[i] == '\\' && i + 1 < raw.len) {
                        const esc = raw[i + 1];
                        if (esc == 'n') { out.push("\n"); i += 2; }
                        else if (esc == 't') { out.push("\t"); i += 2; }
                        else if (esc == 'r') { out.push("\r"); i += 2; }
                        else if (esc == '\\') { out.push("\\"); i += 2; }
                        else if (esc == '\'') { out.push("'"); i += 2; }
                        else { out.push(raw[i + 1 .. i + 2]); i += 2; }
                    } else {
                        out.push(raw[i .. i + 1]);
                        i += 1;
                    }
                }
                if (i < raw.len && raw[i] == '\'') i += 1;
            } else if (ch == '\\' && i + 1 < raw.len) {
                if (raw[i + 1] != '\n') {
                    out.push(raw[i + 1 .. i + 2]);
                }
                i += 2;
            } else if (ch == '$') {
                i = expandDollar(raw, i, env, last_exit, out);
            } else {
                out.push(raw[i .. i + 1]);
                i += 1;
            }
        }
    }
}
