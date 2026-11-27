// Type definitions for CP

import { joinDirAndName } from "../../find-up/src/index.cp";
export class LocateOptions {
    cwd: string,
    entry_type: u8,
    allow_symlinks: bool,
    preserve_order: bool,
    fn defaults(cwd: string): LocateOptions;
}

export class FsEntry {
    path: string,
    kind: u8,
    is_symlink: bool,
}

export fn lcpStrEq(a: string, b: string): bool;
export fn lcpTrimTrailingSlash(s: string): string;
export fn lcpCopyStr(dst: []u8, src: string): usize;
export fn isAbsolutePath(p: string): bool;
export fn lcpResolveCandidate(cwd: string, cand: string, out: []u8): usize;
export fn lcpEntryMatches(entry: FsEntry, target: string, opts: LocateOptions): bool;
export fn locatePathInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, out: []u8): usize?;
export fn locateAllPathsInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, matched_indices: []usize): usize;
