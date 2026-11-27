// Type definitions for CP

import { RunPathOptions, buildRunPath } from "../../npm-run-path/src/index.cp";
export class ExecaOptions {
    cwd: string,
    path_env: string,
    prefer_local: bool,
    reject: bool,
    strip_final_newline: bool,
    timeout_ms: u64,
    is_windows: bool,
    fn defaults(cwd: string, path_env: string): ExecaOptions;
}

export class ExecaResult {
    exit_code: i32,
    signal_code: i32,
    timed_out: bool,
    is_canceled: bool,
    failed: bool,
    stdout: string,
    stderr: string,
    fn success(stdout: string): ExecaResult;
    fn failure(exit_code: i32, stderr: string): ExecaResult;
    fn timeout(): ExecaResult;
    fn isOk(): bool;
}

export fn exaPush(out: []u8, pos: usize, s: string): usize;
export fn exaPushByte(out: []u8, pos: usize, b: u8): usize;
export fn exaNeedsQuote(arg: string): bool;
export fn escapePosixArg(arg: string, out: []u8): usize;
export fn formatCommand(file: string, args: []string, out: []u8): usize;
export fn stripFinalNewline(s: string): string;
export fn resolveExecPath(opts: ExecaOptions, out: []u8): usize;
