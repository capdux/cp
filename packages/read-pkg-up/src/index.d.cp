// Type definitions for CP

import { FindUpOptions, FindUpWalker } from "../../find-up/src/index.cp";
import { ReadPkgResult, parsePackageJson } from "../../read-pkg/src/index.cp";

export class PkgFileEntry {
    path: string,
    content: string,
}

export class ReadPkgUpOptions {
    cwd: string,
    stop_at: string,
    normalize: bool,
    prefer_cap_json: bool,
    static fn defaults(cwd: string): ReadPkgUpOptions;
}

export class PkgQuickMeta {
    name: string,
    version: string,
    description: string,
    has_name: bool,
    has_version: bool,
    has_description: bool,
}

export class ReadPkgUpResult {
    found: bool,
    pkg_path_len: usize,
    pkg: ReadPkgResult,
}

export fn parentDir(path: string): string?;
export fn joinPath(dir: string, name: string, out: []u8): usize;
export fn extractPkgStringField(json: string, key: string): string?;
export fn parseQuickPkgMeta(json: string): PkgQuickMeta;
export fn readPackageUpNamed(cwd: string, stop_at: string, filename: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;
export fn readPackageUpInEntries(cwd: string, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;
export fn readPackageUpWithOptions(opts: ReadPkgUpOptions, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;
