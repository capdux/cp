// Type definitions for CP

export class MakeDirOptions {
    mode: u32,
    is_windows: bool,
    fn defaults(): MakeDirOptions;
}

export fn isValidDirPath(path: string, is_windows: bool): bool;
export fn mkdStrEq(a: string, b: string): bool;
export fn mkdExists(p: string, existing: []string): bool;
export class MakeDirPlan {
    valid: bool,
    mode: u32,
    dirs: [32]string,
    count: usize,
    fn empty(valid: bool, mode: u32): MakeDirPlan;
}

export fn planMakeDir(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan;
export fn applyUmask(mode: u32, umask: u32): u32;
