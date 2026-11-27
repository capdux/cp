// Type definitions for CP

import { ENTRY_ANY, ENTRY_FILE, ENTRY_DIR, LocateOptions, FsEntry, locatePathInEntries, locateAllPathsInEntries } from "../../locate-path/src/index.cp";
export fn pathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
export fn fileExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
export fn dirExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
export fn realPathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
export fn countExistingPaths(targets: []string, cwd: string, entries: []FsEntry, out_indices: []usize): usize;
