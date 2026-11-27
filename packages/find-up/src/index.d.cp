// Type definitions for CP

import { dirnameOf } from "../../npm-run-path/src/index.cp";
export fn fupStrEq(a: string, b: string): bool;
export fn fupTrimTrailingSlash(s: string): string;
export fn fupPush(out: []u8, pos: usize, s: string): usize;
export fn joinDirAndName(dir: string, name: string, out: []u8): usize;
export class FindUpOptions {
    cwd: string,
    stop_at: string,
    allow_symlinks: bool,
    fn defaults(cwd: string): FindUpOptions;
}

export class FindUpWalker {
    current: string,
    stop_at: string,
    done: bool,
    steps: usize,
    fn new(opts: FindUpOptions): FindUpWalker;
    fn nextDir(_: auto): string?;
    fn nextCandidate(_: auto, filename: string, out: []u8): usize?;
}

export fn findUpInList(opts: FindUpOptions, filename: string, existing_paths: []string, out: []u8): usize?;
export fn findUpMultipleInList(opts: FindUpOptions, filenames: []string, existing_paths: []string, out: []u8): usize?;
