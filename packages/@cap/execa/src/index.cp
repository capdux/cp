import { RunPathOptions, buildRunPath } from "../../npm-run-path/src/index.cp";

pub struct ExecaOptions {
    pub cwd: string,
    pub path_env: string,
    pub prefer_local: bool,
    pub reject: bool,
    pub strip_final_newline: bool,
    pub timeout_ms: u64,
    pub is_windows: bool,

    pub static fn defaults(cwd: string, path_env: string): ExecaOptions {
        return ExecaOptions {
            cwd: cwd,
            path_env: path_env,
            prefer_local: true,
            reject: true,
            strip_final_newline: true,
            timeout_ms: 0,
            is_windows: false,
        };
    }
}

pub struct ExecaResult {
    pub exit_code: i32,
    pub signal_code: i32,
    pub timed_out: bool,
    pub is_canceled: bool,
    pub failed: bool,
    pub stdout: string,
    pub stderr: string,

    pub static fn success(stdout: string): ExecaResult {
        return ExecaResult {
            exit_code: 0,
            signal_code: 0,
            timed_out: false,
            is_canceled: false,
            failed: false,
            stdout: stdout,
            stderr: "",
        };
    }

    pub static fn failure(exit_code: i32, stderr: string): ExecaResult {
        return ExecaResult {
            exit_code: exit_code,
            signal_code: 0,
            timed_out: false,
            is_canceled: false,
            failed: exit_code != 0,
            stdout: "",
            stderr: stderr,
        };
    }

    pub static fn timeout(): ExecaResult {
        return ExecaResult {
            exit_code: -1,
            signal_code: 15,
            timed_out: true,
            is_canceled: false,
            failed: true,
            stdout: "",
            stderr: "Command timed out",
        };
    }

    pub fn isOk(): bool {
        return !_.failed && !_.timed_out && !_.is_canceled && _.exit_code == 0;
    }
}

fn exaPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn exaPushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) { return pos; }
    out[pos] = b;
    return pos + 1;
}

fn exaNeedsQuote(arg: string): bool {
    if (arg.len == 0) { return true; }
    let i: usize = 0;
    while (i < arg.len) {
        let c = arg[i];
        let safe = (c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) ||
            c == 45 || c == 95 || c == 46 || c == 47 || c == 58 || c == 61 || c == 43;
        if (!safe) { return true; }
        i = i + 1;
    }
    return false;
}

pub fn escapePosixArg(arg: string, out: []u8): usize {
    if (!exaNeedsQuote(arg)) {
        return exaPush(out, 0, arg);
    }
    let p: usize = 0;
    p = exaPushByte(out, p, 39); // '\''
    let i: usize = 0;
    while (i < arg.len) {
        if (arg[i] == 39) {
            p = exaPush(out, p, "'\\''");
        } else {
            p = exaPushByte(out, p, arg[i]);
        }
        i = i + 1;
    }
    p = exaPushByte(out, p, 39);
    return p;
}

pub fn formatCommand(file: string, args: []string, out: []u8): usize {
    let p: usize = 0;
    let tmp: [256]u8 = [0; 256];
    let fn_len = escapePosixArg(file, tmp[0..256]);
    p = exaPush(out, p, tmp[0..fn_len] as string);
    let i: usize = 0;
    while (i < args.len && p < out.len) {
        p = exaPushByte(out, p, 32); // ' '
        let alen = escapePosixArg(args[i], tmp[0..256]);
        p = exaPush(out, p, tmp[0..alen] as string);
        i = i + 1;
    }
    return p;
}

pub fn stripFinalNewline(s: string): string {
    if (s.len >= 2 && s[s.len - 2] == 13 && s[s.len - 1] == 10) {
        return s[0..(s.len - 2)];
    }
    if (s.len >= 1 && s[s.len - 1] == 10) {
        return s[0..(s.len - 1)];
    }
    return s;
}

pub fn resolveExecPath(opts: ExecaOptions, out: []u8): usize {
    let rpo = RunPathOptions.defaults(opts.cwd, opts.path_env);
    rpo.prefer_local = opts.prefer_local;
    rpo.is_windows = opts.is_windows;
    return buildRunPath(rpo, out);
}
