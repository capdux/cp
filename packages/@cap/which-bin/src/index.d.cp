// Type definitions for CP

import { normalizePath } from "../../normalize-path/src/index.cp";
export fn whEqIgnoreCase(a: string, b: string): bool;
export fn isExplicitPath(cmd: string): bool;
export fn countPathDirs(path_env: string, delim: u8): usize;
export fn pathDirAt(path_env: string, delim: u8, idx: usize, out: []u8): usize;
export fn hasPathExt(cmd: string, pathext: string): bool;
export fn formatCandidatePath(dir: string, cmd: string, ext: string, out: []u8): usize;
export class WhichIterator {
    cmd: string,
    path_env: string,
    pathext: string,
    delim: u8,
    dir_idx: usize,
    dir_count: usize,
    ext_idx: usize,
    ext_count: usize,
    explicit_done: bool,
    fn newPosix(cmd: string, path_env: string): WhichIterator;
    fn newWindows(cmd: string, path_env: string, pathext: string): WhichIterator;
    fn next(_: auto, out: []u8): usize;
}

