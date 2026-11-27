// Type definitions for CP

export class RunPathOptions {
    cwd: string,
    path_env: string,
    exec_path: string,
    prefer_local: bool,
    add_exec_path: bool,
    is_windows: bool,
    bin_suffix: string,
    fn defaults(cwd: string, path_env: string): RunPathOptions;
    fn forCap(cwd: string, path_env: string): RunPathOptions;
}

export fn nrpPushBytes(out: []u8, pos: usize, s: string): usize;
export fn nrpPushChar(out: []u8, pos: usize, c: u8): usize;
export fn nrpIsSep(c: u8, is_windows: bool): bool;
export fn dirnameOf(path: string, is_windows: bool): string;
export fn countAncestorDirs(cwd: string, is_windows: bool): usize;
export fn buildRunPath(opts: RunPathOptions, out: []u8): usize;
export fn pathKey(is_windows: bool): string;
