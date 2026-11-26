// Type definitions for CP

export class ModuleDetails {
    name: string,
    basedir: string,
    subpath: string,
    found: bool,
}

export fn mdfpIsSep(ch: u8): bool;
export fn mdfpMatchDir(s: string, pos: usize, dir: string): bool;
export fn mdfpSkipSeps(s: string, start: usize): usize;
export fn mdfpFindSep(s: string, start: usize): usize;
export fn mdfpEmpty(): ModuleDetails;
export fn parseModulePath(file_path: string): ModuleDetails;
