// Type definitions for CP

import { FindUpOptions, FindUpWalker } from "../../find-up/src/index.cp";
import { ReadPkgResult, parsePackageJson } from "../../read-pkg/src/index.cp";
export class PkgFileEntry {
    path: string,
    content: string,
}

export class ReadPkgUpResult {
    found: bool,
    pkg_path_len: usize,
    pkg: ReadPkgResult,
}

export fn rpuStrEq(a: string, b: string): bool;
export fn rpuCopyBytes(out: []u8, src: string): usize;
export fn readPackageUpInEntries(cwd: string, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;
