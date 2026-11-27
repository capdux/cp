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
export class DirPathValidation {
    valid: bool,
    err_code: i32,
    bad_index: usize,
}

export fn validateDirPath(path: string, is_windows: bool): DirPathValidation;
export fn mkdIsAlpha(c: u8): bool;
export fn normalizeDirPath(path: string, is_windows: bool, out: []u8): usize;
export fn makeDirPlan(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan;
export class ParentDirIter {
    path: string,
    cursor: usize,
    done: bool,
    fn next(_: auto): string?;
}

export fn iterParentDirs(path: string): ParentDirIter;
export fn collectParentDirs(path: string, out: []string): usize;
